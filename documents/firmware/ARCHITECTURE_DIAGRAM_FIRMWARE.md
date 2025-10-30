# Firmware System - Architecture Diagrams

This document provides the system architecture, data flow, state machines, and infrastructure diagrams for the ESP32 and STM32 coordination system.

## System Architecture Overview

```mermaid
graph TB
    subgraph Layer1[" "]
        subgraph External["External Layer"]
            direction LR
            User["Users<br/>Web/Mobile<br/>Dashboard"]
            Broker["MQTT Broker<br/>192.168.1.39:1883<br/>Mosquitto"]
            Network["WiFi Network<br/>2.4GHz<br/>WPA2-PSK"]
            Power["Power Supply<br/>5V DC<br/>2A"]
        end
    end

    subgraph Layer2[" "]
        subgraph ESP32_System["ESP32-WROOM-32 Gateway Layer"]
            direction TB

            subgraph ESP32_Core["Core Application"]
                ESP32_Main["Main Control Loop<br/>FreeRTOS Tasks"]
            end

            subgraph ESP32_Comm["Communication Layer"]
                ESP32_WiFi["WiFi Manager<br/>STA Mode"]
                ESP32_MQTT["MQTT Client<br/>Pub/Sub Handler"]
                ESP32_UART2["UART Interface<br/>Ring Buffer"]
            end

            subgraph ESP32_Control["Control Layer"]
                ESP32_Relay["Relay Driver<br/>GPIO4 Output"]
                ESP32_Button["Button Handler<br/>4x GPIO Input"]
                ESP32_JSON["JSON Parser<br/>Sensor Data"]
            end

            ESP32_Main --> ESP32_WiFi
            ESP32_Main --> ESP32_MQTT
            ESP32_Main --> ESP32_UART2
            ESP32_Main --> ESP32_Relay
            ESP32_Main --> ESP32_Button
            ESP32_Main --> ESP32_JSON
        end
    end

    subgraph Layer3[" "]
        subgraph STM32_System["STM32F103C8T6 Data Acquisition Layer"]
            direction TB

            subgraph STM32_Core["Core Application"]
                STM32_Main["Main State Machine<br/>HAL Framework"]
            end

            subgraph STM32_Protocol["Protocol Layer"]
                STM32_UART["UART Handler<br/>Command Parser"]
                STM32_CMD["Command Executor<br/>8 Commands"]
            end

            subgraph STM32_Data["Data Management Layer"]
                STM32_DM["Data Manager<br/>Single/Periodic"]
                STM32_SD["SD Card Manager<br/>Circular Buffer"]
            end

            subgraph STM32_HAL["Hardware Abstraction Layer"]
                STM32_I2C["I2C Driver<br/>100kHz"]
                STM32_SPI["SPI Driver<br/>18/36MHz"]
                STM32_Display["Display Driver<br/>ILI9225"]
            end

            STM32_Main --> STM32_UART
            STM32_Main --> STM32_I2C
            STM32_Main --> STM32_SPI
            STM32_Main --> STM32_Display
            STM32_UART --> STM32_CMD
            STM32_CMD --> STM32_DM
            STM32_DM --> STM32_SD
        end
    end

    subgraph Layer4[" "]
        subgraph Hardware["Hardware Peripherals Layer"]
            direction LR

            subgraph Sensors_I2C["I2C Sensors"]
                SHT3X["SHT3X<br/>Temp & Humidity<br/>0x44"]
                RTC["DS3231 RTC<br/>Timestamp<br/>0x68"]
            end

            subgraph Storage_Display["SPI Devices"]
                SD["SD Card<br/>Data Buffer<br/>FAT32"]
                TFT["ILI9225 TFT<br/>176x220<br/>Status Display"]
            end

            subgraph IO_Devices["GPIO Devices"]
                RelayHW["Relay Module<br/>10A 250VAC<br/>Power Switch"]
                Buttons["Tactile Buttons<br/>4x Input<br/>Pull-up"]
            end
        end
    end

    %% External to ESP32
    User <-.->|MQTT Commands| Broker
    Broker <-->|TCP 1883| ESP32_MQTT
    Network <-->|802.11n| ESP32_WiFi
    Power -->|5V| ESP32_System
    Power -->|5V via Relay| STM32_System

    %% ESP32 to STM32
    ESP32_UART2 <-->|UART 115200<br/>JSON Protocol| STM32_UART

    %% ESP32 to Hardware
    ESP32_Relay -->|GPIO High/Low| RelayHW
    Buttons -->|Active Low<br/>Debounced| ESP32_Button

    %% STM32 to Hardware
    STM32_I2C <-->|I2C Bus| SHT3X
    STM32_I2C <-->|I2C Bus| RTC
    STM32_SPI <-->|SPI1| SD
    STM32_SPI <-->|SPI2| TFT

    %% Power Control
    RelayHW -.->|Enable/Disable<br/>Power| STM32_System

    %% Styling
    style Layer1 fill:none,stroke:none
    style Layer2 fill:none,stroke:none
    style Layer3 fill:none,stroke:none
    style Layer4 fill:none,stroke:none

    style External fill:#E8F4F8,stroke:#0288D1,stroke-width:3px,color:#000
    style ESP32_System fill:#E3F2FD,stroke:#1976D2,stroke-width:3px,color:#000
    style STM32_System fill:#FFF3E0,stroke:#F57C00,stroke-width:3px,color:#000
    style Hardware fill:#F1F8E9,stroke:#558B2F,stroke-width:3px,color:#000

    style ESP32_Core fill:#BBDEFB,stroke:#1565C0,stroke-width:2px,color:#000
    style ESP32_Comm fill:#B3E5FC,stroke:#0277BD,stroke-width:2px,color:#000
    style ESP32_Control fill:#B2EBF2,stroke:#00838F,stroke-width:2px,color:#000

    style STM32_Core fill:#FFE0B2,stroke:#E65100,stroke-width:2px,color:#000
    style STM32_Protocol fill:#FFCCBC,stroke:#D84315,stroke-width:2px,color:#000
    style STM32_Data fill:#FFAB91,stroke:#BF360C,stroke-width:2px,color:#000
    style STM32_HAL fill:#FF8A65,stroke:#A1320C,stroke-width:2px,color:#000

    style Sensors_I2C fill:#C5E1A5,stroke:#558B2F,stroke-width:2px,color:#000
    style Storage_Display fill:#AED581,stroke:#689F38,stroke-width:2px,color:#000
    style IO_Devices fill:#9CCC65,stroke:#558B2F,stroke-width:2px,color:#000
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

    style HAL fill:#FFE4B5, color:#000000
    style Drivers fill:#F0E68C, color:#000000
    style Middleware fill:#DDA0DD, color:#000000
    style Application fill:#90EE90, color:#000000
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

    style ESP_IDF fill:#B0E0E6, color:#000000
    style Custom_Drivers fill:#DDA0DD, color:#000000
    style Protocol fill:#F0E68C, color:#000000
    style Application fill:#90EE90, color:#000000
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

    style STM32_Package fill:#FFE4B5, color:#000000
    style ESP32_Package fill:#B0E0E6, color:#000000
    style Shared_Package fill:#F0E68C, color:#000000
```

