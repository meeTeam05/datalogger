# Command Execute Library

Handles parsing and routing of commands received from various system interfaces.

## Overview

The Command Execute Library provides the command execution engine for the Datalogger system. It tokenizes incoming command strings, matches them against the registered command table, and dispatches execution to the appropriate handler functions.

## Files

- **cmd_execute.c**: Command execution engine implementation
- **cmd_execute.h**: Command execution API declaration

## Core Functionality

### Main Execution Function

```c
void COMMAND_EXECUTE(char *commandBuffer);
```

This is the primary entry point for command execution. It processes a null-terminated command string from the UART buffer.

## Execution Flow

### Step 1: Tokenization

The execution engine tokenizes the input string using whitespace delimiters:

```c
static uint8_t tokenize_string(char *str, char **argv, uint8_t max_tokens);
```

**Delimiters**: Space, Tab, Carriage Return, Newline
**Maximum Tokens**: Configurable (typically 16)
**Output**: Array of string pointers (argv) and token count (argc)

**Example**:

```
Input:  "SET PERIODIC INTERVAL 30"
Output: argc = 4
        argv[0] = "SET"
        argv[1] = "PERIODIC"
        argv[2] = "INTERVAL"
        argv[3] = "30"
```

**Important**: This function modifies the input string by inserting null terminators.

### Step 2: Command Lookup

The engine searches the command table for a matching command:

```c
static command_function_t *find_command(uint8_t argc, char **argv);
```

**Matching Algorithm**:

1. Count tokens in each table entry command string
2. Build test string from argv with same number of tokens
3. Compare test string with table entry using strcmp()
4. Return pointer to matched entry or NULL

**Example Match Process**:

```
Table Entry: "SET PERIODIC INTERVAL"
Input Tokens: ["SET", "PERIODIC", "INTERVAL", "30"]
Test String: "SET PERIODIC INTERVAL" (3 tokens)
Result: MATCH → SET_PERIODIC_INTERVAL_PARSER
```

### Step 3: Handler Invocation

If a matching command is found, the engine calls the handler function:

```c
if (command != NULL)
{
    command->func(argc, argv);
}
```

**Handler Signature**:

```c
void HandlerFunction(uint8_t argc, char **argv);
```

## Command Table Integration

### External Reference

The engine uses the command table defined in cmd_func.c:

```c
extern command_function_t cmdTable[];
```

### Table Structure

The table must be NULL-terminated:

```c
{.cmdString = NULL, .func = NULL}  // Terminator
```

## Tokenization Details

### Maximum Token Limit

Default: 16 tokens per command (configurable in implementation)

### String Modification

**Warning**: The tokenize_string function modifies the input buffer by:

- Replacing delimiters with null terminators
- This is necessary for efficient argv array creation
- Caller must not rely on original string content after tokenization

### Whitespace Handling

- Leading whitespace: Skipped automatically
- Multiple consecutive spaces: Treated as single delimiter
- Trailing whitespace: Ignored

## Command Matching Algorithm

### Prefix Matching

The engine uses **prefix matching** to support multi-word commands:

**Single-word command**: "SINGLE"

- Matches: "SINGLE"
- Does not match: "SINGLE READ"

**Multi-word command**: "PERIODIC ON"

- Matches: "PERIODIC ON"
- Does not match: "PERIODIC" alone
- Does not match: "PERIODIC OFF"

### Case Sensitivity

All command matching is **case-sensitive**:

- "SINGLE" matches
- "single" does NOT match
- "Single" does NOT match

### Exact Match Requirement

Commands must match exactly (no partial matching):

- "SET TIME" registered → "SET" alone will NOT match
- "PERIODIC ON" registered → "PERIODIC" alone will NOT match

## Error Handling

### No Matching Command

If no command matches:

- Function returns silently
- No error message printed
- Buffer is not cleared (handled by caller)

### Invalid Argument Count

Handler functions are responsible for validating argc:

```c
void HANDLER(uint8_t argc, char **argv)
{
    if (argc != expected_count)
    {
        // Print usage or return silently
        return;
    }
    // Process command
}
```

### Buffer Overflow Protection

Tokenizer enforces max_tokens limit:

```c
while (token != NULL && argc < max_tokens)
{
    argv[argc++] = token;
    token = strtok(NULL, " \t\r\n");
}
```

Excess tokens are ignored (not an error).

## Usage Example

### Basic Command Execution

```c
// Received from UART: "SINGLE\r\n"
char commandBuffer[128] = "SINGLE\r\n";

// Execute the command
COMMAND_EXECUTE(commandBuffer);

// Result:
// 1. Tokenized to: argc=1, argv[0]="SINGLE"
// 2. Matched to: SINGLE_PARSER
// 3. Handler called: SINGLE_PARSER(1, argv)
// 4. Sensor measurement executed
```

### Multi-Word Command

```c
// Received from UART: "SET PERIODIC INTERVAL 30\r\n"
char commandBuffer[128] = "SET PERIODIC INTERVAL 30\r\n";

COMMAND_EXECUTE(commandBuffer);

// Result:
// 1. Tokenized to: argc=4, argv=["SET","PERIODIC","INTERVAL","30"]
// 2. Matched to: "SET PERIODIC INTERVAL"
// 3. Handler called: SET_PERIODIC_INTERVAL_PARSER(4, argv)
// 4. Interval updated to 30 seconds
```

