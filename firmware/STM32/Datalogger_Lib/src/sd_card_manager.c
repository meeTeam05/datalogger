/**
 * @file sd_card_manager.c
 *
 * @brief SD Card Manager - Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <string.h>
#include "sd_card_manager.h"
#include "print_cli.h"

/* EXTERNAL VARIABLES -------------------------------------------------------*/

/* External SPI handle from main.c */
extern SPI_HandleTypeDef hspi1;

/* PRIVATE VARIABLES --------------------------------------------------------*/

/* Private variables */
static bool sd_initialized = false;
static sd_buffer_metadata_t g_metadata = {0};
static uint8_t sd_last_error = 0;

/* PRIVATE FUNCTIONS --------------------------------------------------------*/

/**
 * @brief Read buffer metadata from SD card
 *
 * @return true if successful, false otherwise
 */
static bool _read_metadata(void)
{
    uint8_t buffer[512] = {0};
    uint8_t ret = SD_ReadBlock(SD_DATA_BLOCK, buffer);
    if (ret != 0)
    {
        sd_last_error = ret;
        return false;
    }

    memcpy(&g_metadata, buffer, sizeof(sd_buffer_metadata_t));
    return true;
}

/**
 * @brief Write buffer metadata to SD card
 *
 * @return true if successful, false otherwise
 */
static bool _write_metadata(void)
{
    uint8_t buffer[512] = {0};
    memcpy(buffer, &g_metadata, sizeof(sd_buffer_metadata_t));

    uint8_t ret = SD_WriteBlock(SD_DATA_BLOCK, buffer);
    if (ret != 0)
    {
        sd_last_error = ret;
        return false;
    }

    return true;
}

/**
 * @brief Get SD card block address for given record index
 *
 * @param index Record index
 *
 * @return SD card block address
 */
