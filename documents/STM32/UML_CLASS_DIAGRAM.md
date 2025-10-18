# STM32 Data Logger - UML Class Diagram

This document provides the UML class diagrams showing the structure and relationships of the STM32 firmware components.

## Complete System Class Diagram

```mermaid
classDiagram
    class main {
        +I2C_HandleTypeDef hi2c1
        +UART_HandleTypeDef huart1
        +sht3x_t g_sht3x
        +ds3231_t g_ds3231
        +uint32_t next_fetch_ms
        +uint32_t periodic_interval_ms
        -float outT
        -float outRH
        -uint32_t last_fetch_ms
        +main() int
        +SystemClock_Config() void
        -MX_GPIO_Init() void
        -MX_I2C1_Init() void
        -MX_USART1_UART_Init() void
    }
    
    class UART {
        -UART_HandleTypeDef* huart
        -ring_buffer_t rx_buffer
        -char line_buffer[128]
        -uint16_t line_index
        +UART_Init(UART_HandleTypeDef*) void
        +UART_Handle() void
        +UART_RxCallback(uint8_t) void
        -assemble_line() bool
    }
    
    class RingBuffer {
        -uint8_t* buffer
        -volatile uint16_t head
        -volatile uint16_t tail
        -uint16_t size
        +RingBuffer_Init(ring_buffer_t*, uint8_t*, uint16_t) void
        +RingBuffer_Write(ring_buffer_t*, uint8_t) bool
        +RingBuffer_Read(ring_buffer_t*, uint8_t*) bool
        +RingBuffer_Available(ring_buffer_t*) uint16_t
        +RingBuffer_Peek(ring_buffer_t*, uint8_t*) bool
        +RingBuffer_Clear(ring_buffer_t*) void
        +RingBuffer_IsFull(ring_buffer_t*) bool
        +RingBuffer_IsEmpty(ring_buffer_t*) bool
    }
    
    class CommandExecute {
        +COMMAND_EXECUTE(char*) void
        -tokenize_string(char*, char**, uint8_t) uint8_t
        -find_command(uint8_t, char**) command_function_t*
    }
    
    class CommandTable {
        +command_function_t cmdTable[]
        +GET_CMD_TABLE_SIZE() uint8_t
    }
    
    class CommandFunction {
        +const char* cmdString
        +void (*func)(uint8_t, char**)
    }
    
    class CommandParser {
        +Cmd_Default(uint8_t, char**) void
        +SHT3X_Single_Parser(uint8_t, char**) void
        +SHT3X_Periodic_Parser(uint8_t, char**) void
        +SHT3X_Heater_Parser(uint8_t, char**) void
        +SHT3X_PeriodicStop_Parser(uint8_t, char**) void
        +SHT3X_ART_Parser(uint8_t, char**) void
        +DS3231_SetTime_Parser(uint8_t, char**) void
        +DS3231_GetTime_Parser(uint8_t, char**) void
    }
    
    class SHT3XDriver {
        -I2C_HandleTypeDef* hi2c
        -uint8_t device_address
        -float temperature
        -float humidity
        -sht3x_mode_t currentState
        -sht3x_repeat_t modeRepeat
        +SHT3X_Init(sht3x_t*, I2C_HandleTypeDef*, uint8_t) void
        +SHT3X_Single(sht3x_t*, sht3x_repeat_t*, float*, float*) SHT3X_StatusTypeDef
        +SHT3X_Periodic(sht3x_t*, sht3x_mode_t*, sht3x_repeat_t*) SHT3X_StatusTypeDef
        +SHT3X_FetchData(sht3x_t*, float*, float*) SHT3X_StatusTypeDef
        +SHT3X_PeriodicStop(sht3x_t*) SHT3X_StatusTypeDef
        +SHT3X_Heater(sht3x_t*, sht3x_heater_mode_t*) SHT3X_StatusTypeDef
        +SHT3X_ART(sht3x_t*) SHT3X_StatusTypeDef
        +SHT3X_SoftReset(sht3x_t*) SHT3X_StatusTypeDef
        -calculate_crc(uint8_t*, uint8_t) uint8_t
        -parse_raw_data(uint8_t*, float*, float*) SHT3X_StatusTypeDef
    }
    
    class SHT3XEnums {
        <<enumeration>>
        SHT3X_OK
        SHT3X_ERROR
    }
    
    class SHT3XMode {
        <<enumeration>>
        SHT3X_IDLE
        SHT3X_SINGLE_SHOT
        SHT3X_PERIODIC_05MPS
        SHT3X_PERIODIC_1MPS
        SHT3X_PERIODIC_2MPS
        SHT3X_PERIODIC_4MPS
        SHT3X_PERIODIC_10MPS
    }
    
    class SHT3XRepeat {
        <<enumeration>>
        SHT3X_HIGH
        SHT3X_MEDIUM
        SHT3X_LOW
    }
    
    class SHT3XHeater {
        <<enumeration>>
        SHT3X_HEATER_ENABLE
        SHT3X_HEATER_DISABLE
    }
    
    class DS3231Driver {
        -I2C_HandleTypeDef* hi2c
        -uint8_t device_address
        +DS3231_Init(ds3231_t*, I2C_HandleTypeDef*) void
        +DS3231_Set_Time(ds3231_t*, struct tm*) DS3231_StatusTypeDef
        +DS3231_Get_Time(ds3231_t*, struct tm*) DS3231_StatusTypeDef
        -bcd_to_decimal(uint8_t) uint8_t
        -decimal_to_bcd(uint8_t) uint8_t
    }
    
    class DataManager {
        -datalogger_state_t g_datalogger_state
        +DataManager_Init() void
        +DataManager_UpdateSingle(float, float) void
        +DataManager_UpdatePeriodic(float, float) void
        +DataManager_Print() bool
        +DataManager_IsDataReady() bool
        +DataManager_ClearDataReady() void
        +DataManager_GetState() datalogger_state_t*
    }
    
    class DataLoggerState {
        -datalogger_mode_t mode
        -uint32_t timestamp
        -sensor_data_sht3x_t sht3x
        -bool data_ready
    }
    
    class DataLoggerMode {
        <<enumeration>>
        DATALOGGER_MODE_IDLE
        DATALOGGER_MODE_SINGLE
        DATALOGGER_MODE_PERIODIC
    }
    
    class SensorDataSHT3X {
        +float temperature
        +float humidity
    }
    
    class SensorJSONOutput {
        +sensor_json_output_send(const char*, float, float) void
        -sanitize_float(float, float) float
        -get_unix_timestamp() uint32_t
    }
    
    class PrintCLI {
        -char stringBuffer[256]
        +PRINT_CLI(const char*, ...) void
    }
    
    %% Relationships
    main --> UART : uses
    main --> SHT3XDriver : uses g_sht3x
    main --> DS3231Driver : uses g_ds3231
    main --> DataManager : uses
    
    UART --> RingBuffer : contains rx_buffer
    UART --> CommandExecute : calls
    
    CommandExecute --> CommandTable : searches
    CommandTable --> CommandFunction : contains array of
    CommandExecute --> CommandParser : dispatches to
    
    CommandParser --> SHT3XDriver : calls
    CommandParser --> DS3231Driver : calls
    CommandParser --> DataManager : updates
    CommandParser --> PrintCLI : uses
    
    SHT3XDriver --> SHT3XEnums : returns
    SHT3XDriver --> SHT3XMode : uses
    SHT3XDriver --> SHT3XRepeat : uses
    SHT3XDriver --> SHT3XHeater : uses
    
    DataManager --> DataLoggerState : contains
    DataLoggerState --> DataLoggerMode : uses
    DataLoggerState --> SensorDataSHT3X : contains
    
    DataManager --> SensorJSONOutput : uses
    DataManager --> DS3231Driver : uses for timestamp
    DataManager --> PrintCLI : uses
    
    SensorJSONOutput --> PrintCLI : uses
```

