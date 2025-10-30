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
    subgraph STM32_System["STM32 System - Layered Architecture"]
        direction TB

        subgraph Application_Layer["Application Layer"]
            Main_Control[Main Control<br/>System Orchestration<br/>Event Loop]
        end

        subgraph Business_Logic["Business Logic Layer"]
            Data_Manager[Data Manager<br/>Single/Periodic Mode<br/>State Control]
            SD_Manager[SD Card Manager<br/>Circular Buffer<br/>204800 Records]
            Display_Manager[Display Manager<br/>Status Rendering<br/>UI Update]
        end

        subgraph Protocol_Layer["Protocol Layer"]
            UART_Protocol[UART Protocol Handler<br/>Ring Buffer 512B<br/>JSON Processing]
            Command_Parser[Command Parser<br/>8 Commands<br/>Validation]
        end

        subgraph Driver_Layer["Device Driver Layer"]
            SHT3X_Driver[SHT3X Driver<br/>Temperature Humidity<br/>Single/Periodic Mode]
            DS3231_Driver[DS3231 RTC Driver<br/>Time Management<br/>Battery Backup]
            SD_Card_Driver[SD Card Driver<br/>FAT32 Filesystem<br/>SPI Interface]
            Display_Driver[ILI9225 Driver<br/>176x220 TFT<br/>SPI Interface]
        end

        subgraph HAL_Layer["Hardware Abstraction Layer"]
            I2C_HAL[I2C HAL<br/>100kHz<br/>PB6/PB7]
            SPI1_HAL[SPI1 HAL<br/>18MHz<br/>PA4/5/6/7]
            SPI2_HAL[SPI2 HAL<br/>36MHz<br/>PB12/13/14/15]
            UART_HAL[UART HAL<br/>115200 baud<br/>PA9/PA10]
            GPIO_HAL[GPIO HAL<br/>Digital IO<br/>PC13/PA8/11/12]
        end

        Main_Control --> Data_Manager
        Main_Control --> SD_Manager
        Main_Control --> Display_Manager
        Main_Control --> UART_Protocol

        Data_Manager --> SHT3X_Driver
        Data_Manager --> DS3231_Driver
        Data_Manager --> SD_Manager

        SD_Manager --> SD_Card_Driver
        Display_Manager --> Display_Driver

        UART_Protocol --> Command_Parser
        Command_Parser --> Data_Manager
        Command_Parser --> SHT3X_Driver

        SHT3X_Driver --> I2C_HAL
        DS3231_Driver --> I2C_HAL
        SD_Card_Driver --> SPI1_HAL
        Display_Driver --> SPI2_HAL
        Display_Driver --> GPIO_HAL
        UART_Protocol --> UART_HAL
    end

    style Application_Layer fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000
    style Business_Logic fill:#BBDEFB,stroke:#1976D2,stroke-width:3px,color:#000
    style Protocol_Layer fill:#FFF9C4,stroke:#F57F17,stroke-width:3px,color:#000
    style Driver_Layer fill:#FFE0B2,stroke:#E65100,stroke-width:3px,color:#000
    style HAL_Layer fill:#F8BBD0,stroke:#C2185B,stroke-width:3px,color:#000
```

## Component Diagram - ESP32 Modules

```mermaid
graph TB
    subgraph ESP32_System["ESP32 System - Layered Architecture"]
        direction TB

        subgraph Application_Layer["Application Layer"]
            Main_Coordinator[Main Coordinator<br/>FreeRTOS Tasks<br/>System Control]
        end

        subgraph Business_Logic["Business Logic Layer"]
            State_Manager[State Manager<br/>System State Sync<br/>Retained Messages]
            Event_Coordinator[Event Coordinator<br/>Callback Dispatch<br/>Event Queue]
        end

        subgraph Protocol_Layer["Protocol Layer"]
            JSON_Parser[JSON Parser<br/>Sensor Data Validation<br/>Mode Detection]
            MQTT_Protocol[MQTT Protocol<br/>Topic Management<br/>QoS Handling]
            UART_Protocol[UART Protocol<br/>STM32 Communication<br/>Command Format]
        end

        subgraph Service_Layer["Service Layer"]
            WiFi_Manager[WiFi Manager<br/>Connection Management<br/>Auto Reconnect]
            MQTT_Handler[MQTT Handler<br/>Pub/Sub Management<br/>Reconnect Logic]
            STM32_Interface[STM32 Interface<br/>UART Communication<br/>Ring Buffer 1024B]
            Relay_Controller[Relay Controller<br/>Power Management<br/>State Callback]
            Button_Manager[Button Manager<br/>Debounce 200ms<br/>4x GPIO Input]
        end

        subgraph Driver_Layer["ESP-IDF Framework Layer"]
            WiFi_Stack[WiFi Stack<br/>802.11 b/g/n<br/>STA Mode]
            TCP_IP_Stack[TCP/IP Stack<br/>LwIP<br/>Socket API]
            MQTT_Client[MQTT Client<br/>esp-mqtt<br/>QoS 0/1]
            UART_Driver[UART Driver<br/>esp_uart<br/>DMA Support]
            GPIO_Driver[GPIO Driver<br/>esp_gpio<br/>Interrupt]
        end

        Main_Coordinator --> WiFi_Manager
        Main_Coordinator --> MQTT_Handler
        Main_Coordinator --> STM32_Interface
        Main_Coordinator --> Relay_Controller
        Main_Coordinator --> Button_Manager
        Main_Coordinator --> State_Manager
        Main_Coordinator --> Event_Coordinator

        State_Manager --> MQTT_Protocol
        Event_Coordinator --> State_Manager

        WiFi_Manager --> WiFi_Stack
        MQTT_Handler --> MQTT_Protocol
        MQTT_Handler --> MQTT_Client
        STM32_Interface --> UART_Protocol
        STM32_Interface --> JSON_Parser
        STM32_Interface --> UART_Driver
        Relay_Controller --> GPIO_Driver
        Button_Manager --> GPIO_Driver

        MQTT_Protocol --> State_Manager
        UART_Protocol --> JSON_Parser

        WiFi_Stack --> TCP_IP_Stack
        MQTT_Client --> TCP_IP_Stack
    end

    style Application_Layer fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000
    style Business_Logic fill:#BBDEFB,stroke:#1976D2,stroke-width:3px,color:#000
    style Protocol_Layer fill:#FFF9C4,stroke:#F57F17,stroke-width:3px,color:#000
    style Service_Layer fill:#FFE0B2,stroke:#E65100,stroke-width:3px,color:#000
    style Driver_Layer fill:#F8BBD0,stroke:#C2185B,stroke-width:3px,color:#000
