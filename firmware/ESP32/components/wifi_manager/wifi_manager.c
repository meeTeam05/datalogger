/**
 * @file wifi_manager.c
 * @brief WiFi Manager Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <string.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "WIFI_MANAGER";

/* FreeRTOS event group to signal when we are connected */
static EventGroupHandle_t s_wifi_event_group;

/* DEFINES ------------------------------------------------------------------*/

/* Event bits */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_BOTH_BITS (WIFI_CONNECTED_BIT | WIFI_FAIL_BIT)

/* TYPEDEFS ------------------------------------------------------------------*/

/* WiFi Manager state */
static struct
{
    wifi_manager_config_t config; /*!< Configuration */
    wifi_state_t state;           /*!< Current WiFi state */
    uint8_t retry_count;          /*!< Current retry count */
    esp_netif_t *netif;           /*!< Network interface */
    bool initialized;             /*!< Initialization state */
} s_wifi_manager = {0};

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Get auth mode from Kconfig
 *
 * @return wifi_auth_mode_t Auth mode
 *
 * @details Maps Kconfig settings to wifi_auth_mode_t enum values.
 *          Defaults to WIFI_AUTH_WPA2_PSK if no match found.
 */
static wifi_auth_mode_t get_auth_mode_from_kconfig(void)
{
#if defined(CONFIG_WIFI_AUTH_OPEN)
    return WIFI_AUTH_OPEN;
#elif defined(CONFIG_WIFI_AUTH_WEP)
    return WIFI_AUTH_WEP;
#elif defined(CONFIG_WIFI_AUTH_WPA_PSK)
    return WIFI_AUTH_WPA_PSK;
#elif defined(CONFIG_WIFI_AUTH_WPA2_PSK)
    return WIFI_AUTH_WPA2_PSK;
#elif defined(CONFIG_WIFI_AUTH_WPA_WPA2_PSK)
    return WIFI_AUTH_WPA_WPA2_PSK;
#elif defined(CONFIG_WIFI_AUTH_WPA3_PSK)
    return WIFI_AUTH_WPA3_PSK;
#elif defined(CONFIG_WIFI_AUTH_WPA2_WPA3_PSK)
    return WIFI_AUTH_WPA2_WPA3_PSK;
#else
    return WIFI_AUTH_WPA2_PSK; // Default
#endif
}

/**
 * @brief Get power save mode from Kconfig
 *
 * @return wifi_ps_type_t Power save mode
 *
 * @details Maps Kconfig settings to wifi_ps_type_t enum values.
 *          Defaults to WIFI_PS_NONE if power save is disabled.
 */
static wifi_ps_type_t get_power_save_mode(void)
{
#ifdef CONFIG_WIFI_POWER_SAVE
#ifdef CONFIG_WIFI_POWER_SAVE_MAX_MODEM
    return WIFI_PS_MAX_MODEM;
#else
    return WIFI_PS_MIN_MODEM;
#endif
#else
    return WIFI_PS_NONE;
#endif
}

/**
 * @brief Update WiFi state and notify via callback
 *
 * @param new_state New WiFi state
 *
 * @details Updates the internal state and invokes the user-defined
 *          callback if registered.
 */
static void update_state(wifi_state_t new_state)
{
    if (s_wifi_manager.state != new_state)
    {
        s_wifi_manager.state = new_state;
        ESP_LOGI(TAG, "WiFi state changed: %d", new_state);

        if (s_wifi_manager.config.event_callback)
        {
            s_wifi_manager.config.event_callback(new_state,
                                                 s_wifi_manager.config.callback_arg);
        }
    }
}

/**
 * @brief WiFi event handler
 *
 * @param arg User argument
 * @param event_base Event base
 * @param event_id Event ID
 * @param event_data Event data
 *
 * @details Handles WiFi and IP events, manages connection retries,
 *          and updates state accordingly.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi station started");
            update_state(WIFI_STATE_CONNECTING);
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t *disconnected =
                (wifi_event_sta_disconnected_t *)event_data;
            ESP_LOGW(TAG, "WiFi disconnected, reason: %d", disconnected->reason);

            if (s_wifi_manager.retry_count < s_wifi_manager.config.maximum_retry)
            {
                esp_wifi_connect();
                s_wifi_manager.retry_count++;
                ESP_LOGI(TAG, "Retry connecting to AP, attempt %d/%d",
                         s_wifi_manager.retry_count,
                         s_wifi_manager.config.maximum_retry);
                update_state(WIFI_STATE_CONNECTING);
            }
            else
            {
                ESP_LOGE(TAG, "Failed to connect to AP after %d attempts",
                         s_wifi_manager.config.maximum_retry);
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                update_state(WIFI_STATE_FAILED);
            }
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected to AP");
            s_wifi_manager.retry_count = 0;
            break;

        default:
            break;
        }
    }
}

/**
 * @brief IP event handler
 *
 * @param arg User argument
 * @param event_base Event base
 * @param event_id Event ID
 * @param event_data Event data
 *
 * @details Handles IP events, updates state on IP acquisition or loss.
 */
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG, "Got IPv4 address: " IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            update_state(WIFI_STATE_CONNECTED);
            break;
        }

        case IP_EVENT_STA_LOST_IP:
            ESP_LOGW(TAG, "Lost IP address");
            update_state(WIFI_STATE_DISCONNECTED);
            break;