## Core Component Details

### UART and Ring Buffer Component

```mermaid
classDiagram
    class ring_buffer_t {
        +uint8_t* buffer
        +volatile uint16_t head
        +volatile uint16_t tail
        +uint16_t size
    }
    
    class UARTModule {
        -UART_HandleTypeDef* huart
        -ring_buffer_t rx_buffer
        -uint8_t rx_buffer_storage[256]
        -char line_buffer[128]
        -uint16_t line_index
        +UART_Init(UART_HandleTypeDef*) void
        +UART_Handle() void
        +UART_RxCallback(uint8_t) void
        -assemble_line() bool
    }
    
    class RingBufferAPI {
        <<interface>>
        +RingBuffer_Init(ring_buffer_t*, uint8_t*, uint16_t) void
        +RingBuffer_Write(ring_buffer_t*, uint8_t) bool
        +RingBuffer_Read(ring_buffer_t*, uint8_t*) bool
        +RingBuffer_Available(ring_buffer_t*) uint16_t
        +RingBuffer_Peek(ring_buffer_t*, uint8_t*) bool
        +RingBuffer_Clear(ring_buffer_t*) void
        +RingBuffer_IsFull(ring_buffer_t*) bool
        +RingBuffer_IsEmpty(ring_buffer_t*) bool
    }
    
    UARTModule --> ring_buffer_t : uses
    UARTModule ..> RingBufferAPI : calls
    RingBufferAPI ..> ring_buffer_t : operates on
```

