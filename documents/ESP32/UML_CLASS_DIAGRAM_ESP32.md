# ESP32 Firmware - UML Class Diagrams

This document provides UML class diagrams showing the structure and relationships of components within the ESP32 firmware.

## Overall System Class Diagram

```mermaid
classDiagram
    class main {
        +wifi_manager_t g_wifi_manager
        +stm32_uart_t g_stm32_uart
        +mqtt_handler_t g_mqtt_handler
        +relay_control_t g_relay
        +json_sensor_parser_t g_json_parser
        +bool g_device_on
        +bool g_periodic_active
        +bool mqtt_current_state
        +uint8_t g_interval_index
        +bool mqtt_reconnected
        +int64_t wifi_reconnect_time_us
        +app_main() void
        +initialize_components() void
        +start_services() void
        +create_state_message(char*, size_t) int
        +publish_current_state() void
        +update_and_publish_state(bool, bool) void
        +on_wifi_event(wifi_state_t, void*) void
        +on_stm32_data_received(const char*) void
        +on_mqtt_data_received(const char*, const char*, int) void
        +on_single_sensor_data(const sensor_data_t*) void
        +on_periodic_sensor_data(const sensor_data_t*) void
        +on_relay_state_changed(bool) void
        +on_button_relay_pressed(gpio_num_t) void
        +on_button_single_pressed(gpio_num_t) void
        +on_button_periodic_pressed(gpio_num_t) void
        +on_button_interval_pressed(gpio_num_t) void
    }
    
    class WiFiManager {
        -wifi_state_t current_state
        -uint8_t retry_count
        -wifi_event_callback_t callback
        -void* callback_arg
        +WiFi_Init(wifi_manager_config_t*) bool
        +WiFi_Connect() bool
        +WiFi_Disconnect() bool
        +WiFi_GetState() wifi_state_t
        +WiFi_IsConnected() bool
        +WiFi_GetIPInfo(esp_netif_ip_info_t*) bool
        +WiFi_GetRSSI(int8_t*) bool
        +WiFi_GetStateString(wifi_state_t) const char*
        +WiFi_SetPowerSave(bool) bool
        +WiFi_Deinit() void
    }
    
    class wifi_manager_config_t {
        +const char* ssid
        +const char* password
        +uint8_t maximum_retry
        +uint8_t scan_method
        +uint8_t sort_method
        +int8_t rssi_threshold
        +wifi_auth_mode_t auth_mode_threshold
        +uint16_t listen_interval
        +bool power_save_enabled
        +wifi_ps_type_t power_save_mode
        +bool ipv6_enabled
        +uint32_t connection_timeout_ms
        +wifi_event_callback_t event_callback
        +void* callback_arg
    }
    
    class wifi_state_t {
        <<enumeration>>
        WIFI_STATE_DISCONNECTED
        WIFI_STATE_CONNECTING
        WIFI_STATE_CONNECTED
        WIFI_STATE_FAILED
    }
    
    class STM32_UART {
        -int uart_num
        -int baud_rate
        -int tx_pin
        -int rx_pin
        -ring_buffer_t rx_buffer
        -stm32_data_callback_t data_callback
        -bool initialized
        +STM32_UART_Init(stm32_uart_t*, int, int, int, int, stm32_data_callback_t) bool
        +STM32_UART_SendCommand(stm32_uart_t*, const char*) bool
        +STM32_UART_ProcessData(stm32_uart_t*) void
        +STM32_UART_StartTask(stm32_uart_t*) bool
        +STM32_UART_Deinit(stm32_uart_t*) void
    }
    
    class RingBuffer {
        -uint8_t* buffer
        -size_t size
        -size_t head
        -size_t tail
        -size_t count
        +RingBuffer_Init(ring_buffer_t*, size_t) bool
        +RingBuffer_Write(ring_buffer_t*, const uint8_t*, size_t) bool
        +RingBuffer_Read(ring_buffer_t*, uint8_t*, size_t) size_t
        +RingBuffer_Available(ring_buffer_t*) size_t
        +RingBuffer_IsFull(ring_buffer_t*) bool
        +RingBuffer_IsEmpty(ring_buffer_t*) bool
        +RingBuffer_Clear(ring_buffer_t*) void
        +RingBuffer_Free(ring_buffer_t*) void
    }
    
    class MQTT_Handler {
        -esp_mqtt_client_handle_t client
        -mqtt_data_callback_t callback
        -bool connected
        -int retry_count
        +MQTT_Handler_Init(mqtt_handler_t*, const char*, const char*, const char*, const char*, mqtt_data_callback_t) bool
        +MQTT_Handler_Start(mqtt_handler_t*) bool
        +MQTT_Handler_Stop(mqtt_handler_t*) bool
        +MQTT_Handler_Subscribe(mqtt_handler_t*, const char*, int) bool
        +MQTT_Handler_Publish(mqtt_handler_t*, const char*, const char*, int, bool) bool
        +MQTT_Handler_IsConnected(mqtt_handler_t*) bool
        +MQTT_Handler_Deinit(mqtt_handler_t*) void
    }
    
    class Relay_Control {
        -int gpio_num
        -bool state
        -bool initialized
        -relay_state_callback_t state_callback
        +Relay_Init(relay_control_t*, int, relay_state_callback_t) bool
        +Relay_SetState(relay_control_t*, bool) bool
        +Relay_GetState(relay_control_t*) bool
        +Relay_Toggle(relay_control_t*) bool
        +Relay_ProcessCommand(relay_control_t*, const char*) bool
        +Relay_Deinit(relay_control_t*) void
    }
    
    class JSON_Sensor_Parser {
        -sensor_data_callback_t single_callback
        -sensor_data_callback_t periodic_callback
        -sensor_data_callback_t error_callback
        +JSON_Parser_Init(json_sensor_parser_t*, sensor_data_callback_t, sensor_data_callback_t, sensor_data_callback_t) bool
        +JSON_Parser_ParseLine(json_sensor_parser_t*, const char*) sensor_data_t
        +JSON_Parser_ProcessLine(json_sensor_parser_t*, const char*) bool
        +JSON_Parser_GetMode(const char*) sensor_mode_t
        +JSON_Parser_GetModeString(sensor_mode_t) const char*
        +JSON_Parser_IsValid(const sensor_data_t*) bool
        +JSON_Parser_IsSensorFailed(const sensor_data_t*) bool
        +JSON_Parser_IsRTCFailed(const sensor_data_t*) bool
    }
    
    class sensor_data_t {
        +sensor_mode_t mode
        +uint32_t timestamp
        +bool valid
        +bool has_temperature
        +float temperature
        +bool has_humidity
        +float humidity
    }
    
    class sensor_mode_t {
        <<enumeration>>
        SENSOR_MODE_UNKNOWN
        SENSOR_MODE_SINGLE
        SENSOR_MODE_PERIODIC
    }
    
    class JSON_Utils {
        <<static>>
        +JSON_Utils_CreateSensorData(char*, size_t, const char*, uint32_t, float, float) int
        +JSON_Utils_CreateSystemState(char*, size_t, bool, bool, uint64_t) int
        +JSON_Utils_CreateSimpleMessage(char*, size_t, const char*, const char*) int
        +JSON_Utils_CreateIntMessage(char*, size_t, const char*, int) int
        +JSON_Utils_FormatFloat(char*, size_t, float, int) const char*
        +JSON_Utils_EscapeString(char*, size_t, const char*) int
    }
    
    class Button_Handler {
        -gpio_num_t gpio_num
        -button_press_callback_t callback
        -uint32_t last_press_time
        -bool initialized
        +Button_Init(gpio_num_t, button_press_callback_t) bool
        +Button_StartTask() bool
        +Button_StopTask() void
    }
    
    %% Relationships
    main --> WiFiManager : uses
    main --> STM32_UART : uses
    main --> MQTT_Handler : uses
    main --> Relay_Control : uses
    main --> JSON_Sensor_Parser : uses
    main --> Button_Handler : uses
    main --> JSON_Utils : uses
    
    WiFiManager --> wifi_manager_config_t : configured by
    WiFiManager --> wifi_state_t : returns
    
    STM32_UART --> RingBuffer : contains
    
    JSON_Sensor_Parser --> sensor_data_t : creates
    JSON_Sensor_Parser --> sensor_mode_t : uses
    JSON_Sensor_Parser ..> JSON_Utils : may use
    
    main --> sensor_data_t : processes
```

