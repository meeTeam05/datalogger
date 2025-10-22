/**
 * @file button_config.h
 *
 * @brief Button Handler Configuration from Kconfig
 *
 * This file maps Kconfig settings to button GPIO definitions.
 */

#ifndef BUTTON_CONFIG_H
#define BUTTON_CONFIG_H

/* INCLUDES ------------------------------------------------------------------*/

#include "sdkconfig.h"

/* DEFINES ------------------------------------------------------------------*/

/* Button GPIO Definitions from Kconfig */
#define BUTTON_RELAY_GPIO CONFIG_BUTTON_RELAY_GPIO
#define BUTTON_SINGLE_GPIO CONFIG_BUTTON_SINGLE_GPIO
#define BUTTON_PERIODIC_GPIO CONFIG_BUTTON_PERIODIC_GPIO
#define BUTTON_INTERVAL_GPIO CONFIG_BUTTON_INTERVAL_GPIO

/* Button Debounce Configuration */
#ifndef CONFIG_BUTTON_DEBOUNCE_TIME_MS
#define BUTTON_DEBOUNCE_TIME_MS 50 // Default 50ms if not configured
#else
#define BUTTON_DEBOUNCE_TIME_MS CONFIG_BUTTON_DEBOUNCE_TIME_MS
#endif

#endif /* BUTTON_CONFIG_H */
