# Web Application - Flow Diagrams

Comprehensive flowcharts documenting all web application processes in the DataLogger monitoring system.

---

## 1. Application Initialization Flow

```mermaid
flowchart TB
    Start([Page Load]) --> InitFeather[Initialize Feather Icons]
    InitFeather --> InitVars[Initialize Global Variables<br/>isPeriodic = false<br/>isDeviceOn = false<br/>temperatureData = []<br/>humidityData = []]
    
    InitVars --> StartFirebase[Initialize Firebase]
    StartFirebase --> FirebaseCheck{Firebase Config<br/>Complete?}
    
    FirebaseCheck -->|Yes| FirebaseInit[firebase.initializeApp<br/>Get Database Reference]
    FirebaseCheck -->|No| FirebaseSkip[Skip Firebase<br/>Update Status: Disconnected]
    
    FirebaseInit --> FirebaseTest[Test Write Permission<br/>test/connection]
    FirebaseTest --> FirebaseTestResult{Write<br/>Success?}
    
    FirebaseTestResult -->|Yes| FirebaseOK[Update Status: Connected<br/>Listen to .info/connected]
    FirebaseTestResult -->|No| FirebaseError[Update Status: Error<br/>Show Permission Error]
    
    FirebaseOK --> StartMQTT[Initialize MQTT Client]
    FirebaseError --> StartMQTT
    FirebaseSkip --> StartMQTT
    
    StartMQTT --> MQTTConnect[mqtt.connect<br/>Host: 127.0.0.1:8083<br/>Path: /mqtt<br/>Username: DataLogger]
    
    MQTTConnect --> MQTTEvents[Register Event Handlers<br/>connect, message, error,<br/>offline, reconnect, close]
    
    MQTTEvents --> WaitConnect{MQTT<br/>Connected?}
    
    WaitConnect -->|No| RetryMQTT[Reconnect After 2s]
    RetryMQTT --> WaitConnect
    
    WaitConnect -->|Yes| Subscribe[Subscribe Topics<br/>systemState<br/>singleData<br/>periodicData]
    
    Subscribe --> RequestSync[Publish REQUEST<br/>to systemState<br/>After 500ms delay]
    
    RequestSync --> InitCharts[Initialize Charts<br/>Temperature & Humidity<br/>Chart.js Line Charts]
    
    InitCharts --> ChartCheck{Chart Canvas<br/>Ready?}
    
    ChartCheck -->|No| ChartRetry[Retry After 500ms<br/>Max 10 attempts]
    ChartRetry --> ChartRetryCount{Retry<br/>< 10?}
    ChartRetryCount -->|Yes| ChartCheck
    ChartRetryCount -->|No| ChartFail[Chart Init Failed<br/>Log Error]
    
    ChartCheck -->|Yes| CreateCharts[Create Chart Instances<br/>Configure Options<br/>Set Colors & Animations]
    
    CreateCharts --> BindNav[Bind Navigation Menu<br/>Dashboard, Live Data,<br/>Settings, Time, Data,<br/>Logs, Config]
    
    BindNav --> BindButtons[Bind Quick Action Buttons<br/>Device, Periodic, Single,<br/>Interval, Load History,<br/>Clear Charts]
    
    BindButtons --> StartClock[Start Clock Ticker<br/>Update Footer Time<br/>Every 1 second]
    
    StartClock --> Ready([Ready for User Interaction])
    ChartFail --> Ready
    
    style Start fill:#10B981
    style Ready fill:#10B981
    style FirebaseOK fill:#06B6D4
    style MQTTConnect fill:#8B5CF6
    style CreateCharts fill:#F59E0B
    style RequestSync fill:#EF4444
```

---

## 2. MQTT Message Reception & Processing Flow

