# Complete Firmware System - UML and Architecture Diagrams

This document provides the UML class diagrams and architecture views of the complete integrated firmware system.

## Complete System Architecture

```mermaid
graph TB
    subgraph External[External Systems]
        User[User/Web Interface]
        Power[Power Supply]
        Network[WiFi Network]
        Broker[MQTT Broker<br/>192.168.1.39:1883]
    end
    
    subgraph STM32[STM32F103C8T6 Microcontroller]
        direction TB
        STM32_Main[Main Application]
        STM32_UART[UART Handler]
        STM32_I2C[I2C Handler]
        STM32_SPI[SPI Handler]
        STM32_CMD[Command Parser]
        STM32_DM[Data Manager]
        STM32_SD[SD Card Manager]
        STM32_Display[Display Manager]
        
        STM32_Main --> STM32_UART
        STM32_Main --> STM32_I2C
        STM32_Main --> STM32_SPI
        STM32_UART --> STM32_CMD
        STM32_CMD --> STM32_DM
        STM32_DM --> STM32_SD
        STM32_Main --> STM32_Display
    end
    
    subgraph ESP32[ESP32-WROOM-32 Module]
        direction TB
        ESP32_Main[Main Application]
        ESP32_WiFi[WiFi Manager]
        ESP32_MQTT[MQTT Handler]
        ESP32_UART2[UART Handler]
        ESP32_Relay[Relay Control]
        ESP32_JSON[JSON Parser]
        ESP32_Button[Button Handler]
        
        ESP32_Main --> ESP32_WiFi
        ESP32_Main --> ESP32_MQTT
        ESP32_Main --> ESP32_UART2
        ESP32_Main --> ESP32_Relay
        ESP32_Main --> ESP32_JSON
        ESP32_Main --> ESP32_Button
    end
    
    subgraph Sensors[Sensors and Peripherals]
        SHT3X[SHT3X Temp/Humidity<br/>0x44 I2C]
        RTC[DS3231 RTC<br/>0x68 I2C]
        SD[SD Card<br/>SPI1 18MHz]
        TFT[ILI9225 Display<br/>SPI2 36MHz]
        RelayHW[Relay Module<br/>GPIO4]
        Buttons[4x Buttons<br/>GPIO 5,16,17,4]
    end
    
    Power --> STM32
    Power --> ESP32
    
    STM32_I2C <--> SHT3X
    STM32_I2C <--> RTC
    STM32_SPI <--> SD
    STM32_SPI <--> TFT
    
    STM32_UART <-->|UART1 115200 baud<br/>JSON Protocol| ESP32_UART2
    
    ESP32_WiFi <--> Network
    Network <--> Broker
    ESP32_MQTT <--> Broker
    Broker <--> User
    
    ESP32_Relay --> RelayHW
    Buttons --> ESP32_Button
    RelayHW -.->|Controls Power| STM32
    
    User -.->|HTTP/WebSocket| Broker
    
    style STM32 fill:#FFE4B5
    style ESP32 fill:#B0E0E6
    style External fill:#E0E0E0
    style Sensors fill:#F0E68C
```

## Complete System Class Diagram

```mermaid
classDiagram
    %% STM32 Classes
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
    
    %% ESP32 Classes
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
    
    ESP32_UART -.->|UART Communication| STM32_UART : bidirectional
    ESP32_RelayControl -.->|Power Control| STM32_Main : controls
```

## Data Flow Architecture