## Component Dependencies Diagram

```mermaid
graph TB
    Main[main.c]
    WiFi[WiFi Manager]
    UART[STM32 UART]
    MQTT[MQTT Handler]
    Relay[Relay Control]
    Parser[JSON Sensor Parser]
    Utils[JSON Utils]
    Button[Button Handler]
    RingBuf[Ring Buffer]
    
    Main --> WiFi
    Main --> UART
    Main --> MQTT
    Main --> Relay
    Main --> Parser
    Main --> Button
    Main --> Utils
    
    UART --> RingBuf
    Parser ..> Utils
    
    WiFi -.-> Main : wifi_event_callback
    UART -.-> Main : stm32_data_callback
    MQTT -.-> Main : mqtt_data_callback
    Relay -.-> Main : relay_state_callback
    Parser -.-> Main : sensor_data_callback
    Button -.-> Main : button_press_callback
    
    style Main fill:#90EE90
    style WiFi fill:#87CEEB
    style MQTT fill:#87CEEB
    style Parser fill:#FFD700
    style Relay fill:#DDA0DD
```

## Object Lifecycle Diagram

```mermaid
stateDiagram-v2
    [*] --> Uninitialized
    
    Uninitialized --> Initialized : app_main() calls init functions
    
    state Initialized {
        [*] --> NVS_Ready
        [*] --> EventLoop_Ready
        [*] --> NetIf_Ready
        [*] --> WiFi_Init
        [*] --> UART_Ready
        [*] --> Components_Init
        
        WiFi_Init --> WiFi_Connecting : WiFi_Connect()
        WiFi_Connecting --> WiFi_Connected : connection successful
        WiFi_Connecting --> WiFi_Failed : max retries
        WiFi_Connected --> WiFi_Disconnected : connection lost
        WiFi_Disconnected --> WiFi_Connecting : retry
        WiFi_Failed --> WiFi_Connecting : manual retry (5s)
        
        Components_Init --> MQTT_Initialized
        Components_Init --> Relay_Initialized
        Components_Init --> Parser_Initialized
        Components_Init --> Buttons_Initialized
        
        MQTT_Initialized --> MQTT_Connecting : WiFi stable 4s
        MQTT_Connecting --> MQTT_Connected : broker connected
        MQTT_Connecting --> MQTT_Disconnected : connection failed
        MQTT_Connected --> MQTT_Disconnected : WiFi lost
        MQTT_Disconnected --> MQTT_Connecting : exponential backoff retry
        
        Relay_Initialized --> Relay_OFF
        Relay_OFF --> Relay_ON : Relay_SetState(true)
        Relay_ON --> Relay_OFF : Relay_SetState(false)
    }
    
    Initialized --> Running : start_services()
    
    state Running {
        [*] --> Main_Loop
        
        Main_Loop --> Monitor_WiFi
        Monitor_WiFi --> Check_MQTT
        Check_MQTT --> Process_Callbacks
        Process_Callbacks --> Main_Loop
        
        state Process_Callbacks {
            [*] --> Wait_Event
            Wait_Event --> WiFi_Event : WiFi state change
            Wait_Event --> MQTT_Event : MQTT message
            Wait_Event --> UART_Event : STM32 data
            Wait_Event --> Button_Event : Button press
            Wait_Event --> Relay_Event : Relay state change
            
            WiFi_Event --> [*]
            MQTT_Event --> [*]
            UART_Event --> [*]
            Button_Event --> [*]
            Relay_Event --> [*]
        }
    }
    
    Running --> [*] : system shutdown
```

