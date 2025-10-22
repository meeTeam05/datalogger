/**
 * @file button_handler.h
 *
 * @brief Button Handler with Debounce Support
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "button_config.h"

/* DEFINES -------------------------------------------------------------------*/

/* Configuration */
#define BUTTON_MAX_HANDLERS 4 // Maximum number of buttons

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @brief Button press callback function type
 *
 * @param gpio_num GPIO number of the button pressed
 */
typedef void (*button_press_callback_t)(gpio_num_t gpio_num);

/**
 * @brief Button handler structure
 */
typedef struct
{
    gpio_num_t gpio_num;              // GPIO number
    button_press_callback_t callback; // Callback when button pressed
    uint32_t last_press_time;         // Last press timestamp (for debounce)
    bool initialized;                 // Initialization state
} button_handler_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize button handler
 *
 * @param gpio_num GPIO number for button
 * @param callback Callback function when button is pressed
 *
 * @return true if initialized successfully, false otherwise
 *
 * @details Configures GPIO with internal pull-up resistor.
 *          Button should connect GPIO to GND when pressed.
 */
bool Button_Init(gpio_num_t gpio_num, button_press_callback_t callback);

/**
 * @brief Start button handler task
 *
 * @return true if task started successfully, false otherwise
 *
 * @details Creates FreeRTOS task to monitor button states.
 */
bool Button_StartTask(void);

/**
 * @brief Stop button handler task
 */
void Button_StopTask(void);

#endif /* BUTTON_HANDLER_H */
