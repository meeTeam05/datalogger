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
#include "json_sensor_parser.h"
#include "json_utils.h" // JSON utility library

#ifdef CONFIG_ENABLE_MQTT
#include "mqtt_handler.h"
#endif

#ifdef CONFIG_ENABLE_COAP
#include "coap_handler.h"
#endif

/* DEFINES ------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_MQTT
/**
 * MQTT Topic Naming Convention:
 * Format: {organization}/{device-type}/{component}/{action}/{attribute}
 *
 * Benefits:
 * - Clear hierarchy for access control (ACL)
 * - Easy to add new sensors/devices
 * - Wildcard subscription support
 * - Industry standard compliance
 */

// Command topics
#define TOPIC_STM32_COMMAND "datalogger/stm32/command"
#define TOPIC_RELAY_CONTROL "datalogger/esp32/relay/control"
#define TOPIC_SYSTEM_STATE "datalogger/esp32/system/state"

// Data topics - JSON format
#define TOPIC_STM32_DATA_SINGLE "datalogger/stm32/single/data"
#define TOPIC_STM32_DATA_PERIODIC "datalogger/stm32/periodic/data"

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
static json_sensor_parser_t json_parser; // Use new JSON parser

// Global state tracking
static bool g_periodic_active = false;
static bool g_device_on = false;
static bool g_mqtt_reconnected = false; // Track MQTT reconnection events

// WiFi/MQTT reconnection tracking
static uint32_t g_last_wifi_retry_ms = 0;
static bool g_wifi_disconnected_notified = false; // Track if STM32 was notified about disconnect
static uint32_t g_wifi_reconnect_time_ms = 0;     // Track when WiFi reconnected
static bool g_mqtt_started = false;               // Track if MQTT has been started (for boot stabilization)

/* STATE SYNCHRONIZATION FUNCTIONS -------------------------------------------*/

/**
 * @brief Create JSON state message
 *
 * @param buffer Buffer to store JSON string
 * @param buffer_size Size of the buffer
 *
 * @details Creates a JSON string representing the current state.
 *          Example: {"device":"ON","periodic":"OFF"}
 *          Timestamp removed to avoid confusion with milliseconds-since-boot.
 */
static void create_state_message(char *buffer, size_t buffer_size)
{
    // Create state message WITHOUT timestamp
    // Timestamp is only meaningful for sensor data (from STM32 RTC)
    // State sync doesn't need timestamp - just current status
    snprintf(buffer, buffer_size,
             "{\"device\":\"%s\",\"periodic\":\"%s\"}",
             g_device_on ? "ON" : "OFF",
             g_periodic_active ? "ON" : "OFF");
}

/**
 * @brief Publish current state via MQTT
 *
 * @details Publishes the current state to the TOPIC_SYSTEM_STATE topic
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
    MQTT_Handler_Publish(&mqtt_handler, TOPIC_SYSTEM_STATE, state_msg, 0, 1, 1);
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
 *
 * @details Updates the global state variables and publishes the new state
 *          if any of the values have changed.
 */
static void update_and_publish_state(bool device_on, bool periodic_active)
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

    if (state_changed)
    {
        publish_current_state();
    }
}

/* CALLBACK FUNCTIONS --------------------------------------------------------*/

/**
 * @brief Callback when single sensor data is received
 *
 * @param data Pointer to received sensor data
 *
 * @details Publishes single measurement data via MQTT.
 *          Called by JSON parser after validation, so data is always valid.
 */
static void on_single_sensor_data(const sensor_data_t *data)
{
    // Data is already validated by JSON parser, no need to check again

#ifdef CONFIG_ENABLE_MQTT
    // Publish full JSON data using utility function
    char json_msg[256];
    JSON_Utils_CreateSensorData(json_msg, sizeof(json_msg),
                                JSON_Parser_GetModeString(data->mode),
                                data->timestamp,
                                data->has_temperature ? data->temperature : 0.0f,
                                data->has_humidity ? data->humidity : 0.0f);

    // Publish immediately, MQTT_Handler will queue if not connected
    MQTT_Handler_Publish(&mqtt_handler, TOPIC_STM32_DATA_SINGLE, json_msg, 0, 0, 0);

    ESP_LOGI(TAG, "SINGLE: %.2f°C, %.2f%%",
             data->has_temperature ? data->temperature : 0.0f,
             data->has_humidity ? data->humidity : 0.0f);
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP publish logic here if needed
#endif
}