### Command Processing Component

```mermaid
classDiagram
    class command_function_t {
        +const char* cmdString
        +void (*func)(uint8_t argc, char** argv)
    }
    
    class CommandExecutor {
        <<service>>
        +COMMAND_EXECUTE(char* commandBuffer) void
        -tokenize_string(char*, char**, uint8_t) uint8_t
        -find_command(uint8_t, char**) command_function_t*
    }
    
    class CommandRegistry {
        <<static>>
        +command_function_t cmdTable[]
        +"SHT3X SINGLE HIGH"
        +"SHT3X SINGLE MEDIUM"
        +"SHT3X SINGLE LOW"
        +"SHT3X PERIODIC 0.5 HIGH"
        +"SHT3X PERIODIC 1 HIGH"
        +"SHT3X PERIODIC STOP"
        +"SHT3X HEATER ENABLE"
        +"SHT3X HEATER DISABLE"
        +"DS3231 SETTIME"
        +"DS3231 GETTIME"
        +GET_CMD_TABLE_SIZE() uint8_t
    }
    
    class ICommandHandler {
        <<interface>>
        +handle(uint8_t argc, char** argv) void
    }
    
    class SHT3XHandlers {
        +SHT3X_Single_Parser(uint8_t, char**) void
        +SHT3X_Periodic_Parser(uint8_t, char**) void
        +SHT3X_Heater_Parser(uint8_t, char**) void
        +SHT3X_PeriodicStop_Parser(uint8_t, char**) void
        +SHT3X_ART_Parser(uint8_t, char**) void
    }
    
    class DS3231Handlers {
        +DS3231_SetTime_Parser(uint8_t, char**) void
        +DS3231_GetTime_Parser(uint8_t, char**) void
    }
    
    class DefaultHandler {
        +Cmd_Default(uint8_t, char**) void
    }
    
    CommandExecutor --> CommandRegistry : queries
    CommandRegistry --> command_function_t : array of
    command_function_t --> ICommandHandler : function pointer
    SHT3XHandlers ..|> ICommandHandler
    DS3231Handlers ..|> ICommandHandler
    DefaultHandler ..|> ICommandHandler
```

### SHT3X Sensor Driver Component