```

## Package Diagram - Overall System Structure

```mermaid
graph TB
    subgraph System_Architecture["Complete Firmware System Architecture"]
        direction TB

        subgraph STM32_Firmware["STM32 Firmware Package"]
            direction TB

            subgraph STM32_App["Application Module"]
                STM32_Main[main.c<br/>system_init.c<br/>System Control]
            end

            subgraph STM32_Business["Business Logic Module"]
                STM32_DataMgr[data_manager.c/h<br/>sd_card_manager.c/h<br/>display_manager.c/h]
            end

            subgraph STM32_Protocol["Protocol Module"]
                STM32_UART[uart_handler.c/h<br/>command_parser.c/h<br/>ring_buffer.c/h]
            end

            subgraph STM32_Drivers["Device Drivers Module"]
                STM32_DevDrivers[sht3x.c/h<br/>ds3231.c/h<br/>sd_card.c/h<br/>ili9225.c/h]
            end

            subgraph STM32_HAL["HAL Module"]
                STM32_HAL_Drivers[stm32f1xx_hal_i2c.c<br/>stm32f1xx_hal_spi.c<br/>stm32f1xx_hal_uart.c<br/>stm32f1xx_hal_gpio.c]
            end

            STM32_Main --> STM32_Business
            STM32_Main --> STM32_Protocol
            STM32_Business --> STM32_Drivers
            STM32_Protocol --> STM32_Drivers
            STM32_Drivers --> STM32_HAL
        end

        subgraph ESP32_Firmware["ESP32 Firmware Package"]
            direction TB

            subgraph ESP32_App["Application Module"]
                ESP32_Main[app_main.c<br/>system_init.c<br/>Main Coordinator]
            end

            subgraph ESP32_Business["Business Logic Module"]
                ESP32_State[state_manager.c/h<br/>event_coordinator.c/h]
            end

            subgraph ESP32_Protocol["Protocol Module"]
                ESP32_Proto[json_parser.c/h<br/>mqtt_protocol.c/h<br/>uart_protocol.c/h]
            end

            subgraph ESP32_Services["Service Module"]
                ESP32_Svc[wifi_manager.c/h<br/>mqtt_handler.c/h<br/>stm32_uart.c/h<br/>relay_control.c/h<br/>button_handler.c/h]
            end

            subgraph ESP32_IDF["ESP-IDF Framework"]
                ESP32_Framework[esp_wifi.h<br/>esp_mqtt_client.h<br/>driver/uart.h<br/>driver/gpio.h]
            end

            ESP32_Main --> ESP32_Business
            ESP32_Main --> ESP32_Services
            ESP32_Business --> ESP32_Protocol
            ESP32_Services --> ESP32_Protocol
            ESP32_Services --> ESP32_IDF
        end

        subgraph Shared_Definitions["Shared Protocol Package"]
            direction TB

            subgraph Protocol_Spec["Protocol Specification"]
                JSON_Format[JSON Format<br/>Message Structure<br/>Field Definitions]
            end

            subgraph Command_Spec["Command Specification"]
                Commands[UART Commands<br/>MQTT Topics<br/>Command Syntax]
            end

            subgraph Data_Spec["Data Structure Specification"]
                DataStructs[sensor_data_t<br/>timestamp_t<br/>state_t]
            end

            subgraph Config_Spec["Configuration Specification"]
                Configs[GPIO Pins<br/>MQTT Topics<br/>System Constants]
            end
        end

        STM32_Protocol -.->|Uses| JSON_Format
        STM32_Protocol -.->|Uses| Commands
        STM32_Business -.->|Uses| DataStructs
        STM32_HAL -.->|Uses| Configs

        ESP32_Proto -.->|Uses| JSON_Format
        ESP32_Proto -.->|Uses| Commands
        ESP32_Business -.->|Uses| DataStructs
        ESP32_IDF -.->|Uses| Configs

        STM32_UART <-->|UART 115200<br/>JSON Protocol| ESP32_Svc
    end

    style STM32_Firmware fill:#FFE0B2,stroke:#E65100,stroke-width:4px,color:#000
    style ESP32_Firmware fill:#BBDEFB,stroke:#1976D2,stroke-width:4px,color:#000
    style Shared_Definitions fill:#FFF9C4,stroke:#F57F17,stroke-width:4px,color:#000

    style STM32_App fill:#FFCCBC,stroke:#D84315,stroke-width:2px,color:#000
    style STM32_Business fill:#FFB74D,stroke:#F57C00,stroke-width:2px,color:#000
    style STM32_Protocol fill:#FFA726,stroke:#EF6C00,stroke-width:2px,color:#000
    style STM32_Drivers fill:#FF9800,stroke:#E65100,stroke-width:2px,color:#000
    style STM32_HAL fill:#FB8C00,stroke:#D84315,stroke-width:2px,color:#000

    style ESP32_App fill:#90CAF9,stroke:#1565C0,stroke-width:2px,color:#000
    style ESP32_Business fill:#64B5F6,stroke:#1976D2,stroke-width:2px,color:#000
    style ESP32_Protocol fill:#42A5F5,stroke:#1E88E5,stroke-width:2px,color:#000
    style ESP32_Services fill:#2196F3,stroke:#1976D2,stroke-width:2px,color:#000
    style ESP32_IDF fill:#1E88E5,stroke:#1565C0,stroke-width:2px,color:#000

    style Protocol_Spec fill:#FFF59D,stroke:#F9A825,stroke-width:2px,color:#000
    style Command_Spec fill:#FFEE58,stroke:#F57F17,stroke-width:2px,color:#000
    style Data_Spec fill:#FFEB3B,stroke:#F57F17,stroke-width:2px,color:#000
    style Config_Spec fill:#FDD835,stroke:#F57F17,stroke-width:2px,color:#000
