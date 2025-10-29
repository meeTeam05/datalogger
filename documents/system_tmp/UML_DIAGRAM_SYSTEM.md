# SYSTEM - UML & Architecture Diagrams (Complete System Architecture)

Comprehensive UML and architecture diagrams showing the complete DATALOGGER system structure including all 4 subsystems, their interactions, deployment, and data flows.

---

## 1. Complete System Architecture (C4 Level 1 - Context)

```mermaid
graph TB
    subgraph External["External Systems"]
        User[("👤 User<br/>(Dashboard Access)")]
        Internet[("🌐 Internet<br/>(NTP Time)")]
        Cloud["☁️ Firebase<br/>(Cloud Storage)"]
    end
    
    subgraph DataloggerSystem["DATALOGGER System"]
        WebApp["🖥️ Web Dashboard<br/>(Browser Application)"]
        MQTTBroker["📡 MQTT Broker<br/>(Mosquitto)"]
        Gateway["📱 ESP32 Gateway<br/>(WiFi Bridge)"]
        Firmware["🎛️ STM32 Firmware<br/>(Data Acquisition)"]
    end
    
    subgraph Physical["Physical World"]
        Sensor["🌡️ SHT3X Sensor<br/>(Temperature/Humidity)"]
        Relay["⚡ Relay Module<br/>(Device Control)"]
        Device["🔌 Connected Device"]
    end
    
    User -->|HTTPS/WSS| WebApp
    WebApp -->|MQTT over WebSocket| MQTTBroker
    WebApp -->|Store/Query| Cloud
    WebApp -->|Get Time| Internet
    
    Gateway -->|MQTT TCP| MQTTBroker
    Gateway -->|UART 115200| Firmware
    Gateway -->|GPIO Control| Relay
    Relay -->|Power| Device
    
    Firmware -->|I2C Read| Sensor
    Sensor -->|Temp & Hum| Firmware
    
    style User fill:#90EE90
    style WebApp fill:#87CEEB
    style MQTTBroker fill:#FFD700
    style Gateway fill:#FFA07A
    style Firmware fill:#DDA0DD
    style Sensor fill:#F0E68C
    style Cloud fill:#B0C4DE
```

---

## 2. System Component Diagram

```mermaid
graph TB
    subgraph STM32_System["STM32 Subsystem"]
        STM32_Main["Main Application<br/>main.c"]
        
        subgraph Drivers["Hardware Drivers"]
            SHT3X_Drv["SHT3X Driver<br/>sht3x.c"]
            DS3231_Drv["DS3231 Driver<br/>ds3231.c"]
            SD_Drv["SD Card Driver<br/>sd_card.c"]
            LCD_Drv["ILI9225 Driver<br/>ili9225.c"]
        end
        
        subgraph Managers["System Managers"]
            Data_Mgr["Data Manager<br/>data_manager.c"]
            SD_Mgr["SD Manager<br/>sd_card_manager.c"]
            Display_Mgr["Display Manager<br/>display.c"]
        end
        
        subgraph Communication["Communication"]
            UART_Handler["UART Handler<br/>uart.c"]
            CMD_Parser["Command Parser<br/>cmd_parser.c"]
            Ring_Buf["Ring Buffer<br/>ring_buffer.c"]
        end
    end
    
    subgraph ESP32_System["ESP32 Subsystem"]
        ESP32_Main["Main Task<br/>main.c"]
        
        subgraph ESP32_Components["FreeRTOS Components"]
            WiFi_Mgr["WiFi Manager<br/>wifi_manager"]
            MQTT_Handler["MQTT Handler<br/>mqtt_handler"]
            UART_Bridge["UART Bridge<br/>stm32_uart"]
            Relay_Ctrl["Relay Control<br/>relay_control"]
            JSON_Parser["JSON Parser<br/>json_sensor_parser"]
        end
    end
    
    subgraph Broker_System["MQTT Broker"]
        Mosquitto["Mosquitto 2.x<br/>(Docker)"]
        Auth["Authentication<br/>(bcrypt)"]
        Persistence["Message<br/>Persistence"]
        WebSocket["WebSocket<br/>Protocol"]
    end
    
    subgraph Web_System["Web Dashboard"]
        Web_Main["WebApplication<br/>app.js"]
        
        subgraph Web_Managers["Managers"]
            MQTT_Mgr["MQTTManager"]
            Firebase_Mgr["FirebaseManager"]
            Chart_Mgr["ChartManager"]
            State_Mgr["StateManager"]
            UI_Ctrl["UIController"]
            Health_Mon["HealthMonitor"]
        end
    end
    
    STM32_Main --> Drivers
    STM32_Main --> Managers
    STM32_Main --> Communication
    
    Drivers --> Data_Mgr
    Data_Mgr --> SD_Mgr
    Data_Mgr --> Display_Mgr
    Data_Mgr --> UART_Handler
    
    UART_Handler --> Ring_Buf
    Ring_Buf --> CMD_Parser
    CMD_Parser --> STM32_Main
    
    ESP32_Main --> ESP32_Components
    WiFi_Mgr --> MQTT_Handler
    MQTT_Handler --> JSON_Parser
    JSON_Parser --> UART_Bridge
    UART_Bridge -.UART.-> UART_Handler
    
    MQTT_Handler -.MQTT.-> Mosquitto
    Auth --> Mosquitto
    Persistence --> Mosquitto
    WebSocket --> Mosquitto
    
    Web_Main --> Web_Managers
    MQTT_Mgr -.WebSocket.-> Mosquitto
    Firebase_Mgr -.HTTPS.-> Firebase_Mgr
    
    style STM32_System fill:#DDA0DD,stroke:#9370DB,stroke-width:3px
    style ESP32_System fill:#FFA07A,stroke:#FF6347,stroke-width:3px
    style Broker_System fill:#FFD700,stroke:#FFA500,stroke-width:3px
    style Web_System fill:#87CEEB,stroke:#4682B4,stroke-width:3px
```

