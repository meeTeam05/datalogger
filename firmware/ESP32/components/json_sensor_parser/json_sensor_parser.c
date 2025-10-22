/**
 * @file json_sensor_parser.c
 *
 * @brief Universal JSON Sensor Data Parser Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "json_sensor_parser.h"
#include "esp_log.h"

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "JSON_SENSOR_PARSER";

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Find JSON value for a given key
 *
 * @param json JSON string
 * @param key Key to search for
 * @param value Buffer to store the value
 * @param value_size Size of value buffer
 *
 * @return true if key found and value extracted
 */
static bool json_get_value(const char *json, const char *key, char *value, size_t value_size)
{
    if (!json || !key || !value || value_size == 0)
    {
        return false;
    }

    // Create search pattern: "key":
    char search_key[JSON_PARSER_MAX_KEY_LEN + 10];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);

    // Find the key
    const char *key_pos = strstr(json, search_key);
    if (!key_pos)
    {
        return false;
    }

    // Move past the key to the value
    const char *value_start = key_pos + strlen(search_key);

    // Skip whitespace
    while (*value_start && isspace((unsigned char)*value_start))
    {
        value_start++;
    }

    // Check if value is a string (starts with ")
    bool is_string = (*value_start == '"');
    if (is_string)
    {
        value_start++; // Skip opening quote
    }

    // Find the end of the value
    const char *value_end = value_start;
    if (is_string)
    {
        // For strings, find closing quote
        while (*value_end && *value_end != '"')
        {
            value_end++;
        }
    }
    else
    {
        // For numbers, find comma or closing brace
        while (*value_end && *value_end != ',' && *value_end != '}')
        {
            value_end++;
        }
    }

    // Calculate value length
    size_t len = value_end - value_start;
    if (len >= value_size)
    {
        len = value_size - 1;
    }

    // Copy value
    strncpy(value, value_start, len);
    value[len] = '\0';

    return true;
}

/**
 * @brief Extract string value from JSON
 *
 * @param json JSON string
 * @param key Key to search for
 * @param value Buffer to store the value
 * @param value_size Size of value buffer
 *
 * @return true if key found and value extracted
 */
static bool json_get_string(const char *json, const char *key, char *value, size_t value_size)
{
    return json_get_value(json, key, value, value_size);
}

/**
 * @brief Extract integer value from JSON
 *
 * @param json JSON string
 * @param key Key to search for
 * @param value Pointer to store the integer value
 *
 * @return true if key found and value extracted
 */
__attribute__((unused)) static bool json_get_int(const char *json, const char *key, int32_t *value)
{
    char str_value[32];
    if (!json_get_value(json, key, str_value, sizeof(str_value)))
    {
        return false;
    }

    *value = atoi(str_value);
    return true;
}

/**
 * @brief Extract unsigned integer value from JSON
 *
 * @param json JSON string
 * @param key Key to search for
 * @param value Pointer to store the unsigned integer value
 *
 * @return true if key found and value extracted
 */
static bool json_get_uint(const char *json, const char *key, uint32_t *value)
{
    char str_value[32];
    if (!json_get_value(json, key, str_value, sizeof(str_value)))
    {
        return false;
    }

    *value = (uint32_t)strtoul(str_value, NULL, 10);
    return true;
}

/**
 * @brief Extract float value from JSON
 *
 * @param json JSON string
 * @param key Key to search for
 * @param value Pointer to store the float value
 *
 * @return true if key found and value extracted
 */
static bool json_get_float(const char *json, const char *key, float *value)
{
    char str_value[32];
    if (!json_get_value(json, key, str_value, sizeof(str_value)))
    {
        return false;
    }

    *value = atof(str_value);
    return true;
}

/* PUBLIC API  ---------------------------------------------------------------*/

/**
 * @brief Initialize JSON sensor parser
 */
bool JSON_Parser_Init(json_sensor_parser_t *parser,
                      sensor_data_callback_t single_callback,
                      sensor_data_callback_t periodic_callback,
                      sensor_data_callback_t error_callback)
{
    if (!parser)
    {
        ESP_LOGE(TAG, "Parser is NULL");
        return false;
    }

    parser->single_callback = single_callback;
    parser->periodic_callback = periodic_callback;
    parser->error_callback = error_callback;

    ESP_LOGI(TAG, "JSON sensor parser initialized");
    return true;
}

/**
 * @brief Parse JSON sensor data line
 */
