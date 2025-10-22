/**
 * @file cmd_parser.c
 *
 * @brief Command Parser Implementations
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include "cmd_parser.h"
#include "ds3231.h"
#include "data_manager.h"
#include "print_cli.h"
#include "wifi_manager.h"
#include "sht3x.h"
#include "sd_card_manager.h"
#include "stm32f1xx_hal.h"

/* DEFINES -------------------------------------------------------------------*/

#define SHT3X_MODE_REPEAT_DEFAULT SHT3X_HIGH
#define SHT3X_MODE_PERIODIC_DEFAULT SHT3X_PERIODIC_1MPS

// External variables for timing and state control
extern mqtt_state_t mqtt_current_state;

/* PUBLIC API ----------------------------------------------------------------*/

void CHECK_UART_STATUS(uint8_t argc, char **argv)
{
	if (argc != 1)
	{
		PRINT_CLI("Usage: CHECK_UART_STATUS\r\n");
		return;
	}

	// Check UART status
	if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_READY)
	{
		PRINT_CLI("UART is READY\r\n");
	}
	else
	{
		PRINT_CLI("UART is NOT READY\r\n");
	}
}

/**
 * @brief Command parser for SHT3X heater commands
 */
void SHT3X_Heater_Parser(uint8_t argc, char **argv)
{
	if (argc == 3 && strcmp(argv[2], "ENABLE") == 0)
	{
		sht3x_heater_mode_t modeHeater = SHT3X_HEATER_ENABLE;
		if (SHT3X_Heater(&g_sht3x, &modeHeater) == SHT3X_OK)
		{
			PRINT_CLI("SHT3X HEATER ENABLE SUCCEEDED\r\n");
		}
		else
		{
			PRINT_CLI("SHT3X HEATER ENABLE FAILED\r\n");
		}
	}

	else if (argc == 3 && strcmp(argv[2], "DISABLE") == 0)
	{
		sht3x_heater_mode_t modeHeater = SHT3X_HEATER_DISABLE;
		if (SHT3X_Heater(&g_sht3x, &modeHeater) == SHT3X_OK)
		{
			PRINT_CLI("SHT3X HEATER DISABLE SUCCEEDED\r\n");
		}
		else
		{
			PRINT_CLI("SHT3X HEATER DISABLE FAILED\r\n");
		}
	}
}

/**
 * @brief Command parser for SHT3X ART (Accelerated Response Time) commands
 */
void SHT3X_ART_Parser(uint8_t argc, char **argv)
{
	if (SHT3X_ART(&g_sht3x) == SHT3X_OK)
	{
		PRINT_CLI("SHT3X ART MODE SUCCEEDED\r\n");
	}
	else
	{
		PRINT_CLI("SHT3X ART MODE FAILED\r\n");
	}
}

/**
 * @brief Command parser for DS3231 set time input commands
 */