---

## 3. Deployment Diagram

```mermaid
graph TB
    subgraph Hardware["Hardware Layer"]
        subgraph MCU_Board["STM32 Blue Pill Board"]
            STM32["STM32F103C8T6<br/>ARM Cortex-M3<br/>72MHz, 64KB Flash"]
            STM32_Periph["Peripherals<br/>I2C1, SPI1<br/>UART1, TIM2"]
        end
        
        subgraph ESP_Board["ESP32 Development Board"]
            ESP32["ESP32-WROOM-32<br/>Dual-core Xtensa<br/>240MHz, WiFi"]
            ESP32_Periph["Peripherals<br/>UART2, GPIO4<br/>WiFi Radio"]
        end
        
        subgraph Sensors_HW["Sensor Hardware"]
            SHT3X["SHT3X<br/>I2C 0x44"]
            DS3231["DS3231<br/>I2C 0x68<br/>+ CR2032"]
            SD_Card["MicroSD Card<br/>FAT32<br/>1GB+"]
            LCD_HW["ILI9225<br/>176×220<br/>SPI"]
            Relay_HW["5V Relay<br/>GPIO4<br/>NO/NC"]
        end
    end
    
    subgraph Network["Network Layer"]
        Router["WiFi Router<br/>2.4GHz<br/>SSID: xxx"]
        
        subgraph Server["Server (Docker Host)"]
            Docker["Docker Engine"]
            Broker_Container["mosquitto-broker<br/>Container<br/>Port 1883, 8083"]
            Volumes["Docker Volumes<br/>- config<br/>- data<br/>- logs"]
        end
    end
    
    subgraph Client["Client Layer"]
        Browser["Web Browser<br/>(Chrome/Firefox)<br/>User Device"]
        Firebase_Cloud["Firebase RTDB<br/>(Google Cloud)<br/>Realtime Database"]
    end
    
    STM32_Periph -->|I2C| SHT3X
    STM32_Periph -->|I2C| DS3231
    STM32_Periph -->|SPI| SD_Card
    STM32_Periph -->|SPI| LCD_HW
    STM32_Periph -->|UART<br/>115200bps| ESP32_Periph
    
    ESP32_Periph -->|GPIO| Relay_HW
    ESP32_Periph -->|WiFi<br/>802.11n| Router
    
    Router -->|Ethernet/WiFi| Docker
    Docker --> Broker_Container
    Broker_Container --> Volumes
    
    Browser -->|WebSocket<br/>ws://host:8083| Broker_Container
    Browser -->|HTTPS<br/>Firebase SDK| Firebase_Cloud
    
    style Hardware fill:#FFE4E1
    style Network fill:#E0FFFF
    style Client fill:#F0FFF0
    style STM32 fill:#DDA0DD
    style ESP32 fill:#FFA07A
    style Broker_Container fill:#FFD700
```

---

## 4. Network Topology Diagram

```mermaid
graph TB
    subgraph Internet_Zone["Internet"]
        Firebase["Firebase<br/>Realtime Database<br/>firebaseio.com"]
        NTP["NTP Servers<br/>pool.ntp.org"]
    end
    
    subgraph LAN["Local Area Network (192.168.1.0/24)"]
        Router["WiFi Router<br/>192.168.1.1<br/>Gateway + DHCP"]
        
        subgraph Server_Subnet["Server Subnet"]
            DockerHost["Docker Host<br/>192.168.1.100"]
            
            subgraph Docker_Network["Docker Bridge (172.17.0.0/16)"]
                Broker["MQTT Broker<br/>172.17.0.2<br/>Port 1883, 8083"]
            end
        end
        
        subgraph IoT_Subnet["IoT Device Subnet"]
            ESP32_Dev["ESP32 Gateway<br/>192.168.1.150<br/>WiFi Client"]
        end
        
        subgraph Client_Subnet["Client Subnet"]
            Laptop["User Laptop<br/>192.168.1.50"]
            Phone["Mobile Phone<br/>192.168.1.51"]
        end
        
        subgraph Embedded["Embedded (No IP)"]
            STM32_Dev["STM32 Board<br/>(UART Only)"]
        end
    end
    
    Router -->|Port Forward<br/>8883:1883<br/>8083:8083| DockerHost
    DockerHost -->|Docker NAT| Broker
    
    ESP32_Dev -->|MQTT TCP<br/>192.168.1.100:1883| Broker
    ESP32_Dev -.UART 115200.-> STM32_Dev
    
    Laptop -->|WebSocket<br/>ws://192.168.1.100:8083| Broker
    Phone -->|WebSocket<br/>ws://192.168.1.100:8083| Broker
    
    Laptop -->|HTTPS| Firebase
    Phone -->|HTTPS| Firebase
    
    Laptop -->|NTP| NTP
    
    Router -->|Internet| Firebase
    Router -->|Internet| NTP
    
    style Internet_Zone fill:#FFE4B5
    style LAN fill:#E0FFFF
    style Docker_Network fill:#F0F8FF
    style Broker fill:#FFD700
    style ESP32_Dev fill:#FFA07A
    style STM32_Dev fill:#DDA0DD
```