## Data Flow Architecture

```mermaid
graph LR
    subgraph Input[Data Input Sources]
        Sensor[SHT3X Sensor<br/>Temperature & Humidity<br/>I2C Read 15ms]
        Time[DS3231 RTC<br/>Timestamp<br/>I2C Read 5ms]
        WebCmd[Web Commands<br/>via MQTT<br/>QoS 1]
        ButtonCmd[Button Presses<br/>GPIO Interrupt<br/>200ms debounce]
    end

    subgraph STM32_Processing[STM32 Processing]
        I2C[I2C Read<br/>100kHz<br/>~20ms total]
        Parse[JSON Format<br/>with Timestamp<br/>~5ms]
        Route{MQTT<br/>Connected?}
        Buffer[SD Card Buffer<br/>204,800 records<br/>Write ~10ms]

        Sensor --> I2C
        Time --> I2C
        I2C --> Parse
        Parse --> Route
        Route -->|No| Buffer
    end

    subgraph ESP32_Processing[ESP32 Processing]
        UARTRx[UART RX<br/>Ring Buffer<br/>115200 baud]
        JSONParse[JSON Parser<br/>Mode Detection<br/>Validation]
        MQTTPub[MQTT Publish<br/>QoS 0/1<br/>Retained State]
        CmdRoute[Command Router<br/>Topic-based<br/>Dispatch]
        StateSync[State Manager<br/>Retained Messages<br/>System State]

        Route -->|Yes| UARTRx
        UARTRx --> JSONParse
        JSONParse --> MQTTPub

        WebCmd --> CmdRoute
        ButtonCmd --> CmdRoute
        CmdRoute --> StateSync
    end

    subgraph Output[Data Output]
        Cloud[MQTT Broker<br/>192.168.1.39<br/>Port 1883]
        Display[TFT Display<br/>ILI9225<br/>Status Info]
        LED[Status LEDs<br/>WiFi/MQTT<br/>Connection State]
        RelayOut[Relay Output<br/>Device Control<br/>Power Management]

        MQTTPub --> Cloud
        Parse --> Display
        StateSync --> Display
        StateSync --> LED
        CmdRoute --> RelayOut
    end

    Buffer -.->|On Reconnect<br/>Buffered Data| UARTRx

    style Input fill:#90EE90, color:#000000
    style STM32_Processing fill:#FFE4B5, color:#000000
    style ESP32_Processing fill:#B0E0E6, color:#000000
    style Output fill:#DDA0DD, color:#000000
```