```

## Data Flow Architecture

```mermaid
graph LR
    subgraph Input_Sources["Data Input Layer"]
        direction TB

        subgraph Physical_Sensors["Physical Sensors"]
            Sensor[SHT3X Sensor<br/>Temperature Humidity<br/>I2C Read 15ms]
            Clock[DS3231 RTC<br/>Timestamp Source<br/>I2C Read 5ms]
        end

        subgraph External_Commands["External Commands"]
            MQTT_Cmd[MQTT Commands<br/>Remote Control<br/>QoS 1]
            Button_Input[Button Input<br/>Local Control<br/>Debounced 200ms]
        end
    end

    subgraph STM32_Layer["STM32 Processing Layer"]
        direction TB

        subgraph Data_Acquisition["Data Acquisition"]
            I2C_Read[I2C Bus Read<br/>100kHz Bus<br/>20ms Total Time]
            Format_Data[JSON Formatter<br/>Add Timestamp<br/>5ms Processing]
        end

        subgraph Routing_Logic["Data Routing Logic"]
            Connection_Check{MQTT Status<br/>Connected?}
            Buffer_Write[SD Buffer Write<br/>Circular Buffer<br/>10ms Write Time]
            UART_Send[UART Transmit<br/>115200 baud<br/>JSON Format]
        end
    end

    subgraph ESP32_Layer["ESP32 Processing Layer"]
        direction TB

        subgraph Reception["Data Reception"]
            UART_Receive[UART Receive<br/>Ring Buffer 1024B<br/>DMA Transfer]
            Parse_JSON[JSON Parser<br/>Mode Detection<br/>Validation]
        end

        subgraph Publishing["Data Publishing"]
            Publish_MQTT[MQTT Publish<br/>QoS 0 for Data<br/>QoS 1 for State]
        end

        subgraph Command_Processing["Command Processing"]
            Route_Command[Command Router<br/>Topic Based<br/>Dispatch Logic]
            Update_State[State Manager<br/>System State<br/>Retained Msg]
        end
    end

    subgraph Output_Layer["Data Output Layer"]
        direction TB

        subgraph Cloud_Output["Cloud Services"]
            Broker[MQTT Broker<br/>192.168.1.39<br/>Port 1883]
        end

        subgraph Local_Output["Local Outputs"]
            TFT_Display[TFT Display<br/>ILI9225 176x220<br/>Status Info]
            Status_LED[Status LEDs<br/>WiFi MQTT<br/>Connection State]
            Power_Relay[Relay Output<br/>Device Control<br/>Power Switch]
        end
    end

    Sensor --> I2C_Read
    Clock --> I2C_Read
    I2C_Read --> Format_Data
    Format_Data --> Connection_Check
    Connection_Check -->|Connected| UART_Send
    Connection_Check -->|Disconnected| Buffer_Write

    UART_Send --> UART_Receive
    Buffer_Write -.->|On Reconnect<br/>Replay Data| UART_Receive

    UART_Receive --> Parse_JSON
    Parse_JSON --> Publish_MQTT
    Publish_MQTT --> Broker

    MQTT_Cmd --> Route_Command
    Button_Input --> Route_Command
    Route_Command --> Update_State

    Update_State --> Publish_MQTT
    Update_State --> TFT_Display
    Update_State --> Status_LED
    Route_Command --> Power_Relay
    Format_Data --> TFT_Display

    style Input_Sources fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000
    style STM32_Layer fill:#FFE0B2,stroke:#E65100,stroke-width:3px,color:#000
    style ESP32_Layer fill:#BBDEFB,stroke:#1976D2,stroke-width:3px,color:#000
    style Output_Layer fill:#E1BEE7,stroke:#7B1FA2,stroke-width:3px,color:#000

    style Physical_Sensors fill:#A5D6A7,stroke:#388E3C,stroke-width:2px,color:#000
    style External_Commands fill:#81C784,stroke:#388E3C,stroke-width:2px,color:#000
    style Data_Acquisition fill:#FFCC80,stroke:#E65100,stroke-width:2px,color:#000
    style Routing_Logic fill:#FFB74D,stroke:#E65100,stroke-width:2px,color:#000
    style Reception fill:#90CAF9,stroke:#1976D2,stroke-width:2px,color:#000
    style Publishing fill:#64B5F6,stroke:#1976D2,stroke-width:2px,color:#000
    style Command_Processing fill:#42A5F5,stroke:#1976D2,stroke-width:2px,color:#000
    style Cloud_Output fill:#CE93D8,stroke:#7B1FA2,stroke-width:2px,color:#000
    style Local_Output fill:#BA68C8,stroke:#7B1FA2,stroke-width:2px,color:#000