---

## 5. Data Flow Architecture

```mermaid
graph LR
    subgraph Sources["Data Sources"]
        Sensor["🌡️ SHT3X Sensor<br/>±0.2°C, ±2% RH"]
        RTC["⏰ DS3231 RTC<br/>Unix Timestamp"]
    end
    
    subgraph Acquisition["Data Acquisition"]
        STM32["STM32 Firmware<br/>- I2C Read<br/>- JSON Format<br/>- Validation"]
    end
    
    subgraph Buffering["Offline Buffering"]
        SD["SD Card<br/>204,800 records<br/>512B blocks"]
        Memory["RAM Buffer<br/>256B Ring<br/>UART RX"]
    end
    
    subgraph Transport["Data Transport"]
        UART["UART Link<br/>115200 bps<br/>8N1"]
        ESP32["ESP32 Gateway<br/>- Parse JSON<br/>- WiFi Bridge"]
    end
    
    subgraph Distribution["Data Distribution"]
        MQTT["MQTT Broker<br/>- Topic Routing<br/>- QoS 0/1<br/>- Persistence"]
    end
    
    subgraph Storage["Data Storage"]
        Firebase["Firebase RTDB<br/>- Historical<br/>- Queryable<br/>- Cloud Backup"]
    end
    
    subgraph Visualization["Data Visualization"]
        Chart["Chart.js<br/>Real-time Line<br/>50-point window"]
        Table["Data Table<br/>Live Updates<br/>Filtering"]
        Display["ILI9225 LCD<br/>Local Display<br/>176×220 px"]
    end
    
    Sensor -->|I2C 100kHz| STM32
    RTC -->|I2C 100kHz| STM32
    
    STM32 -->|Online| UART
    STM32 -->|Offline| SD
    SD -->|Sync| UART
    
    UART --> Memory
    Memory --> ESP32
    
    ESP32 -->|WiFi| MQTT
    MQTT -->|WebSocket| Chart
    MQTT -->|WebSocket| Table
    MQTT -.Store.-> Firebase
    
    STM32 -->|SPI| Display
    
    style Sources fill:#F0E68C
    style Acquisition fill:#DDA0DD
    style Buffering fill:#FFB6C1
    style Transport fill:#FFA07A
    style Distribution fill:#FFD700
    style Storage fill:#B0C4DE
    style Visualization fill:#87CEEB
```

---

## 6. STM32 State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> INIT: Power On
    
    INIT --> IDLE: Init Success
    INIT --> ERROR: Init Failed
    
    IDLE --> SINGLE_READ: Single Command
    IDLE --> PERIODIC_START: Periodic ON
    IDLE --> CONFIG: Set Time/Config
    
    SINGLE_READ --> MEASURING: Trigger Sensor
    MEASURING --> PROCESSING: Data Ready
    PROCESSING --> TRANSMITTING: Format JSON
    TRANSMITTING --> IDLE: TX Complete
    TRANSMITTING --> BUFFERING: WiFi Down
    BUFFERING --> IDLE: Buffered
    
    PERIODIC_START --> PERIODIC_ACTIVE: Timer Started
    PERIODIC_ACTIVE --> PERIODIC_MEASURE: Timer Interrupt
    PERIODIC_MEASURE --> PERIODIC_PROCESS: Data Ready
    PERIODIC_PROCESS --> PERIODIC_TX: Format JSON
    PERIODIC_TX --> PERIODIC_ACTIVE: TX Complete
    PERIODIC_TX --> PERIODIC_BUFFER: WiFi Down
    PERIODIC_BUFFER --> PERIODIC_ACTIVE: Buffered
    PERIODIC_ACTIVE --> IDLE: Stop Command
    
    CONFIG --> IDLE: Config Complete
    
    ERROR --> RECOVERY: Retry Init
    RECOVERY --> IDLE: Recovered
    RECOVERY --> ERROR: Failed
    
    BUFFERING --> SYNCING: WiFi Restored
    PERIODIC_BUFFER --> SYNCING: WiFi Restored
    SYNCING --> IDLE: Sync Complete
    
    note right of IDLE
        - Sensor Ready
        - Waiting Commands
        - Display Active
    end note
    
    note right of PERIODIC_ACTIVE
        - Timer Running
        - Continuous Measurement
        - Real-time TX
    end note
    
    note right of BUFFERING
        - SD Card Writing
        - Offline Mode
        - Data Preserved
    end note
