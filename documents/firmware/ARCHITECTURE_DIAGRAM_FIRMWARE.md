# STM32 Data Logger - Architecture Diagrams

This document provides high-level architecture diagrams showing the system structure, component dependencies, and data flow of the STM32 firmware.

## System Architecture Overview

```mermaid
graph TB
    subgraph "STM32F103C8T6 Microcontroller"
        subgraph "Application Layer"
            Main[Main Control Loop]
            CMD[Command Processing]
            DM[Data Manager]
        end

        subgraph "Driver Layer"
            SHT3X[SHT3X Driver]
            DS3231[DS3231 RTC Driver]
            SD[SD Card Manager]
            DISP[Display Driver]
        end

        subgraph "Communication Layer"
            UART[UART Module]
            RB[Ring Buffer]
            JSON[JSON Formatter]
        end

        subgraph "HAL Layer"
            HAL_I2C[HAL I2C]
            HAL_SPI[HAL SPI]
            HAL_UART[HAL UART]
        end
    end

    subgraph "External Hardware"
        SENSOR[SHT3X Sensor]
        RTC[DS3231 RTC]
        SDCARD[SD Card]
        LCD[ILI9225 Display]
        ESP32[ESP32 WiFi Module]
    end

    %% Application Layer Connections
    Main --> CMD
    Main --> DM
    Main --> SHT3X
    Main --> DS3231
    Main --> SD
    Main --> DISP

    CMD --> UART
    CMD --> DM

    DM --> JSON
    DM --> SD

    %% Communication Layer
    UART --> RB
    JSON --> UART

    %% Driver to HAL
    SHT3X --> HAL_I2C
    DS3231 --> HAL_I2C
    SD --> HAL_SPI
    DISP --> HAL_SPI
    UART --> HAL_UART

    %% HAL to Hardware
    HAL_I2C --> SENSOR
    HAL_I2C --> RTC
    HAL_SPI --> SDCARD
    HAL_SPI --> LCD
    HAL_UART --> ESP32

    style Main fill:#90EE90
    style DM fill:#FFD700
    style SD fill:#FF6B6B
    style DISP fill:#DDA0DD
    style SHT3X fill:#87CEEB
    style DS3231 fill:#87CEEB
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
    WiFiMgr[WiFi/MQTT Manager]
    SD[SD Card Manager]
    Display[Display Library]
    ILI9225[ILI9225 Driver]
    PrintCLI[Print CLI]
    HAL_I2C[HAL I2C]
    HAL_SPI[HAL SPI]
    HAL_UART[HAL UART]

    Main --> UART
    Main --> SHT3X
    Main --> DS3231
    Main --> DataMgr
    Main --> WiFiMgr
    Main --> SD
    Main --> Display

    UART --> RingBuf
    UART --> CmdExec

    CmdExec --> CmdTable
    CmdExec --> CmdParser

    CmdParser --> SHT3X
    CmdParser --> DS3231
    CmdParser --> DataMgr
    CmdParser --> WiFiMgr
    CmdParser --> SD
    CmdParser --> PrintCLI

    SHT3X --> HAL_I2C
    DS3231 --> HAL_I2C

    DataMgr --> JSON
    DataMgr --> DS3231
    DataMgr --> WiFiMgr
    DataMgr --> SD
    DataMgr --> PrintCLI

    SD --> HAL_SPI

    Display --> ILI9225
    ILI9225 --> HAL_SPI

    JSON --> PrintCLI
    PrintCLI --> HAL_UART

    style Main fill:#90EE90
    style DataMgr fill:#FFD700
    style SD fill:#FF6B6B
    style WiFiMgr fill:#87CEEB
    style Display fill:#DDA0DD
    style SHT3X fill:#87CEEB
    style DS3231 fill:#87CEEB
```

## Data Flow Architecture

### Normal Operation Data Flow

