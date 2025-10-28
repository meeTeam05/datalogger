# SD Card Manager Library

Handles buffered data storage and retrieval for SD card–based logging operations.

## Overview

The SD Card Manager Library provides high-level buffering and management for sensor data storage on SD cards. It implements a circular buffer capable of storing up to 204,800 sensor records, enabling reliable offline data logging and synchronization with the ESP32 module.

## Files

- **sd_card_manager.c**: SD Card Manager implementation
- **sd_card_manager.h**: SD Card Manager API and data structures

## Core Concepts

### Buffer Architecture

The SD Card Manager implements a **circular buffer** (ring buffer) stored directly on the SD card:

```
[Metadata Block] → [Data Block 0] → [Data Block 1] → ... → [Data Block 204799] → (wrap to 0)
     Block 1           Block 2          Block 3                Block 204801
```

**Capacity**: 204,800 sensor records (200K records)
**Record Size**: 512 bytes (1 SD card block per record)
**Total Storage**: ~100 MB

### Metadata Management

**Metadata Block** (Block 1):

```c
typedef struct {
    uint32_t write_index;   // Next write position (0-204799)
    uint32_t read_index;    // Next read position (0-204799)
    uint32_t count;         // Number of valid records in buffer
    uint32_t sequence_num;  // Global sequence counter
} sd_buffer_metadata_t;
```

**Purpose**: Tracks buffer state across power cycles.

### Data Record Structure

```c
typedef struct {
    uint32_t timestamp;     // Unix timestamp (4 bytes)
    float temperature;      // Temperature in Celsius (4 bytes)
    float humidity;         // Humidity in percentage (4 bytes)
    char mode[16];          // "SINGLE" or "PERIODIC" (16 bytes)
    uint32_t sequence_num;  // Sequence number (4 bytes)
    uint8_t padding[480];   // Padding to 512 bytes total
} sd_data_record_t;
```

**Total Size**: 512 bytes (exactly 1 SD block)

**Field Details**:

- `timestamp`: Seconds since January 1, 1970 (Unix epoch)
- `temperature`: Range -40.0 to +125.0°C (SHT3X sensor range)
- `humidity`: Range 0.0 to 100.0%
- `mode`: "SINGLE" or "PERIODIC" (null-terminated string)
- `sequence_num`: Monotonic counter for record ordering
- `padding`: Reserved for future use

## Configuration

### Buffer Size

```c
#define SD_BUFFER_SIZE 204800  // Maximum number of records
```

**Capacity**: 204,800 records
**Storage**: ~100 MB (204,800 × 512 bytes)

### SD Block Allocation

```c
#define SD_DATA_BLOCK 1         // Metadata block address
#define SD_DATA_START_BLOCK 2   // First data block address
```

**Block Map**:

- Block 0: Reserved (MBR/boot sector, not used)
- Block 1: Buffer metadata
- Blocks 2-204801: Data records (204,800 blocks)

## API Functions

### Initialize SD Card Manager

```c
bool SDCardManager_Init(void);
```

Initializes the SD card and loads buffer metadata.

**Returns**:

- `true`: SD card initialized successfully
- `false`: Initialization failed

**Operations**:

1. Initialize low-level SD card driver
2. Read metadata from block 1
3. Validate metadata (check for corruption)
4. If invalid metadata: Initialize to empty state

**Usage Example**:

```c
if (SDCardManager_Init())
{
    printf("SD Card Manager initialized\n");
    printf("Buffered records: %lu\n", SDCardManager_GetBufferedCount());
}
else
{
    printf("SD Card initialization failed\n");
}
```

**Typical Execution Time**: 100-300ms

### Write Sensor Data

```c
bool SDCardManager_WriteData(uint32_t timestamp, float temperature,
                              float humidity, const char *mode_str);
```

Writes a sensor data record to the SD card buffer.

**Parameters**:

- `timestamp`: Unix timestamp (0 = use current RTC time)
- `temperature`: Temperature in °C
- `humidity`: Humidity in %
- `mode_str`: "SINGLE" or "PERIODIC"

**Returns**:

- `true`: Data written successfully
- `false`: Buffer full or SD error

**Behavior**:

1. Check if buffer is full (`count >= SD_BUFFER_SIZE`)
2. Prepare record structure
3. Write record to SD block: `SD_DATA_START_BLOCK + write_index`
4. Increment `write_index` (wrap at `SD_BUFFER_SIZE`)
5. Increment `count` and `sequence_num`
6. Update metadata block

**Usage Example**:

```c
// Write sensor reading with current time
SDCardManager_WriteData(0, 25.5, 65.2, "PERIODIC");

// Write with specific timestamp
uint32_t timestamp = 1710000000;
SDCardManager_WriteData(timestamp, 23.8, 58.4, "SINGLE");
```

**Write Time**: 5-15ms (includes SD card write + metadata update)

### Read Buffered Data

```c
bool SDCardManager_ReadData(sd_data_record_t *record);
```

Reads the next buffered record (does not remove it).

**Parameters**:

- `record`: Pointer to structure where data will be stored

**Returns**:

- `true`: Record read successfully
- `false`: No records available

**Behavior**:

1. Check if buffer is empty (`count == 0`)
2. Read record from SD block: `SD_DATA_START_BLOCK + read_index`
3. Does NOT increment `read_index` (use `SDCardManager_RemoveRecord()` to mark as sent)

**Usage Example**:

```c
sd_data_record_t record;
if (SDCardManager_ReadData(&record))
{
    printf("Record %lu: Temp=%.2f C, Hum=%.2f %%\n",
           record.sequence_num,
           record.temperature,
           record.humidity);
}
```

**Read Time**: 2-5ms

### Remove Sent Record

```c
bool SDCardManager_RemoveRecord(void);
```

Marks the last read record as sent (removes from buffer).

**Returns**:

- `true`: Record removed successfully
- `false`: No records to remove

**Behavior**:

1. Check if buffer is empty
2. Increment `read_index` (wrap at `SD_BUFFER_SIZE`)
3. Decrement `count`
4. Update metadata block

**Usage Example**:

```c
// Read and process workflow
sd_data_record_t record;
if (SDCardManager_ReadData(&record))
{
    // Send to ESP32/server
    if (SendToServer(&record))
    {
        // Successfully sent, remove from buffer
        SDCardManager_RemoveRecord();
    }
    // If send fails, record remains in buffer for retry
}
```

**Remove Time**: 5-10ms (metadata update only)

### Get Buffered Record Count

```c
uint32_t SDCardManager_GetBufferedCount(void);
```

Returns the number of records currently buffered.

**Returns**: Number of records (0 to `SD_BUFFER_SIZE`)

**Usage Example**:

```c
uint32_t count = SDCardManager_GetBufferedCount();
printf("Buffered: %lu / %d records\n", count, SD_BUFFER_SIZE);

if (count > SD_BUFFER_SIZE * 0.9)
{
    printf("WARNING: Buffer 90%% full\n");
}
```

### Clear Buffer

```c
bool SDCardManager_ClearBuffer(void);
```

Clears all buffered data (resets buffer to empty).

**Returns**:

- `true`: Buffer cleared successfully
- `false`: SD card error

**Warning**: This operation is **irreversible**. All buffered data is lost.

**Usage Example**:

```c
// After successful synchronization
if (all_data_sent_successfully)
{
    SDCardManager_ClearBuffer();
    printf("Buffer cleared\n");
}
```

### Check SD Card Status

```c
bool SDCardManager_IsReady(void);
```

Checks if SD card is initialized and ready for operations.

**Returns**:

- `true`: SD card ready
- `false`: SD card not ready or error

**Usage Example**:

```c
if (!SDCardManager_IsReady())
{
    printf("SD card not ready, reinitializing...\n");
    SDCardManager_Init();
}
```

### Get Last Error Code

```c
uint8_t SDCardManager_GetLastError(void);
```

Returns the last error code from SD operations.

**Returns**: Error code (0 = no error)

**Usage Example**:

```c
if (!SDCardManager_WriteData(timestamp, temp, hum, mode))
{
    uint8_t error = SDCardManager_GetLastError();
    printf("Write failed: error code %d\n", error);
}
```

## Buffer Operation Modes

### Circular Buffer Behavior

When buffer is full:

```
Option 1: Reject new writes (current implementation)
  - SDCardManager_WriteData() returns false
  - Data is lost if not handled

Option 2: Overwrite oldest data (alternative)
  - Automatically increment read_index when full
  - Always accepts new data
```

**Current Behavior**: Write fails when `count >= SD_BUFFER_SIZE`.

### Full Buffer Handling

```c
if (!SDCardManager_WriteData(timestamp, temp, hum, mode))
{
    // Buffer full or SD error

    // Option 1: Log error and discard
    printf("ERROR: SD buffer full\n");

    // Option 2: Force send to clear space
    ForceBufferSynchronization();

    // Option 3: Overwrite oldest (requires code modification)
}
```

## Data Synchronization Workflow

### Offline Logging (WiFi Disconnected)

```c
// Periodic mode: Log to SD card
if (DATA_MANAGER_Get_Mode() == PERIODIC_MODE)
{
    float temp = DATA_MANAGER_Get_Temperature();
    float hum = DATA_MANAGER_Get_Humidity();
    uint32_t timestamp = DS3231_ReadUnixTime();

    SDCardManager_WriteData(timestamp, temp, hum, "PERIODIC");
}
```

### Online Synchronization (WiFi Connected)

```c
void SynchronizeBufferedData(void)
{
    sd_data_record_t record;
    uint32_t count = SDCardManager_GetBufferedCount();

    printf("Syncing %lu records...\n", count);

    while (SDCardManager_ReadData(&record))
    {
        // Format as JSON
        char json[256];
        snprintf(json, sizeof(json),
                 "{\"timestamp\":%lu,\"temp\":%.2f,\"hum\":%.2f,\"mode\":\"%s\",\"seq\":%lu}",
                 record.timestamp,
                 record.temperature,
                 record.humidity,
                 record.mode,
                 record.sequence_num);

        // Send to ESP32 via UART
        if (SendToESP32(json))
        {
            // Successfully sent, remove from buffer
            SDCardManager_RemoveRecord();
        }
        else
        {
            // Send failed, keep in buffer and retry later
            break;
        }
    }

    printf("Sync complete. Remaining: %lu\n", SDCardManager_GetBufferedCount());
}
```

## Storage Capacity Analysis

### Maximum Storage Duration

**Periodic Mode @ 5-second interval**:

```
Records per hour = 3600 / 5 = 720
Buffer capacity = 204,800 records
Duration = 204,800 / 720 = 284 hours ≈ 11.8 days
```

**Periodic Mode @ 30-second interval**:

```
Records per hour = 3600 / 30 = 120
Duration = 204,800 / 120 = 1,707 hours ≈ 71 days
```

**Periodic Mode @ 60-second interval**:

```
Records per hour = 3600 / 60 = 60
Duration = 204,800 / 60 = 3,413 hours ≈ 142 days (4.7 months)
```

### Storage Requirements

**Total SD Card Usage**:

```
Metadata: 1 block (512 bytes)
Data: 204,800 blocks (104,857,600 bytes ≈ 100 MB)
Total: 204,801 blocks ≈ 100 MB
```

**Minimum SD Card Size**: 128 MB (100 MB buffer + overhead)

## Performance Characteristics

### Operation Times

| Operation                  | Time       |
| -------------------------- | ---------- |
| SDCardManager_Init         | 100-300 ms |
| SDCardManager_WriteData    | 5-15 ms    |
| SDCardManager_ReadData     | 2-5 ms     |
| SDCardManager_RemoveRecord | 5-10 ms    |
| SDCardManager_ClearBuffer  | 10-20 ms   |

### Throughput

**Write Throughput**:

```
Max write rate = 1000 ms / 10 ms = 100 records/second
Practical rate @ 5s interval = 0.2 records/second
Utilization = 0.2%
```

**Read Throughput** (during sync):

```
Read + Send + Remove = 2ms + 50ms (UART) + 5ms = 57ms/record
Sync rate = 1000 / 57 ≈ 17 records/second
Time to sync 1000 records = 1000 / 17 ≈ 59 seconds
```

## Error Handling

### Write Errors