```mermaid
flowchart TB
    MsgReceive([MQTT Message Received]) --> ParseTopic{Topic<br/>Type?}
    
    ParseTopic -->|systemState| ParseState[Parse State Message<br/>Try JSON first<br/>Fallback to text]
    ParseTopic -->|singleData| ParseData[Parse JSON Payload<br/>Extract temperature<br/>humidity, timestamp]
    ParseTopic -->|periodicData| ParseData
    
    ParseState --> StateCheck{Valid<br/>State?}
    StateCheck -->|No| IgnoreMsg[Ignore Message]
    StateCheck -->|Yes| UpdateState[Update isDeviceOn<br/>Update isPeriodic]
    
    UpdateState --> UpdateDeviceBtn[Update Device Button<br/>ON: green + "Device ON"<br/>OFF: gray + "Device OFF"]
    
    UpdateDeviceBtn --> UpdatePeriodicBtn[Update Periodic Button<br/>ON: green + "Periodic ON"<br/>OFF: gray + "Periodic OFF"]
    
    UpdatePeriodicBtn --> LogSync[Log Sync Status<br/>"State synced: Device=ON/OFF..."]
    LogSync --> SyncEnd([Sync Complete])
    
    ParseData --> CheckTopic{Topic =<br/>periodicData?}
    
    CheckTopic -->|No - singleData| DataProcess[Process Data<br/>mode = "single"]
    CheckTopic -->|Yes| CheckActive{Device ON<br/>AND<br/>Periodic ON?}
    
    CheckActive -->|No| IgnorePeriodic[Ignore Periodic Message<br/>Log: "Ignored periodic<br/>(inactive)"]
    IgnorePeriodic --> DataEnd([End])
    
    CheckActive -->|Yes| DataProcess
    
    DataProcess --> CheckSensor{Sensor<br/>Failed?<br/>temp=0<br/>humi=0}
    
    CheckSensor -->|Yes| LogSensorFail[Log ERROR<br/>"SHT31 sensor failed"<br/>Add WARNING]
    CheckSensor -->|No| CheckRTC{RTC<br/>Failed?<br/>time=0}
    
    LogSensorFail --> CheckRTC
    
    CheckRTC -->|Yes| LogRTCFail[Log ERROR<br/>"DS3231 RTC failed"<br/>Add WARNING]
    CheckRTC -->|No| UpdateCurrent[Update Current Display<br/>currentTemp = data.temp<br/>currentHumi = data.humi<br/>lastReadingTimestamp]
    
    LogRTCFail --> UpdateCurrent
    
    UpdateCurrent --> UpdateDisplay[Update UI Elements<br/>#currentTemp<br/>#currentHumi<br/>#lastUpdate]
    
    UpdateDisplay --> SaveFB{Firebase<br/>Connected?}
    
    SaveFB -->|Yes| SaveRecord[Save to Firebase<br/>readings/date/id<br/>temp, humi, mode,<br/>sensor, status, time]
    SaveFB -->|No| SkipFB[Skip Firebase Save]
    
    SaveRecord --> UpdateHealth[Update Component Health<br/>SHT31: !sensorFailed<br/>DS3231: !rtcFailed]
    SkipFB --> UpdateHealth
    
    UpdateHealth --> UpdateCards[Update Device Settings Cards<br/>Show component status<br/>Last update time]
    
    UpdateCards --> CheckMode{Mode =<br/>periodic<br/>AND<br/>Valid Data?}
    
    CheckMode -->|Yes| PushCharts[Push to Chart Arrays<br/>pushTemperature<br/>pushHumidity<br/>Update chart.js]
    CheckMode -->|No| SkipCharts[Skip Chart Update]
    
    PushCharts --> UpdateStats[Update Chart Stats<br/>Min, Max, Avg<br/>Calculate from arrays]
    
    UpdateStats --> AddLiveTable[Add to Live Data Table<br/>liveDataBuffer.unshift<br/>Max 100 entries<br/>renderLiveDataTable]
    
    SkipCharts --> AddLiveTable
    
    AddLiveTable --> DataEnd
    
    style MsgReceive fill:#10B981
    style DataEnd fill:#10B981
    style SyncEnd fill:#10B981
    style ParseState fill:#8B5CF6
    style ParseData fill:#8B5CF6
    style LogSensorFail fill:#EF4444
    style LogRTCFail fill:#EF4444
    style PushCharts fill:#06B6D4
    style SaveRecord fill:#F97316
```

---

## 3. Device Control (Relay) Flow

```mermaid
flowchart TB
    ClickDevice([User Clicks Device Button]) --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"]
    CheckMQTT -->|Yes| ToggleState[Toggle isDeviceOn<br/>isDeviceOn = !isDeviceOn]
    
    ToggleState --> BuildCmd{isDeviceOn?}
    
    BuildCmd -->|true| CmdOn[command = "RELAY ON"]
    BuildCmd -->|false| CmdOff[command = "RELAY OFF"]
    
    CmdOn --> Publish[Publish to MQTT<br/>Topic: relayControl<br/>QoS: 1]
    CmdOff --> Publish
    
    Publish --> UpdateBtn{isDeviceOn?}
    
    UpdateBtn -->|true| BtnOn[Set Button Style<br/>className = "btn-success"<br/>text = "Device ON"]
    UpdateBtn -->|false| BtnOff[Set Button Style<br/>className = "btn-secondary"<br/>text = "Device OFF"]
    
    BtnOn --> LogCmd[Log to Console<br/>"Device RELAY ON/OFF sent"]
    BtnOff --> CheckStop{isDeviceOn<br/>= false<br/>AND<br/>isPeriodic?}
    
    CheckStop -->|Yes| StopPeriodic[Stop Periodic Mode<br/>isPeriodic = false]
    CheckStop -->|No| LogCmd
    
    StopPeriodic --> UpdatePeriodicBtn[Update Periodic Button<br/>className = "btn-secondary"<br/>text = "Periodic OFF"]
    
    UpdatePeriodicBtn --> PublishStop[Publish to MQTT<br/>Topic: stm32Command<br/>Message: "PERIODIC OFF"<br/>QoS: 1]
    
    PublishStop --> LogStop[Log to Console<br/>"Periodic mode stopped<br/>(device OFF)"]
    
    LogStop --> ResetDisplay[Reset Current Display<br/>currentTemp = null<br/>currentHumi = null<br/>text = "--"]
    
    ResetDisplay --> LogCmd
    
    LogCmd --> End([Device Control Complete])
    ErrorMQTT --> End
    
    style ClickDevice fill:#10B981
    style End fill:#10B981
    style Publish fill:#8B5CF6
    style StopPeriodic fill:#EF4444
    style ResetDisplay fill:#F59E0B
```

