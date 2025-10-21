/**
 * @file sd_card_manager.h
 *
 * @brief SD Card Manager - Handles buffering and storing sensor data to SD card
 */

#ifndef SD_CARD_MANAGER_H
#define SD_CARD_MANAGER_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include "sd_card.h"

/* DEFINES -------------------------------------------------------------------*/

/* Configuration */
#define SD_BUFFER_SIZE 204800 // Max number of records to buffer
#define SD_DATA_BLOCK 1       // SD block address to store buffer metadata
#define SD_DATA_START_BLOCK 2 // Starting block for actual data

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @brief Sensor data record structure (512 bytes - ONE SD BLOCK)
 */
typedef struct
{
    uint32_t timestamp;    // Unix timestamp (4 bytes)
    float temperature;     // Temperature in Celsius (4 bytes)
    float humidity;        // Humidity in percentage (4 bytes)
    char mode[16];         // "SINGLE" or "PERIODIC" (16 bytes)
    uint32_t sequence_num; // Sequence number (4 bytes)
    uint8_t padding[480];  // Padding to 512 bytes (512 - 32 = 480)
} sd_data_record_t;

/**
 * @brief Buffer metadata structure
 */
typedef struct
{
    uint32_t write_index;  // Next write position (0-204799)
    uint32_t read_index;   // Next read position (0-204799)
    uint32_t count;        // Number of valid records
    uint32_t sequence_num; // Global sequence counter
} sd_buffer_metadata_t;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize SD Card Manager
 *
 * @return true if SD card initialized successfully, false otherwise
 */
bool SDCardManager_Init(void);

/**
 * @brief Write sensor data to SD card buffer
 *
 * @param timestamp Unix timestamp
 * @param temperature Temperature value
 * @param humidity Humidity value
 * @param mode_str "SINGLE" or "PERIODIC"
 *
 * @return true if data was buffered, false if buffer is full or SD error
 */
bool SDCardManager_WriteData(uint32_t timestamp, float temperature, float humidity, const char *mode_str);

/**
 * @brief Read next buffered data record from SD card
 *
 * @param record Pointer to buffer where data will be read into
 *
 * @return true if a record was read, false if no more records available
 */
bool SDCardManager_ReadData(sd_data_record_t *record);

/**
 * @brief Get number of buffered records waiting to be sent
 *
 * @return Number of records in buffer (0 to SD_BUFFER_SIZE)
 */
uint32_t SDCardManager_GetBufferedCount(void);

/**
 * @brief Remove (mark as sent) the last read record
 *
 * @return true if record was removed, false if no records to remove
 */
bool SDCardManager_RemoveRecord(void);

/**
 * @brief Clear all buffered data (use with caution!)
 *
 * @return true if cleared successfully
 */
bool SDCardManager_ClearBuffer(void);

/**
 * @brief Check if SD card is initialized and ready
 *
 * @return true if SD card is ready, false otherwise
 */
bool SDCardManager_IsReady(void);

/**
 * @brief Get last error code
 *
 * @return Error code from last SD operation
 */
uint8_t SDCardManager_GetLastError(void);

#endif /* SD_CARD_MANAGER_H */