```mermaid
graph LR
    subgraph Input[Data Input Sources]
        Sensor[SHT3X Sensor<br/>Temperature & Humidity]
        Time[DS3231 RTC<br/>Timestamp]
        WebCmd[Web Commands<br/>via MQTT]
        ButtonCmd[Button Presses<br/>GPIO]
    end
    
    subgraph STM32_Processing[STM32 Processing]
        I2C[I2C Read<br/>100kHz]
        Parse[JSON Format<br/>with Timestamp]
        Route{MQTT<br/>Connected?}
        Buffer[SD Card Buffer<br/>204,800 records]
        
        Sensor --> I2C
        Time --> I2C
        I2C --> Parse
        Parse --> Route
        Route -->|No| Buffer
    end
    
    subgraph ESP32_Processing[ESP32 Processing]
        UARTRx[UART RX<br/>Ring Buffer]
        JSONParse[JSON Parser<br/>Mode Detection]
        MQTTPub[MQTT Publish<br/>QoS 0/1]
        CmdRoute[Command Router<br/>Topic-based]
        StateSync[State Manager<br/>Retained Messages]
        
        Route -->|Yes| UARTRx
        UARTRx --> JSONParse
        JSONParse --> MQTTPub
        
        WebCmd --> CmdRoute
        ButtonCmd --> CmdRoute
        CmdRoute --> StateSync
    end
    
    subgraph Output[Data Output]
        Cloud[MQTT Broker<br/>Cloud]
        Display[TFT Display<br/>ILI9225]
        LED[Status LEDs<br/>WiFi/MQTT]
        RelayOut[Relay Output<br/>Device Control]
        
        MQTTPub --> Cloud
        Parse --> Display
        StateSync --> Display
        StateSync --> LED
        CmdRoute --> RelayOut
    end
    
    Buffer -.->|On Reconnect| UARTRx
    
    style Input fill:#90EE90
    style STM32_Processing fill:#FFE4B5
    style ESP32_Processing fill:#B0E0E6
    style Output fill:#DDA0DD
```

## Communication Protocol Architecture

```mermaid
sequenceDiagram
    participant STM32
    participant UART as UART Link<br/>115200 baud
    participant ESP32
    participant MQTT as MQTT Protocol<br/>v5.0
    participant Broker as MQTT Broker
    
    Note over STM32,Broker: Command Flow (Web → STM32)
    
    Broker->>MQTT: PUBLISH datalogger/stm32/command
    MQTT->>ESP32: MQTT message
    ESP32->>ESP32: Route by topic
    ESP32->>UART: Text command + '\n'
    UART->>STM32: UART RX interrupt
    STM32->>STM32: Parse and execute
    
    Note over STM32,Broker: Data Flow (STM32 → Web)
    
    STM32->>STM32: Collect sensor data
    STM32->>UART: JSON + '\n'
    UART->>ESP32: UART RX interrupt
    ESP32->>ESP32: JSON parse
    ESP32->>MQTT: PUBLISH with QoS
    MQTT->>Broker: Forward message
    
    Note over STM32,Broker: State Synchronization
    
    ESP32->>ESP32: State change detected
    ESP32->>MQTT: PUBLISH with retain=1
    MQTT->>Broker: Store retained
    Broker->>Broker: Persist state
    
    Note over STM32,Broker: MQTT Status Notification
    
    ESP32->>ESP32: MQTT state change
    ESP32->>UART: "MQTT CONNECTED" or<br/>"MQTT DISCONNECTED"
    UART->>STM32: Command
    STM32->>STM32: Switch TX mode
```

## State Machine Architecture

```mermaid
stateDiagram-v2
    [*] --> PowerOn
    
    state PowerOn {
        [*] --> STM32_Init
        [*] --> ESP32_Init
        
        STM32_Init --> STM32_Idle : Init complete
        ESP32_Init --> ESP32_Connecting : Init complete
        
        state STM32_Idle {
            [*] --> WaitingCommand
            WaitingCommand --> SingleMeasure : SINGLE
            WaitingCommand --> PeriodicMeasure : PERIODIC ON
            SingleMeasure --> WaitingCommand : Done
            PeriodicMeasure --> WaitingCommand : PERIODIC OFF
        }
        
        state ESP32_Connecting {
            [*] --> WiFi_Disconnected
            WiFi_Disconnected --> WiFi_Connecting : Connect attempt
            WiFi_Connecting --> WiFi_Connected : Success
            WiFi_Connecting --> WiFi_Failed : Max retries
            WiFi_Failed --> WiFi_Connecting : Retry 5s
            WiFi_Connected --> MQTT_Disconnected : 4s stable
            
            MQTT_Disconnected --> MQTT_Connecting : Start
            MQTT_Connecting --> MQTT_Connected : Success
            MQTT_Connecting --> MQTT_Disconnected : Fail + backoff
            MQTT_Connected --> MQTT_Disconnected : WiFi lost
        }
    }
    
    state SystemOperational {
        state STM32_Operation {
            [*] --> DataCollection
            DataCollection --> DataReady : Measurement complete
            DataReady --> CheckMQTT : data_ready flag
            CheckMQTT --> LiveTransmit : MQTT connected
            CheckMQTT --> BufferToSD : MQTT disconnected
            LiveTransmit --> DataCollection
            BufferToSD --> DataCollection
        }
        
        state ESP32_Operation {
            [*] --> MonitorState
            MonitorState --> ProcessCommand : MQTT message
            MonitorState --> ProcessData : UART data
            MonitorState --> ProcessButton : Button press
            ProcessCommand --> UpdateState
            ProcessData --> PublishMQTT
            ProcessButton --> UpdateState
            UpdateState --> MonitorState
            PublishMQTT --> MonitorState
        }
    }
    
    PowerOn --> SystemOperational : Both ready
    SystemOperational --> ErrorRecovery : Error detected
    ErrorRecovery --> SystemOperational : Recovered
```

