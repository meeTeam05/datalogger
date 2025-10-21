/**
 * @file sd_card.c
 *
 * @brief SD card interface implementation file
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "sd_card.h"
#include "print_cli.h"

/* PRIVATE VARIABLES --------------------------------------------------------*/

static uint8_t SD_Type = SD_TYPE_UNKNOWN;
static SPI_HandleTypeDef *g_hspi = NULL; /* Global SPI handle */

/* PRIVATE VARIABLES --------------------------------------------------------*/

/**
 * @brief Send command to SD card (assumes CS already controlled externally)
 *
 * @param cmd Command index
 * @param arg Command argument
 * @param crc CRC7 checksum
 *
 * @return uint8_t R1 response byte
 */
static uint8_t SD_SendCommandRaw(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r1;
    uint16_t retry = 0;

    /* Send command byte */
    SD_SPI_ReadWrite(0x40 | cmd);

    /* Send argument (4 bytes) */
    SD_SPI_ReadWrite((uint8_t)(arg >> 24));
    SD_SPI_ReadWrite((uint8_t)(arg >> 16));
    SD_SPI_ReadWrite((uint8_t)(arg >> 8));
    SD_SPI_ReadWrite((uint8_t)arg);

    /* Send CRC */
    SD_SPI_ReadWrite(crc);

    /* Wait for response (R1) - SD card needs up to 8 bytes */
    while ((r1 = SD_SPI_ReadWrite(0xFF)) == 0xFF)
    {
        if (retry++ > 8)
            break; // Max 8 retries for R1 response
    }

    return r1;
}

/* PUBLIC API ---------------------------------------------------------------*/

/**
 * @brief Initialize GPIO pins for SD card
 */
