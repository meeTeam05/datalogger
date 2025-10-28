# Web Application - Architecture & UML Diagrams

Comprehensive architecture documentation for the DataLogger web monitoring application.

---

## 1. Complete System Architecture

```mermaid
graph TB
    subgraph Browser["Browser Environment"]
        subgraph UI["User Interface Layer"]
            HTML[index.html<br/>DOM Structure<br/>7 Pages]
            CSS[style.css<br/>Cyan Theme<br/>Dark/Light Mode]
            Icons[Feather Icons<br/>SVG Icons]
        end
        
        subgraph AppLayer["Application Layer"]
            AppJS[app.js<br/>3062 lines<br/>Main Logic]
            ChartJS[Chart.js<br/>Line Charts<br/>Animations]
            MQTTJS[mqtt.min.js<br/>WebSocket Client]
            FirebaseJS[Firebase SDK<br/>Realtime Database]
        end
        
        subgraph State["State Management"]
            GlobalVars[Global Variables<br/>isPeriodic<br/>isDeviceOn<br/>currentTemp/Humi]
            DataArrays[Data Arrays<br/>temperatureData<br/>humidityData<br/>logBuffer<br/>liveDataBuffer]
            ComponentHealth[Component Health<br/>SHT31 Status<br/>DS3231 Status]
        end
    end
    
    subgraph External["External Services"]
        MQTT[MQTT Broker<br/>Mosquitto<br/>127.0.0.1:8083<br/>WebSocket]
        Firebase[Firebase<br/>Realtime Database<br/>datalogger-8c5d5]
        Internet[Internet Time<br/>Browser Time API<br/>UTC+7]
    end
    
    subgraph Hardware["Hardware Layer"]
        ESP32[ESP32<br/>WiFi Bridge<br/>MQTT Client]
        STM32[STM32<br/>Data Logger<br/>Sensor Controller]
        Sensors[Sensors<br/>SHT31<br/>DS3231 RTC]
    end
    
    HTML --> AppJS
    CSS --> HTML
    Icons --> HTML
    
    AppJS --> ChartJS
    AppJS --> MQTTJS
    AppJS --> FirebaseJS
    
    AppJS --> GlobalVars
    AppJS --> DataArrays
    AppJS --> ComponentHealth
    
    MQTTJS --> MQTT
    FirebaseJS --> Firebase
    AppJS --> Internet
    
    MQTT --> ESP32
    ESP32 --> STM32
    STM32 --> Sensors
    
    ESP32 -.UART.-> STM32
    
    style Browser fill:#06B6D4,stroke:#0891B2,stroke-width:3px,color:#fff
    style External fill:#F97316,stroke:#EA580C,stroke-width:2px,color:#fff
    style Hardware fill:#8B5CF6,stroke:#7C3AED,stroke-width:2px,color:#fff
    style AppJS fill:#10B981,stroke:#059669,stroke-width:2px,color:#fff
```

---

## 2. Application Class Diagram