```mermaid
sequenceDiagram
    participant Main
    participant SHT3X
    participant DataMgr
    participant WiFi
    participant SD
    participant JSON
    participant UART
    participant ESP32

    Main->>SHT3X: Fetch Temperature & Humidity
    SHT3X-->>Main: Return sensor data

    Main->>DataMgr: Update with sensor data
    DataMgr->>DataMgr: Set data_ready flag

    Main->>DataMgr: Check if data ready
    DataMgr-->>Main: true

    Main->>DataMgr: Print data
    DataMgr->>WiFi: Check MQTT state

    alt MQTT Connected
        WiFi-->>DataMgr: CONNECTED
        DataMgr->>JSON: Format sensor data
        JSON->>UART: Send JSON string
        UART->>ESP32: Transmit via UART

        DataMgr->>SD: Check buffered count
        alt Has buffered data
            SD-->>DataMgr: count > 0
            DataMgr->>SD: Read record
            SD-->>DataMgr: Return record
            DataMgr->>JSON: Format buffered data
            JSON->>UART: Send JSON string
            UART->>ESP32: Transmit via UART
            DataMgr->>SD: Remove record
        end
    else MQTT Disconnected
        WiFi-->>DataMgr: DISCONNECTED
        DataMgr->>SD: Write data to buffer
        SD->>SD: Store in circular buffer
    end

    DataMgr->>DataMgr: Clear data_ready flag
```

### Command Processing Data Flow

```mermaid
sequenceDiagram
    participant ESP32
    participant UART
    participant RingBuf
    participant Main
    participant CmdExec
    participant CmdParser
    participant Module

    ESP32->>UART: Send command via UART
    UART->>UART: Interrupt triggered
    UART->>RingBuf: Write byte to buffer

    Main->>UART: UART_Handle()
    UART->>RingBuf: Read available bytes
    UART->>UART: Assemble complete line

    alt Line complete (ends with \n)
        UART->>CmdExec: COMMAND_EXECUTE(line)
        CmdExec->>CmdExec: Tokenize command string
        CmdExec->>CmdExec: Find command in table

        alt Command found
            CmdExec->>CmdParser: Call handler function
            CmdParser->>Module: Execute command
            Module-->>CmdParser: Return result
            CmdParser->>UART: Send response
        else Command not found
            CmdExec->>UART: Send error message
        end
    end
```

## Layered Architecture

```mermaid
graph TB
    subgraph "Layer 5: Application Logic"
        L5A[Main Control Loop]
        L5B[State Machine]
        L5C[Error Handling]
    end

    subgraph "Layer 4: Service/Manager Layer"
        L4A[Data Manager]
        L4B[Command Processor]
        L4C[WiFi/MQTT Manager]
    end

    subgraph "Layer 3: Driver Layer"
        L3A[SHT3X Driver]
        L3B[DS3231 Driver]
        L3C[SD Card Manager]
        L3D[Display Driver]
    end

    subgraph "Layer 2: Communication Layer"
        L2A[UART Module]
        L2B[Ring Buffer]
        L2C[JSON Formatter]
        L2D[Print CLI]
    end

    subgraph "Layer 1: Hardware Abstraction Layer"
        L1A[HAL I2C]
        L1B[HAL SPI]
        L1C[HAL UART]
        L1D[HAL GPIO]
    end

    subgraph "Layer 0: Hardware"
        L0A[STM32F103C8T6]
        L0B[SHT3X Sensor]
        L0C[DS3231 RTC]
        L0D[SD Card]
        L0E[ILI9225 LCD]
        L0F[ESP32 Module]
    end

    %% Layer connections
    L5A --> L4A
    L5A --> L4B
    L5A --> L4C

    L4A --> L3A
    L4A --> L3B
    L4A --> L3C
    L4B --> L2A
    L4C --> L2A

    L3A --> L1A
    L3B --> L1A
    L3C --> L1B
    L3D --> L1B

    L2A --> L2B
    L2A --> L1C
    L2C --> L2D
    L2D --> L1C

    L1A --> L0A
    L1B --> L0A
    L1C --> L0A
    L1D --> L0A

    L0A --> L0B
    L0A --> L0C
    L0A --> L0D
    L0A --> L0E
    L0A --> L0F

    style L5A fill:#90EE90
    style L4A fill:#FFD700
    style L3C fill:#FF6B6B
    style L3D fill:#DDA0DD
```

