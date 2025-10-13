/**
 * @file app_main.c
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_timer.h"

// Include custom libraries
#include "wifi_manager.h"
#include "stm32_uart.h"
#include "relay_control.h"
#include "sht3x_parser.h"

#ifdef CONFIG_ENABLE_MQTT
#include "mqtt_handler.h"
#endif

#ifdef CONFIG_ENABLE_COAP
#include "coap_handler.h"
#endif

/* DEFINES ------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_MQTT
// MQTT Topics
#define TOPIC_SHT3X_COMMAND "esp32/sensor/sht3x/command"
#define TOPIC_SHT3X_SINGLE_TEMPERATURE "esp32/sensor/sht3x/single/temperature"
#define TOPIC_SHT3X_SINGLE_HUMIDITY "esp32/sensor/sht3x/single/humidity"
#define TOPIC_SHT3X_PERIODIC_TEMPERATURE "esp32/sensor/sht3x/periodic/temperature"
#define TOPIC_SHT3X_PERIODIC_HUMIDITY "esp32/sensor/sht3x/periodic/humidity"
#define TOPIC_CONTROL_RELAY "esp32/control/relay"
#define TOPIC_STATE_SYNC "esp32/state"
#endif

/* STATIC VARIABLES ----------------------------------------------------------*/

static const char *TAG = "MQTT_BRIDGE_APP";

#ifdef CONFIG_ENABLE_MQTT
static mqtt_handler_t mqtt_handler;
#endif

#ifdef CONFIG_ENABLE_COAP
// TODO: Uncomment if you have COAP handler
// static coap_handler_t coap_handler;
#endif

// Global components
static stm32_uart_t stm32_uart;
static relay_control_t relay_control;
static sht3x_parser_t sht3x_parser;

// Global state tracking
static bool g_periodic_active = false;
static bool g_device_on = false;
static int g_periodic_rate = 1; // Default 1 Hz

/* STATE SYNCHRONIZATION FUNCTIONS -------------------------------------------*/

/**
 * @brief Create JSON state message
 *
 * @param buffer Buffer to store JSON string
 * @param buffer_size Size of the buffer
 *
 * @details Creates a JSON string representing the current state.
 *          Example: {"device":"ON","periodic":"OFF","rate":1,"timestamp":123456789}
 *          Timestamp is in milliseconds since boot.
 */
static void create_state_message(char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size,
             "{\"device\":\"%s\",\"periodic\":\"%s\",\"rate\":%d,\"timestamp\":%lld}",
             g_device_on ? "ON" : "OFF",
             g_periodic_active ? "ON" : "OFF",
             g_periodic_rate,
             (long long)(esp_timer_get_time() / 1000)); // milliseconds
}

/**
 * @brief Publish current state via MQTT
 *
 * @details Publishes the current state to the TOPIC_STATE_SYNC topic
 *          with retain flag.
 */
static void publish_current_state(void)
{
#ifdef CONFIG_ENABLE_MQTT
    if (!MQTT_Handler_IsConnected(&mqtt_handler))
    {
        return;
    }

    char state_msg[256];
    create_state_message(state_msg, sizeof(state_msg));

    // Publish with retain flag so new clients get latest state
    MQTT_Handler_Publish(&mqtt_handler, TOPIC_STATE_SYNC, state_msg, 0, 1, 1);
    ESP_LOGI(TAG, "State published: %s", state_msg);
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP publish logic here if needed
#endif
}

/**
 * @brief Update global state and publish if changed
 *
 * @param device_on Current device (relay) state
 * @param periodic_active Current periodic reporting state
 * @param rate Current periodic reporting rate
 *
 * @details Updates the global state variables and publishes the new state
 *          if any of the values have changed.
 */
static void update_and_publish_state(bool device_on, bool periodic_active, int rate)
{
    bool state_changed = false;

    if (g_device_on != device_on)
    {
        g_device_on = device_on;
        state_changed = true;
        ESP_LOGI(TAG, "Device state changed: %s", device_on ? "ON" : "OFF");
    }

    if (g_periodic_active != periodic_active)
    {
        g_periodic_active = periodic_active;
        state_changed = true;
        ESP_LOGI(TAG, "Periodic state changed: %s", periodic_active ? "ON" : "OFF");
    }

    if (g_periodic_rate != rate)
    {
        g_periodic_rate = rate;
        state_changed = true;
        ESP_LOGI(TAG, "Periodic rate changed: %d Hz", rate);
    }

    if (state_changed)
    {
        publish_current_state();
    }
}