```

## Hardware Configuration and Pinout

```mermaid
graph TB
    subgraph Hardware_System["Hardware System Configuration"]
        direction TB

        subgraph STM32_Hardware["STM32F103C8T6 Hardware Layer"]
            direction TB

            subgraph STM32_Core["Core Specifications"]
                STM32_Processor[Processor<br/>ARM Cortex-M3<br/>Clock 72MHz<br/>Flash 64KB<br/>RAM 20KB]
            end

            subgraph STM32_Interfaces["Communication Interfaces"]
                STM32_I2C_Bus[I2C1 Bus<br/>Pins PB6 SCL PB7 SDA<br/>Speed 100kHz<br/>Pull-up 4.7k ohm]

                STM32_SPI1_Bus[SPI1 Bus<br/>Pins PA5 SCK PA6 MISO<br/>PA7 MOSI PA4 CS<br/>Speed 18MHz]

                STM32_SPI2_Bus[SPI2 Bus<br/>Pins PB13 SCK PB14 MISO<br/>PB15 MOSI PB12 CS<br/>Speed 36MHz]

                STM32_UART_Port[UART1 Port<br/>Pins PA9 TX PA10 RX<br/>Baud 115200<br/>Format 8N1]
            end

            subgraph STM32_GPIO["GPIO Configuration"]
                STM32_Control[Control Pins<br/>LED PC13<br/>Display DC PA8<br/>Display RS PA11<br/>Display BL PA12]
            end

            STM32_Processor --> STM32_I2C_Bus
            STM32_Processor --> STM32_SPI1_Bus
            STM32_Processor --> STM32_SPI2_Bus
            STM32_Processor --> STM32_UART_Port
            STM32_Processor --> STM32_Control
        end

        subgraph ESP32_Hardware["ESP32-WROOM-32 Hardware Layer"]
            direction TB

            subgraph ESP32_Core["Core Specifications"]
                ESP32_Processor[Processor<br/>Xtensa LX6 Dual-core<br/>Clock 240MHz<br/>Flash 4MB<br/>SRAM 520KB]
            end

            subgraph ESP32_Wireless["Wireless Interface"]
                ESP32_WiFi_Radio[WiFi Radio<br/>Standard 802.11 b/g/n<br/>Frequency 2.4GHz<br/>Range Indoor 50m<br/>Power Configurable]
            end

            subgraph ESP32_Interfaces["Communication Interfaces"]
                ESP32_UART_Port[UART1 Port<br/>Pins GPIO17 TX GPIO16 RX<br/>Baud 115200<br/>Format 8N1]
            end

            subgraph ESP32_GPIO["GPIO Configuration"]
                ESP32_Outputs[Output Pins<br/>GPIO4 Relay Control<br/>GPIO2 WiFi LED<br/>GPIO15 MQTT LED<br/>Active HIGH]

                ESP32_Inputs[Input Pins<br/>GPIO5 Button1<br/>GPIO16 Button2<br/>GPIO17 Button3<br/>GPIO4 Button4<br/>Pull-up Internal]
            end

            ESP32_Processor --> ESP32_WiFi_Radio
            ESP32_Processor --> ESP32_UART_Port
            ESP32_Processor --> ESP32_Outputs
            ESP32_Processor --> ESP32_Inputs
        end

        subgraph Peripheral_Hardware["Peripheral Hardware Layer"]
            direction TB

            subgraph I2C_Peripherals["I2C Peripheral Devices"]
                SHT3X_Module[SHT3X Sensor<br/>Address 0x44<br/>Voltage 2.4V to 5.5V<br/>Accuracy Temp 0.2C Hum 2 percent<br/>Power 1.5uA idle]

                RTC_Module[DS3231 RTC<br/>Address 0x68<br/>Voltage 3.3V<br/>Accuracy 2ppm<br/>Battery CR2032<br/>Backup 10 years]
            end

            subgraph SPI_Peripherals["SPI Peripheral Devices"]
                SD_Module[SD Card Module<br/>Interface SPI<br/>Format FAT32<br/>Capacity 4GB to 32GB<br/>Voltage 3.3V<br/>Speed Class 10]

                Display_Module[ILI9225 Display<br/>Resolution 176x220<br/>Size 2.2 inch<br/>Interface SPI<br/>Colors 262K<br/>Backlight LED]
            end

            subgraph GPIO_Peripherals["GPIO Peripheral Devices"]
                Relay_Module[Relay Module<br/>Type Mechanical<br/>Control Active HIGH<br/>Voltage 5V<br/>Current 10A max<br/>Switching 250VAC 30VDC]

                Button_Module[Button Array<br/>Type 4x Tactile<br/>Configuration Active LOW<br/>Debounce 200ms<br/>Pull-up 10k ohm]
            end
        end

        subgraph Physical_Connections["Physical Connection Layer"]
            direction LR

            I2C_Connection[I2C Bus Connection<br/>Speed 100kHz<br/>Protocol I2C Standard]

            SPI_Connection[SPI Bus Connection<br/>Speed 18MHz and 36MHz<br/>Protocol SPI Mode 0]

            UART_Connection[UART Connection<br/>Speed 115200 baud<br/>Protocol 8N1<br/>Voltage 3.3V Logic]

            GPIO_Connection[GPIO Connection<br/>Digital Signals<br/>3.3V Logic Level]

            Power_Connection[Power Connection<br/>Voltage 5V DC<br/>Control via Relay]
        end
    end

    STM32_I2C_Bus <--> I2C_Connection
    I2C_Connection <--> SHT3X_Module
    I2C_Connection <--> RTC_Module

    STM32_SPI1_Bus <--> SPI_Connection
    SPI_Connection <--> SD_Module

    STM32_SPI2_Bus <--> SPI_Connection
    SPI_Connection <--> Display_Module

    STM32_UART_Port <--> UART_Connection
    UART_Connection <--> ESP32_UART_Port

    ESP32_Outputs --> GPIO_Connection
    GPIO_Connection --> Relay_Module

    Button_Module --> GPIO_Connection
    GPIO_Connection --> ESP32_Inputs

    Relay_Module -.-> Power_Connection
    Power_Connection -.-> STM32_Processor

    ESP32_WiFi_Radio <-.->|Wireless Protocol<br/>2.4GHz Band| Network_AP[WiFi Access Point<br/>SSID Configuration<br/>Security WPA2-PSK]

    style Hardware_System fill:#F5F5F5,stroke:#424242,stroke-width:2px,color:#000
    style STM32_Hardware fill:#FFE0B2,stroke:#E65100,stroke-width:3px,color:#000
    style ESP32_Hardware fill:#BBDEFB,stroke:#1976D2,stroke-width:3px,color:#000
    style Peripheral_Hardware fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000
    style Physical_Connections fill:#FFF9C4,stroke:#F57F17,stroke-width:3px,color:#000

    style STM32_Core fill:#FFCCBC,stroke:#D84315,stroke-width:2px,color:#000
    style STM32_Interfaces fill:#FFB74D,stroke:#F57C00,stroke-width:2px,color:#000
    style STM32_GPIO fill:#FFA726,stroke:#EF6C00,stroke-width:2px,color:#000

    style ESP32_Core fill:#90CAF9,stroke:#1565C0,stroke-width:2px,color:#000
    style ESP32_Wireless fill:#64B5F6,stroke:#1976D2,stroke-width:2px,color:#000
    style ESP32_Interfaces fill:#42A5F5,stroke:#1E88E5,stroke-width:2px,color:#000
    style ESP32_GPIO fill:#2196F3,stroke:#1976D2,stroke-width:2px,color:#000

    style I2C_Peripherals fill:#A5D6A7,stroke:#388E3C,stroke-width:2px,color:#000
    style SPI_Peripherals fill:#81C784,stroke:#388E3C,stroke-width:2px,color:#000
    style GPIO_Peripherals fill:#66BB6A,stroke:#388E3C,stroke-width:2px,color:#000
