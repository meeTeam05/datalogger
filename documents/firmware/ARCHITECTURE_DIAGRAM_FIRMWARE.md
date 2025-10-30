# Architecture Diagrams - Firmware System

This document provides the system architecture, data flow, state machines, and infrastructure diagrams for the ESP32 and STM32 coordination system.

## System Architecture Overview

```mermaid
graph TB
    subgraph External[External Systems]
        User[External Clients<br/>Web/Mobile Apps]
        Power[Power Supply<br/>5V DC]
        Network[WiFi Network<br/>Redmi Note 9 Pro]
        Broker[MQTT Broker<br/>192.168.1.39:1883<br/>admin/password]
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
        SHT3X[SHT3X Temp/Humidity<br/>I2C 0x44<br/>±0.2°C, ±2%RH]
        RTC[DS3231 RTC<br/>I2C 0x68<br/>±2ppm accuracy]
        SD[SD Card<br/>SPI1 18MHz<br/>FAT32 Circular Buffer]
        TFT[ILI9225 Display<br/>SPI2 36MHz<br/>176x220 pixels]
        RelayHW[Relay Module<br/>GPIO4<br/>Active HIGH]
        Buttons[4x Buttons<br/>GPIO 5,16,17,4<br/>Active LOW + Pull-up]
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

    User -.->|MQTT Protocol| Broker

    style STM32 fill:#FFE4B5
    style ESP32 fill:#B0E0E6
    style External fill:#E0E0E0
    style Sensors fill:#F0E68C
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

    style Input fill:#90EE90
    style STM32_Processing fill:#FFE4B5
    style ESP32_Processing fill:#B0E0E6
    style Output fill:#DDA0DD
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

    style STM32_HW fill:#FFE4B5
    style ESP32_HW fill:#B0E0E6
    style Peripherals_HW fill:#F0E68C
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

    style Broker fill:#87CEEB
    style Messages fill:#FFE4B5
    style Web fill:#90EE90
    style Mobile fill:#98FB98
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

    style Errors fill:#FF6B6B
    style Recovery fill:#90EE90
    style Monitoring fill:#FFD700
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

---

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
