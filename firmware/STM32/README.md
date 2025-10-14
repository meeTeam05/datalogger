# STM32 SHT3X Data Logger Firmware

A robust command-line interface firmware for controlling SHT3X temperature/humidity sensors with integrated RTC timestamp logging on STM32F1 microcontrollers. Features centralized data management, JSON output, and dual measurement modes.

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────────┐
│                        STM32 Data Logger                         │
│                                                                  │
│  UART RX ──→ Ring Buffer ──→ Command Parser ──→ SHT3X Driver     │
│     ↓              ↓               ↓                  ↓          │
│  Interrupt    256B Circular   String Match      I2C @ 100kHz     │
│  Driven       Buffer          Dispatch          + CRC Check      │
│                                    ↓                             │
│                           Data Manager (Centralized)             │
│                                    ↓                             │
│                      ┌─────────────┴─────────────┐               │
│                      ↓                           ↓               │
│              JSON Formatter              DS3231 RTC              │
│              (Single/Periodic)           Unix Timestamp          │
│                      ↓                           ↓               │
│                 UART TX ←────────────────────────┘               │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘

Main Loop: UART_Handle() → DataManager_Print() → Periodic_Fetch()
```

## Project Structure

```
STM32/
├── Core/                                    # STM32 HAL & System
│   ├── Inc/
│   │   └── main.h                          # Main application header
│   └── Src/
│       ├── main.c                          # Application entry & main loop
│       ├── stm32f1xx_it.c                  # Interrupt handlers
│       └── stm32f1xx_hal_msp.c             # HAL MSP initialization
│
├── Datalogger_Lib/                         # Core Firmware Library
│   ├── inc/                                # Public Headers
│   │   ├── uart.h                          # UART + ring buffer interface
│   │   ├── ring_buffer.h                   # Circular buffer API
│   │   ├── print_cli.h                     # UART formatted output
│   │   ├── cmd_func.h                      # Command table definitions
│   │   ├── cmd_parser.h                    # Command parser functions
│   │   ├── command_execute.h               # Command execution engine
│   │   ├── sht3x.h                         # SHT3X sensor driver
│   │   ├── ds3231.h                        # DS3231 RTC driver
│   │   ├── data_manager.h                  # Centralized data management
│   │   └── sensor_json_output.h            # JSON output formatter
│   │
│   └── src/                                # Implementation Files
│       ├── uart.c                          # UART ISR + line assembly
│       ├── ring_buffer.c                   # Lock-free circular buffer
│       ├── print_cli.c                     # Printf-style UART TX
│       ├── cmd_func.c                      # Command lookup table
│       ├── cmd_parser.c                    # Command handlers
│       ├── command_execute.c               # Tokenizer + dispatcher
│       ├── sht3x.c                         # I2C sensor communication
│       ├── ds3231.c                        # I2C RTC communication
│       ├── data_manager.c                  # Centralized state manager
│       └── sensor_json_output.c            # JSON string builder
│
├── Drivers/                                # STM32 HAL Drivers
│   ├── STM32F1xx_HAL_Driver/               # HAL peripheral drivers
│   └── CMSIS/                              # ARM CMSIS headers
│
├── FLOW_DIAGRAM.md                         # Control flow documentation
├── SEQUENCE_DIAGRAM.md                     # Interaction sequences
├── UML_CLASS_DIAGRAM.md                    # Class structure diagrams
└── README.md                               # This file
```

## Key Features

### Core Capabilities
- **Interrupt-Driven UART**: 256-byte lock-free ring buffer with zero data loss
- **Exact Command Matching**: Case-sensitive string-based command dispatch system
- **Dual Measurement Modes**: 
  - Single-shot: On-demand measurements with immediate response
  - Periodic: Continuous sampling (0.5 - 10 Hz) with automatic output
- **Centralized Data Management**: Single point of control for all sensor data output
- **JSON Output Format**: Structured data with Unix timestamps for easy parsing
- **State Preservation**: Seamless mode switching without data loss
- **Robust Error Recovery**: I2C timeout handling, CRC validation, buffer overflow protection

### Technical Highlights
- **Real-Time Clock Integration**: DS3231 RTC provides Unix timestamps for all measurements
- **Low Latency**: <100ms command-to-response time
- **Memory Efficient**: <1KB RAM overhead, <3KB flash for core logic
- **Thread-Safe Design**: Volatile pointers and atomic operations for ISR safety
- **Modular Architecture**: Clean separation of concerns for easy maintenance


## Hardware Setup

### Required Components
- **Microcontroller**: STM32F103C8T6 or compatible STM32F1xx
- **Sensor**: SHT30/SHT31/SHT35 temperature/humidity sensor
- **RTC**: DS3231 real-time clock module
- **Programmer**: ST-Link V2 or compatible
- **Resistors**: 4.7kΩ pull-ups for I2C lines (if not included in modules)

### Pin Configuration

| Function | STM32 Pin | Device | Notes |
|----------|-----------|--------|-------|
| **I2C1 SCL** | PB6 | SHT3X + DS3231 | Shared I2C bus |
| **I2C1 SDA** | PB7 | SHT3X + DS3231 | Shared I2C bus |
| **UART1 TX** | PA9 | USB-Serial | Output to PC |
| **UART1 RX** | PA10 | USB-Serial | Input from PC |
| **VCC** | 3.3V | SHT3X + DS3231 | Common power |
| **GND** | GND | All | Common ground |

### I2C Address Configuration

| Device | ADDR Pin | I2C Address | Default |
|--------|----------|-------------|---------|
| SHT3X | GND | 0x44 (7-bit) | ✓ Used in code |
| SHT3X | VDD | 0x45 (7-bit) | Alternative |
| DS3231 | - | 0x68 (7-bit) | Fixed |

### Wiring Diagram

```
                    STM32F103C8T6
                   ┌──────────────┐
     USB-Serial    │              │
    ┌──────────────┤ PA9  (TX)    │
    │    ┌─────────┤ PA10 (RX)    │
    │    │         │              │
    │    │    ┌────┤ PB6  (SCL)   ├──┬── 4.7kΩ ──┬── VCC
    │    │    │    │              │  │           │
    │    │    │┌───┤ PB7  (SDA)   ├──┼── 4.7kΩ ──┘
    │    │    ││   │              │  │
    │    │    ││   │ 3.3V         ├──┼───────────────── VCC
    │    │    ││   │ GND          ├──┼───────────────── GND
    │    │    ││   └──────────────┘  │
    │    │    ││                      │
    │    │    ││   SHT3X Sensor       │   DS3231 RTC
    │    │    ││   ┌──────────┐       │   ┌──────────┐
    │    │    │└───┤ SDA      │       └───┤ SDA      │
    │    │    └────┤ SCL      │           ├ SCL      │
    │    │         ├ VCC      │           ├ VCC      │
    │    │         ├ GND      │           ├ GND      │
    │    │         ├ ADDR─GND │           └──────────┘
    │    │         └──────────┘
    │    │
   TX   RX