## Hardware Configuration and Pinout

```mermaid
graph TB
    subgraph STM32_HW[STM32F103C8T6 Hardware Configuration]
        STM32_MCU[ARM Cortex-M3<br/>Clock: 72MHz<br/>Flash: 64KB<br/>RAM: 20KB<br/>Power: 20mA active]

        STM32_I2C[I2C1 Interface<br/>PB6: SCL<br/>PB7: SDA<br/>Speed: 100kHz<br/>Pull-up: 4.7k ohm]

        STM32_SPI1[SPI1 SD Card<br/>PA5: SCK<br/>PA6: MISO<br/>PA7: MOSI<br/>PA4: CS<br/>Speed: 18MHz]

        STM32_SPI2[SPI2 Display<br/>PB13: SCK<br/>PB14: MISO<br/>PB15: MOSI<br/>PB12: CS<br/>Speed: 36MHz]

        STM32_UART[UART1 Interface<br/>PA9: TX<br/>PA10: RX<br/>Baud: 115200<br/>Format: 8N1]

        STM32_GPIO[GPIO Pins<br/>LED: PC13<br/>Display DC: PA8<br/>Display RS: PA11<br/>Display BL: PA12]

        STM32_MCU --> STM32_I2C
        STM32_MCU --> STM32_SPI1
        STM32_MCU --> STM32_SPI2
        STM32_MCU --> STM32_UART
        STM32_MCU --> STM32_GPIO
    end

    subgraph ESP32_HW[ESP32-WROOM-32 Hardware Configuration]
        ESP32_MCU[Xtensa LX6 Dual-core<br/>Clock: 240MHz<br/>Flash: 4MB<br/>SRAM: 520KB<br/>Power: 160mA WiFi ON]

        ESP32_WiFi[WiFi Radio<br/>802.11 b/g/n<br/>Frequency: 2.4GHz<br/>Range: 50m indoor<br/>Power: -40dBm to +20dBm]

        ESP32_UART_HW[UART1 Interface<br/>GPIO17: TX<br/>GPIO16: RX<br/>Baud: 115200<br/>Format: 8N1]

        ESP32_GPIO[GPIO Configuration<br/>GPIO4: Relay Output<br/>GPIO5: Button1 Input<br/>GPIO16: Button2 Input<br/>GPIO17: Button3 Input<br/>Pull-up: Internal]

        ESP32_LED[Status LEDs<br/>GPIO2: WiFi Status<br/>GPIO15: MQTT Status<br/>Active: HIGH]

        ESP32_MCU --> ESP32_WiFi
        ESP32_MCU --> ESP32_UART_HW
        ESP32_MCU --> ESP32_GPIO
        ESP32_MCU --> ESP32_LED
    end

    subgraph Peripherals_HW[Peripheral Hardware Specifications]
        SHT3X_HW[SHT3X Sensor Module<br/>I2C Address: 0x44<br/>Voltage: 2.4V-5.5V<br/>Accuracy: ±0.2°C, ±2% RH<br/>Response: 8s typical<br/>Power: 1.5uA idle]

        RTC_HW[DS3231 RTC Module<br/>I2C Address: 0x68<br/>Voltage: 3.3V<br/>Accuracy: ±2ppm<br/>Battery: CR2032<br/>Backup: 10 years]

        SD_HW[SD Card Module<br/>Interface: SPI<br/>Format: FAT32<br/>Capacity: 4GB-32GB<br/>Voltage: 3.3V<br/>Speed Class: 10]

        Display_HW[ILI9225 TFT Display<br/>Resolution: 176x220<br/>Size: 2.2 inch<br/>Interface: SPI<br/>Colors: 262K<br/>Backlight: LED]

        Relay_HW[Relay Module<br/>Type: Mechanical<br/>Control: Active HIGH<br/>Voltage: 5V<br/>Current: 10A max<br/>Switching: 250VAC/30VDC]

        Buttons_HW[Tactile Buttons<br/>Type: 4x Push button<br/>Configuration: Active LOW<br/>Debounce: 200ms<br/>Pull-up: 10k ohm internal]
    end

    STM32_I2C <-->|I2C Bus<br/>100kHz| SHT3X_HW
    STM32_I2C <-->|I2C Bus<br/>100kHz| RTC_HW
    STM32_SPI1 <-->|SPI Bus<br/>18MHz| SD_HW
    STM32_SPI2 <-->|SPI Bus<br/>36MHz| Display_HW

    STM32_UART <-->|UART<br/>115200 baud<br/>3.3V logic| ESP32_UART_HW

    ESP32_GPIO -->|Digital Out<br/>3.3V| Relay_HW
    Buttons_HW -->|Digital In<br/>Pull-up| ESP32_GPIO
    Relay_HW -.->|Power Switch<br/>5V DC| STM32_MCU

    ESP32_WiFi <-.->|Wireless<br/>2.4GHz| Network[WiFi Access Point<br/>SSID: Redmi Note 9 Pro<br/>Security: WPA2-PSK]

    style STM32_HW fill:#FFE4B5, color:#000000
    style ESP32_HW fill:#B0E0E6, color:#000000
    style Peripherals_HW fill:#F0E68C, color:#000000
```

