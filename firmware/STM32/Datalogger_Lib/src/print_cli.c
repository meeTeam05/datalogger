/**
 * @file print_cli.c
 *
 * @brief Formatted print function over UART
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <stdarg.h>
#include <stdio.h>
#include "print_cli.h"

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Formatted print function over UART
 */
void PRINT_CLI(char *fmt, ...)
{
	char stringBuffer[BUFFER_PRINT];
	va_list args;
	va_start(args, fmt);
	uint8_t len_str = vsprintf(stringBuffer, fmt, args);
	va_end(args);

	if (len_str > 0)
	{
		HAL_UART_Transmit(&huart1, (uint8_t *)stringBuffer, len_str, 100);
	}
}