```

## MQTT Protocol and Topic Architecture

```mermaid
graph TB
    subgraph MQTT_Architecture["MQTT Protocol Architecture"]
        direction TB

        subgraph Broker_Layer["Message Broker Layer"]
            Broker_Server[MQTT Broker Server<br/>Protocol MQTT v5.0<br/>Address IP Address Port 1883<br/>Server Mosquitto<br/>Authentication Username Password]
        end

        subgraph Topic_Structure["Topic Hierarchy Structure"]
            direction TB

            subgraph Command_Topics["Command Topics - ESP32 Subscribes"]
                Topic_STM32_Cmd[Topic Path<br/>datalogger/stm32/command<br/>QoS Level 1<br/>Retained No<br/>Purpose STM32 Control Commands]

                Topic_Relay_Cmd[Topic Path<br/>datalogger/esp32/relay/control<br/>QoS Level 1<br/>Retained No<br/>Purpose Relay ON OFF TOGGLE]

                Topic_State_Req[Topic Path<br/>datalogger/esp32/system/state<br/>QoS Level 1<br/>Retained No<br/>Purpose State Request Trigger]
            end

            subgraph Data_Topics["Data Topics - ESP32 Publishes"]
                Topic_Single_Data[Topic Path<br/>datalogger/stm32/single/data<br/>QoS Level 0<br/>Retained No<br/>Purpose Single Measurements]

                Topic_Periodic_Data[Topic Path<br/>datalogger/stm32/periodic/data<br/>QoS Level 0<br/>Retained No<br/>Purpose Periodic Measurements]

                Topic_System_State[Topic Path<br/>datalogger/esp32/system/state<br/>QoS Level 1<br/>Retained Yes<br/>Purpose System State Sync]
            end
        end

        subgraph Message_Formats["Message Format Specifications"]
            direction TB

            subgraph Command_Messages["Command Message Formats"]
                STM32_Commands[STM32 Command Set<br/>SINGLE<br/>PERIODIC ON interval_seconds<br/>PERIODIC OFF<br/>SET TIME timestamp_value<br/>SD CLEAR<br/>MQTT CONNECTED<br/>MQTT DISCONNECTED]

                Relay_Commands[Relay Command Set<br/>ON<br/>OFF<br/>TOGGLE]

                State_Request[State Request Format<br/>REQUEST]
            end

            subgraph Data_Messages["Data Message Formats JSON"]
                Single_Format[Single Measurement<br/>mode SINGLE<br/>timestamp unix_time<br/>temperature celsius_value<br/>humidity percent_value]

                Periodic_Format[Periodic Measurement<br/>mode PERIODIC<br/>timestamp unix_time<br/>temperature celsius_value<br/>humidity percent_value]

                State_Format[System State<br/>device ON or OFF<br/>periodic ON or OFF<br/>timestamp unix_time<br/>wifi connected or disconnected<br/>mqtt connected or disconnected]
            end
        end

        subgraph Client_Layer["Client Application Layer"]
            direction LR

            Web_Client[Web Dashboard<br/>Technology MQTT.js<br/>Protocol WebSocket<br/>Function Monitoring Control]

            Mobile_Client[Mobile Application<br/>Technology Native MQTT<br/>Protocol TCP<br/>Function Remote Monitoring]

            ESP32_Client[ESP32 Device<br/>Technology esp-mqtt<br/>Protocol TCP<br/>Function IoT Gateway]
        end
    end

    Broker_Server --> Topic_STM32_Cmd
    Broker_Server --> Topic_Relay_Cmd
    Broker_Server --> Topic_State_Req
    Broker_Server --> Topic_Single_Data
    Broker_Server --> Topic_Periodic_Data
    Broker_Server --> Topic_System_State

    Topic_STM32_Cmd -.->|Message Format| STM32_Commands
    Topic_Relay_Cmd -.->|Message Format| Relay_Commands
    Topic_State_Req -.->|Message Format| State_Request
    Topic_Single_Data -.->|Message Format| Single_Format
    Topic_Periodic_Data -.->|Message Format| Periodic_Format
    Topic_System_State -.->|Message Format| State_Format

    Web_Client -->|Publish Commands| Broker_Server
    Web_Client <-->|Subscribe Data| Broker_Server

    Mobile_Client -->|Publish Commands| Broker_Server
    Mobile_Client <-->|Subscribe Data| Broker_Server

    ESP32_Client <-->|Pub/Sub All Topics| Broker_Server

    style MQTT_Architecture fill:#F5F5F5,stroke:#424242,stroke-width:2px,color:#000
    style Broker_Layer fill:#B39DDB,stroke:#5E35B1,stroke-width:3px,color:#000
    style Topic_Structure fill:#90CAF9,stroke:#1976D2,stroke-width:3px,color:#000
    style Message_Formats fill:#FFF59D,stroke:#F57F17,stroke-width:3px,color:#000
    style Client_Layer fill:#A5D6A7,stroke:#388E3C,stroke-width:3px,color:#000

    style Command_Topics fill:#64B5F6,stroke:#1976D2,stroke-width:2px,color:#000
    style Data_Topics fill:#42A5F5,stroke:#1E88E5,stroke-width:2px,color:#000
    style Command_Messages fill:#FFEE58,stroke:#F9A825,stroke-width:2px,color:#000
    style Data_Messages fill:#FDD835,stroke:#F57F17,stroke-width:2px,color:#000
