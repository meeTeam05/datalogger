/**
 * @file cmd_func.c
 *
 * @brief Command Function Implementations
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stddef.h>
#include "cmd_func.h"
#include "cmd_parser.h"

/* VARIABLES -----------------------------------------------------------------*/

/**
 * @brief Command function table
 *
 * @details Maps command strings to their handler functions
 *
 * @note The table is terminated by an entry with NULL values.
 */
command_function_t cmdTable[] = {
	{.cmdString = "CHECK UART STATUS", // Check UART status for debugging
	 .func = CHECK_UART_STATUS},

	{.cmdString = "SHT3X HEATER ENABLE", // Enable the heater for Debugging
	 .func = SHT3X_Heater_Parser},

	{.cmdString = "SHT3X HEATER DISABLE", // Disable the heater for Debugging
	 .func = SHT3X_Heater_Parser},

	{.cmdString = "SHT3X ART", // Mode SHT3X for Debugging
	 .func = SHT3X_ART_Parser},

	{.cmdString = "DS3231 SET TIME", // Mode DS3231 Set Time for Debugging
	 .func = DS3231_Set_Time_Parser},

	{.cmdString = "SINGLE", // Single Measurement Command
	 .func = SINGLE_PARSER},

	{.cmdString = "PERIODIC ON", // Periodic Measurement ON Command
	 .func = PERIODIC_ON_PARSER},

	{.cmdString = "PERIODIC OFF", // Periodic Measurement OFF Command
	 .func = PERIODIC_OFF_PARSER},

	{.cmdString = "SET TIME", // Set Time Command
	 .func = SET_TIME_PARSER},

	{.cmdString = "SET PERIODIC INTERVAL", // Set Periodic Print Interval Command
	 .func = SET_PERIODIC_INTERVAL_PARSER},

	{.cmdString = "MQTT CONNECTED", // MQTT Connected Notification
	 .func = MQTT_CONNECTED_PARSER},

	{.cmdString = "MQTT DISCONNECTED", // MQTT Disconnected Notification
	 .func = MQTT_DISCONNECTED_PARSER},

	{.cmdString = "SD CLEAR", // Clear SD card buffer
	 .func = SD_CLEAR_PARSER},

	{.cmdString = NULL, .func = NULL}, // Table terminator

};