---

## 4. Periodic Mode Control Flow

```mermaid
flowchart TB
    ClickPeriodic([User Clicks Periodic Button]) --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"]
    CheckMQTT -->|Yes| CheckDevice{isDeviceOn?}
    
    CheckDevice -->|No| ErrorDevice[Show Warning<br/>"Device must be ON first"]
    CheckDevice -->|Yes| TogglePeriodic[Toggle isPeriodic<br/>isPeriodic = !isPeriodic]
    
    TogglePeriodic --> BuildCmd{isPeriodic?}
    
    BuildCmd -->|true| CmdOn[command = "PERIODIC ON"]
    BuildCmd -->|false| CmdOff[command = "PERIODIC OFF"]
    
    CmdOn --> Publish[Publish to MQTT<br/>Topic: stm32Command<br/>QoS: 1]
    CmdOff --> Publish
    
    Publish --> UpdateBtn{isPeriodic?}
    
    UpdateBtn -->|true| BtnOn[Set Button Style<br/>className = "btn-success"<br/>text = "Periodic ON"]
    UpdateBtn -->|false| BtnOff[Set Button Style<br/>className = "btn-secondary"<br/>text = "Periodic OFF"]
    
    BtnOn --> LogCmd[Log to Console<br/>"Periodic ON/OFF<br/>command sent"]
    BtnOff --> LogCmd
    
    LogCmd --> End([Periodic Control Complete])
    ErrorMQTT --> End
    ErrorDevice --> End
    
    style ClickPeriodic fill:#10B981
    style End fill:#10B981
    style Publish fill:#8B5CF6
    style ErrorDevice fill:#F59E0B
```

---

## 5. Single Read Command Flow

```mermaid
flowchart TB
    ClickSingle([User Clicks Single Button]) --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"]
    CheckMQTT -->|Yes| CheckDevice{isDeviceOn?}
    
    CheckDevice -->|No| ErrorDevice[Show Warning<br/>"Device must be ON first"]
    CheckDevice -->|Yes| Publish[Publish to MQTT<br/>Topic: stm32Command<br/>Message: "SINGLE"<br/>QoS: 1]
    
    Publish --> LogCmd[Log to Console<br/>"Single read command sent"]
    
    LogCmd --> WaitResponse[Wait for Response<br/>On Topic: singleData]
    
    WaitResponse --> Timeout{Response<br/>Within<br/>Timeout?}
    
    Timeout -->|No| TimeoutMsg[Implicit Timeout<br/>No explicit handling<br/>User sees no update]
    Timeout -->|Yes| ProcessData[Process Single Data<br/>Flow: MQTT Message<br/>Reception & Processing]
    
    ProcessData --> UpdateUI[Update Current Display<br/>Update Charts<br/>Save to Firebase<br/>Add to Live Table]
    
    UpdateUI --> End([Single Read Complete])
    TimeoutMsg --> End
    ErrorMQTT --> End
    ErrorDevice --> End
    
    style ClickSingle fill:#10B981
    style End fill:#10B981
    style Publish fill:#8B5CF6
    style ProcessData fill:#06B6D4
    style TimeoutMsg fill:#F59E0B
```

---

## 6. Interval Configuration Flow

```mermaid
flowchart TB
    ClickInterval([User Clicks Interval Button]) --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"]
    CheckMQTT -->|Yes| CheckDevice{isDeviceOn?}
    
    CheckDevice -->|No| ErrorDevice[Show Warning<br/>"Device must be ON first"]
    CheckDevice -->|Yes| OpenModal[Open Interval Modal<br/>Display: block<br/>Show minute/second pickers]
    
    OpenModal --> InitPicker[Initialize Picker Values<br/>selectedMinute = 0<br/>selectedSecond = 5]
    
    InitPicker --> BindMinute[Bind Minute Up/Down<br/>Range: 0-59<br/>Wrap around]
    
    BindMinute --> BindSecond[Bind Second Up/Down<br/>Range: 0-59<br/>Wrap around]
    
    BindSecond --> WaitUser[Wait for User Action<br/>Update display on<br/>Up/Down clicks]
    
    WaitUser --> UserAction{User<br/>Action?}
    
    UserAction -->|Cancel| CloseModal[Close Modal<br/>Display: none<br/>No changes]
    
    UserAction -->|Set| CalculateInterval[Calculate Total Seconds<br/>interval = minute * 60<br/>+ second]
    
    CalculateInterval --> ValidateInterval{interval<br/>>= 5?}
    
    ValidateInterval -->|No| ShowError[Show Error<br/>"Interval must be<br/>at least 5 seconds"]
    ShowError --> WaitUser
    
    ValidateInterval -->|Yes| BuildCommand[Build Command<br/>"SET INTERVAL<br/>&lt;seconds&gt;"]
    
    BuildCommand --> PublishInterval[Publish to MQTT<br/>Topic: stm32Command<br/>QoS: 1]
    
    PublishInterval --> LogSuccess[Log to Console<br/>"Interval set to<br/>&lt;MM:SS&gt;"]
    
    LogSuccess --> CloseModal
    
    CloseModal --> End([Interval Config Complete])
    ErrorMQTT --> End
    ErrorDevice --> End
    
    style ClickInterval fill:#10B981
    style End fill:#10B981
    style OpenModal fill:#06B6D4
    style PublishInterval fill:#8B5CF6
    style ShowError fill:#EF4444
```