## Hardware Configuration Diagram

```mermaid
graph TB
    subgraph STM32_HW[STM32F103C8T6 Hardware]
        STM32_MCU[ARM Cortex-M3<br/>72MHz<br/>64KB Flash<br/>20KB RAM]
        
        STM32_I2C[I2C1<br/>PB6: SCL<br/>PB7: SDA<br/>100kHz]
        
        STM32_SPI1[SPI1 SD Card<br/>PA5: SCK<br/>PA6: MISO<br/>PA7: MOSI<br/>18MHz]
        
        STM32_SPI2[SPI2 Display<br/>PB13: SCK<br/>PB14: MISO<br/>PB15: MOSI<br/>36MHz]
        
        STM32_UART[UART1<br/>PA9: TX<br/>PA10: RX<br/>115200 baud]
        
        STM32_MCU --> STM32_I2C
        STM32_MCU --> STM32_SPI1
        STM32_MCU --> STM32_SPI2
        STM32_MCU --> STM32_UART
    end
    
    subgraph ESP32_HW[ESP32-WROOM-32 Hardware]
        ESP32_MCU[Xtensa LX6<br/>240MHz<br/>4MB Flash<br/>520KB RAM]
        
        ESP32_WiFi[WiFi Radio<br/>802.11 b/g/n<br/>2.4GHz]
        
        ESP32_UART_HW[UART1<br/>GPIO17: TX<br/>GPIO16: RX<br/>115200 baud]
        
        ESP32_GPIO[GPIO<br/>GPIO4: Relay<br/>GPIO5: Button1<br/>GPIO16: Button2<br/>GPIO17: Button3<br/>GPIO4: Button4]
        
        ESP32_MCU --> ESP32_WiFi
        ESP32_MCU --> ESP32_UART_HW
        ESP32_MCU --> ESP32_GPIO
    end
    
    subgraph Peripherals_HW[Peripherals]
        SHT3X_HW[SHT3X Sensor<br/>I2C Address: 0x44<br/>±0.2°C, ±2%RH]
        RTC_HW[DS3231 RTC<br/>I2C Address: 0x68<br/>±2ppm accuracy]
        SD_HW[SD Card<br/>FAT32<br/>Circular Buffer]
        Display_HW[ILI9225 TFT<br/>176x220 pixels<br/>2.2 inch]
        Relay_HW[Relay Module<br/>Active HIGH<br/>Controls STM32 power]
        Buttons_HW[4x Tactile Buttons<br/>Active LOW<br/>Pull-up enabled]
    end
    
    STM32_I2C <--> SHT3X_HW
    STM32_I2C <--> RTC_HW
    STM32_SPI1 <--> SD_HW
    STM32_SPI2 <--> Display_HW
    
    STM32_UART <--> ESP32_UART_HW
    
    ESP32_GPIO --> Relay_HW
    Buttons_HW --> ESP32_GPIO
    Relay_HW -.->|Power Control| STM32_MCU
    
    ESP32_WiFi <--> Network[WiFi Network<br/>SSID: Redmi Note 9 Pro]
    
    style STM32_HW fill:#FFE4B5
    style ESP32_HW fill:#B0E0E6
    style Peripherals_HW fill:#F0E68C
```

## MQTT Topic and Message Architecture

