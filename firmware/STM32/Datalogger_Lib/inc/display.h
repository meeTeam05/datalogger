/**
 * @file display.h
 *
 * @brief Display Library for ILI9225 LCD
 *
 * @note Real-time status display with dynamic updates
 */

#ifndef DISPLAY_H
#define DISPLAY_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize display system
 *
 * @note Must be called after ILI9225_Init()
 */
void display_init(void);

/**
 * @brief Update all display data
 *
 * @param time_unix Unix timestamp
 * @param temperature Temperature in °C
 * @param humidity Humidity in %RH
 * @param mqtt_on MQTT status (true = Connected, false = Disconnected)
 * @param periodic_on Periodic mode status
 * @param interval Update interval in seconds
 */
void display_update(time_t time_unix, float temperature, float humidity,
                    bool mqtt_on, bool periodic_on, int interval);

/**
 * @brief Clear entire display
 */
void display_clear(void);

#endif /* DISPLAY_H */