```

## Software Setup

### Prerequisites
- **STM32CubeIDE** or **Keil MDK** or **GCC ARM toolchain**
- **STM32CubeMX** (for hardware configuration)
- **ST-Link Utility** or **OpenOCD** (for flashing)
- **Serial Terminal**: PuTTY, Tera Term, minicom, or screen

### Build Instructions

#### Using STM32CubeIDE (Recommended)

1. **Import Project**
   ```bash
   File → Open Projects from File System
   Select: STM32/ directory
   ```

2. **Build Project**
   ```bash
   Project → Build All (Ctrl+B)
   ```

3. **Flash Firmware**
   ```bash
   Run → Debug (F11)
   ```

#### Using Command Line (GCC ARM)

1. **Install Toolchain**
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-arm-none-eabi openocd
   
   # macOS
   brew install gcc-arm-embedded openocd
   
   # Windows
   # Download from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm
   ```

2. **Build**
   ```bash
   cd STM32/
   make clean
   make all
   ```

3. **Flash**
   ```bash
   openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
     -c "program build/STM32_DATALOGGER.elf verify reset exit"
   ```

### Terminal Connection

#### Windows (PuTTY)
```
Connection Type: Serial
Serial Line: COM3 (check Device Manager)
Speed: 115200
Data bits: 8
Stop bits: 1
Parity: None
Flow control: None
```

#### Linux/macOS (minicom)
```bash
# Find device
ls /dev/ttyUSB* /dev/ttyACM*

# Connect
minicom -D /dev/ttyUSB0 -b 115200

# Or using screen
screen /dev/ttyUSB0 115200
```

#### Python (pySerial)
```python
import serial

ser = serial.Serial(
    port='/dev/ttyUSB0',  # Windows: 'COM3'
    baudrate=115200,
    bytesize=8,
    parity='N',
    stopbits=1,
    timeout=1
)

# Send command
ser.write(b"SHT3X SINGLE HIGH\n")

# Read response
response = ser.readline()
print(response.decode())
```


## Command Reference

### Measurement Commands

#### Single-Shot Measurements
Perform one-time measurement with immediate response.

