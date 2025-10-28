# Command Function Library

Defines and manages the set of available command handlers used by the system.

## Overview

The Command Function Library provides a centralized command dispatch table that maps command strings to their corresponding handler functions. This library serves as the bridge between the command parser and command execution layers.

## Files

- **cmd_func.c**: Command function table implementation
- **cmd_func.h**: Command function type definitions and structures

## Core Components

### Command Function Structure

```c
typedef struct
{
    const char *cmdString;  // Command string to match
    CmdHandlerFunc func;    // Pointer to handler function
} command_function_t;
```

### Function Pointer Type

```c
typedef void (*CmdHandlerFunc)(uint8_t argc, char **argv);
```

## Command Table

The library maintains a global command table (cmdTable) that contains all supported commands:

### Supported Commands

1. **CHECK UART STATUS**

   - Handler: CHECK_UART_STATUS
   - Purpose: Debug UART communication state
   - Usage: Verify UART is ready for transmission

2. **SHT3X HEATER ENABLE**

   - Handler: SHT3X_Heater_Parser
   - Purpose: Enable SHT3X sensor heater for moisture removal
   - Usage: Debugging and sensor maintenance

3. **SHT3X HEATER DISABLE**

   - Handler: SHT3X_Heater_Parser
   - Purpose: Disable SHT3X sensor heater
   - Usage: Normal operation mode

4. **SHT3X ART**

   - Handler: SHT3X_ART_Parser
   - Purpose: Trigger Accelerated Response Time mode
   - Usage: Quick sensor response test

5. **DS3231 SET TIME**

   - Handler: DS3231_Set_Time_Parser
   - Purpose: Set RTC time manually with full datetime
   - Format: DS3231 SET TIME <WEEKDAY> <DAY> <MONTH> <YEAR> <HOUR> <MIN> <SEC>
   - Usage: Debug and manual time configuration

6. **SINGLE**

   - Handler: SINGLE_PARSER
   - Purpose: Perform single sensor measurement
   - Usage: On-demand data acquisition

7. **PERIODIC ON**

   - Handler: PERIODIC_ON_PARSER
   - Purpose: Start periodic sensor measurements
   - Usage: Continuous monitoring mode

8. **PERIODIC OFF**

   - Handler: PERIODIC_OFF_PARSER
   - Purpose: Stop periodic sensor measurements
   - Usage: End continuous monitoring

9. **SET TIME**

   - Handler: SET_TIME_PARSER
   - Purpose: Set RTC time from Unix timestamp
   - Format: SET TIME <unix_timestamp>
   - Usage: Time synchronization from ESP32

10. **SET PERIODIC INTERVAL**

    - Handler: SET_PERIODIC_INTERVAL_PARSER
    - Purpose: Configure periodic measurement interval
    - Format: SET PERIODIC INTERVAL <SECONDS>
    - Usage: Adjust sampling rate (5s to 3600s)

11. **MQTT CONNECTED**

    - Handler: MQTT_CONNECTED_PARSER
    - Purpose: Notification from ESP32 that MQTT broker is connected
    - Usage: State synchronization

12. **MQTT DISCONNECTED**

    - Handler: MQTT_DISCONNECTED_PARSER
    - Purpose: Notification from ESP32 that MQTT broker disconnected
    - Usage: State synchronization and offline buffering trigger

13. **SD CLEAR**
    - Handler: SD_CLEAR_PARSER
    - Purpose: Clear all buffered data on SD card
    - Format: SD CLEAR
    - Usage: Reset offline buffer manually

## Table Structure

The command table is an array of `command_function_t` structures, terminated by a NULL entry:

```c
command_function_t cmdTable[] = {
    {.cmdString = "CHECK UART STATUS", .func = CHECK_UART_STATUS},
    {.cmdString = "SHT3X HEATER ENABLE", .func = SHT3X_Heater_Parser},
    // ... more entries ...
    {.cmdString = NULL, .func = NULL},  // Terminator
};
```

## Usage Pattern

### Command Registration

Commands are statically registered in the cmdTable array. No runtime registration is supported.

### Command Lookup

The command_execute module uses the table to:

1. Parse incoming command string
2. Match against cmdString entries
3. Call corresponding handler function with argc/argv

### Handler Function Signature

All command handlers must follow the signature:

```c
void HandlerFunction(uint8_t argc, char **argv);
```

Where:

- `argc`: Argument count (including command itself)
- `argv`: Argument vector (argv[0] is the command)

## Integration

### Dependencies

This library depends on:

- **cmd_parser.h**: For parser function declarations
- None (header only defines types)

### Used By

This library is used by:

- **command_execute**: For command dispatch and execution
- All command parser implementations in cmd_parser.c

## Design Pattern

This library implements the **Command Pattern**:

- Decouples command invocation from implementation
- Allows easy addition of new commands
- Provides centralized command management
- Supports runtime command lookup

## Memory Footprint

- Command table size: 14 entries x 8 bytes = 112 bytes (flash)
- Function pointers: No runtime allocation
- Total: Approximately 112 bytes in flash memory

## Extension Guide

To add a new command:

1. Declare handler function in cmd_parser.h:

```c
void NEW_COMMAND_PARSER(uint8_t argc, char **argv);
```

2. Implement handler in cmd_parser.c:

```c
void NEW_COMMAND_PARSER(uint8_t argc, char **argv)
{
    // Implementation
}
```

3. Add entry to cmdTable in cmd_func.c:

```c
{.cmdString = "NEW COMMAND", .func = NEW_COMMAND_PARSER},
```

## Thread Safety

This library is **NOT thread-safe**. The command table is read-only after initialization, making it safe for concurrent reads but not for modifications.

## Notes

- All command strings are case-sensitive
- Multi-word commands use spaces as delimiters
- Command table must be NULL-terminated
- Handler functions are responsible for argument validation
- Commands are matched using exact string comparison

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
