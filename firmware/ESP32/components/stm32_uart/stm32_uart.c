/**
 * @file stm32_uart.c
 * 
 * @brief STM32 UART Communication Library Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "stm32_uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "STM32_UART";

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Clean and validate a received line of data
 *
 * @param input Raw input line
 * @param output Buffer to store cleaned line
 * @param output_size Size of output buffer
 *
 * @return true if line is valid and cleaned, false otherwise
 *
 * @details Accepts two formats:
 *          1. JSON: {"mode":"SINGLE","timestamp":123,"temperature":25.50,"humidity":60.00}
 *          2. Legacy: SINGLE timestamp temperature humidity
 */
static bool STM32_UART_CleanLine(const char *input, char *output, size_t output_size)
{
    if (!input || !output || output_size == 0)
    {
        return false;
    }

    // Skip leading whitespace and garbage
    while (*input && (*input < 32 || *input > 126))
    {
        input++;
    }

    if (*input == '\0')
    {
        return false;
    }

    // Check if it's JSON format (starts with '{')
    if (*input == '{')
    {
        // Find the end of JSON (first '}')
        const char *json_end = strchr(input, '}');
        if (!json_end)
        {
            ESP_LOGW(TAG, "Incomplete JSON: %s", input);
            return false;
        }

        // Calculate JSON length
        size_t json_len = json_end - input + 1;
        if (json_len >= output_size)
        {
            ESP_LOGW(TAG, "JSON too long: %zu bytes", json_len);
            return false;
        }

        // Validate JSON contains required fields
        if (!strstr(input, "\"mode\"") || !strstr(input, "\"timestamp\""))
        {
            ESP_LOGW(TAG, "Invalid JSON structure");
            return false;
        }

        // Copy cleaned JSON
        memcpy(output, input, json_len);
        output[json_len] = '\0';

        ESP_LOGI(TAG, "Valid JSON received: %s", output);
        return true;
    }

    // Legacy format: MODE timestamp temp humidity
    const char *src = input;
    char *dst = output;
    size_t dst_pos = 0;

    // Copy only printable ASCII characters
    while (*src && dst_pos < output_size - 1)
    {
        if (*src >= 32 && *src <= 126)
        {
            *dst++ = *src;
            dst_pos++;
        }
        src++;
    }
    *dst = '\0';

    // Validate legacy format
    if (strstr(output, "SINGLE") == NULL && strstr(output, "PERIODIC") == NULL)
    {
        ESP_LOGW(TAG, "No valid mode keyword: %s", output);
        return false;
    }

    // Check minimum fields (mode + at least timestamp)
    int space_count = 0;
    for (size_t i = 0; i < dst_pos; i++)
    {
        if (output[i] == ' ')
        {
            space_count++;
        }
    }

    if (space_count < 1)
    {
        ESP_LOGW(TAG, "Invalid format: %s", output);
        return false;
    }

    ESP_LOGI(TAG, "Valid legacy format: %s", output);
    return true;
}

/**
 * @brief UART event task to handle incoming data
 *
 * @param pvParameters Pointer to stm32_uart_t structure
 */
