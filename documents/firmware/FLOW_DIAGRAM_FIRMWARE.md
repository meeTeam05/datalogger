# Complete Firmware System - Flow Diagram

This document describes the integrated control flow of the complete firmware system including STM32 data logger and ESP32 IoT bridge.

## System Startup and Initialization Flow

```mermaid
flowchart TD
    Start([Power ON Both Devices]) --> PowerSTM32[STM32 Power Up]
    Start --> PowerESP32[ESP32 Power Up]
    
    PowerSTM32 --> InitSTM32[STM32 System Init]
    InitSTM32 --> InitSTM32UART[Initialize UART1 115200]
    InitSTM32UART --> InitI2C[Initialize I2C1 100kHz]
    InitI2C --> InitSPI[Initialize SPI1 & SPI2]
    InitSPI --> InitSHT3X[Initialize SHT3X Sensor 0x44]
    InitSHT3X --> InitDS3231[Initialize DS3231 RTC 0x68]
    InitDS3231 --> InitSDCard[Initialize SD Card Manager]
    InitSDCard --> InitDisplay[Initialize ILI9225 Display]
    InitDisplay --> STM32Ready[STM32 Main Loop Ready]
    
    PowerESP32 --> InitNVS[Initialize NVS Flash]
    InitNVS --> InitNetif[Initialize Network Interface]
    InitNetif --> InitWiFi[Initialize WiFi Manager]
    InitWiFi --> InitESP32UART[Initialize UART1 115200]
    InitESP32UART --> InitComponents[Initialize MQTT, Relay, Parser, Buttons]
    InitComponents --> ConnectWiFi[WiFi Connect with 15s timeout]
    
    ConnectWiFi --> CheckWiFi{WiFi Connected?}
    CheckWiFi -->|Yes| WaitStable[Wait 4s for network stability]
    CheckWiFi -->|No| ESP32Ready
    WaitStable --> StartMQTT[Start MQTT Client]
    StartMQTT --> ESP32Ready[ESP32 Main Loop Ready]
    
    STM32Ready --> BothReady{Both Systems Ready?}
    ESP32Ready --> BothReady
    BothReady -->|Yes| SystemRunning[System Fully Operational]
    
    style Start fill:#90EE90
    style BothReady fill:#FFD700
    style SystemRunning fill:#87CEEB
```

## Single Measurement End-to-End Flow

```mermaid
flowchart TD
    Start([User/Button Request Single]) --> Source{Request Source?}
    
    Source -->|Web Interface| WebMQTT[Web sends to MQTT broker]
    Source -->|ESP32 Button| ButtonPress[ESP32 button pressed GPIO16]
    
    WebMQTT --> ESP32MQTT[ESP32 receives on datalogger/stm32/command]
    ButtonPress --> CheckDevice{Device ON?}
    CheckDevice -->|No| IgnoreButton[Log: Ignored - device OFF]
    CheckDevice -->|Yes| SendSingle
    
    ESP32MQTT --> ForwardUART[ESP32 forwards via UART]
    ButtonPress --> SendSingle[ESP32 sends SINGLE command]
    
    ForwardUART --> STM32RX[STM32 UART RX Interrupt]
    SendSingle --> STM32RX
    
    STM32RX --> RingBuffer[Store in Ring Buffer]
    RingBuffer --> UARTHandle[UART_Handle detects newline]
    UARTHandle --> ParseCmd[Parse: SINGLE]
    ParseCmd --> CallSHT3X[SHT3X_Single HIGH repeatability]
    
    CallSHT3X --> I2CCmd[I2C Write 0x2400]
    I2CCmd --> WaitMeasure[Delay 15ms for measurement]
    WaitMeasure --> I2CRead[I2C Read 6 bytes]
    I2CRead --> CheckI2C{I2C OK?}
    
    CheckI2C -->|No| SetError[temp=0.0, hum=0.0]
    CheckI2C -->|Yes| ValidateCRC{CRC Valid?}
    ValidateCRC -->|No| SetError
    ValidateCRC -->|Yes| ParseData[Parse temperature & humidity]
    
    SetError --> UpdateDM[DataManager_UpdateSingle]
    ParseData --> UpdateDM
    UpdateDM --> SetFlag[Set data_ready flag]
    SetFlag --> NextLoop[Wait for next main loop]
    
    NextLoop --> CheckMQTT{MQTT Connected?}
    CheckMQTT -->|Yes| GetTime[DS3231_Get_Time]
    CheckMQTT -->|No| BufferSD[SDCardManager_WriteData]
    
    GetTime --> FormatJSON[Format JSON with timestamp]
    FormatJSON --> TransmitUART[HAL_UART_Transmit to ESP32]
    TransmitUART --> ESP32Parse[ESP32 JSON Parser]
    
    ESP32Parse --> ValidJSON{JSON Valid?}
    ValidJSON -->|No| LogError[Log parse error]
    ValidJSON -->|Yes| MQTTPublish[MQTT Publish to datalogger/stm32/single/data]
    
    MQTTPublish --> WebReceive[Web Interface receives data]
    BufferSD --> WaitMQTT[Wait for MQTT reconnection]
    
    IgnoreButton --> Done([Done])
    LogError --> Done
    WebReceive --> Done
    WaitMQTT --> Done
    
    style Start fill:#90EE90
    style CheckDevice fill:#FFD700
    style CheckI2C fill:#FFD700
    style CheckMQTT fill:#FFD700
    style MQTTPublish fill:#87CEEB
    style BufferSD fill:#FF6B6B
```