```mermaid
classDiagram
    class WebApplication {
        -isPeriodic: boolean
        -isDeviceOn: boolean
        -isMqttConnected: boolean
        -isFirebaseConnected: boolean
        -currentTemp: float
        -currentHumi: float
        -lastReadingTimestamp: number
        +initialize()
        +initializeFirebase()
        +initializeMQTT()
        +initializeCharts()
    }
    
    class MQTTManager {
        -mqttClient: MQTTClient
        -MQTT_CONFIG: object
        +connect()
        +subscribe(topic, qos)
        +publish(topic, message, qos)
        +handleMessage(topic, payload)
        +updateStatus(connected)
        +requestStateSync()
    }
    
    class FirebaseManager {
        -firebaseDb: Database
        -FIREBASE_CONFIG: object
        +initialize()
        +saveRecord(data)
        +loadHistory(dateRange)
        +testPermissions()
        +updateStatus(connected)
    }
    
    class ChartManager {
        -tempChart: Chart
        -humiChart: Chart
        -temperatureData: array
        -humidityData: array
        -maxDataPoints: number
        +initializeCharts()
        +pushTemperature(value, update, timestamp)
        +pushHumidity(value, update, timestamp)
        +updateChartStats(type)
        +clearChartData()
        +ensureChartsInitialized()
    }
    
    class UIController {
        +updateCurrentDisplay()
        +updateMQTTStatus(connected)
        +updateFirebaseStatus(connected)
        +updateDeviceButton(isOn)
        +updatePeriodicButton(isOn)
        +renderLiveDataTable()
        +renderDataManagementTable(data)
        +renderFullConsole()
    }
    
    class CommandHandler {
        +handleDeviceToggle()
        +handlePeriodicToggle()
        +handleSingleRead()
        +handleIntervalConfig()
        +handleTimeSync()
        +handleManualTimeSet()
    }
    
    class StateManager {
        -stateSync: object
        -deviceClockMs: number
        -deviceClockSetAtMs: number
        +syncUIWithHardwareState(state)
        +parseStateMessage(message)
        +startDeviceClockTicker()
    }
    
    class DataManager {
        -liveDataBuffer: array
        -filteredDataCache: array
        +addToLiveDataTable(data)
        +applyDataFilters()
        +applySorting(data, sortBy)
        +updateDataStatistics(data)
        +exportFilteredData()
        +resetDataFilters()
    }
    
    class ComponentHealthMonitor {
        -componentHealth: object
        +updateComponentHealth(component, isHealthy)
        +updateComponentCard(component)
        +getTimeAgo(timestamp)
    }
    
    class LoggingSystem {
        -logBuffer: array
        -maxLogsInMemory: number
        -logFilterType: string
        +addStatus(message, type)
        +updateConsoleDisplay(logEntry)
        +renderFullConsole()
        +filterLogs(type)
        +clearLogs()
        +exportLogs()
    }
    
    class TimeManager {
        -timePickerYear: number
        -timePickerMonth: number
        -timePickerDay: number
        -timePickerHour: number
        -timePickerMinute: number
        -timePickerSecond: number
        +renderCalendar()
        +updateTimeDisplay()
        +adjustTime(unit, delta)
    }
    
    class NavigationManager {
        +bindMenuNavigation()
        +switchPage(page)
        +initializePage(page)
        +bindCalendarButtons()
        +bindDataManagementButtons()
        +bindLogsButtons()
    }
    
    WebApplication --> MQTTManager
    WebApplication --> FirebaseManager
    WebApplication --> ChartManager
    WebApplication --> UIController
    WebApplication --> CommandHandler
    WebApplication --> StateManager
    WebApplication --> DataManager
    WebApplication --> ComponentHealthMonitor
    WebApplication --> LoggingSystem
    WebApplication --> TimeManager
    WebApplication --> NavigationManager
    
    MQTTManager --> StateManager
    MQTTManager --> DataManager
    MQTTManager --> ComponentHealthMonitor
    
    FirebaseManager --> DataManager
    
    ChartManager --> UIController
    
    CommandHandler --> MQTTManager
    CommandHandler --> TimeManager
    
    DataManager --> FirebaseManager
    DataManager --> UIController
    
    LoggingSystem --> UIController
```

---

## 3. MQTT Topics & Message Architecture

