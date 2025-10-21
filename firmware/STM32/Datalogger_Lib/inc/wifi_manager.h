/**
 * @file wifi_manager.h
 *
 * @brief MQTT Manager Header File (formerly WiFi Manager)
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @brief MQTT connection state enumeration
 *
 * @details Represents the current state of the MQTT connection from ESP32.
 */
typedef enum
{
    MQTT_STATE_DISCONNECTED,
    MQTT_STATE_CONNECTED
} mqtt_state_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Get current MQTT connection state
 *
 * @return Current MQTT state
 */
mqtt_state_t mqtt_manager_get_state(void);

#endif