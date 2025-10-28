/**
 * @file cmd_execute.h
 *
 * @brief Header file for command execution functions.
 */

#ifndef CMD_EXECUTE_H
#define CMD_EXECUTE_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Executes a command from the command buffer.
 *
 * @param commandBuffer Pointer to the command string buffer.
 */
void COMMAND_EXECUTE(char *commandBuffer);

#endif /* CMD_EXECUTE_H */
