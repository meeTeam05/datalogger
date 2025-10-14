/**
 * @file cmd_parser.h
 *
 * @brief Command Parser Header
 */

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Command parser function for unknown commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void Cmd_Default(uint8_t argc, char **argv);

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
 * @brief Command parser for SHT3X single measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Single_Parser(uint8_t argc, char **argv);

/**
 * @brief Command parser for SHT3X periodic measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Periodic_Parser(uint8_t argc, char **argv);

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
 * @brief Command parser for SHT3X stop periodic measurement commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void SHT3X_Stop_Periodic_Parser(uint8_t argc, char **argv);

/**
 * @brief Command parser for DS3231 set time input commands
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
void DS3231_Set_Time_Parser(uint8_t argc, char **argv);

/* EXTERNAL VARIABLES --------------------------------------------------------*/

/**
 * @brief External variable for periodic timing control
 * @note Allows cmd_parser to reset timing when starting PERIODIC mode
 */
extern uint32_t next_fetch_ms;

/**
 * @brief External variable for periodic interval in milliseconds
 * @note Set by cmd_parser based on the periodic mode (0.5/1/2/4/10 MPS)
 */
extern uint32_t periodic_interval_ms;

#endif /* CMD_PARSER_H */