```mermaid
graph TB
    subgraph WebApp["Web Application"]
        Publisher[MQTT Publisher]
        Subscriber[MQTT Subscriber]
    end
    
    subgraph Broker["MQTT Broker (Mosquitto)<br/>ws://127.0.0.1:8083/mqtt"]
        Topic1[datalogger/stm32/command<br/>QoS: 1<br/>Web → STM32]
        Topic2[datalogger/esp32/relay/control<br/>QoS: 1<br/>Web → ESP32]
        Topic3[datalogger/esp32/system/state<br/>QoS: 1<br/>Bidirectional]
        Topic4[datalogger/stm32/single/data<br/>QoS: 1<br/>STM32 → Web]
        Topic5[datalogger/stm32/periodic/data<br/>QoS: 1<br/>STM32 → Web]
    end
    
    subgraph Messages["Message Formats"]
        Cmd1[SINGLE<br/>PERIODIC ON/OFF<br/>SET TIME timestamp<br/>SET INTERVAL seconds]
        Cmd2[RELAY ON<br/>RELAY OFF]
        Cmd3[REQUEST<br/>JSON: device/periodic]
        Data1[JSON: temperature<br/>humidity<br/>timestamp]
        Data2[JSON: temperature<br/>humidity<br/>timestamp<br/>mode: periodic]
    end
    
    Publisher --> Topic1
    Publisher --> Topic2
    Publisher --> Topic3
    
    Topic4 --> Subscriber
    Topic5 --> Subscriber
    Topic3 --> Subscriber
    
    Topic1 -.->|Command String| Cmd1
    Topic2 -.->|Command String| Cmd2
    Topic3 -.->|Request/JSON| Cmd3
    Topic4 -.->|JSON Data| Data1
    Topic5 -.->|JSON Data| Data2
    
    style WebApp fill:#06B6D4,stroke:#0891B2,stroke-width:2px
    style Broker fill:#8B5CF6,stroke:#7C3AED,stroke-width:2px
    style Messages fill:#10B981,stroke:#059669,stroke-width:2px
```

---

## 4. Firebase Database Structure

```mermaid
graph TB
    subgraph Firebase["Firebase Realtime Database<br/>datalogger-8c5d5"]
        Root[Root /]
        
        subgraph Readings["readings/"]
            Date1[2025-01-20/]
            Date2[2025-01-21/]
            Date3[YYYY-MM-DD/]
            
            subgraph Record["timestamp_id/"]
                Temp[temp: 25.3]
                Humi[humi: 65.2]
                Mode[mode: periodic]
                Sensor[sensor: SHT31]
                Status[status: success]
                Error[error: null]
                Time[time: 1704067200]
                Device[device: ESP32_01]
                Created[created_at: timestamp]
            end
        end
        
        subgraph Test["test/"]
            Connection[connection/<br/>timestamp<br/>message]
        end
    end
    
    Root --> Readings
    Root --> Test
    
    Readings --> Date1
    Readings --> Date2
    Readings --> Date3
    
    Date1 --> Record
    Date2 --> Record
    Date3 --> Record
    
    Record --> Temp
    Record --> Humi
    Record --> Mode
    Record --> Sensor
    Record --> Status
    Record --> Error
    Record --> Time
    Record --> Device
    Record --> Created
    
    style Firebase fill:#F97316,stroke:#EA580C,stroke-width:3px
    style Readings fill:#06B6D4,stroke:#0891B2,stroke-width:2px
    style Test fill:#10B981,stroke:#059669,stroke-width:2px
```

---

## 5. State Management Architecture

