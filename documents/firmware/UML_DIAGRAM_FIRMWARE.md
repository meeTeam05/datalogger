# Firmware System - UML Class Diagrams

This document provides the UML class diagrams and component diagrams for the ESP32 and STM32 firmware system.

## System Class Diagram

```mermaid
classDiagram
    %% STM32 Firmware Classes
    class STM32_Main {
        +I2C_HandleTypeDef hi2c1
        +SPI_HandleTypeDef hspi1, hspi2
        +UART_HandleTypeDef huart1
        +sht3x_t g_sht3x
        +ds3231_t g_ds3231
        +mqtt_state_t mqtt_current_state
        +uint32_t periodic_interval_ms
        +main() int
        +SystemClock_Config() void
    }

    class STM32_UART {
        -UART_HandleTypeDef* huart
        -ring_buffer_t rx_buffer
        +UART_Init() void
        +UART_Handle() void
        +UART_RxCallback(uint8_t) void
    }

    class STM32_CommandParser {
        +COMMAND_EXECUTE(char*) void
        +SINGLE_PARSER() void
        +PERIODIC_ON_PARSER() void
        +PERIODIC_OFF_PARSER() void
        +SET_TIME_PARSER() void
        +MQTT_CONNECTED_PARSER() void
        +MQTT_DISCONNECTED_PARSER() void
        +SD_CLEAR_PARSER() void
    }

    class STM32_DataManager {
        -data_manager_state_t state
        -data_manager_mode_t mode
        +DataManager_Init() void
        +DataManager_UpdateSingle(float, float) void
        +DataManager_UpdatePeriodic(float, float) void
        +DataManager_Print() bool
    }

    class STM32_SDCardManager {
        -sd_card_state_t state
        -uint32_t write_pointer
        -uint32_t read_pointer
        -uint32_t record_count
        +SDCardManager_Init() bool
        +SDCardManager_WriteData(char*) bool
        +SDCardManager_ReadData(char*) bool
        +SDCardManager_RemoveRecord() bool
        +SDCardManager_Clear() bool
    }

    class STM32_SHT3X {
        -I2C_HandleTypeDef* hi2c
        -float temperature
        -float humidity
        -sht3x_mode_t currentState
        +SHT3X_Init() void
        +SHT3X_Single() SHT3X_StatusTypeDef
        +SHT3X_Periodic() SHT3X_StatusTypeDef
        +SHT3X_FetchData() SHT3X_StatusTypeDef
        +SHT3X_PeriodicStop() SHT3X_StatusTypeDef
    }

    class STM32_DS3231 {
        -I2C_HandleTypeDef* hi2c
        +DS3231_Init() void
        +DS3231_Set_Time(struct tm*) DS3231_StatusTypeDef
        +DS3231_Get_Time(struct tm*) DS3231_StatusTypeDef
    }

    class STM32_Display {
        +Display_Init() void
        +Display_Update() void
        +Display_ShowSensorData(float, float) void
        +Display_ShowMQTTState(bool) void
        +Display_ShowBufferCount(uint32_t) void
    }

    %% ESP32 Firmware Classes
    class ESP32_Main {
        +wifi_manager_t g_wifi_manager
        +stm32_uart_t g_stm32_uart
        +mqtt_handler_t g_mqtt_handler
        +relay_control_t g_relay
        +bool g_device_on
        +bool g_periodic_active
        +app_main() void
        +initialize_components() void
        +update_and_publish_state() void
    }

    class ESP32_WiFiManager {
        -wifi_state_t current_state
        -uint8_t retry_count
        +WiFi_Init() bool
        +WiFi_Connect() bool
        +WiFi_GetState() wifi_state_t
        +WiFi_IsConnected() bool
    }

    class ESP32_MQTT_Handler {
        -esp_mqtt_client_handle_t client
        -bool connected
        -int retry_count
        +MQTT_Handler_Init() bool
        +MQTT_Handler_Start() bool
        +MQTT_Handler_Subscribe() bool
        +MQTT_Handler_Publish() bool
    }

    class ESP32_UART {
        -int uart_num
        -ring_buffer_t rx_buffer
        -stm32_data_callback_t callback
        +STM32_UART_Init() bool
        +STM32_UART_SendCommand() bool
        +STM32_UART_ProcessData() void
    }

    class ESP32_RelayControl {
        -int gpio_num
        -bool state
        -relay_state_callback_t callback
        +Relay_Init() bool
        +Relay_SetState(bool) bool
        +Relay_Toggle() bool
    }

    class ESP32_JSONParser {
        -sensor_data_callback_t single_callback
        -sensor_data_callback_t periodic_callback
        +JSON_Parser_Init() bool
        +JSON_Parser_ProcessLine() bool
        +JSON_Parser_IsValid() bool
    }

    class ESP32_ButtonHandler {
        -gpio_num_t gpio_num
        -button_press_callback_t callback
        +Button_Init() bool
        +Button_StartTask() bool
    }

    %% Relationships
    STM32_Main --> STM32_UART : uses
    STM32_Main --> STM32_SHT3X : uses
    STM32_Main --> STM32_DS3231 : uses
    STM32_Main --> STM32_DataManager : uses
    STM32_Main --> STM32_SDCardManager : uses
    STM32_Main --> STM32_Display : uses
    STM32_UART --> STM32_CommandParser : triggers
    STM32_CommandParser --> STM32_SHT3X : controls
    STM32_CommandParser --> STM32_DataManager : updates
    STM32_DataManager --> STM32_SDCardManager : buffers to

    ESP32_Main --> ESP32_WiFiManager : uses
    ESP32_Main --> ESP32_MQTT_Handler : uses
    ESP32_Main --> ESP32_UART : uses
    ESP32_Main --> ESP32_RelayControl : uses
    ESP32_Main --> ESP32_JSONParser : uses
    ESP32_Main --> ESP32_ButtonHandler : uses

    ESP32_UART ..> STM32_UART : UART Communication
    ESP32_RelayControl ..> STM32_Main : Power Control
```