### Invalid Command

```c
// Unknown command
char commandBuffer[128] = "INVALID COMMAND\r\n";

COMMAND_EXECUTE(commandBuffer);

// Result:
// 1. Tokenized to: argc=2, argv=["INVALID","COMMAND"]
// 2. No match found in table
// 3. Function returns silently
// 4. No action taken
```

## Integration with UART Handler

### Typical Integration

```c
// In UART_Handle() function
if (Flag_UART == 1)  // Complete command received
{
    Flag_UART = 0;

    // Execute the command
    COMMAND_EXECUTE((char *)buff);

    // Clear buffer
    memset(buff, 0, sizeof(buff));
    index_uart = 0;
}
```

### Command Buffer Requirements

- Must be null-terminated
- Should contain single command
- Newline characters are handled by tokenizer
- Minimum buffer size: 16 bytes
- Recommended: 128 bytes or larger

## Performance Characteristics

### Time Complexity

- Tokenization: O(n) where n is command string length
- Command lookup: O(m) where m is number of table entries
- Total: O(n + m)

### Memory Usage

- Stack usage: Approximately 256 bytes
  - argv array: 16 pointers × 4 bytes = 64 bytes
  - tableCmdCopy buffer: 256 bytes
  - testCmd buffer: 256 bytes
- No heap allocation
- Input buffer modified in-place

### Typical Execution Time

- Short command (e.g., "SINGLE"): < 100 microseconds
- Long command (e.g., "SET PERIODIC INTERVAL 30"): < 200 microseconds
- Command table scan: < 50 microseconds per entry

## Thread Safety

This library is **NOT thread-safe**:

- Modifies input buffer during tokenization
- Uses static buffers internally
- Should only be called from main loop or single UART ISR

## Limitations

### Maximum Command Length

Limited by tokenization buffers (256 bytes):

- Test string buffer: 256 bytes
- Table copy buffer: 256 bytes
- Commands longer than 256 characters will be truncated

### Maximum Token Count

Hard-coded limit (typically 16):

- Commands with more than 16 words are truncated
- Excess arguments are discarded

### String Modification

Input buffer is modified during tokenization:

- Original command string is destroyed
- Caller must preserve original if needed

## Best Practices

### Command Design

1. Keep commands short and simple
2. Use consistent naming conventions (ALL CAPS)
3. Multi-word commands should be unambiguous
4. Avoid commands that are prefixes of other commands

### Error Handling

1. Validate argc in handler functions
2. Print usage information for incorrect arguments
3. Handle NULL argv pointers safely
4. Return early on validation failure

### Buffer Management

1. Allocate sufficient buffer space (minimum 128 bytes)
2. Ensure null-termination before calling COMMAND_EXECUTE
3. Clear buffer after execution to prevent command replay
4. Check buffer overflow before UART reception

## Debugging

### Enable Detailed Logging

Add debug prints in command_execute.c:

```c
void COMMAND_EXECUTE(char *commandBuffer)
{
    printf("CMD: %s\n", commandBuffer);  // Debug: raw command

    // After tokenization
    printf("argc=%d\n", argc);
    for (int i = 0; i < argc; i++)
    {
        printf("argv[%d]=%s\n", i, argv[i]);
    }

    // After lookup
    if (command != NULL)
    {
        printf("MATCHED: %s\n", command->cmdString);
    }
    else
    {
        printf("NO MATCH\n");
    }
}
```

### Common Issues

1. **Command Not Recognized**

   - Check case sensitivity
   - Verify command is in cmdTable
   - Check for extra spaces or special characters

2. **Handler Not Called**

   - Verify handler function is not NULL in table
   - Check argc validation in handler
   - Ensure table is NULL-terminated

3. **Incorrect Arguments**
   - Print argc and argv in handler for debugging
   - Check tokenization with test strings
   - Verify delimiter handling

## Extension Example

To add a new multi-word command:

1. Register in cmd_func.c:

```c
{.cmdString = "GET SENSOR STATUS", .func = GET_SENSOR_STATUS_PARSER}
```

2. Implement handler in cmd_parser.c:

```c
void GET_SENSOR_STATUS_PARSER(uint8_t argc, char **argv)
{
    if (argc != 3)  // "GET" "SENSOR" "STATUS"
    {
        PRINT_CLI("Usage: GET SENSOR STATUS\r\n");
        return;
    }

    // Implementation
    PRINT_CLI("Sensor: OK\r\n");
}
```

3. Declare in cmd_parser.h:

```c
void GET_SENSOR_STATUS_PARSER(uint8_t argc, char **argv);
```

## Summary

The Command Execute Library provides a robust, extensible command execution engine that:

- Tokenizes input strings efficiently
- Matches commands against registered handlers
- Dispatches execution with argument passing
- Handles errors gracefully
- Integrates seamlessly with UART reception

This design pattern enables easy addition of new commands without modifying the execution engine itself.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