```mermaid
classDiagram
    class sht3x_t {
        +I2C_HandleTypeDef* hi2c
        +uint8_t device_address
        +float temperature
        +float humidity
        +sht3x_mode_t currentState
        +sht3x_repeat_t modeRepeat
    }
    
    class SHT3X_StatusTypeDef {
        <<enumeration>>
        SHT3X_OK
        SHT3X_ERROR
    }
    
    class sht3x_mode_t {
        <<enumeration>>
        SHT3X_IDLE
        SHT3X_SINGLE_SHOT
        SHT3X_PERIODIC_05MPS
        SHT3X_PERIODIC_1MPS
        SHT3X_PERIODIC_2MPS
        SHT3X_PERIODIC_4MPS
        SHT3X_PERIODIC_10MPS
    }
    
    class sht3x_repeat_t {
        <<enumeration>>
        SHT3X_HIGH
        SHT3X_MEDIUM
        SHT3X_LOW
    }
    
    class sht3x_heater_mode_t {
        <<enumeration>>
        SHT3X_HEATER_ENABLE
        SHT3X_HEATER_DISABLE
    }
    
    class SHT3XDriverAPI {
        <<interface>>
        +SHT3X_Init(sht3x_t*, I2C_HandleTypeDef*, uint8_t) void
        +SHT3X_Single(sht3x_t*, sht3x_repeat_t*, float*, float*) SHT3X_StatusTypeDef
        +SHT3X_Periodic(sht3x_t*, sht3x_mode_t*, sht3x_repeat_t*) SHT3X_StatusTypeDef
        +SHT3X_FetchData(sht3x_t*, float*, float*) SHT3X_StatusTypeDef
        +SHT3X_PeriodicStop(sht3x_t*) SHT3X_StatusTypeDef
        +SHT3X_Heater(sht3x_t*, sht3x_heater_mode_t*) SHT3X_StatusTypeDef
        +SHT3X_ART(sht3x_t*) SHT3X_StatusTypeDef
        +SHT3X_SoftReset(sht3x_t*) SHT3X_StatusTypeDef
    }
    
    class SHT3XDriverPrivate {
        <<private>>
        -calculate_crc(uint8_t* data, uint8_t length) uint8_t
        -parse_raw_data(uint8_t* raw_data, float* temp, float* hum) SHT3X_StatusTypeDef
        -build_command_word(sht3x_mode_t, sht3x_repeat_t) uint16_t
    }
    
    sht3x_t --> sht3x_mode_t : currentState
    sht3x_t --> sht3x_repeat_t : modeRepeat
    SHT3XDriverAPI ..> sht3x_t : operates on
    SHT3XDriverAPI ..> SHT3X_StatusTypeDef : returns
    SHT3XDriverAPI ..> sht3x_heater_mode_t : uses
    SHT3XDriverPrivate ..> sht3x_t : supports
```

### Data Manager Component

```mermaid
classDiagram
    class sensor_data_sht3x_t {
        +float temperature
        +float humidity
    }
    
    class datalogger_mode_t {
        <<enumeration>>
        DATALOGGER_MODE_IDLE
        DATALOGGER_MODE_SINGLE
        DATALOGGER_MODE_PERIODIC
    }
    
    class datalogger_state_t {
        +datalogger_mode_t mode
        +uint32_t timestamp
        +sensor_data_sht3x_t sht3x
        +bool data_ready
    }
    
    class DataManagerAPI {
        <<interface>>
        +DataManager_Init() void
        +DataManager_UpdateSingle(float, float) void
        +DataManager_UpdatePeriodic(float, float) void
        +DataManager_Print() bool
        +DataManager_IsDataReady() bool
        +DataManager_ClearDataReady() void
        +DataManager_GetState() datalogger_state_t*
    }
    
    class DataManagerPrivate {
        <<private>>
        -datalogger_state_t g_datalogger_state
        -format_json_output(char*, size_t) void
        -validate_sensor_data() bool
    }
    
    datalogger_state_t --> datalogger_mode_t : mode
    datalogger_state_t --> sensor_data_sht3x_t : sht3x
    DataManagerAPI ..> datalogger_state_t : manages
    DataManagerPrivate --> datalogger_state_t : stores
```

### DS3231 RTC Driver Component

```mermaid
classDiagram
    class ds3231_t {
        +I2C_HandleTypeDef* hi2c
        +uint8_t device_address
    }
    
    class DS3231_StatusTypeDef {
        <<enumeration>>
        DS3231_OK
        DS3231_ERROR
    }
    
    class tm {
        <<struct>>
        +int tm_sec
        +int tm_min
        +int tm_hour
        +int tm_mday
        +int tm_mon
        +int tm_year
        +int tm_wday
    }
    
    class DS3231DriverAPI {
        <<interface>>
        +DS3231_Init(ds3231_t*, I2C_HandleTypeDef*) void
        +DS3231_Set_Time(ds3231_t*, struct tm*) DS3231_StatusTypeDef
        +DS3231_Get_Time(ds3231_t*, struct tm*) DS3231_StatusTypeDef
    }
    
    class DS3231DriverPrivate {
        <<private>>
        -bcd_to_decimal(uint8_t) uint8_t
        -decimal_to_bcd(uint8_t) uint8_t
        -read_registers(uint8_t, uint8_t*, uint8_t) DS3231_StatusTypeDef
        -write_registers(uint8_t, uint8_t*, uint8_t) DS3231_StatusTypeDef
    }
    
    DS3231DriverAPI ..> ds3231_t : operates on
    DS3231DriverAPI ..> DS3231_StatusTypeDef : returns
    DS3231DriverAPI ..> tm : uses
    DS3231DriverPrivate ..> ds3231_t : supports
```