## Periodic Measurement Flow

```mermaid
flowchart TD
    Start([PERIODIC ON Command]) --> ESP32Receive[ESP32 receives command]
    ESP32Receive --> UpdateState[ESP32: g_periodic_active = true]
    UpdateState --> ForwardSTM32[ESP32 forwards PERIODIC ON to STM32]
    ForwardSTM32 --> PublishState[ESP32 publishes state to MQTT]
    
    PublishState --> STM32Parse[STM32 parses PERIODIC ON]
    STM32Parse --> CallPeriodic[SHT3X_Periodic 1MPS HIGH]
    CallPeriodic --> I2CStart[I2C Write 0x2032]
    I2CStart --> CheckI2C{I2C OK?}
    
    CheckI2C -->|No| SetError[temp=0.0, hum=0.0]
    CheckI2C -->|Yes| UpdateMode[currentState = PERIODIC_1MPS]
    UpdateMode --> InitTiming[Initialize next_fetch_ms]
    InitTiming --> FirstFetch[Fetch first data]
    
    SetError --> UpdateDM[DataManager_UpdatePeriodic]
    FirstFetch --> UpdateDM
    UpdateDM --> MainLoop[STM32 Main Loop]
    
    MainLoop --> CheckTime{Time to fetch?}
    CheckTime -->|No| CheckMQTTLoop
    CheckTime -->|Yes| FetchNow[SHT3X_FetchData]
    FetchNow --> CheckFetch{Fetch OK?}
    
    CheckFetch -->|No| UseError[Use 0.0 values]
    CheckFetch -->|Yes| UseData[Use fetched data]
    UseError --> DataReady
    UseData --> DataReady[DataManager_UpdatePeriodic]
    
    DataReady --> CheckMQTTLoop{MQTT Connected?}
    CheckMQTTLoop -->|Yes| SendLive[Send JSON to ESP32]
    CheckMQTTLoop -->|No| BufferSD[Buffer to SD Card]
    
    SendLive --> ESP32Process[ESP32 publishes to MQTT]
    BufferSD --> CheckFull{SD Buffer Full?}
    CheckFull -->|Yes| Overwrite[Circular buffer overwrites oldest]
    CheckFull -->|No| StoreOK[Store successfully]
    Overwrite --> MainLoop
    StoreOK --> MainLoop
    
    ESP32Process --> WebUpdate[Web receives periodic data]
    WebUpdate --> MainLoop
    
    MainLoop --> CheckStop{PERIODIC OFF<br/>command?}
    CheckStop -->|No| CheckTime
    CheckStop -->|Yes| StopPeriodic[SHT3X_PeriodicStop]
    StopPeriodic --> ResetState[currentState = IDLE]
    ResetState --> Done([Done])
    
    style Start fill:#90EE90
    style CheckI2C fill:#FFD700
    style CheckMQTTLoop fill:#FFD700
    style CheckFull fill:#FFD700
    style BufferSD fill:#FF6B6B
```