## Memory and Resource Management Class Diagram

```mermaid
classDiagram
    class STM32_Memory {
        +Flash: 64KB
        +RAM: 20KB
        +Stack: 4KB
        +Heap: ~10KB
        +Peripherals: Memory-mapped
    }

    class STM32_Resources {
        +I2C1: 100kHz
        +SPI1: 18MHz SD Card
        +SPI2: 36MHz Display
        +UART1: 115200 baud
        +Timers: SysTick, TIM2
        +RTC: DS3231 external
    }

    class ESP32_Memory {
        +Flash: 4MB
        +SRAM: 520KB
        +PSRAM: None
        +RTC_Memory: 8KB
        +Stack_per_task: 4KB
    }

    class ESP32_Resources {
        +WiFi: 802.11 b/g/n
        +UART1: 115200 baud
        +GPIO: 4x input, 1x output
        +FreeRTOS: Multi-tasking
        +Timers: Hardware timers
    }

    class RingBuffer {
        +Size: 512-1024 bytes
        +Usage: UART RX
        +Type: Circular
        +Thread-safe: ISR access
        +write_ptr: uint32_t
        +read_ptr: uint32_t
        +buffer: uint8_t[]
        +Write(uint8_t) bool
        +Read(uint8_t*) bool
        +Available() uint32_t
        +Clear() void
    }

    class SDCardBuffer {
        +Capacity: 204,800 records
        +Record_size: ~80 bytes
        +Total: ~16MB
        +Type: Circular FAT32
        +Persistence: Non-volatile
        +write_pointer: uint32_t
        +read_pointer: uint32_t
        +record_count: uint32_t
        +WriteRecord(char*) bool
        +ReadRecord(char*) bool
        +RemoveRecord() bool
        +Clear() bool
        +IsFull() bool
    }

    STM32_Memory --> RingBuffer : allocates
    STM32_Memory --> SDCardBuffer : manages
    ESP32_Memory --> RingBuffer : allocates

    STM32_Resources ..> STM32_Memory : constrained by
    ESP32_Resources ..> ESP32_Memory : constrained by
```

## State Machine - System Lifecycle

```mermaid
stateDiagram-v2
    [*] --> PowerOn

    state PowerOn {
        [*] --> STM32_Init
        [*] --> ESP32_Init

        STM32_Init --> STM32_Idle : Init complete<br/>500ms
        ESP32_Init --> ESP32_Connecting : Init complete<br/>1000ms

        state STM32_Idle {
            [*] --> WaitingCommand
            WaitingCommand --> SingleMeasure : SINGLE command
            WaitingCommand --> PeriodicMeasure : PERIODIC ON
            SingleMeasure --> WaitingCommand : Done (~20ms)
            PeriodicMeasure --> WaitingCommand : PERIODIC OFF

            note right of PeriodicMeasure
                Continuous measurement
                Interval: 5s-60s
                Auto-buffer if offline
            end note
        }

        state ESP32_Connecting {
            [*] --> WiFi_Disconnected
            WiFi_Disconnected --> WiFi_Connecting : Connect attempt
            WiFi_Connecting --> WiFi_Connected : Success
            WiFi_Connecting --> WiFi_Failed : Max 5 retries<br/>@ 2s interval
            WiFi_Failed --> WiFi_Connecting : Manual retry<br/>@ 5s interval
            WiFi_Connected --> MQTT_Disconnected : 4s stable wait

            MQTT_Disconnected --> MQTT_Connecting : Start connection
            MQTT_Connecting --> MQTT_Connected : Success
            MQTT_Connecting --> MQTT_Disconnected : Fail + backoff<br/>60s, 120s, 240s...
            MQTT_Connected --> MQTT_Disconnected : WiFi lost

            note right of MQTT_Connecting
                Exponential backoff
                Min 60s, 2^retry
                Infinite retries
            end note
        }
    }

    state SystemOperational {
        state STM32_Operation {
            [*] --> DataCollection
            DataCollection --> DataReady : Measurement complete
            DataReady --> CheckMQTT : data_ready flag set
            CheckMQTT --> LiveTransmit : MQTT connected
            CheckMQTT --> BufferToSD : MQTT disconnected
            LiveTransmit --> DataCollection : Via UART
            BufferToSD --> DataCollection : Store locally

            note right of BufferToSD
                Max 204,800 records
                ~16MB capacity
                Circular overwrite
            end note
        }

        state ESP32_Operation {
            [*] --> MonitorState
            MonitorState --> ProcessCommand : MQTT message
            MonitorState --> ProcessData : UART data
            MonitorState --> ProcessButton : Button press
            ProcessCommand --> UpdateState : Parse & Execute
            ProcessData --> PublishMQTT : Validate & Publish
            ProcessButton --> UpdateState : Toggle or Control
            UpdateState --> MonitorState
            PublishMQTT --> MonitorState

            note right of MonitorState
                Multi-threaded
                FreeRTOS tasks
                Event-driven
            end note
        }
    }

    PowerOn --> SystemOperational : Both ready
    SystemOperational --> ErrorRecovery : Error detected
    ErrorRecovery --> SystemOperational : Recovered
    ErrorRecovery --> PowerOn : Critical error<br/>Reset required
```

