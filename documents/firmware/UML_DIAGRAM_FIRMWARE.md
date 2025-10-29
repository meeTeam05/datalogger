# UML Diagrams - Firmware System

This document contains only UML diagrams for the firmware system.

## 1. Class Diagram - Complete System

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

## 2. Sequence Diagram - Command Flow

```mermaid
sequenceDiagram
    participant Web as Web Interface
    participant Broker as MQTT Broker
    participant ESP32
    participant UART as UART Link
    participant STM32
    participant Sensor as SHT3X

    Note over Web,Sensor: Command Flow (Web → STM32)

    Web->>Broker: PUBLISH datalogger/stm32/command<br/>"SINGLE"
    Broker->>ESP32: MQTT message
    ESP32->>ESP32: Route command by topic
    ESP32->>UART: "SINGLE\n"
    UART->>STM32: UART RX interrupt
    STM32->>STM32: Parse command
    STM32->>Sensor: I2C read request
    Sensor-->>STM32: Temperature & Humidity
    STM32->>STM32: Format JSON with timestamp
    STM32->>UART: JSON + "\n"
    UART->>ESP32: UART RX interrupt
    ESP32->>ESP32: Parse JSON
    ESP32->>Broker: PUBLISH datalogger/stm32/single/data
    Broker->>Web: Forward sensor data
```

## 3. Sequence Diagram - Periodic Mode

```mermaid
sequenceDiagram
    participant Web as Web Interface
    participant Broker as MQTT Broker
    participant ESP32
    participant UART as UART Link
    participant STM32
    participant Sensor as SHT3X

    Note over Web,Sensor: Periodic Mode Flow

    Web->>Broker: PUBLISH "PERIODIC ON"
    Broker->>ESP32: Command
    ESP32->>UART: "PERIODIC ON\n"
    UART->>STM32: Command received
    STM32->>Sensor: Start periodic mode

    loop Every 5 seconds
        Sensor-->>STM32: Auto measurement
        STM32->>STM32: Fetch data via I2C
        STM32->>STM32: Format JSON
        STM32->>UART: JSON + "\n"
        UART->>ESP32: Data received
        ESP32->>ESP32: Parse JSON
        ESP32->>Broker: PUBLISH periodic data
        Broker->>Web: Display data
    end

    Web->>Broker: PUBLISH "PERIODIC OFF"
    Broker->>ESP32: Command
    ESP32->>UART: "PERIODIC OFF\n"
    UART->>STM32: Command received
    STM32->>Sensor: Stop periodic mode
```

## 4. Sequence Diagram - MQTT Connection Status

```mermaid
sequenceDiagram
    participant STM32
    participant UART as UART Link
    participant ESP32
    participant WiFi as WiFi Network
    participant Broker as MQTT Broker

    Note over STM32,Broker: ESP32 Initialization

    ESP32->>WiFi: Connect to network
    WiFi-->>ESP32: Connected
    ESP32->>ESP32: Wait 4s for stability
    ESP32->>Broker: MQTT Connect
    Broker-->>ESP32: Connection accepted
    ESP32->>UART: "MQTT CONNECTED\n"
    UART->>STM32: Status notification
    STM32->>STM32: Switch to LIVE mode

    Note over STM32,Broker: Connection Lost

    Broker->>ESP32: Disconnect
    ESP32->>UART: "MQTT DISCONNECTED\n"
    UART->>STM32: Status notification
    STM32->>STM32: Switch to BUFFER mode

    Note over STM32,Broker: Reconnection

    ESP32->>Broker: Retry connection
    Broker-->>ESP32: Connected
    ESP32->>UART: "MQTT CONNECTED\n"
    UART->>STM32: Status notification
    STM32->>STM32: Switch to LIVE mode
    STM32->>STM32: Flush SD buffer
```

## 5. State Diagram - STM32 Operation

```mermaid
stateDiagram-v2
    [*] --> Init

    Init --> Idle : Initialization complete

    state Idle {
        [*] --> WaitingCommand
        WaitingCommand --> ProcessCommand : Command received
        ProcessCommand --> WaitingCommand : Command executed
    }

    Idle --> SingleMeasure : SINGLE command
    Idle --> PeriodicMode : PERIODIC ON

    state SingleMeasure {
        [*] --> ReadSensor
        ReadSensor --> FormatData : Data acquired
        FormatData --> CheckMQTT : JSON ready

        state CheckMQTT <<choice>>
        CheckMQTT --> SendUART : MQTT connected
        CheckMQTT --> BufferSD : MQTT disconnected

        SendUART --> [*]
        BufferSD --> [*]
    }

    state PeriodicMode {
        [*] --> StartPeriodic
        StartPeriodic --> WaitInterval : Timer started
        WaitInterval --> FetchData : 5s elapsed
        FetchData --> CheckMQTT2 : Data ready

        state CheckMQTT2 <<choice>>
        CheckMQTT2 --> SendUART2 : MQTT connected
        CheckMQTT2 --> BufferSD2 : MQTT disconnected

        SendUART2 --> WaitInterval
        BufferSD2 --> WaitInterval

        WaitInterval --> StopPeriodic : PERIODIC OFF
        StopPeriodic --> [*]
    }

    SingleMeasure --> Idle : Completed
    PeriodicMode --> Idle : Stopped

    Idle --> SetTime : SET TIME command
    SetTime --> Idle : Time updated

    Idle --> ClearSD : SD CLEAR command
    ClearSD --> Idle : SD cleared
```