```

## Error Handling and Recovery Strategy

```mermaid
graph TB
    subgraph Error_Management["Error Management System"]
        direction TB

        subgraph Error_Detection["Error Detection Layer"]
            direction TB

            subgraph Hardware_Errors["Hardware Error Types"]
                I2C_Error[I2C Communication Error<br/>Detection HAL Timeout<br/>Frequency Rare<br/>Impact Sensor Read Fail]

                RTC_Error[RTC Communication Error<br/>Detection I2C Bus Error<br/>Frequency Rare<br/>Impact Timestamp Loss]

                SD_Error[SD Card Error<br/>Detection FAT Init Fail<br/>Frequency Occasional<br/>Impact No Buffering]
            end

            subgraph Network_Errors["Network Error Types"]
                WiFi_Error[WiFi Connection Error<br/>Detection Event Callback<br/>Frequency Periodic<br/>Impact No Network Access]

                MQTT_Error[MQTT Broker Error<br/>Detection Client Event<br/>Frequency Occasional<br/>Impact No Cloud Sync]
            end

            subgraph Software_Errors["Software Error Types"]
                Buffer_Error[Buffer Overflow Error<br/>Detection Buffer Full Check<br/>Frequency Rare<br/>Impact Data Loss]

                Parse_Error[JSON Parse Error<br/>Detection Syntax Validation<br/>Frequency Occasional<br/>Impact Invalid Data]
            end
        end

        subgraph Recovery_Strategies["Recovery Strategy Layer"]
            direction TB

            subgraph Hardware_Recovery["Hardware Error Recovery"]
                Sensor_Recovery[Sensor Error Strategy<br/>Action Return Zero Values<br/>Action Continue Operation<br/>Action Auto Retry Next Cycle<br/>Action Log to Display]

                RTC_Recovery[RTC Error Strategy<br/>Action Use Timestamp Zero<br/>Action Continue Operation<br/>Action Auto Retry Query<br/>Action Display Warning]

                SD_Recovery[SD Error Strategy<br/>Action Disable Buffering<br/>Action MQTT Only Mode<br/>Action Log Error Display<br/>Action Continue Operation]
            end

            subgraph Network_Recovery["Network Error Recovery"]
                WiFi_Recovery[WiFi Error Strategy<br/>Action Auto Retry 5 Times<br/>Action Retry Interval 2 Seconds<br/>Action Manual Retry 5 Seconds<br/>Action Update LED Status]

                MQTT_Recovery[MQTT Error Strategy<br/>Action Exponential Backoff<br/>Action Max Delay 60 Seconds<br/>Action Infinite Retries<br/>Action Buffer to SD Card]
            end

            subgraph Software_Recovery["Software Error Recovery"]
                Buffer_Recovery[Buffer Error Strategy<br/>Action Discard Oldest Data<br/>Action Log Overflow Event<br/>Action Continue Reception<br/>Action Alert via MQTT]

                Parse_Recovery[Parse Error Strategy<br/>Action Log Parse Error<br/>Action Discard Message<br/>Action Continue Processing<br/>Action Increment Counter]
            end
        end

        subgraph Monitoring_Layer["Error Monitoring Layer"]
            direction TB

            subgraph Display_Monitor["Display Monitoring"]
                TFT_Monitor[TFT Display Output<br/>Error Messages<br/>Connection Status<br/>Buffer Status<br/>System Health]
            end

            subgraph LED_Monitor["LED Indicator Monitoring"]
                LED_Indicator[LED Status Indicators<br/>WiFi Blink or Solid<br/>MQTT Blink or Solid<br/>Error Fast Blink<br/>Normal Slow Blink]
            end

            subgraph Remote_Monitor["Remote Monitoring"]
                MQTT_Report[MQTT Error Reports<br/>Error Topic Publish<br/>Error Counters<br/>System Health Status<br/>Diagnostic Data]
            end

            subgraph Persistent_Monitor["Persistent Monitoring"]
                SD_Log[SD Card Error Log<br/>Log to Storage<br/>Timestamp Events<br/>Persist Error History<br/>Analysis Support]
            end
        end
    end

    I2C_Error --> Sensor_Recovery
    RTC_Error --> RTC_Recovery
    SD_Error --> SD_Recovery
    WiFi_Error --> WiFi_Recovery
    MQTT_Error --> MQTT_Recovery
    Buffer_Error --> Buffer_Recovery
    Parse_Error --> Parse_Recovery

    Sensor_Recovery --> TFT_Monitor
    RTC_Recovery --> TFT_Monitor
    SD_Recovery --> TFT_Monitor
    Buffer_Recovery --> TFT_Monitor
    Parse_Recovery --> TFT_Monitor

    WiFi_Recovery --> LED_Indicator
    MQTT_Recovery --> LED_Indicator

    Sensor_Recovery --> MQTT_Report
    SD_Recovery --> MQTT_Report
    WiFi_Recovery --> MQTT_Report
    MQTT_Recovery --> MQTT_Report

    Sensor_Recovery --> SD_Log
    RTC_Recovery --> SD_Log
    SD_Recovery --> SD_Log
    Buffer_Recovery --> SD_Log

    style Error_Management fill:#F5F5F5,stroke:#424242,stroke-width:2px,color:#000
    style Error_Detection fill:#FFCDD2,stroke:#C62828,stroke-width:3px,color:#000
    style Recovery_Strategies fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000
    style Monitoring_Layer fill:#FFF9C4,stroke:#F57F17,stroke-width:3px,color:#000

    style Hardware_Errors fill:#EF9A9A,stroke:#C62828,stroke-width:2px,color:#000
    style Network_Errors fill:#E57373,stroke:#C62828,stroke-width:2px,color:#000
    style Software_Errors fill:#EF5350,stroke:#C62828,stroke-width:2px,color:#000

    style Hardware_Recovery fill:#A5D6A7,stroke:#388E3C,stroke-width:2px,color:#000
    style Network_Recovery fill:#81C784,stroke:#388E3C,stroke-width:2px,color:#000
    style Software_Recovery fill:#66BB6A,stroke:#388E3C,stroke-width:2px,color:#000

    style Display_Monitor fill:#FFF59D,stroke:#F9A825,stroke-width:2px,color:#000
    style LED_Monitor fill:#FFEE58,stroke:#F9A825,stroke-width:2px,color:#000
    style Remote_Monitor fill:#FFEB3B,stroke:#F57F17,stroke-width:2px,color:#000
    style Persistent_Monitor fill:#FDD835,stroke:#F57F17,stroke-width:2px,color:#000
