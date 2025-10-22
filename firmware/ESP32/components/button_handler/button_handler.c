/**
 * @file button_handler.c
 *
 * @brief Button Handler Implementation with Debounce
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "button_handler.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* PRIVATE VARIABLES --------------------------------------------------------*/

/* Private variables */
static const char *TAG = "BUTTON_HANDLER";
static button_handler_t g_buttons[BUTTON_MAX_HANDLERS];
static uint8_t g_button_count = 0;
static TaskHandle_t g_button_task_handle = NULL;
static bool g_task_running = false;

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/* Private function prototypes */
static void button_task(void *arg);
static bool is_button_pressed(gpio_num_t gpio_num);
static bool should_process_press(button_handler_t *button);

/* PRIVATE FUNCTION IMPLEMENTATIONS -----------------------------------------*/

/**
 * @brief Button monitoring task
 */
static void button_task(void *arg)
{
    ESP_LOGI(TAG, "Button monitoring task started");

    while (g_task_running)
    {
        // Check all registered buttons
        for (uint8_t i = 0; i < g_button_count; i++)
        {
            button_handler_t *button = &g_buttons[i];

            if (!button->initialized)
            {
                continue;
            }

            // Check if button is pressed (LOW = pressed with pull-up)
            if (is_button_pressed(button->gpio_num))
            {
                // Check debounce - enough time since last press?
                if (should_process_press(button))
                {
                    // Update last press time IMMEDIATELY to prevent multiple triggers
                    button->last_press_time = esp_timer_get_time() / 1000;

                    // Small delay to confirm it's not a glitch
                    vTaskDelay(pdMS_TO_TICKS(50));

                    if (!is_button_pressed(button->gpio_num))
                    {
                        // False trigger, button not actually pressed
                        continue;
                    }

                    // Call callback
                    if (button->callback)
                    {
                        ESP_LOGI(TAG, "Button GPIO_%d pressed", button->gpio_num);
                        button->callback(button->gpio_num);
                    }

                    // Wait for button release to avoid multiple triggers
                    while (is_button_pressed(button->gpio_num))
                    {
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }

                    // Small delay after release
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Check every 20ms (reduced from 10ms to save CPU)
    }

    ESP_LOGI(TAG, "Button monitoring task stopped");
    g_button_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief Check if button is currently pressed
 *
 * @param gpio_num GPIO number of the button
 *
 * @return true if pressed, false if released
 */
static bool is_button_pressed(gpio_num_t gpio_num)
{
    // With pull-up: pressed = LOW (0), released = HIGH (1)
    return (gpio_get_level(gpio_num) == 0);
}

/**
 * @brief Check if button press should be processed (debounce)
 *
 * @param button Pointer to button handler
 *
 * @return true if enough time has passed since last press, false otherwise
 */
static bool should_process_press(button_handler_t *button)
{
    uint32_t current_time = esp_timer_get_time() / 1000; // Convert to ms
    uint32_t time_since_last_press = current_time - button->last_press_time;

    return (time_since_last_press >= BUTTON_DEBOUNCE_TIME_MS);
}

/* PUBLIC API ---------------------------------------------------------------*/

/**
 * @brief Initialize button handler
 */
bool Button_Init(gpio_num_t gpio_num, button_press_callback_t callback)
{
    if (g_button_count >= BUTTON_MAX_HANDLERS)
    {
        ESP_LOGE(TAG, "Max button handlers reached");
        return false;
    }

    if (callback == NULL)
    {
        ESP_LOGE(TAG, "Callback is NULL");
        return false;
    }

    // Configure GPIO with internal pull-up
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE, // Enable internal pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE // No interrupt, polling mode
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to configure GPIO_%d: %s", gpio_num, esp_err_to_name(ret));
        return false;
    }

    // Register button
    button_handler_t *button = &g_buttons[g_button_count];
    button->gpio_num = gpio_num;
    button->callback = callback;
    button->last_press_time = 0;
    button->initialized = true;

    g_button_count++;

    ESP_LOGI(TAG, "Button GPIO_%d initialized (pull-up enabled)", gpio_num);
    return true;
}

/**
 * @brief Start button handler task
 */
bool Button_StartTask(void)
{
    if (g_button_task_handle != NULL)
    {
        ESP_LOGW(TAG, "Task already running");
        return true;
    }

    if (g_button_count == 0)
    {
        ESP_LOGE(TAG, "No buttons registered");
        return false;
    }

    g_task_running = true;

    BaseType_t ret = xTaskCreate(
        button_task,
        "button_task",
        4096, // Increased stack size from 2048 to 4096 bytes
        NULL,
        4, // Priority (lower than UART task priority 5)
        &g_button_task_handle);

    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create task");
        g_task_running = false;
        return false;
    }

    ESP_LOGI(TAG, "Task started (monitoring %d buttons)", g_button_count);
    return true;
}

/**
 * @brief Stop button handler task
 */
void Button_StopTask(void)
{
    if (g_button_task_handle == NULL)
    {
        return;
    }

    ESP_LOGI(TAG, "Stopping task...");
    g_task_running = false;

    // Wait for task to finish
    vTaskDelay(pdMS_TO_TICKS(100));
}
