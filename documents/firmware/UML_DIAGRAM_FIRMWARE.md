# UML Class Diagrams - Firmware System

This document provides the UML class diagrams and component diagrams for the ESP32 and STM32 firmware system.

## Complete System Class Diagram

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

    ESP32_UART -.->|UART Communication| STM32_UART : bidirectional
    ESP32_RelayControl -.->|Power Control| STM32_Main : controls
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

## Component Diagram - STM32 Modules

```mermaid
graph TB
    subgraph STM32_System[STM32 System Components]
        direction TB

        subgraph HAL[Hardware Abstraction Layer]
            I2C_HAL[I2C HAL Driver]
            SPI_HAL[SPI HAL Driver]
            UART_HAL[UART HAL Driver]
            GPIO_HAL[GPIO HAL Driver]
        end

        subgraph Drivers[Device Drivers]
            SHT3X_Driver[SHT3X Driver<br/>Temperature & Humidity]
            DS3231_Driver[DS3231 Driver<br/>Real-Time Clock]
            SD_Driver[SD Card Driver<br/>FAT32 Filesystem]
            Display_Driver[ILI9225 Driver<br/>TFT Display]
        end

        subgraph Middleware[Middleware Layer]
            UART_Handler[UART Handler<br/>Ring Buffer + Protocol]
            CMD_Parser[Command Parser<br/>JSON Commands]
            Data_Mgr[Data Manager<br/>Mode Control]
            SD_Mgr[SD Card Manager<br/>Circular Buffer]
        end

        subgraph Application[Application Layer]
            Main_App[Main Application<br/>System Control]
        end
    end

    Main_App --> UART_Handler
    Main_App --> Data_Mgr
    Main_App --> SD_Mgr
    Main_App --> Display_Driver

    UART_Handler --> CMD_Parser
    CMD_Parser --> Data_Mgr
    CMD_Parser --> SHT3X_Driver

    Data_Mgr --> SHT3X_Driver
    Data_Mgr --> DS3231_Driver
    Data_Mgr --> SD_Mgr
    Data_Mgr --> UART_Handler

    SD_Mgr --> SD_Driver

    SHT3X_Driver --> I2C_HAL
    DS3231_Driver --> I2C_HAL
    SD_Driver --> SPI_HAL
    Display_Driver --> SPI_HAL
    UART_Handler --> UART_HAL

    style HAL fill:#FFE4B5
    style Drivers fill:#F0E68C
    style Middleware fill:#DDA0DD
    style Application fill:#90EE90
```

## Component Diagram - ESP32 Modules

```mermaid
graph TB
    subgraph ESP32_System[ESP32 System Components]
        direction TB

        subgraph ESP_IDF[ESP-IDF Framework]
            WiFi_Stack[WiFi Stack<br/>802.11 b/g/n]
            TCP_IP[TCP/IP Stack<br/>LwIP]
            MQTT_Client[MQTT Client<br/>esp-mqtt]
            UART_Driver[UART Driver<br/>esp_uart]
            GPIO_Driver[GPIO Driver<br/>esp_gpio]
        end

        subgraph Custom_Drivers[Custom Drivers]
            WiFi_Mgr[WiFi Manager<br/>Connection Management]
            MQTT_Handler[MQTT Handler<br/>Pub/Sub + Reconnect]
            UART_Handler[STM32 UART Handler<br/>Protocol Layer]
            Relay_Ctrl[Relay Control<br/>Power Management]
            Button_Handler[Button Handler<br/>Input Processing]
        end

        subgraph Protocol[Protocol Layer]
            JSON_Parser[JSON Parser<br/>Data Validation]
            State_Mgr[State Manager<br/>System State Sync]
        end

        subgraph Application[Application Layer]
            Main_App[Main Application<br/>Coordination Logic]
        end
    end

    Main_App --> WiFi_Mgr
    Main_App --> MQTT_Handler
    Main_App --> UART_Handler
    Main_App --> Relay_Ctrl
    Main_App --> Button_Handler
    Main_App --> State_Mgr

    WiFi_Mgr --> WiFi_Stack
    MQTT_Handler --> MQTT_Client
    UART_Handler --> UART_Driver
    UART_Handler --> JSON_Parser
    Relay_Ctrl --> GPIO_Driver
    Button_Handler --> GPIO_Driver

    JSON_Parser --> State_Mgr
    State_Mgr --> MQTT_Handler

    MQTT_Client --> TCP_IP
    WiFi_Stack --> TCP_IP

    style ESP_IDF fill:#B0E0E6
    style Custom_Drivers fill:#DDA0DD
    style Protocol fill:#F0E68C
    style Application fill:#90EE90
```

## Deployment Diagram