## ESP32 Data Flow Diagram

```mermaid
graph LR
    subgraph STM32_to_Cloud["STM32 → Cloud"]
        STM32_HW[STM32 Hardware]
        UART_RX[UART RX]
        RingBuf[Ring Buffer]
        LineExtract[Line Extraction]
        JSONParse[JSON Parser]
        SensorCB[Sensor Callback]
        FormatJSON[JSON Utils]
        MQTTPub[MQTT Publish]
        Broker[MQTT Broker]
        
        STM32_HW -->|UART 115200| UART_RX
        UART_RX --> RingBuf
        RingBuf --> LineExtract
        LineExtract --> JSONParse
        JSONParse -->|mode routing| SensorCB
        SensorCB --> FormatJSON
        FormatJSON --> MQTTPub
        MQTTPub --> Broker
    end
    
    subgraph Cloud_to_STM32["Cloud → STM32"]
        Broker2[MQTT Broker]
        MQTTSub[MQTT Subscribe]
        TopicRoute[Topic Router]
        UART_TX[UART TX]
        STM32_HW2[STM32 Hardware]
        
        Broker2 --> MQTTSub
        MQTTSub --> TopicRoute
        TopicRoute -->|command| UART_TX
        UART_TX -->|UART 115200| STM32_HW2
    end
    
    subgraph Relay_Control["Relay Control"]
        MQTTCtrl[MQTT Control]
        RelayHW[Relay Hardware]
        StateUpdate[State Update]
        StatePub[State Publish]
        BrokerState[MQTT Broker]
        
        MQTTCtrl --> RelayHW
        RelayHW --> StateUpdate
        StateUpdate --> StatePub
        StatePub --> BrokerState
    end
    
    style STM32_HW fill:#90EE90
    style Broker fill:#87CEEB
    style RelayHW fill:#DDA0DD
```

