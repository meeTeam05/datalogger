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

void sensor_json_output_send(const char *mode, float temperature, float humidity)
{
    static char json_buffer[JSON_BUFFER_SIZE];

    // Get Unix timestamp from RTC
    time_t timestamp = get_unix_timestamp();

    // Format JSON string with strict format (no spaces, single line, \r\n at end)
    int written = snprintf(json_buffer, JSON_BUFFER_SIZE,
                           "{\"mode\":\"%s\",\"timestamp\":%lu,\"temperature\":%.2f,\"humidity\":%.2f}\r\n",
                           mode,
                           (unsigned long)timestamp,
                           temperature,
                           humidity);

    // Check for buffer overflow
    if (written < 0 || written >= JSON_BUFFER_SIZE)
    {
        // Buffer overflow detected, send error JSON instead
        PRINT_CLI((char *)ERROR_JSON);
        return;
    }

    // Send the formatted JSON string via UART
    PRINT_CLI(json_buffer);
}
