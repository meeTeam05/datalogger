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
#include "sht3x.h"
#include "stm32f1xx_hal.h"  // For HAL_Delay and HAL_GetTick

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Command parser function for unknown commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void Cmd_Default(uint8_t argc, char **argv)
{
	PRINT_CLI("UNKNOWN COMMAND\r\n");
}

/**
 * @brief Command parser for SHT3X heater commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
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
 * @brief Command parser for SHT3X single measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Single_Parser(uint8_t argc, char **argv)
{
	if (argc < 3)
	{
		PRINT_CLI("SHT3X SINGLE <HIGH|MEDIUM|LOW>\r\n");
		return;
	}

	sht3x_repeat_t modeRepeat;

	if (strcmp(argv[2], "HIGH") == 0)
	{
		modeRepeat = SHT3X_HIGH;
	}
	else if (strcmp(argv[2], "MEDIUM") == 0)
	{
		modeRepeat = SHT3X_MEDIUM;
	}
	else if (strcmp(argv[2], "LOW") == 0)
	{
		modeRepeat = SHT3X_LOW;
	}
	else
	{
		return;
	}

	float temp = 0.0f, hum = 0.0f;
	
	// Always try to read sensor, if fails temp/hum remain 0.0
	SHT3X_Single(&g_sht3x, &modeRepeat, &temp, &hum);
	
	// Always update data manager with measurement data (0.0 if sensor failed)
	DataManager_UpdateSingle(temp, hum);
}

/**
 * @brief Command parser for SHT3X periodic measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Periodic_Parser(uint8_t argc, char **argv)
{

	if (argc < 4)
	{
		PRINT_CLI("SHT3X PERIODIC <0.5|1|2|4|10> <HIGH|MEDIUM|LOW>\r\n");
		return;
	}

	sht3x_mode_t modePeriodic;
	
	if (strcmp(argv[2], "0.5") == 0)
	{
		modePeriodic = SHT3X_PERIODIC_05MPS;
	}
	else if (strcmp(argv[2], "1") == 0)
	{
		modePeriodic = SHT3X_PERIODIC_1MPS;
	}
	else if (strcmp(argv[2], "2") == 0)
	{
		modePeriodic = SHT3X_PERIODIC_2MPS;
	}
	else if (strcmp(argv[2], "4") == 0)
	{
		modePeriodic = SHT3X_PERIODIC_4MPS;
	}
	else if (strcmp(argv[2], "10") == 0)
	{
		modePeriodic = SHT3X_PERIODIC_10MPS;
	}
	else
	{
		return;
	}

	sht3x_repeat_t modeRepeat;
	if (strcmp(argv[3], "HIGH") == 0)
	{
		modeRepeat = SHT3X_HIGH;
	}
	else if (strcmp(argv[3], "MEDIUM") == 0)
	{
		modeRepeat = SHT3X_MEDIUM;
	}
	else if (strcmp(argv[3], "LOW") == 0)
	{
		modeRepeat = SHT3X_LOW;
	}
	else
	{
		return;
	}

	float temp = 0.0f, hum = 0.0f;
	
	// Start periodic mode on sensor
	SHT3X_Periodic(&g_sht3x, &modePeriodic, &modeRepeat, &temp, &hum);
	
	// Always update data manager with first measurement (0.0 if sensor failed)
	DataManager_UpdatePeriodic(temp, hum);
	
	// Schedule next fetch regardless of sensor status
	next_fetch_ms = HAL_GetTick() + periodic_interval_ms;
}

/**
 * @brief Command parser for SHT3X ART (Accelerated Response Time) commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
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
 * @brief Command parser for SHT3X stop periodic measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Stop_Periodic_Parser(uint8_t argc, char **argv)
{
	if (SHT3X_Stop_Periodic(&g_sht3x) == SHT3X_OK)
	{
		PRINT_CLI("SHT3X STOP PERIODIC SUCCEEDED\r\n");
	}
	else
	{
		PRINT_CLI("SHT3X STOP PERIODIC FAILED\r\n");
	}
}

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
