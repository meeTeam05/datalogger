/**
 * @file command_execute.c
 *
 * @brief Implementation of command execution functions.
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <string.h>
#include "command_execute.h"
#include "cmd_func.h"
#include "cmd_parser.h"
#include <stm32f1xx_hal.h>
#include <stdio.h>

/* VARIABLES -----------------------------------------------------------------*/

// Extern command table defined in cmd_func.c
extern command_function_t cmdTable[];

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Tokenizes the input string into arguments.
 *
 * @param str The input string to tokenize.
 * @param argv Array to hold pointers to the tokens.
 * @param max_tokens Maximum number of tokens to extract.
 *
 * @return The number of tokens extracted.
 *
 * @note This function modifies the input string by inserting null terminators.
 */
static uint8_t tokenize_string(char *str, char **argv, uint8_t max_tokens)
{
    uint8_t argc = 0;
    char *token = strtok(str, " \t\r\n");

    while (token != NULL && argc < max_tokens)
    {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    return argc;
}

/**
 * @brief Finds a command in the command table by matching prefix.
 *
 * @param argc Number of arguments.
 * @param argv Array of argument strings.
 *
 * @return Pointer to the command_function_t if found, NULL otherwise.
 */
static command_function_t *find_command(uint8_t argc, char **argv)
{
    if (argc == 0)
        return NULL;

    for (uint8_t i = 0; cmdTable[i].cmdString != NULL; i++)
    {
        // Build command string from argv to match against table
        char testCmd[256] = {0};
        uint8_t cmdTokens = 0;

        // Count tokens in table command
        char tableCmdCopy[256];
        strncpy(tableCmdCopy, cmdTable[i].cmdString, sizeof(tableCmdCopy) - 1);
        tableCmdCopy[sizeof(tableCmdCopy) - 1] = '\0';

        char *token = strtok(tableCmdCopy, " ");
        while (token != NULL)
        {
            cmdTokens++;
            token = strtok(NULL, " ");
        }

        // Build test string from argv with same number of tokens
        if (cmdTokens > argc)
            continue;

        for (uint8_t j = 0; j < cmdTokens; j++)
        {
            strcat(testCmd, argv[j]);
            if (j < cmdTokens - 1)
                strcat(testCmd, " ");
        }

        // Compare
        if (strcmp(cmdTable[i].cmdString, testCmd) == 0)
        {
            return &cmdTable[i];
        }
    }
    return NULL;
}

/* PUBLIC API ----------------------------------------------------------------*/

void COMMAND_EXECUTE(char *commandBuffer)
{
    if (commandBuffer == NULL)
        return;

    char buffer[256];
    strncpy(buffer, commandBuffer, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    char *argv[20]; // Increased to support more arguments
    uint8_t argc = tokenize_string(buffer, argv, 20);

    if (argc == 0)
        return;

    command_function_t *command = find_command(argc, argv);

    command->func(argc, argv);
}