```

## Deployment Diagram

```mermaid
graph TB
    subgraph Deployment_System["System Deployment Architecture"]
        direction TB

        subgraph Physical_Device["Physical Device Layer"]
            direction TB

            subgraph MCU_STM32["STM32 Microcontroller Node"]
                STM32_Deployment[STM32 Firmware<br/>Language C<br/>Framework STM32 HAL<br/>Function Data Collection<br/>Function Local Buffering<br/>Function Display Control]
            end

            subgraph MCU_ESP32["ESP32 Microcontroller Node"]
                ESP32_Deployment[ESP32 Firmware<br/>Language C<br/>Framework ESP-IDF<br/>Function IoT Gateway<br/>Function MQTT Client<br/>Function WiFi Management]
            end

            subgraph Sensor_Node["Sensor Peripheral Node"]
                Sensor_SHT3X[SHT3X Sensor<br/>Interface I2C<br/>Address 0x44<br/>Function Temperature Humidity]

                Sensor_RTC[DS3231 RTC<br/>Interface I2C<br/>Address 0x68<br/>Function Real-time Clock]
            end

            subgraph Storage_Node["Storage Peripheral Node"]
                Storage_SD[SD Card Storage<br/>Interface SPI<br/>Speed 18MHz<br/>Function Data Buffering<br/>Capacity Configurable]
            end

            subgraph Display_Node["Display Peripheral Node"]
                Display_TFT[ILI9225 Display<br/>Interface SPI<br/>Speed 36MHz<br/>Resolution 176x220<br/>Function Status Display]
            end

            subgraph Control_Node["Control Peripheral Node"]
                Control_Relay[Relay Module<br/>Interface GPIO<br/>Pin Configurable<br/>Function Power Control]

                Control_Buttons[Button Array<br/>Interface GPIO<br/>Count 4 Units<br/>Function User Input]
            end
        end

        subgraph Network_Infrastructure["Network Infrastructure Layer"]
            direction TB

            subgraph Broker_Node["MQTT Broker Node"]
                Broker_Deploy[MQTT Broker<br/>Software Mosquitto<br/>Protocol MQTT v5.0<br/>Port Standard Port<br/>Function Message Routing<br/>Function Message Persistence]
            end

            subgraph WiFi_Node["WiFi Access Point Node"]
                WiFi_AP[WiFi Access Point<br/>Standard 802.11 b/g/n<br/>Band 2.4GHz<br/>Security WPA2-PSK<br/>Function Network Access]
            end
        end

        subgraph Client_Applications["Client Application Layer"]
            direction LR

            Web_App[Web Dashboard<br/>Platform Browser<br/>Technology HTML CSS JavaScript<br/>Library MQTT.js<br/>Function Monitoring<br/>Function Control Interface]

            Mobile_App[Mobile Application<br/>Platform iOS Android<br/>Technology Native App<br/>Library MQTT Client<br/>Function Remote Monitoring<br/>Function Remote Control]
        end
    end

    STM32_Deployment <-->|Protocol UART 115200<br/>Format JSON| ESP32_Deployment

    STM32_Deployment <-->|Protocol I2C<br/>Speed 100kHz| Sensor_SHT3X
    STM32_Deployment <-->|Protocol I2C<br/>Speed 100kHz| Sensor_RTC
    STM32_Deployment <-->|Protocol SPI<br/>Speed 18MHz| Storage_SD
    STM32_Deployment -->|Protocol SPI<br/>Speed 36MHz| Display_TFT

    ESP32_Deployment <-->|Protocol MQTT<br/>Transport TCP| Broker_Deploy
    ESP32_Deployment -->|Protocol GPIO<br/>Level Digital| Control_Relay
    Control_Buttons -->|Protocol GPIO<br/>Trigger Interrupt| ESP32_Deployment

    ESP32_Deployment <-->|Protocol 802.11<br/>Band 2.4GHz| WiFi_AP
    WiFi_AP <-->|Protocol TCP/IP<br/>Layer Network| Broker_Deploy

    Web_App <-->|Protocol MQTT<br/>Transport WebSocket| Broker_Deploy
    Mobile_App <-->|Protocol MQTT<br/>Transport TCP| Broker_Deploy

    Control_Relay -.->|Function Power Switch<br/>Voltage 5V DC| STM32_Deployment

    style Deployment_System fill:#F5F5F5,stroke:#424242,stroke-width:2px,color:#000
    style Physical_Device fill:#FFE0B2,stroke:#E65100,stroke-width:3px,color:#000
    style Network_Infrastructure fill:#BBDEFB,stroke:#1976D2,stroke-width:3px,color:#000
    style Client_Applications fill:#C8E6C9,stroke:#388E3C,stroke-width:3px,color:#000

    style MCU_STM32 fill:#FFCCBC,stroke:#D84315,stroke-width:2px,color:#000
    style MCU_ESP32 fill:#90CAF9,stroke:#1565C0,stroke-width:2px,color:#000
    style Sensor_Node fill:#FFF59D,stroke:#F9A825,stroke-width:2px,color:#000
    style Storage_Node fill:#FFEE58,stroke:#F9A825,stroke-width:2px,color:#000
    style Display_Node fill:#FFEB3B,stroke:#F57F17,stroke-width:2px,color:#000
    style Control_Node fill:#FDD835,stroke:#F57F17,stroke-width:2px,color:#000
    style Broker_Node fill:#64B5F6,stroke:#1976D2,stroke-width:2px,color:#000
    style WiFi_Node fill:#42A5F5,stroke:#1E88E5,stroke-width:2px,color:#000
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
