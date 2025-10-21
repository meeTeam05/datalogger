/**
 * @file data_manager.c
 *
 * @brief Data Manager - Centralized sensor data management
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <string.h>
#include "data_manager.h"
#include "sensor_json_output.h"
#include "print_cli.h"

/* PRIVATE VARIABLES ---------------------------------------------------------*/

// Global data manager state
static data_manager_state_t g_data_manager_state = {0};

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize the Data Manager
 */
void DataManager_Init(void)
{
    memset(&g_data_manager_state, 0, sizeof(data_manager_state_t));
    g_data_manager_state.mode = DATA_MANAGER_MODE_IDLE;
    g_data_manager_state.data_ready = false;
}

/**
 * @brief Update data manager with single measurement data
 */
void DataManager_UpdateSingle(float temperature, float humidity)
{
    g_data_manager_state.mode = DATA_MANAGER_MODE_SINGLE;
    g_data_manager_state.sht3x.temperature = temperature;
    g_data_manager_state.sht3x.humidity = humidity;
    g_data_manager_state.sht3x.valid = true;
    g_data_manager_state.data_ready = true;
}

/**
 * @brief Update data manager with periodic measurement data
 */
void DataManager_UpdatePeriodic(float temperature, float humidity)
{
    g_data_manager_state.mode = DATA_MANAGER_MODE_PERIODIC;
    g_data_manager_state.sht3x.temperature = temperature;
    g_data_manager_state.sht3x.humidity = humidity;
    g_data_manager_state.sht3x.valid = true;
    g_data_manager_state.data_ready = true;
}

/**
 * @brief Print the current data in JSON format
 */
bool DataManager_Print(void)
{
    if (!g_data_manager_state.data_ready)
    {
        return false;
    }

    if (!g_data_manager_state.sht3x.valid)
    {
        return false;
    }

    // Determine mode string
    const char *mode_str;
    switch (g_data_manager_state.mode)
    {
    case DATA_MANAGER_MODE_SINGLE:
        mode_str = "SINGLE";
        break;
    case DATA_MANAGER_MODE_PERIODIC:
        mode_str = "PERIODIC";
        break;
    default:
        return false;
    }

    // Print JSON output
    sensor_json_output_send(mode_str,
                            g_data_manager_state.sht3x.temperature,
                            g_data_manager_state.sht3x.humidity);

    // Clear data_ready flag after printing
    g_data_manager_state.data_ready = false;

    return true;
}

/**
 * @brief Get current data manager state (read-only)
 */
const data_manager_state_t *DataManager_GetState(void)
{
    return &g_data_manager_state;
}

/**
 * @brief Clear the data_ready flag
 */
void DataManager_ClearDataReady(void)
{
    g_data_manager_state.data_ready = false;
}

/**
 * @brief Check if new data is ready to be printed
 */
bool DataManager_IsDataReady(void)
{
    return g_data_manager_state.data_ready;
}