---

## 7. Time Sync from Internet Flow

```mermaid
flowchart TB
    ClickSync([User Clicks<br/>"Sync from Internet"]) --> GetLocal[Get Local Browser Time<br/>now = new Date()<br/>timestamp = now.getTime() / 1000]
    
    GetLocal --> UpdatePicker[Update Time Picker State<br/>timePickerYear<br/>timePickerMonth<br/>timePickerDay<br/>timePickerHour<br/>timePickerMinute<br/>timePickerSecond]
    
    UpdatePicker --> RenderCalendar[Render Calendar<br/>Highlight selected date<br/>Update month/year display]
    
    RenderCalendar --> RenderTime[Render Time Display<br/>Update HH:MM:SS<br/>24-hour format]
    
    RenderTime --> AddOffset[Add UTC+7 Offset<br/>timestampWithOffset<br/>= timestamp + 7*3600]
    
    AddOffset --> BuildCmd[Build Command<br/>"SET TIME<br/>&lt;timestampWithOffset&gt;"]
    
    BuildCmd --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"<br/>Cannot send SET TIME]
    
    CheckMQTT -->|Yes| PublishTime[Publish to MQTT<br/>Topic: stm32Command<br/>QoS: 1]
    
    PublishTime --> LogSync[Log to Console<br/>"Time synced from internet:<br/>&lt;datetime&gt; (UTC+7)"]
    
    LogSync --> UpdateDevice[Update Device Clock<br/>deviceClockMs = timestamp*1000<br/>deviceClockSetAtMs = Date.now()]
    
    UpdateDevice --> StartTicker[Start Clock Ticker<br/>Update footer time<br/>every 1 second]
    
    StartTicker --> End([Time Sync Complete])
    ErrorMQTT --> UpdateDevice
    
    style ClickSync fill:#10B981
    style End fill:#10B981
    style PublishTime fill:#8B5CF6
    style AddOffset fill:#F97316
    style StartTicker fill:#06B6D4
```

---

## 8. Manual Time Setting Flow

```mermaid
flowchart TB
    ClickManual([User Clicks<br/>"Set Time Manually"]) --> ReadPicker[Read Time Picker State<br/>year, month, day<br/>hour, minute, second]
    
    ReadPicker --> CreateDate[Create Date Object<br/>date = new Date<br/>(year, month, day,<br/>hour, minute, second)]
    
    CreateDate --> GetTimestamp[Get Unix Timestamp<br/>timestamp = date.getTime() / 1000]
    
    GetTimestamp --> AddOffset[Add UTC+7 Offset<br/>timestampWithOffset<br/>= timestamp + 7*3600]
    
    AddOffset --> BuildCmd[Build Command<br/>"SET TIME<br/>&lt;timestampWithOffset&gt;"]
    
    BuildCmd --> CheckMQTT{MQTT<br/>Connected?}
    
    CheckMQTT -->|No| ErrorMQTT[Show Error<br/>"MQTT not connected"<br/>Cannot send SET TIME]
    
    CheckMQTT -->|Yes| PublishTime[Publish to MQTT<br/>Topic: stm32Command<br/>QoS: 1]
    
    PublishTime --> LogSet[Log to Console<br/>"Manual time set:<br/>&lt;datetime&gt; (UTC+7)"]
    
    LogSet --> UpdateDevice[Update Device Clock<br/>deviceClockMs = timestamp*1000<br/>deviceClockSetAtMs = Date.now()]
    
    UpdateDevice --> StartTicker[Start Clock Ticker<br/>Update footer time<br/>every 1 second]
    
    StartTicker --> End([Manual Time Set Complete])
    ErrorMQTT --> UpdateDevice
    
    style ClickManual fill:#10B981
    style End fill:#10B981
    style PublishTime fill:#8B5CF6
    style AddOffset fill:#F97316
    style StartTicker fill:#06B6D4
```

---

## 9. Data Management & Filtering Flow