## MQTT Connection State Management Flow

```mermaid
flowchart TD
    Start([System Running]) --> MonitorWiFi[ESP32 monitors WiFi state]
    
    MonitorWiFi --> CheckWiFi{WiFi State?}
    
    CheckWiFi -->|CONNECTED| CheckStable{Stable for 4s?}
    CheckWiFi -->|DISCONNECTED| StopMQTT[Stop MQTT Handler]
    CheckWiFi -->|FAILED| WaitRetry[Wait 5s for retry]
    CheckWiFi -->|CONNECTING| MonitorWiFi
    
    WaitRetry --> RetryWiFi[WiFi_Connect]
    RetryWiFi --> MonitorWiFi
    
    CheckStable -->|Yes| CheckMQTTStart{MQTT Started?}
    CheckStable -->|No| MonitorWiFi
    
    CheckMQTTStart -->|No| StartMQTT[MQTT_Handler_Start]
    CheckMQTTStart -->|Yes| CheckMQTTConn
    
    StartMQTT --> Connecting[MQTT Connecting to broker]
    Connecting --> BrokerResponse{Broker Response?}
    
    BrokerResponse -->|CONNECTED| SetConnected[connected = true]
    BrokerResponse -->|ERROR| IncRetry[Increment retry_count]
    
    SetConnected --> Subscribe[Subscribe to command topics]
    Subscribe --> SetReconnFlag[Set mqtt_reconnected flag]
    SetReconnFlag --> NotifySTM32Conn[Send MQTT CONNECTED to STM32]
    NotifySTM32Conn --> CheckMQTTConn
    
    IncRetry --> BackoffDelay[Exponential backoff: min 60s, 2^retry]
    BackoffDelay --> CheckMaxRetry{Max retries?}
    CheckMaxRetry -->|No| StartMQTT
    CheckMaxRetry -->|Yes| WaitReset[Wait 60s]
    WaitReset --> ResetRetry[Reset retry_count]
    ResetRetry --> StartMQTT
    
    CheckMQTTConn{MQTT State?}
    CheckMQTTConn -->|CONNECTED| Active[Handle messages]
    CheckMQTTConn -->|DISCONNECTED| CheckWiFiState
    
    Active --> CheckMsg{New Message?}
    CheckMsg -->|Yes| RouteMsg[Route to handler]
    CheckMsg -->|No| CheckWiFiState
    
    RouteMsg --> Active
    
    CheckWiFiState{WiFi still connected?}
    CheckWiFiState -->|Yes| CheckMQTTConn
    CheckWiFiState -->|No| StopMQTT
    
    StopMQTT --> UpdateMQTTState[mqtt_current_state = DISCONNECTED]
    UpdateMQTTState --> NotifySTM32Disc[Send MQTT DISCONNECTED to STM32]
    NotifySTM32Disc --> STM32Response[STM32 switches to SD buffering]
    STM32Response --> MonitorWiFi
    
    style Start fill:#90EE90
    style CheckWiFi fill:#FFD700
    style BrokerResponse fill:#FFD700
    style CheckMQTTConn fill:#FFD700
    style Active fill:#87CEEB
```

## Relay Control End-to-End Flow