#ifdef CONFIG_WIFI_ENABLE_IPV6
        case IP_EVENT_GOT_IP6:
        {
            ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
            ESP_LOGI(TAG, "Got IPv6 address: " IPV6STR, IPV62STR(event->ip6_info.ip));
            break;
        }
#endif

        default:
            break;
        }
    }
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Get default WiFi Manager configuration from Kconfig
 *
 * @return Default configuration structure
 *
 * @details Populates a wifi_manager_config_t structure with default values
 *          defined in Kconfig. This includes SSID, password, retry limits,
 *          scan methods, power save settings, and more.
 */
wifi_manager_config_t wifi_manager_get_default_config(void)
{
    wifi_manager_config_t config = {
        .ssid = CONFIG_WIFI_SSID,
        .password = CONFIG_WIFI_PASSWORD,
        .maximum_retry = CONFIG_WIFI_MAXIMUM_RETRY,
        .scan_method = CONFIG_WIFI_SCAN_METHOD,
        .sort_method = CONFIG_WIFI_CONNECT_AP_SORT_METHOD,
        .rssi_threshold = CONFIG_WIFI_SCAN_RSSI_THRESHOLD,
        .auth_mode_threshold = get_auth_mode_from_kconfig(),
        .listen_interval = CONFIG_WIFI_LISTEN_INTERVAL,
        .power_save_enabled =
#ifdef CONFIG_WIFI_POWER_SAVE
            true,
#else
            false,
#endif
        .power_save_mode = get_power_save_mode(),
        .ipv6_enabled =
#ifdef CONFIG_WIFI_ENABLE_IPV6
            true,
#else
            false,
#endif
        .connection_timeout_ms = CONFIG_WIFI_CONNECTION_TIMEOUT_MS,
        .event_callback = NULL,
        .callback_arg = NULL};

    return config;
}

/**
 * @brief Initialize WiFi Manager
 *
 * @param config Configuration structure (NULL to use default from Kconfig)
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Initializes the WiFi Manager with the provided configuration.
 *          Sets up WiFi, event handlers, and prepares for connection.
 */
esp_err_t wifi_manager_init(wifi_manager_config_t *config)
{
    if (s_wifi_manager.initialized)
    {
        ESP_LOGW(TAG, "WiFi Manager already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Use provided config or default from Kconfig
    if (config)
    {
        memcpy(&s_wifi_manager.config, config, sizeof(wifi_manager_config_t));
    }
    else
    {
        s_wifi_manager.config = wifi_manager_get_default_config();
    }

    // Validate configuration
    if (!s_wifi_manager.config.ssid || strlen(s_wifi_manager.config.ssid) == 0)
    {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing WiFi Manager");
    ESP_LOGI(TAG, "  SSID: %s", s_wifi_manager.config.ssid);
    ESP_LOGI(TAG, "  Max Retry: %d", s_wifi_manager.config.maximum_retry);
    ESP_LOGI(TAG, "  Power Save: %s",
             s_wifi_manager.config.power_save_enabled ? "enabled" : "disabled");

    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group)
    {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize netif
    ESP_ERROR_CHECK(esp_netif_init());
    s_wifi_manager.netif = esp_netif_create_default_wifi_sta();
    if (!s_wifi_manager.netif)
    {
        ESP_LOGE(TAG, "Failed to create default WiFi station");
        vEventGroupDelete(s_wifi_event_group);
        return ESP_FAIL;
    }

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        ESP_EVENT_ANY_ID,
        &ip_event_handler,
        NULL,
        NULL));

    // Configure WiFi
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, s_wifi_manager.config.ssid,
            sizeof(wifi_config.sta.ssid) - 1);
    if (s_wifi_manager.config.password)
    {
        strncpy((char *)wifi_config.sta.password, s_wifi_manager.config.password,
                sizeof(wifi_config.sta.password) - 1);
    }

    wifi_config.sta.scan_method = s_wifi_manager.config.scan_method;
    wifi_config.sta.sort_method = s_wifi_manager.config.sort_method;
    wifi_config.sta.threshold.rssi = s_wifi_manager.config.rssi_threshold;
    wifi_config.sta.threshold.authmode = s_wifi_manager.config.auth_mode_threshold;
    wifi_config.sta.listen_interval = s_wifi_manager.config.listen_interval;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Set power save mode
    if (s_wifi_manager.config.power_save_enabled)
    {
        ESP_ERROR_CHECK(esp_wifi_set_ps(s_wifi_manager.config.power_save_mode));
        ESP_LOGI(TAG, "WiFi power save enabled: %d",
                 s_wifi_manager.config.power_save_mode);
    }
    else
    {
        ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    }

    s_wifi_manager.state = WIFI_STATE_DISCONNECTED;
    s_wifi_manager.retry_count = 0;
    s_wifi_manager.initialized = true;

    ESP_LOGI(TAG, "WiFi Manager initialized successfully");
    return ESP_OK;
}