| Command | Repeatability | Duration | Typical Use |
|---------|---------------|----------|-------------|
| `SHT3X SINGLE HIGH` | High (0.015% RH) | 15 ms | Precision logging |
| `SHT3X SINGLE MEDIUM` | Medium (0.024% RH) | 6 ms | General purpose |
| `SHT3X SINGLE LOW` | Low (0.045% RH) | 4 ms | Fast sampling |

**Example:**
```bash
> SHT3X SINGLE HIGH
{"mode":"SINGLE","timestamp":1728930680,"temperature":23.45,"humidity":65.20}
```

#### Periodic Measurements
Continuous sampling with automatic 5-second interval output.

| Command | Rate | Repeatability | Use Case |
|---------|------|---------------|----------|
| `SHT3X PERIODIC 0.5 HIGH` | 0.5 Hz | High | Long-term logging |
| `SHT3X PERIODIC 1 MEDIUM` | 1 Hz | Medium | Standard monitoring |
| `SHT3X PERIODIC 2 LOW` | 2 Hz | Low | Fast response |
| `SHT3X PERIODIC 4 HIGH` | 4 Hz | High | Real-time control |
| `SHT3X PERIODIC 10 MEDIUM` | 10 Hz | Medium | High-speed sampling |

**Example:**
```bash
> SHT3X PERIODIC 1 HIGH
{"mode":"PERIODIC","timestamp":1728930680,"temperature":23.45,"humidity":65.20}
{"mode":"PERIODIC","timestamp":1728930685,"temperature":23.46,"humidity":65.18}
{"mode":"PERIODIC","timestamp":1728930690,"temperature":23.47,"humidity":65.22}
...
```

**Stop Periodic Mode:**
```bash
> SHT3X PERIODIC STOP
SHT3X PERIODIC STOP SUCCEEDED
```

### Control Commands

#### Heater Control
Built-in heater for condensation prevention and diagnostics.

| Command | Function | Power | Use Case |
|---------|----------|-------|----------|
| `SHT3X HEATER ENABLE` | Enable heater | ~5.5 mW | Remove condensation |
| `SHT3X HEATER DISABLE` | Disable heater | 0 mW | Normal operation |

**Example:**
```bash
> SHT3X HEATER ENABLE
SHT3X HEATER ENABLE SUCCEEDED

> SHT3X HEATER DISABLE
SHT3X HEATER DISABLE SUCCEEDED
```

**⚠️ Note:** Heater significantly affects temperature readings. Disable before measurement.

#### Accelerated Response Time (ART)
Quick-start periodic mode for fastest response.

| Command | Effect | Equivalent To |
|---------|--------|---------------|
| `SHT3X ART` | Start 4Hz high-precision mode | `SHT3X PERIODIC 4 HIGH` |

**Example:**
```bash
> SHT3X ART
{"mode":"PERIODIC","timestamp":1728930680,"temperature":23.45,"humidity":65.20}
...
```

### RTC Commands

#### Set Time
Configure DS3231 real-time clock.

**Command Format:**
```
DS3231 SETTIME <YYYY> <MM> <DD> <HH> <mm> <ss>
```

**Example:**
```bash
> DS3231 SETTIME 2024 10 14 15 30 00
DS3231 SETTIME SUCCEEDED
```

#### Get Time
Read current time from DS3231.

**Command:**
```
DS3231 GETTIME
```

**Example:**
```bash
> DS3231 GETTIME
Current Time: 2024-10-14 15:30:45
Unix Timestamp: 1728930645
```

### Command Summary Table

| Category | Command | Parameters | Response Type |
|----------|---------|------------|---------------|
| **Single** | `SHT3X SINGLE` | `HIGH\|MEDIUM\|LOW` | JSON object |
| **Periodic** | `SHT3X PERIODIC` | `0.5\|1\|2\|4\|10` `HIGH\|MEDIUM\|LOW` | JSON stream |
| **Periodic** | `SHT3X PERIODIC STOP` | - | Status message |
| **Control** | `SHT3X ART` | - | JSON stream |
| **Heater** | `SHT3X HEATER` | `ENABLE\|DISABLE` | Status message |
| **RTC** | `DS3231 SETTIME` | `YYYY MM DD HH mm ss` | Status message |
| **RTC** | `DS3231 GETTIME` | - | Formatted time |

## Data Output Format

### JSON Structure

All sensor measurements are output in JSON format with Unix timestamps:

```json
{
  "mode": "SINGLE|PERIODIC",
  "timestamp": 1728930680,
  "temperature": 23.45,
  "humidity": 65.20
}
```