## 6. State Diagram - ESP32 Operation

```mermaid
stateDiagram-v2
    [*] --> PowerOn

    PowerOn --> InitWiFi : System init

    state InitWiFi {
        [*] --> Disconnected
        Disconnected --> Connecting : Start connect
        Connecting --> Connected : Success
        Connecting --> RetryWait : Failed
        RetryWait --> Connecting : After 2s delay
        Connected --> Disconnected : Connection lost
    }

    InitWiFi --> InitMQTT : WiFi stable 4s

    state InitMQTT {
        [*] --> MQTT_Disconnected
        MQTT_Disconnected --> MQTT_Connecting : Attempt connect
        MQTT_Connecting --> MQTT_Connected : Success
        MQTT_Connecting --> MQTT_Backoff : Failed
        MQTT_Backoff --> MQTT_Connecting : Exponential backoff
        MQTT_Connected --> MQTT_Disconnected : WiFi lost
    }

    InitMQTT --> Operational : MQTT connected

    state Operational {
        [*] --> Monitoring

        Monitoring --> HandleMQTT : MQTT message
        Monitoring --> HandleUART : UART data
        Monitoring --> HandleButton : Button press

        HandleMQTT --> RouteCommand : Parse topic
        RouteCommand --> SendSTM32 : STM32 command
        RouteCommand --> ControlRelay : Relay command
        RouteCommand --> PublishState : State request

        HandleUART --> ParseJSON : Extract data
        ParseJSON --> PublishMQTT : Valid JSON
        ParseJSON --> LogError : Invalid JSON

        HandleButton --> ToggleDevice : Button 1
        HandleButton --> ToggleRelay : Button 2
        HandleButton --> SendCommand : Button 3/4

        SendSTM32 --> Monitoring
        ControlRelay --> PublishState
        PublishState --> Monitoring
        PublishMQTT --> Monitoring
        LogError --> Monitoring
        ToggleDevice --> PublishState
        ToggleRelay --> PublishState
        SendCommand --> Monitoring
    }

    Operational --> InitMQTT : Connection lost
```

## 7. Component Diagram

```mermaid
graph TB
    subgraph STM32_System["STM32 System"]
        STM32_Main["Main Application"]
        STM32_HAL["HAL Drivers"]
        STM32_Peripherals["Peripheral Handlers"]
        STM32_Middleware["Middleware"]

        STM32_Main --> STM32_HAL
        STM32_Main --> STM32_Peripherals
        STM32_Main --> STM32_Middleware

        STM32_Peripherals --> STM32_HAL
        STM32_Middleware --> STM32_HAL
    end

    subgraph ESP32_System["ESP32 System"]
        ESP32_Main["Main Application"]
        ESP32_IDF["ESP-IDF"]
        ESP32_Network["Network Stack"]
        ESP32_Drivers["Device Drivers"]

        ESP32_Main --> ESP32_IDF
        ESP32_Main --> ESP32_Network
        ESP32_Main --> ESP32_Drivers

        ESP32_Network --> ESP32_IDF
        ESP32_Drivers --> ESP32_IDF
    end

    subgraph External_Systems["External Systems"]
        MQTT_Broker["MQTT Broker"]
        Web_UI["Web Interface"]
        Sensors["Physical Sensors"]
    end

    STM32_Peripherals <-->|I2C/SPI| Sensors
    STM32_Peripherals <-->|UART| ESP32_Drivers
    ESP32_Network <-->|WiFi/MQTT| MQTT_Broker
    MQTT_Broker <-->|WebSocket/HTTP| Web_UI

    style STM32_System fill:#FFE4B5
    style ESP32_System fill:#B0E0E6
    style External_Systems fill:#E0E0E0
```

## 8. Deployment Diagram