## Deployment Diagram

```mermaid
graph TB
    subgraph Device["IoT Device - Hardware"]
        subgraph STM32["STM32F103C8T6 - ARM Cortex-M3"]
            STM32_FW[STM32 Firmware<br/>C, STM32 HAL<br/>Data collection & buffering]
        end

        subgraph ESP32["ESP32-WROOM-32 - Xtensa LX6"]
            ESP32_FW[ESP32 Firmware<br/>C, ESP-IDF<br/>IoT gateway & MQTT]
        end

        subgraph Sensors["Sensors & Peripherals - I2C/SPI"]
            SHT3X[SHT3X Sensor<br/>I2C 0x44<br/>Temp & Humidity]
            RTC[DS3231 RTC<br/>I2C 0x68<br/>Real-time clock]
            SD[SD Card<br/>SPI 18MHz<br/>Data buffering]
            Display[ILI9225 TFT<br/>SPI 36MHz<br/>176x220 display]
        end

        subgraph Actuators["Actuators & Inputs - GPIO"]
            Relay[Relay Module<br/>GPIO4<br/>Power control]
            Buttons[4x Buttons<br/>GPIO 5,16,17,4<br/>User input]
        end
    end

    subgraph Network["Local Network - WiFi 2.4GHz"]
        subgraph Broker["MQTT Broker - Mosquitto"]
            MQTT[MQTT Server<br/>v5.0 Port 1883<br/>Message broker]
        end

        subgraph Clients["Client Devices"]
            Web[Web Dashboard<br/>Browser<br/>Monitoring UI]
            Mobile[Mobile App<br/>MQTT Client<br/>Remote control]
        end
    end

    STM32_FW <-->|UART 115200<br/>JSON Protocol| ESP32_FW
    STM32_FW <-->|I2C 100kHz| SHT3X
    STM32_FW <-->|I2C 100kHz| RTC
    STM32_FW <-->|SPI 18MHz| SD
    STM32_FW -->|SPI 36MHz| Display

    ESP32_FW <-->|MQTT over TCP| MQTT
    ESP32_FW -->|GPIO Control| Relay
    Buttons -->|GPIO Interrupt| ESP32_FW

    Web <-->|MQTT WebSocket| MQTT
    Mobile <-->|MQTT over TCP| MQTT

    Relay -.->|Power Control| STM32_FW

    style Device fill:#FFE4B5
    style STM32 fill:#FFB6C1
    style ESP32 fill:#B0E0E6
    style Sensors fill:#F0E68C
    style Actuators fill:#DDA0DD
    style Network fill:#98FB98
    style Broker fill:#87CEEB
    style Clients fill:#FFDAB9
```

## Class Details and Specifications

### STM32 Key Classes

**STM32_DataManager**

- Manages sensor data collection modes (SINGLE/PERIODIC)
- Coordinates between sensor reading and data output
- Handles buffering logic based on MQTT connection state

**STM32_SDCardManager**

- Implements circular buffer on SD card using FAT32
- Capacity: 204,800 records (~16MB)
- Pointer-based read/write management
- Persistence across power cycles

**STM32_CommandParser**

- Parses JSON commands from ESP32
- Routes commands to appropriate handlers
- Validates command format and parameters

### ESP32 Key Classes

**ESP32_MQTT_Handler**

- Manages MQTT connection with exponential backoff
- Publishes sensor data to appropriate topics
- Subscribes to command topics
- QoS management (0 for data, 1 for commands)

**ESP32_WiFiManager**

- Handles WiFi connection and reconnection
- Auto-retry with configurable intervals
- State tracking and reporting

**ESP32_JSONParser**

- Validates and parses JSON data from STM32
- Extracts mode, timestamp, temperature, humidity
- Callbacks for single/periodic data

### Memory Classes

**RingBuffer**

- Thread-safe circular buffer for UART RX
- Used by both STM32 and ESP32
- ISR-safe implementation
- Typical size: 512-1024 bytes

**SDCardBuffer**

- Large persistent circular buffer
- Non-volatile storage for offline operation
- Manages 204,800 records with metadata
- Automatic wraparound when full