## MQTT Protocol and Topic Architecture

```mermaid
graph TB
    subgraph Broker[MQTT Broker: 192.168.1.39:1883]
        direction TB

        subgraph Subscribe[ESP32 Subscribes To]
            T1[Topic: datalogger/stm32/command<br/>QoS: 1<br/>Retained: No<br/>Purpose: STM32 control commands]

            T2[Topic: datalogger/esp32/relay/control<br/>QoS: 1<br/>Retained: No<br/>Purpose: Relay ON/OFF/TOGGLE]

            T3[Topic: datalogger/esp32/system/state<br/>QoS: 1<br/>Retained: No<br/>Purpose: State request trigger]
        end

        subgraph Publish[ESP32 Publishes To]
            T4[Topic: datalogger/stm32/single/data<br/>QoS: 0<br/>Retained: No<br/>Purpose: Single measurements]

            T5[Topic: datalogger/stm32/periodic/data<br/>QoS: 0<br/>Retained: No<br/>Purpose: Periodic measurements]

            T6[Topic: datalogger/esp32/system/state<br/>QoS: 1<br/>Retained: Yes<br/>Purpose: System state sync]
        end
    end

    subgraph Messages[Message Formats and Examples]
        direction TB

        CMD1["STM32 Commands:<br/>• SINGLE<br/>• PERIODIC ON [interval_sec]<br/>• PERIODIC OFF<br/>• SET TIME [timestamp]<br/>• SD CLEAR<br/>• MQTT CONNECTED<br/>• MQTT DISCONNECTED"]

        CMD2["Relay Commands:<br/>• ON<br/>• OFF<br/>• TOGGLE"]

        CMD3["State Request:<br/>• REQUEST"]

        DATA1["Single Measurement JSON:<br/>{<br/>  'mode': 'SINGLE',<br/>  'timestamp': 1698765432,<br/>  'temperature': 25.3,<br/>  'humidity': 65.2<br/>}"]

        DATA2["Periodic Measurement JSON:<br/>{<br/>  'mode': 'PERIODIC',<br/>  'timestamp': 1698765437,<br/>  'temperature': 25.4,<br/>  'humidity': 65.1<br/>}"]

        DATA3["System State JSON:<br/>{<br/>  'device': 'ON',<br/>  'periodic': 'OFF',<br/>  'timestamp': 1698765432,<br/>  'wifi': 'connected',<br/>  'mqtt': 'connected'<br/>}"]
    end

    T1 -.-> CMD1
    T2 -.-> CMD2
    T3 -.-> CMD3
    T4 -.-> DATA1
    T5 -.-> DATA2
    T6 -.-> DATA3

    Web[Web Dashboard<br/>MQTT.js Client] -->|Publish Command| T1
    Web -->|Publish Control| T2
    Web -->|Publish Request| T3

    T4 -->|Subscribe Data| Web
    T5 -->|Subscribe Data| Web
    T6 -->|Subscribe State| Web

    Mobile[Mobile App<br/>MQTT Client] -->|Publish| T1
    Mobile -->|Publish| T2
    T4 -->|Subscribe| Mobile
    T5 -->|Subscribe| Mobile

    style Broker fill:#87CEEB, color:#000000
    style Messages fill:#FFE4B5, color:#000000
    style Web fill:#90EE90, color:#000000
    style Mobile fill:#98FB98, color:#000000
```

