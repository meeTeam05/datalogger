/**
 * @file wifi_manager.h
 * @brief WiFi Manager - Custom WiFi connection library
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief WiFi connection states
 */
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
} wifi_state_t;

/**
 * @brief WiFi event callback function type
 */
typedef void (*wifi_event_callback_t)(wifi_state_t state, void* arg);

/**
 * @brief WiFi Manager configuration structure
 */
typedef struct {
    const char* ssid;                    /*!< WiFi SSID */
    const char* password;                /*!< WiFi password */
    uint8_t maximum_retry;               /*!< Maximum retry attempts */
    uint8_t scan_method;                 /*!< Scan method: 0=fast, 1=all_channel */
    uint8_t sort_method;                 /*!< Sort method: 0=rssi, 1=security */
    int8_t rssi_threshold;               /*!< Minimum RSSI threshold */
    wifi_auth_mode_t auth_mode_threshold;/*!< Minimum auth mode */
    uint16_t listen_interval;            /*!< Listen interval for power save */
    bool power_save_enabled;             /*!< Enable power save mode */
    wifi_ps_type_t power_save_mode;      /*!< Power save mode type */
    bool ipv6_enabled;                   /*!< Enable IPv6 */
    uint32_t connection_timeout_ms;      /*!< Connection timeout in milliseconds */
    wifi_event_callback_t event_callback;/*!< Event callback function */
    void* callback_arg;                  /*!< Callback argument */
} wifi_manager_config_t;

/**
 * @brief Get default WiFi Manager configuration from Kconfig
 * 
 * @return Default configuration structure
 */
wifi_manager_config_t wifi_manager_get_default_config(void);

/**
 * @brief Initialize WiFi Manager
 * 
 * @param config Configuration structure (NULL to use default from Kconfig)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(wifi_manager_config_t* config);

/**
 * @brief Connect to WiFi
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from WiFi
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi connection state
 * 
 * @return Current WiFi state
 */
wifi_state_t wifi_manager_get_state(void);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * @brief Wait for WiFi connection
 * 
 * @param timeout_ms Timeout in milliseconds (0 = wait forever)
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t wifi_manager_wait_connected(uint32_t timeout_ms);

/**
 * @brief Get WiFi RSSI (signal strength)
 * 
 * @param[out] rssi Pointer to store RSSI value
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_get_rssi(int8_t* rssi);

/**
 * @brief Get WiFi IP address
 * 
 * @param[out] ip_addr Buffer to store IP address string (minimum 16 bytes)
 * @param len Buffer length
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_get_ip_addr(char* ip_addr, size_t len);

/**
 * @brief Deinitialize WiFi Manager
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H