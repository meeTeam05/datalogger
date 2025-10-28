/**
 * @file relay_control.h
 *
 * @brief Relay Control Library for ESP32
 */

#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @typedef relay_state_callback_t
 *
 * @brief Function pointer type for relay state change notifications
 *
 * @param state New relay state (true = ON, false = OFF)
 *
 * @details This callback is triggered whenever relay state changes
 *          through any control method.
 */
typedef void (*relay_state_callback_t)(bool state);

/**
 * @typedef relay_control_t
 *
 * @brief Relay control structure
 *
 * @param gpio_num GPIO pin number for relay control
 * @param state Current relay state (true = ON, false = OFF)
 * @param initialized Initialization state flag
 * @param state_callback Function pointer for state change notifications
 */
typedef struct
{
  int gpio_num;
  bool state;
  bool initialized;
  relay_state_callback_t state_callback;
} relay_control_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize relay control
 *
 * @param relay Relay control structure
 * @param gpio_num GPIO pin number for relay control
 * @param callback State change callback function (optional)
 *
 * @return true if successful
 */
bool Relay_Init(relay_control_t *relay, int gpio_num,
                relay_state_callback_t callback);

/**
 * @brief Set relay state
 *
 * @param relay Relay control structure
 * @param state true for ON, false for OFF
 *
 * @return true if successful
 */
bool Relay_SetState(relay_control_t *relay, bool state);

/**
 * @brief Get relay state
 *
 * @param relay Relay control structure
 *
 * @return Current relay state
 */
bool Relay_GetState(relay_control_t *relay);

/**
 * @brief Toggle relay state
 *
 * @param relay Relay control structure
 *
 * @return New relay state
 */
bool Relay_Toggle(relay_control_t *relay);

/**
 * @brief Process relay command string
 *
 * @param relay Relay control structure
 * @param command Command string ("ON", "OFF", "1", "0", etc.)
 *
 * @return true if command was processed
 */
bool Relay_ProcessCommand(relay_control_t *relay, const char *command);

/**
 * @brief Deinitialize relay control
 *
 * @param relay Relay control structure
 */
void Relay_Deinit(relay_control_t *relay);

#endif /* RELAY_CONTROL_H */