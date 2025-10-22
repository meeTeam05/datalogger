/**
 * @file cmd_parser.h
 *
 * @brief Command Parser Header
 */

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>

/* EXTERNAL VARIABLES --------------------------------------------------------*/

/**
 * @brief External variable for periodic timing control
 * @note Allows cmd_parser to reset timing when starting PERIODIC mode
 */
extern uint32_t next_fetch_ms;

/**
 * @brief External variable for periodic interval configuration
 * @note Allows cmd_parser to access and modify the periodic measurement interval
 */
extern uint32_t periodic_interval_ms;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Command parser for SD CLEAR command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself.
 */
void CHECK_UART_STATUS(uint8_t argc, char **argv);

/**
 * @brief Command parser for SHT3X heater commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Heater_Parser(uint8_t argc, char **argv);

/**
 * @brief Command parser for SHT3X ART (Accelerated Response Time) commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_ART_Parser(uint8_t argc, char **argv);

/**
 * @brief Command parser for DS3231 set time input commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void DS3231_Set_Time_Parser(uint8_t argc, char **argv);

/**
 * @brief Command parser for SINGLE measurement command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SINGLE_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for PERIODIC ON command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void PERIODIC_ON_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for PERIODIC OFF command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void PERIODIC_OFF_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for SET TIME command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SET_TIME_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for SET PERIODIC INTERVAL command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SET_PERIODIC_INTERVAL_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for MQTT CONNECTED notification
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void MQTT_CONNECTED_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for MQTT DISCONNECTED notification
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void MQTT_DISCONNECTED_PARSER(uint8_t argc, char **argv);

/**
 * @brief Command parser for SD CLEAR command
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself. Clears all buffered data on SD card.
 */
void SD_CLEAR_PARSER(uint8_t argc, char **argv);

#endif /* CMD_PARSER_H */