```mermaid
graph TB
    subgraph Physical_Device["IoT Device Hardware"]
        subgraph STM32_Node["STM32F103C8T6<br/>Microcontroller"]
            STM32_FW["STM32 Firmware<br/>C/HAL"]
        end

        subgraph ESP32_Node["ESP32-WROOM-32<br/>WiFi Module"]
            ESP32_FW["ESP32 Firmware<br/>C/ESP-IDF"]
        end

        subgraph Sensor_Node["Sensor Hardware"]
            SHT3X["SHT3X<br/>I2C 0x44"]
            DS3231["DS3231<br/>I2C 0x68"]
            SD_Card["SD Card<br/>SPI"]
            Display["ILI9225<br/>SPI"]
        end
    end

    subgraph Network_Infrastructure["Network Layer"]
        Router["WiFi Router<br/>SSID: Redmi Note 9 Pro"]

        subgraph Server_Node["Server: 192.168.1.39"]
            MQTT["Mosquitto Broker<br/>Port 1883<br/>MQTT v5.0"]
        end
    end

    subgraph Client_Device["User Device"]
        Browser["Web Browser<br/>Dashboard UI<br/>HTML/CSS/JS"]
    end

    STM32_FW <-->|UART 115200| ESP32_FW
    STM32_FW <-->|I2C| SHT3X
    STM32_FW <-->|I2C| DS3231
    STM32_FW <-->|SPI| SD_Card
    STM32_FW <-->|SPI| Display

    ESP32_FW <-->|WiFi 2.4GHz| Router
    Router <-->|TCP/IP| MQTT
    MQTT <-->|WebSocket| Browser

    style Physical_Device fill:#F0E68C
    style Network_Infrastructure fill:#87CEEB
    style Client_Device fill:#90EE90
    style STM32_Node fill:#FFE4B5
    style ESP32_Node fill:#B0E0E6
```

## 9. Activity Diagram - Single Measurement Flow

```mermaid
stateDiagram-v2
    [*] --> ReceiveCommand

    ReceiveCommand --> ValidateCommand : SINGLE command
    ValidateCommand --> InitSensor : Valid
    ValidateCommand --> [*] : Invalid

    InitSensor --> ReadI2C : SHT3X ready
    InitSensor --> ReturnError : Init failed

    ReadI2C --> GetTimestamp : Data received
    ReadI2C --> ReturnZero : Read timeout

    GetTimestamp --> FormatJSON : RTC success
    GetTimestamp --> UseZeroTime : RTC failed

    UseZeroTime --> FormatJSON
    ReturnZero --> FormatJSON

    FormatJSON --> CheckMQTTStatus : JSON ready

    state CheckMQTTStatus <<choice>>
    CheckMQTTStatus --> TransmitUART : Connected
    CheckMQTTStatus --> BufferToSD : Disconnected

    TransmitUART --> UpdateDisplay
    BufferToSD --> UpdateDisplay

    UpdateDisplay --> [*]
    ReturnError --> [*]
```

## 10. Activity Diagram - ESP32 Message Routing

```mermaid
stateDiagram-v2
    [*] --> MessageReceived

    state MessageReceived <<choice>>
    MessageReceived --> MQTTMessage : From MQTT
    MessageReceived --> UARTMessage : From UART
    MessageReceived --> ButtonEvent : From GPIO

    MQTTMessage --> ParseTopic

    state ParseTopic <<choice>>
    ParseTopic --> STM32Command : /stm32/command
    ParseTopic --> RelayCommand : /relay/control
    ParseTopic --> StateRequest : /system/state

    STM32Command --> ForwardUART
    RelayCommand --> ControlRelay
    StateRequest --> PublishState

    UARTMessage --> ParseJSON

    state ParseJSON <<choice>>
    ParseJSON --> PublishSingle : mode: SINGLE
    ParseJSON --> PublishPeriodic : mode: PERIODIC
    ParseJSON --> LogError : Invalid JSON

    ButtonEvent --> IdentifyButton

    state IdentifyButton <<choice>>
    IdentifyButton --> ToggleDevice : Button 1
    IdentifyButton --> ToggleRelay : Button 2
    IdentifyButton --> SendSingle : Button 3
    IdentifyButton --> SendPeriodic : Button 4

    ForwardUART --> [*]
    ControlRelay --> PublishState
    PublishState --> [*]
    PublishSingle --> [*]
    PublishPeriodic --> [*]
    LogError --> [*]
    ToggleDevice --> PublishState
    ToggleRelay --> PublishState
    SendSingle --> ForwardUART
    SendPeriodic --> ForwardUART
```

## Diagram Summary

This document contains **10 UML diagrams**:

1. **Class Diagram** - Complete system classes and relationships
2. **Sequence Diagram** - Command flow from web to sensor
3. **Sequence Diagram** - Periodic mode operation
4. **Sequence Diagram** - MQTT connection management
5. **State Diagram** - STM32 operation states
6. **State Diagram** - ESP32 operation states
7. **Component Diagram** - System components and dependencies
8. **Deployment Diagram** - Physical deployment architecture
9. **Activity Diagram** - Single measurement workflow
10. **Activity Diagram** - ESP32 message routing logic

These diagrams follow standard UML notation and provide comprehensive views of the firmware system architecture.