```

---

## 7. ESP32 State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> BOOT: Power On
    
    BOOT --> WIFI_INIT: FreeRTOS Start
    WIFI_INIT --> WIFI_CONNECTING: Start WiFi
    
    WIFI_CONNECTING --> WIFI_CONNECTED: Association OK
    WIFI_CONNECTING --> WIFI_RETRY: Timeout
    WIFI_RETRY --> WIFI_CONNECTING: Backoff Delay
    WIFI_RETRY --> WIFI_FAILED: Max Retries
    
    WIFI_CONNECTED --> MQTT_INIT: Got IP Address
    MQTT_INIT --> MQTT_CONNECTING: Connect Broker
    
    MQTT_CONNECTING --> MQTT_CONNECTED: CONNACK
    MQTT_CONNECTING --> MQTT_RETRY: Timeout
    MQTT_RETRY --> MQTT_CONNECTING: Retry Delay
    
    MQTT_CONNECTED --> OPERATIONAL: Subscribed
    
    OPERATIONAL --> PROCESSING_CMD: Command Received
    OPERATIONAL --> PROCESSING_DATA: Data from STM32
    OPERATIONAL --> RELAY_CONTROL: Relay Command
    
    PROCESSING_CMD --> OPERATIONAL: Forwarded to STM32
    PROCESSING_DATA --> OPERATIONAL: Published to MQTT
    RELAY_CONTROL --> OPERATIONAL: GPIO Updated
    
    OPERATIONAL --> MQTT_DISCONNECTED: Connection Lost
    OPERATIONAL --> WIFI_DISCONNECTED: WiFi Lost
    
    MQTT_DISCONNECTED --> MQTT_CONNECTING: Auto Reconnect
    WIFI_DISCONNECTED --> WIFI_CONNECTING: Auto Reconnect
    
    WIFI_FAILED --> RECOVERY: Soft Reset
    RECOVERY --> WIFI_INIT: Reset Complete
    
    note right of OPERATIONAL
        - Bidirectional Bridge
        - UART ↔ MQTT
        - Relay Control
        - State Reporting
    end note
    
    note right of MQTT_CONNECTED
        - Subscribed Topics:
          * datalogger/command
          * datalogger/relay
          * datalogger/time/sync
    end note
```

---

## 8. Web Dashboard State Machine

```mermaid
stateDiagram-v2
    [*] --> LOADING: Page Load
    
    LOADING --> INIT_MQTT: HTML Parsed
    INIT_MQTT --> INIT_FIREBASE: MQTT Connecting
    
    INIT_FIREBASE --> CONNECTING: Services Init
    
    CONNECTING --> CONNECTED: All Connected
    CONNECTING --> PARTIAL: MQTT Only
    CONNECTING --> OFFLINE: All Failed
    
    CONNECTED --> ACTIVE: Receiving Data
    
    ACTIVE --> DISPLAYING: Update UI
    ACTIVE --> COMMANDING: User Action
    ACTIVE --> QUERYING: Data Request
    ACTIVE --> EXPORTING: Export Request
    
    DISPLAYING --> ACTIVE: Render Complete
    COMMANDING --> ACTIVE: Command Sent
    QUERYING --> ACTIVE: Query Complete
    EXPORTING --> ACTIVE: Export Done
    
    ACTIVE --> DEGRADED: MQTT Lost
    DEGRADED --> ACTIVE: MQTT Restored
    DEGRADED --> OFFLINE: Firebase Lost
    
    OFFLINE --> CONNECTING: Auto Reconnect
    PARTIAL --> CONNECTED: Firebase OK
    
    note right of ACTIVE
        - Real-time Charts
        - Live Data Table
        - Component Health
        - User Interactions
    end note
    
    note right of DEGRADED
        - MQTT Down
        - Firebase Active
        - Historical Data Only
        - No Real-time
    end note
```

---

## 9. MQTT Topic Hierarchy

```mermaid
graph TB
    Root["datalogger/ (Root Topic)"]
    
    Root --> Command["command<br/>(QoS 1)<br/>Web → STM32"]
    Root --> Relay["relay<br/>(QoS 1)<br/>Web → ESP32"]
    Root --> TimeSync["time/sync<br/>(QoS 1)<br/>Web → STM32"]
    Root --> State["state<br/>(QoS 0)<br/>ESP32 → Web"]
    Root --> Data["data/<br/>(Data Topics)"]
    
    Data --> Single["single<br/>(QoS 0)<br/>STM32 → Web"]
    Data --> Periodic["periodic<br/>(QoS 0)<br/>STM32 → Web"]
    
    Command -.->|Payload| CMD_Schema["Command Schema:<br/>{<br/>  command: string,<br/>  interval?: number<br/>}"]
    
    Relay -.->|Payload| Relay_Schema["Relay Schema:<br/>{<br/>  state: 'ON'|'OFF'<br/>}"]
    
    TimeSync -.->|Payload| Time_Schema["Time Schema:<br/>{<br/>  command: 'SET_TIME',<br/>  timestamp: number<br/>}"]
    
    State -.->|Payload| State_Schema["State Schema:<br/>{<br/>  status: string,<br/>  uptime: number,<br/>  wifi_rssi: number,<br/>  relay_state: string,<br/>  sensor: string,<br/>  periodic_mode: boolean<br/>}"]
    
    Single -.->|Payload| Data_Schema["Data Schema:<br/>{<br/>  mode: string,<br/>  timestamp: number,<br/>  temperature: number,<br/>  humidity: number<br/>}"]
    
    Periodic -.->|Payload| Data_Schema
    
    style Root fill:#FFD700
    style Command fill:#FFB6C1
    style Relay fill:#FFA07A
    style TimeSync fill:#87CEEB
    style State fill:#98FB98
    style Single fill:#DDA0DD
    style Periodic fill:#DDA0DD
```

---

## 10. Firebase Database Schema