/**
 * @brief Callback when periodic sensor data is received
 *
 * @param data Pointer to received sensor data
 *
 * @details Publishes periodic measurement data via MQTT.
 *          Called by JSON parser after validation, so data is always valid.
 */
static void on_periodic_sensor_data(const sensor_data_t *data)
{
    // Data is already validated by JSON parser, no need to check again

#ifdef CONFIG_ENABLE_MQTT
    // Publish full JSON data using utility function
    char json_msg[256];
    JSON_Utils_CreateSensorData(json_msg, sizeof(json_msg),
                                JSON_Parser_GetModeString(data->mode),
                                data->timestamp,
                                data->has_temperature ? data->temperature : 0.0f,
                                data->has_humidity ? data->humidity : 0.0f);

    // Publish immediately, MQTT_Handler will queue if not connected
    MQTT_Handler_Publish(&mqtt_handler, TOPIC_STM32_DATA_PERIODIC, json_msg, 0, 0, 0);

    ESP_LOGI(TAG, "PERIODIC: %.2f°C, %.2f%%",
             data->has_temperature ? data->temperature : 0.0f,
             data->has_humidity ? data->humidity : 0.0f);
#endif

#ifdef CONFIG_ENABLE_COAP
    // TODO: Add COAP publish logic here if needed
#endif
}

/**
 * @brief Callback when data is received from STM32
 *
 * @param line Received data line (JSON format)
 *
 * @details Forwards received JSON data to the JSON parser.
 */
static void on_stm32_data_received(const char *line)
{
    // Don't log every data line - too verbose, STM32 UART already logs
    // ESP_LOGI(TAG, "← STM32: %s", line);

    // Parse and process JSON sensor data
    JSON_Parser_ProcessLine(&json_parser, line);
}

/**
 * @brief Callback when relay state changes
 *
 * @param state New relay state (true=ON, false=OFF)
 *
 * @details CRITICAL: This is called by relay hardware changes.
 *          We MUST update state and publish, but NEVER send commands here
 *          to avoid infinite loops.
 *
 *          When device turns OFF, periodic MUST also stop.
 */
static void on_relay_state_changed(bool state)
{
    ESP_LOGI(TAG, "Relay hardware state changed: %s", state ? "ON" : "OFF");

    // If device turned OFF, force periodic OFF
    bool new_periodic_state = state ? g_periodic_active : false;

    if (!state && g_periodic_active)
    {
        ESP_LOGI(TAG, "Device OFF detected - periodic mode will be stopped");
    }

    // Update and publish state (this notifies web)
    update_and_publish_state(state, new_periodic_state);
}

#ifdef CONFIG_ENABLE_MQTT
/**
 * @brief Callback when MQTT data is received
 *
 * @param topic Topic of the received message
 * @param data Message payload
 * @param data_len Length of the payload
 *
 * @details Handles commands for STM32 sensor, relay control, and state sync.
 */