### Output and Formatting Component

```mermaid
classDiagram
    class SensorJSONOutput {
        <<service>>
        +sensor_json_output_send(const char* mode, float temp, float hum) void
        -sanitize_float(float value, float default_value) float
        -get_unix_timestamp() uint32_t
        -JSON_BUFFER_SIZE : 128
    }
    
    class PrintCLI {
        <<service>>
        +PRINT_CLI(const char* fmt, ...) void
        -stringBuffer[256]
    }
    
    class HAL_UART {
        <<external>>
        +HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t*, uint16_t, uint32_t) HAL_StatusTypeDef
    }
    
    SensorJSONOutput ..> PrintCLI : uses
    PrintCLI ..> HAL_UART : uses
```

## Component Dependencies

```mermaid
graph TB
    Main[main.c]
    UART[UART Module]
    RingBuf[Ring Buffer]
    CmdExec[Command Execute]
    CmdTable[Command Table]
    CmdParser[Command Parser]
    SHT3X[SHT3X Driver]
    DS3231[DS3231 Driver]
    DataMgr[Data Manager]
    JSON[Sensor JSON Output]
    PrintCLI[Print CLI]
    HAL_I2C[HAL I2C]
    HAL_UART[HAL UART]
    
    Main --> UART
    Main --> SHT3X
    Main --> DS3231
    Main --> DataMgr
    
    UART --> RingBuf
    UART --> CmdExec
    
    CmdExec --> CmdTable
    CmdExec --> CmdParser
    
    CmdParser --> SHT3X
    CmdParser --> DS3231
    CmdParser --> DataMgr
    CmdParser --> PrintCLI
    
    SHT3X --> HAL_I2C
    DS3231 --> HAL_I2C
    
    DataMgr --> JSON
    DataMgr --> DS3231
    DataMgr --> PrintCLI
    
    JSON --> PrintCLI
    PrintCLI --> HAL_UART
    
    style Main fill:#90EE90
    style DataMgr fill:#FFD700
    style SHT3X fill:#87CEEB
    style DS3231 fill:#87CEEB
```

## Object Lifecycle Diagram

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    
    Uninitialized --> Initialized : main() calls init functions
    
    state Initialized {
        [*] --> UART_Ready
        [*] --> SHT3X_IDLE
        [*] --> DS3231_Ready
        [*] --> DataManager_Ready
        
        UART_Ready --> UART_Receiving : Interrupt
        UART_Receiving --> UART_Ready : Character stored
        
        SHT3X_IDLE --> SHT3X_SingleShot : Single command
        SHT3X_IDLE --> SHT3X_Periodic : Periodic command
        SHT3X_SingleShot --> SHT3X_IDLE : Measurement complete
        SHT3X_Periodic --> SHT3X_IDLE : Stop command
        
        DataManager_Ready --> DataManager_DataReady : Update called
        DataManager_DataReady --> DataManager_Ready : Print complete
    }
    
    Initialized --> Error : HAL Error
    Error --> Initialized : Reset/Recovery
```

---

**Key Design Patterns:**

1. **Service Locator**: Command table acts as a registry of command handlers
2. **Strategy Pattern**: Different command parsers implement different handling strategies
3. **State Pattern**: SHT3X driver maintains state (IDLE, SINGLE_SHOT, PERIODIC_*)
4. **Singleton**: DataManager uses static internal state (g_datalogger_state)
5. **Observer**: UART interrupt observes hardware and notifies ring buffer
6. **Producer-Consumer**: Ring buffer mediates between interrupt (producer) and main loop (consumer)

**Key Relationships:**

- **Composition** (solid diamond): UART contains ring_buffer_t
- **Association** (solid line): main uses SHT3X driver
- **Dependency** (dashed line): Parser depends on Driver
- **Realization** (dashed line with triangle): Handlers implement ICommandHandler