```mermaid
graph TB
    Root["Firebase Realtime Database<br/>(Root)"]
    
    Root --> Datalogger["datalogger/<br/>(Project Root)"]
    
    Datalogger --> Readings["readings/<br/>(Sensor Data)"]
    Datalogger --> States["states/<br/>(System States)"]
    Datalogger --> Config["config/<br/>(Configuration)"]
    Datalogger --> Latest["latest/<br/>(Latest Reading)"]
    Datalogger --> Stats["statistics/<br/>(Statistics)"]
    
    Readings --> Reading1["1729780245/<br/>(Unix Timestamp Key)"]
    Reading1 --> R1_Temp["temperature: 25.43"]
    Reading1 --> R1_Hum["humidity: 60.21"]
    Reading1 --> R1_Mode["mode: 'periodic'"]
    Reading1 --> R1_Time["timestamp: 1729780245"]
    Reading1 --> R1_Received["receivedAt: 1729780246"]
    
    Readings --> Reading2["1729780250/"]
    Readings --> Reading3["1729780255/"]
    Readings --> ReadingN["... (10,000 records max)"]
    
    States --> State1["1729780300/<br/>(State Timestamp)"]
    State1 --> S1_Status["status: 'connected'"]
    State1 --> S1_Uptime["uptime: 12345"]
    State1 --> S1_RSSI["wifi_rssi: -52"]
    State1 --> S1_Relay["relay_state: 'OFF'"]
    
    Config --> MQTT_Config["mqtt/<br/>(MQTT Settings)"]
    MQTT_Config --> MQTT_Broker["broker: 'ws://...'"]
    MQTT_Config --> MQTT_User["username: 'admin'"]
    
    Config --> Device_Config["device/<br/>(Device Settings)"]
    Device_Config --> Dev_Name["name: 'DATALOGGER-01'"]
    Device_Config --> Dev_Interval["interval: 5000"]
    
    Latest --> L_Temp["temperature: 25.48"]
    Latest --> L_Hum["humidity: 60.15"]
    Latest --> L_Time["timestamp: 1729780345"]
    Latest --> L_Mode["mode: 'periodic'"]
    
    Stats --> S_Count["total_readings: 5432"]
    Stats --> S_Min["min_temp: 18.2"]
    Stats --> S_Max["max_temp: 32.8"]
    Stats --> S_Avg["avg_temp: 24.5"]
    
    style Root fill:#B0C4DE
    style Datalogger fill:#87CEEB
    style Readings fill:#FFB6C1
    style States fill:#98FB98
    style Config fill:#FFD700
    style Latest fill:#FFA07A
    style Stats fill:#DDA0DD
```

---

## 11. Hardware Pin Mapping Diagram

```mermaid
graph TB
    subgraph STM32_Pins["STM32F103C8T6 Pin Configuration"]
        subgraph I2C_Bus["I2C1 Bus (100kHz)"]
            PB6["PB6 (SCL)<br/>Clock"]
            PB7["PB7 (SDA)<br/>Data"]
        end
        
        subgraph SPI_Bus["SPI1 Bus (18MHz)"]
            PA5["PA5 (SCK)<br/>Clock"]
            PA6["PA6 (MISO)<br/>Master In"]
            PA7["PA7 (MOSI)<br/>Master Out"]
            PA4["PA4 (NSS)<br/>SD Card CS"]
            PB0["PB0 (GPIO)<br/>LCD CS"]
            PB1["PB1 (GPIO)<br/>LCD RS"]
        end
        
        subgraph UART_Interface["UART1 (115200bps)"]
            PA9["PA9 (TX)<br/>Transmit"]
            PA10["PA10 (RX)<br/>Receive"]
        end
        
        subgraph Power_Pins["Power"]
            VCC["3.3V<br/>Output"]
            VIN["5V<br/>Input"]
            GND1["GND<br/>Common"]
        end
    end
    
    subgraph ESP32_Pins["ESP32-WROOM-32 Pin Configuration"]
        subgraph UART2_Interface["UART2 (115200bps)"]
            GPIO16["GPIO16 (RX)<br/>Receive"]
            GPIO17["GPIO17 (TX)<br/>Transmit"]
        end
        
        subgraph GPIO_Control["GPIO Output"]
            GPIO4["GPIO4<br/>Relay Control<br/>Active HIGH"]
        end
        
        subgraph ESP_Power["Power"]
            VIN_ESP["5V (VIN)<br/>Input"]
            V33_ESP["3.3V<br/>LDO Output"]
            GND_ESP["GND<br/>Common"]
        end
    end
    
    subgraph Connections["Physical Connections"]
        PB6 -.I2C SCL.-> SHT3X_SCL["SHT3X SCL<br/>(0x44)"]
        PB7 -.I2C SDA.-> SHT3X_SDA["SHT3X SDA"]
        PB6 -.I2C SCL.-> RTC_SCL["DS3231 SCL<br/>(0x68)"]
        PB7 -.I2C SDA.-> RTC_SDA["DS3231 SDA"]
        
        PA5 -.SPI CLK.-> SD_CLK["SD Card CLK"]
        PA6 -.SPI MISO.-> SD_MISO["SD Card MISO"]
        PA7 -.SPI MOSI.-> SD_MOSI["SD Card MOSI"]
        PA4 -.CS.-> SD_CS["SD Card CS"]
        
        PB0 -.CS.-> LCD_CS["ILI9225 CS"]
        PB1 -.RS.-> LCD_RS["ILI9225 RS"]
        PA5 -.SPI CLK.-> LCD_CLK["ILI9225 CLK"]
        PA7 -.SPI MOSI.-> LCD_SDI["ILI9225 SDI"]
        
        PA9 ==>|UART TX| GPIO16
        GPIO17 ==>|UART TX| PA10
        GND1 === GND_ESP
        
        GPIO4 -->|3.3V Signal| Relay_IN["Relay IN"]
        VIN_ESP -->|5V| Relay_VCC["Relay VCC"]
        GND_ESP --> Relay_GND["Relay GND"]
    end
    
    style STM32_Pins fill:#DDA0DD,stroke:#9370DB,stroke-width:2px
    style ESP32_Pins fill:#FFA07A,stroke:#FF6347,stroke-width:2px
    style I2C_Bus fill:#FFE4E1
    style SPI_Bus fill:#E0FFFF
    style UART_Interface fill:#F0FFF0
    style UART2_Interface fill:#FFF8DC
```