## Error Handling and Recovery Strategy

```mermaid
graph TB
    subgraph Errors[Error Types and Detection]
        E1[I2C Sensor Timeout<br/>Detection: HAL timeout<br/>Frequency: Rare]
        E2[RTC Communication Fail<br/>Detection: I2C error<br/>Impact: Timestamp loss]
        E3[SD Card Mount Fail<br/>Detection: FAT init error<br/>Impact: No buffering]
        E4[WiFi Connection Lost<br/>Detection: Event callback<br/>Frequency: Periodic]
        E5[MQTT Broker Disconnect<br/>Detection: Client event<br/>Frequency: Occasional]
        E6[UART Buffer Overflow<br/>Detection: Buffer full<br/>Impact: Data loss]
        E7[JSON Parse Error<br/>Detection: Syntax check<br/>Impact: Invalid data]
    end

    subgraph Recovery[Recovery Strategies and Actions]
        R1[Sensor Error Recovery:<br/>• Return 0.0 values<br/>• Continue operation<br/>• Retry next cycle<br/>• Log to display]

        R2[RTC Error Recovery:<br/>• Use timestamp=0<br/>• Continue operation<br/>• Retry next query<br/>• Display warning]

        R3[SD Card Error Recovery:<br/>• Disable SD buffering<br/>• Switch to MQTT-only<br/>• Log error to display<br/>• Continue operation]

        R4[WiFi Error Recovery:<br/>• Auto-retry 5x @ 2s<br/>• Manual retry @ 5s<br/>• Continue until success<br/>• Update LED status]

        R5[MQTT Error Recovery:<br/>• Exponential backoff<br/>• Min 60s, 2^retry<br/>• Infinite retries<br/>• Buffer data to SD]

        R6[UART Overflow Recovery:<br/>• Discard oldest data<br/>• Log overflow event<br/>• Continue reception<br/>• Alert via MQTT]

        R7[JSON Error Recovery:<br/>• Log parse error<br/>• Discard message<br/>• Continue processing<br/>• Increment error count]
    end

    subgraph Monitoring[Error Monitoring and Reporting]
        M1[Display Status<br/>• Error messages<br/>• Connection state<br/>• Buffer status]

        M2[LED Indicators<br/>• WiFi: Blink/Solid<br/>• MQTT: Blink/Solid<br/>• Error: Fast blink]

        M3[MQTT Error Reports<br/>• Error topic pub<br/>• Error counters<br/>• System health]

        M4[Buffered Error Data<br/>• Log to SD card<br/>• Timestamp errors<br/>• Persist events]
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
    R3 --> M4

    style Errors fill:#FF6B6B, color:#000000
    style Recovery fill:#90EE90, color:#000000
    style Monitoring fill:#FFD700, color:#000000
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

    style Device fill:#FFE4B5, color:#000000
    style STM32 fill:#FFB6C1, color:#000000
    style ESP32 fill:#B0E0E6, color:#000000
    style Sensors fill:#F0E68C, color:#000000
    style Actuators fill:#DDA0DD, color:#000000
    style Network fill:#98FB98, color:#000000
    style Broker fill:#87CEEB, color:#000000
    style Clients fill:#FFDAB9, color:#000000
```

## System Timing and Synchronization Diagram

