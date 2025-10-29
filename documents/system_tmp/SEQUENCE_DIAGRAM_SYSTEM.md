# SYSTEM - Sequence Diagrams (Complete System Interactions)

Comprehensive sequence diagrams showing interactions between all 4 subsystems: STM32 Firmware, ESP32 Gateway, MQTT Broker, and Web Dashboard.

---

## 1. Complete System Startup Sequence

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant STM32 as STM32 Firmware
    participant Sensor as SHT3X Sensor
    participant RTC as DS3231 RTC
    participant SD as SD Card
    participant LCD as ILI9225 Display
    
    Note over STM32,LCD: Power On
    
    STM32->>STM32: HAL_Init()
    STM32->>STM32: SystemClock_Config(72MHz)
    STM32->>STM32: Init Peripherals (I2C, SPI, UART, TIM)
    
    STM32->>RTC: DS3231_Init (I2C 0x68)
    RTC-->>STM32: Init OK
    
    STM32->>Sensor: SHT3X_Init (I2C 0x44)
    Sensor-->>STM32: Init OK
    
    STM32->>SD: SDCardManager_Init (SPI)
    SD-->>STM32: Card Detected, FAT32
    
    STM32->>LCD: ILI9225_Init (SPI)
    LCD-->>STM32: Init OK
    STM32->>LCD: Display "STARTING..."
    
    Note over ESP32: Power On (3s after STM32)
    
    ESP32->>ESP32: FreeRTOS Init
    ESP32->>ESP32: Create Tasks (WiFi, MQTT, UART)
    
    ESP32->>ESP32: WiFi_Manager_Init
    ESP32->>ESP32: Connect to AP (SSID)
    
    Note over ESP32: WiFi Connecting...
    
    ESP32->>ESP32: WiFi Connected (IP: 192.168.1.x)
    
    ESP32->>Broker: MQTT Connect (ws://broker:1883)
    Broker-->>ESP32: CONNACK (Session Present=0)
    
    ESP32->>Broker: Subscribe datalogger/command
    Broker-->>ESP32: SUBACK QoS=1
    
    ESP32->>Broker: Subscribe datalogger/relay
    Broker-->>ESP32: SUBACK QoS=1
    
    ESP32->>Broker: Subscribe datalogger/time/sync
    Broker-->>ESP32: SUBACK QoS=1
    
    ESP32->>STM32: UART: "MQTT CONNECTED\r\n"
    STM32->>STM32: wifi_manager_set_connected(true)
    STM32->>LCD: Update Display "WiFi: OK"
    
    ESP32->>Broker: Publish datalogger/state
    Note right of ESP32: {"status":"connected",<br/>"uptime":5,"wifi_rssi":-45}
    
    User->>Web: Open Dashboard URL
    Web->>Web: Load HTML/CSS/JS
    Web->>Web: WebApplication.init()
    
    Web->>Broker: WebSocket Connect (ws://broker:8083/mqtt)
    Broker-->>Web: WebSocket Handshake OK
    
    Web->>Broker: MQTT Connect (username/password)
    Broker-->>Web: CONNACK (Auth OK)
    
    Web->>Broker: Subscribe datalogger/#
    Broker-->>Web: SUBACK QoS=1
    
    Web->>Web: Firebase.initializeApp()
    Web->>Web: Firebase Connected
    
    Web->>Web: Query Historical Data (last 100)
    Web->>Web: Render Dashboard
    
    Note over STM32,Web: System Fully Operational
    
    STM32->>LCD: Display "READY"
    Web->>User: Show "All Systems Connected"
```

---

## 2. Single Read Command Flow (End-to-End)

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Firebase as Firebase RTDB
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant STM32 as STM32 Firmware
    participant Sensor as SHT3X Sensor
    participant RTC as DS3231 RTC
    participant LCD as ILI9225 Display
    
    User->>Web: Click "Read Single" Button
    Web->>Web: Build Command JSON
    Note right of Web: {"command":"READ_SINGLE"}
    
    Web->>Broker: Publish datalogger/command (QoS=1)
    Broker->>Broker: Route Message
    Broker->>ESP32: Deliver to Subscriber
    
    ESP32->>ESP32: MQTT Event Handler
    ESP32->>ESP32: Parse JSON: "READ_SINGLE"
    ESP32->>STM32: UART TX: "SINGLE\r\n"
    
    STM32->>STM32: UART RX Interrupt
    STM32->>STM32: Ring Buffer Store
    STM32->>STM32: Parse Complete Line
    STM32->>STM32: cmd_parser: "SINGLE"
    STM32->>STM32: cmd_execute: Lookup & Call
    
    STM32->>Sensor: SHT3X_Single (High Repeatability)
    Sensor->>Sensor: Measure (15ms)
    Sensor-->>STM32: Temp=25.43°C, Hum=60.21%
    
    STM32->>RTC: DS3231_Get_Time
    RTC-->>STM32: 2025-10-24 14:30:45
    STM32->>STM32: Convert to Unix: 1729780245
    
    STM32->>STM32: DataManager_UpdateSingle
    STM32->>STM32: Format JSON Output
    Note right of STM32: {"mode":"single",<br/>"timestamp":1729780245,<br/>"temperature":25.43,<br/>"humidity":60.21}
    
    STM32->>ESP32: UART TX: JSON String
    
    ESP32->>ESP32: UART RX Handler
    ESP32->>ESP32: Parse JSON (cJSON)
    ESP32->>Broker: Publish datalogger/data/single (QoS=0)
    
    Broker->>Broker: Route to Subscribers
    Broker->>Web: Deliver Message
    
    Web->>Web: MQTT Message Handler
    Web->>Web: Parse JSON Data
    Web->>Web: Update Dashboard Display
    Web->>Web: Update Chart (Add Point)
    Web->>Web: Update Live Table (New Row)
    
    Web->>Firebase: Store to /datalogger/readings/1729780245
    Firebase-->>Web: Write Success
    
    Web->>User: Show Toast: "Temperature: 25.43°C"
    
    STM32->>LCD: Update Display
    Note right of STM32: Temp: 25.43°C<br/>Hum: 60.21%<br/>14:30:45
    LCD->>User: Visual Feedback
    
    Note over User,LCD: Total Latency: ~150ms
```

---

## 3. Periodic Monitoring Session

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant STM32 as STM32 Firmware
    participant Sensor as SHT3X Sensor
    participant TIM as TIM2 Timer
    participant LCD as ILI9225 Display
    
    User->>Web: Enable Periodic Mode
    User->>Web: Set Interval: 5000ms
    
    Web->>Broker: Publish datalogger/command
    Note right of Web: {"command":"START_PERIODIC",<br/>"interval":5000}
    
    Broker->>ESP32: Deliver Command
    ESP32->>ESP32: Parse JSON
    ESP32->>STM32: UART: "PERIODIC ON 5000\r\n"
    
    STM32->>STM32: Parse Command
    STM32->>STM32: Validate Interval (500-60000ms)
    
    STM32->>Sensor: SHT3X_Periodic(Mode=1MPS)
    Sensor-->>STM32: Periodic Mode Started
    
    STM32->>TIM: Configure TIM2 (5000ms)
    STM32->>TIM: HAL_TIM_Base_Start_IT
    TIM-->>STM32: Timer Running
    
    STM32->>STM32: Set periodic_active = true
    STM32->>LCD: Update "PERIODIC: ON (5s)"
    
    STM32->>ESP32: Response: "OK"
    ESP32->>Broker: Publish datalogger/state
    Note right of ESP32: {"periodic_mode":true,<br/>"interval":5000}
    Broker->>Web: Deliver State
    Web->>User: Show "Periodic Mode Active"
    
    loop Every 5 seconds
        TIM->>STM32: TIM2 IRQ (Overflow)
        STM32->>STM32: Set measurement_flag
        
        STM32->>Sensor: SHT3X_FetchData
        Sensor-->>STM32: Temp=25.48°C, Hum=60.15%
        
        STM32->>STM32: Get Timestamp (RTC)
        STM32->>STM32: Format JSON (mode=periodic)
        STM32->>ESP32: UART TX: JSON
        
        ESP32->>Broker: Publish datalogger/data/periodic
        Broker->>Web: Deliver Data
        
        Web->>Web: Update Chart (Real-time)
        Web->>Web: Update Live Table
        Web->>Web: Store to Firebase
        
        STM32->>LCD: Update Display Values
    end
    
    Note over User,LCD: Continuous Monitoring...
    
    User->>Web: Click "Stop Periodic"
    Web->>Broker: Publish Command: STOP_PERIODIC
    Broker->>ESP32: Deliver
    ESP32->>STM32: UART: "PERIODIC OFF\r\n"
    
    STM32->>TIM: HAL_TIM_Base_Stop_IT
    STM32->>Sensor: SHT3X_Stop_Periodic
    STM32->>STM32: Clear periodic_active
    
    STM32->>LCD: Update "PERIODIC: OFF"
    STM32->>ESP32: Response: "STOPPED"
    ESP32->>Broker: Publish State Update
    Broker->>Web: Deliver
    Web->>User: Show "Periodic Mode Stopped"
```

---

## 4. Offline Buffering & Synchronization Sequence

```mermaid
sequenceDiagram
    participant STM32 as STM32 Firmware
    participant ESP32 as ESP32 Gateway
    participant SD as SD Card
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    participant LCD as ILI9225 Display
    
    Note over STM32,Web: System Operating Normally
    
    STM32->>STM32: Periodic Measurement
    STM32->>STM32: Format JSON Data
    STM32->>ESP32: UART TX: Sensor Data
    ESP32->>Broker: MQTT Publish
    Broker->>Web: Deliver Data
    
    Note over ESP32,Broker: WiFi Connection Lost!
    
    ESP32->>ESP32: WiFi Disconnect Event
    ESP32->>STM32: UART: "MQTT DISCONNECTED\r\n"
    
    STM32->>STM32: wifi_manager_set_connected(false)
    STM32->>LCD: Update "WiFi: OFFLINE"
    
    Note over STM32,SD: Enter Offline Mode
    
    loop Offline Data Collection
        STM32->>STM32: Periodic Measurement Continues
        STM32->>STM32: Get Sensor Data + Timestamp
        
        STM32->>SD: SDCardManager_WriteData
        Note right of SD: Record: Timestamp, Temp<br/>Humidity, Mode, Seq#
        
        SD->>SD: SPI Write (512 bytes)
        SD-->>STM32: Write Success
        
        STM32->>STM32: Increment Write Index
        STM32->>STM32: Buffered Count++
        
        STM32->>LCD: Update "SD: WRITING (Count)"
    end
    
    Note over ESP32: WiFi Reconnects
    
    ESP32->>ESP32: WiFi Connect Event
    ESP32->>ESP32: WiFi Associated
    ESP32->>Broker: MQTT Reconnect
    Broker-->>ESP32: CONNACK
    
    ESP32->>Broker: Resubscribe Topics
    Broker-->>ESP32: SUBACK
    
    ESP32->>STM32: UART: "MQTT CONNECTED\r\n"
    
    STM32->>STM32: wifi_manager_set_connected(true)
    STM32->>STM32: Check Buffered Data
    
    STM32->>SD: SDCardManager_GetBufferedCount()
    SD-->>STM32: Count = 47 records
    
    STM32->>LCD: Update "SD: SYNCING (47)"
    
    loop Sync Buffered Data (47 times)
        STM32->>SD: SDCardManager_ReadData (Oldest)
        SD-->>STM32: Record Data
        
        STM32->>STM32: Format JSON (Buffered Data)
        STM32->>ESP32: UART TX: JSON
        
        ESP32->>ESP32: Parse & Validate
        ESP32->>Broker: Publish datalogger/data/periodic
        Broker->>Web: Deliver (Historical)
        
        Web->>Web: Process Buffered Data
        Web->>Web: Store to Firebase
        
        alt ACK Received
            ESP32->>STM32: UART: "ACK\r\n"
            STM32->>SD: SDCardManager_RemoveRecord
            STM32->>STM32: Decrement Counter
        else Timeout
            STM32->>STM32: Retry (Max 3)
        end
        
        STM32->>LCD: Update "SD: SYNCING (46...1)"
    end
    
    STM32->>SD: All Data Synced
    STM32->>STM32: Clear Sync Flag
    STM32->>LCD: Update "WiFi: OK (Synced)"
    
    Note over STM32,Web: Resume Normal Operation
    
    STM32->>ESP32: Send Current Data
    ESP32->>Broker: Publish Real-time
    Broker->>Web: Deliver
    Web->>Web: Update Dashboard
```

---

## 5. Relay Control Sequence

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant GPIO as GPIO4 Pin
    participant Relay as Relay Module
    participant Device as Connected Device
    
    User->>Web: Click "Relay ON" Button
    Web->>Web: UI State Change (Button Blue)
    
    Web->>Broker: Publish datalogger/relay
    Note right of Web: {"state":"ON"}
    
    Broker->>Broker: Route Message (QoS=1)
    Broker->>ESP32: Deliver to Subscriber
    
    ESP32->>ESP32: MQTT Event Handler
    ESP32->>ESP32: Parse JSON: state="ON"
    
    ESP32->>ESP32: relay_control_set(RELAY_ON)
    ESP32->>GPIO: GPIO_SetBits(GPIO4)
    GPIO->>GPIO: Pin Level = HIGH (3.3V)
    
    GPIO->>Relay: Control Signal HIGH
    Relay->>Relay: Energize Coil (15mA)
    Relay->>Relay: Contact Closes (Click Sound)
    
    Relay->>Device: Power Applied (230V AC)
    Device->>Device: Device Turns ON
    
    ESP32->>ESP32: Log "Relay ON"
    ESP32->>Broker: Publish datalogger/state
    Note right of ESP32: {"relay_state":"ON",<br/>"timestamp":1729780345}
    
    Broker->>Web: Deliver State Update
    Web->>Web: Update UI Confirmation
    Web->>User: Show Toast "Relay Activated"
    
    Note over User,Device: Device Operating...
    
    User->>Web: Click "Relay OFF" Button
    Web->>Web: UI State Change (Button Gray)
    
    Web->>Broker: Publish datalogger/relay
    Note right of Web: {"state":"OFF"}
    
    Broker->>ESP32: Deliver Command
    ESP32->>ESP32: Parse: state="OFF"
    
    ESP32->>ESP32: relay_control_set(RELAY_OFF)
    ESP32->>GPIO: GPIO_ResetBits(GPIO4)
    GPIO->>GPIO: Pin Level = LOW (0V)
    
    GPIO->>Relay: Control Signal LOW
    Relay->>Relay: De-energize Coil
    Relay->>Relay: Contact Opens (Click Sound)
    
    Relay->>Device: Power Removed
    Device->>Device: Device Turns OFF
    
    ESP32->>Broker: Publish State: relay_state=OFF
    Broker->>Web: Deliver Update
    Web->>User: Show "Relay Deactivated"
```

---

## 6. Time Synchronization Sequence (NTP)

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant STM32 as STM32 Firmware
    participant RTC as DS3231 RTC
    participant LCD as ILI9225 Display
    
    User->>Web: Navigate to Time Settings
    User->>Web: Click "Sync with Internet"
    
    Web->>Web: Get Browser Time
    Note right of Web: new Date().getTime()
    Web->>Web: Convert to Unix Timestamp
    Note right of Web: timestamp = 1729780445
    
    Web->>Web: Validate Timestamp
    Web->>Broker: Publish datalogger/time/sync
    Note right of Web: {"command":"SET_TIME",<br/>"timestamp":1729780445}
    
    Broker->>ESP32: Deliver Command
    ESP32->>ESP32: Parse JSON
    ESP32->>ESP32: Extract Timestamp
    
    ESP32->>ESP32: Convert Unix to DateTime
    Note right of ESP32: 2025-10-24 14:34:05
    
    ESP32->>STM32: UART TX
    Note right of ESP32: "SET TIME 2025 10 24 14 34 5\r\n"
    
    STM32->>STM32: UART RX + Parse
    STM32->>STM32: cmd_parser: Extract Components
    Note right of STM32: Year=2025, Month=10, Day=24<br/>Hour=14, Minute=34, Second=5
    
    STM32->>STM32: Validate DateTime
    alt Valid DateTime
        STM32->>RTC: DS3231_Set_Time (I2C Write)
        Note right of RTC: Set Registers 0x00-0x06
        
        RTC-->>STM32: Write Success
        
        STM32->>RTC: DS3231_Get_Time (Verify)
        RTC-->>STM32: Read Back Time
        
        STM32->>STM32: Compare Values
        
        alt Time Matches
            STM32->>LCD: Update Display
            Note right of LCD: 14:34:05<br/>2025-10-24
            
            STM32->>ESP32: UART: "TIME SET OK\r\n"
            ESP32->>Broker: Publish datalogger/state
            Note right of ESP32: {"time_updated":true,<br/>"timestamp":1729780445}
            
            Broker->>Web: Deliver Success
            Web->>User: Show Success Toast
            Note right of Web: "Time Synchronized"
            
            Web->>Web: Update Time Display
        else Verification Failed
            STM32->>ESP32: UART: "TIME SET FAILED\r\n"
            ESP32->>Broker: Publish Error State
            Broker->>Web: Deliver Error
            Web->>User: Show Error "Sync Failed"
        end
    else Invalid DateTime
        STM32->>ESP32: UART: "INVALID TIME\r\n"
        ESP32->>Broker: Publish Error
        Broker->>Web: Deliver Error
        Web->>User: Show "Invalid Time Format"
    end
```

---

## 7. Sensor Error Recovery Sequence

```mermaid
sequenceDiagram
    participant STM32 as STM32 Firmware
    participant Sensor as SHT3X Sensor
    participant ESP32 as ESP32 Gateway
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    participant LCD as ILI9225 Display
    
    Note over STM32,Sensor: Normal Operation
    
    STM32->>Sensor: SHT3X_Single (Measurement)
    
    alt Sensor I2C Error
        Sensor--xSTM32: NACK / Timeout
        
        STM32->>STM32: Error Detected
        STM32->>STM32: Increment Error Counter
        
        loop Retry 3 Times
            STM32->>Sensor: Retry Measurement
            
            alt Success on Retry
                Sensor-->>STM32: Valid Data
                STM32->>STM32: Clear Error Counter
                STM32->>STM32: Log Recovery
            else Retry Failed
                Sensor--xSTM32: Still NACK
                STM32->>STM32: Increment Counter
            end
        end
        
        alt All Retries Failed
            STM32->>STM32: Mark Sensor Failed
            STM32->>STM32: Set Error Values (0.0, 0.0)
            
            STM32->>LCD: Display "SENSOR ERR"
            LCD->>LCD: Show Red Error Icon
            
            STM32->>ESP32: UART TX: Error Data
            Note right of STM32: {"temperature":0.0,<br/>"humidity":0.0,<br/>"error":"sensor_failed"}
            
            ESP32->>Broker: Publish datalogger/data/single
            Broker->>Web: Deliver Error Data
            
            Web->>Web: Detect Error Values
            Web->>Web: ComponentHealthMonitor Update
            Note right of Web: STM32_Sensor: ERROR
            
            Web->>Web: Show Alert Modal
            Web->>Web: Display "Sensor Communication Lost"
            
            Note over STM32: Attempt Recovery
            
            STM32->>STM32: Wait 10 seconds
            STM32->>Sensor: SHT3X_Init (Reinitialize)
            
            alt Reinitialization Success
                Sensor-->>STM32: Init OK
                STM32->>STM32: Clear Error Flag
                STM32->>LCD: Update "SENSOR OK"
                
                STM32->>Sensor: Test Measurement
                Sensor-->>STM32: Valid Data
                
                STM32->>ESP32: UART: Valid Data
                ESP32->>Broker: Publish
                Broker->>Web: Deliver
                
                Web->>Web: Update Health: Sensor OK
                Web->>Web: Show "Sensor Recovered"
            else Reinitialization Failed
                Sensor--xSTM32: Init Failed
                STM32->>LCD: Keep "SENSOR ERR"
                STM32->>STM32: Continue with Error State
            end
        end
    else Sensor OK
        Sensor-->>STM32: Valid Data (Temp, Hum)
        STM32->>STM32: Normal Processing
    end
```

---

## 8. Firebase Data Query & Display Sequence

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Firebase as Firebase RTDB
    participant Chart as Chart.js
    participant Table as Data Table
    
    User->>Web: Navigate to Data Management
    Web->>Web: Load Page UI
    
    User->>Web: Select Date Range
    Note right of User: Start: 2025-10-23<br/>End: 2025-10-24
    
    User->>Web: Click "Load Data"
    
    Web->>Web: Validate Date Range
    Web->>Web: Show Loading Spinner
    
    Web->>Firebase: Query Database
    Note right of Web: ref('datalogger/readings')<br/>.orderByChild('timestamp')<br/>.startAt(start_ts)<br/>.endAt(end_ts)
    
    Firebase->>Firebase: Process Query
    Firebase->>Firebase: Filter Records
    
    Firebase-->>Web: Return Snapshot
    Note left of Firebase: 247 records
    
    Web->>Web: snapshot.forEach()
    Web->>Web: Extract Data Records
    
    Web->>Web: Sort by Timestamp
    Web->>Web: Apply Filters (if any)
    
    alt Records Found
        Web->>Table: Clear Existing Rows
        
        loop For Each Record
            Web->>Web: Format Timestamp
            Web->>Web: Format Temperature
            Web->>Web: Format Humidity
            Web->>Table: Add Table Row
        end
        
        Table->>User: Display Data Table (247 rows)
        
        Web->>Web: Extract Chart Data
        Web->>Web: Prepare Datasets
        Note right of Web: X-axis: Timestamps<br/>Y-axis: Temp & Humidity
        
        Web->>Chart: Update Chart Data
        Chart->>Chart: Clear Previous Data
        Chart->>Chart: Add New Datasets
        Chart->>Chart: Render Chart
        
        Chart->>User: Display Line Chart
        
        Web->>Web: Calculate Statistics
        Note right of Web: Min, Max, Avg, StdDev
        Web->>Web: Update Stats Panel
        
        Web->>User: Hide Loading Spinner
        Web->>User: Show "247 records loaded"
        
    else No Records
        Web->>Web: Hide Loading
        Web->>User: Show "No data for range"
        Web->>Table: Display Empty State
    end
    
    User->>Web: Apply Temperature Filter
    Note right of User: Temp > 25°C
    
    Web->>Web: Re-filter Dataset
    Web->>Web: Filter Records
    Note right of Web: 247 → 143 records
    
    Web->>Table: Update Table (143 rows)
    Web->>Chart: Update Chart (143 points)
    Web->>User: Show "143 records match"
    
    User->>Web: Click "Export to CSV"
    Web->>Web: Trigger Export Flow
    Note right of Web: (See Export Sequence)
```

---

## 9. System Health Monitoring Sequence

```mermaid
sequenceDiagram
    participant Monitor as Health Monitor (Web)
    participant MQTT as MQTTManager
    participant Firebase as FirebaseManager
    participant Components as Component Status
    participant UI as UI Display
    participant User
    
    Note over Monitor: Timer Triggers (Every 5s)
    
    Monitor->>MQTT: Check MQTT Status
    MQTT-->>Monitor: connected: true<br/>last_message: 2s ago
    Monitor->>Monitor: MQTT Status: OK
    
    Monitor->>Firebase: Check Firebase Status
    Firebase-->>Monitor: connected: true<br/>last_write: 5s ago
    Monitor->>Monitor: Firebase Status: OK
    
    Monitor->>Components: Check STM32 Status
    Components-->>Monitor: last_data: 3s ago<br/>sensor_valid: true
    Monitor->>Monitor: STM32 Status: OK
    
    Monitor->>Components: Check ESP32 Status
    Components-->>Monitor: state_received: 10s ago<br/>wifi_rssi: -52dBm
    Monitor->>Monitor: ESP32 Status: OK (WiFi Good)
    
    Monitor->>Monitor: Aggregate Health
    Note right of Monitor: All Systems: HEALTHY
    
    Monitor->>UI: Update Health Panel
    UI->>UI: Set All Icons Green ✓
    UI->>User: Display "System Healthy"
    
    Note over Monitor,User: 30 seconds later...
    
    Note over MQTT: MQTT Connection Lost
    
    MQTT->>MQTT: Connection Error Event
    MQTT->>Monitor: Notify Connection Lost
    
    Monitor->>Monitor: Update MQTT Status: ERROR
    Monitor->>Monitor: Increment Error Counter
    
    Monitor->>UI: Update MQTT Icon Red ✗
    UI->>User: Show Alert Toast
    Note right of User: "MQTT Connection Lost"
    
    Monitor->>Monitor: Calculate Overall Health
    Note right of Monitor: System: WARNING<br/>(MQTT down, others OK)
    
    Monitor->>UI: Update Health Panel Yellow ⚠
    UI->>User: Display "System Degraded"
    
    Note over MQTT: MQTT Reconnecting...
    
    MQTT->>MQTT: Reconnect Attempt
    MQTT->>MQTT: Connection Success
    MQTT->>Monitor: Notify Connected
    
    Monitor->>Monitor: Update MQTT Status: OK
    Monitor->>Monitor: Clear Error Counter
    Monitor->>Monitor: Log Recovery Event
    
    Monitor->>UI: Update MQTT Icon Green ✓
    UI->>User: Show Success Toast
    Note right of User: "MQTT Reconnected"
    
    Monitor->>Monitor: Recalculate Health
    Note right of Monitor: System: HEALTHY<br/>(All components OK)
    
    Monitor->>UI: Update Health Panel Green
    UI->>User: Display "System Recovered"
```

---

## 10. Chart Real-Time Update Sequence

```mermaid
sequenceDiagram
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    participant MQTT as MQTT Client
    participant Chart as Chart.js Manager
    participant Canvas as Chart Canvas
    participant User
    
    Broker->>MQTT: MQTT Message Arrives
    Note right of Broker: datalogger/data/periodic
    
    MQTT->>Web: onMessage Event
    Web->>Web: Parse JSON Payload
    Note right of Web: {temp:25.48, hum:60.15,<br/>timestamp:1729780545}
    
    Web->>Web: Validate Data
    Web->>Web: Extract Values
    
    Web->>Chart: chartManager.addDataPoint()
    Chart->>Chart: Get Current Dataset
    Chart->>Chart: Check Dataset Length
    
    alt Dataset > 50 Points
        Chart->>Chart: Remove Oldest Point (shift)
        Note right of Chart: Rolling Window (50 points)
    end
    
    Chart->>Chart: Add New Data Point
    Note right of Chart: Temperature: 25.48<br/>Humidity: 60.15<br/>Time: 14:35:45
    
    Chart->>Chart: Format X-axis Label
    Note right of Chart: Convert timestamp to HH:MM:SS
    
    Chart->>Chart: Update Chart Data
    Chart->>Chart: chart.update('none')
    Note right of Chart: Animation: none for real-time
    
    Chart->>Canvas: Render Update
    Canvas->>Canvas: Redraw Canvas
    Canvas->>Canvas: Draw Axes
    Canvas->>Canvas: Draw Line (Temperature)
    Canvas->>Canvas: Draw Line (Humidity)
    Canvas->>Canvas: Draw Data Points
    
    Canvas->>User: Display Updated Chart
    
    Web->>Web: Update Statistics
    Note right of Web: Recalculate Min/Max/Avg
    Web->>User: Update Stats Panel
    
    Note over Broker,User: 5 seconds later...
    
    Broker->>MQTT: Next Message
    Note over Web,User: Continuous updates...
```

---

## 11. Data Export to CSV Sequence

```mermaid
sequenceDiagram
    participant User
    participant Web as Web Dashboard
    participant Firebase as Firebase RTDB
    participant Export as Export Handler
    participant Browser
    
    User->>Web: Navigate to Data Management
    User->>Web: Select Date Range
    Note right of User: 2025-10-23 to 2025-10-24
    
    User->>Web: Click "Export to CSV"
    
    Web->>Export: exportToCSV(startDate, endDate)
    Export->>Export: Show Progress Modal
    
    Export->>Firebase: Query Data Range
    Note right of Export: orderByChild('timestamp')<br/>.startAt(start).endAt(end)
    
    Firebase-->>Export: Return 247 Records
    
    Export->>Export: Process Records
    Export->>Export: Build CSV Header
    Note right of Export: "Timestamp,Date,Time,<br/>Temperature,Humidity,Mode"
    
    loop For Each Record (247)
        Export->>Export: Extract Fields
        Export->>Export: Format Timestamp
        Note right of Export: Unix → YYYY-MM-DD HH:MM:SS
        
        Export->>Export: Format Temperature
        Note right of Export: Round to 2 decimals
        
        Export->>Export: Format Humidity
        Export->>Export: Append CSV Row
        Note right of Export: "1729780345,2025-10-24,<br/>14:35:45,25.48,60.15,periodic"
    end
    
    Export->>Export: CSV String Complete
    Export->>Export: Create Blob
    Note right of Export: MIME: text/csv
    
    Export->>Export: Generate Filename
    Note right of Export: datalogger_20251024_143545.csv
    
    Export->>Export: Create Object URL
    Export->>Export: Create Download Link
    
    Export->>Browser: Trigger Download
    Browser->>Browser: Show Save Dialog
    Browser->>User: Download Started
    
    Export->>Export: Revoke Object URL
    Export->>Export: Update Export Stats
    Export->>Export: Log Export Event
    
    Export->>Web: Hide Progress Modal
    Web->>User: Show Success Toast
    Note right of User: "247 records exported"
```

---

## 12. System State Synchronization Sequence

```mermaid
sequenceDiagram
    participant STM32 as STM32 Firmware
    participant ESP32 as ESP32 Gateway
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    participant Firebase as Firebase RTDB
    
    Note over STM32,Firebase: Periodic State Updates (Every 30s)
    
    STM32->>STM32: Collect System State
    Note right of STM32: - Uptime<br/>- Sensor Status<br/>- SD Card Status<br/>- Display Status<br/>- Periodic Mode<br/>- Buffered Count
    
    STM32->>ESP32: UART TX: State JSON
    Note right of STM32: {"stm32_uptime":12345,<br/>"sensor":"OK",<br/>"sd_card":"OK",<br/>"display":"OK",<br/>"periodic":true,<br/>"buffered":0}
    
    ESP32->>ESP32: Receive & Parse
    ESP32->>ESP32: Add ESP32 State
    Note right of ESP32: - WiFi RSSI<br/>- MQTT Connected<br/>- Relay State<br/>- ESP32 Uptime<br/>- Free Heap
    
    ESP32->>ESP32: Merge State Objects
    Note right of ESP32: Combined system state
    
    ESP32->>Broker: Publish datalogger/state
    Note right of ESP32: {"stm32_uptime":12345,<br/>"esp32_uptime":12340,<br/>"sensor":"OK",<br/>"wifi_rssi":-52,<br/>"mqtt_connected":true,<br/>"relay_state":"OFF",<br/>"free_heap":180000}
    
    Broker->>Web: Deliver State
    Web->>Web: StateManager.updateState()
    
    Web->>Web: Update Component Status
    Note right of Web: STM32: OK ✓<br/>ESP32: OK ✓<br/>Sensor: OK ✓
    
    Web->>Web: Update WiFi Signal
    Note right of Web: RSSI: -52dBm → Good
    
    Web->>Web: Update Uptime Display
    Note right of Web: Format: 3h 25m 45s
    
    Web->>Web: Update Memory Status
    Note right of Web: Free Heap: 180KB
    
    Web->>Firebase: Store State History
    Note right of Firebase: /datalogger/states/<br/>{timestamp}
    
    Firebase-->>Web: Write Success
    
    Web->>Web: Trigger Health Check
    Note right of Web: Verify all components OK
    
    Note over STM32,Firebase: State Synchronized
```

---

## 13. MQTT Broker Restart & Recovery Sequence

```mermaid
sequenceDiagram
    participant Broker as MQTT Broker
    participant ESP32 as ESP32 Gateway
    participant Web as Web Dashboard
    participant STM32 as STM32 Firmware
    participant Admin as System Admin
    
    Note over Broker,Web: System Operating Normally
    
    Admin->>Broker: docker restart mosquitto-broker
    Broker->>Broker: SIGTERM Received
    Broker->>Broker: Save Persistent Data
    Broker->>Broker: Disconnect All Clients
    
    Broker->>ESP32: Connection Close
    ESP32->>ESP32: Disconnect Event
    ESP32->>ESP32: MQTT_EVENT_DISCONNECTED
    ESP32->>STM32: UART: "MQTT DISCONNECTED\r\n"
    STM32->>STM32: Set WiFi Connected = false
    
    Broker->>Web: WebSocket Close
    Web->>Web: MQTT Disconnect Event
    Web->>Web: Update UI: Disconnected
    Web->>Web: Show Warning Banner
    
    Note over Broker: Container Restarting...
    
    Broker->>Broker: Container Stopped
    Broker->>Broker: Container Starting
    Broker->>Broker: Load mosquitto.conf
    Broker->>Broker: Load Persistent Data
    Broker->>Broker: Open Port 1883 (MQTT)
    Broker->>Broker: Open Port 8083 (WebSocket)
    Broker->>Broker: Broker Ready
    
    Note over ESP32: Auto Reconnect (5s delay)
    
    ESP32->>ESP32: Reconnect Timer Expires
    ESP32->>Broker: MQTT Connect
    Broker-->>ESP32: CONNACK (Session Present=1)
    
    ESP32->>Broker: Resubscribe Topics
    Broker-->>ESP32: SUBACK
    
    ESP32->>STM32: UART: "MQTT CONNECTED\r\n"
    STM32->>STM32: Set WiFi Connected = true
    
    Note over Web: Auto Reconnect (JavaScript)
    
    Web->>Broker: WebSocket Connect
    Broker-->>Web: WebSocket Handshake OK
    
    Web->>Broker: MQTT Connect
    Broker-->>Web: CONNACK
    
    Web->>Broker: Resubscribe datalogger/#
    Broker-->>Web: SUBACK
    
    Web->>Web: Update UI: Connected
    Web->>Web: Hide Warning Banner
    Web->>Web: Show Success Toast
    Note right of Web: "Reconnected to Broker"
    
    Note over Broker,STM32: Check for Buffered Data
    
    STM32->>STM32: Check SD Buffer
    alt Buffered Data Exists
        STM32->>STM32: Start Sync Process
        Note right of STM32: (See Buffering Sequence)
    else No Buffered Data
        STM32->>STM32: Resume Normal Operation
    end
    
    Note over Broker,STM32: System Fully Recovered
```

---

## 14. Display Update Sequence (LCD)

```mermaid
sequenceDiagram
    participant STM32 as STM32 Firmware
    participant Data as Data Manager
    participant Display as Display Controller
    participant LCD as ILI9225 Driver
    participant SPI as SPI1 Peripheral
    participant Screen as Physical LCD
    
    STM32->>Data: New Sensor Data Available
    Data->>Data: dataManager.data_ready = true
    
    STM32->>Display: display_update()
    Display->>Display: Check Update Needed
    
    Display->>Data: Get Latest Data
    Data-->>Display: Temp, Hum, Timestamp, Mode
    
    Display->>Display: Format Display Strings
    Note right of Display: Temperature: "25.48°C"<br/>Humidity: "60.15%"<br/>Time: "14:35:45"<br/>Mode: "PERIODIC: ON (5s)"<br/>WiFi: "WiFi: OK"
    
    Display->>LCD: Clear Previous Values
    LCD->>LCD: Calculate Coordinates
    
    loop For Each Display Element
        Display->>LCD: ILI9225_DrawText()
        LCD->>LCD: Set Text Position
        LCD->>LCD: Select Font (Font16/Font24)
        LCD->>LCD: Select Color (RGB565)
        
        loop For Each Character
            LCD->>LCD: Get Character Bitmap
            LCD->>LCD: Set Draw Window
            LCD->>SPI: Write Command (0x20-0x23)
            SPI->>Screen: Set Window Coordinates
            
            LCD->>SPI: Write Command (0x22)
            SPI->>Screen: Start Data Transfer
            
            loop For Each Pixel in Character
                LCD->>LCD: Get Pixel Color
                LCD->>SPI: Write Color Data (16-bit)
                SPI->>Screen: Set Pixel Color
            end
        end
    end
    
    Display->>LCD: Draw Status Icons
    LCD->>LCD: Draw WiFi Icon
    alt WiFi Connected
        LCD->>SPI: Draw Green Icon
    else WiFi Disconnected
        LCD->>SPI: Draw Red Icon
    end
    
    LCD->>LCD: Draw Mode Indicator
    alt Periodic Mode
        LCD->>SPI: Draw Blue Circle (Animated)
    else Idle Mode
        LCD->>SPI: No Indicator
    end
    
    Display->>Display: Update Complete Flag
    Display->>STM32: Return Success
    
    Screen->>Screen: Pixels Updated
    Note over Screen: User Sees:<br/>Temp: 25.48°C<br/>Hum: 60.15%<br/>14:35:45<br/>PERIODIC: ON (5s)<br/>WiFi: OK ✓
```

---

## 15. Complete Error Cascade & Recovery Sequence

```mermaid
sequenceDiagram
    participant STM32 as STM32 Firmware
    participant ESP32 as ESP32 Gateway
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    participant User
    
    Note over STM32,Web: Multiple Failures Occur
    
    par Sensor Failure
        STM32->>STM32: SHT3X Read Timeout
        STM32->>STM32: Set sensor_error = true
        STM32->>STM32: Send Error Data (0.0, 0.0)
    and WiFi Failure
        ESP32->>ESP32: WiFi Disconnect Event
        ESP32->>ESP32: Set wifi_connected = false
        ESP32->>STM32: UART: "MQTT DISCONNECTED\r\n"
    and Broker Overload
        Broker->>Broker: High CPU Usage
        Broker->>Broker: Slow Response Times
    end
    
    STM32->>STM32: Multiple Errors Detected
    STM32->>STM32: Enter Error Mode
    
    Web->>Web: No Data for 120s
    Web->>Web: Health Monitor: ALL FAIL
    Web->>User: Show Critical Alert
    Note right of User: "System Offline"
    
    Note over STM32,ESP32: Recovery Sequence Begins
    
    par Sensor Recovery
        loop Retry 3 Times
            STM32->>STM32: Wait 10s
            STM32->>STM32: Reinit SHT3X
            alt Init Success
                STM32->>STM32: sensor_error = false
                STM32->>STM32: Resume Measurements
            end
        end
    and WiFi Recovery
        ESP32->>ESP32: Reconnect WiFi
        ESP32->>ESP32: Wait for IP
        ESP32->>ESP32: WiFi Connected
        ESP32->>Broker: MQTT Reconnect
        Broker-->>ESP32: CONNACK
        ESP32->>STM32: UART: "MQTT CONNECTED\r\n"
    end
    
    STM32->>STM32: All Errors Cleared
    STM32->>STM32: Resume Normal Operation
    STM32->>ESP32: Send Valid Data
    ESP32->>Broker: Publish Data
    Broker->>Web: Deliver Data
    
    Web->>Web: Data Received
    Web->>Web: Update Health: All OK
    Web->>User: Show Success
    Note right of User: "System Recovered"
    
    Web->>Web: Generate Recovery Report
    Note right of Web: Downtime: 2m 15s<br/>Data Lost: 27 records<br/>Buffered: 27 records
```

---

*End of SEQUENCE_DIAGRAM_SYSTEM.md - Total: 15 comprehensive sequences*

