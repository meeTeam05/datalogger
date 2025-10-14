/**
 * @file sensor_json_output.h
 *
 * @brief Header file for sensor data JSON serialization and output
 */

#ifndef SENSOR_JSON_OUTPUT_H
#define SENSOR_JSON_OUTPUT_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Formats sensor data into a JSON string and prints it via UART.
 *
 * @param mode A string literal, must be "SINGLE" or "PERIODIC".
 * @param temperature The temperature value in Celsius.
 * @param humidity The humidity value in percentage.
 * 
 * @details This function is thread-safe as it uses a static buffer internally.
 *          It handles potential buffer overflows.
 *          The output strictly follows the JSON format specification.
 */
void sensor_json_output_send(const char *mode, float temperature, float humidity);

#endif /* SENSOR_JSON_OUTPUT_H */