#### Field Descriptions

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `mode` | String | `SINGLE` or `PERIODIC` | Measurement mode |
| `timestamp` | Integer | Unix epoch (seconds) | UTC time from DS3231 |
| `temperature` | Float | -40.0 to 125.0 | Temperature in °C (2 decimals) |
| `humidity` | Float | 0.0 to 100.0 | Relative humidity in % (2 decimals) |

### Output Timing

| Mode | Output Frequency | Trigger |
|------|------------------|---------|
| **Single** | Once per command | Immediate after measurement |
| **Periodic** | Every 5 seconds | Automatic from main loop |

**Note:** Sensor measures at configured rate (0.5-10Hz), but output is limited to 5-second intervals (200ms) to prevent UART buffer overflow.

### Status Messages

Non-JSON status messages for command acknowledgment:

```
SHT3X HEATER ENABLE SUCCEEDED
SHT3X HEATER DISABLE SUCCEEDED
SHT3X PERIODIC STOP SUCCEEDED
DS3231 SETTIME SUCCEEDED
UNKNOWN COMMAND
SHT3X SINGLE MODE FAILED
```

### Error Handling

| Error Condition | Output | Cause |
|-----------------|--------|-------|
| Unknown command | `UNKNOWN COMMAND` | Invalid command string |
| I2C timeout | `SHT3X SINGLE MODE FAILED` | Sensor not responding |
| Invalid parameter | _(No response)_ | Wrong parameter count/value |
| Buffer overflow | `JSON BUFFER OVERFLOW` | JSON exceeds 128 bytes |

## System Behavior

### Command Processing Flow

1. **UART Reception**: Bytes received via interrupt → Ring buffer
2. **Line Assembly**: Main loop reads ring buffer until `\r` or `\n`
3. **Tokenization**: Split command on whitespace: `["SHT3X", "SINGLE", "HIGH"]`
4. **Command Matching**: Exact string match in command table
5. **Function Dispatch**: Call corresponding parser function
6. **Driver Execution**: Parser validates parameters → SHT3X driver
7. **Data Management**: Driver results → DataManager → Set `data_ready` flag
8. **Output**: Next main loop iteration → `DataManager_Print()` → UART TX

**Detailed Flow:** See [FLOW_DIAGRAM.md](FLOW_DIAGRAM.md)

### State Management

#### Single-Shot Behavior
```
IDLE ──[SHT3X SINGLE]──> Measure ──> Update DataManager ──> IDLE
                         (15ms)       (set data_ready)
```
- Temporarily interrupts periodic mode if active
- Previous periodic state restored after measurement
- Immediate JSON output in next main loop iteration

#### Periodic Behavior
```
IDLE ──[SHT3X PERIODIC 1 HIGH]──> PERIODIC_1MPS ──[every 5s]──> JSON Output
                                        ↓
                           [SHT3X PERIODIC STOP]
                                        ↓
                                      IDLE
```
- Sensor measures at specified rate (e.g., 1Hz)
- Main loop fetches data every 5 seconds
- Output continues until `STOP` command or power cycle

#### Mode Transitions
| From State | Command | To State | Notes |
|------------|---------|----------|-------|
| IDLE | `SINGLE` | IDLE | One-time measurement |
| IDLE | `PERIODIC` | PERIODIC_* | Start continuous |
| PERIODIC_* | `SINGLE` | PERIODIC_* | Temporary interrupt |
| PERIODIC_* | `STOP` | IDLE | End continuous |
| Any | `HEATER` | Unchanged | Independent control |

### Timing Characteristics

| Operation | Duration | Notes |
|-----------|----------|-------|
| **Command Response** | <100 ms | From UART RX to JSON TX |
| **I2C Single Measurement** | 4-15 ms | Depends on repeatability |
| **I2C Fetch Data** | <5 ms | Read from periodic buffer |
| **JSON Formatting** | <1 ms | sprintf + sanitization |
| **Periodic Interval** | 5000 ms | Fixed output rate |
| **UART Transmission** | ~8 ms | 128 bytes @ 115200 baud |

### Main Loop Execution

```c
while (1) {
    UART_Handle();              // Process incoming commands (non-blocking)
    DataManager_Print();        // Output JSON if data_ready (non-blocking)
    
    if (periodic_mode_active) {
        if (time_to_fetch) {
            SHT3X_FetchData();          // I2C read (blocking ~5ms)
            DataManager_UpdatePeriodic(); // Set data_ready flag
        }
    }
    
    __WFI();  // Wait For Interrupt (low power)
}
```

**Cycle Time:** ~100-200ms per iteration (dominated by `__WFI()`)