```mermaid
flowchart TB
    ClickApply([User Clicks<br/>"Apply Filters"]) --> CheckFB{Firebase<br/>Connected?}
    
    CheckFB -->|No| ErrorFB[Show Error<br/>"Firebase not connected"]
    
    CheckFB -->|Yes| ReadFilters[Read Filter Inputs<br/>dateFrom, dateTo<br/>mode, status<br/>tempMin, tempMax<br/>humiMin, humiMax<br/>sortBy]
    
    ReadFilters --> GenerateRange[Generate Date Range<br/>dates = []<br/>for d = dateFrom to dateTo<br/>dates.push(YYYY-MM-DD)]
    
    GenerateRange --> LoadData[Load Data from Firebase<br/>For each date:<br/>firebase.ref('readings/date')<br/>.once('value')]
    
    LoadData --> MergeData[Merge All Days<br/>allData = results.flat()]
    
    MergeData --> ApplyMode{Mode<br/>Filter?}
    
    ApplyMode -->|Yes| FilterMode[Filter by Mode<br/>record.mode == filter]
    ApplyMode -->|No| ApplyStatus
    
    FilterMode --> ApplyStatus{Status<br/>Filter?}
    
    ApplyStatus -->|Yes| FilterStatus[Filter by Status<br/>record.status == filter]
    ApplyStatus -->|No| ApplyTemp
    
    FilterStatus --> ApplyTemp{Temp<br/>Range?}
    
    ApplyTemp -->|Yes| FilterTemp[Filter by Temperature<br/>tempMin <= temp <= tempMax]
    ApplyTemp -->|No| ApplyHumi
    
    FilterTemp --> ApplyHumi{Humi<br/>Range?}
    
    ApplyHumi -->|Yes| FilterHumi[Filter by Humidity<br/>humiMin <= humi <= humiMax]
    ApplyHumi -->|No| ApplySort
    
    FilterHumi --> ApplySort[Apply Sorting<br/>time-desc/asc<br/>temp-desc/asc<br/>humi-desc/asc]
    
    ApplySort --> CacheData[Cache Filtered Data<br/>filteredDataCache = filtered]
    
    CacheData --> RenderTable[Render Data Table<br/>Build HTML rows<br/>Show #, Date/Time,<br/>Temp, Humi, Mode, Status]
    
    RenderTable --> UpdateStats[Update Statistics<br/>Total Records<br/>Success Rate<br/>Avg Temperature<br/>Avg Humidity]
    
    UpdateStats --> LogResult[Log to Console<br/>"Loaded X records<br/>from Y total"]
    
    LogResult --> End([Filtering Complete])
    ErrorFB --> End
    
    style ClickApply fill:#10B981
    style End fill:#10B981
    style LoadData fill:#F97316
    style ApplySort fill:#06B6D4
    style UpdateStats fill:#F59E0B
```

---

## 10. Load History from Firebase Flow

```mermaid
flowchart TB
    ClickLoad([User Clicks<br/>"Load History"]) --> CheckFB{Firebase<br/>Connected?}
    
    CheckFB -->|No| ErrorFB[Show Error<br/>"Cannot load history"<br/>"Firebase not connected"]
    
    CheckFB -->|Yes| GetTimeRange[Get Last 24 Hours<br/>now = new Date()<br/>yesterday = now - 24h]
    
    GetTimeRange --> GenerateDates[Generate Date Range<br/>dates = []<br/>currentDate = yesterday<br/>while currentDate <= now<br/>dates.push(YYYY-MM-DD)]
    
    GenerateDates --> LoadFirebase[Load Firebase Data<br/>For each date:<br/>firebase.ref('readings/date')<br/>.once('value')]
    
    LoadFirebase --> ProcessData[Process Each Day<br/>dayData = snapshot.val()<br/>Extract id, record<br/>Add date field]
    
    ProcessData --> MergeAll[Merge All Data<br/>allData = results.flat()]
    
    MergeAll --> CheckEmpty{Data<br/>Length > 0?}
    
    CheckEmpty -->|No| ShowWarning[Show Warning<br/>"No historical data<br/>in last 24 hours"]
    
    CheckEmpty -->|Yes| SortData[Sort by Timestamp<br/>Newest first<br/>b.timestamp - a.timestamp]
    
    SortData --> TakeLast50[Take Last 50 Points<br/>recentData = allData<br/>.slice(0, 50)<br/>.reverse()]
    
    TakeLast50 --> ClearCharts[Clear Existing Charts<br/>temperatureData = []<br/>humidityData = []]
    
    ClearCharts --> PopulateArrays[Populate Data Arrays<br/>For each record:<br/>pushTemperature(false)<br/>pushHumidity(false)]
    
    PopulateArrays --> UpdateCharts[Update Chart.js<br/>tempChart.data.labels<br/>tempChart.data.datasets<br/>tempChart.update('active')]
    
    UpdateCharts --> UpdateStats[Update Chart Stats<br/>Calculate min, max, avg<br/>Update UI elements]
    
    UpdateStats --> LogSuccess[Log to Console<br/>"Loaded X records (24h)<br/>showing Y on charts"]
    
    LogSuccess --> End([History Load Complete])
    ShowWarning --> End
    ErrorFB --> End
    
    style ClickLoad fill:#10B981
    style End fill:#10B981
    style LoadFirebase fill:#F97316
    style UpdateCharts fill:#06B6D4
    style TakeLast50 fill:#F59E0B
```