static void on_mqtt_data_received(const char *topic, const char *data, int data_len)
{
    // Don't log here - MQTT handler already logs incoming messages

    // Handle STM32 commands from web
    if (strcmp(topic, TOPIC_STM32_COMMAND) == 0)
    {
        // Forward command to STM32
        if (STM32_UART_SendCommand(&stm32_uart, data))
        {
            // Update periodic state based on command (for state tracking only)
            if (strcmp(data, "PERIODIC ON") == 0)
            {
                update_and_publish_state(g_device_on, true);
            }
            else if (strcmp(data, "PERIODIC OFF") == 0)
            {
                update_and_publish_state(g_device_on, false);
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to forward to STM32: %s", data);
        }
    }
    // Handle relay commands
    else if (strcmp(topic, TOPIC_RELAY_CONTROL) == 0)
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
    // Handle state sync requests - only on ESP32 boot or reconnect
    else if (strcmp(topic, TOPIC_SYSTEM_STATE) == 0 &&
             strstr(data, "REQUEST"))
    {
        // Only respond if MQTT just reconnected (flag set by connection handler)
        if (g_mqtt_reconnected)
        {
            ESP_LOGI(TAG, "State sync requested after reconnect");
            publish_current_state();
            g_mqtt_reconnected = false; // Clear flag after responding
        }
        else
        {
            ESP_LOGD(TAG, "Ignoring state sync request (no reconnect event)");
        }
    }
    // Handle state updates from web - sync ESP32 hardware with web UI
    else if (strcmp(topic, TOPIC_SYSTEM_STATE) == 0 &&
             strchr(data, '{') != NULL)
    {
        // Parse JSON state: {"device":"ON/OFF","periodic":"ON/OFF"}
        bool new_device_state = (strstr(data, "\"device\":\"ON\"") != NULL);
        bool new_periodic_state = (strstr(data, "\"periodic\":\"ON\"") != NULL);

        // CRITICAL: Check if states actually changed before doing anything
        bool device_changed = (new_device_state != g_device_on);
        bool periodic_changed = (new_periodic_state != g_periodic_active);

        if (!device_changed && !periodic_changed)
        {
            // State unchanged - this is likely a retained message or echo
            // Don't log or process to avoid spam
            return;
        }

        ESP_LOGI(TAG, "State sync from web: device=%s, periodic=%s",
                 new_device_state ? "ON" : "OFF",
                 new_periodic_state ? "ON" : "OFF");

        // CRITICAL FIX: Update global state FIRST to prevent callback loop
        // When Relay_SetState() triggers callback, g_device_on will already match
        // new state, so callback won't publish (no change detected)
        bool old_device_state = g_device_on;
        bool old_periodic_state = g_periodic_active;

        g_device_on = new_device_state;
        g_periodic_active = new_periodic_state;

        // Sync relay hardware if device state changed
        if (device_changed)
        {
            Relay_SetState(&relay_control, new_device_state);
            ESP_LOGI(TAG, "Relay synced: %s → %s",
                     old_device_state ? "ON" : "OFF",
                     new_device_state ? "ON" : "OFF");
            // Callback will NOT publish because g_device_on already updated
        }

        // Sync periodic state with STM32 if changed and device is ON
        if (periodic_changed && new_device_state)
        {
            const char *cmd = new_periodic_state ? "PERIODIC ON" : "PERIODIC OFF";
            STM32_UART_SendCommand(&stm32_uart, cmd);
            ESP_LOGI(TAG, "Periodic: %s", cmd);
        }

        // If device is OFF, ensure periodic is also OFF
        if (!new_device_state && old_periodic_state)
        {
            STM32_UART_SendCommand(&stm32_uart, "PERIODIC OFF");
            ESP_LOGI(TAG, "Periodic forced OFF (device OFF)");
        }
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
 * @details Logs WiFi state changes, retrieves IP/RSSI on connection,
 *          and sends WiFi status to STM32 (only once per state change).
 */
static void on_wifi_event(wifi_state_t state, void *arg)
{
    switch (state)
    {
    case WIFI_STATE_CONNECTING:
        ESP_LOGI(TAG, "WiFi: Connecting...");
        break;

    case WIFI_STATE_CONNECTED:
        ESP_LOGI(TAG, "WiFi: Connected");

        // Get and display IP address
        char ip_addr[16];
        if (wifi_manager_get_ip_addr(ip_addr, sizeof(ip_addr)) == ESP_OK)
        {
            ESP_LOGI(TAG, "  IP: %s", ip_addr);
        }

        // Get and display signal strength
        int8_t rssi;
        if (wifi_manager_get_rssi(&rssi) == ESP_OK)
        {
            ESP_LOGI(TAG, "  RSSI: %d dBm", rssi);
        }

        // Send WiFi CONNECTED status to STM32 (only once)
        STM32_UART_SendCommand(&stm32_uart, "WIFI CONNECTED\n");
        g_wifi_disconnected_notified = false; // Reset disconnect flag
        break;

    case WIFI_STATE_DISCONNECTED:
        ESP_LOGW(TAG, "WiFi: Disconnected");

        // Send WiFi DISCONNECTED status to STM32 (only once, not on every retry)
        if (!g_wifi_disconnected_notified)
        {
            STM32_UART_SendCommand(&stm32_uart, "WIFI DISCONNECTED\n");
            g_wifi_disconnected_notified = true;
        }
        break;

    case WIFI_STATE_FAILED:
        ESP_LOGE(TAG, "WiFi: Failed (all retries exhausted)");

        // Send WiFi DISCONNECTED status to STM32 (only if not already sent)
        if (!g_wifi_disconnected_notified)
        {
            STM32_UART_SendCommand(&stm32_uart, "WIFI DISCONNECTED");
            g_wifi_disconnected_notified = true;
        }
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
 * @details Initializes MQTT Handler, Relay Control, and JSON Parser.
 *          STM32 UART is already initialized before WiFi.
 *          Sets initial global state based on hardware.
 */
static bool initialize_components(void)
{
    bool success = true;

    // STM32 UART already initialized before WiFi (to send WiFi status)

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

    // Initialize JSON Sensor Parser
    if (!JSON_Parser_Init(&json_parser, on_single_sensor_data, on_periodic_sensor_data, NULL))
    {
        ESP_LOGE(TAG, "Failed to initialize JSON Sensor Parser");
        success = false;
    }

    // Initialize global state with actual hardware state
    g_device_on = Relay_GetState(&relay_control);
    g_periodic_active = false;

    return success;
}

/**
 * @brief Start all services/tasks
 *
 * @return true if all services started successfully, false otherwise
 *
 * @details Starts STM32 UART task, and MQTT client (only if WiFi is connected).
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
    // DO NOT start MQTT here - will start in main loop when network is stable
    // This prevents MQTT session conflicts when ESP32 reboots
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
 * @details Subscribes to command and control topics. Called when MQTT connects.
 */
static void subscribe_mqtt_topics(void)
{
    // Subscribe to command topics
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_STM32_COMMAND, 1);
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_RELAY_CONTROL, 1);
    MQTT_Handler_Subscribe(&mqtt_handler, TOPIC_SYSTEM_STATE, 1);

    // Set reconnection flag to allow next REQUEST to trigger state publish
    g_mqtt_reconnected = true;

    // Publish initial state with retain flag
    publish_current_state();
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

    // Initialize STM32 UART early (before WiFi) so we can send WiFi status
    ESP_LOGI(TAG, "=== Initializing STM32 UART ===");
    if (!STM32_UART_Init(&stm32_uart,
                         CONFIG_MQTT_UART_PORT_NUM,
                         CONFIG_MQTT_UART_BAUD_RATE,
                         CONFIG_MQTT_UART_TXD,
                         CONFIG_MQTT_UART_RXD,
                         on_stm32_data_received))
    {
        ESP_LOGE(TAG, "Failed to initialize STM32 UART, restarting...");
        esp_restart();
    }
    ESP_LOGI(TAG, "STM32 UART initialized successfully");

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

    // Start WiFi connection (non-blocking)
    if (wifi_manager_connect() != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to start WiFi connection");
        // Don't restart, will retry in main loop
    }
    else
    {
        ESP_LOGI(TAG, "WiFi connection started, waiting for connection...");
        // Try to wait, but don't fail if timeout
        if (wifi_manager_wait_connected(CONFIG_WIFI_CONNECTION_TIMEOUT_MS) == ESP_OK)
        {
            ESP_LOGI(TAG, "WiFi connected successfully!");
            // Mark WiFi connect time for 2-second MQTT stabilization delay (same as reconnect logic)
            g_wifi_reconnect_time_ms = esp_timer_get_time() / 1000;
            ESP_LOGI(TAG, "MQTT will start after 2s network stabilization delay");
        }
        else
        {
            ESP_LOGW(TAG, "WiFi initial connection timeout, will retry in background");
        }
    }

    // Initialize all components
    if (!initialize_components())
    {
        ESP_LOGE(TAG, "Failed to initialize components, restarting...");
        esp_restart();
    }
    ESP_LOGI(TAG, "All components initialized successfully");

    // Start all services (but NOT MQTT yet - will start in main loop when network stable)
    if (!start_services())
    {
        ESP_LOGE(TAG, "Failed to start services, restarting...");
        esp_restart();
    }
    ESP_LOGI(TAG, "All services started successfully (MQTT will start when network stable)");

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
    ESP_LOGI(TAG, "Command: %s", TOPIC_STM32_COMMAND);
    ESP_LOGI(TAG, "Relay: %s", TOPIC_RELAY_CONTROL);
    ESP_LOGI(TAG, "State: %s", TOPIC_SYSTEM_STATE);
    ESP_LOGI(TAG, "Single Data: %s", TOPIC_STM32_DATA_SINGLE);
    ESP_LOGI(TAG, "Periodic Data: %s", TOPIC_STM32_DATA_PERIODIC);
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
        wifi_state_t wifi_state = wifi_manager_get_state();
        uint32_t now_ms = esp_timer_get_time() / 1000; // Convert to milliseconds

        // WiFi reconnection logic - ONLY retry if WiFi has FAILED (exhausted all retries)
        // Don't interfere while WiFi Manager is doing its own retries (CONNECTING state)
        if (wifi_state == WIFI_STATE_FAILED)
        {
            // Try to reconnect every 5 seconds (after WiFi Manager's 5 retries are exhausted)
            if ((now_ms - g_last_wifi_retry_ms) >= 5000)
            {
                ESP_LOGI(TAG, "Retrying WiFi connection...");
                wifi_manager_connect();
                g_last_wifi_retry_ms = now_ms;
            }
        }
        else if (wifi_now)
        {
            // WiFi is connected, reset retry timer
            g_last_wifi_retry_ms = now_ms;
        }

#ifdef CONFIG_ENABLE_MQTT
        bool mqtt_now = MQTT_Handler_IsConnected(&mqtt_handler);

        // WiFi state changed from connected to disconnected - STOP MQTT immediately
        if (!wifi_now && last_wifi)
        {
            ESP_LOGI(TAG, "Stopping MQTT (WiFi lost)");
            MQTT_Handler_Stop(&mqtt_handler);
            g_wifi_reconnect_time_ms = 0; // Reset reconnect timer
        }

        // WiFi state changed from disconnected to connected - Wait before starting MQTT
        if (wifi_now && !last_wifi)
        {
            ESP_LOGI(TAG, "WiFi restored, network stabilizing...");
            g_wifi_reconnect_time_ms = now_ms; // Mark reconnect time
        }

        // Start MQTT after 2 seconds of WiFi being stable (both on boot and reconnect)
        if (wifi_now && g_wifi_reconnect_time_ms > 0 && 
            (now_ms - g_wifi_reconnect_time_ms) >= 2000 && !mqtt_now)
        {
            uint32_t elapsed = now_ms - g_wifi_reconnect_time_ms;
            ESP_LOGI(TAG, "Starting MQTT (network stable after %u ms delay)", elapsed);
            MQTT_Handler_Start(&mqtt_handler);
            g_wifi_reconnect_time_ms = 0; // Clear timer
            g_mqtt_started = true;
        }

        // MQTT state changed from disconnected to connected - SUBSCRIBE to topics
        if (mqtt_now && !last_mqtt)
        {
            ESP_LOGI(TAG, "MQTT connected, subscribing...");
            subscribe_mqtt_topics();
        }

        // Log status changes only
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