static void uart_event_task(void *pvParameters)
{
    stm32_uart_t *uart = (stm32_uart_t *)pvParameters;
    uint8_t data[128];

    while (uart->initialized)
    {
        int len = uart_read_bytes(uart->uart_num, data, sizeof(data), pdMS_TO_TICKS(100));

        if (len > 0)
        {
            // Put data into ring buffer
            for (int i = 0; i < len; i++)
            {
                if (!RingBuffer_Put(&uart->rx_buffer, data[i]))
                {
                    ESP_LOGW(TAG, "Ring buffer full, data lost");
                    break;
                }
            }
        }

        // Process received data
        STM32_UART_ProcessData(uart);

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize STM32 UART communication 
 */
bool STM32_UART_Init(stm32_uart_t *uart, int uart_num, int baud_rate,
                     int tx_pin, int rx_pin, stm32_data_callback_t callback)
{
    if (!uart || uart_num < 0 || uart_num >= UART_NUM_MAX)
    {
        return false;
    }

    // Initialize structure
    uart->uart_num = uart_num;
    uart->baud_rate = baud_rate;
    uart->tx_pin = tx_pin;
    uart->rx_pin = rx_pin;
    uart->data_callback = callback;
    uart->initialized = false;

    // Initialize ring buffer
    RingBuffer_Init(&uart->rx_buffer);

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = uart_param_config(uart_num, &uart_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        uart_driver_delete(uart_num);
        return false;
    }

    ret = uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
        uart_driver_delete(uart_num);
        return false;
    }

    for (int i = 0; i < 3; i++)
    {
        uart_flush(uart_num);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    uart->initialized = true;
    ESP_LOGI(TAG, "STM32 UART%d initialized: TXD=%d, RXD=%d, Baud=%d",
             uart_num, tx_pin, rx_pin, baud_rate);

    return true;
}

/**
 * @brief Send command to STM32
 */
bool STM32_UART_SendCommand(stm32_uart_t *uart, const char *command)
{
    if (!uart || !uart->initialized || !command)
    {
        return false;
    }

    // Clear RX buffer before sending to prevent old data contamination
    uart_flush_input(uart->uart_num);
    RingBuffer_Clear(&uart->rx_buffer);

    // Small delay to ensure STM32 is ready
    vTaskDelay(pdMS_TO_TICKS(20));

    char cmd_with_lf[STM32_UART_MAX_LINE_LENGTH];
    int len = snprintf(cmd_with_lf, sizeof(cmd_with_lf), "%s\n", command);

    int sent = uart_write_bytes(uart->uart_num, cmd_with_lf, len);

    // Wait for transmission complete
    uart_wait_tx_done(uart->uart_num, pdMS_TO_TICKS(50));

    if (sent == len)
    {
        ESP_LOGI(TAG, "-> STM32: %s", command);
        return true;
    }
    else
    {
        ESP_LOGE(TAG, "Failed to send command: %s", command);
        return false;
    }
}

/**
 * @brief Process data received from STM32
 */
void STM32_UART_ProcessData(stm32_uart_t *uart)
{
    if (!uart || !uart->initialized)
    {
        return;
    }

    static char line_buffer[STM32_UART_MAX_LINE_LENGTH];
    static int line_pos = 0;
    uint8_t data;

    while (RingBuffer_Get(&uart->rx_buffer, &data))
    {
        // FIXED: Better handling of line endings and invalid characters
        if (data == '\n' || data == '\r')
        {
            // End of line
            if (line_pos > 0)
            {
                line_buffer[line_pos] = '\0';

                // FIXED: Clean up the line before processing
                char cleaned_line[STM32_UART_MAX_LINE_LENGTH];
                if (STM32_UART_CleanLine(line_buffer, cleaned_line, sizeof(cleaned_line)))
                {
                    // Only call callback if line contains valid data
                    if (uart->data_callback)
                    {
                        uart->data_callback(cleaned_line);
                    }
                }

                line_pos = 0;
            }
        }
        else if (data >= 32 && data <= 126 && line_pos < sizeof(line_buffer) - 1)
        {
            // Only accept printable ASCII characters
            line_buffer[line_pos++] = data;
        }
        else if (line_pos >= sizeof(line_buffer) - 1)
        {
            // Line too long, reset
            ESP_LOGW(TAG, "Line too long, resetting buffer");
            line_pos = 0;
        }
        // Ignore non-printable characters silently
    }
}

/**
 * @brief Start UART event task
 */
bool STM32_UART_StartTask(stm32_uart_t *uart)
{
    if (!uart || !uart->initialized)
    {
        return false;
    }

    BaseType_t ret = xTaskCreate(uart_event_task, "stm32_uart", 4096, uart, 5, NULL);
    if (ret != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create UART task");
        return false;
    }

    ESP_LOGI(TAG, "STM32 UART task started");
    return true;
}

/**
 * @brief Deinitialize STM32 UART communication
 *
 * @param uart STM32 UART structure
 */
void STM32_UART_Deinit(stm32_uart_t *uart)
{
    if (!uart)
    {
        return;
    }

    uart->initialized = false;

    if (uart->uart_num >= 0 && uart->uart_num < UART_NUM_MAX)
    {
        uart_driver_delete(uart->uart_num);
    }

    ESP_LOGI(TAG, "STM32 UART%d deinitialized", uart->uart_num);
}