---

## 11. Chart Data Management Flow

```mermaid
flowchart TB
    PushData([Push Data to Chart]) --> GetTimestamp[Get Display Time<br/>Use device timestamp<br/>or current time<br/>Format: HH:MM:SS]
    
    GetTimestamp --> CreateEntry[Create Data Entry<br/>{ value, time }]
    
    CreateEntry --> AddArray{Array Type?}
    
    AddArray -->|Temperature| PushTemp[temperatureData.push<br/>{ value, time }]
    AddArray -->|Humidity| PushHumi[humidityData.push<br/>{ value, time }]
    
    PushTemp --> CheckLimit{Length ><br/>maxDataPoints?}
    PushHumi --> CheckLimit
    
    CheckLimit -->|Yes| ShiftOldest[Remove Oldest<br/>array.shift()]
    CheckLimit -->|No| CheckUpdate{Update<br/>Flag?}
    
    ShiftOldest --> CheckUpdate
    
    CheckUpdate -->|No| SkipUpdate[Skip Chart Update<br/>Data buffered only]
    
    CheckUpdate -->|Yes| CheckChart{Chart<br/>Exists?}
    
    CheckChart -->|No| LazyInit[Lazy Initialize Charts<br/>ensureChartsInitialized()<br/>Retry init if needed]
    
    CheckChart -->|Yes| UpdateData[Update Chart Data<br/>chart.data.labels = times<br/>chart.data.datasets[0].data<br/>= values]
    
    LazyInit --> LazyCheck{Init<br/>Success?}
    
    LazyCheck -->|No| WarnNoChart[Log Warning<br/>"Chart not ready<br/>data saved"]
    LazyCheck -->|Yes| UpdateData
    
    UpdateData --> UpdateChart[chart.update('active')<br/>Animate transitions]
    
    UpdateChart --> CalcStats[Calculate Statistics<br/>min = Math.min(values)<br/>max = Math.max(values)<br/>avg = sum / count]
    
    CalcStats --> UpdateStats[Update Stats UI<br/>#tempMin, #tempMax<br/>#tempAvg<br/>#humiMin, #humiMax<br/>#humiAvg]
    
    UpdateStats --> LogPush[Log Push<br/>"Chart updated with<br/>X points"]
    
    LogPush --> End([Chart Update Complete])
    SkipUpdate --> End
    WarnNoChart --> End
    
    style PushData fill:#10B981
    style End fill:#10B981
    style UpdateChart fill:#06B6D4
    style CalcStats fill:#F59E0B
    style LazyInit fill:#F97316
```

---

## 12. Live Data Table Management Flow

```mermaid
flowchart TB
    AddData([Add to Live Data Table]) --> CreateEntry[Create Table Entry<br/>{ time, temp, humi,<br/>mode, status }]
    
    CreateEntry --> Unshift[Add to Buffer Head<br/>liveDataBuffer.unshift(entry)]
    
    Unshift --> CheckLimit{Buffer Length<br/>> 100?}
    
    CheckLimit -->|Yes| RemoveLast[Remove Last Entry<br/>liveDataBuffer.pop()]
    CheckLimit -->|No| GetFilter[Get Filter Selection<br/>filter = #liveDataFilter.value]
    
    RemoveLast --> GetFilter
    
    GetFilter --> ApplyFilter{Filter<br/>Type?}
    
    ApplyFilter -->|all| FilteredAll[filtered = liveDataBuffer]
    ApplyFilter -->|success| FilterSuccess[filtered = buffer.filter<br/>(d => d.status == 'success')]
    ApplyFilter -->|error| FilterError[filtered = buffer.filter<br/>(d => d.status == 'error')]
    
    FilteredAll --> CheckEmpty{Filtered<br/>Length > 0?}
    FilterSuccess --> CheckEmpty
    FilterError --> CheckEmpty
    
    CheckEmpty -->|No| ShowEmpty[Show Empty State<br/>"No data available"<br/>Count = 0]
    
    CheckEmpty -->|Yes| BuildRows[Build HTML Rows<br/>For each entry:<br/>#, Time, Temp, Humi,<br/>Mode, Status Badge]
    
    BuildRows --> FormatStatus{Status?}
    
    FormatStatus -->|success| GreenBadge[Badge: connected<br/>"✓ Success"]
    FormatStatus -->|error| RedBadge[Badge: disconnected<br/>"✗ Error"]
    
    GreenBadge --> RenderTable[Render to #liveDataTable<br/>tbody.innerHTML = rows]
    RedBadge --> RenderTable
    
    RenderTable --> UpdateCount[Update Entry Count<br/>#liveDataCount.textContent<br/>= filtered.length]
    
    UpdateCount --> RefreshIcons[Refresh Feather Icons<br/>feather.replace()]
    
    RefreshIcons --> End([Live Table Updated])
    ShowEmpty --> End
    
    style AddData fill:#10B981
    style End fill:#10B981
    style BuildRows fill:#06B6D4
    style GreenBadge fill:#22D3EE
    style RedBadge fill:#EF4444
```