## MQTT Topic Architecture

```mermaid
graph TB
    subgraph MQTT_Broker["MQTT Broker: 192.168.1.39:1883"]
        subgraph Subscribe["ESP32 Subscribe"]
            T1[datalogger/stm32/command]
            T2[datalogger/esp32/relay/control]
            T3[datalogger/esp32/system/state]
        end
        
        subgraph Publish["ESP32 Publish"]
            T4[datalogger/stm32/single/data]
            T5[datalogger/stm32/periodic/data]
            T6[datalogger/esp32/system/state]
        end
    end
    
    T1 -->|forward to STM32| ESP32_1[ESP32]
    T2 -->|process relay command| ESP32_2[ESP32]
    T3 -->|respond state| ESP32_3[ESP32]
    
    ESP32_4[ESP32] -->|sensor data| T4
    ESP32_5[ESP32] -->|sensor data| T5
    ESP32_6[ESP32] -->|state change| T6
    
    style MQTT_Broker fill:#87CEEB
    style ESP32_1 fill:#90EE90
    style ESP32_2 fill:#90EE90
    style ESP32_3 fill:#90EE90
    style ESP32_4 fill:#90EE90
    style ESP32_5 fill:#90EE90
    style ESP32_6 fill:#90EE90
```

## GPIO Hardware Configuration

```mermaid
graph LR
    subgraph ESP32_WROOM_32
        subgraph UART1["UART1 - STM32 Communication"]
            TX[GPIO17 - TX]
            RX[GPIO16 - RX]
        end
        
        subgraph Relay_GPIO["Relay Control"]
            R[GPIO4 - Relay]
        end
        
        subgraph Button_GPIO["Button Inputs"]
            B1[GPIO5 - Relay Toggle]
            B2[GPIO16 - Single Measure]
            B3[GPIO17 - Periodic Toggle]
            B4[GPIO4 - Interval Adjust]
        end
        
        subgraph LED_GPIO["Status LEDs"]
            L1[GPIO_WiFi - WiFi LED]
            L2[GPIO_MQTT - MQTT LED]
        end
    end
    
    TX -->|115200 baud| STM32_RX[STM32 RX]
    RX -->|115200 baud| STM32_TX[STM32 TX]
    
    R --> RelayModule[Relay Module]
    
    B1 -.->|pull-up, active low| GND1[GND]
    B2 -.->|pull-up, active low| GND2[GND]
    B3 -.->|pull-up, active low| GND3[GND]
    B4 -.->|pull-up, active low| GND4[GND]
    
    L1 --> WiFi_LED[WiFi Status LED]
    L2 --> MQTT_LED[MQTT Status LED]
    
    style ESP32_WROOM_32 fill:#90EE90
    style UART1 fill:#87CEEB
    style Relay_GPIO fill:#DDA0DD
    style Button_GPIO fill:#FFD700
    style LED_GPIO fill:#98FB98
```