```mermaid
C4Deployment
    title Firmware Deployment Diagram - ESP32 and STM32 Coordination

    Deployment_Node(device, "IoT Device", "Hardware"){
        Deployment_Node(stm32, "STM32F103C8T6", "ARM Cortex-M3 Microcontroller"){
            Container(stm32_fw, "STM32 Firmware", "C, STM32 HAL", "Data collection, buffering, and local control")
        }

        Deployment_Node(esp32, "ESP32-WROOM-32", "Xtensa LX6 WiFi Module"){
            Container(esp32_fw, "ESP32 Firmware", "C, ESP-IDF", "IoT gateway, MQTT client, and coordination")
        }

        Deployment_Node(sensors, "Sensors & Peripherals", "I2C/SPI Hardware"){
            Container(sht3x, "SHT3X Sensor", "I2C 0x44", "Temperature & Humidity measurement")
            Container(rtc, "DS3231 RTC", "I2C 0x68", "Real-time clock with battery backup")
            Container(sd, "SD Card", "SPI 18MHz", "Non-volatile data buffering")
            Container(display, "ILI9225 TFT", "SPI 36MHz", "Status display 176x220")
        }

        Deployment_Node(actuators, "Actuators & Inputs", "GPIO Hardware"){
            Container(relay, "Relay Module", "GPIO4", "Power control for STM32")
            Container(buttons, "4x Buttons", "GPIO 5,16,17,4", "User input interface")
        }
    }

    Deployment_Node(network, "Local Network", "WiFi 2.4GHz"){
        Deployment_Node(broker, "MQTT Broker", "Mosquitto Server"){
            Container(mqtt, "MQTT Server", "v5.0 Port 1883", "Message broker and persistence")
        }

        Deployment_Node(clients, "Client Devices", "Web/Mobile"){
            Container(web, "Web Dashboard", "Browser", "Monitoring and control interface")
            Container(mobile, "Mobile App", "MQTT Client", "Remote monitoring")
        }
    }

    Rel(stm32_fw, esp32_fw, "UART 115200 baud", "JSON Protocol")
    Rel(stm32_fw, sht3x, "I2C 100kHz", "Read sensor data")
    Rel(stm32_fw, rtc, "I2C 100kHz", "Get/Set timestamp")
    Rel(stm32_fw, sd, "SPI 18MHz", "Write/Read buffer")
    Rel(stm32_fw, display, "SPI 36MHz", "Update display")

    Rel(esp32_fw, mqtt, "MQTT over TCP", "Pub/Sub messages")
    Rel(esp32_fw, relay, "GPIO Control", "Power ON/OFF")
    Rel(buttons, esp32_fw, "GPIO Interrupt", "Button events")

    Rel(web, mqtt, "MQTT WebSocket", "Commands & Data")
    Rel(mobile, mqtt, "MQTT over TCP", "Commands & Data")

    Rel(relay, stm32, "Power Control", "Enable/Disable")

    UpdateLayoutConfig($c4ShapeInRow="3", $c4BoundaryInRow="2")
```

## Package Diagram - Overall System Structure

```mermaid
graph TB
    subgraph Firmware_System[Complete Firmware System]
        direction LR

        subgraph STM32_Package[STM32 Firmware Package]
            STM32_Core[Core<br/>main.c, system_init]
            STM32_Drivers[Drivers<br/>sht3x, ds3231, sd, display]
            STM32_Middleware[Middleware<br/>uart, cmd_parser, data_mgr]
            STM32_HAL[HAL<br/>i2c, spi, uart, gpio]

            STM32_Core --> STM32_Middleware
            STM32_Middleware --> STM32_Drivers
            STM32_Drivers --> STM32_HAL
        end

        subgraph ESP32_Package[ESP32 Firmware Package]
            ESP32_Core[Core<br/>app_main.c, init]
            ESP32_Components[Components<br/>wifi, mqtt, uart, relay]
            ESP32_Protocol[Protocol<br/>json_parser, state_mgr]
            ESP32_IDF[ESP-IDF<br/>wifi, mqtt, uart, gpio]

            ESP32_Core --> ESP32_Components
            ESP32_Components --> ESP32_Protocol
            ESP32_Components --> ESP32_IDF
        end

        subgraph Shared_Package[Shared Definitions]
            Protocol_Def[Protocol<br/>JSON format, commands]
            Data_Structures[Data Structures<br/>sensor_data, timestamps]
            Constants[Constants<br/>topics, pins, configs]
        end
    end

    STM32_Package -.->|UART Protocol| ESP32_Package
    STM32_Middleware -.->|Uses| Protocol_Def
    ESP32_Protocol -.->|Uses| Protocol_Def
    STM32_Core -.->|Uses| Data_Structures
    ESP32_Core -.->|Uses| Data_Structures
    STM32_HAL -.->|Uses| Constants
    ESP32_IDF -.->|Uses| Constants

    style STM32_Package fill:#FFE4B5
    style ESP32_Package fill:#B0E0E6
    style Shared_Package fill:#F0E68C
```

---

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