---

## 13. Component Health Monitoring Flow

```mermaid
flowchart TB
    Update([Update Component Health]) --> GetComponent{Component<br/>Type?}
    
    GetComponent -->|SHT31| UpdateSHT[componentHealth.SHT31<br/>healthy = isHealthy<br/>lastUpdate = Date.now()]
    GetComponent -->|DS3231| UpdateRTC[componentHealth.DS3231<br/>healthy = isHealthy<br/>lastUpdate = Date.now()]
    
    UpdateSHT --> CheckHealthy{isHealthy?}
    UpdateRTC --> CheckHealthy
    
    CheckHealthy -->|No| IncrementError[errorCount++<br/>Or failCount++]
    CheckHealthy -->|Yes| ResetError[errorCount = 0<br/>Or failCount = 0]
    
    IncrementError --> GetCard[Get Component Card<br/>#component-SHT31<br/>or #component-DS3231]
    ResetError --> GetCard
    
    GetCard --> GetBadge[Get Status Badge<br/>Find .status-badge]
    
    GetBadge --> SHTCheck{Component =<br/>SHT31?}
    
    SHTCheck -->|Yes| SHTHealthy{healthy?}
    
    SHTHealthy -->|Yes| SHTRunning[Badge: connected<br/>"Running"<br/>Update #sht31-temp<br/>Update #sht31-humi<br/>Update #sht31-time]
    
    SHTHealthy -->|No| SHTError[Badge: disconnected<br/>"Error"<br/>temp = "Sensor Failed"<br/>humi = "Check Wiring"<br/>time = "Error"]
    
    SHTCheck -->|No| RTCHealthy{healthy?}
    
    RTCHealthy -->|Yes| RTCRunning[Badge: connected<br/>"Running"<br/>Update #rtc-time<br/>status = "Operating normally"]
    
    RTCHealthy -->|No| RTCError[Badge: disconnected<br/>"Error"<br/>time = "RTC Failed"<br/>status = "Time sync failed"]
    
    SHTRunning --> CalcTime[Calculate Time Ago<br/>seconds = (now - lastUpdate)/1000<br/>Format: "X seconds/minutes/hours ago"]
    SHTError --> CalcTime
    RTCRunning --> CalcTime
    RTCError --> CalcTime
    
    CalcTime --> UpdateUI[Update Card UI<br/>Display status<br/>Show last update time<br/>Show error messages]
    
    UpdateUI --> End([Health Update Complete])
    
    style Update fill:#10B981
    style End fill:#10B981
    style SHTRunning fill:#22D3EE
    style RTCRunning fill:#22D3EE
    style SHTError fill:#EF4444
    style RTCError fill:#EF4444
```

---

## 14. Logs Page Management Flow

```mermaid
flowchart TB
    AddLog([Add Log Entry]) --> CreateLog[Create Log Object<br/>{ type, message,<br/>timestamp, date_iso }]
    
    CreateLog --> UnshiftBuffer[Unshift to logBuffer<br/>logBuffer.unshift(entry)]
    
    UnshiftBuffer --> CheckLimit{Buffer Length<br/>> maxLogsInMemory?}
    
    CheckLimit -->|Yes| PopOldest[Remove Oldest<br/>logBuffer.pop()]
    CheckLimit -->|No| UpdatePreview[Update Console Preview<br/>Get #consolePreview]
    
    PopOldest --> UpdatePreview
    
    UpdatePreview --> FormatLine[Format Log Line<br/>"[TYPE] HH:MM:SS: message"<br/>Add CSS class: log-type]
    
    FormatLine --> PrependPreview[Prepend to Preview<br/>innerHTML = line + innerHTML]
    
    PrependPreview --> CheckPreviewLimit{Preview<br/>Lines > 5?}
    
    CheckPreviewLimit -->|Yes| RemovePreviewLast[Remove Last Line<br/>lines[-1].remove()]
    CheckPreviewLimit -->|No| CheckLogsPage{On Logs<br/>Page?}
    
    RemovePreviewLast --> CheckLogsPage
    
    CheckLogsPage -->|Yes| RenderFull[Render Full Console<br/>Get #consoleFull]
    CheckLogsPage -->|No| ConsoleLog[console.log<br/>"[TYPE] message"]
    
    RenderFull --> GetFilters[Get Active Filters<br/>logFilterType<br/>searchTerm]
    
    GetFilters --> FilterType{Type<br/>Filter?}
    
    FilterType -->|ALL| FilterSearch{Search<br/>Term?}
    FilterType -->|Specific| FilterByType[Filter by Type<br/>log.type == filter]
    
    FilterByType --> FilterSearch
    
    FilterSearch -->|Yes| FilterBySearch[Filter by Search<br/>message.includes(term)]
    FilterSearch -->|No| ColorMap[Map Type to Color<br/>INFO: blue<br/>ERROR: red<br/>MQTT: purple<br/>etc.]
    
    FilterBySearch --> ColorMap
    
    ColorMap --> BuildLines[Build HTML Lines<br/>For each log:<br/>"[time] [TYPE] message"<br/>style="color: X"]
    
    BuildLines --> RenderConsole[Render to #consoleFull<br/>innerHTML = lines.join()]
    
    RenderConsole --> UpdateCount[Update Log Count<br/>#logCount.textContent<br/>= filtered.length]
    
    UpdateCount --> AutoScroll[Auto-scroll to Bottom<br/>scrollTop = scrollHeight]
    
    AutoScroll --> ConsoleLog
    
    ConsoleLog --> End([Log Entry Complete])
    
    style AddLog fill:#10B981
    style End fill:#10B981
    style FilterByType fill:#F59E0B
    style ColorMap fill:#06B6D4
    style AutoScroll fill:#8B5CF6
```