**Detailed Sequences:** See [SEQUENCE_DIAGRAM.md](SEQUENCE_DIAGRAM.md)


## Technical Specifications

### Communication Parameters

| Parameter | Value | Configuration |
|-----------|-------|---------------|
| **UART Baud Rate** | 115200 | 8 data bits, no parity, 1 stop bit (8N1) |
| **UART Mode** | Interrupt RX | DMA disabled, polling TX |
| **I2C Speed** | 100 kHz | Standard mode (Sm) |
| **I2C Addressing** | 7-bit | SHT3X: 0x44, DS3231: 0x68 |
| **I2C Timeout** | 100 ms | Per transaction |
| **I2C Clock Stretch** | Disabled | Not used by SHT3X/DS3231 |

### Memory Usage

| Component | RAM | Flash | Description |
|-----------|-----|-------|-------------|
| **Ring Buffer** | 256 B | - | UART RX circular buffer |
| **Line Buffer** | 128 B | - | Command assembly buffer |
| **Command Table** | ~200 B | ~800 B | Command strings + function pointers |
| **SHT3X Driver** | 24 B | ~2 KB | Device state + I2C logic |
| **DS3231 Driver** | 8 B | ~1 KB | RTC interface |
| **DataManager** | 32 B | ~1.5 KB | State + JSON formatter |
| **Print Buffer** | 256 B | - | sprintf output buffer |
| **Total Overhead** | **<1 KB** | **<6 KB** | Core firmware only |

**Note:** HAL library and startup code require additional ~20KB flash.

### Performance Metrics

| Metric | Value | Conditions |
|--------|-------|------------|
| **End-to-End Latency** | <150 ms | Command RX → JSON TX |
| **Command Processing** | <100 ms | Tokenize + dispatch + execute |
| **I2C Transaction** | 4-15 ms | Single measurement (repeatability-dependent) |
| **I2C Fetch** | <5 ms | Periodic data read |
| **JSON Formatting** | <1 ms | sprintf + timestamp + sanitize |
| **UART TX** | ~8 ms | 128 bytes @ 115200 baud |
| **Main Loop Cycle** | 100-200 ms | With `__WFI()` sleep |
| **Max Throughput** | 10 Hz | Sensor periodic rate limit |

### Sensor Specifications (SHT3X)

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Temperature Range** | -40°C to +125°C | Functional range |
| **Humidity Range** | 0% to 100% RH | Non-condensing |
| **Temperature Accuracy** | ±0.2°C | Typical @ 25°C |
| **Humidity Accuracy** | ±2% RH | Typical @ 25°C, 20-80% RH |
| **Repeatability (HIGH)** | ±0.04°C, ±0.015% RH | Best precision |
| **Repeatability (MEDIUM)** | ±0.08°C, ±0.024% RH | Balanced |
| **Repeatability (LOW)** | ±0.15°C, ±0.045% RH | Fastest |
| **Response Time** | 2-8 seconds | 63% step change |
| **Heater Power** | 5.5 mW | For condensation removal |

### Error Handling

#### I2C Error Codes

| Error Type | Detection | Recovery |
|------------|-----------|----------|
| **Timeout** | No ACK within 100ms | Return `SHT3X_ERROR`, preserve state |
| **NACK** | Slave does not acknowledge | Return `SHT3X_ERROR`, log error code |
| **Arbitration Loss** | Multi-master collision | Retry (handled by HAL) |
| **Bus Error** | Invalid bus state | Reset I2C peripheral |

#### Data Validation

| Validation | Method | Action on Failure |
|------------|--------|-------------------|
| **CRC Check** | Polynomial 0x31 | Discard data, return error |
| **NaN/Inf Check** | `isnan()`, `isinf()` | Substitute with 0.0 |
| **Range Check** | -40 to 125°C, 0-100% | Log warning (optional) |
| **Buffer Overflow** | `snprintf()` return value | Print `JSON BUFFER OVERFLOW` |

#### Error Messages

```c
// I2C Errors
"SHT3X SINGLE MODE FAILED"
"SHT3X PERIODIC MODE FAILED"
"SHT3X HEATER ENABLE FAILED"

// Data Errors
"JSON BUFFER OVERFLOW"
"CRC VALIDATION FAILED" (internal)

// Command Errors
"UNKNOWN COMMAND"
```

### State Persistence

| Scenario | State Behavior | Example |
|----------|----------------|---------|
| **Single during Periodic** | Periodic preserved | 1Hz mode → Single → 1Hz resumes |
| **Failed I2C** | State unchanged | Command fails, mode stays PERIODIC |
| **Power Cycle** | Reset to IDLE | All settings lost |
| **Watchdog Reset** | Reset to IDLE | Emergency recovery |