```mermaid
flowchart TD
    Start([Relay Control Request]) --> Source{Control Source?}
    
    Source -->|Web Interface| WebCmd[Web sends to datalogger/esp32/relay/control]
    Source -->|ESP32 Button| ButtonCmd[GPIO5 button pressed]
    
    WebCmd --> ESP32MQTT[ESP32 MQTT callback]
    ButtonCmd --> ProcessButton[on_button_relay_pressed]
    
    ESP32MQTT --> ParseCmd{Command?}
    ParseCmd -->|ON| SetOn[Relay_SetState true]
    ParseCmd -->|OFF| SetOff[Relay_SetState false]
    ParseCmd -->|TOGGLE| ToggleRelay[Relay_Toggle]
    
    ProcessButton --> ToggleRelay
    
    SetOn --> UpdateGPIO[GPIO4 = HIGH]
    SetOff --> UpdateGPIO2[GPIO4 = LOW]
    ToggleRelay --> UpdateGPIO3[Toggle GPIO4]
    
    UpdateGPIO --> UpdateState[g_device_on = true]
    UpdateGPIO2 --> UpdateState2[g_device_on = false]
    UpdateGPIO3 --> UpdateState3[Toggle g_device_on]
    
    UpdateState --> CheckOff
    UpdateState2 --> CheckOff{Relay turned OFF?}
    UpdateState3 --> CheckOff
    
    CheckOff -->|Yes| ForcePeriodic[Force g_periodic_active = false]
    CheckOff -->|No| StateCallback
    
    ForcePeriodic --> SendPOff[Send PERIODIC OFF to STM32]
    SendPOff --> StateCallback[Relay callback triggered]
    
    StateCallback --> PublishState[update_and_publish_state]
    PublishState --> Delay500[Wait 500ms for STM32 boot]
    Delay500 --> CheckMQTT{MQTT Connected?}
    
    CheckMQTT -->|Yes| SendConn[Send MQTT CONNECTED to STM32]
    CheckMQTT -->|No| SendDisc[Send MQTT DISCONNECTED to STM32]
    
    SendConn --> STM32Process[STM32 updates WiFi state]
    SendDisc --> STM32Process
    
    STM32Process --> UpdateDisplay[Display shows relay state]
    UpdateDisplay --> WebSync[Web interface syncs state]
    WebSync --> Done([Done])
    
    style Start fill:#90EE90
    style Source fill:#FFD700
    style CheckOff fill:#FFD700
    style CheckMQTT fill:#FFD700
    style PublishState fill:#87CEEB
```

## Button Control Flow (ESP32 Buttons)

```mermaid
flowchart TD
    Start([Button Event]) --> Debounce[Software debounce 200ms]
    Debounce --> Identify{Which Button?}
    
    Identify -->|GPIO5| RelayBtn[Relay Toggle Button]
    Identify -->|GPIO16| SingleBtn[Single Measurement Button]
    Identify -->|GPIO17| PeriodicBtn[Periodic Toggle Button]
    Identify -->|GPIO4| IntervalBtn[Interval Adjustment Button]
    
    RelayBtn --> RelayAction[on_button_relay_pressed]
    RelayAction --> ToggleRelay[Toggle relay state]
    ToggleRelay --> RelayFlow[See Relay Control Flow]
    RelayFlow --> Done([Done])
    
    SingleBtn --> CheckDev1{Device ON?}
    CheckDev1 -->|No| LogIgnore1[Log: Ignored - device OFF]
    CheckDev1 -->|Yes| SendSingle[Send SINGLE to STM32]
    LogIgnore1 --> Done
    SendSingle --> STM32Single[STM32 performs single measurement]
    STM32Single --> Done
    
    PeriodicBtn --> CheckDev2{Device ON?}
    CheckDev2 -->|No| LogIgnore2[Log: Ignored - device OFF]
    CheckDev2 -->|Yes| TogglePeriodic[Toggle g_periodic_active]
    LogIgnore2 --> Done
    
    TogglePeriodic --> CheckState{New State?}
    CheckState -->|ON| SendPOn[Send PERIODIC ON to STM32]
    CheckState -->|OFF| SendPOff[Send PERIODIC OFF to STM32]
    
    SendPOn --> UpdatePublish[update_and_publish_state]
    SendPOff --> UpdatePublish
    UpdatePublish --> STM32Process[STM32 starts/stops periodic]
    STM32Process --> Done
    
    IntervalBtn --> CheckDev3{Device ON?}
    CheckDev3 -->|No| LogIgnore3[Log: Ignored - device OFF]
    CheckDev3 -->|Yes| CycleInterval[Cycle interval: 5s → 10s → 15s → 30s → 60s → 5s]
    LogIgnore3 --> Done
    
    CycleInterval --> BuildCmd[Build SET PERIODIC INTERVAL command]
    BuildCmd --> SendCmd[Send to STM32]
    SendCmd --> STM32Update[STM32 updates periodic_interval_ms]
    STM32Update --> Done
    
    style Start fill:#90EE90
    style Identify fill:#FFD700
    style CheckDev1 fill:#FFD700
    style CheckDev2 fill:#FFD700
    style CheckDev3 fill:#FFD700
```