/* CALLBACK FUNCTIONS --------------------------------------------------------*/

/**
 * @brief Callback when single sht3x data is received
 *
 * @param data Pointer to received sht3x data
 *
 * @details Publishes single measurement data via MQTT if connected.
 */
static void on_single_sht3x_data(const sht3x_data_t *data)
{
    if (!SHT3X_Parser_IsValid(data))
    {
        return;
    }

#ifdef CONFIG_ENABLE_MQTT
    if (MQTT_Handler_IsConnected(&mqtt_handler))
    {
        char temp_str[16], hum_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.2f", data->temperature);
        snprintf(hum_str, sizeof(hum_str), "%.2f", data->humidity);

        MQTT_Handler_Publish(&mqtt_handler, TOPIC_SHT3X_SINGLE_TEMPERATURE, temp_str, 0, 0, 0);
        MQTT_Handler_Publish(&mqtt_handler, TOPIC_SHT3X_SINGLE_HUMIDITY, hum_str, 0, 0, 0);

        ESP_LOGI(TAG, "MQTT SINGLE data: T=%.2f°C, H=%.2f%%",
                 data->temperature, data->humidity);
    }
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP publish logic here if needed
    ESP_LOGI(TAG, "COAP SINGLE data: T=%.2f°C, H=%.2f%%",
             data->temperature, data->humidity);
#endif

#if !(CONFIG_ENABLE_MQTT || CONFIG_ENABLE_COAP)
    ESP_LOGW(TAG, "To receive data, enable MQTT or COAP in configuration");
#endif
}

/**
 * @brief Callback when periodic sht3x data is received
 *
 * @param data Pointer to received sht3x data
 *
 * @details Publishes periodic measurement data via MQTT if connected.
 */
static void on_periodic_sht3x_data(const sht3x_data_t *data)
{
    if (!SHT3X_Parser_IsValid(data))
    {
        return;
    }

#ifdef CONFIG_ENABLE_MQTT
    if (MQTT_Handler_IsConnected(&mqtt_handler))
    {
        char temp_str[16], hum_str[16];
        snprintf(temp_str, sizeof(temp_str), "%.2f", data->temperature);
        snprintf(hum_str, sizeof(hum_str), "%.2f", data->humidity);

        MQTT_Handler_Publish(&mqtt_handler, TOPIC_SHT3X_PERIODIC_TEMPERATURE, temp_str, 0, 0, 0);
        MQTT_Handler_Publish(&mqtt_handler, TOPIC_SHT3X_PERIODIC_HUMIDITY, hum_str, 0, 0, 0);

        ESP_LOGI(TAG, "MQTT PERIODIC data: T=%.2f°C, H=%.2f%%",
                 data->temperature, data->humidity);
    }
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP publish logic here if needed
    ESP_LOGI(TAG, "COAP PERIODIC data: T=%.2f°C, H=%.2f%%",
             data->temperature, data->humidity);
#endif

#if !(CONFIG_ENABLE_MQTT || CONFIG_ENABLE_COAP)
    ESP_LOGW(TAG, "To receive data, enable MQTT or COAP in configuration");
#endif
}

/**
 * @brief Callback when data is received from STM32
 *
 * @param line Received data line
 *
 * @details Forwards received data line to sht3x parser.
 */
static void on_stm32_data_received(const char *line)
{
    ESP_LOGI(TAG, "<- STM32: %s", line);

    // Parse and process sht3x data
    SHT3X_Parser_ProcessLine(&sht3x_parser, line);
}

/**
 * @brief Callback when relay state changes
 *
 * @param state New relay state (true=ON, false=OFF)
 *
 * @details Updates global state and publishes new state.
 */
static void on_relay_state_changed(bool state)
{
    // Update global state
    update_and_publish_state(state, g_periodic_active, g_periodic_rate);

    ESP_LOGI(TAG, "Relay state changed: %s", state ? "ON" : "OFF");
}

