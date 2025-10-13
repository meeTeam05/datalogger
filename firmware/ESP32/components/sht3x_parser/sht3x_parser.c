/**
 * @file sht3x_parser.c
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "sht3x_parser.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static const char *TAG = "SHT3X_PARSER";

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize sht3x parser
 *
 * @param parser Sht3x parser structure
 * @param single_callback Callback for single measurements
 * @param periodic_callback Callback for periodic measurements
 *
 * @return true if successful
 */
bool SHT3X_Parser_Init(sht3x_parser_t *parser,
                       sht3x_data_callback_t single_callback,
                       sht3x_data_callback_t periodic_callback)
{
    if (!parser)
    {
        return false;
    }

    parser->single_callback = single_callback;
    parser->periodic_callback = periodic_callback;

    ESP_LOGI(TAG, "Sht3x parser initialized");
    return true;
}

/**
 * @brief Parse sht3x data line
 *
 * @param parser Sht3x parser structure
 * @param line Data line from STM32 (e.g., "SINGLE 27.85 85.69")
 *
 * @return Parsed sht3x data structure
 */
sht3x_data_t SHT3X_Parser_ParseLine(sht3x_parser_t *parser, const char *line)
{
    sht3x_data_t data = {0};
    data.valid = false;

    if (!line)
    {
        return data;
    }

    char mode[16];
    float temp, hum;

    // Parse format: "MODE TEMPERATURE HUMIDITY"
    // Example: "PERIODIC 27.82 85.65" or "SINGLE 27.85 85.69"
    int parsed = sscanf(line, "%15s %f %f", mode, &temp, &hum);

    if (parsed == 3)
    {
        // Determine sht3x type
        if (strcmp(mode, SHT3X_MODE_SINGLE) == 0)
        {
            data.type = SHT3X_TYPE_SINGLE;
        }
        else if (strcmp(mode, SHT3X_MODE_PERIODIC) == 0)
        {
            data.type = SHT3X_TYPE_PERIODIC;
        }
        else
        {
            data.type = SHT3X_TYPE_UNKNOWN;
            ESP_LOGW(TAG, "Unknown sht3x mode: %s", mode);
            return data;
        }

        // Validate temperature range (-40 to 125°C for SHT3X)
        if (temp < -40.0f || temp > 125.0f)
        {
            ESP_LOGW(TAG, "Temperature out of range: %.2f°C", temp);
            return data;
        }

        // Validate humidity range (0 to 100% for SHT3X)
        if (hum < 0.0f || hum > 100.0f)
        {
            ESP_LOGW(TAG, "Humidity out of range: %.2f%%", hum);
            return data;
        }

        data.temperature = temp;
        data.humidity = hum;
        data.valid = true;

        ESP_LOGI(TAG, "Parsed %s: T=%.2f°C, H=%.2f%%",
                 SHT3X_Parser_GetTypeString(data.type), temp, hum);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to parse sht3x data: %s", line);
    }

    return data;
}

/**
 * @brief Process sht3x data line and invoke callbacks
 *
 * @param parser Sht3x parser structure
 * @param line Data line from STM32
 *
 * @return true if line was processed and valid
 */
bool SHT3X_Parser_ProcessLine(sht3x_parser_t *parser, const char *line)
{
    if (!parser)
    {
        return false;
    }

    sht3x_data_t data = SHT3X_Parser_ParseLine(parser, line);

    if (!data.valid)
    {
        return false;
    }

    // Call appropriate callback
    switch (data.type)
    {
    case SHT3X_TYPE_SINGLE:
        if (parser->single_callback)
        {
            parser->single_callback(&data);
        }
        break;

    case SHT3X_TYPE_PERIODIC:
        if (parser->periodic_callback)
        {
            parser->periodic_callback(&data);
        }
        break;

    default:
        ESP_LOGW(TAG, "No callback for sht3x type: %d", data.type);
        return false;
    }

    return true;
}

/**
 * @brief Get sht3x type from string
 *
 * @param type_str Type string ("SINGLE" or "PERIODIC")
 *
 * @return Sht3x type enum
 */
sht3x_type_t SHT3X_Parser_GetType(const char *type_str)
{
    if (!type_str)
    {
        return SHT3X_TYPE_UNKNOWN;
    }

    if (strcmp(type_str, SHT3X_MODE_SINGLE) == 0)
    {
        return SHT3X_TYPE_SINGLE;
    }
    else if (strcmp(type_str, SHT3X_MODE_PERIODIC) == 0)
    {
        return SHT3X_TYPE_PERIODIC;
    }

    return SHT3X_TYPE_UNKNOWN;
}

/**
 * @brief Get sht3x type string
 *
 * @param type Sht3x type enum
 *
 * @return Type string
 */
const char *SHT3X_Parser_GetTypeString(sht3x_type_t type)
{
    switch (type)
    {
    case SHT3X_TYPE_SINGLE:
        return SHT3X_MODE_SINGLE;
    case SHT3X_TYPE_PERIODIC:
        return SHT3X_MODE_PERIODIC;
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Validate sht3x data
 *
 * @param data Sht3x data structure
 *
 * @return true if data is valid
 */
bool SHT3X_Parser_IsValid(const sht3x_data_t *data)
{
    if (!data)
    {
        return false;
    }

    return data->valid &&
           (data->type == SHT3X_TYPE_SINGLE || data->type == SHT3X_TYPE_PERIODIC) &&
           (data->temperature >= -40.0f && data->temperature <= 125.0f) &&
           (data->humidity >= 0.0f && data->humidity <= 100.0f);
}