## Configuration Constants

```mermaid
classDiagram
    class WiFi_Config {
        +SSID: "Redmi Note 9 Pro"
        +Password: "12345678"
        +Max_Retry: 5
        +Connection_Timeout: 10000ms
        +Retry_Interval: 2000ms
        +Manual_Retry_Interval: 5000ms
    }
    
    class MQTT_Config {
        +Broker_URI: "mqtt://192.168.1.39:1883"
        +Client_ID: "esp32_client"
        +Username: "admin"
        +Password: "password"
        +Protocol_Version: MQTT v5.0
        +Keepalive: 60s
        +Reconnect_Backoff: min(60s, 2^retry)
    }
    
    class UART_Config {
        +Port: UART_NUM_1
        +Baud_Rate: 115200
        +TX_Pin: GPIO_NUM_17
        +RX_Pin: GPIO_NUM_16
        +RX_Buffer_Size: 1024
        +Line_Max_Length: 128
    }
    
    class Relay_Config {
        +GPIO: GPIO_NUM_4
        +Active_Level: HIGH
        +STM32_Boot_Delay: 500ms
    }
    
    class Button_Config {
        +Relay_Button: GPIO_NUM_5
        +Single_Button: GPIO_NUM_16
        +Periodic_Button: GPIO_NUM_17
        +Interval_Button: GPIO_NUM_4
        +Debounce_Time: 200ms
    }
    
    class LED_Config {
        +WiFi_LED: GPIO_Configurable
        +MQTT_LED: GPIO_Configurable
        +Active_Level: HIGH
    }
    
    class Timing_Config {
        +WiFi_Stabilization_Delay: 4000ms
        +Main_Loop_Delay: 100ms
        +Status_Log_Interval: 30000ms
        +Button_Debounce_Time: 50ms
        +STM32_Boot_Wait: 500ms
    }
```

## Retry Logic and Error Handling

```mermaid
stateDiagram-v2
    [*] --> WiFi_Retry_Logic
    
    state WiFi_Retry_Logic {
        [*] --> Attempt_1
        Attempt_1 --> Attempt_2 : failed (delay 2s)
        Attempt_2 --> Attempt_3 : failed (delay 2s)
        Attempt_3 --> Attempt_4 : failed (delay 2s)
        Attempt_4 --> Attempt_5 : failed (delay 2s)
        Attempt_5 --> Manual_Retry : failed (delay 2s)
        Manual_Retry --> Attempt_1 : retry (interval 5s)
        
        Attempt_1 --> [*] : success
        Attempt_2 --> [*] : success
        Attempt_3 --> [*] : success
        Attempt_4 --> [*] : success
        Attempt_5 --> [*] : success
    }
    
    WiFi_Retry_Logic --> MQTT_Retry_Logic
    
    state MQTT_Retry_Logic {
        [*] --> First_Attempt
        First_Attempt --> Retry_1 : failed (delay 1s)
        Retry_1 --> Retry_2 : failed (delay 2s)
        Retry_2 --> Retry_3 : failed (delay 4s)
        Retry_3 --> Retry_4 : failed (delay 8s)
        Retry_4 --> Retry_5 : failed (delay 16s)
        Retry_5 --> Retry_Max : failed (delay 32s)
        Retry_Max --> Retry_Max : failed (delay 60s max)
        
        First_Attempt --> [*] : success
        Retry_1 --> [*] : success
        Retry_2 --> [*] : success
        Retry_3 --> [*] : success
        Retry_4 --> [*] : success
        Retry_5 --> [*] : success
        Retry_Max --> [*] : success
    }
    
    MQTT_Retry_Logic --> [*]
```