void SD_GPIO_Init(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief Initialize SPI for SD card
 */
uint8_t SD_SPI_Init(SPI_HandleTypeDef *hspi)
{
    if (hspi == NULL || hspi->Instance == NULL)
    {
        return 1;
    }

    /* Store the SPI handle for later use */
    g_hspi = hspi;

    /* Set low speed for initialization phase */
    SD_SetSpeedLow();
    return 0;
}

/**
 * @brief Set low SPI speed (initialization) - prescaler 128 for slow clock
 */
void SD_SetSpeedLow(void)
{
    if (g_hspi == NULL)
        return;

    /* Use MODIFY_REG to safely change baud rate prescaler */
    /* Clears BR bits using SPI_BAUDRATEPRESCALER_256 as mask, then sets new value */
    MODIFY_REG(g_hspi->Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_128);
}

/**
 * @brief Set high SPI speed - prescaler 8 for fast clock
 */
void SD_SetSpeedHigh(void)
{
    if (g_hspi == NULL)
        return;

    /* Use MODIFY_REG to safely change baud rate prescaler */
    /* Clears BR bits using SPI_BAUDRATEPRESCALER_256 as mask, then sets new value */
    MODIFY_REG(g_hspi->Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_8);
}

/**
 * @brief CS pin - High
 */
void SD_CS_High(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief CS pin - Low
 */
void SD_CS_Low(void)
{
    HAL_GPIO_WritePin(SD_CS_PORT, SD_CS_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Send and receive one byte via SPI
 */
uint8_t SD_SPI_ReadWrite(uint8_t data)
{
    uint8_t rx_data = 0;
    if (g_hspi == NULL)
        return 0xFF;
    HAL_SPI_TransmitReceive(g_hspi, &data, &rx_data, 1, 1000);
    return rx_data;
}

/**
 * @brief Send clock cycles
 */
void SD_SendClock(uint8_t count)
{
    while (count--)
    {
        SD_SPI_ReadWrite(0xFF);
    }
}

/**
 * @brief Send command to SD card (with CS control)
 */
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r1;

    /* Select SD card */
    SD_CS_Low();
    SD_SPI_ReadWrite(0xFF); // Send dummy byte

    /* Send command */
    r1 = SD_SendCommandRaw(cmd, arg, crc);

    /* Deselect SD card */
    SD_SPI_ReadWrite(0xFF); // Dummy byte before deselect
    SD_CS_High();
    SD_SPI_ReadWrite(0xFF); // Dummy byte after deselect

    return r1;
}

/**
 * @brief Initialize SD card
 */
uint8_t SD_Init(SPI_HandleTypeDef *hspi)
{
    uint8_t r1, r7, i;
    uint16_t retry;
    uint32_t arg;

    /* Initialize SPI with passed handle */
    if (SD_SPI_Init(hspi) != 0)
    {
        return 1;
    }

    SD_GPIO_Init();

    /* CRITICAL: Power cycle SD card by pulling CS LOW then HIGH */
    /* This helps reset SD card if it was left in unknown state */
    SD_CS_Low();
    HAL_Delay(1);
    SD_CS_High();
    HAL_Delay(50); // Wait for SD card to reset

    /* Send 74+ clock cycles - SD card power-up sequence */
    /* CRITICAL: SD card needs at least 74 clock cycles with CS=HIGH and DI=HIGH */
    SD_CS_High();
    for (uint8_t i = 0; i < 20; i++)
    { // Send 160 clocks (20 bytes * 8 bits) - more than 74 required
        SD_SPI_ReadWrite(0xFF);
    }
    HAL_Delay(10); // Wait 10ms after power-up clocks

    /* CMD0: GO_IDLE_STATE - MUST receive R1_IDLE_STATE (0x01) */
    retry = 0;
    r1 = 0xFF; // Initialize to invalid value
    // PRINT_CLI("[SD] Sending CMD0...\r\n");
    while (r1 != R1_IDLE_STATE && retry < 100)
    {
        r1 = SD_SendCommand(CMD0, 0, 0x95);
        if (r1 == R1_IDLE_STATE)
            break; // Success (0x01)
        if (retry < 5)
        {
            // PRINT_CLI("[SD] CMD0 retry %d, R1=0x%02X\r\n", retry, r1);
        }
        HAL_Delay(10); // Increase delay between retries (was 1ms)
        retry++;
    }

    /* Check if CMD0 succeeded */
    if (r1 != R1_IDLE_STATE)
    {
        // NOTE: Detailed error messages commented out to reduce log spam
        // CMD0 failed - card not in idle state
        // PRINT_CLI("[SD] CMD0 FAILED after %d retries! Last R1=0x%02X\r\n", retry, r1);
        // PRINT_CLI("[SD] Possible causes:\r\n");
        // PRINT_CLI("[SD]  1. No SD card inserted\r\n");
        // PRINT_CLI("[SD]  2. MISO not connected or floating (needs pull-up)\r\n");
        // PRINT_CLI("[SD]  3. Wrong wiring (check CLK, MOSI, MISO, CS)\r\n");
        // PRINT_CLI("[SD]  4. Card not powered (3.3V required)\r\n");
        // PRINT_CLI("[SD]  5. Card not inserted properly\r\n");
        PRINT_CLI("[SD] CMD0 FAILED\r\n");
        return 1; // CMD0 timeout/failure
    }

    PRINT_CLI("[SD] CMD0 OK! Card in IDLE state.\r\n");

    SD_SendClock(2);
    HAL_Delay(1); // Short delay before CMD8

    /* CMD8: Check voltage - detect SD v2.0 card (R7 response = R1 + 4 bytes) */
    SD_CS_Low();
    SD_SPI_ReadWrite(0xFF);
    r1 = SD_SendCommandRaw(CMD8, 0x1AA, 0x87);

    if (r1 == R1_IDLE_STATE)
    {
        /* SD v2.0/SDHC card - Read R7 response (4 bytes) while CS still LOW */
        for (i = 0; i < 4; i++)
        {
            r7 = SD_SPI_ReadWrite(0xFF);
        }
        SD_Type = SD_TYPE_SDV2;
        arg = 0x40000000; // HCS bit = 1 for SDHC support
    }
    else if (r1 == (R1_IDLE_STATE | R1_ILLEGAL_COMMAND))
    {
        /* SD v1.x or MMC - CMD8 not supported, illegal command response */
        SD_Type = SD_TYPE_SDV1;
        arg = 0; // HCS bit = 0 for SD v1.x
    }
    else
    {
        /* Unexpected response - treat as SD v1.x */
        SD_Type = SD_TYPE_SDV1;
        arg = 0;
    }

    /* Deselect after CMD8 */
    SD_SPI_ReadWrite(0xFF);
    SD_CS_High();
    SD_SPI_ReadWrite(0xFF);

    SD_SendClock(2);
    HAL_Delay(1); // Delay before ACMD41

    /* ACMD41: Send Operating Condition - MUST keep CS LOW between CMD55 and CMD41 */
    retry = 0;
    r1 = 0xFF; // Initialize to invalid
    while (r1 != R1_READY && retry < 200)
    {
        /* Send CMD55 (prefix for ACMD) */
        SD_CS_Low();
        SD_SPI_ReadWrite(0xFF);
        r1 = SD_SendCommandRaw(CMD55, 0, 0);

        /* Send CMD41 immediately (keep CS LOW) */
        r1 = SD_SendCommandRaw(CMD41, arg, 0);

        /* Deselect after both commands */
        SD_SPI_ReadWrite(0xFF);
        SD_CS_High();
        SD_SPI_ReadWrite(0xFF);

        if (r1 == R1_READY)
            break;     // Success - card initialized
        HAL_Delay(10); // Add 10ms delay between retries
        retry++;
    }

    /* Check if ACMD41 succeeded */
    if (r1 != R1_READY)
    {
        // ACMD41 failed - card not ready after 2 seconds
        // This usually means:
        // 1. MISO shorted or not connected
        // 2. Voltage issue (should be 3.3V)
        // 3. Faulty card
        return 2; // ACMD41 timeout
    }

    /* CMD58: Read OCR (R3 response = R1 + 4 bytes OCR) */
    SD_CS_Low();
    SD_SPI_ReadWrite(0xFF);
    r1 = SD_SendCommandRaw(CMD58, 0, 0);
    if (r1 != R1_READY)
    {
        SD_CS_High();
        return 3;
    }

    /* Read OCR (4 bytes) while CS still LOW */
    for (i = 0; i < 4; i++)
    {
        r7 = SD_SPI_ReadWrite(0xFF);
        if (i == 0 && (r7 & 0x40))
        {
            SD_Type = SD_TYPE_SDHC;
        }
    }

    /* Deselect after CMD58 */
    SD_SPI_ReadWrite(0xFF);
    SD_CS_High();
    SD_SPI_ReadWrite(0xFF);

    SD_SendClock(2);

    SD_SetSpeedHigh();

    return 0;
}

/**
 * @brief Read a block from SD card
 */
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer)
{
    uint16_t i, retry;
    uint8_t r1;

    if (SD_Type != SD_TYPE_SDHC)
    {
        block_addr <<= 9;
    }

    /* Send CMD17 (READ_SINGLE_BLOCK) and keep CS LOW for entire operation */
    SD_CS_Low();
    SD_SPI_ReadWrite(0xFF); // Dummy byte before command

    r1 = SD_SendCommandRaw(CMD17, block_addr, 0);
    if (r1 != R1_READY)
    {
        SD_SPI_ReadWrite(0xFF);
        SD_CS_High();
        return 1; /* Command failed */
    }

    /* Wait for data token (0xFE) - SD card needs time to read from flash */
    retry = 0;
    while (SD_SPI_ReadWrite(0xFF) != 0xFE)
    {
        if (retry++ > 10000)
        {
            SD_CS_High();
            return 2; /* Timeout waiting for data token */
        }
    }

    /* Read 512 bytes (CS still LOW) */
    for (i = 0; i < SD_BLOCK_SIZE; i++)
    {
        buffer[i] = SD_SPI_ReadWrite(0xFF);
    }

    /* Read and ignore CRC (2 bytes) */
    SD_SPI_ReadWrite(0xFF);
    SD_SPI_ReadWrite(0xFF);

    /* Deselect card after read complete */
    SD_SPI_ReadWrite(0xFF);
    SD_CS_High();
    SD_SPI_ReadWrite(0xFF);

    return 0;
}

/**
 * @brief Write a block to SD card
 */
uint8_t SD_WriteBlock(uint32_t block_addr, uint8_t *buffer)
{
    uint16_t i, retry;
    uint8_t r1, response;

    if (SD_Type != SD_TYPE_SDHC)
    {
        block_addr <<= 9;
    }

    /* Send CMD24 (WRITE_BLOCK) and keep CS LOW for entire operation */
    SD_CS_Low();
    SD_SPI_ReadWrite(0xFF); // Dummy byte before command

    r1 = SD_SendCommandRaw(CMD24, block_addr, 0);
    if (r1 != R1_READY)
    {
        SD_SPI_ReadWrite(0xFF);
        SD_CS_High();
        return 1; /* Command failed */
    }

    /* Wait for SD card to prepare (CS still LOW) */
    SD_SPI_ReadWrite(0xFF);
    SD_SPI_ReadWrite(0xFF);

    /* Send data start token (CS still LOW) */
    SD_SPI_ReadWrite(0xFE);

    /* Write 512 bytes (CS still LOW) */
    for (i = 0; i < SD_BLOCK_SIZE; i++)
    {
        SD_SPI_ReadWrite(buffer[i]);
    }

    /* Send dummy CRC (2 bytes) - CRC disabled in init, but must send dummy */
    SD_SPI_ReadWrite(0xFF);
    SD_SPI_ReadWrite(0xFF);

    /* Read data response token (should be 0x05 = accepted) */
    retry = 0;
    response = SD_SPI_ReadWrite(0xFF);
    while (response == 0xFF && retry++ < 100)
    {
        response = SD_SPI_ReadWrite(0xFF);
    }

    /* Check if write was accepted */
    if ((response & 0x1F) != 0x05)
    {
        SD_SPI_ReadWrite(0xFF);
        SD_CS_High();
        return 2; /* Write rejected - data response token error */
    }

    /* Wait for write completion (card will send 0x00 while busy, then 0xFF when done) */
    retry = 0;
    while (SD_SPI_ReadWrite(0xFF) == 0x00)
    {
        if (retry++ > 50000)
        {
            SD_CS_High();
            return 3; /* Timeout waiting for write to complete */
        }
    }

    /* Deselect card after write complete */
    SD_SPI_ReadWrite(0xFF);
    SD_CS_High();
    SD_SPI_ReadWrite(0xFF);

    return 0;
}

/**
 * @brief Get SD card type
 */
uint8_t SD_GetType(void)
{
    return SD_Type;
}