## SD Card Buffering and Transmission Flow

```mermaid
flowchart TD
    Start([Data Ready to Send]) --> CheckMQTT{MQTT Connected?}
    
    CheckMQTT -->|No| BufferDecision[STM32 decides to buffer]
    CheckMQTT -->|Yes| SendLive[STM32 sends live to ESP32]
    
    BufferDecision --> FormatJSON[Format JSON with timestamp]
    FormatJSON --> WriteSD[SDCardManager_WriteData]
    WriteSD --> CheckSpace{Buffer Space?}
    
    CheckSpace -->|Full| CircularOverwrite[Overwrite oldest record]
    CheckSpace -->|Available| WriteNew[Write new record]
    
    CircularOverwrite --> UpdatePointers[Update read/write pointers]
    WriteNew --> UpdatePointers
    UpdatePointers --> DisplayCount[Display: Buffered count]
    DisplayCount --> WaitReconnect[Wait for MQTT reconnection]
    
    WaitReconnect --> MQTTRestored{MQTT Reconnected?}
    MQTTRestored -->|No| WaitReconnect
    MQTTRestored -->|Yes| NotifySTM32[ESP32 sends MQTT CONNECTED]
    
    NotifySTM32 --> STM32Checks{Has buffered data?}
    STM32Checks -->|No| NormalOp[Resume normal operation]
    STM32Checks -->|Yes| ReadSD[SDCardManager_ReadData]
    
    ReadSD --> ValidRecord{Valid record?}
    ValidRecord -->|No| RemoveCorrupt[Remove corrupted record]
    ValidRecord -->|Yes| TransmitBuffer[Transmit buffered JSON]
    
    RemoveCorrupt --> CheckMore
    TransmitBuffer --> ESP32Forward[ESP32 forwards to MQTT]
    ESP32Forward --> RemoveRecord[SDCardManager_RemoveRecord]
    RemoveRecord --> CheckMore{More records?}
    
    CheckMore -->|Yes| RateLimit[Delay 100ms rate limiting]
    CheckMore -->|No| ClearBuffer[All buffered data sent]
    
    RateLimit --> ReadSD
    ClearBuffer --> UpdateDisplay[Display: Buffer empty]
    UpdateDisplay --> NormalOp
    
    SendLive --> ESP32Parse[ESP32 parses JSON]
    ESP32Parse --> PublishMQTT[Publish to MQTT broker]
    PublishMQTT --> WebReceive[Web receives real-time data]
    WebReceive --> NormalOp
    
    NormalOp --> Done([Done])
    
    style Start fill:#90EE90
    style CheckMQTT fill:#FFD700
    style CheckSpace fill:#FFD700
    style MQTTRestored fill:#FFD700
    style BufferDecision fill:#FF6B6B
    style WriteSD fill:#FF6B6B
```

## Error Handling and Recovery Flow