/**
 * @brief Extract periodic rate from command string
 *
 * @param command Command string
 *
 * @return Extracted rate or current rate if not found
 *
 * @details Looks for "PERIODIC <rate>" in the command string and extracts the rate.
 *          Returns current rate if not found or invalid.
 */
static int extract_periodic_rate(const char *command)
{
    // Look for rate in command like "SHT3X PERIODIC 2 HIGH"
    char *rate_str = strstr(command, "PERIODIC");
    if (rate_str)
    {
        rate_str += 8; // Skip "PERIODIC"
        while (*rate_str == ' ')
            rate_str++; // Skip spaces

        int rate = atoi(rate_str);
        if (rate > 0 && rate <= 10)
        {
            return rate;
        }
    }
    return g_periodic_rate; // Return current rate if not found
}

#ifdef CONFIG_ENABLE_MQTT
/**
 * @brief Callback when MQTT data is received
 *
 * @param topic Topic of the received message
 * @param data Message payload
 * @param data_len Length of the payload
 *
 * @details Handles commands for SHT3X sensor and relay control.
 */
static void on_mqtt_data_received(const char *topic, const char *data, int data_len)
{
    ESP_LOGI(TAG, "<- MQTT: %s = %.*s", topic, data_len, data);

    // Handle SHT3X commands
    if (strcmp(topic, TOPIC_SHT3X_COMMAND) == 0)
    {
        // Track periodic state based on commands
        if (strstr(data, "PERIODIC") && !strstr(data, "STOP"))
        {
            int new_rate = extract_periodic_rate(data);
            update_and_publish_state(g_device_on, true, new_rate);
        }
        else if (strstr(data, "PERIODIC STOP"))
        {
            update_and_publish_state(g_device_on, false, g_periodic_rate);
        }

        if (STM32_UART_SendCommand(&stm32_uart, data))
        {
            ESP_LOGI(TAG, "Command forwarded to STM32: %s", data);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to send command to STM32: %s", data);
        }
    }
    // Handle relay commands
    else if (strcmp(topic, TOPIC_CONTROL_RELAY) == 0)
    {
        if (Relay_ProcessCommand(&relay_control, data))
        {
            ESP_LOGI(TAG, "Relay command processed: %s", data);
        }
        else
        {
            ESP_LOGW(TAG, "Unknown relay command: %s", data);
        }
    }
    // Handle state sync requests
    else if (strcmp(topic, TOPIC_STATE_SYNC) == 0 &&
             strstr(data, "REQUEST"))
    {
        ESP_LOGI(TAG, "State sync requested by client");
        publish_current_state();
    }
}
#endif

#ifdef CONFIG_ENABLE_COAP

// TODO: Add COAP callback functions if needed

#endif

/**
 * @brief WiFi event callback
 *
 * @param state Current WiFi state
 * @param arg User argument (not used)
 *
 * @details Logs WiFi state changes and retrieves IP/RSSI on connection.
 */
static void on_wifi_event(wifi_state_t state, void *arg)
{
    switch (state)
    {
    case WIFI_STATE_CONNECTING:
        ESP_LOGI(TAG, "WiFi: Connecting...");
        break;

    case WIFI_STATE_CONNECTED:
        ESP_LOGI(TAG, "WiFi: Connected successfully");

        // Get and display IP address
        char ip_addr[16];
        if (wifi_manager_get_ip_addr(ip_addr, sizeof(ip_addr)) == ESP_OK)
        {
            ESP_LOGI(TAG, "WiFi IP: %s", ip_addr);
        }

        // Get and display signal strength
        int8_t rssi;
        if (wifi_manager_get_rssi(&rssi) == ESP_OK)
        {
            ESP_LOGI(TAG, "WiFi RSSI: %d dBm", rssi);
        }
        break;

    case WIFI_STATE_DISCONNECTED:
        ESP_LOGW(TAG, "WiFi: Disconnected");
        break;

    case WIFI_STATE_FAILED:
        ESP_LOGE(TAG, "WiFi: Connection failed");
        break;

    default:
        break;
    }
}