```mermaid
graph TB
    subgraph Broker[MQTT Broker: 192.168.1.39:1883]
        direction TB
        
        subgraph Subscribe[ESP32 Subscribes]
            T1[datalogger/stm32/command<br/>QoS: 1]
            T2[datalogger/esp32/relay/control<br/>QoS: 1]
            T3[datalogger/esp32/system/state<br/>QoS: 1]
        end
        
        subgraph Publish[ESP32 Publishes]
            T4[datalogger/stm32/single/data<br/>QoS: 0, Retain: 0]
            T5[datalogger/stm32/periodic/data<br/>QoS: 0, Retain: 0]
            T6[datalogger/esp32/system/state<br/>QoS: 1, Retain: 1]
        end
    end
    
    subgraph Messages[Message Formats]
        direction TB
        
        CMD1[Commands:<br/>SINGLE<br/>PERIODIC ON<br/>PERIODIC OFF<br/>SET TIME ...]
        
        CMD2[Relay Commands:<br/>ON<br/>OFF<br/>TOGGLE]
        
        CMD3[State Request:<br/>REQUEST]
        
        DATA1[Single Data:<br/>{mode:SINGLE,<br/>timestamp:...,<br/>temperature:...,<br/>humidity:...}]
        
        DATA2[Periodic Data:<br/>{mode:PERIODIC,<br/>timestamp:...,<br/>temperature:...,<br/>humidity:...}]
        
        DATA3[System State:<br/>{device:ON/OFF,<br/>periodic:ON/OFF,<br/>timestamp:...}]
    end
    
    T1 -.-> CMD1
    T2 -.-> CMD2
    T3 -.-> CMD3
    T4 -.-> DATA1
    T5 -.-> DATA2
    T6 -.-> DATA3
    
    Web[Web Interface] -->|Publish| T1
    Web -->|Publish| T2
    Web -->|Publish| T3
    
    T4 -->|Subscribe| Web
    T5 -->|Subscribe| Web
    T6 -->|Subscribe| Web
    
    style Broker fill:#87CEEB
    style Messages fill:#FFE4B5
    style Web fill:#90EE90
```

## Memory and Resource Management

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
    }
    
    class SDCardBuffer {
        +Capacity: 204,800 records
        +Record_size: ~80 bytes
        +Total: ~16MB
        +Type: Circular FAT32
        +Persistence: Non-volatile
    }
    
    STM32_Memory --> RingBuffer : allocates
    STM32_Memory --> SDCardBuffer : manages
    ESP32_Memory --> RingBuffer : allocates
```

## Error Handling and Recovery Architecture

```mermaid
graph TB
    subgraph Errors[Error Types]
        E1[I2C Sensor Timeout]
        E2[RTC Communication Fail]
        E3[SD Card Mount Fail]
        E4[WiFi Connection Lost]
        E5[MQTT Broker Disconnect]
        E6[UART Buffer Overflow]
        E7[JSON Parse Error]
    end
    
    subgraph Recovery[Recovery Strategies]
        R1[Return 0.0 values<br/>Continue operation<br/>Retry next cycle]
        
        R2[Use timestamp=0<br/>Continue operation<br/>Retry next query]
        
        R3[Disable SD buffering<br/>MQTT-only mode<br/>Log to display]
        
        R4[Auto-retry 5x @ 2s<br/>Manual retry @ 5s<br/>Continue until success]
        
        R5[Exponential backoff<br/>min 60s, 2^retry<br/>Infinite retries]
        
        R6[Discard oldest data<br/>Log overflow<br/>Continue reception]
        
        R7[Log parse error<br/>Discard message<br/>Continue processing]
    end
    
    subgraph Monitoring[Error Monitoring]
        M1[Display Status]
        M2[LED Indicators]
        M3[MQTT Error Reports]
        M4[Buffered Error Data]
    end
    
    E1 --> R1
    E2 --> R2
    E3 --> R3
    E4 --> R4
    E5 --> R5
    E6 --> R6
    E7 --> R7
    
    R1 --> M1
    R2 --> M1
    R3 --> M1
    R4 --> M2
    R5 --> M2
    R6 --> M1
    R7 --> M1
    
    R1 --> M3
    R3 --> M3
    R4 --> M3
    R5 --> M3
    
    R1 --> M4
    R2 --> M4
    
    style Errors fill:#FF6B6B
    style Recovery fill:#90EE90
    style Monitoring fill:#FFD700
```

## System Timing and Synchronization

```mermaid
gantt
    title System Timing Diagram
    dateFormat X
    axisFormat %L ms
    
    section STM32
    System Init           :0, 500
    SHT3X Single 15ms     :600, 615
    I2C Read              :615, 620
    Format JSON           :620, 625
    UART Transmit 10ms    :625, 635
    
    section ESP32
    System Init           :0, 1000
    WiFi Connect 10s      :1000, 11000
    Wait 4s Stable        :11000, 15000
    MQTT Connect 2s       :15000, 17000
    UART Process          :635, 645
    JSON Parse            :645, 650
    MQTT Publish          :650, 655
    
    section Periodic
    Fetch Interval 5s     :20000, 25000
    Next Fetch            :25000, 30000
    Next Fetch            :30000, 35000
    
    section Delays
    WiFi Retry 2s         :milestone, 2000
    Manual Retry 5s       :milestone, 5000
    MQTT Backoff 1s       :milestone, 1000
    MQTT Backoff 2s       :milestone, 2000
    MQTT Backoff 4s       :milestone, 4000
    STM32 Boot 500ms      :milestone, 500
    Button Debounce 200ms :milestone, 200
