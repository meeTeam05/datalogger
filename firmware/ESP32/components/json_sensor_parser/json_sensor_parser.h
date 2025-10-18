/**
 * @file json_sensor_parser.h
 *
 * @brief Universal JSON Sensor Data Parser Library
 */

#ifndef JSON_SENSOR_PARSER_H
#define JSON_SENSOR_PARSER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/* DEFINES -------------------------------------------------------------------*/

#define JSON_PARSER_MAX_KEY_LEN 32
#define JSON_PARSER_MAX_VALUE_LEN 64
#define JSON_PARSER_MAX_BUFFER_LEN 512

/* Sensor modes */
#define JSON_MODE_SINGLE "SINGLE"
#define JSON_MODE_PERIODIC "PERIODIC"

/* JSON field names */
#define JSON_FIELD_MODE "mode"
#define JSON_FIELD_TIMESTAMP "timestamp"
#define JSON_FIELD_TEMPERATURE "temperature"
#define JSON_FIELD_HUMIDITY "humidity"

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @enum sensor_mode_t
 * @brief Sensor operating mode
 */
typedef enum
{
    SENSOR_MODE_UNKNOWN = 0, /*!< Invalid or unknown mode */
    SENSOR_MODE_SINGLE,      /*!< One-shot measurement mode */
    SENSOR_MODE_PERIODIC     /*!< Continuous measurement mode */
} sensor_mode_t;

/**
 * @struct sensor_data_t
 * @brief Universal sensor data structure
 *
 * @details Contains all measurement data from sensors.
 *          Fields marked with 'has_*' indicate if the field is valid.
 *          This design allows for extensibility to sensors with different fields.
 */
typedef struct
{
    /* Metadata */
    sensor_mode_t mode; /*!< Operating mode (SINGLE/PERIODIC) */
    uint32_t timestamp; /*!< Unix timestamp (0 = RTC failure) */
    bool valid;         /*!< Overall data validity flag */

    /* SHT3X sensor fields */
    bool has_temperature; /*!< Temperature field available */
    float temperature;    /*!< Temperature in Celsius (0.00 = sensor failure) */

    bool has_humidity; /*!< Humidity field available */
    float humidity;    /*!< Relative humidity in % (0.00 = sensor failure) */

    /* Future extensibility: Add more sensor fields here
     * Example:
     * bool has_pressure;
     * float pressure;          // Barometric pressure in hPa
     *
     * bool has_co2;
     * uint16_t co2;            // CO2 concentration in ppm
     */

} sensor_data_t;

/**
 * @typedef sensor_data_callback_t
 * @brief Callback function type for handling parsed sensor data
 *
 * @param data Pointer to the parsed sensor data structure
 */
typedef void (*sensor_data_callback_t)(const sensor_data_t *data);

/**
 * @struct json_sensor_parser_t
 * @brief JSON sensor parser structure
 *
 * @details Holds parser state and callbacks for different modes
 */
typedef struct
{
    sensor_data_callback_t single_callback;   /*!< Callback for SINGLE mode data */
    sensor_data_callback_t periodic_callback; /*!< Callback for PERIODIC mode data */
    sensor_data_callback_t error_callback;    /*!< Callback for parsing errors (optional) */
} json_sensor_parser_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize JSON sensor parser
 *
 * @param parser Parser structure to initialize
 * @param single_callback Callback for SINGLE mode measurements (optional)
 * @param periodic_callback Callback for PERIODIC mode measurements (optional)
 * @param error_callback Callback for parsing errors (optional)
 *
 * @return true if successful, false otherwise
 *
 * @note At least one callback should be provided for the parser to be useful
 */
bool JSON_Parser_Init(json_sensor_parser_t *parser,
                      sensor_data_callback_t single_callback,
                      sensor_data_callback_t periodic_callback,
                      sensor_data_callback_t error_callback);

/**
 * @brief Parse JSON sensor data line
 *
 * @param parser Parser structure (can be NULL if callbacks not needed)
 * @param json_line JSON string from STM32
 *
 * @return Parsed sensor data structure
 *
 * @example Input: {"mode":"SINGLE","timestamp":1760739567,"temperature":30.59,"humidity":73.97}
 *          Output: sensor_data_t with mode=SINGLE, timestamp=1760739567, temp=30.59, hum=73.97
 *
 * @note Returns data.valid=false if parsing fails
 */
sensor_data_t JSON_Parser_ParseLine(json_sensor_parser_t *parser, const char *json_line);

/**
 * @brief Process JSON line and invoke callbacks
 *
 * @param parser Parser structure with callbacks
 * @param json_line JSON string from STM32
 *
 * @return true if line was successfully parsed and processed, false otherwise
 *
 * @note Automatically calls the appropriate callback based on mode
 */
bool JSON_Parser_ProcessLine(json_sensor_parser_t *parser, const char *json_line);

/**
 * @brief Get sensor mode from string
 *
 * @param mode_str Mode string ("SINGLE" or "PERIODIC")
 *
 * @return sensor_mode_t enum value
 */
sensor_mode_t JSON_Parser_GetMode(const char *mode_str);

/**
 * @brief Get sensor mode string
 *
 * @param mode sensor_mode_t enum value
 *
 * @return Mode string ("SINGLE", "PERIODIC", or "UNKNOWN")
 */
const char *JSON_Parser_GetModeString(sensor_mode_t mode);

/**
 * @brief Validate sensor data structure
 *
 * @param data Sensor data structure to validate
 *
 * @return true if data is valid, false otherwise
 *
 * @note Checks:
 *       - valid flag is true
 *       - mode is SINGLE or PERIODIC
 *       - at least one sensor field is present
 */
bool JSON_Parser_IsValid(const sensor_data_t *data);

/**
 * @brief Check if sensor measurement indicates hardware failure
 *
 * @param data Sensor data structure
 *
 * @return true if sensor hardware has failed (temp=0.00 AND hum=0.00)
 *
 * @note This checks if the sensor is disconnected/failed
 *       STM32 returns 0.00 for both fields when sensor fails
 */
bool JSON_Parser_IsSensorFailed(const sensor_data_t *data);

/**
 * @brief Check if RTC has failed
 *
 * @param data Sensor data structure
 *
 * @return true if RTC has failed (timestamp=0)
 *
 * @note This checks if the RTC is disconnected/failed
 *       STM32 returns timestamp=0 when RTC fails
 */
bool JSON_Parser_IsRTCFailed(const sensor_data_t *data);

/* BACKWARD COMPATIBILITY LAYER ----------------------------------------------*/

/**
 * @brief Convert sensor_data_t to old sht3x_data_t format
 *
 * @param sensor_data Modern sensor data structure
 * @param sht3x_data Legacy SHT3X data structure to fill
 *
 * @return true if conversion successful
 *
 * @note Provided for backward compatibility with existing code
 *       New code should use sensor_data_t directly
 */
bool JSON_Parser_ToLegacyFormat(const sensor_data_t *sensor_data, void *sht3x_data);

#endif /* JSON_SENSOR_PARSER_H */