/* INITIALIZATION FUNCTIONS --------------------------------------------------*/

/**
 * @brief Initialize all components
 *
 * @return true if all components initialized successfully, false otherwise
 *
 * @details Initializes STM32 UART, MQTT Handler, Relay Control, and SHT3X Parser.
 *          Sets initial global state based on hardware.
 */
static bool initialize_components(void)
{
    bool success = true;

    // Initialize STM32 UART
    if (!STM32_UART_Init(&stm32_uart,
                         CONFIG_MQTT_UART_PORT_NUM,
                         CONFIG_MQTT_UART_BAUD_RATE,
                         CONFIG_MQTT_UART_TXD,
                         CONFIG_MQTT_UART_RXD,
                         on_stm32_data_received))
    {
        ESP_LOGE(TAG, "Failed to initialize STM32 UART");
        success = false;
    }

#ifdef CONFIG_ENABLE_MQTT
    // Initialize MQTT Handler
    if (!MQTT_Handler_Init(&mqtt_handler,
                           CONFIG_BROKER_URL,
                           CONFIG_MQTT_USERNAME,
                           CONFIG_MQTT_PASSWORD,
                           on_mqtt_data_received))
    {
        ESP_LOGE(TAG, "Failed to initialize MQTT Handler");
        success = false;
    }
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Initialize CoAP Handler if implemented
    // if (!CoAP_Handler_Init(&coap_handler, ...))
    // {
    //     ESP_LOGE(TAG, "Failed to initialize CoAP Handler");
    //     success = false;
    // }
    ESP_LOGI(TAG, "CoAP support enabled but not implemented yet");
#endif

    // Initialize Relay Control
    if (!Relay_Init(&relay_control, CONFIG_RELAY_GPIO_NUM, on_relay_state_changed))
    {
        ESP_LOGE(TAG, "Failed to initialize Relay Control");
        success = false;
    }

    // Initialize sht3x Parser
    if (!SHT3X_Parser_Init(&sht3x_parser, on_single_sht3x_data, on_periodic_sht3x_data))
    {
        ESP_LOGE(TAG, "Failed to initialize Sht3x Parser");
        success = false;
    }

    // Initialize global state with actual hardware state
    g_device_on = Relay_GetState(&relay_control);
    g_periodic_active = false;
    g_periodic_rate = 1;

    return success;
}

/**
 * @brief Start all services/tasks
 *
 * @return true if all services started successfully, false otherwise
 *
 * @details Starts STM32 UART task, MQTT client, and CoAP client if enabled.
 */
static bool start_services(void)
{
    bool success = true;

    // Start STM32 UART task
    if (!STM32_UART_StartTask(&stm32_uart))
    {
        ESP_LOGE(TAG, "Failed to start STM32 UART task");
        success = false;
    }

#ifdef CONFIG_ENABLE_MQTT
    // Start MQTT client
    if (!MQTT_Handler_Start(&mqtt_handler))
    {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        success = false;
    }
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Start CoAP client if implemented
    // if (!CoAP_Handler_Start(&coap_handler))
    // {
    //     ESP_LOGE(TAG, "Failed to start CoAP client");
    //     success = false;
    // }
#endif

    return success;
}

#ifdef CONFIG_ENABLE_MQTT
/**
 * @brief Subscribe to MQTT topics
 *
 * @details Subscribes to command and control topics after ensuring MQTT connection.
 */
static void subscribe_mqtt_topics(void)
{
    // Wait for MQTT connection
    int retry_count = 0;
    while (!MQTT_Handler_IsConnected(&mqtt_handler) && retry_count < 30)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retry_count++;
    }

    if (!MQTT_Handler_IsConnected(&mqtt_handler))
    {
        ESP_LOGE(TAG, "MQTT connection timeout, cannot subscribe to topics");
        return;
    }

    // Subscribe to command topics
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_SHT3X_COMMAND, 1);
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_CONTROL_RELAY, 1);
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_STATE_SYNC, 1);

    // Publish initial state with retain flag
    publish_current_state();

    ESP_LOGI(TAG, "MQTT topics subscribed and initial state published");
}
#endif