---

## 12. Power Distribution Diagram

```mermaid
graph TB
    Supply["5V DC Power Supply<br/>1A Minimum"]
    
    Supply --> STM32_5V["STM32 Board 5V Pin"]
    Supply --> ESP32_VIN["ESP32 VIN Pin (5V)"]
    Supply --> Relay_VCC["Relay Module VCC"]
    
    STM32_5V --> STM32_LDO["STM32 Internal<br/>3.3V LDO<br/>(AMS1117)"]
    ESP32_VIN --> ESP32_LDO["ESP32 Internal<br/>3.3V LDO<br/>(500mA max)"]
    
    STM32_LDO --> STM32_Core["STM32 Core<br/>72MHz<br/>30-50mA"]
    STM32_LDO --> STM32_3V3["3.3V Output Pin"]
    
    STM32_3V3 --> SHT3X_Power["SHT3X Sensor<br/>1.5µA idle<br/>800µA measuring"]
    STM32_3V3 --> DS3231_Power["DS3231 RTC<br/>100µA active<br/>+ CR2032 Backup"]
    STM32_3V3 --> SD_Power["SD Card Module<br/>20-50mA read/write"]
    STM32_3V3 --> LCD_Power["ILI9225 Display<br/>30-50mA active"]
    
    ESP32_LDO --> ESP32_Core["ESP32 Core<br/>Dual-core 240MHz<br/>80-120mA WiFi"]
    ESP32_Core --> WiFi_Radio["WiFi Radio<br/>+160mA TX peak"]
    
    Relay_VCC --> Relay_Coil["Relay Coil<br/>15-20mA<br/>70mA peak"]
    
    STM32_Core -.GPIO4.-> Relay_Signal["Relay Control<br/>3.3V Logic"]
    
    Supply --> Common_GND["Common Ground<br/>(All GND Connected)"]
    
    subgraph Power_Budget["Total Power Budget"]
        Total["Peak Current:<br/>~500mA @ 5V<br/>~2.5W Total"]
        
        STM32_Total["STM32 System:<br/>~150mA"]
        ESP32_Total["ESP32 System:<br/>~240mA"]
        Relay_Total["Relay Module:<br/>~70mA"]
        Other["Margin:<br/>~40mA"]
    end
    
    style Supply fill:#FFD700
    style STM32_LDO fill:#FFB6C1
    style ESP32_LDO fill:#FFA07A
    style Common_GND fill:#A9A9A9
    style Power_Budget fill:#E0FFFF
```

---

## 13. Complete System Class Diagram

```mermaid
classDiagram
    class WebApplication {
        -mqttManager: MQTTManager
        -firebaseManager: FirebaseManager
        -chartManager: ChartManager
        -uiController: UIController
        -stateManager: StateManager
        +init()
        +start()
        +shutdown()
    }
    
    class MQTTManager {
        -client: mqtt.Client
        -connected: boolean
        -topics: string[]
        +connect()
        +subscribe(topic)
        +publish(topic, message, qos)
        +onMessage(callback)
        +disconnect()
    }
    
    class FirebaseManager {
        -database: firebase.Database
        -connected: boolean
        +init(config)
        +storeReading(data)
        +queryReadings(start, end)
        +getLatest()
        +updateConfig(config)
    }
    
    class ChartManager {
        -chart: Chart
        -maxPoints: number
        -datasets: Dataset[]
        +init(canvas)
        +addDataPoint(temp, hum, time)
        +updateChart()
        +clearChart()
        +exportImage()
    }
    
    class UIController {
        -pages: Page[]
        -currentPage: string
        +renderPage(name)
        +bindEvents()
        +showToast(message)
        +showModal(content)
        +updateDisplay(data)
    }
    
    class StateManager {
        -deviceState: State
        -periodicMode: boolean
        -relayState: boolean
        +loadState()
        +saveState()
        +updateState(key, value)
        +getState(key)
    }
    
    class ComponentHealthMonitor {
        -components: Component[]
        -checkInterval: number
        +startMonitoring()
        +checkHealth()
        +updateStatus(component, status)
        +getOverallHealth()
    }
    
    class STM32_Firmware {
        <<Embedded>>
        -sht3x: SHT3X_Handle
        -ds3231: DS3231_Handle
        -sdCard: SDCard_Handle
        -display: Display_Handle
        +initialize()
        +singleRead()
        +startPeriodic(interval)
        +stopPeriodic()
        +processCommand(cmd)
    }
    
    class ESP32_Gateway {
        <<Embedded>>
        -wifiManager: WiFiManager_Handle
        -mqttHandler: MQTTHandler_Handle
        -relayControl: RelayControl_Handle
        +initialize()
        +connectWiFi()
        +connectMQTT()
        +forwardUART()
        +controlRelay(state)
    }
    
    class MQTTBroker {
        <<Service>>
        -port: 1883, 8083
        -authentication: boolean
        -persistence: boolean
        +routeMessage(topic, payload)
        +authenticate(user, pass)
        +manageSubscriptions()
    }
    
    WebApplication --> MQTTManager
    WebApplication --> FirebaseManager
    WebApplication --> ChartManager
    WebApplication --> UIController
    WebApplication --> StateManager
    WebApplication --> ComponentHealthMonitor
    
    MQTTManager --> MQTTBroker: connects to
    ESP32_Gateway --> MQTTBroker: connects to
    ESP32_Gateway --> STM32_Firmware: UART
    
    MQTTManager ..> ESP32_Gateway: publishes commands
    ESP32_Gateway ..> MQTTManager: publishes data
    STM32_Firmware ..> ESP32_Gateway: sends sensor data
```

