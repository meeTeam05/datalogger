# Command Parser Library

Handles interpretation and validation of incoming command strings before execution.

## Overview

The Command Parser Library implements handler functions for all supported commands in the Datalogger system. Each parser function validates arguments, executes the corresponding action, and provides feedback via UART.

## Files

- **cmd_parser.c**: Command parser implementations
- **cmd_parser.h**: Command parser function declarations

## External Dependencies

### Required Modules

- **ds3231**: RTC time management
- **data_manager**: Centralized sensor data handling
- **print_cli**: UART output for debugging
- **wifi_manager**: MQTT state tracking
- **sht3x**: SHT3X sensor operations
- **sd_card_manager**: SD card buffering control
- **stm32f1xx_hal**: HAL library functions

### External Variables

```c
extern uint32_t next_fetch_ms;          // Periodic timing control
extern uint32_t periodic_interval_ms;   // Periodic interval configuration
extern mqtt_state_t mqtt_current_state; // MQTT connection state
```

## Command Parsers

### 1. CHECK_UART_STATUS

**Purpose**: Verify UART peripheral readiness

**Signature**:

```c
void CHECK_UART_STATUS(uint8_t argc, char **argv);
```

**Arguments**: None (argc must be 1)

**Behavior**:

- Checks HAL_UART_GetState(&huart1)
- Prints "UART is READY" or "UART is NOT READY"

**Usage Example**:

```
CHECK UART STATUS
```

**Output**:

```
UART is READY
```

---

### 2. SHT3X_Heater_Parser

**Purpose**: Control SHT3X sensor integrated heater

**Signature**:

```c
void SHT3X_Heater_Parser(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 3
- argv[2]: "ENABLE" or "DISABLE"

**Behavior**:

- Enables/disables sensor heater via SHT3X_Heater()
- Prints success or failure message

**Usage Examples**:

```
SHT3X HEATER ENABLE
SHT3X HEATER DISABLE
```

**Output**:

```
SHT3X HEATER ENABLE SUCCEEDED
```

---

### 3. SHT3X_ART_Parser

**Purpose**: Trigger Accelerated Response Time mode

**Signature**:

```c
void SHT3X_ART_Parser(uint8_t argc, char **argv);
```

**Arguments**: None

**Behavior**:

- Calls SHT3X_ART(&g_sht3x)
- Prints operation result

**Usage Example**:

```
SHT3X ART
```

**Output**:

```
SHT3X ART MODE SUCCEEDED
```

---

### 4. DS3231_Set_Time_Parser

**Purpose**: Manually set RTC time with full datetime

**Signature**:

```c
void DS3231_Set_Time_Parser(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 10
- argv[3]: Weekday (1-7)
- argv[4]: Day (1-31)
- argv[5]: Month (1-12)
- argv[6]: Year (00-99, represents 2000-2099)
- argv[7]: Hour (0-23)
- argv[8]: Minute (0-59)
- argv[9]: Second (0-59)

**Validation**:

- Validates all parameter ranges
- Converts to struct tm format
- Calls DS3231_Set_Time()

**Usage Example**:

```
DS3231 SET TIME 5 24 10 24 14 30 45
```

(Friday, October 24, 2024, 14:30:45)

**Output**:

```
DS3231 TIME SET: 2024-10-24 14:30:45 (WD:5)
```

---

### 5. SINGLE_PARSER

**Purpose**: Execute single sensor measurement

**Signature**:

```c
void SINGLE_PARSER(uint8_t argc, char **argv);
```

**Arguments**: None (argc must be 1)

**Behavior**:

1. Prints "[CMD] SINGLE"
2. Reads sensor via SHT3X_Single()
3. Updates DataManager with measurement
4. If sensor fails, reports 0.0 values
5. Prints temperature and humidity or failure

**Usage Example**:

```
SINGLE
```

**Output (Success)**:

```
[CMD] SINGLE
[CMD] T=25.43 H=60.21
```

**Output (Failure)**:

```
[CMD] SINGLE
[CMD] Sensor FAIL
```

**Data Flow**:

- Sensor → Parser → DataManager → JSON Output

---

### 6. PERIODIC_ON_PARSER

**Purpose**: Start periodic sensor measurements

**Signature**:

```c
void PERIODIC_ON_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 2
- argv[1]: "ON"

**Behavior**:

1. Prints "[CMD] PERIODIC ON"
2. Starts periodic mode via SHT3X_Periodic()
3. Updates DataManager with first measurement
4. Sets next_fetch_ms for timing control
5. Continues periodic reads in main loop

**Usage Example**:

```
PERIODIC ON
```

**Output**:

```
[CMD] PERIODIC ON
[CMD] T=25.48 H=60.15
```

**Timing**:

- Uses default interval (periodic_interval_ms)
- Default: 5000ms (5 seconds)
- Configurable via SET PERIODIC INTERVAL

---

### 7. PERIODIC_OFF_PARSER

**Purpose**: Stop periodic sensor measurements

**Signature**:

```c
void PERIODIC_OFF_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 2
- argv[1]: "OFF"

**Behavior**:

- Calls SHT3X_Stop_Periodic(&g_sht3x)
- Stops sensor periodic measurement mode
- No output (silent stop)

**Usage Example**:

```
PERIODIC OFF
```

**Output**: None

---

### 8. SET_TIME_PARSER

**Purpose**: Set RTC time from Unix timestamp

**Signature**:

```c
void SET_TIME_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 3
- argv[2]: Unix timestamp (seconds since epoch)

**Behavior**:

1. Parses Unix timestamp string
2. Converts to struct tm via localtime()
3. Calls DS3231_Set_Time()
4. Forces immediate display update

**Usage Example**:

```
SET TIME 1729780245
```

**Notes**:

- This is the primary time synchronization method from ESP32
- More convenient than DS3231_Set_Time_Parser (no manual datetime)
- Automatically forces display refresh

---

### 9. SET_PERIODIC_INTERVAL_PARSER

**Purpose**: Configure periodic measurement interval

**Signature**:

```c
void SET_PERIODIC_INTERVAL_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 4
- argv[3]: Interval in seconds

**Behavior**:

- Parses interval value
- Converts seconds to milliseconds
- Updates periodic_interval_ms
- Minimum: 1 second
- No maximum validation (recommend 5-3600s)

**Usage Example**:

```
SET PERIODIC INTERVAL 30
```

**Common Values**:

- 5: Fast monitoring
- 30: Production default
- 60: 1 minute interval
- 600: 10 minutes
- 1800: 30 minutes
- 3600: 1 hour

---

### 10. MQTT_CONNECTED_PARSER

**Purpose**: Handle MQTT connection notification from ESP32

**Signature**:

```c
void MQTT_CONNECTED_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 2
- Command: "MQTT CONNECTED"

**Behavior**:

- Updates mqtt_current_state to MQTT_STATE_CONNECTED
- Enables normal data transmission
- Stops SD card buffering mode
- No output

**Usage**:
Automatically sent by ESP32 when MQTT broker connects

---

### 11. MQTT_DISCONNECTED_PARSER

**Purpose**: Handle MQTT disconnection notification from ESP32

**Signature**:

```c
void MQTT_DISCONNECTED_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 2
- Command: "MQTT DISCONNECTED"

**Behavior**:

- Updates mqtt_current_state to MQTT_STATE_DISCONNECTED
- Triggers SD card buffering mode
- Data continues to be collected and stored locally
- No output

**Usage**:
Automatically sent by ESP32 when MQTT broker disconnects or WiFi lost

**Data Flow During Disconnect**:

```
Sensor → DataManager → SD Card Buffer (instead of UART)
```

---

### 12. SD_CLEAR_PARSER

**Purpose**: Clear all buffered data on SD card

**Signature**:

```c
void SD_CLEAR_PARSER(uint8_t argc, char **argv);
```

**Arguments**:

- argc: Must be 2
- Command: "SD CLEAR"

**Behavior**:

- Calls SDCardManager_ClearBuffer()
- Resets write/read indices
- Clears all 204,800 record slots
- Prints success or failure message

**Usage Example**:

```
SD CLEAR
```

**Output (Success)**:

```
SD buffer cleared successfully! All buffered data deleted.
```

**Output (Failure)**:

```
FAILED to clear SD buffer!
```

**Warning**: This operation is irreversible and will delete all offline-buffered data.

---

## Default Configuration

### SHT3X Modes

```c
#define SHT3X_MODE_REPEAT_DEFAULT SHT3X_HIGH
#define SHT3X_MODE_PERIODIC_DEFAULT SHT3X_PERIODIC_1MPS
```

- Repeatability: HIGH (highest accuracy)
- Periodic frequency: 1 measurement per second

## Error Handling

### Common Error Cases

1. **Invalid Argument Count**

   - Behavior: Silent return (no output)
   - Handled by argc validation

2. **Sensor Communication Failure**

   - Behavior: Reports 0.0 values
   - Prints error message
   - DataManager still processes (for consistency)

3. **Parameter Validation Failure**

   - Example: DS3231_Set_Time_Parser with invalid dates
   - Prints: "DS3231 INVALID PARAMETER VALUES"
   - Does not modify RTC

4. **SD Card Operation Failure**
   - Prints failure message
   - Returns false status

## Integration Points

### Data Flow

```
Command (UART) → cmd_parser → Action
                              ↓
                         Sensor/Module
                              ↓
                         DataManager
                              ↓
              JSON Output (UART) or SD Card Buffer
```

### State Management

- **mqtt_current_state**: Controls data routing (UART vs SD)
- **next_fetch_ms**: Controls periodic timing
- **periodic_interval_ms**: Configurable sampling rate

## Thread Safety

This library is **NOT thread-safe**. All functions should be called from the main loop or UART interrupt context only.

## Memory Usage

- Stack usage: Up to 256 bytes (struct tm, temporary buffers)
- No heap allocation
- External variables: 8 bytes (timing control)

## Performance

- Command parsing: < 1ms
- Sensor operations: 15-20ms (I2C communication)
- RTC operations: < 5ms (I2C communication)
- SD operations: Variable (10-50ms depending on operation)

## Best Practices

1. **Always validate argc** before accessing argv
2. **Print confirmation** for user-initiated commands
3. **Handle sensor failures gracefully** (report 0.0, continue operation)
4. **Update timing variables** when starting periodic mode
5. **Force display updates** when changing time
6. **Clear data flags** after processing

## Testing Commands

Quick test sequence:

```
CHECK UART STATUS
SINGLE
PERIODIC ON
SET PERIODIC INTERVAL 10
PERIODIC OFF
SD CLEAR
```

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