## State Machine Architecture

```mermaid
stateDiagram-v2
    [*] --> System_Init

    System_Init --> Idle : Initialization Complete

    state Idle {
        [*] --> Waiting
        Waiting --> Processing_Command : Command Received
        Processing_Command --> Waiting : Command Complete
    }

    Idle --> Single_Measurement : SINGLE Command
    Single_Measurement --> Idle : Measurement Complete

    Idle --> Periodic_Mode : PERIODIC ON Command

    state Periodic_Mode {
        [*] --> Waiting_Interval
        Waiting_Interval --> Taking_Measurement : Timer Expired
        Taking_Measurement --> Processing_Data : Data Ready
        Processing_Data --> Waiting_Interval : Data Sent/Buffered
    }

    Periodic_Mode --> Idle : PERIODIC OFF Command

    state Processing_Data {
        [*] --> Check_MQTT
        Check_MQTT --> Send_Direct : MQTT Connected
        Check_MQTT --> Buffer_SD : MQTT Disconnected
        Send_Direct --> Send_Buffered : Check Buffer
        Send_Buffered --> [*] : Buffer Empty
        Buffer_SD --> [*] : Write Complete
    }

    note right of Processing_Data
        Data Manager handles routing
        based on MQTT connection state
    end note

    state "Error State" as Error
    Idle --> Error : Critical Error
    Periodic_Mode --> Error : Critical Error
    Error --> System_Init : Recovery/Reset
```

## Memory Architecture

```mermaid
graph TB
    subgraph "Flash Memory (64KB)"
        CODE[Program Code]
        CONST[Constants & Strings]
        CMDTABLE[Command Table]
    end

    subgraph "RAM (20KB)"
        subgraph "Stack"
            STACK[Function Call Stack]
        end

        subgraph "Heap/Global"
            GLOBALS[Global Variables]
            BUFFERS[Buffers]
        end

        subgraph "Key Data Structures"
            UART_BUF[UART Ring Buffer<br/>256 bytes]
            LINE_BUF[Line Buffer<br/>128 bytes]
            PRINT_BUF[Print Buffer<br/>256 bytes]
            JSON_BUF[JSON Buffer<br/>128 bytes]
            SHT3X_STATE[SHT3X State]
            DS3231_STATE[DS3231 State]
            DATA_STATE[Data Manager State]
            MQTT_STATE[MQTT State]
        end
    end

    subgraph "External SD Card"
        METADATA[Buffer Metadata<br/>512 bytes]
        DATABUF[Circular Data Buffer<br/>204,800 records<br/>512 bytes each]
    end

    CODE --> STACK
    CONST --> GLOBALS
    CMDTABLE --> GLOBALS

    GLOBALS --> UART_BUF
    GLOBALS --> LINE_BUF
    GLOBALS --> PRINT_BUF
    GLOBALS --> JSON_BUF
    GLOBALS --> SHT3X_STATE
    GLOBALS --> DS3231_STATE
    GLOBALS --> DATA_STATE
    GLOBALS --> MQTT_STATE

    DATA_STATE -.->|writes when offline| DATABUF
    DATA_STATE -.->|reads when online| DATABUF
    DATABUF -.->|persists| METADATA

    style CODE fill:#E6E6FA
    style CONST fill:#E6E6FA
    style STACK fill:#FFE4B5
    style GLOBALS fill:#FFD700
    style DATABUF fill:#FF6B6B
    style METADATA fill:#FF6B6B
```

## Interrupt and Timing Architecture