```mermaid
stateDiagram-v2
    [*] --> Initializing
    
    state Initializing {
        [*] --> LoadingFirebase
        LoadingFirebase --> FirebaseReady: Init Success
        LoadingFirebase --> FirebaseError: Init Failed
        
        FirebaseReady --> ConnectingMQTT
        FirebaseError --> ConnectingMQTT
        
        ConnectingMQTT --> MQTTReady: Connected
        ConnectingMQTT --> MQTTError: Connection Failed
        
        MQTTReady --> InitializingCharts
        MQTTError --> InitializingCharts
        
        InitializingCharts --> ChartsReady: Canvas Found
        InitializingCharts --> ChartsRetry: Canvas Not Found
        
        ChartsRetry --> InitializingCharts: Retry < 10
        ChartsRetry --> ChartsFailed: Retry >= 10
        
        ChartsReady --> [*]
        ChartsFailed --> [*]
    end
    
    Initializing --> Ready: All Components Loaded
    
    state Ready {
        [*] --> Idle
        
        Idle --> DeviceControl: User Clicks Device Button
        DeviceControl --> DeviceON: Toggle ON
        DeviceControl --> DeviceOFF: Toggle OFF
        
        state DeviceON {
            [*] --> Running
            Running --> PeriodicControl: User Clicks Periodic
            PeriodicControl --> PeriodicON: Enable
            PeriodicControl --> Running: Disable
            
            state PeriodicON {
                [*] --> Measuring
                Measuring --> DataReceived: MQTT Message
                DataReceived --> ProcessingData
                ProcessingData --> SavingFirebase
                SavingFirebase --> UpdatingCharts
                UpdatingCharts --> Measuring
            }
        }
        
        state DeviceOFF {
            [*] --> Stopped
            Stopped --> Idle: No Activity
        }
        
        DeviceON --> DeviceOFF: Toggle OFF
        DeviceOFF --> DeviceON: Toggle ON
        
        Idle --> SingleRead: User Clicks Single
        SingleRead --> WaitingData: Command Sent
        WaitingData --> ProcessingData: Data Received
        ProcessingData --> Idle: Complete
        
        Idle --> TimeSettings: User Opens Time Page
        TimeSettings --> SyncingTime: Internet Sync / Manual Set
        SyncingTime --> Idle: Time Synced
        
        Idle --> DataManagement: User Opens Data Page
        DataManagement --> FilteringData: Apply Filters
        FilteringData --> LoadingFirebase: Query Database
        LoadingFirebase --> RenderingTable: Data Loaded
        RenderingTable --> Idle: Complete
    }
    
    Ready --> Error: Critical Failure
    Error --> Ready: Reconnect / Retry
    
    state Error {
        [*] --> MQTTDisconnected
        [*] --> FirebaseDisconnected
        [*] --> SensorFailed
        [*] --> RTCFailed
        
        MQTTDisconnected --> Reconnecting: Auto-retry 2s
        FirebaseDisconnected --> Reconnecting: Auto-retry
        SensorFailed --> [*]: Continue with Errors
        RTCFailed --> [*]: Use Local Time
        
        Reconnecting --> [*]: Success
    }
```

---

## 6. UI Component Hierarchy

```mermaid
graph TB
    subgraph Body["<body>"]
        subgraph Sidebar["Sidebar Navigation"]
            SidebarHeader[Sidebar Header<br/>DataLogger Logo]
            Menu[Menu Items<br/>- Dashboard<br/>- Live Data<br/>- Device Settings<br/>- Time Settings<br/>- Data Management<br/>- Logs<br/>- Config]
        end
        
        subgraph ContentArea["Content Area"]
            Header[Header<br/>MQTT Status Badge<br/>Firebase Status Badge]
            
            subgraph MainContent["Main Content"]
                Dashboard[Dashboard Page<br/>- Quick Actions<br/>- Current Reading<br/>- Console Preview<br/>- Temperature Chart<br/>- Humidity Chart]
                
                LiveData[Live Data Page<br/>- Filter Controls<br/>- Data Table<br/>- Export Button]
                
                DeviceSettings[Device Settings Page<br/>- SHT31 Component Card<br/>- DS3231 Component Card<br/>- Reserved Slots]
                
                TimeSettings[Time Settings Page<br/>- Calendar Widget<br/>- Time Picker<br/>- Sync/Set Buttons]
                
                DataMgmt[Data Management Page<br/>- Statistics Summary<br/>- Advanced Filters<br/>- Data Table<br/>- Export Button]
                
                Logs[Logs Page<br/>- Search Input<br/>- Filter Buttons<br/>- Console Display<br/>- Export/Clear Buttons]
                
                Config[Config Page<br/>- MQTT Settings<br/>- Firebase Settings<br/>- Display Preferences<br/>- Chart Settings<br/>- Data Settings]
            end
            
            Footer[Footer<br/>System Time Display]
        end
        
        subgraph Modals["Modal Dialogs"]
            IntervalModal[Interval Modal<br/>- Minute Picker<br/>- Second Picker<br/>- Set/Cancel Buttons]
        end
    end
    
    Body --> Sidebar
    Body --> ContentArea
    Body --> Modals
    
    ContentArea --> Header
    ContentArea --> MainContent
    ContentArea --> Footer
    
    MainContent --> Dashboard
    MainContent --> LiveData
    MainContent --> DeviceSettings
    MainContent --> TimeSettings
    MainContent --> DataMgmt
    MainContent --> Logs
    MainContent --> Config
    
    style Body fill:#F0F9FF
    style Sidebar fill:#06B6D4,color:#fff
    style ContentArea fill:#ECFEFF
    style MainContent fill:#BAE6FD
    style Modals fill:#22D3EE,color:#fff
```