---

## 14. Message Sequence Flow (Complete Lifecycle)

```mermaid
graph TB
    Start([User Interaction]) --> WebUI[Web UI Event]
    WebUI --> BuildCmd[Build Command<br/>JSON Format]
    BuildCmd --> MQTT_Pub[MQTT Publish<br/>QoS 1]
    
    MQTT_Pub --> Broker[MQTT Broker<br/>Route Message]
    Broker --> ESP32_Sub[ESP32 Subscriber<br/>Receive]
    
    ESP32_Sub --> Parse[Parse JSON<br/>Extract Fields]
    Parse --> UART_TX[UART Transmit<br/>115200 bps]
    
    UART_TX --> STM32_RX[STM32 UART IRQ<br/>Ring Buffer]
    STM32_RX --> CMD_Parse[Command Parser<br/>Text Protocol]
    
    CMD_Parse --> Execute[Execute Command<br/>Function Call]
    Execute --> Sensor[Sensor Operation<br/>I2C Read]
    
    Sensor --> Format[Format Response<br/>JSON Output]
    Format --> UART_TX2[UART TX to ESP32]
    
    UART_TX2 --> ESP32_RX[ESP32 UART RX<br/>Parse JSON]
    ESP32_RX --> MQTT_Pub2[MQTT Publish<br/>Data Topic]
    
    MQTT_Pub2 --> Broker2[Broker Route]
    Broker2 --> Web_Sub[Web Subscriber<br/>Receive Data]
    
    Web_Sub --> Update_UI[Update UI<br/>Chart & Table]
    Update_UI --> Firebase[Store to Firebase<br/>Cloud Backup]
    
    Firebase --> End([Complete Cycle])
    
    STM32_RX -.Offline Path.-> SD_Buffer[SD Card Buffer<br/>204,800 records]
    SD_Buffer -.Sync Later.-> UART_TX2
    
    Format -.Local Display.-> LCD_Update[ILI9225 Display<br/>Local Feedback]
    
    style Start fill:#90EE90
    style End fill:#90EE90
    style Broker fill:#FFD700
    style Broker2 fill:#FFD700
    style ESP32_Sub fill:#FFA07A
    style ESP32_RX fill:#FFA07A
    style STM32_RX fill:#DDA0DD
    style Execute fill:#DDA0DD
    style Web_Sub fill:#87CEEB
    style Firebase fill:#B0C4DE
    style SD_Buffer fill:#FFB6C1
    style LCD_Update fill:#F0E68C
```

---

## 15. Security Architecture Diagram

```mermaid
graph TB
    subgraph Internet["Internet (Untrusted)"]
        Attacker["🔴 Potential Attacker"]
        Public_Net["Public Network"]
    end
    
    subgraph Firewall["Firewall / Router"]
        NAT["NAT Gateway"]
        Port_Filter["Port Filtering"]
    end
    
    subgraph DMZ["DMZ / Broker Zone"]
        Broker["MQTT Broker"]
        
        subgraph Auth_Layer["Authentication Layer"]
            User_Auth["Username/Password<br/>(bcrypt)"]
            ACL["Access Control Lists"]
        end
        
        subgraph Encryption_Layer["Encryption (Optional)"]
            TLS["TLS 1.2/1.3<br/>(Certificates)"]
        end
    end
    
    subgraph Trusted_Zone["Trusted Local Network"]
        ESP32_Dev["ESP32 Gateway<br/>(Authenticated)"]
        Web_Client["Web Dashboard<br/>(Authenticated)"]
    end
    
    subgraph Cloud["Cloud Services"]
        Firebase_Auth["Firebase<br/>(OAuth 2.0)"]
    end
    
    Attacker -->|Blocked| NAT
    Public_Net -->|Port 8883<br/>TLS only| Port_Filter
    
    Port_Filter --> TLS
    TLS --> User_Auth
    User_Auth --> ACL
    ACL --> Broker
    
    ESP32_Dev -->|Authenticated<br/>Connection| User_Auth
    Web_Client -->|WebSocket<br/>+ Auth| User_Auth
    
    Web_Client -->|HTTPS<br/>OAuth| Firebase_Auth
    
    Broker -->|Publish/Subscribe<br/>Verified| ESP32_Dev
    Broker -->|Publish/Subscribe<br/>Verified| Web_Client
    
    subgraph Security_Measures["Security Measures"]
        SM1["✓ Password Authentication"]
        SM2["✓ bcrypt Password Hash"]
        SM3["✓ Port Filtering"]
        SM4["✓ NAT Protection"]
        SM5["✓ WebSocket WSS (Optional)"]
        SM6["✓ Firebase OAuth"]
        SM7["✓ No Default Credentials"]
        SM8["✓ Regular Updates"]
    end
    
    style Attacker fill:#FF6B6B
    style NAT fill:#FFD700
    style Broker fill:#FFD700
    style Auth_Layer fill:#98FB98
    style Encryption_Layer fill:#87CEEB
    style Security_Measures fill:#E0FFFF
```

