/**
 * @file app_main.c
 *
 * @brief Main Application for ESP32 IoT Bridge
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
#include "json_utils.h"
#include "button_handler.h"
#include "driver/gpio.h"

// Include hardware configuration from Kconfig
#include "button_config.h"
#include "led_config.h"

#ifdef CONFIG_ENABLE_MQTT
#include "mqtt_handler.h"
#endif

#ifdef CONFIG_ENABLE_COAP
#include "coap_handler.h"
#endif

/* DEFINES ------------------------------------------------------------------*/

#ifdef CONFIG_ENABLE_MQTT

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

// Button state tracking
static uint8_t g_interval_index = 0; // Current interval index (0-5)

// WiFi/MQTT reconnection tracking
static uint32_t g_last_wifi_retry_ms = 0;
static bool g_wifi_disconnected_notified = false; // Track if STM32 was notified about disconnect
static uint32_t g_wifi_reconnect_time_ms = 0;     // Track when WiFi reconnected
static bool g_mqtt_started = false;               // Track if MQTT has been started (for boot stabilization)

// Periodic interval values (in seconds)
static const uint16_t INTERVAL_VALUES[] = {5, 30, 60, 600, 1800, 3600};
static const uint8_t INTERVAL_COUNT = sizeof(INTERVAL_VALUES) / sizeof(INTERVAL_VALUES[0]);

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

    ESP_LOGI(TAG, "SINGLE: T=%.1f°C H=%.1f%%",
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

    ESP_LOGI(TAG, "PERIODIC: T=%.1f°C H=%.1f%%",
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
 *
 *          When relay toggles, STM32 gets reset, so we resend WiFi status
 *          after a delay to ensure STM32 receives it.
 */
static void on_relay_state_changed(bool state)
{
    ESP_LOGI(TAG, "Relay: %s", state ? "ON" : "OFF");

    // If device turned OFF, force periodic OFF
    bool new_periodic_state = state ? g_periodic_active : false;

    if (!state && g_periodic_active)
    {
        ESP_LOGI(TAG, "Device OFF - Periodic stopped");
    }

    // Update and publish state (this notifies web)
    update_and_publish_state(state, new_periodic_state);

#ifdef CONFIG_ENABLE_MQTT
    // CRITICAL: When relay toggles, STM32 gets reset and misses MQTT status
    // Wait 500ms for STM32 to boot, then resend current MQTT status
    vTaskDelay(pdMS_TO_TICKS(500));

    bool mqtt_connected = MQTT_Handler_IsConnected(&mqtt_handler);
    if (mqtt_connected)
    {
        STM32_UART_SendCommand(&stm32_uart, "MQTT CONNECTED");
        ESP_LOGI(TAG, "TX STM32: MQTT CONNECTED (relay toggled)");
    }
    else
    {
        STM32_UART_SendCommand(&stm32_uart, "MQTT DISCONNECTED");
        ESP_LOGI(TAG, "TX STM32: MQTT DISCONNECTED (relay toggled)");
    }
#endif
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
    // REMOVED: State synchronization from web to ESP32
    // Let web handle state synchronization to avoid relay bouncing
    // Web will sync its UI based on ESP32's published state
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
 *          sends WiFi status to STM32, and controls WiFi LED indicator.
 */
static void on_wifi_event(wifi_state_t state, void *arg)
{
    switch (state)
    {
    case WIFI_STATE_CONNECTING:
        ESP_LOGI(TAG, "WiFi: Connecting...");
        // Turn off LED while connecting
        gpio_set_level(WIFI_LED_GPIO, 0);
        break;

    case WIFI_STATE_CONNECTED:
        ESP_LOGI(TAG, "WiFi: Connected");

        // Turn on WiFi LED indicator
        gpio_set_level(WIFI_LED_GPIO, 1);

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

        // Don't send WiFi status to STM32 - will send MQTT status instead
        g_wifi_disconnected_notified = false; // Reset disconnect flag
        break;

    case WIFI_STATE_DISCONNECTED:
        ESP_LOGW(TAG, "WiFi: Disconnected");

        // Turn off WiFi LED indicator
        gpio_set_level(WIFI_LED_GPIO, 0);

        // Don't send WiFi status to STM32 - will send MQTT status instead
        break;

    case WIFI_STATE_FAILED:
        ESP_LOGE(TAG, "WiFi: Failed (all retries exhausted)");

        // Turn off WiFi LED indicator
        gpio_set_level(WIFI_LED_GPIO, 0);

        // Don't send WiFi status to STM32 - will send MQTT status instead
        break;

    default:
        break;
    }
}

/* BUTTON CALLBACK FUNCTIONS -------------------------------------------------*/

/**
 * @brief Button callback: Toggle relay ON/OFF
 *
 * @details GPIO_5: Toggles relay state and publishes system state to MQTT
 */
static void on_button_relay_pressed(gpio_num_t gpio_num)
{
    ESP_LOGI(TAG, "Button: Relay toggle");

    // Toggle relay
    bool new_state = !Relay_GetState(&relay_control);
    Relay_SetState(&relay_control, new_state);

    // Update global state immediately for button handlers
    g_device_on = new_state;
    if (!new_state)
    {
        g_periodic_active = false; // Force periodic OFF when device OFF
    }

    // State will be updated and published by on_relay_state_changed callback
}

/**
 * @brief Button callback: Send SINGLE command
 *
 * @details GPIO_17: Sends SINGLE measurement command to STM32
 */
static void on_button_single_pressed(gpio_num_t gpio_num)
{
    // Only send if device is ON (relay active)
    if (!g_device_on)
    {
        ESP_LOGW(TAG, "Button: SINGLE ignored (device OFF)");
        return;
    }

    ESP_LOGI(TAG, "Button: SINGLE");
    STM32_UART_SendCommand(&stm32_uart, "SINGLE");
}

/**
 * @brief Button callback: Toggle PERIODIC ON/OFF
 *
 * @details GPIO_16: Toggles periodic mode and publishes system state to MQTT
 */
static void on_button_periodic_pressed(gpio_num_t gpio_num)
{
    // Only allow if device is ON (relay active)
    if (!g_device_on)
    {
        ESP_LOGW(TAG, "Button: PERIODIC ignored (device OFF)");
        return;
    }

    // Toggle periodic state (calculate new state WITHOUT modifying global yet)
    bool new_periodic_state = !g_periodic_active;

    const char *cmd = new_periodic_state ? "PERIODIC ON" : "PERIODIC OFF";
    ESP_LOGI(TAG, "Button: %s", cmd);
    STM32_UART_SendCommand(&stm32_uart, cmd);

    // Update and publish state to MQTT (this will update g_periodic_active)
    update_and_publish_state(g_device_on, new_periodic_state);
}

/**
 * @brief Button callback: Cycle through interval values
 *
 * @details GPIO_4: Cycles through intervals: 5s -> 30s -> 60s -> 600s -> 1800s -> 3600s -> back to 5s
 */
static void on_button_interval_pressed(gpio_num_t gpio_num)
{
    // Only allow if device is ON (relay active)
    if (!g_device_on)
    {
        ESP_LOGW(TAG, "Button: INTERVAL ignored (device OFF)");
        return;
    }

    // Cycle to next interval
    g_interval_index = (g_interval_index + 1) % INTERVAL_COUNT;
    uint16_t interval = INTERVAL_VALUES[g_interval_index];

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "SET PERIODIC INTERVAL %d", interval);

    ESP_LOGI(TAG, "Button: Interval %ds", interval);
    STM32_UART_SendCommand(&stm32_uart, cmd);
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

    // Initialize button handlers
    ESP_LOGI(TAG, "Initializing button handlers...");

    if (!Button_Init(BUTTON_RELAY_GPIO, on_button_relay_pressed))
    {
        ESP_LOGE(TAG, "Failed to initialize relay button");
        success = false;
    }

    if (!Button_Init(BUTTON_SINGLE_GPIO, on_button_single_pressed))
    {
        ESP_LOGE(TAG, "Failed to initialize single button");
        success = false;
    }

    if (!Button_Init(BUTTON_PERIODIC_GPIO, on_button_periodic_pressed))
    {
        ESP_LOGE(TAG, "Failed to initialize periodic button");
        success = false;
    }

    if (!Button_Init(BUTTON_INTERVAL_GPIO, on_button_interval_pressed))
    {
        ESP_LOGE(TAG, "Failed to initialize interval button");
        success = false;
    }

    if (success)
    {
        ESP_LOGI(TAG, "All button handlers initialized");
    }

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

    // Start button handler task
    if (!Button_StartTask())
    {
        ESP_LOGE(TAG, "Failed to start button handler task");
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
// Disable brownout detector
// TODO: comment it if you want to enable brownout detector
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable Brownout

    ESP_LOGI(TAG, "=== IoT Bridge Starting ===");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "IDF version: %s", esp_get_idf_version());

    // Initialize WiFi LED indicator GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << WIFI_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&io_conf);
    gpio_set_level(WIFI_LED_GPIO, 0); // Initially off
    ESP_LOGI(TAG, "WiFi LED indicator initialized on GPIO %d", WIFI_LED_GPIO);

#ifdef CONFIG_ENABLE_MQTT
    // Initialize MQTT LED indicator GPIO
    gpio_config_t mqtt_io_conf = {
        .pin_bit_mask = (1ULL << MQTT_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&mqtt_io_conf);
    gpio_set_level(MQTT_LED_GPIO, 0); // Initially off
    ESP_LOGI(TAG, "MQTT LED indicator initialized on GPIO %d", MQTT_LED_GPIO);
#endif

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
            // Mark WiFi connect time for 4-second MQTT stabilization delay
            g_wifi_reconnect_time_ms = esp_timer_get_time() / 1000;
            ESP_LOGI(TAG, "MQTT will start after 4s network stabilization delay");
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
            gpio_set_level(MQTT_LED_GPIO, 0); // Turn off MQTT LED
            g_wifi_reconnect_time_ms = 0;     // Reset reconnect timer
        }

        // WiFi state changed from disconnected to connected - Wait before starting MQTT
        if (wifi_now && !last_wifi)
        {
            ESP_LOGI(TAG, "WiFi restored, network stabilizing...");
            g_wifi_reconnect_time_ms = now_ms; // Mark reconnect time
        }

        // Start MQTT after 4 seconds of WiFi being stable (both on boot and reconnect)
        // Increased from 2s to 4s to ensure:
        // - DHCP assignment is complete
        // - DNS resolver is ready
        // - Network stack is fully initialized
        if (wifi_now && g_wifi_reconnect_time_ms > 0 &&
            (now_ms - g_wifi_reconnect_time_ms) >= 4000 && !mqtt_now)
        {
            uint32_t elapsed = now_ms - g_wifi_reconnect_time_ms;
            ESP_LOGI(TAG, "Starting MQTT (network stable after %u ms delay)", elapsed);
            MQTT_Handler_Start(&mqtt_handler);
            g_wifi_reconnect_time_ms = 0; // Clear timer
            g_mqtt_started = true;
        }

        // Manual reconnection with exponential backoff (since auto-reconnect is disabled)
        // This provides more control and prevents connection spam
        if (wifi_now && !mqtt_now && g_mqtt_started)
        {
            if (MQTT_Handler_Reconnect(&mqtt_handler))
            {
                ESP_LOGD(TAG, "MQTT reconnection attempt in progress");
            }
        }

        // MQTT state changed from disconnected to connected - SUBSCRIBE to topics
        if (mqtt_now && !last_mqtt)
        {
            ESP_LOGI(TAG, "MQTT connected - Subscribing...");
            gpio_set_level(MQTT_LED_GPIO, 1); // Turn on MQTT LED
            subscribe_mqtt_topics();

            // Send MQTT CONNECTED status to STM32
            STM32_UART_SendCommand(&stm32_uart, "MQTT CONNECTED");
            ESP_LOGI(TAG, "TX STM32: MQTT CONNECTED");
        }

        // MQTT state changed from connected to disconnected - Turn off LED
        if (!mqtt_now && last_mqtt)
        {
            ESP_LOGI(TAG, "MQTT disconnected");
            gpio_set_level(MQTT_LED_GPIO, 0); // Turn off MQTT LED

            // Send MQTT DISCONNECTED status to STM32
            STM32_UART_SendCommand(&stm32_uart, "MQTT DISCONNECTED");
            ESP_LOGI(TAG, "TX STM32: MQTT DISCONNECTED");
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