#ifdef CONFIG_ENABLE_MQTT
/**
 * @brief MQTT subscribe task
 *
 * @param param Task parameter (not used)
 *
 * @details Task to handle MQTT topic subscriptions.
 */
static void mqtt_subscribe_task(void *param)
{
    subscribe_mqtt_topics();
    vTaskDelete(NULL);
}
#endif

/* MAIN APPLICATION ----------------------------------------------------------*/

void app_main(void)
{
    ESP_LOGI(TAG, "=== IoT Bridge Starting ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

#if (CONFIG_ENABLE_MQTT || CONFIG_ENABLE_COAP)
    // Display enabled protocols
    ESP_LOGI(TAG, "Protocols enabled:");
#endif

#ifdef CONFIG_ENABLE_MQTT
    ESP_LOGI(TAG, "MQTT: (Broker: %s)", CONFIG_BROKER_URL);
#endif

#ifdef CONFIG_ENABLE_COAP
    ESP_LOGI(TAG, "CoAP: (Server: %s:%d)", CONFIG_COAP_SERVER_IP, CONFIG_COAP_SERVER_PORT);
#endif

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize event loop (required for WiFi)
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Event loop initialized");

    // Initialize and connect WiFi using custom WiFi Manager
    ESP_LOGI(TAG, "=== Initializing WiFi Manager ===");
    wifi_manager_config_t wifi_config = wifi_manager_get_default_config();
    wifi_config.event_callback = on_wifi_event;
    wifi_config.callback_arg = NULL;

    // Initialize WiFi Manager
    if (wifi_manager_init(&wifi_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to initialize WiFi Manager, restarting...");
        esp_restart();
    }

    // Start WiFi connection
    if (wifi_manager_connect() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start WiFi connection, restarting...");
        esp_restart();
    }

    // Wait for WiFi connection
    if (wifi_manager_wait_connected(CONFIG_WIFI_CONNECTION_TIMEOUT_MS) != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi connection failed, restarting...");
        esp_restart();
    }

    ESP_LOGI(TAG, "WiFi connected successfully!");

    // Initialize all components
    if (!initialize_components())
    {
        ESP_LOGE(TAG, "Failed to initialize components, restarting...");
        esp_restart();
    }
    ESP_LOGI(TAG, "All components initialized successfully");

    // Start all services
    if (!start_services())
    {
        ESP_LOGE(TAG, "Failed to start services, restarting...");
        esp_restart();
    }
    ESP_LOGI(TAG, "All services started successfully");

#if CONFIG_ENABLE_MQTT
    // Subscribe to MQTT topics (runs in background)
    xTaskCreate(mqtt_subscribe_task, "mqtt_subscribe", 10240, NULL, 3, NULL);
#endif

    // Log wifi configuration details
    ESP_LOGI(TAG, "WiFi Configuration:");
    ESP_LOGI(TAG, "WiFi SSID: %s", CONFIG_WIFI_SSID);
    ESP_LOGI(TAG, "WiFi Password: %s", CONFIG_WIFI_PASSWORD);
    ESP_LOGI(TAG, "WiFi Connection Timeout: %d ms", CONFIG_WIFI_CONNECTION_TIMEOUT_MS);

    // Log hardware configuration details
    ESP_LOGI(TAG, "Hardware Configuration:");
    ESP_LOGI(TAG, "STM32 UART: Port %d, TXD=%d, RXD=%d, Baud=%d",
             CONFIG_MQTT_UART_PORT_NUM, CONFIG_MQTT_UART_TXD,
             CONFIG_MQTT_UART_RXD, CONFIG_MQTT_UART_BAUD_RATE);
    ESP_LOGI(TAG, "Relay GPIO: %d", CONFIG_RELAY_GPIO_NUM);

#ifdef CONFIG_ENABLE_MQTT
    // Log MQTT configuration details
    ESP_LOGI(TAG, "MQTT Configuration:");
    ESP_LOGI(TAG, "MQTT Broker: %s", CONFIG_BROKER_URL);
    ESP_LOGI(TAG, "Topics:");
    ESP_LOGI(TAG, "Commands: %s", TOPIC_SHT3X_COMMAND);
    ESP_LOGI(TAG, "Relay: %s", TOPIC_CONTROL_RELAY);
    ESP_LOGI(TAG, "State: %s", TOPIC_STATE_SYNC);
    ESP_LOGI(TAG, "Single Temperature: %s", TOPIC_SHT3X_SINGLE_TEMPERATURE);
    ESP_LOGI(TAG, "Single Humidity: %s", TOPIC_SHT3X_SINGLE_HUMIDITY);
    ESP_LOGI(TAG, "Periodic Temperature: %s", TOPIC_SHT3X_PERIODIC_TEMPERATURE);
    ESP_LOGI(TAG, "Periodic Humidity: %s", TOPIC_SHT3X_PERIODIC_HUMIDITY);
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP configuration details here
    // Log CoAP configuration details
    // ESP_LOGI(TAG, "CoAP Configuration:");
    // ESP_LOGI(TAG, "Server: %s:%d", CONFIG_COAP_SERVER_IP, CONFIG_COAP_SERVER_PORT);
    // ESP_LOGI(TAG, "URI Path: %s", CONFIG_COAP_URI_PATH);
#endif

#ifdef CONFIG_ENABLE_MQTT
    bool last_mqtt = MQTT_Handler_IsConnected(&mqtt_handler);
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Uncomment if you have COAP handler
    // bool last_coap = CoAP_Handler_IsConnected(&coap_handler);
#endif

    // Status tracking
    bool last_relay = g_device_on;
    bool last_periodic = g_periodic_active;
    bool last_wifi = wifi_manager_is_connected();

    ESP_LOGI(TAG, "Initial State: WiFi=%s, MQTT=%s, COAP=%s, Device=%s, Periodic=%s",
             last_wifi ? "Connected" : "Disconnected",
#ifdef CONFIG_ENABLE_MQTT
             last_mqtt ? "Connected" : "Disconnected",
#else
             "No Protocol",
#endif
#ifdef CONFIG_ENABLE_COAP
            // TODO: Uncomment if you have COAP handler
            //  last_coap ? "Connected" : "Disconnected",
             "No COAP",
#else
             "No Protocol",
#endif
             last_relay ? "ON" : "OFF",
             last_periodic ? "ON" : "OFF");

    // Main monitoring loop
    while (1)
    {
        bool relay_now = g_device_on;
        bool periodic_now = g_periodic_active;
        bool wifi_now = wifi_manager_is_connected();

#ifdef CONFIG_ENABLE_MQTT
        bool mqtt_now = MQTT_Handler_IsConnected(&mqtt_handler);

        if (relay_now != last_relay || periodic_now != last_periodic ||
            mqtt_now != last_mqtt || wifi_now != last_wifi)
        {
            ESP_LOGI(TAG, "Status: WiFi=%s, MQTT=%s, Device=%s, Periodic=%s, Heap=%lu",
                     wifi_now ? "Connected" : "Disconnected",
                     mqtt_now ? "Connected" : "Disconnected",
                     relay_now ? "ON" : "OFF",
                     periodic_now ? "ON" : "OFF",
                     esp_get_free_heap_size());
        }

        last_relay = relay_now;
        last_periodic = periodic_now;
        last_mqtt = mqtt_now;
        last_wifi = wifi_now;
#endif

#ifdef CONFIG_ENABLE_COAP
        // TODO: Uncomment if you have COAP handler
        // bool coap_now = CoAP_Handler_IsConnected(&coap_handler);
        // if (relay_now != last_relay || periodic_now != last_periodic ||
        //     coap_now != last_coap || wifi_now != last_wifi)
        // {
        //     ESP_LOGI(TAG, "Status: WiFi=%s, CoAP=%s, Device=%s, Periodic=%s, Heap=%lu",
        //              wifi_now ? "Connected" : "Disconnected",
        //              coap_now ? "Connected" : "Disconnected",
        //              relay_now ? "ON" : "OFF",
        //              periodic_now ? "ON" : "OFF",
        //              esp_get_free_heap_size());
        // }
        // last_relay = relay_now;
        // last_periodic = periodic_now;
        // last_coap = coap_now;
        // last_wifi = wifi_now;
#endif
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}