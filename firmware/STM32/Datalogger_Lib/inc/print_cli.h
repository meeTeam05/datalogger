/**
 * @file print_cli.h
 * 
 * @brief Header file for PRINT_CLI function.
 */

#ifndef PRINT_CLI_H
#define PRINT_CLI_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stm32f1xx_hal.h>

/* DEFINES -------------------------------------------------------------------*/

#define BUFFER_PRINT 128

/* EXTERNAL VARIABLES --------------------------------------------------------*/

extern UART_HandleTypeDef huart1;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Formatted print function over UART
 * 
 * @param fmt Format string (like printf)
 * @param ... Variable arguments
 * 
 * @note Uses HAL_UART_Transmit to send data over UART
 */
void PRINT_CLI(char *fmt, ...);

#endif /* PRINT_CLI_H */