---

## 16. Performance Monitoring Architecture

```mermaid
graph TB
    subgraph Metrics_Sources["Metrics Collection Points"]
        STM32_Metrics["STM32 Metrics<br/>- Sensor Read Time<br/>- UART TX Time<br/>- SD Write Time<br/>- Display Update Time"]
        
        ESP32_Metrics["ESP32 Metrics<br/>- WiFi RSSI<br/>- MQTT Round-trip<br/>- Free Heap<br/>- Task Stack Usage"]
        
        Broker_Metrics["Broker Metrics<br/>- Message Rate<br/>- Client Count<br/>- Queue Depth<br/>- CPU Usage"]
        
        Web_Metrics["Web Metrics<br/>- Chart Render Time<br/>- Firebase Query Time<br/>- UI Response Time<br/>- Memory Usage"]
    end
    
    subgraph Monitoring_Layer["Monitoring & Aggregation"]
        Health_Monitor["Component Health<br/>Monitor"]
        
        Performance_Tracker["Performance<br/>Tracker"]
        
        Alert_Manager["Alert Manager<br/>Threshold Detection"]
    end
    
    subgraph Visualization["Performance Visualization"]
        Dashboard_Perf["Performance<br/>Dashboard"]
        
        Graphs["Real-time Graphs<br/>- Latency<br/>- Throughput<br/>- Error Rate"]
        
        Logs["System Logs<br/>- Console<br/>- File<br/>- Remote"]
    end
    
    STM32_Metrics --> Health_Monitor
    ESP32_Metrics --> Health_Monitor
    Broker_Metrics --> Health_Monitor
    Web_Metrics --> Health_Monitor
    
    Health_Monitor --> Performance_Tracker
    Performance_Tracker --> Alert_Manager
    
    Alert_Manager --> Dashboard_Perf
    Performance_Tracker --> Graphs
    Health_Monitor --> Logs
    
    Dashboard_Perf --> User_View["👤 System Admin<br/>Performance View"]
    
    subgraph KPIs["Key Performance Indicators"]
        KPI1["End-to-End Latency<br/>Target: < 150ms"]
        KPI2["Data Loss Rate<br/>Target: < 0.01%"]
        KPI3["System Uptime<br/>Target: > 99.5%"]
        KPI4["WiFi Signal<br/>Target: > -70dBm"]
        KPI5["Free Memory<br/>Target: > 100KB"]
    end
    
    style Metrics_Sources fill:#FFE4E1
    style Monitoring_Layer fill:#E0FFFF
    style Visualization fill:#F0FFF0
    style KPIs fill:#FFD700
```

---

*End of UML_DIAGRAM_SYSTEM.md - Total: 16 comprehensive architecture diagrams*

---

## Summary of SYSTEM Documentation

### FLOW_DIAGRAM_SYSTEM.md (12 Flowcharts)
1. Complete System Initialization
2. Data Acquisition & Transmission
3. Command Processing (Web to STM32)
4. Offline Buffering & Sync
5. Error Detection & Recovery
6. Periodic Mode Operation
7. Relay Control (End-to-End)
8. Time Synchronization
9. Web Dashboard Initialization
10. Firebase Data Storage
11. Component Health Monitoring
12. Data Export & Download

### SEQUENCE_DIAGRAM_SYSTEM.md (15 Sequences)
1. Complete System Startup
2. Single Read Command
3. Periodic Monitoring Session
4. Offline Buffering & Synchronization
5. Relay Control
6. Time Synchronization (NTP)
7. Sensor Error Recovery
8. Firebase Data Query & Display
9. System Health Monitoring
10. Chart Real-Time Update
11. Data Export to CSV
12. System State Synchronization
13. MQTT Broker Restart & Recovery
14. Display Update (LCD)
15. Complete Error Cascade & Recovery

### UML_DIAGRAM_SYSTEM.md (16 Architecture Diagrams)
1. Complete System Architecture (C4 Level 1)
2. System Component Diagram
3. Deployment Diagram
4. Network Topology
5. Data Flow Architecture
6. STM32 State Machine
7. ESP32 State Machine
8. Web Dashboard State Machine
9. MQTT Topic Hierarchy
10. Firebase Database Schema
11. Hardware Pin Mapping
12. Power Distribution
13. Complete System Class Diagram
14. Message Sequence Flow
15. Security Architecture
16. Performance Monitoring Architecture

**Total: 43 comprehensive system-level diagrams covering all aspects of the DATALOGGER system**