---

## 7. Data Flow Architecture

```mermaid
graph LR
    subgraph Input["Data Input Sources"]
        MQTT_In[MQTT Messages<br/>singleData<br/>periodicData<br/>systemState]
        Firebase_Load[Firebase Queries<br/>Load History<br/>Load Filtered Data]
        User_Input[User Actions<br/>Button Clicks<br/>Form Inputs<br/>Time Settings]
    end
    
    subgraph Processing["Processing Layer"]
        MessageHandler[handleMQTTMessage<br/>Parse JSON<br/>Validate Data<br/>Check Status]
        
        DataValidator[Data Validation<br/>Sensor Check<br/>RTC Check<br/>Error Detection]
        
        StateUpdater[State Updates<br/>Update Global Vars<br/>Update Component Health<br/>Sync UI State]
        
        DataAggregator[Data Aggregation<br/>Push to Arrays<br/>Calculate Stats<br/>Apply Filters]
    end
    
    subgraph Storage["Data Storage"]
        Memory[In-Memory Storage<br/>temperatureData<br/>humidityData<br/>logBuffer<br/>liveDataBuffer<br/>filteredDataCache]
        
        Firebase_Save[Firebase Database<br/>readings/date/id<br/>Persistent Storage]
        
        LocalStorage[Browser LocalStorage<br/>Settings<br/>Preferences]
    end
    
    subgraph Output["Data Output Destinations"]
        Charts[Chart.js Visualization<br/>Line Charts<br/>Animated Updates]
        
        Tables[HTML Tables<br/>Live Data Table<br/>Data Management Table]
        
        Console_Log[Console Display<br/>Log Preview<br/>Full Console]
        
        UI_Updates[UI Updates<br/>Current Display<br/>Status Badges<br/>Component Cards]
        
        Exports[Data Exports<br/>CSV Downloads<br/>Log Exports]
    end
    
    MQTT_In --> MessageHandler
    Firebase_Load --> DataAggregator
    User_Input --> StateUpdater
    
    MessageHandler --> DataValidator
    DataValidator --> StateUpdater
    StateUpdater --> DataAggregator
    
    DataAggregator --> Memory
    DataAggregator --> Firebase_Save
    StateUpdater --> LocalStorage
    
    Memory --> Charts
    Memory --> Tables
    Memory --> Console_Log
    Memory --> UI_Updates
    Memory --> Exports
    
    Firebase_Save -.Sync.-> Firebase_Load
    
    style Input fill:#10B981,stroke:#059669,stroke-width:2px,color:#fff
    style Processing fill:#06B6D4,stroke:#0891B2,stroke-width:2px,color:#fff
    style Storage fill:#F97316,stroke:#EA580C,stroke-width:2px,color:#fff
    style Output fill:#8B5CF6,stroke:#7C3AED,stroke-width:2px,color:#fff
```

---

## 8. Event Handling System

```mermaid
sequenceDiagram
    participant DOM
    participant EventListener
    participant Handler
    participant State
    participant UI
    
    Note over DOM,UI: User Interaction Events
    
    DOM->>EventListener: User clicks button
    EventListener->>Handler: Dispatch event
    
    alt Device Button
        Handler->>State: Check MQTT connection
        
        alt Connected
            Handler->>State: Toggle isDeviceOn
            Handler->>MQTT: Publish command
            Handler->>State: Update button state
            Handler->>UI: Update button style
        else Not Connected
            Handler->>UI: Show error message
        end
        
    else Periodic Button
        Handler->>State: Check device state
        
        alt Device ON
            Handler->>State: Toggle isPeriodic
            Handler->>MQTT: Publish command
            Handler->>State: Update button state
            Handler->>UI: Update button style
        else Device OFF
            Handler->>UI: Show warning
        end
        
    else Single Button
        Handler->>State: Check device state
        Handler->>MQTT: Publish SINGLE command
        Handler->>Logging: Add status message
        
    else Time Sync Button
        Handler->>Browser: Get current time
        Handler->>State: Update time picker
        Handler->>UI: Render calendar
        Handler->>MQTT: Publish SET TIME
        Handler->>State: Update device clock
        Handler->>UI: Start clock ticker
    end
    
    Note over DOM,UI: MQTT Message Events
    
    MQTT->>EventListener: Message received
    EventListener->>Handler: handleMQTTMessage
    
    Handler->>State: Parse message
    
    alt System State
        Handler->>State: Update isDeviceOn, isPeriodic
        Handler->>UI: Update button styles
        
    else Sensor Data
        Handler->>State: Update currentTemp, currentHumi
        Handler->>UI: Update display
        Handler->>Charts: Push data
        Handler->>Firebase: Save record
        Handler->>UI: Update tables
    end
    
    Note over DOM,UI: Navigation Events
    
    DOM->>EventListener: Menu item clicked
    EventListener->>Handler: switchPage
    
    Handler->>UI: Hide all pages
    Handler->>UI: Show selected page
    Handler->>Handler: Initialize page components
    Handler->>UI: Bind page-specific events
    Handler->>UI: Refresh icons
```

