/**
 * @file relay_control.c
 *
 * @brief Relay Control Library Implementation for ESP32
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "relay_control.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "RELAY_CONTROL";

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize relay control
 */
bool Relay_Init(relay_control_t *relay, int gpio_num,
                relay_state_callback_t callback)
{
    if (!relay || gpio_num < 0)
    {
        return false;
    }

    // Initialize structure
    relay->gpio_num = gpio_num;
    relay->state = false;
    relay->initialized = false;
    relay->state_callback = callback;

    // Configure GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << gpio_num),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", gpio_num, esp_err_to_name(ret));
        return false;
    }

    // Set initial state (OFF)
    gpio_set_level(gpio_num, 0);
    relay->initialized = true;

    ESP_LOGI(TAG, "Relay initialized on GPIO%d", gpio_num);
    return true;
}

/**
 * @brief Set relay state
 */
bool Relay_SetState(relay_control_t *relay, bool state)
{
    if (!relay || !relay->initialized)
    {
        return false;
    }

    gpio_set_level(relay->gpio_num, state ? 1 : 0);
    relay->state = state;

    ESP_LOGI(TAG, "Relay %s (GPIO%d)", state ? "ON" : "OFF", relay->gpio_num);

    // Call callback if available
    if (relay->state_callback)
    {
        relay->state_callback(state);
    }

    return true;
}

/**
 * @brief Get relay state
 */
bool Relay_GetState(relay_control_t *relay)
{
    if (!relay || !relay->initialized)
    {
        return false;
    }

    return relay->state;
}

/**
 * @brief Toggle relay state
 */
bool Relay_Toggle(relay_control_t *relay)
{
    if (!relay || !relay->initialized)
    {
        return false;
    }

    bool new_state = !relay->state;
    Relay_SetState(relay, new_state);
    return new_state;
}

/**
 * @brief Process relay command string
 */
bool Relay_ProcessCommand(relay_control_t *relay, const char *command)
{
    if (!relay || !relay->initialized || !command)
    {
        return false;
    }

    // Convert to uppercase for comparison
    if (strstr(command, "ON") || strstr(command, "on") ||
        strstr(command, "1") || strstr(command, "true") || strstr(command, "TRUE"))
    {
        return Relay_SetState(relay, true);
    }
    else if (strstr(command, "OFF") || strstr(command, "off") ||
             strstr(command, "0") || strstr(command, "false") || strstr(command, "FALSE"))
    {
        return Relay_SetState(relay, false);
    }
    else if (strstr(command, "TOGGLE") || strstr(command, "toggle"))
    {
        Relay_Toggle(relay);
        return true;
    }

    ESP_LOGW(TAG, "Unknown relay command: %s", command);
    return false;
}

/**
 * @brief Deinitialize relay control
 */
void Relay_Deinit(relay_control_t *relay)
{
    if (!relay)
    {
        return;
    }

    if (relay->initialized)
    {
        // Turn off relay before deinit
        gpio_set_level(relay->gpio_num, 0);
        relay->initialized = false;
    }

    ESP_LOGI(TAG, "Relay GPIO%d deinitialized", relay->gpio_num);
}