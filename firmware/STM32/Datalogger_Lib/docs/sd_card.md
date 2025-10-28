# SD Card Low-Level Driver Library

Implements SPI-based low-level communication and control for SD card operations.

## Overview

The SD Card Low-Level Driver Library provides complete SPI-based SD card interface for the Datalogger system. It implements the SD card protocol at the physical layer, supporting initialization, block read/write operations, and command handling.

## Files

- **sd_card.c**: SD card driver implementation
- **sd_card.h**: SD card API declarations and definitions

## Hardware Configuration

### SD Card Specifications

- **Interface**: SPI (4-wire)
- **Supported Cards**: SD (SDSC), SDHC, SDXC, MMC
- **Block Size**: 512 bytes (fixed)
- **Maximum Capacity**: 2 TB (SDXC)
- **Voltage**: 3.3V

### SPI Connection

| STM32 Pin | SD Card Pin | Description          |
| --------- | ----------- | -------------------- |
| SPI_SCK   | CLK         | Clock                |
| SPI_MOSI  | MOSI / CMD  | Master Out, Slave In |
| SPI_MISO  | MISO / DAT0 | Master In, Slave Out |
| GPIO_CS   | CS / DAT3   | Chip Select          |
| 3.3V      | VDD         | Power Supply         |
| GND       | VSS         | Ground               |

### Pin Definitions

```c
#define SD_CS_PORT    GPIOA
#define SD_CS_PIN     GPIO_PIN_4
```

**Chip Select Control**:

- Low (0): SD card selected
- High (1): SD card deselected

## SD Card Types

### Type Detection

```c
#define SD_TYPE_UNKNOWN  0  // Not initialized
#define SD_TYPE_MMC      1  // MultiMediaCard
#define SD_TYPE_SDV1     2  // SD Card Version 1.x (SDSC, ≤2GB)
#define SD_TYPE_SDV2     3  // SD Card Version 2.0 (SDSC, ≤2GB)
#define SD_TYPE_SDHC     4  // SD High Capacity (SDHC/SDXC, >2GB)
```

**Addressing**:

- **SDV1/SDV2**: Byte addressing (address = byte offset)
- **SDHC/SDXC**: Block addressing (address = block number)

### Get Card Type

```c
uint8_t SD_GetType(void);
```

Returns the detected SD card type.

**Usage Example**:

```c
uint8_t cardType = SD_GetType();
if (cardType == SD_TYPE_SDHC)
{
    printf("SDHC card detected\n");
}
```

## SD Card Commands

### Command Format

SD commands are 6 bytes:

```
Byte 0: 01xxxxxx (command index with start bits)
Byte 1-4: Argument (32-bit, MSB first)
Byte 5: CRC7 + stop bit
```

### Supported Commands

```c
#define CMD0    0   // GO_IDLE_STATE - Reset card
#define CMD1    1   // SEND_OP_COND - Initialize MMC
#define CMD8    8   // SEND_IF_COND - Check voltage range (SDV2+)
#define CMD9    9   // SEND_CSD - Read card-specific data
#define CMD10   10  // SEND_CID - Read card identification
#define CMD12   12  // STOP_TRANSMISSION - Stop multi-block read/write
#define CMD13   13  // SEND_STATUS - Read card status
#define CMD17   17  // READ_SINGLE_BLOCK - Read one block
#define CMD18   18  // READ_MULTIPLE_BLOCK - Read multiple blocks
#define CMD24   24  // WRITE_BLOCK - Write one block
#define CMD25   25  // WRITE_MULTIPLE_BLOCK - Write multiple blocks
#define CMD41   41  // SEND_OP_COND (ACMD) - Initialize SD card
#define CMD55   55  // APP_CMD - Prefix for application commands
#define CMD58   58  // READ_OCR - Read operating conditions
```

### Send Command Function

```c
uint8_t SD_SendCommand(uint8_t cmd, uint32_t arg, uint8_t crc);
```

Sends a command to the SD card and returns R1 response.

**Parameters**:

- `cmd`: Command index (0-63)
- `arg`: 32-bit argument
- `crc`: CRC7 checksum (only required for CMD0 and CMD8)

**Returns**: R1 response byte

**Usage Example**:

```c
// Send CMD0 (GO_IDLE_STATE)
uint8_t response = SD_SendCommand(CMD0, 0, 0x95);
if (response == R1_IDLE_STATE)
{
    printf("Card entered idle state\n");
}
```

## R1 Response Flags

### Response Byte Format

```
Bit 7: 0 (always 0)
Bit 6: Parameter error
Bit 5: Address error
Bit 4: Erase sequence error
Bit 3: CRC error
Bit 2: Illegal command
Bit 1: Erase reset
Bit 0: In idle state
```

### Response Flags

```c
#define R1_READY           0x00  // Card ready
#define R1_IDLE_STATE      0x01  // Card in idle state
#define R1_ERASE_RESET     0x02  // Erase reset
#define R1_ILLEGAL_COMMAND 0x04  // Illegal command
#define R1_COM_CRC_ERROR   0x08  // CRC error
#define R1_ERASE_SEQ_ERROR 0x10  // Erase sequence error
#define R1_ADDRESS_ERROR   0x20  // Address error
#define R1_PARAM_ERROR     0x40  // Parameter error
```

**Checking Response**:

```c
uint8_t r1 = SD_SendCommand(CMD17, 0, 0x00);
if (r1 & R1_ILLEGAL_COMMAND)
{
    printf("Illegal command\n");
}
if (r1 == R1_READY)
{
    printf("Command accepted\n");
}
```

## Initialization

### SD Card Initialization Function

```c
uint8_t SD_Init(SPI_HandleTypeDef *hspi);
```

Initializes the SD card and detects card type.

**Parameters**:

- `hspi`: Pointer to SPI handle

**Returns**:

- 0: Success
- Non-zero: Error code

**Initialization Sequence**:

1. Set SPI to low speed (400 kHz)
2. Send 80+ clock cycles with CS high (power-up sequence)
3. Send CMD0 to enter idle state
4. Send CMD8 to check SDV2 support
5. Send ACMD41 repeatedly until card ready
6. Determine card type (SDV1, SDV2, SDHC)
7. Set SPI to high speed (up to 18 MHz)

**Usage Example**:

```c
extern SPI_HandleTypeDef hspi1;

void SystemInit(void)
{
    MX_SPI1_Init();

    if (SD_Init(&hspi1) == 0)
    {
        printf("SD card initialized successfully\n");
        printf("Card type: %d\n", SD_GetType());
    }
    else
    {
        printf("SD card initialization failed\n");
    }
}
```

**Typical Initialization Time**: 50-200ms

## SPI Speed Control

### Set Low Speed (Initialization)

```c
void SD_SetSpeedLow(void);
```

Sets SPI clock to low speed (≤400 kHz) for initialization.

**Use Case**: Called during SD card initialization sequence.

### Set High Speed (Normal Operation)

```c
void SD_SetSpeedHigh(void);
```

Sets SPI clock to high speed (up to 25 MHz SD spec, typically 18 MHz STM32).

**Use Case**: Called after successful initialization for faster data transfer.

**Speed Impact**:
| SPI Speed | 512-byte Block Read Time |
|-----------|--------------------------|
| 400 kHz | ~10 ms |
| 4.5 MHz | ~1 ms |
| 18 MHz | ~0.25 ms |

## Block Read Operations

### Read Single Block

```c
uint8_t SD_ReadBlock(uint32_t block_addr, uint8_t *buffer);
```

Reads a 512-byte block from the SD card.

**Parameters**:

- `block_addr`: Block address (block number for SDHC, byte address for SDV1/V2)
- `buffer`: Pointer to 512-byte buffer to store read data

**Returns**:

- 0: Success
- Non-zero: Error code

**Usage Example**:

```c
uint8_t read_buffer[512];
uint32_t block_number = 0;  // First block

if (SD_ReadBlock(block_number, read_buffer) == 0)
{
    printf("Block read successfully\n");
    printf("First byte: 0x%02X\n", read_buffer[0]);
}
else
{
    printf("Block read failed\n");
}
```

**Read Sequence**:

1. Send CMD17 with block address
2. Wait for R1 response (0x00 = ready)
3. Wait for data start token (0xFE)
4. Read 512 bytes of data
5. Read 2 bytes CRC (ignored in SPI mode)
6. Deselect card

