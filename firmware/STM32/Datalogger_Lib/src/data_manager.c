/**
 * @file data_manager.c
 *
 * @brief Data Manager - Centralized sensor data management
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <string.h>
#include "data_manager.h"
#include "sensor_json_output.h"

/* PRIVATE VARIABLES ---------------------------------------------------------*/

static data_manager_state_t g_data_manager_state = {0};

/* PUBLIC API ----------------------------------------------------------------*/

void DataManager_Init(void)
{
    memset(&g_data_manager_state, 0, sizeof(data_manager_state_t));
    g_data_manager_state.mode = DATA_MANAGER_MODE_IDLE;
    g_data_manager_state.data_ready = false;
}

void DataManager_UpdateSingle(float temperature, float humidity)
{
    g_data_manager_state.mode = DATA_MANAGER_MODE_SINGLE;
    g_data_manager_state.sht3x.temperature = temperature;
    g_data_manager_state.sht3x.humidity = humidity;
    g_data_manager_state.sht3x.valid = true;
    g_data_manager_state.data_ready = true;
}

void DataManager_UpdatePeriodic(float temperature, float humidity)
{
    g_data_manager_state.mode = DATA_MANAGER_MODE_PERIODIC;
    g_data_manager_state.sht3x.temperature = temperature;
    g_data_manager_state.sht3x.humidity = humidity;
    g_data_manager_state.sht3x.valid = true;
    g_data_manager_state.data_ready = true;
}

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

const data_manager_state_t *DataManager_GetState(void)
{
    return &g_data_manager_state;
}

void DataManager_ClearDataReady(void)
{
    g_data_manager_state.data_ready = false;
}

bool DataManager_IsDataReady(void)
{
    return g_data_manager_state.data_ready;
}
