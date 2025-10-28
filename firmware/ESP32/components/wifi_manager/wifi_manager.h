/**
 * @file wifi_manager.h
 * 
 * @brief WiFi Manager Library for ESP32
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/* INCLUDES ------------------------------------------------------------------*/

#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @typedef wifi_state_t
 *
 * @brief WiFi connection state enumeration
 *
 * @details This enum defines the various states of the WiFi connection:
 *          - WIFI_STATE_DISCONNECTED (0): Not connected to any network
 *          - WIFI_STATE_CONNECTING: Attempting to connect
 *          - WIFI_STATE_CONNECTED: Successfully connected
 *          - WIFI_STATE_FAILED: Connection attempt failed
 */
typedef enum
{
    WIFI_STATE_DISCONNECTED = 0, /**< Not connected */
    WIFI_STATE_CONNECTING,       /**< Connection in progress */
    WIFI_STATE_CONNECTED,        /**< Successfully connected */
    WIFI_STATE_FAILED            /**< Connection failed */
} wifi_state_t;

/**
 * @typedef wifi_event_callback_t
 *
 * @brief Callback function type for WiFi events
 *
 * @param state Current WiFi state
 * @param arg User-defined argument passed during initialization
 *
 * @details This callback is invoked on WiFi state changes, providing the new
 *          state and a user-defined argument for context.
 */
typedef void (*wifi_event_callback_t)(wifi_state_t state, void *arg);

/**
 * @typedef wifi_manager_config_t
 *
 * @brief Configuration structure for WiFi Manager
 *
 * @details This structure holds all configurable parameters for the WiFi Manager:
 *          - ssid: WiFi SSID (network name)
 *          - password: WiFi password (can be NULL for open networks)
 *          - maximum_retry: Maximum number of connection retries
 *          - scan_method: WiFi scan method (0=fast, 1=all_channel)
 *          - sort_method: AP sorting method (0=rssi, 1=security)
 *          - rssi_threshold: Minimum RSSI threshold for connection
 *          - auth_mode_threshold: Minimum authentication mode for connection
 *          - listen_interval: Listen interval for power save mode
 *          - power_save_enabled: Enable/disable power save mode
 *          - power_save_mode: Power save mode type
 *          - ipv6_enabled: Enable/disable IPv6 support
 *          - connection_timeout_ms: Connection timeout in milliseconds
 *          - event_callback: User-defined callback for WiFi events
 *          - callback_arg: Argument passed to the event callback
 */
typedef struct
{
    const char *ssid;                     /*!< WiFi SSID */
    const char *password;                 /*!< WiFi password */
    uint8_t maximum_retry;                /*!< Maximum retry attempts */
    uint8_t scan_method;                  /*!< Scan method: 0=fast, 1=all_channel */
    uint8_t sort_method;                  /*!< Sort method: 0=rssi, 1=security */
    int8_t rssi_threshold;                /*!< Minimum RSSI threshold */
    wifi_auth_mode_t auth_mode_threshold; /*!< Minimum auth mode */
    uint16_t listen_interval;             /*!< Listen interval for power save */
    bool power_save_enabled;              /*!< Enable power save mode */
    wifi_ps_type_t power_save_mode;       /*!< Power save mode type */
    bool ipv6_enabled;                    /*!< Enable IPv6 */
    uint32_t connection_timeout_ms;       /*!< Connection timeout in milliseconds */
    wifi_event_callback_t event_callback; /*!< Event callback function */
    void *callback_arg;                   /*!< Callback argument */
} wifi_manager_config_t;

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
wifi_manager_config_t wifi_manager_get_default_config(void);

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
esp_err_t wifi_manager_init(wifi_manager_config_t *config);

/**
 * @brief Connect to WiFi
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Starts the WiFi connection process. Must be called after
 *          wifi_manager_init(). Returns immediately; use
 *          wifi_manager_wait_connected() to block until connected.
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from WiFi
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Disconnects from the current WiFi network.
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi connection state
 *
 * @return Current WiFi state
 *
 * @details Returns the current state of the WiFi connection, which can be
 *          one of the values defined in the wifi_state_t enum.
 */
wifi_state_t wifi_manager_get_state(void);

/**
 * @brief Check if WiFi is connected
 *
 * @return true if connected, false otherwise
 *
 * @details Returns true if the WiFi is currently connected to an AP,
 *          false otherwise.
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Wait for WiFi connection
 *
 * @param timeout_ms Timeout in milliseconds (0 = wait forever)
 *
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT on timeout
 *
 * @details Blocks until the WiFi is connected or the timeout expires.
 */
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);

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
esp_err_t wifi_manager_get_rssi(int8_t *rssi);

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
esp_err_t wifi_manager_get_ip_addr(char *ip_addr, size_t len);

/**
 * @brief Deinitialize WiFi Manager
 *
 * @return ESP_OK on success, error code otherwise
 *
 * @details Cleans up resources allocated by the WiFi Manager, stops WiFi,
 *          and unregisters event handlers.
 */
esp_err_t wifi_manager_deinit(void);

#endif // WIFI_MANAGER_H