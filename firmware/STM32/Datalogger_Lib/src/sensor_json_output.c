/**
 * @file sensor_json_output.c
 *
 * @brief Source file for sensor data JSON serialization and output
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sensor_json_output.h"
#include "print_cli.h"
#include "ds3231.h"

/* DEFINES -------------------------------------------------------------------*/

#define JSON_BUFFER_SIZE 128
#define ERROR_JSON "{\"error\":\"buffer_overflow\"}\r\n"

/* PRIVATE FUNCTION PROTOTYPES -----------------------------------------------*/

static time_t get_unix_timestamp(void);

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Retrieves the current Unix timestamp from the DS3231 RTC
 *
 * @return time_t The current Unix timestamp, or 0 on error
 */
static time_t get_unix_timestamp(void)
{
    struct tm time;

    if (DS3231_Get_Time(&g_ds3231, &time) == HAL_OK)
    {
        return mktime(&time);
    }

    return 0; // Return 0 on error
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Formats sensor data into a JSON string and writes to provided buffer
 */
int sensor_json_format(char *buffer, size_t buffer_size,
                       const char *mode, float temperature, float humidity,
                       uint32_t timestamp)
{
    if (buffer == NULL || buffer_size == 0)
    {
        return -1;
    }

    // If timestamp is 0, get it from RTC
    if (timestamp == 0)
    {
        timestamp = (uint32_t)get_unix_timestamp();
    }

    // Format JSON string with strict format (no spaces, single line, \r\n at end)
    int written = snprintf(buffer, buffer_size,
                           "{\"mode\":\"%s\",\"timestamp\":%lu,\"temperature\":%.2f,\"humidity\":%.2f}\r\n",
                           mode,
                           (unsigned long)timestamp,
                           temperature,
                           humidity);

    // Check for buffer overflow
    if (written < 0 || written >= (int)buffer_size)
    {
        return -1; // Buffer overflow
    }

    return written;
}

/**
 * @brief Formats sensor data into a JSON string and prints it via UART
 */
void sensor_json_output_send(const char *mode, float temperature, float humidity)
{
    static char json_buffer[JSON_BUFFER_SIZE];

    // Use the new format function
    int written = sensor_json_format(json_buffer, JSON_BUFFER_SIZE,
                                     mode, temperature, humidity, 0);

    // Check for errors
    if (written < 0)
    {
        // Buffer overflow detected, send error JSON instead
        PRINT_CLI((char *)ERROR_JSON);
        return;
    }

    // Send the formatted JSON string via UART
    PRINT_CLI(json_buffer);
}
