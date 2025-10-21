/**
 * @file data_manager.h
 *
 * @brief Data Manager - Centralized sensor data management
 */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @brief Measurement mode enumeration
 */
typedef enum
{
    DATA_MANAGER_MODE_IDLE = 0,
    DATA_MANAGER_MODE_SINGLE,
    DATA_MANAGER_MODE_PERIODIC
} data_manager_mode_t;

/**
 * @brief Sensor data structure for SHT3X
 */
typedef struct
{
    float temperature; // Temperature in Celsius
    float humidity;    // Humidity in percentage
    bool valid;        // Data validity flag
} sensor_data_sht3x_t;

/**
 * @brief Complete data manager state structure
 */
typedef struct
{
    data_manager_mode_t mode;  // Current measurement mode
    uint32_t timestamp;        // Unix timestamp from RTC
    sensor_data_sht3x_t sht3x; // SHT3X sensor data
    /* ... */                  // Placeholder for future sensors
    bool data_ready;           // Flag indicating new data available
} data_manager_state_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize the data manager
 *
 * @details Clears all data and sets initial state
 */
void DataManager_Init(void);

/**
 * @brief Update datalogger with SINGLE measurement
 *
 * @param temperature Temperature value from sensor
 * @param humidity Humidity value from sensor
 *
 * @note This function updates internal state and sets data_ready flag
 */
void DataManager_UpdateSingle(float temperature, float humidity);

/**
 * @brief Update data manager with PERIODIC measurement
 *
 * @param temperature Temperature value from sensor
 * @param humidity Humidity value from sensor
 *
 * @note This function updates internal state and sets data_ready flag
 */
void DataManager_UpdatePeriodic(float temperature, float humidity);

/**
 * @brief Print the current sensor data in JSON format
 *
 * @details Outputs: {"mode":"SINGLE|PERIODIC","timestamp":...,"temperature":...,"humidity":...}\r\n
 *          Only prints if data_ready flag is set
 *
 * @return true if data was printed, false if no data ready
 */
bool DataManager_Print(void);

/**
 * @brief Get current data manager state (for debugging)
 *
 * @return Pointer to internal state structure (read-only)
 */
const data_manager_state_t *DataManager_GetState(void);

/**
 * @brief Clear the data_ready flag
 *
 * @details Call this after printing to avoid duplicate output
 */
void DataManager_ClearDataReady(void);

/**
 * @brief Check if new data is ready to be printed
 *
 * @return true if data_ready, false otherwise
 */
bool DataManager_IsDataReady(void);

#endif /* DATA_MANAGER_H */