static uint32_t _get_data_block_addr(uint32_t index)
{
    return SD_DATA_START_BLOCK + (index % SD_BUFFER_SIZE);
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize SD Card Manager
 */
bool SDCardManager_Init(void)
{
    PRINT_CLI("[SD] Init...\r\n");

    // Initialize SD card with SPI1 handle
    uint8_t ret = SD_Init(&hspi1);
    if (ret != 0)
    {
        sd_last_error = ret;
        sd_initialized = false;
        PRINT_CLI("[SD] Init FAILED (err=%d)\r\n", ret);
        return false;
    }

    // Try to load existing metadata from SD card
    if (!_read_metadata())
    {
        // If metadata doesn't exist, initialize it fresh
        memset(&g_metadata, 0, sizeof(sd_buffer_metadata_t));
        g_metadata.write_index = 0;
        g_metadata.read_index = 0;
        g_metadata.count = 0;
        g_metadata.sequence_num = 0;

        // Write initial metadata to SD card
        if (!_write_metadata())
        {
            sd_initialized = false;
            PRINT_CLI("[SD] Metadata write FAILED (err=%d)\r\n", sd_last_error);
            return false;
        }
        PRINT_CLI("[SD] New buffer created\r\n");
    }
    else
    {
        // Warn if buffer is full
        if (g_metadata.count >= SD_BUFFER_SIZE)
        {
            PRINT_CLI("[SD] WARNING: Buffer FULL (%lu/%d) - oldest will be overwritten\r\n",
                      (unsigned long)g_metadata.count, SD_BUFFER_SIZE);
        }
    }

    sd_initialized = true;
    PRINT_CLI("[SD] Ready | Buffered: %lu/%d\r\n",
              (unsigned long)g_metadata.count, SD_BUFFER_SIZE);
    return true;
}

/**
 * @brief Write sensor data to SD card buffer
 */
bool SDCardManager_WriteData(uint32_t timestamp, float temperature, float humidity, const char *mode_str)
{
    if (!sd_initialized)
    {
        PRINT_CLI("[SD] Write FAILED - Not initialized\r\n");
        return false;
    }

    // Check if buffer is full - implement circular buffer (overwrite oldest)
    if (g_metadata.count >= SD_BUFFER_SIZE)
    {
        PRINT_CLI("[SD] Buffer FULL - overwriting oldest\r\n");
        // Move read pointer forward to discard oldest record
        g_metadata.read_index = (g_metadata.read_index + 1) % SD_BUFFER_SIZE;
        g_metadata.count--; // Decrease count to make room for new record
    }

    // Create record
    sd_data_record_t record = {0};
    record.timestamp = timestamp;
    record.temperature = temperature;
    record.humidity = humidity;
    record.sequence_num = g_metadata.sequence_num++;
    strncpy(record.mode, mode_str, sizeof(record.mode) - 1);

    // Get block address for this record
    uint32_t block_addr = _get_data_block_addr(g_metadata.write_index);

    // Write record to SD card
    uint8_t ret = SD_WriteBlock(block_addr, (uint8_t *)&record);
    if (ret != 0)
    {
        sd_last_error = ret;
        PRINT_CLI("[SD] Write FAILED (err=%d)\r\n", ret);
        return false;
    }

    // Update metadata
    g_metadata.write_index = (g_metadata.write_index + 1) % SD_BUFFER_SIZE;
    g_metadata.count++;

    // Save metadata
    if (!_write_metadata())
    {
        PRINT_CLI("[SD] Metadata save FAILED (err=%d)\r\n", sd_last_error);
        return false;
    }

    PRINT_CLI("[SD] Saved: T=%.1fC H=%.1f%% [%s] | Buffer: %lu/%d\r\n",
              temperature, humidity, mode_str,
              (unsigned long)g_metadata.count, SD_BUFFER_SIZE);
    return true;
}

/**
 * @brief Read next buffered data record from SD card
 */
bool SDCardManager_ReadData(sd_data_record_t *record)
{
    if (!sd_initialized || !record)
    {
        PRINT_CLI("[SD] Read FAILED - Invalid state\r\n");
        return false;
    }

    // Check if there are records to read
    if (g_metadata.count == 0)
    {
        return false;
    }

    // Get block address for next read
    uint32_t block_addr = _get_data_block_addr(g_metadata.read_index);

    // Read record from SD card (512 bytes - ONE FULL SD BLOCK)
    uint8_t ret = SD_ReadBlock(block_addr, (uint8_t *)record);
    if (ret != 0)
    {
        sd_last_error = ret;
        PRINT_CLI("[SD] Read FAILED (err=%d)\r\n", ret);
        return false;
    }

    return true;
}

/**
 * @brief Get number of buffered records waiting to be sent
 */
uint32_t SDCardManager_GetBufferedCount(void)
{
    if (!sd_initialized)
        return 0;
    return g_metadata.count;
}

/**
 * @brief Remove the oldest record from the buffer after successful sending
 */
bool SDCardManager_RemoveRecord(void)
{
    if (!sd_initialized || g_metadata.count == 0)
        return false;

    // Update metadata IN RAM ONLY (defer SD write for performance)
    g_metadata.read_index = (g_metadata.read_index + 1) % SD_BUFFER_SIZE;
    g_metadata.count--;

    // Save metadata to SD card
    // NOTE: This writes metadata after EVERY record removal
    // For better performance, could batch updates, but trades off crash recovery
    return _write_metadata();
}

/**
 * @brief Clear entire buffer
 */
bool SDCardManager_ClearBuffer(void)
{
    if (!sd_initialized)
        return false;

    // Reset metadata
    g_metadata.write_index = 0;
    g_metadata.read_index = 0;
    g_metadata.count = 0;
    g_metadata.sequence_num = 0;

    // Save metadata
    return _write_metadata();
}

/**
 * @brief Check if SD Card Manager is initialized
 */
bool SDCardManager_IsReady(void)
{
    return sd_initialized;
}

/**
 * @brief Get last SD card error code
 */
uint8_t SDCardManager_GetLastError(void)
{
    return sd_last_error;
}