---

## 9. Chart Integration Architecture

```mermaid
graph TB
    subgraph ChartSystem["Chart.js Integration"]
        subgraph Config["Chart Configuration"]
            TempConfig[Temperature Chart Config<br/>- Type: line<br/>- Color: #06B6D4<br/>- Tension: 0.4<br/>- Animation: 750ms]
            
            HumiConfig[Humidity Chart Config<br/>- Type: line<br/>- Color: #22D3EE<br/>- Tension: 0.4<br/>- Animation: 750ms]
        end
        
        subgraph DataManagement["Data Management"]
            TempArray[temperatureData[]<br/>- Max: 50 points<br/>- FIFO buffer<br/>- {value, time}]
            
            HumiArray[humidityData[]<br/>- Max: 50 points<br/>- FIFO buffer<br/>- {value, time}]
            
            Push[Push Functions<br/>- pushTemperature()<br/>- pushHumidity()<br/>- Lazy init if needed]
        end
        
        subgraph Charts["Chart Instances"]
            TempChart[tempChart<br/>- Canvas: #tempChart<br/>- Labels: times<br/>- Data: values<br/>- Update: 'active']
            
            HumiChart[humiChart<br/>- Canvas: #humiChart<br/>- Labels: times<br/>- Data: values<br/>- Update: 'active']
        end
        
        subgraph Stats["Statistics Display"]
            Calculate[Calculate Stats<br/>- min = Math.min()<br/>- max = Math.max()<br/>- avg = sum/count]
            
            UpdateUI[Update UI Elements<br/>- #tempMin, #tempMax, #tempAvg<br/>- #humiMin, #humiMax, #humiAvg]
        end
        
        subgraph Init["Initialization"]
            Retry[Retry Logic<br/>- Max 10 attempts<br/>- 500ms interval<br/>- Check canvas ready]
            
            LazyInit[Lazy Initialization<br/>- ensureChartsInitialized()<br/>- Called on first push<br/>- Fallback mechanism]
        end
    end
    
    subgraph DataSource["Data Sources"]
        MQTT[MQTT Messages<br/>periodicData topic]
        Firebase[Firebase History<br/>loadHistory()]
    end
    
    subgraph Display["Visual Output"]
        Canvas1[Canvas Element<br/>#tempChart]
        Canvas2[Canvas Element<br/>#humiChart]
        StatsDisplay[Stats Elements<br/>Min/Max/Avg values]
    end
    
    TempConfig --> TempChart
    HumiConfig --> HumiChart
    
    MQTT --> Push
    Firebase --> Push
    
    Push --> TempArray
    Push --> HumiArray
    
    TempArray --> TempChart
    HumiArray --> HumiChart
    
    TempChart --> Canvas1
    HumiChart --> Canvas2
    
    TempChart --> Calculate
    HumiChart --> Calculate
    
    Calculate --> UpdateUI
    UpdateUI --> StatsDisplay
    
    Retry --> TempChart
    Retry --> HumiChart
    LazyInit --> TempChart
    LazyInit --> HumiChart
    
    style ChartSystem fill:#06B6D4,stroke:#0891B2,stroke-width:3px
    style DataSource fill:#10B981,stroke:#059669,stroke-width:2px
    style Display fill:#F59E0B,stroke:#D97706,stroke-width:2px
```