```mermaid
flowchart TD
    Start([Error Detected]) --> ErrorType{Error Type?}
    
    ErrorType -->|I2C Sensor Error| SensorError[SHT3X I2C timeout/NACK]
    ErrorType -->|RTC Error| RTCError[DS3231 I2C timeout/NACK]
    ErrorType -->|SD Card Error| SDError[SD Card mount/write failure]
    ErrorType -->|WiFi Error| WiFiError[WiFi connection failed]
    ErrorType -->|MQTT Error| MQTTError[MQTT broker connection failed]
    
    SensorError --> SetZero[Set temp=0.0, hum=0.0]
    SetZero --> ContinueOp1[Continue operation with error values]
    ContinueOp1 --> LogSensor[Log sensor failure to display]
    LogSensor --> RetryNext1[Retry on next measurement cycle]
    RetryNext1 --> Monitor1[Monitor for recovery]
    Monitor1 --> Done([System Continues])
    
    RTCError --> UseZero[Use timestamp=0]
    UseZero --> ContinueOp2[Continue operation with zero timestamp]
    ContinueOp2 --> LogRTC[Log RTC failure to display]
    LogRTC --> RetryNext2[Retry on next time query]
    RetryNext2 --> Monitor2[Monitor for recovery]
    Monitor2 --> Done
    
    SDError --> CheckInit{Initialization error?}
    CheckInit -->|Yes| DisableSD[Disable SD buffering]
    CheckInit -->|No| RetryWrite[Retry write operation]
    
    DisableSD --> LogSD[Log SD disabled to display]
    LogSD --> MQTTOnly[Rely on MQTT only]
    MQTTOnly --> Done
    
    RetryWrite --> CheckSuccess{Write successful?}
    CheckSuccess -->|Yes| RecoverSD[SD recovered]
    CheckSuccess -->|No| DisableSD
    RecoverSD --> Done
    
    WiFiError --> CheckRetry{Retry count?}
    CheckRetry -->|< 5| WaitWiFi[Wait 2s]
    CheckRetry -->|>= 5| ManualRetry[Wait 5s for manual retry]
    
    WaitWiFi --> RetryConnect[WiFi_Connect]
    ManualRetry --> RetryConnect
    RetryConnect --> CheckWiFiOK{Connected?}
    
    CheckWiFiOK -->|Yes| WiFiRecovered[WiFi recovered]
    CheckWiFiOK -->|No| CheckRetry
    
    WiFiRecovered --> WaitStable[Wait 4s stability]
    WaitStable --> StartMQTT[Start MQTT]
    StartMQTT --> Done
    
    MQTTError --> IncRetry[Increment retry_count]
    IncRetry --> CalcBackoff[Calculate backoff: min 60s, 2^retry]
    CalcBackoff --> WaitBackoff[Wait backoff delay]
    WaitBackoff --> RetryMQTT[MQTT_Handler_Start]
    RetryMQTT --> CheckMQTTOK{Connected?}
    
    CheckMQTTOK -->|Yes| MQTTRecovered[MQTT recovered]
    CheckMQTTOK -->|No| CheckMaxRetry{Max retries?}
    
    CheckMaxRetry -->|No| IncRetry
    CheckMaxRetry -->|Yes| ResetCount[Reset retry_count]
    ResetCount --> Wait60[Wait 60s]
    Wait60 --> RetryMQTT
    
    MQTTRecovered --> NotifySTM32[Send MQTT CONNECTED to STM32]
    NotifySTM32 --> TransmitBuffer[Transmit buffered SD data]
    TransmitBuffer --> Done
    
    style Start fill:#90EE90
    style ErrorType fill:#FFD700
    style CheckRetry fill:#FFD700
    style SetZero fill:#FF6B6B
    style DisableSD fill:#FF6B6B
```

## System State Synchronization Flow