**Typical Read Time**: 0.5-2 ms @ 18 MHz SPI

### Addressing Considerations

```c
uint32_t GetBlockAddress(uint32_t sector)
{
    uint8_t cardType = SD_GetType();

    if (cardType == SD_TYPE_SDHC)
    {
        return sector;  // Block addressing
    }
    else
    {
        return sector * 512;  // Byte addressing
    }
}
```

## Block Write Operations

### Write Single Block

```c
uint8_t SD_WriteBlock(uint32_t block_addr, uint8_t *buffer);
```

Writes a 512-byte block to the SD card.

**Parameters**:

- `block_addr`: Block address
- `buffer`: Pointer to 512-byte buffer containing data to write

**Returns**:

- 0: Success
- Non-zero: Error code

**Usage Example**:

```c
uint8_t write_buffer[512];
memset(write_buffer, 0xAA, 512);  // Fill with test pattern

uint32_t block_number = 100;
if (SD_WriteBlock(block_number, write_buffer) == 0)
{
    printf("Block written successfully\n");
}
else
{
    printf("Block write failed\n");
}
```

**Write Sequence**:

1. Send CMD24 with block address
2. Wait for R1 response (0x00 = ready)
3. Send data start token (0xFE)
4. Send 512 bytes of data
5. Send 2 dummy CRC bytes
6. Wait for data response token
7. Wait for card to finish programming (busy signal)
8. Deselect card

**Typical Write Time**: 2-10 ms (includes programming time)

## SPI Helper Functions

### SPI Read/Write Byte

```c
uint8_t SD_SPI_ReadWrite(uint8_t data);
```

Sends one byte and receives one byte via SPI.

**Parameters**:

- `data`: Byte to send

**Returns**: Received byte

**Usage Example**:

```c
uint8_t response = SD_SPI_ReadWrite(0xFF);  // Send 0xFF, read response
```

### Chip Select Control

#### CS Low (Select Card)

```c
void SD_CS_Low(void);
```

Pulls CS pin low to select SD card.

#### CS High (Deselect Card)

```c
void SD_CS_High(void);
```

Pulls CS pin high to deselect SD card.

**Usage Pattern**:

```c
SD_CS_Low();
// Perform SD card operation
SD_CS_High();
```

### Send Clock Cycles

```c
void SD_SendClock(uint8_t count);
```

Sends dummy bytes (0xFF) to generate clock cycles.

**Parameters**:

- `count`: Number of bytes to send (8 clocks each)

**Usage Example**:

```c
SD_SendClock(10);  // Send 80 clock cycles for initialization
```

## Error Handling

### Common Error Codes

| Error Code | Meaning                      |
| ---------- | ---------------------------- |
| 0          | Success                      |
| 1          | Timeout waiting for response |
| 2          | Card not ready               |
| 3          | Read error                   |
| 4          | Write error                  |
| 5          | CRC error                    |

### Error Detection Example

```c
uint8_t result = SD_ReadBlock(0, buffer);
switch (result)
{
    case 0:
        printf("Success\n");
        break;
    case 1:
        printf("Timeout\n");
        break;
    case 3:
        printf("Read error\n");
        break;
    default:
        printf("Unknown error: %d\n", result);
}
```

### Retry Mechanism

```c
uint8_t SD_ReadBlockWithRetry(uint32_t addr, uint8_t *buffer, uint8_t max_retries)
{
    for (uint8_t i = 0; i < max_retries; i++)
    {
        if (SD_ReadBlock(addr, buffer) == 0)
        {
            return 0;  // Success
        }
        HAL_Delay(10);  // Wait before retry
    }
    return 1;  // Failed after retries
}
```

## Performance Optimization

### SPI Speed Tuning

**Maximum Safe Speed**:

- SD card spec: Up to 25 MHz
- STM32F103 @ 72 MHz: SPI can run at 36 MHz (APB2/2), but SD cards typically stable at 18 MHz

**Setting SPI Speed**:

```c
// In MX_SPI1_Init() or similar
hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;  // 72MHz / 4 = 18MHz
```

### Bulk Read Optimization

For reading multiple consecutive blocks:

```c
uint8_t SD_ReadMultipleBlocks(uint32_t start_addr, uint8_t *buffer, uint32_t block_count)
{
    // Use CMD18 (READ_MULTIPLE_BLOCK) instead of CMD17
    // Faster than repeated single block reads
    // Send CMD12 (STOP_TRANSMISSION) at end
}
```

## Integration with SD Card Manager

The low-level SD card driver is typically used by the SD Card Manager layer:

```c
// SD Card Manager uses low-level driver
void SD_CARD_MANAGER_Write(const char *jsonData)
{
    uint8_t buffer[512];
    PrepareBlock(buffer, jsonData);

    uint32_t block_addr = GetNextBlockAddress();
    SD_WriteBlock(block_addr, buffer);  // Low-level write
}
```

## Usage Examples

### Complete Initialization and Read

```c
void SD_Example(void)
{
    extern SPI_HandleTypeDef hspi1;
    uint8_t buffer[512];

    // Initialize SD card
    if (SD_Init(&hspi1) != 0)
    {
        printf("Initialization failed\n");
        return;
    }

    printf("Card type: ");
    switch (SD_GetType())
    {
        case SD_TYPE_SDHC:
            printf("SDHC\n");
            break;
        case SD_TYPE_SDV2:
            printf("SD V2\n");
            break;
        default:
            printf("Unknown\n");
    }

    // Read first block
    if (SD_ReadBlock(0, buffer) == 0)
    {
        printf("Block 0 read successfully\n");

        // Print first 16 bytes
        for (int i = 0; i < 16; i++)
        {
            printf("%02X ", buffer[i]);
        }
        printf("\n");
    }
}
```

### Write and Verify

```c
void SD_WriteVerify(uint32_t block_addr)
{
    uint8_t write_buffer[512];
    uint8_t read_buffer[512];

    // Prepare test data
    for (int i = 0; i < 512; i++)
    {
        write_buffer[i] = i & 0xFF;
    }

    // Write block
    if (SD_WriteBlock(block_addr, write_buffer) != 0)
    {
        printf("Write failed\n");
        return;
    }

    // Read back
    if (SD_ReadBlock(block_addr, read_buffer) != 0)
    {
        printf("Read failed\n");
        return;
    }

    // Verify
    if (memcmp(write_buffer, read_buffer, 512) == 0)
    {
        printf("Write/Read verified OK\n");
    }
    else
    {
        printf("Verification failed\n");
    }
}
```

## Memory Footprint

### Flash Usage

- Driver code: ~2-3 KB
- Command table: ~100 bytes

### RAM Usage

- Static variables: ~20 bytes (card type, state)
- Stack during operations: ~50 bytes
- Buffers: Provided by caller (512 bytes each)

## Best Practices

### 1. Check Initialization

```c
if (SD_Init(&hspi1) != 0)
{
    // Handle error before attempting operations
    return;
}
```

### 2. Validate Addresses

```c
#define MAX_BLOCKS 1000000  // Example limit

uint8_t SafeReadBlock(uint32_t addr, uint8_t *buffer)
{
    if (addr >= MAX_BLOCKS)
    {
        return 1;  // Invalid address
    }
    return SD_ReadBlock(addr, buffer);
}
```

### 3. Handle Card Removal

```c
// Periodically check card presence
if (SD_SendCommand(CMD13, 0, 0) != R1_READY)
{
    printf("Card removed or error\n");
    SD_Init(&hspi1);  // Re-initialize
}
```

### 4. Use Proper SPI Speed

```c
SD_SetSpeedLow();   // During initialization
SD_Init(&hspi1);
SD_SetSpeedHigh();  // After successful init
```

## Dependencies

### Required

- **STM32 HAL SPI**: SPI hardware abstraction
- **GPIO HAL**: Chip select pin control

### Used By

- **sd_card_manager.c**: High-level SD card buffering and management

## Summary

The SD Card Low-Level Driver Library provides:

- Complete SPI-based SD card protocol implementation
- Support for SD, SDHC, SDXC, and MMC cards
- Block read/write operations (512-byte blocks)
- Card type detection and initialization
- SPI speed control (400 kHz init, 18 MHz operation)
- Error handling and retry mechanisms
- Minimal RAM footprint (no block buffering at this layer)

This driver enables reliable SD card storage for the Datalogger system with industry-standard block-level interface.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