```mermaid
gantt
    title System Timing and Event Sequence
    dateFormat X
    axisFormat %L ms

    section STM32 Boot
    Power ON              :milestone, 0, 0
    System Init           :0, 500
    I2C Init              :100, 150
    SPI Init              :150, 200
    UART Init             :200, 250
    Sensor Init           :250, 350
    SD Card Mount         :350, 450
    Display Init          :450, 500
    Ready State           :milestone, 500, 500

    section STM32 Single Measure
    Receive SINGLE cmd    :milestone, 600, 600
    SHT3X Measure 15ms    :600, 615
    I2C Read              :615, 620
    RTC Get Time          :620, 625
    Format JSON           :625, 630
    UART Transmit 10ms    :630, 640
    Complete              :milestone, 640, 640

    section ESP32 Boot
    Power ON              :milestone, 0, 0
    System Init           :0, 1000
    FreeRTOS Start        :100, 200
    UART Init             :200, 300
    GPIO Init             :300, 400
    WiFi Init             :400, 500
    WiFi Connect Start    :1000, 1100
    WiFi Connecting       :1100, 11000
    WiFi Connected        :milestone, 11000, 11000
    Wait 4s Stable        :11000, 15000
    MQTT Connect Start    :15000, 15100
    MQTT Connecting       :15100, 17000
    MQTT Connected        :milestone, 17000, 17000

    section ESP32 Process
    UART RX Interrupt     :milestone, 640, 640
    Read from Buffer      :640, 642
    JSON Parse            :642, 645
    Validate Data         :645, 647
    MQTT Publish          :647, 652
    Update State          :652, 655
    Display Update        :655, 660

    section Periodic Mode
    Start Periodic        :milestone, 20000, 20000
    Measure Interval 5s   :20000, 25000
    Next Measure          :25000, 30000
    Next Measure          :30000, 35000
    Next Measure          :35000, 40000

    section Delays Reference
    WiFi Retry 2s         :milestone, 2000
    Manual Retry 5s       :milestone, 5000
    MQTT Backoff 1s       :milestone, 1000
    MQTT Backoff 2s       :milestone, 2000
    MQTT Backoff 4s       :milestone, 4000
    Button Debounce 200ms :milestone, 200
```

## System Performance Characteristics

### Latency and Throughput

- **End-to-End Latency**: <100ms (Sensor → Web)
  - I2C Read: ~20ms
  - JSON Format: ~5ms
  - UART Transfer: ~10ms
  - MQTT Publish: ~30ms
  - Network propagation: ~30ms
- **Measurement Rate**: 0.2 Hz to 1 Hz (5s-60s configurable)
- **UART Throughput**: ~11.5 KB/s (115200 baud, 8N1)
- **SD Write Speed**: ~100 KB/s (18MHz SPI, FAT32)
- **Buffer Capacity**: 204,800 records (~16MB, >14 days @ 5s interval)

### Reliability and Availability

- **MTBF**: >10,000 hours (estimated)
- **Data Integrity**: CRC-16 for sensor data
- **Persistence**: Non-volatile SD card storage
- **Network Uptime**: Auto-reconnect with exponential backoff
- **Power Resilience**: Relay-based power management
- **Clock Accuracy**: ±2ppm (DS3231 with battery backup)

### Power Consumption Profile

| Component        | Active | Idle   | Sleep | Notes           |
| ---------------- | ------ | ------ | ----- | --------------- |
| STM32F103        | 20mA   | 20mA   | N/A   | Always active   |
| ESP32 WiFi ON    | 160mA  | -      | N/A   | TX/RX           |
| ESP32 WiFi OFF   | 80mA   | -      | N/A   | Processing only |
| SHT3X            | 0.3mA  | 1.5μA  | -     | During measure  |
| DS3231           | 0.2mA  | 0.1μA  | -     | Battery backup  |
| SD Card          | 100mA  | 0.2mA  | -     | During write    |
| TFT Display      | 50mA   | 20mA   | -     | Backlight on    |
| **Total System** | ~200mA | ~120mA | N/A   | @ 5V DC         |

### Scalability Considerations

- **Current**: Single device deployment
- **Future**: Multi-device MQTT hierarchy ready
- **Sensor Expansion**: I2C bus supports 127 addresses
- **Storage Scaling**: SD card up to 32GB (FAT32 limit)
- **Network Scaling**: MQTT broker can handle 100+ devices
- **Cloud Integration**: MQTT bridge to AWS IoT/Azure IoT ready

### Security Features

- **Authentication**: MQTT username/password (admin/password)
- **Encryption**: WPA2-PSK for WiFi network
- **Physical Security**: Relay power control for STM32
- **Data Privacy**: Local network only (no internet exposure)
- **Future Enhancements**: TLS/SSL for MQTT, certificate-based auth
