/**
 * @file led_config.h
 * @brief Status LED Configuration from Kconfig
 *
 * This file maps Kconfig settings to LED GPIO definitions.
 */

#ifndef LED_CONFIG_H
#define LED_CONFIG_H

/* INCLUDES ------------------------------------------------------------------*/

#include "sdkconfig.h"

/* DEFINES ------------------------------------------------------------------*/

/* Status LED GPIO Definitions from Kconfig */
#define WIFI_LED_GPIO CONFIG_WIFI_LED_GPIO
#define MQTT_LED_GPIO CONFIG_MQTT_LED_GPIO

/* LED Active Level Configuration */
#ifndef CONFIG_LED_ACTIVE_LEVEL
#define LED_ACTIVE_LEVEL 1 // Default active HIGH if not configured
#else
#define LED_ACTIVE_LEVEL CONFIG_LED_ACTIVE_LEVEL
#endif

/* LED Control Macros */
#if LED_ACTIVE_LEVEL == 1
// Active HIGH: LED on when GPIO = 1
#define LED_ON(gpio) gpio_set_level(gpio, 1)
#define LED_OFF(gpio) gpio_set_level(gpio, 0)
#else
// Active LOW: LED on when GPIO = 0
#define LED_ON(gpio) gpio_set_level(gpio, 0)
#define LED_OFF(gpio) gpio_set_level(gpio, 1)
#endif

#endif /* LED_CONFIG_H */