void DS3231_Set_Time_Parser(uint8_t argc, char **argv)
{
	if (argc != 10)
	{
		PRINT_CLI("DS3231 SET TIME <WEEKDAY> <DAY> <MONTH> <YEAR> <HOUR> <MIN> <SEC>\r\n");
		return;
	}

	uint8_t weekday = (uint8_t)atoi(argv[3]);
	uint8_t day = (uint8_t)atoi(argv[4]);
	uint8_t month = (uint8_t)atoi(argv[5]);
	uint16_t year = (uint16_t)atoi(argv[6]);
	uint8_t hour = (uint8_t)atoi(argv[7]);
	uint8_t min = (uint8_t)atoi(argv[8]);
	uint8_t sec = (uint8_t)atoi(argv[9]);

	// Validate parameters
	if (year < 0 || year > 99 || month < 1 || month > 12 || day < 1 || day > 31 || weekday < 1 ||
		weekday > 7 || hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
	{
		PRINT_CLI("DS3231 INVALID PARAMETER VALUES\r\n");
		return;
	}

	// Create struct tm and populate it
	struct tm time;
	time.tm_sec = sec;
	time.tm_min = min;
	time.tm_hour = hour;
	time.tm_mday = day;
	time.tm_mon = month - 1;	// tm_mon is 0-11
	time.tm_year = year + 100;	// tm_year is years since 1900
	time.tm_wday = weekday - 1; // tm_wday is 0-6 (Sunday = 0)

	// Call DS3231_Set_Time with struct tm pointer
	HAL_StatusTypeDef status = DS3231_Set_Time(&g_ds3231, &time);

	if (status == HAL_OK)
	{
		PRINT_CLI("DS3231 TIME SET: 20%02d-%02d-%02d %02d:%02d:%02d (WD:%d)\r\n",
				  year, month, day, hour, min, sec, weekday);
	}
	else
	{
		PRINT_CLI("DS3231 FAILED TO SET TIME\r\n");
	}
}

/**
 * @brief Command parser for SINGLE measurement command
 */
void SINGLE_PARSER(uint8_t argc, char **argv)
{
	if (argc != 1)
	{
		return;
	}

	PRINT_CLI("[CMD] SINGLE\r\n");

	float temp = 0.0f, hum = 0.0f;
	sht3x_repeat_t repeat = SHT3X_MODE_REPEAT_DEFAULT; // Create variable for pointer

	// Read sensor data in single-shot mode
	SHT3X_StatusTypeDef status = SHT3X_Single(&g_sht3x, &repeat, &temp, &hum);

	// Update data manager with measurement (0.0 if sensor failed)
	if (status == SHT3X_OK)
	{
		PRINT_CLI("[CMD] T=%.2f H=%.2f\r\n", temp, hum);
		DataManager_UpdateSingle(temp, hum);
	}
	else
	{
		PRINT_CLI("[CMD] Sensor FAIL\r\n");
		// Sensor read failed, report 0.0 values
		DataManager_UpdateSingle(0.0f, 0.0f);
	}
}

/**
 * @brief Command parser for PERIODIC ON command
 */
void PERIODIC_ON_PARSER(uint8_t argc, char **argv)
{
	if (argc != 2)
	{
		return;
	}

	PRINT_CLI("[CMD] PERIODIC ON\r\n");

	float temp = 0.0f, hum = 0.0f;

	sht3x_mode_t mode = SHT3X_MODE_PERIODIC_DEFAULT;
	sht3x_repeat_t repeat = SHT3X_MODE_REPEAT_DEFAULT;

	// Start periodic mode on sensor
	SHT3X_StatusTypeDef status = SHT3X_Periodic(&g_sht3x, &mode, &repeat, &temp, &hum);

	// Update data manager with first measurement
	if (status == SHT3X_OK)
	{
		PRINT_CLI("[CMD] T=%.2f H=%.2f\r\n", temp, hum);
		DataManager_UpdatePeriodic(temp, hum);
	}
	else
	{
		PRINT_CLI("[CMD] Sensor FAIL\r\n");
		// Sensor failed to start, report 0.0 values
		DataManager_UpdatePeriodic(0.0f, 0.0f);
	}

	// Schedule next fetch regardless of sensor status
	next_fetch_ms = HAL_GetTick() + periodic_interval_ms;
}

/**
 * @brief Command parser for PERIODIC OFF command
 */
void PERIODIC_OFF_PARSER(uint8_t argc, char **argv)
{
	if (argc != 2)
	{
		return;
	}

	SHT3X_Stop_Periodic(&g_sht3x);
}

/**
 * @brief Command parser for DS3231 set time input commands
 */
void SET_TIME_PARSER(uint8_t argc, char **argv)
{
	// SET TIME <unix_timestamp>
	if (argc != 3)
	{
		return;
	}

	// Parse Unix timestamp
	time_t timestamp = (time_t)atol(argv[2]);

	// Convert to struct tm
	struct tm *time = localtime(&timestamp);

	if (time == NULL)
	{
		return;
	}

	// Call DS3231_Set_Time with struct tm pointer
	DS3231_Set_Time(&g_ds3231, time);

	// Force display update immediately after setting time
	extern bool force_display_update;
	force_display_update = true;
}

/**
 * @brief Command parser for SET PERIODIC INTERVAL command
 */
void SET_PERIODIC_INTERVAL_PARSER(uint8_t argc, char **argv)
{
	// SET PERIODIC INTERVAL <SECONDS>
	uint32_t interval;

	if (argc != 4)
	{
		return;
	}
	interval = (uint32_t)atoi(argv[3]);

	// Validate interval (minimum 1 second)
	periodic_interval_ms = interval * 1000; // Convert seconds to milliseconds
}

/**
 * @brief Command parser for MQTT CONNECTED notification
 */
void MQTT_CONNECTED_PARSER(uint8_t argc, char **argv)
{
	if (argc != 2) // "MQTT CONNECTED" = 2 words
	{
		return;
	}

	mqtt_current_state = MQTT_STATE_CONNECTED;
}

/**
 * @brief Command parser for MQTT DISCONNECTED notification
 */
void MQTT_DISCONNECTED_PARSER(uint8_t argc, char **argv)
{
	if (argc != 2) // "MQTT DISCONNECTED" = 2 words
	{
		return;
	}

	mqtt_current_state = MQTT_STATE_DISCONNECTED;
}

/**
 * @brief Command parser for SD CLEAR command
 */
void SD_CLEAR_PARSER(uint8_t argc, char **argv)
{
	if (argc != 2) // "SD CLEAR" = 2 words
	{
		return;
	}

	// Clear SD card buffer
	if (SDCardManager_ClearBuffer())
	{
		PRINT_CLI("SD buffer cleared successfully! All buffered data deleted.\r\n");
	}
	else
	{
		PRINT_CLI("FAILED to clear SD buffer!\r\n");
	}
}