## Architecture Details

### Centralized Data Management

The firmware uses a **single-point-of-output** architecture:

```c
// OLD Architecture (scattered output)
Parser → SHT3X_Single() → PRINT_CLI() ❌ Multiple output points
Parser → SHT3X_Periodic() → PRINT_CLI() ❌

// NEW Architecture (centralized)
Parser → SHT3X_Single() → DataManager_UpdateSingle() → Set flag
Main Loop → DataManager_Print() → PRINT_CLI() ✓ Single output point
```

**Benefits:**
- Consistent JSON formatting
- Unified timestamp handling
- Thread-safe output (no race conditions)
- Easy debugging (one place to monitor)

**Implementation:** See [DATALOGGER_MANAGER_GUIDE.md](Datalogger_Lib/DATALOGGER_MANAGER_GUIDE.md)

### Ring Buffer Design

Lock-free circular buffer for interrupt-safe UART reception:

```c
typedef struct {
    uint8_t* buffer;           // Storage array
    volatile uint16_t head;    // Write pointer (ISR)
    volatile uint16_t tail;    // Read pointer (main loop)
    uint16_t size;             // Buffer capacity
} ring_buffer_t;
```

**Key Features:**
- **Thread-Safe**: Volatile pointers prevent compiler optimizations
- **Single Producer/Consumer**: ISR writes, main loop reads
- **No Locking**: No mutex required (single producer/consumer model)
- **Overflow Handling**: Overwrites oldest data when full

### Command Dispatch System

String-based exact matching with function pointer table:

```c
typedef struct {
    const char* cmdString;                      // "SHT3X SINGLE HIGH"
    void (*func)(uint8_t argc, char** argv);    // SHT3X_Single_Parser
} command_function_t;

command_function_t cmdTable[] = {
    {"SHT3X SINGLE HIGH",    SHT3X_Single_Parser},
    {"SHT3X SINGLE MEDIUM",  SHT3X_Single_Parser},
    {"SHT3X PERIODIC 1 HIGH", SHT3X_Periodic_Parser},
    // ... 22 total commands
};
```

**Advantages:**
- O(n) lookup, acceptable for small table (n=22)
- Human-readable command strings
- Easy to add new commands
- No hash collisions

**Detailed UML:** See [UML_CLASS_DIAGRAM.md](UML_CLASS_DIAGRAM.md)

### JSON Output Pipeline

```
Sensor Data → DataManager → Sanitize → Timestamp → Format → UART
     ↓            ↓            ↓           ↓          ↓        ↓
  float[]     Store in    Check NaN   DS3231 RTC  sprintf   TX DMA
             g_state                   I2C read    buffer
```

**Safety Features:**
1. **Sanitization**: Replace NaN/Inf with 0.0
2. **Timestamp Validation**: mktime() bounds checking
3. **Buffer Overflow**: snprintf() return value check
4. **Atomic Flag**: `data_ready` prevents concurrent access

## Integration Guide

### Adding Custom Commands

**Step 1:** Add command string to `cmd_func.c`
```c
command_function_t cmdTable[] = {
    // ... existing commands ...
    {"MY_SENSOR READ", MySensor_Read_Parser},
    {NULL, NULL}  // Terminator
};
```

**Step 2:** Implement parser in `cmd_parser.c`
```c
void MySensor_Read_Parser(uint8_t argc, char** argv) {
    if (argc < 3) {
        PRINT_CLI("Usage: MY_SENSOR READ <param>\r\n");
        return;
    }
    
    // Parse parameters
    // Call driver
    // Update DataManager or print directly
}
```

**Step 3:** Add function prototype to `cmd_parser.h`
```c
void MySensor_Read_Parser(uint8_t argc, char** argv);
```

### Modifying Periodic Output Interval

Change the fetch interval in `main.c`:

```c
// Default: 5 seconds
uint32_t periodic_interval_ms = 5000;

// Change to 1 second (faster output)
uint32_t periodic_interval_ms = 1000;

// Change to 10 seconds (slower output)
uint32_t periodic_interval_ms = 10000;
```

**Note:** Sensor still measures at configured rate (e.g., 1Hz), only output frequency changes.

### Supporting Multiple Sensors

**Option 1:** Different I2C addresses
```c
// Initialize multiple SHT3X instances
SHT3X_Init(&g_sht3x_1, &hi2c1, SHT3X_I2C_ADDR_GND);  // 0x44
SHT3X_Init(&g_sht3x_2, &hi2c1, SHT3X_I2C_ADDR_VDD);  // 0x45
```

