/**
 * @file wifi_manager.c
 *
 * @brief MQTT Manager Implementation (formerly WiFi Manager)
 */

#include "wifi_manager.h"

/* EXTERNAL VARIABLES -------------------------------------------------------*/
extern mqtt_state_t mqtt_current_state;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Get current MQTT connection state
 */
mqtt_state_t mqtt_manager_get_state(void)
{
    return mqtt_current_state;
}