```mermaid
graph TB
    subgraph "Interrupt Handlers"
        UART_IRQ[UART RX Interrupt]
        SYSTICK[SysTick Interrupt<br/>1ms]
    end

    subgraph "Main Loop"
        MAIN[Main Loop]
        UART_PROC[UART Processing]
        SENSOR_PROC[Sensor Processing]
        DATA_PROC[Data Processing]
        DISPLAY_PROC[Display Update]
    end

    subgraph "Timing Control"
        HAL_TICK[HAL_GetTick()]
        NEXT_FETCH[next_fetch_ms]
        LAST_FETCH[last_fetch_ms]
        PERIODIC_INT[periodic_interval_ms]
    end

    SYSTICK -->|Increment| HAL_TICK

    UART_IRQ -->|Store byte| UART_BUF[Ring Buffer]

    MAIN --> UART_PROC
    UART_PROC -->|Read| UART_BUF

    MAIN --> SENSOR_PROC
    SENSOR_PROC -->|Check time| HAL_TICK
    SENSOR_PROC -->|Compare| NEXT_FETCH
    SENSOR_PROC -->|Update| LAST_FETCH
    SENSOR_PROC -->|Use interval| PERIODIC_INT

    MAIN --> DATA_PROC
    DATA_PROC -->|Check flag| DATA_READY[data_ready]

    MAIN --> DISPLAY_PROC
    DISPLAY_PROC -->|Check flag| FORCE_UPDATE[force_display_update]

    style UART_IRQ fill:#FF6B6B
    style SYSTICK fill:#FF6B6B
    style MAIN fill:#90EE90
```

## Communication Protocol Architecture

```mermaid
graph LR
    subgraph "STM32 Firmware"
        STM32[STM32F103C8T6]
        UART_TX[UART TX]
        UART_RX[UART RX]
        JSON_ENC[JSON Encoder]
        CMD_DEC[Command Decoder]
    end

    subgraph "ESP32 WiFi Module"
        ESP32_DEV[ESP32]
        ESP_RX[UART RX]
        ESP_TX[UART TX]
        MQTT_CLIENT[MQTT Client]
        PARSER[Command Parser]
    end

    subgraph "Cloud/Server"
        MQTT_BROKER[MQTT Broker]
        BACKEND[Backend Server]
    end

    %% STM32 to ESP32
    JSON_ENC -->|JSON String| UART_TX
    UART_TX -->|115200 baud| ESP_RX
    ESP_RX --> MQTT_CLIENT
    MQTT_CLIENT -->|Publish| MQTT_BROKER

    %% ESP32 to STM32
    MQTT_BROKER -->|Subscribe| MQTT_CLIENT
    MQTT_CLIENT --> PARSER
    PARSER -->|Command String| ESP_TX
    ESP_TX -->|115200 baud| UART_RX
    UART_RX --> CMD_DEC

    %% Backend
    MQTT_BROKER <-->|TCP/IP| BACKEND

    style STM32 fill:#87CEEB
    style ESP32_DEV fill:#90EE90
    style MQTT_BROKER fill:#FFD700
```

## Data Storage Architecture

```mermaid
graph TB
    subgraph "SD Card Physical Layout"
        BLOCK0[Block 0: Metadata<br/>512 bytes]
        BLOCK1[Block 1: Record 0<br/>512 bytes]
        BLOCK2[Block 2: Record 1<br/>512 bytes]
        BLOCKN[Block N: Record N-1<br/>512 bytes]
        BLOCK_LAST[Block 204800: Record 204799<br/>512 bytes]
    end

    subgraph "Metadata Structure"
        META_WRITE[write_index: uint32_t]
        META_READ[read_index: uint32_t]
        META_COUNT[count: uint32_t]
        META_SEQ[sequence_num: uint32_t]
    end

    subgraph "Data Record Structure"
        REC_TS[timestamp: uint32_t<br/>4 bytes]
        REC_TEMP[temperature: float<br/>4 bytes]
        REC_HUM[humidity: float<br/>4 bytes]
        REC_MODE[mode: char[16]<br/>16 bytes]
        REC_SEQ[sequence_num: uint32_t<br/>4 bytes]
        REC_PAD[padding: uint8_t[480]<br/>480 bytes]
    end

    BLOCK0 --> META_WRITE
    BLOCK0 --> META_READ
    BLOCK0 --> META_COUNT
    BLOCK0 --> META_SEQ

    BLOCK1 --> REC_TS
    BLOCK1 --> REC_TEMP
    BLOCK1 --> REC_HUM
    BLOCK1 --> REC_MODE
    BLOCK1 --> REC_SEQ
    BLOCK1 --> REC_PAD

    BLOCK1 -.->|Circular| BLOCK2
    BLOCK2 -.->|Circular| BLOCKN
    BLOCKN -.->|Circular| BLOCK_LAST
    BLOCK_LAST -.->|Wrap around| BLOCK1

    style BLOCK0 fill:#FFD700
    style BLOCK1 fill:#FF6B6B
    style BLOCK2 fill:#FF6B6B
    style BLOCKN fill:#FF6B6B
    style BLOCK_LAST fill:#FF6B6B
```