**Option 2:** I2C multiplexer (TCA9548A)
```c
// Select channel before communication
I2C_Multiplexer_Select(CHANNEL_0);
SHT3X_Single(&g_sht3x, ...);

I2C_Multiplexer_Select(CHANNEL_1);
SHT3X_Single(&g_sht3x, ...);
```

### Extending DataManager for New Sensors

Add sensor data structure in `data_manager.h`:

```c
typedef struct {
    float pressure;
    float altitude;
} sensor_data_bmp280_t;

typedef struct {
    datalogger_mode_t mode;
    uint32_t timestamp;
    sensor_data_sht3x_t sht3x;
    sensor_data_bmp280_t bmp280;  // New sensor
    bool data_ready;
} datalogger_state_t;
```

Update JSON formatter in `sensor_json_output.c`:

```c
snprintf(buffer, size,
    "{\"mode\":\"%s\",\"timestamp\":%lu,"
    "\"temperature\":%.2f,\"humidity\":%.2f,"
    "\"pressure\":%.2f,\"altitude\":%.2f}\r\n",  // New fields
    mode_str, timestamp, temp, hum, pressure, altitude);
```

## Troubleshooting

### Common Issues

| Problem | Symptoms | Solution |
|---------|----------|----------|
| **No serial output** | Terminal shows nothing | Check UART TX pin (PA9), baud rate (115200) |
| **Garbled output** | Random characters | Verify baud rate, check GND connection |
| **UNKNOWN COMMAND** | Valid commands not recognized | Check exact command spelling (case-sensitive) |
| **I2C timeout** | Measurements fail | Verify I2C pull-ups (4.7kΩ), check connections |
| **Wrong timestamp** | Future/past dates | Set DS3231 time with `DS3231 SETTIME` |
| **NaN values** | Temperature/humidity = 0.00 | Check SHT3X power, I2C address (ADDR pin) |
| **Periodic not stopping** | Keeps outputting after STOP | Send `SHT3X PERIODIC STOP` (not just `STOP`) |

### Debug Tools

#### Enable Debug Output (Optional)

Add to `main.c`:

```c
#define DEBUG_ENABLED

#ifdef DEBUG_ENABLED
  PRINT_CLI("DEBUG: Command received: %s\r\n", command_buffer);
  PRINT_CLI("DEBUG: I2C status: %d\r\n", i2c_status);
#endif
```

#### UART Loopback Test

Short PA9 (TX) to PA10 (RX), type characters – should echo back.

#### I2C Bus Scan

```c
for (uint8_t addr = 0x00; addr < 0x7F; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 100) == HAL_OK) {
        PRINT_CLI("Found device: 0x%02X\r\n", addr);
    }
}
// Expected: 0x44 (SHT3X), 0x68 (DS3231)
```

#### Monitor I2C with Logic Analyzer

- **Tool**: Saleae Logic, DSLogic
- **Signals**: SCL (PB6), SDA (PB7), GND
- **Protocol**: I2C, 100kHz
- **Look for**: START, address, ACK, data, STOP

### Error Recovery

| Error | Manual Recovery | Automatic Recovery |
|-------|-----------------|-------------------|
| **I2C Bus Hung** | Power cycle | I2C peripheral reset (not implemented) |
| **UART Buffer Full** | Stop sending commands | Ring buffer overwrites oldest data |
| **Watchdog Reset** | Check infinite loops | System restarts to IDLE |
| **Hard Fault** | Debug with ST-Link | Fault handler (not implemented) |

## Advanced Features

### Low-Power Operation (Optional)

Enable low-power mode in main loop:

```c
while (1) {
    UART_Handle();
    DataManager_Print();
    
    // Enter STOP mode instead of WFI
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    
    // Wake on UART interrupt or timer
}
```

**Power Savings:** ~1mA in STOP mode vs ~10mA in RUN mode

### Watchdog Timer (Optional)

Enable IWDG for fault recovery:

```c
// In main.c init
MX_IWDG_Init();  // 4-second timeout

// In main loop
while (1) {
    HAL_IWDG_Refresh(&hiwdg);  // Pet the watchdog
    // ... normal operations ...
}
```

### DMA UART (Optional)

Replace polling TX with DMA for non-blocking output:

```c
// In print_cli.c
HAL_UART_Transmit_DMA(&huart1, (uint8_t*)buffer, len);
```

**Benefits:** CPU-free during transmission, better multitasking

## Documentation

### Detailed Design Documents

