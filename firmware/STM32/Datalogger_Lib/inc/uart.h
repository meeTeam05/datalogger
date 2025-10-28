/**
 * @file uart.h
 *
 * @brief Header file for UART communication handling
 */

#ifndef UART_H
#define UART_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stm32f1xx_hal.h>

/* DEFINES -------------------------------------------------------------------*/

// Define buffer size for UART
#define BUFFER_UART 128

/* EXTERNAL VARIABLES --------------------------------------------------------*/

// Global UART handle (to be defined in main.c)
extern UART_HandleTypeDef huart1;

// Received data byte
extern uint8_t data_rx;

// Ring buffer for UART reception
extern uint8_t buff[BUFFER_UART];
extern uint8_t index_uart;
extern uint8_t Flag_UART;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize UART for receiving data with interrupt
 *
 * @param huart Pointer to UART handle
 *
 * @details This function initializes the UART peripheral to receive data
 *          using interrupts. It also initializes the ring buffer used for
 *          storing received data.
 */
void UART_Init(UART_HandleTypeDef *huart);

/**
 * @brief UART receive complete callback
 *
 * @param huart Pointer to UART handle
 *
 * @details This function is called when a byte is received via UART.
 *          It stores the received byte in the ring buffer and re-enables
 *          the interrupt for the next byte.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief Handle received UART data
 *
 * @details This function processes the data received via UART.
 *          It checks for end-of-line characters and sets a flag
 *          when a complete command is received. The command is then
 *          executed and the buffer is cleared for the next command.
 */
void UART_Handle(void);

#endif /* UART_H */
