/**
 * @file json_utils.c
 *
 * @brief Universal JSON Utility Library Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "json_utils.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

/* PUBLIC API  ---------------------------------------------------------------*/

/**
 * @brief Create JSON sensor data message
 */
int JSON_Utils_CreateSensorData(char *buffer, size_t buffer_size,
                                const char *mode,
                                uint32_t timestamp,
                                float temperature,
                                float humidity)
{
    if (!buffer || buffer_size == 0 || !mode)
    {
        return -1;
    }

    return snprintf(buffer, buffer_size,
                    "{\"mode\":\"%s\",\"timestamp\":%" PRIu32 ",\"temperature\":%.2f,\"humidity\":%.2f}",
                    mode, timestamp, temperature, humidity);
}

/**
 * @brief Create JSON system state message
 */
int JSON_Utils_CreateSystemState(char *buffer, size_t buffer_size,
                                 bool device_on,
                                 bool periodic_active,
                                 uint64_t timestamp)
{
    if (!buffer || buffer_size == 0)
    {
        return -1;
    }

    return snprintf(buffer, buffer_size,
                    "{\"device\":\"%s\",\"periodic\":\"%s\",\"timestamp\":%llu}",
                    device_on ? "ON" : "OFF",
                    periodic_active ? "ON" : "OFF",
                    timestamp);
}

/**
 * @brief Create simple key-value JSON message
 */
int JSON_Utils_CreateSimpleMessage(char *buffer, size_t buffer_size,
                                   const char *key,
                                   const char *value)
{
    if (!buffer || buffer_size == 0 || !key || !value)
    {
        return -1;
    }

    return snprintf(buffer, buffer_size,
                    "{\"%s\":\"%s\"}",
                    key, value);
}

/**
 * @brief Create integer value JSON message
 */
int JSON_Utils_CreateIntMessage(char *buffer, size_t buffer_size,
                                const char *key,
                                int value)
{
    if (!buffer || buffer_size == 0 || !key)
    {
        return -1;
    }

    return snprintf(buffer, buffer_size,
                    "{\"%s\":%d}",
                    key, value);
}

/**
 * @brief Format float value for JSON (handles special cases)
 */
const char *JSON_Utils_FormatFloat(char *buffer, size_t buffer_size,
                                   float value, int decimals)
{
    if (!buffer || buffer_size == 0)
    {
        return NULL;
    }

    // Handle special cases
    if (isnan(value))
    {
        snprintf(buffer, buffer_size, "null");
    }
    else if (isinf(value))
    {
        snprintf(buffer, buffer_size, value > 0 ? "\"Infinity\"" : "\"-Infinity\"");
    }
    else
    {
        snprintf(buffer, buffer_size, "%.*f", decimals, value);
    }

    return buffer;
}

/**
 * @brief Escape string for JSON (handles quotes, backslashes, etc.)
 */
int JSON_Utils_EscapeString(char *dest, size_t dest_size, const char *src)
{
    if (!dest || dest_size == 0 || !src)
    {
        return -1;
    }

    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j < dest_size - 1; i++)
    {
        switch (src[i])
        {
        case '"':
        case '\\':
            if (j < dest_size - 2)
            {
                dest[j++] = '\\';
                dest[j++] = src[i];
            }
            break;
        case '\n':
            if (j < dest_size - 2)
            {
                dest[j++] = '\\';
                dest[j++] = 'n';
            }
            break;
        case '\r':
            if (j < dest_size - 2)
            {
                dest[j++] = '\\';
                dest[j++] = 'r';
            }
            break;
        case '\t':
            if (j < dest_size - 2)
            {
                dest[j++] = '\\';
                dest[j++] = 't';
            }
            break;
        default:
            dest[j++] = src[i];
            break;
        }
    }

    dest[j] = '\0';
    return j;
}