- **[FLOW_DIAGRAM.md](FLOW_DIAGRAM.md)** - Control flow and decision logic
- **[SEQUENCE_DIAGRAM.md](SEQUENCE_DIAGRAM.md)** - Time-ordered component interactions
- **[UML_CLASS_DIAGRAM.md](UML_CLASS_DIAGRAM.md)** - Class structure and relationships
- **[DATALOGGER_MANAGER_GUIDE.md](Datalogger_Lib/DATALOGGER_MANAGER_GUIDE.md)** - Centralized architecture guide

### API Documentation

Generate Doxygen documentation (requires Doxygen installed):

```bash
cd STM32/
doxygen Doxyfile  # Generate HTML docs
firefox html/index.html
```

### Code Examples

See `firmware/ESP32/pytest_mqtt5.py` for Python integration example.

## Testing

### Unit Testing (Manual)

**Test 1: Single Measurement**
```bash
> SHT3X SINGLE HIGH
Expected: JSON with mode="SINGLE", valid temp/hum
```

**Test 2: Periodic Start/Stop**
```bash
> SHT3X PERIODIC 1 HIGH
Wait 15 seconds (expect 3 outputs)
> SHT3X PERIODIC STOP
Expected: Outputs stop, "SUCCEEDED" message
```

**Test 3: Mode Switching**
```bash
> SHT3X PERIODIC 1 HIGH
Wait 5 seconds
> SHT3X SINGLE MEDIUM
Expected: Single JSON, then periodic resumes
```

**Test 4: RTC**
```bash
> DS3231 SETTIME 2024 10 15 12 00 00
> DS3231 GETTIME
Expected: Time matches, timestamp ≈ 1729000000
```

### Integration Testing

**ESP32 Bridge Test:**
1. Connect STM32 to ESP32 via UART
2. ESP32 forwards MQTT commands to STM32
3. Verify JSON output appears on MQTT broker

**Web Dashboard Test:**
1. Open web dashboard
2. Send command via MQTT
3. Verify real-time graph updates

## Performance Optimization

### Reducing Latency

1. **Increase I2C Speed** (400kHz Fast Mode)
   ```c
   // In stm32f1xx_hal_msp.c
   hi2c1.Init.ClockSpeed = 400000;
   ```

2. **Disable `__WFI()`** (higher power, lower latency)
   ```c
   // In main.c
   // __WFI();  // Comment out for continuous polling
   ```

3. **Use DMA for UART TX** (non-blocking)

### Increasing Throughput

1. **Reduce Periodic Interval**
   ```c
   uint32_t periodic_interval_ms = 1000;  // 1 second instead of 5
   ```

2. **Compress JSON Output**
   ```json
   {"m":"P","t":1728930680,"T":23.45,"H":65.20}
   ```

3. **Binary Protocol** (not JSON)
   - Replace JSON with binary struct
   - 16 bytes vs 82 bytes JSON

## License

MIT License - See project root `LICENSE.md` for details.

---

## Quick Reference Card

### Most Used Commands
```bash
SHT3X SINGLE HIGH              # One measurement
SHT3X PERIODIC 1 HIGH          # Start 1Hz monitoring
SHT3X PERIODIC STOP            # Stop monitoring
DS3231 SETTIME 2024 10 15 12 0 0  # Set RTC
```

### Hardware Checklist
- [ ] STM32F103 powered (3.3V)
- [ ] SHT3X connected (SCL=PB6, SDA=PB7, ADDR=GND)
- [ ] DS3231 connected (shared I2C bus)
- [ ] I2C pull-ups installed (4.7kΩ)
- [ ] UART connected (TX=PA9, RX=PA10)
- [ ] Terminal configured (115200 baud, 8N1)

### Troubleshooting Checklist
- [ ] Check UART baud rate matches (115200)
- [ ] Verify I2C devices respond (0x44, 0x68)
- [ ] Confirm command spelling (case-sensitive)
- [ ] Check power supply (3.3V stable)
- [ ] Test with known-good commands (`SHT3X SINGLE HIGH`)

### Technical Specs Summary
| Parameter | Value |
|-----------|-------|
| UART | 115200 baud, 8N1 |
| I2C | 100kHz, 7-bit addressing |
| Memory | <1KB RAM, <6KB Flash |
| Latency | <150ms end-to-end |
| Output Rate | 5 seconds (periodic) |
| Temperature | -40°C to +125°C |
| Humidity | 0% to 100% RH |

---

**Project Repository:** [GitHub - Datalogger](https://github.com/meeTeam05/Datalogger)  
**Documentation:** See `FLOW_DIAGRAM.md`, `SEQUENCE_DIAGRAM.md`, `UML_CLASS_DIAGRAM.md`  
**Support:** Open an issue on GitHub