```mermaid
flowchart TD
    Start([State Change Event]) --> StateType{State Type?}
    
    StateType -->|Device State| DeviceChange[g_device_on changed]
    StateType -->|Periodic State| PeriodicChange[g_periodic_active changed]
    StateType -->|MQTT State| MQTTChange[mqtt_current_state changed]
    
    DeviceChange --> CheckDevice{New Value?}
    CheckDevice -->|ON| DeviceOn[Device turned ON]
    CheckDevice -->|OFF| DeviceOff[Device turned OFF]
    
    DeviceOn --> UpdateESP32_1[ESP32 updates state]
    DeviceOff --> ForcePeriodic[Force periodic OFF]
    ForcePeriodic --> SendPOff[Send PERIODIC OFF to STM32]
    SendPOff --> UpdateESP32_2[ESP32 updates state]
    
    UpdateESP32_1 --> PublishDevice
    UpdateESP32_2 --> PublishDevice[Create state JSON]
    
    PeriodicChange --> CheckPeriodic{New Value?}
    CheckPeriodic -->|ON| PeriodicOn[Periodic mode started]
    CheckPeriodic -->|OFF| PeriodicOff[Periodic mode stopped]
    
    PeriodicOn --> SendPOn[Send PERIODIC ON to STM32]
    PeriodicOff --> SendPOffCmd[Send PERIODIC OFF to STM32]
    
    SendPOn --> UpdateESP32_3[ESP32 updates state]
    SendPOffCmd --> UpdateESP32_4[ESP32 updates state]
    
    UpdateESP32_3 --> PublishPeriodic
    UpdateESP32_4 --> PublishPeriodic[Create state JSON]
    
    PublishDevice --> CheckMQTT
    PublishPeriodic --> CheckMQTT{MQTT Connected?}
    
    CheckMQTT -->|Yes| FormatJSON[JSON_Utils_CreateSystemState]
    CheckMQTT -->|No| LogOnly[Log state change only]
    
    FormatJSON --> PublishRetain[MQTT Publish with retain=1<br/>datalogger/esp32/system/state]
    PublishRetain --> BrokerStore[Broker stores retained message]
    BrokerStore --> WebUpdate[Web receives state update]
    WebUpdate --> UpdateUI[Web UI updates display]
    UpdateUI --> SyncComplete
    
    LogOnly --> SyncComplete[State logged locally]
    
    MQTTChange --> CheckMQTTState{New State?}
    CheckMQTTState -->|CONNECTED| MQTTConnected[MQTT connection established]
    CheckMQTTState -->|DISCONNECTED| MQTTDisconnected[MQTT connection lost]
    
    MQTTConnected --> NotifyConn[Send MQTT CONNECTED to STM32]
    MQTTDisconnected --> NotifyDisc[Send MQTT DISCONNECTED to STM32]
    
    NotifyConn --> STM32Switch1[STM32 switches to live transmission]
    NotifyDisc --> STM32Switch2[STM32 switches to SD buffering]
    
    STM32Switch1 --> CheckBuffer{Has buffered data?}
    CheckBuffer -->|Yes| TransmitBuffer[Transmit SD buffer]
    CheckBuffer -->|No| SyncComplete
    
    TransmitBuffer --> SyncComplete
    STM32Switch2 --> SyncComplete
    
    SyncComplete --> Done([Done])
    
    style Start fill:#90EE90
    style StateType fill:#FFD700
    style CheckMQTT fill:#FFD700
    style PublishRetain fill:#87CEEB
```

## Legend

```mermaid
flowchart LR
    Start([Start/End Point])
    Decision{Decision Point}
    Process[Process/Action]
    Critical[Critical State]
    External[External System]
    
    style Start fill:#90EE90
    style Decision fill:#FFD700
    style Critical fill:#FF6B6B
    style External fill:#87CEEB
```

---

## Key System Features

### Communication Protocol
- **UART**: 115200 baud, 8N1, line-based JSON protocol
- **I2C**: 100kHz for sensors (SHT3X 0x44, DS3231 0x68)
- **SPI**: 18MHz for SD Card, 36MHz for Display
- **MQTT**: v5.0, QoS 0/1, retained state messages

### Timing Requirements
- **WiFi Stabilization**: 4-second delay before MQTT start
- **STM32 Boot Delay**: 500ms after relay toggle
- **Button Debounce**: 200ms software filter
- **Periodic Intervals**: 5s, 10s, 15s, 30s, 60s (user selectable)
- **SD Transmission Rate**: 100ms between records

### State Management
- **Device State**: g_device_on (relay control)
- **Periodic State**: g_periodic_active (measurement mode)
- **MQTT State**: mqtt_current_state (connection status)
- **State Sync**: MQTT retained messages for web synchronization

### Error Recovery
- **WiFi**: 5 auto-retries @ 2s, then manual retry @ 5s
- **MQTT**: Exponential backoff min(60s, 2^retry)
- **Sensor**: Return 0.0 values, continue operation
- **RTC**: Return timestamp=0, continue operation
- **SD Card**: Circular buffer, 204,800 records capacity

### Data Flow Priority
1. **Real-time**: When MQTT connected, direct transmission
2. **Buffered**: When MQTT disconnected, SD card storage
3. **Catch-up**: When MQTT reconnects, transmit buffered data first
4. **Rate Limited**: 100ms delay between buffered transmissions
