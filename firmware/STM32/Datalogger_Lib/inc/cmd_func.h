/**
 * @file cmd_func.h
 *
 * @brief Command Function Definitions
 */

#ifndef CMD_FUNC_H
#define CMD_FUNC_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @brief Command handler function pointer type
 *
 * @param argc Argument count
 * @param argv Argument vector
 *
 * @note argv[0] is the command itself
 */
typedef void (*CmdHandlerFunc)(uint8_t argc, char **argv);

/**
 * @brief Command function structure
 *
 * @details Maps command strings to their handler functions
 */
typedef struct
{
    const char *cmdString; /*!< Command string */
    CmdHandlerFunc func;   /*!< Pointer to command handler function */
} command_function_t;

#endif /* CMD_FUNC_H */