---

## 15. Navigation & Page Switching Flow

```mermaid
flowchart TB
    ClickMenu([User Clicks Menu Item]) --> GetPage[Get Page ID<br/>page = item.dataset.page]
    
    GetPage --> RemoveActive[Remove Active Class<br/>All menu items<br/>classList.remove('active')]
    
    RemoveActive --> AddActive[Add Active Class<br/>Clicked item<br/>classList.add('active')]
    
    AddActive --> HidePages[Hide All Pages<br/>All .page-section<br/>classList.remove('active')]
    
    HidePages --> ShowPage[Show Selected Page<br/>#page-{page}<br/>classList.add('active')]
    
    ShowPage --> CheckPage{Page<br/>Type?}
    
    CheckPage -->|time| InitTime[renderCalendar()<br/>updateTimeDisplay()<br/>bindCalendarButtons()<br/>attachTimeButtons()]
    
    CheckPage -->|livedata| InitLive[renderLiveDataTable()<br/>bindLiveDataButtons()]
    
    CheckPage -->|data| InitData[bindDataManagementButtons()<br/>applyDataFilters()]
    
    CheckPage -->|logs| InitLogs[bindLogsButtons()<br/>renderFullConsole()]
    
    CheckPage -->|config| InitConfig[loadSettingsPage()]
    
    CheckPage -->|dashboard| InitDashboard[Default View<br/>No special init]
    
    CheckPage -->|settings| InitSettings[Default View<br/>No special init]
    
    InitTime --> RefreshIcons[Refresh Feather Icons<br/>feather.replace()]
    InitLive --> RefreshIcons
    InitData --> RefreshIcons
    InitLogs --> RefreshIcons
    InitConfig --> RefreshIcons
    InitDashboard --> RefreshIcons
    InitSettings --> RefreshIcons
    
    RefreshIcons --> End([Page Switch Complete])
    
    style ClickMenu fill:#10B981
    style End fill:#10B981
    style InitTime fill:#F59E0B
    style InitData fill:#F97316
    style InitLogs fill:#8B5CF6
    style RefreshIcons fill:#06B6D4
```

---

## System Characteristics

### Performance Metrics
- **Page Load**: ~2s (includes Firebase + MQTT init)
- **Chart Update**: 750ms animation, 60 FPS
- **MQTT Latency**: ~50-100ms (local network)
- **Firebase Write**: ~200-500ms (network dependent)
- **UI Refresh Rate**: 1s (footer clock), on-demand (charts/tables)

### Memory Management
- **Temperature Data**: Max 50 points (FIFO)
- **Humidity Data**: Max 50 points (FIFO)
- **Log Buffer**: Max 100 entries (FIFO)
- **Live Data Table**: Max 100 entries (FIFO)
- **Filtered Data Cache**: Unlimited (cleared on filter reset)

### Connection Handling
- **MQTT Reconnect**: Every 2 seconds on disconnect
- **Chart Init Retry**: Max 10 attempts, 500ms interval
- **Firebase Auto-reconnect**: Built-in .info/connected listener
- **State Sync Request**: 500ms delay after MQTT connect

### Error Handling
- **Sensor Failure**: Log ERROR, show warning, mark component unhealthy
- **RTC Failure**: Log ERROR, use local time, mark component unhealthy
- **MQTT Disconnect**: Show status, auto-reconnect, queue messages
- **Firebase Error**: Log error, continue operation, skip saves
- **Chart Init Fail**: Log error, buffer data, retry on next push

### User Experience
- **Responsive Layout**: 260px sidebar, fluid content area
- **Theme Support**: Auto dark/light mode detection
- **Real-time Updates**: MQTT messages instantly reflected
- **Smooth Animations**: Chart.js easeInOutQuart 750ms
- **Icon System**: Feather icons auto-refresh on page change

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-XX  
**Total Flowcharts**: 15  
**Coverage**: Complete web application lifecycle