```c
bool result = SDCardManager_WriteData(timestamp, temp, hum, mode);
if (!result)
{
    uint8_t error = SDCardManager_GetLastError();

    switch (error)
    {
        case 0:
            // Buffer full
            printf("Buffer full: %lu records\n", SDCardManager_GetBufferedCount());
            TriggerSynchronization();
            break;

        case 3:
            // SD card read error
            printf("SD card read error\n");
            SDCardManager_Init();  // Reinitialize
            break;

        case 4:
            // SD card write error
            printf("SD card write error\n");
            SDCardManager_Init();
            break;

        default:
            printf("Unknown error: %d\n", error);
    }
}
```

### Metadata Corruption Detection

```c
// On initialization, validate metadata
if (metadata.write_index >= SD_BUFFER_SIZE ||
    metadata.read_index >= SD_BUFFER_SIZE ||
    metadata.count > SD_BUFFER_SIZE)
{
    printf("Corrupted metadata detected, resetting buffer\n");
    ResetMetadata();
}
```

## Integration Examples

### Periodic Mode Data Logging

```c
void TIM2_IRQHandler(void)
{
    // Timer interrupt for periodic measurements
    if (/* timer flag */)
    {
        Flag_Periodic = 1;  // Set flag for main loop
    }
}

void main_loop(void)
{
    if (Flag_Periodic)
    {
        Flag_Periodic = 0;

        // Read sensor
        float temp, hum;
        SHT3X_Read_TempHumi(&temp, &hum);

        // Update data manager
        DATA_MANAGER_Update_Sensor(temp, hum);

        // Log to SD card
        uint32_t timestamp = DS3231_ReadUnixTime();
        if (!SDCardManager_WriteData(timestamp, temp, hum, "PERIODIC"))
        {
            printf("WARNING: SD write failed\n");
        }

        // Send via UART (if WiFi connected)
        if (WiFi_IsConnected())
        {
            sensor_json_output_send("PERIODIC", temp, hum);
        }
    }
}
```

### WiFi Reconnection Sync

```c
void WiFi_OnConnected(void)
{
    printf("WiFi connected, starting sync...\n");

    uint32_t buffered = SDCardManager_GetBufferedCount();
    if (buffered > 0)
    {
        printf("Syncing %lu buffered records\n", buffered);
        SynchronizeBufferedData();
    }
}
```

## Best Practices

### 1. Check Write Success

```c
if (!SDCardManager_WriteData(timestamp, temp, hum, mode))
{
    // Handle error immediately
    ErrorCount++;
}
```

### 2. Monitor Buffer Usage

```c
uint32_t count = SDCardManager_GetBufferedCount();
if (count > SD_BUFFER_SIZE * 0.8)
{
    // Trigger urgent synchronization
    printf("CRITICAL: Buffer 80%% full\n");
}
```

### 3. Graceful Degradation

```c
if (!SDCardManager_IsReady())
{
    // SD card failed, continue without buffering
    printf("WARNING: Operating without SD card\n");

    // Still send live data via UART
    sensor_json_output_send(mode, temp, hum);
}
```

### 4. Periodic Metadata Validation

```c
// Every 100 writes, validate metadata
static uint32_t write_count = 0;
if (++write_count % 100 == 0)
{
    ValidateMetadata();
}
```

## Memory Footprint

### RAM Usage

- Metadata structure: 16 bytes
- Static variables: ~50 bytes
- Record buffer (temporary): 512 bytes
- **Total**: ~580 bytes

### Flash Usage

- Function code: ~2-3 KB

## Dependencies

### Required

- **sd_card.h**: Low-level SD card block read/write
- **ds3231.h**: RTC for timestamps
- **data_manager.h**: Sensor data access

### Used By

- **main.c**: Periodic data logging
- **cmd_parser.c**: Manual sync commands

## Summary

The SD Card Manager Library provides:

- High-level circular buffer for 204,800 sensor records
- Reliable offline data storage (~100 MB capacity)
- Automatic metadata management (survives power cycles)
- Flexible synchronization workflow
- Buffer status monitoring
- Error detection and recovery
- 11-142 days of buffering (depends on interval)

This library enables robust offline data logging and seamless synchronization when WiFi connectivity is restored, ensuring no sensor data is lost even during extended network outages.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
