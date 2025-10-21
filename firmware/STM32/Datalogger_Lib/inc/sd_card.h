/**
 * @file sd_card.h
 *
 * @brief SD card interface header file
 */

#ifndef SD_CARD_H
#define SD_CARD_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stm32f1xx_hal.h>

/* DEFINES -------------------------------------------------------------------*/

/* SD Card Command Definitions */
#define CMD0 0   /* GO_IDLE_STATE */
#define CMD1 1   /* SEND_OP_COND */
#define CMD8 8   /* SEND_IF_COND */
#define CMD9 9   /* SEND_CSD */
#define CMD10 10 /* SEND_CID */
#define CMD12 12 /* STOP_TRANSMISSION */
#define CMD13 13 /* SEND_STATUS */
#define CMD17 17 /* READ_SINGLE_BLOCK */
#define CMD18 18 /* READ_MULTIPLE_BLOCK */
#define CMD24 24 /* WRITE_BLOCK */
#define CMD25 25 /* WRITE_MULTIPLE_BLOCK */
#define CMD41 41 /* SEND_OP_COND (ACMD) */
#define CMD55 55 /* APP_CMD */
#define CMD58 58 /* READ_OCR */

/* Response Types */
#define R1_LENGTH 1
#define R3_LENGTH 5
#define R7_LENGTH 5

/* R1 Response Flags */
#define R1_IDLE_STATE 0x01
#define R1_ERASE_RESET 0x02
#define R1_ILLEGAL_COMMAND 0x04
#define R1_COM_CRC_ERROR 0x08
#define R1_ERASE_SEQ_ERROR 0x10
#define R1_ADDRESS_ERROR 0x20
#define R1_PARAM_ERROR 0x40
#define R1_READY 0x00

/* SD Card Type */
#define SD_TYPE_UNKNOWN 0
#define SD_TYPE_MMC 1
#define SD_TYPE_SDV1 2
#define SD_TYPE_SDV2 3
#define SD_TYPE_SDHC 4

/* Block Size */
#define SD_BLOCK_SIZE 512

/* SPI Configuration */
#define SD_CS_PORT GPIOA
#define SD_CS_PIN GPIO_PIN_4 // Must match main.h SD_CS_Pin definition

/* PUBLIC API ----------------------------------------------------------------*/

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief SD Card GPIO Initialization
 */
void SD_GPIO_Init(void);

/**
 * @brief SD Card SPI Initialization
 *
 * @param *hspi Pointer to SPI handle
 *
 * @return uint8_t 0 = success, non-zero = error code
 */
uint8_t SD_SPI_Init(SPI_HandleTypeDef *hspi);

/**
 * @brief Set SPI to low speed (initialization)
 */
void SD_SetSpeedLow(void);

/**
 * @brief Set SPI to high speed
 */
void SD_SetSpeedHigh(void);

/**
 * @brief Initialize SD card
 *
 * @param *hspi Pointer to SPI handle
 *
 * @return uint8_t 0 = success, non-zero = error code
 */
uint8_t SD_Init(SPI_HandleTypeDef *hspi);

/**
 * @brief Send command to SD card
 *
 * @param cmd Command index
 * @param arg Command argument
 * @param crc CRC7 checksum
 *
 * @return uint8_t R1 response byte
 */
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc);

/**
 * @brief Read a block from SD card
 *
 * @param block_addr Block address to read
 * @param *buffer Pointer to buffer to store read data (must be at least 512 bytes)
 *
 * @return uint8_t 0 = success, non-zero = error code
 */
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer);

/**
 * @brief Write a block to SD card
 *
 * @param block_addr Block address to write
 * @param *buffer Pointer to buffer containing data to write (must be at least 512 bytes)
 *
 * @return uint8_t 0 = success, non-zero = error code
 */
uint8_t SD_WriteBlock(uint32_t block_addr, uint8_t *buffer);

/**
 * @brief Get SD card type
 *
 * @return uint8_t SD card type (SD_TYPE_*)
 */
uint8_t SD_GetType(void);

/* SPI Helper Functions ------------------------------------------------------*/

/**
 * @brief SPI Read/Write a single byte
 *
 * @param data Byte to send
 *
 * @return uint8_t Received byte
 */
uint8_t SD_SPI_ReadWrite(uint8_t data);

/* CS Control Functions ------------------------------------------------------*/

/**
 * @brief Set SD card CS pin high
 */
void SD_CS_High(void);

/**
 * @brief Set SD card CS pin low
 */
void SD_CS_Low(void);

/**
 * @brief Send specified number of clock cycles (dummy bytes) to SD card
 *
 * @param count Number of bytes (8 clocks each) to send
 */
void SD_SendClock(uint8_t count);

#endif /* SD_CARD_H */