sensor_data_t JSON_Parser_ParseLine(json_sensor_parser_t *parser, const char *json_line)
{
    sensor_data_t data = {0};
    data.valid = false;

    if (!json_line)
    {
        ESP_LOGE(TAG, "JSON line is NULL");
        return data;
    }

    ESP_LOGD(TAG, "Parsing JSON: %s", json_line);

    // Validate JSON format (basic check)
    if (json_line[0] != '{' || strrchr(json_line, '}') == NULL)
    {
        ESP_LOGW(TAG, "Invalid JSON format (missing braces)");
        return data;
    }

    // Extract mode
    char mode_str[16];
    if (!json_get_string(json_line, JSON_FIELD_MODE, mode_str, sizeof(mode_str)))
    {
        ESP_LOGW(TAG, "Failed to extract mode field");
        return data;
    }

    data.mode = JSON_Parser_GetMode(mode_str);
    if (data.mode == SENSOR_MODE_UNKNOWN)
    {
        ESP_LOGW(TAG, "Unknown sensor mode: %s", mode_str);
        return data;
    }

    // Extract timestamp
    if (!json_get_uint(json_line, JSON_FIELD_TIMESTAMP, &data.timestamp))
    {
        ESP_LOGW(TAG, "Failed to extract timestamp field");
        return data;
    }

    // Extract temperature (optional for extensibility)
    float temp;
    if (json_get_float(json_line, JSON_FIELD_TEMPERATURE, &temp))
    {
        data.has_temperature = true;
        data.temperature = temp;
    }

    // Extract humidity (optional for extensibility)
    float hum;
    if (json_get_float(json_line, JSON_FIELD_HUMIDITY, &hum))
    {
        data.has_humidity = true;
        data.humidity = hum;
    }

    // Validate that we have at least one sensor reading
    if (!data.has_temperature && !data.has_humidity)
    {
        ESP_LOGW(TAG, "No sensor fields found in JSON");
        return data;
    }

    // Validate temperature range (if present)
    if (data.has_temperature)
    {
        // Allow 0.00 as it indicates sensor failure
        if (data.temperature != 0.00f &&
            (data.temperature < -40.0f || data.temperature > 125.0f))
        {
            ESP_LOGW(TAG, "Temperature out of range: %.2f°C", data.temperature);
            return data;
        }
    }

    // Validate humidity range (if present)
    if (data.has_humidity)
    {
        // Allow 0.00 as it indicates sensor failure
        if (data.humidity != 0.00f &&
            (data.humidity < 0.0f || data.humidity > 100.0f))
        {
            ESP_LOGW(TAG, "Humidity out of range: %.2f%%", data.humidity);
            return data;
        }
    }

    // Mark as valid
    data.valid = true;

    // Log parsed data
    ESP_LOGI(TAG, "Parsed %s: timestamp=%" PRIu32 ", T=%.2f°C, H=%.2f%%",
             JSON_Parser_GetModeString(data.mode),
             data.timestamp,
             data.has_temperature ? data.temperature : 0.0f,
             data.has_humidity ? data.humidity : 0.0f);

    // Check for hardware failures
    if (JSON_Parser_IsSensorFailed(&data))
    {
        ESP_LOGW(TAG, "Sensor hardware failure detected (T=0.00, H=0.00)");
    }

    if (JSON_Parser_IsRTCFailed(&data))
    {
        ESP_LOGW(TAG, "RTC failure detected (timestamp=0)");
    }

    return data;
}

/**
 * @brief Process JSON line and invoke callbacks
 */
bool JSON_Parser_ProcessLine(json_sensor_parser_t *parser, const char *json_line)
{
    if (!parser)
    {
        ESP_LOGE(TAG, "Parser is NULL");
        return false;
    }

    sensor_data_t data = JSON_Parser_ParseLine(parser, json_line);

    if (!data.valid)
    {
        // Call error callback if provided
        if (parser->error_callback)
        {
            parser->error_callback(&data);
        }
        return false;
    }

    // Call appropriate callback based on mode
    switch (data.mode)
    {
    case SENSOR_MODE_SINGLE:
        if (parser->single_callback)
        {
            parser->single_callback(&data);
        }
        break;

    case SENSOR_MODE_PERIODIC:
        if (parser->periodic_callback)
        {
            parser->periodic_callback(&data);
        }
        break;

    default:
        ESP_LOGW(TAG, "No callback for sensor mode: %d", data.mode);
        return false;
    }

    return true;
}

/**
 * @brief Get sensor mode from string
 */
sensor_mode_t JSON_Parser_GetMode(const char *mode_str)
{
    if (!mode_str)
    {
        return SENSOR_MODE_UNKNOWN;
    }

    if (strcmp(mode_str, JSON_MODE_SINGLE) == 0)
    {
        return SENSOR_MODE_SINGLE;
    }
    else if (strcmp(mode_str, JSON_MODE_PERIODIC) == 0)
    {
        return SENSOR_MODE_PERIODIC;
    }

    return SENSOR_MODE_UNKNOWN;
}

/**
 * @brief Get sensor mode string
 */
const char *JSON_Parser_GetModeString(sensor_mode_t mode)
{
    switch (mode)
    {
    case SENSOR_MODE_SINGLE:
        return JSON_MODE_SINGLE;
    case SENSOR_MODE_PERIODIC:
        return JSON_MODE_PERIODIC;
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Validate sensor data structure
 */
bool JSON_Parser_IsValid(const sensor_data_t *data)
{
    if (!data)
    {
        return false;
    }

    return data->valid &&
           (data->mode == SENSOR_MODE_SINGLE || data->mode == SENSOR_MODE_PERIODIC) &&
           (data->has_temperature || data->has_humidity);
}

/**
 * @brief Check if sensor measurement indicates hardware failure
 */
bool JSON_Parser_IsSensorFailed(const sensor_data_t *data)
{
    if (!data || !data->valid)
    {
        return false;
    }

    // Sensor failed if both temperature and humidity are 0.00
    return (data->has_temperature && data->temperature == 0.00f) &&
           (data->has_humidity && data->humidity == 0.00f);
}

/**
 * @brief Check if RTC has failed
 */
bool JSON_Parser_IsRTCFailed(const sensor_data_t *data)
{
    if (!data || !data->valid)
    {
        return false;
    }

    // RTC failed if timestamp is 0
    return (data->timestamp == 0);
}

/**
 * @brief Convert sensor_data_t to old sht3x_data_t format
 */
bool JSON_Parser_ToLegacyFormat(const sensor_data_t *sensor_data, void *sht3x_data)
{
    // This function is for backward compatibility
    // Implementation depends on the old sht3x_data_t structure
    // Left as a placeholder for now

    ESP_LOGW(TAG, "Legacy format conversion not implemented");
    return false;
}