```

## Deployment Architecture

```mermaid
C4Deployment
    title Deployment Diagram - IoT Data Logger System
    
    Deployment_Node(device, "IoT Device", "Hardware"){
        Deployment_Node(stm32, "STM32F103C8T6", "Microcontroller"){
            Container(stm32_fw, "STM32 Firmware", "C, HAL", "Data collection and buffering")
        }
        
        Deployment_Node(esp32, "ESP32-WROOM-32", "WiFi Module"){
            Container(esp32_fw, "ESP32 Firmware", "C, ESP-IDF", "IoT bridge and MQTT client")
        }
        
        Deployment_Node(sensors, "Sensors", "Hardware"){
            Container(sht3x, "SHT3X", "I2C", "Temperature & Humidity")
            Container(rtc, "DS3231", "I2C", "Real-time clock")
            Container(sd, "SD Card", "SPI", "Data buffering")
            Container(display, "ILI9225", "SPI", "Status display")
        }
    }
    
    Deployment_Node(network, "Local Network", "WiFi"){
        Deployment_Node(broker, "MQTT Broker", "Mosquitto"){
            Container(mqtt, "MQTT Server", "v5.0", "Message broker")
        }
    }
    
    Deployment_Node(cloud, "Web Interface", "Browser"){
        Container(web, "Dashboard", "HTML/JS", "Monitoring UI")
    }
    
    Rel(stm32_fw, esp32_fw, "UART 115200", "JSON")
    Rel(stm32_fw, sht3x, "I2C", "Read data")
    Rel(stm32_fw, rtc, "I2C", "Get time")
    Rel(stm32_fw, sd, "SPI", "Buffer data")
    Rel(stm32_fw, display, "SPI", "Show status")
    Rel(esp32_fw, mqtt, "MQTT", "Pub/Sub")
    Rel(mqtt, web, "WebSocket", "Real-time")
    Rel(web, mqtt, "HTTP", "Commands")
```

---

## System Characteristics

### Performance Metrics
- **Measurement Rate**: 0.2 Hz to 1 Hz (configurable 5s-60s intervals)
- **Sensor Accuracy**: ±0.2°C, ±2%RH (SHT3X)
- **RTC Accuracy**: ±2ppm (DS3231)
- **Communication Latency**: <100ms (STM32 → Web)
- **Buffer Capacity**: 204,800 records (~16MB SD card)
- **UART Throughput**: ~11.5 KB/s (115200 baud)
- **MQTT Throughput**: Limited by network, typically <1KB/s for sensor data

### Reliability Features
- **Sensor Failure**: Graceful degradation with 0.0 values
- **Communication Loss**: SD card buffering (>14 days @ 5s intervals)
- **Power Interruption**: Non-volatile SD card storage
- **Network Instability**: Automatic WiFi/MQTT reconnection
- **Data Integrity**: CRC validation for sensor data
- **Clock Drift**: RTC with backup battery

### Scalability
- **Single Device**: Current implementation
- **Multi-Device**: MQTT topic hierarchy ready
- **Extended Sensors**: Modular driver architecture
- **Cloud Services**: MQTT broker can forward to cloud
- **Historical Data**: SD card provides local time-series storage

### Power Consumption
- **STM32 Active**: ~20mA @ 72MHz
- **STM32 Sleep**: Not implemented (always active)
- **ESP32 Active**: ~160mA (WiFi on), ~80mA (WiFi off)
- **ESP32 Sleep**: Not implemented (always active)
- **Total System**: ~200mA @ 5V (relay ON)
- **Relay Control**: Enables power-saving by powering down STM32

### Security Considerations
- **MQTT Authentication**: Username/password (admin/password)
- **Network**: WPA2 WiFi encryption
- **Physical**: Relay can disconnect STM32 power
- **Data Privacy**: Local network only (no internet by default)
- **Future**: TLS/SSL for MQTT can be enabled