/**
 * @brief Connect to WiFi
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Starts the WiFi connection process. Must be called after
 *          wifi_manager_init(). Returns immediately; use
 *          wifi_manager_wait_connected() to block until connected.
 */
esp_err_t wifi_manager_connect(void)
{
    if (!s_wifi_manager.initialized)
    {
        ESP_LOGE(TAG, "WiFi Manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting WiFi connection...");
    s_wifi_manager.retry_count = 0;

    // Clear event bits
    xEventGroupClearBits(s_wifi_event_group, WIFI_BOTH_BITS);

    // Start WiFi
    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

/**
 * @brief Disconnect from WiFi
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Disconnects from the current WiFi network and updates state.
 */
esp_err_t wifi_manager_disconnect(void)
{
    if (!s_wifi_manager.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Disconnecting WiFi...");

    esp_err_t ret = esp_wifi_disconnect();
    if (ret == ESP_OK)
    {
        update_state(WIFI_STATE_DISCONNECTED);
    }

    return ret;
}

/**
 * @brief Get current WiFi connection state
 *
 * @return Current WiFi state
 *
 * @details Returns the current state of the WiFi connection, which can be
 *          one of the values defined in the wifi_state_t enum.
 */
wifi_state_t wifi_manager_get_state(void)
{
    return s_wifi_manager.state;
}

/**
 * @brief Check if WiFi is connected
 *
 * @return true if connected, false otherwise
 *
 * @details Returns true if the WiFi is currently connected to an AP,
 *          false otherwise.
 */
bool wifi_manager_is_connected(void)
{
    return s_wifi_manager.state == WIFI_STATE_CONNECTED;
}

/**
 * @brief Wait for WiFi connection
 *
 * @param timeout_ms Timeout in milliseconds (0 = wait forever)
 *
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT on timeout
 *
 * @details Blocks until the WiFi is connected or the timeout expires.
 */
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms)
{
    if (!s_wifi_manager.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    ESP_LOGI(TAG, "Waiting for WiFi connection (timeout: %lu ms)...", timeout_ms);

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_BOTH_BITS,
        pdFALSE,
        pdFALSE,
        timeout_ticks);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "Connected to AP successfully");
        return ESP_OK;
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Failed to connect to AP");
        return ESP_FAIL;
    }
    else
    {
        ESP_LOGW(TAG, "Connection timeout");
        return ESP_ERR_TIMEOUT;
    }
}

/**
 * @brief Get WiFi RSSI (signal strength)
 *
 * @param[out] rssi Pointer to store RSSI value
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Retrieves the Received Signal Strength Indicator (RSSI) of the
 *          currently connected WiFi network. The RSSI value is stored in the
 *          provided pointer.
 */
esp_err_t wifi_manager_get_rssi(int8_t *rssi)
{
    if (!rssi || !s_wifi_manager.initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_wifi_manager.state != WIFI_STATE_CONNECTED)
    {
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    if (ret == ESP_OK)
    {
        *rssi = ap_info.rssi;
    }

    return ret;
}

/**
 * @brief Get WiFi IP address
 *
 * @param[out] ip_addr Buffer to store IP address string (minimum 16 bytes)
 * @param len Buffer length
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Retrieves the current IP address assigned to the WiFi interface.
 *          The IP address is stored as a string in the provided buffer.
 */
esp_err_t wifi_manager_get_ip_addr(char *ip_addr, size_t len)
{
    if (!ip_addr || len < 16 || !s_wifi_manager.initialized)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_wifi_manager.state != WIFI_STATE_CONNECTED)
    {
        return ESP_ERR_WIFI_NOT_CONNECT;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(s_wifi_manager.netif, &ip_info);
    if (ret == ESP_OK)
    {
        snprintf(ip_addr, len, IPSTR, IP2STR(&ip_info.ip));
    }

    return ret;
}

/**
 * @brief Deinitialize WiFi Manager
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Cleans up resources allocated by the WiFi Manager, stops WiFi,
 *          and unregisters event handlers.
 */
esp_err_t wifi_manager_deinit(void)
{
    if (!s_wifi_manager.initialized)
    {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Deinitializing WiFi Manager...");

    // Stop WiFi
    esp_wifi_stop();

    // Unregister event handlers
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler);

    // Deinit WiFi
    esp_wifi_deinit();

    // Destroy netif
    if (s_wifi_manager.netif)
    {
        esp_netif_destroy(s_wifi_manager.netif);
        s_wifi_manager.netif = NULL;
    }

    // Delete event group
    if (s_wifi_event_group)
    {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }

    s_wifi_manager.initialized = false;
    update_state(WIFI_STATE_DISCONNECTED);

    ESP_LOGI(TAG, "WiFi Manager deinitialized");
    return ESP_OK;
}