## System Integration Architecture

```mermaid
graph TB
    subgraph "Complete System"
        subgraph "STM32 Subsystem"
            STM32[STM32F103C8T6<br/>Main Controller]
            SHT3X_HW[SHT3X<br/>Temp & Humidity]
            DS3231_HW[DS3231<br/>RTC]
            SD_HW[SD Card<br/>Offline Storage]
            LCD_HW[ILI9225<br/>LCD Display]
        end

        subgraph "ESP32 Subsystem"
            ESP32[ESP32<br/>WiFi Module]
            WiFi[WiFi Network]
        end

        subgraph "Cloud Infrastructure"
            MQTT[MQTT Broker]
            DB[(Database)]
            API[REST API]
            WEB[Web Dashboard]
        end
    end

    %% Hardware connections
    STM32 <-->|I2C| SHT3X_HW
    STM32 <-->|I2C| DS3231_HW
    STM32 <-->|SPI| SD_HW
    STM32 <-->|SPI| LCD_HW
    STM32 <-->|UART| ESP32

    %% Network connections
    ESP32 <-->|TCP/IP| WiFi
    WiFi <-->|Internet| MQTT

    %% Cloud connections
    MQTT --> DB
    DB <--> API
    API <--> WEB

    %% Data flow annotations
    STM32 -.->|JSON| ESP32
    ESP32 -.->|Commands| STM32
    ESP32 -.->|Publish| MQTT
    MQTT -.->|Subscribe| ESP32

    style STM32 fill:#87CEEB
    style ESP32 fill:#90EE90
    style SD_HW fill:#FF6B6B
    style LCD_HW fill:#DDA0DD
    style MQTT fill:#FFD700
    style WEB fill:#FFB6C1
```

---

## Architecture Principles

### 1. Separation of Concerns

- **Hardware Abstraction**: HAL layer isolates hardware-specific code
- **Driver Layer**: Encapsulates device-specific protocols
- **Service Layer**: Manages application logic and data flow
- **Application Layer**: Coordinates overall system behavior

### 2. Data Flow Patterns

#### Online Mode (MQTT Connected):

```
Sensor → DataManager → JSON Formatter → UART → ESP32 → MQTT Broker
```

#### Offline Mode (MQTT Disconnected):

```
Sensor → DataManager → SD Card Manager → SD Card (Circular Buffer)
```

#### Recovery Mode (MQTT Reconnected):

```
SD Card → SD Card Manager → DataManager → JSON Formatter → UART → ESP32
```

### 3. Key Design Decisions

- **Ring Buffer for UART**: Handles asynchronous command reception without blocking
- **Circular Buffer on SD**: Provides 204,800 record capacity with automatic wraparound
- **State-based MQTT Handling**: Routes data based on connection state
- **Interrupt-driven Communication**: UART RX uses interrupts, main loop polls
- **Persistent Metadata**: SD card metadata survives power cycles

### 4. Resource Constraints

- **Flash**: 64KB (program code, constants, command table)
- **RAM**: 20KB (stack, buffers, state variables)
- **SD Card**: ~100MB usable (204,800 × 512 bytes)
- **I2C Speed**: 100kHz (SHT3X, DS3231)
- **SPI Speed**: 18MHz (SD Card, ILI9225)
- **UART Speed**: 115200 baud (ESP32 communication)

### 5. Timing Constraints

- **SHT3X Measurement**: 15ms (high repeatability)
- **Display Update**: ~100ms (full screen redraw)
- **SD Card Write**: <100ms per record
- **Buffered Data Send**: 100ms spacing between records
- **Periodic Interval**: Configurable (default 60000ms)