---

## 10. Settings & Configuration Management

```mermaid
graph TB
    subgraph Settings["Settings Management"]
        subgraph MQTT["MQTT Configuration"]
            MQTTHost[Host: 127.0.0.1]
            MQTTPort[Port: 8083]
            MQTTPath[Path: /mqtt]
            MQTTUser[Username: DataLogger]
            MQTTPass[Password: datalogger]
            MQTTClient[Client ID: web_client_1]
        end
        
        subgraph Firebase["Firebase Configuration"]
            FBApiKey[API Key]
            FBProject[Project ID: datalogger-8c5d5]
            FBDbUrl[Database URL]
        end
        
        subgraph Display["Display Preferences"]
            TempUnit[Temperature Unit<br/>C / F / K<br/>Default: C]
            TimeFormat[Time Format<br/>24h / 12h<br/>Default: 24h]
            DateFormat[Date Format<br/>DD/MM/YYYY<br/>MM/DD/YYYY<br/>YYYY-MM-DD]
        end
        
        subgraph Chart["Chart Settings"]
            MaxPoints[Max Data Points<br/>Default: 50<br/>Range: 10-200]
            UpdateInterval[Update Interval (s)<br/>Default: 1<br/>Range: 1-60]
            SkipErrors[Skip Error Readings<br/>Checkbox<br/>Default: false]
        end
        
        subgraph Data["Data Management"]
            DefaultInterval[Default Interval (s)<br/>Default: 5<br/>Min: 1]
            RetentionDays[Retention Days<br/>Default: 30<br/>Min: 1]
            AutoSave[Auto-save to Firebase<br/>Checkbox<br/>Default: true]
        end
    end
    
    subgraph Storage["Storage Mechanism"]
        LocalStorage[Browser LocalStorage<br/>- tempUnit<br/>- timeFormat<br/>- dateFormat<br/>- chartMaxPoints<br/>- chartUpdateInterval<br/>- chartSkipErrors<br/>- dataDefaultInterval<br/>- dataRetentionDays<br/>- dataAutoSave]
        
        ConfigObj[MQTT_CONFIG object<br/>FIREBASE_CONFIG object<br/>In-memory during session]
    end
    
    subgraph Actions["Settings Actions"]
        Load[loadSettingsPage()<br/>Read from localStorage<br/>Populate form inputs]
        
        Save[saveSettings()<br/>Write to localStorage<br/>Apply changes]
        
        Export[exportSettings()<br/>Create JSON file<br/>Download to browser]
        
        Import[importSettings()<br/>Read JSON file<br/>Restore settings]
        
        Restore[restoreDefaults()<br/>Reset to factory values<br/>Clear localStorage]
    end
    
    Settings --> LocalStorage
    Settings --> ConfigObj
    
    LocalStorage --> Load
    Load --> Settings
    
    Settings --> Save
    Save --> LocalStorage
    
    Settings --> Export
    Export --> JSONFile[settings.json]
    
    JSONFile --> Import
    Import --> Settings
    
    Restore --> Settings
    
    style Settings fill:#8B5CF6,stroke:#7C3AED,stroke-width:3px,color:#fff
    style Storage fill:#F97316,stroke:#EA580C,stroke-width:2px,color:#fff
    style Actions fill:#10B981,stroke:#059669,stroke-width:2px,color:#fff
```

---

## 11. Complete Deployment Architecture

