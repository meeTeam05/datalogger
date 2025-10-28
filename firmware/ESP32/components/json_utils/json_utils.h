/**
 * @file json_utils.h
 * 
 * @brief Universal JSON Utility Library
 * 
 * @details Provides common JSON formatting functions for MQTT publishing.
 *          Centralized JSON creation to ensure consistency across all components.
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* DEFINES -------------------------------------------------------------------*/

#define JSON_UTILS_MAX_BUFFER_SIZE  512

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Create JSON sensor data message
 * 
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @param mode "SINGLE" or "PERIODIC"
 * @param timestamp Unix timestamp (0 = RTC failure)
 * @param temperature Temperature in Celsius (0.00 = sensor failure)
 * @param humidity Humidity in % (0.00 = sensor failure)
 * 
 * @return Number of characters written, or -1 on error
 * 
 * @example Output: {"mode":"SINGLE","timestamp":1760739567,"temperature":30.59,"humidity":73.97}
 */
int JSON_Utils_CreateSensorData(char *buffer, size_t buffer_size,
                                 const char *mode,
                                 uint32_t timestamp,
                                 float temperature,
                                 float humidity);

/**
 * @brief Create JSON system state message
 * 
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @param device_on Device relay state (true=ON, false=OFF)
 * @param periodic_active Periodic mode active (true=ON, false=OFF)
 * @param timestamp Current timestamp in milliseconds
 * 
 * @return Number of characters written, or -1 on error
 * 
 * @example Output: {"device":"ON","periodic":"ON","timestamp":123456789}
 */
int JSON_Utils_CreateSystemState(char *buffer, size_t buffer_size,
                                  bool device_on,
                                  bool periodic_active,
                                  uint64_t timestamp);

/**
 * @brief Create simple key-value JSON message
 * 
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @param key JSON key name
 * @param value String value
 * 
 * @return Number of characters written, or -1 on error
 * 
 * @example Output: {"key":"value"}
 */
int JSON_Utils_CreateSimpleMessage(char *buffer, size_t buffer_size,
                                    const char *key,
                                    const char *value);

/**
 * @brief Create integer value JSON message
 * 
 * @param buffer Output buffer for JSON string
 * @param buffer_size Size of output buffer
 * @param key JSON key name
 * @param value Integer value
 * 
 * @return Number of characters written, or -1 on error
 * 
 * @example Output: {"key":123}
 */
int JSON_Utils_CreateIntMessage(char *buffer, size_t buffer_size,
                                 const char *key,
                                 int value);

/**
 * @brief Format float value for JSON (handles special cases)
 * 
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @param value Float value to format
 * @param decimals Number of decimal places
 * 
 * @return Pointer to buffer
 * 
 * @note Handles NaN, Infinity, and rounds to specified decimals
 */
const char* JSON_Utils_FormatFloat(char *buffer, size_t buffer_size,
                                    float value, int decimals);

/**
 * @brief Escape string for JSON (handles quotes, backslashes, etc.)
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source string to escape
 * 
 * @return Number of characters written, or -1 on error
 */
int JSON_Utils_EscapeString(char *dest, size_t dest_size, const char *src);

#endif /* JSON_UTILS_H */