## Overall ESP32 State Machine

```mermaid
stateDiagram-v2
    [*] --> Startup
    
    state Startup {
        [*] --> Init_NVS
        Init_NVS --> Init_EventLoop
        Init_EventLoop --> Init_NetIf
        Init_NetIf --> Init_WiFi
        Init_WiFi --> Init_UART
        Init_UART --> Init_LED
        Init_LED --> Connect_WiFi
        Connect_WiFi --> Init_Components
        Init_Components --> Start_Services
        Start_Services --> [*]
    }
    
    Startup --> Operating : initialization successful
    Startup --> Init_Error : initialization failed
    
    state Operating {
        [*] --> Main_Loop
        
        state Main_Loop {
            [*] --> Check_WiFi
            Check_WiFi --> Manage_MQTT
            Manage_MQTT --> Process_Events
            Process_Events --> Delay_100ms
            Delay_100ms --> Check_WiFi
        }
        
        Main_Loop --> WiFi_States : WiFi event
        Main_Loop --> MQTT_States : MQTT event
        Main_Loop --> Button_States : Button event
        Main_Loop --> Relay_States : Relay event
        
        state WiFi_States {
            [*] --> Disconnected
            Disconnected --> Connecting : retry
            Connecting --> Connected : success
            Connecting --> Failed : max retries
            Connected --> Disconnected : lost connection
            Failed --> Connecting : manual retry
        }
        
        state MQTT_States {
            [*] --> MQTT_Disconnected
            MQTT_Disconnected --> MQTT_Connecting : WiFi stable 4s
            MQTT_Connecting --> MQTT_Connected : broker connected
            MQTT_Connecting --> MQTT_Disconnected : failed
            MQTT_Connected --> MQTT_Disconnected : WiFi lost
        }
        
        state Button_States {
            [*] --> Button_Idle
            Button_Idle --> Button_Pressed : interrupt
            Button_Pressed --> Button_Debounce : 50ms wait
            Button_Debounce --> Button_Released : confirm release
            Button_Released --> Button_Process : execute action
            Button_Process --> Button_Idle
        }
        
        state Relay_States {
            [*] --> Relay_OFF
            Relay_OFF --> Relay_ON : command/button
            Relay_ON --> Relay_OFF : command/button
            Relay_ON --> State_Update : state change
            Relay_OFF --> State_Update : state change
            State_Update --> MQTT_Publish : if connected
            MQTT_Publish --> [*]
        }
        
        WiFi_States --> Main_Loop
        MQTT_States --> Main_Loop
        Button_States --> Main_Loop
        Relay_States --> Main_Loop
    }
    
    Init_Error --> Restart : ESP32 restart
    Restart --> [*]
    
    Operating --> [*] : system shutdown
```

## Summary

The ESP32 firmware architecture is built on a component-based design with clear separation of concerns:

- **Main Application (main)**: Orchestrates all components and manages application state
- **WiFi Manager**: Handles WiFi connection with automatic retry logic
- **UART Handler**: Manages line-based communication with STM32 via ring buffer
- **MQTT Handler**: Provides MQTT client with exponential backoff retry
- **Relay Control**: Controls GPIO relay with state callbacks
- **JSON Parser**: Parses sensor data with mode-specific routing
- **JSON Utils**: Provides centralized JSON formatting utilities
- **Button Handler**: Monitors button presses with debouncing
- **Ring Buffer**: Circular buffer for UART data reception

All components use callback-based event handling for loose coupling and operate within the FreeRTOS task model. The architecture ensures reliable WiFi/MQTT connectivity, proper STM32 communication, and responsive user interaction via buttons.

**Key Features:**
- 4-second WiFi stabilization delay before MQTT start
- 500ms delay after relay toggle for STM32 boot time
- Exponential backoff retry for MQTT reconnection
- Device state protection (buttons disabled when relay OFF)
- State synchronization via MQTT retained messages
- Automatic WiFi reconnection with 5 retry attempts
- Hardware and software debouncing for buttons
- Ring buffer for reliable UART reception