```mermaid
C4Deployment
    title Web Application Deployment Architecture
    
    Deployment_Node(browser, "User Browser", "Chrome/Firefox/Safari") {
        Container(webapp, "Web Application", "HTML/CSS/JavaScript", "Single-page monitoring dashboard")
        Container(mqtt_client, "MQTT Client", "mqtt.min.js", "WebSocket MQTT client")
        Container(firebase_sdk, "Firebase SDK", "firebase-app-compat.js", "Realtime database client")
        Container(chartjs, "Chart.js", "chart.min.js", "Data visualization library")
    }
    
    Deployment_Node(local_network, "Local Network", "192.168.1.x") {
        Deployment_Node(mqtt_server, "MQTT Server", "Mosquitto") {
            Container(broker, "MQTT Broker", "Mosquitto 2.x", "Message broker with WebSocket support")
        }
        
        Deployment_Node(esp32_node, "ESP32 Device", "ESP32-WROOM-32") {
            Container(wifi_bridge, "WiFi Bridge", "ESP-IDF", "MQTT client + UART bridge")
        }
        
        Deployment_Node(stm32_node, "STM32 Device", "STM32F103C8T6") {
            Container(data_logger, "Data Logger", "STM32 HAL", "Sensor controller + SD buffer")
        }
    }
    
    Deployment_Node(cloud, "Cloud Services", "Google Cloud") {
        Deployment_Node(firebase, "Firebase", "Google Firebase") {
            ContainerDb(rtdb, "Realtime Database", "Firebase RTDB", "JSON document store")
        }
    }
    
    Rel(webapp, mqtt_client, "Uses", "WebSocket")
    Rel(webapp, firebase_sdk, "Uses", "HTTPS API")
    Rel(webapp, chartjs, "Uses", "JavaScript API")
    
    Rel(mqtt_client, broker, "Connects", "ws://127.0.0.1:8083/mqtt")
    Rel(firebase_sdk, rtdb, "Syncs", "HTTPS + WebSocket")
    
    Rel(broker, wifi_bridge, "Publishes/Subscribes", "MQTT v5.0")
    Rel(wifi_bridge, data_logger, "Communicates", "UART 115200")
    
    UpdateLayoutConfig($c4ShapeInRow="3", $c4BoundaryInRow="2")
```

---

## System Characteristics

### Architecture Patterns
- **Frontend**: Single-page application (SPA) with dynamic routing
- **Communication**: Publish-Subscribe (MQTT), Real-time sync (Firebase)
- **State Management**: Global variables + component-level state
- **Data Flow**: Unidirectional (input → processing → storage → output)
- **UI Pattern**: Component-based with event-driven updates

### Technology Stack
- **Core**: Vanilla JavaScript (ES6+), HTML5, CSS3
- **Libraries**:
  - Chart.js 3.9.1 (data visualization)
  - MQTT.js (WebSocket MQTT client)
  - Firebase 9.22.0 (realtime database)
  - Feather Icons (SVG icons)
  - TailwindCSS CDN (utility-first CSS)

### Scalability Considerations
- **Data Buffer Limits**: 50 points (charts), 100 entries (logs/live data)
- **Firebase Structure**: Daily partitioning (readings/YYYY-MM-DD/)
- **MQTT QoS**: Level 1 (at-least-once delivery)
- **Chart Performance**: 750ms animations, debounced updates
- **Memory Management**: FIFO buffers with automatic cleanup

### Security Features
- **MQTT Authentication**: Username/password (DataLogger/datalogger)
- **Firebase Rules**: Database access control (must be configured)
- **WebSocket**: Local network only (127.0.0.1)
- **No External Exposure**: Web app accesses local broker only
- **Data Validation**: JSON parsing with error handling

### Responsive Design
- **Layout**: Fixed 260px sidebar + fluid content area
- **Theme Support**: Auto dark/light mode via CSS media query
- **Color Scheme**: Cyan/Aqua primary (#06B6D4) with semantic colors
- **Breakpoints**: Mobile-friendly (grid-template-columns: auto-fit)
- **Icon System**: SVG icons (Feather) with automatic replacement

### Error Recovery Mechanisms
- **MQTT**: Auto-reconnect every 2 seconds on disconnect
- **Firebase**: Auto-reconnect via .info/connected listener
- **Charts**: Retry init 10 times (500ms interval), lazy init fallback
- **Sensor Failure**: Continue operation, log errors, mark unhealthy
- **RTC Failure**: Use browser time as fallback
- **Network Failure**: Queue operations, retry on reconnect

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-XX  
**Total Architecture Diagrams**: 11  
**Coverage**: Complete system architecture from UI to deployment
