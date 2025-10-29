# ESP32 Firmware - Flow Diagrams

This document describes the operational flows and control logic within the ESP32 firmware.

## System Startup Flow

```mermaid
flowchart TD
    Start([ESP32 Power On]) --> NVSInit[Initialize NVS Flash]
    NVSInit --> EventInit[Initialize Event Loop]
    EventInit --> NetifInit[Initialize Network Interface]

    NetifInit --> InitWiFi[Initialize WiFi Manager]
    InitWiFi --> InitUART[Initialize STM32 UART]
    InitUART --> InitLED[Initialize LED GPIOs]
    InitLED --> ConnectWiFi[Connect WiFi<br/>15s Timeout]

    ConnectWiFi --> CheckWiFi{WiFi connected<br/>within 15s?}
    CheckWiFi -->|Yes| MarkTime[Mark connection time]
    CheckWiFi -->|No| WarnRetry[Log warning, continue<br/>WiFi will retry in background]

    MarkTime --> InitComponents[Initialize Components:<br/>MQTT, Relay, JSON Parser, Buttons]
    WarnRetry --> InitComponents

    InitComponents --> CheckInit{All Components<br/>initialized OK?}
    CheckInit -->|No| Restart[ESP32 Restart]
    CheckInit -->|Yes| StartServices[Start Services<br/>Button Task, UART RX Task]

    StartServices --> MainLoop[Main Loop]

    style Start fill:#90EE90, color:#000000
    style MainLoop fill:#FFD700, color:#000000
    style CheckWiFi fill:#FFD700, color:#000000
    style CheckInit fill:#FFD700, color:#000000
    style Restart fill:#FF6B6B, color:#000000
```

## Main Loop Flow

```mermaid
flowchart TD
    MainLoop[Main Loop] --> CheckWiFiState{WiFi State?}

    CheckWiFiState -->|FAILED| CheckRetry{5s passed since<br/>last retry?}
    CheckRetry -->|Yes| RetryWiFi[WiFi Manager Connect]
    CheckRetry -->|No| CheckStabilize[Check stabilization]
    RetryWiFi --> CheckStabilize

    CheckWiFiState -->|CONNECTED| CheckStabilize{WiFi stable<br/>for 4s?}
    CheckStabilize -->|Yes & MQTT not started| StartMQTT[MQTT Handler Start]
    CheckStabilize -->|No| CheckMQTTState[Check MQTT state]
    StartMQTT --> CheckMQTTState

    CheckWiFiState -->|DISCONNECTED| StopMQTT[MQTT Handler Stop]
    StopMQTT --> CheckMQTTState

    CheckMQTTState{MQTT State<br/>changed?}
    CheckMQTTState -->|Connected| NotifySTM32Conn[Send MQTT CONNECTED to STM32]
    CheckMQTTState -->|Disconnected| NotifySTM32Disc[Send MQTT DISCONNECTED to STM32]
    CheckMQTTState -->|No change| LogStatus[Log status every 30s]

    NotifySTM32Conn --> SetLED[Turn on MQTT LED]
    NotifySTM32Disc --> ClearLED[Turn off MQTT LED]
    SetLED --> LogStatus
    ClearLED --> LogStatus

    LogStatus --> Delay[Delay 100ms]
    Delay --> MainLoop

    style MainLoop fill:#FFD700, color:#000000
    style CheckWiFiState fill:#FFD700, color:#000000
    style CheckMQTTState fill:#FFD700, color:#000000
```

## MQTT Message Processing Flow

```mermaid
flowchart TD
    Start([Receive MQTT Message]) --> CheckTopic{Check Topic?}

    CheckTopic -->|datalogger/stm32/command| ForwardSTM32[Forward to STM32]
    CheckTopic -->|datalogger/esp32/relay/control| ProcessRelay[Process relay control]
    CheckTopic -->|datalogger/esp32/system/state| CheckRequest{Contains<br/>REQUEST?}
    CheckTopic -->|Other| Ignore([Ignore])

    ForwardSTM32 --> SendUART[STM32_UART_SendCommand]
    SendUART --> CheckCmd{Command Type?}
    CheckCmd -->|PERIODIC ON| UpdateStatePOn[Update state periodic=true]
    CheckCmd -->|PERIODIC OFF| UpdateStatePOff[Update state periodic=false]
    CheckCmd -->|Other| Done([Complete])

    UpdateStatePOn --> Done
    UpdateStatePOff --> Done

    ProcessRelay --> CheckRelayCmd{Relay Command?}
    CheckRelayCmd -->|ON| SetRelayOn[Relay_SetState true]
    CheckRelayCmd -->|OFF| SetRelayOff[Relay_SetState false]
    CheckRelayCmd -->|Invalid| Done

    SetRelayOn --> Done
    SetRelayOff --> Done

    CheckRequest -->|Yes & mqtt_reconnected flag| PublishState[publish_current_state]
    CheckRequest -->|No| Done
    PublishState --> ClearFlag[Clear mqtt_reconnected flag]
    ClearFlag --> Done

    style Start fill:#90EE90, color:#000000
    style CheckTopic fill:#FFD700, color:#000000
    style CheckCmd fill:#FFD700, color:#000000
```

## STM32 Data Processing Flow

```mermaid
flowchart TD
    Start([Receive UART line from STM32]) --> ParseJSON[JSON_Parser_ProcessLine]
    ParseJSON --> ValidateJSON{Valid JSON?}

    ValidateJSON -->|No| LogError[Log parse error]
    ValidateJSON -->|Yes| CheckMode{Mode?}

    LogError --> Done([Complete])

    CheckMode -->|SINGLE| CallbackSingle[on_single_sensor_data callback]
    CheckMode -->|PERIODIC| CallbackPeriodic[on_periodic_sensor_data callback]

    CallbackSingle --> FormatJSON1[JSON_Utils_CreateSensorData]
    CallbackPeriodic --> FormatJSON2[JSON_Utils_CreateSensorData]

    FormatJSON1 --> PublishSingle[MQTT_Handler_Publish<br/>datalogger/stm32/single/data]
    FormatJSON2 --> PublishPeriodic[MQTT_Handler_Publish<br/>datalogger/stm32/periodic/data]

    PublishSingle --> LogData[Log sensor values]
    PublishPeriodic --> LogData

    LogData --> Done

    style Start fill:#90EE90, color:#000000
    style ValidateJSON fill:#FFD700, color:#000000
    style CheckMode fill:#FFD700, color:#000000
```

## Relay State Change Flow

```mermaid
flowchart TD
    Start([Relay state changed]) --> LogState[Log new state]
    LogState --> CheckOff{Relay<br/>turned OFF?}

    CheckOff -->|Yes| ForcePeriodic[Force periodic=false]
    CheckOff -->|No| KeepPeriodic[Keep periodic state]

    ForcePeriodic --> UpdateState[update_and_publish_state]
    KeepPeriodic --> UpdateState

    UpdateState --> Delay[Wait 500ms for STM32 boot]
    Delay --> CheckMQTT{MQTT<br/>connected?}

    CheckMQTT -->|Yes| SendConnected[Send MQTT CONNECTED]
    CheckMQTT -->|No| SendDisconnected[Send MQTT DISCONNECTED]

    SendConnected --> Done([Complete])
    SendDisconnected --> Done

    style Start fill:#90EE90, color:#000000
    style CheckOff fill:#FFD700, color:#000000
    style CheckMQTT fill:#FFD700, color:#000000
    style UpdateState fill:#FF6B6B, color:#000000
```

## Button Press Processing Flow

```mermaid
flowchart TD
    Start([Button pressed]) --> Debounce[Hardware + software noise filtering]
    Debounce --> CheckButton{Which button?}

    CheckButton -->|GPIO_5 Relay| ToggleRelay[Toggle relay state]
    CheckButton -->|GPIO_17 Single| CheckDevOn1{Device ON?}
    CheckButton -->|GPIO_16 Periodic| CheckDevOn2{Device ON?}
    CheckButton -->|GPIO_4 Interval| CheckDevOn3{Device ON?}

    ToggleRelay --> UpdateGlobal[Update g_device_on]
    UpdateGlobal --> CheckNewState{New state<br/>OFF?}
    CheckNewState -->|Yes| ForcePeriodicOff[Force g_periodic_active=false]
    CheckNewState -->|No| StateCallback[Relay callback processing]
    ForcePeriodicOff --> StateCallback
    StateCallback --> Done([Complete])

    CheckDevOn1 -->|No| LogIgnore1[Log ignore - device OFF]
    CheckDevOn1 -->|Yes| SendSingle[Send SINGLE to STM32]
    LogIgnore1 --> Done
    SendSingle --> Done

    CheckDevOn2 -->|No| LogIgnore2[Log ignore - device OFF]
    CheckDevOn2 -->|Yes| TogglePeriodic[Toggle g_periodic_active]
    LogIgnore2 --> Done
    TogglePeriodic --> FormatCmd{New periodic<br/>state?}
    FormatCmd -->|true| SendPOn[Send PERIODIC ON]
    FormatCmd -->|false| SendPOff[Send PERIODIC OFF]
    SendPOn --> UpdatePublish[update_and_publish_state]
    SendPOff --> UpdatePublish
    UpdatePublish --> Done

    CheckDevOn3 -->|No| LogIgnore3[Log ignore - device OFF]
    CheckDevOn3 -->|Yes| CycleInterval[Cycle g_interval_index]
    LogIgnore3 --> Done
    CycleInterval --> BuildCmd[Create SET PERIODIC INTERVAL command]
    BuildCmd --> SendInterval[Send command to STM32]
    SendInterval --> Done

    style Start fill:#90EE90, color:#000000
    style CheckButton fill:#FFD700, color:#000000
    style CheckNewState fill:#FFD700, color:#000000
```

## WiFi State Change Flow

```mermaid
flowchart TD
    Start([WiFi state changed]) --> CheckState{WiFi State?}

    CheckState -->|CONNECTING| LogConn[Log Connecting...]
    CheckState -->|CONNECTED| LogSuccess[Log Connected]
    CheckState -->|DISCONNECTED| LogDisc[Log Disconnected]
    CheckState -->|FAILED| LogFail[Log Connection failed]

    LogConn --> LEDOff1[Turn off WiFi LED]
    LogSuccess --> LEDOn[Turn on WiFi LED]
    LogDisc --> LEDOff2[Turn off WiFi LED]
    LogFail --> LEDOff3[Turn off WiFi LED]

    LEDOn --> GetIP[Get IP address]
    GetIP --> GetRSSI[Get signal strength RSSI]
    GetRSSI --> ResetFlag[Reset disconnect flag]
    ResetFlag --> Done([Complete])

    LEDOff1 --> Done
    LEDOff2 --> Done
    LEDOff3 --> Done

    style Start fill:#90EE90, color:#000000
    style CheckState fill:#FFD700, color:#000000
```

## State Update and Publish Flow

```mermaid
flowchart TD
    Start([update_and_publish_state called]) --> CheckDevice{g_device_on<br/>changed?}

    CheckDevice -->|Yes| SetDevChanged[state_changed = true]
    CheckDevice -->|No| CheckPeriodic[Check periodic]

    SetDevChanged --> LogDevice[Log device state change]
    LogDevice --> CheckPeriodic{g_periodic_active<br/>changed?}

    CheckPeriodic -->|Yes| SetPerChanged[state_changed = true]
    CheckPeriodic -->|No| CheckChanged[Check if changed]

    SetPerChanged --> LogPeriodic[Log periodic state change]
    LogPeriodic --> CheckChanged{state_changed<br/>== true?}

    CheckChanged -->|Yes| CheckMQTT{MQTT<br/>connected?}
    CheckChanged -->|No| Done([Complete])

    CheckMQTT -->|Yes| FormatJSON[Format JSON state]
    CheckMQTT -->|No| Done

    FormatJSON --> PublishMQTT[MQTT_Handler_Publish<br/>datalogger/esp32/system/state<br/>retain=1]
    PublishMQTT --> LogPublish[Log state published]
    LogPublish --> Done

    style Start fill:#90EE90, color:#000000
    style CheckDevice fill:#FFD700, color:#000000
    style CheckPeriodic fill:#FFD700, color:#000000
    style CheckChanged fill:#FFD700, color:#000000
```

## MQTT Connection State Machine

```mermaid
flowchart TD
    Start([MQTT Handler]) --> CheckWiFi{WiFi<br/>connected?}

    CheckWiFi -->|No| StateDisc[State: DISCONNECTED]
    CheckWiFi -->|Yes| Check4s{Network stable<br/>for 4s?}

    Check4s -->|No| Wait[Wait for stabilization]
    Check4s -->|Yes| StartClient[esp_mqtt_client_start]

    Wait --> CheckWiFi
    StartClient --> StateConn[State: CONNECTING]

    StateConn --> WaitEvent{MQTT Event?}

    WaitEvent -->|CONNECTED| SetConnected[connected = true]
    WaitEvent -->|DISCONNECTED| SetDisconnected[connected = false]
    WaitEvent -->|ERROR| HandleError[Exponential backoff retry]

    SetConnected --> SubTopics[Subscribe command topics]
    SubTopics --> SetReconnFlag[Set mqtt_reconnected flag]
    SetReconnFlag --> StateActive[State: ACTIVE]

    SetDisconnected --> StateDisc
    HandleError --> CheckRetry{Max retries<br/>reached?}

    CheckRetry -->|Yes| StateDisc
    CheckRetry -->|No| BackoffDelay[Delay: exponential backoff up to 60s]
    BackoffDelay --> StartClient

    StateActive --> WaitMsg{New message?}
    WaitMsg -->|DATA| CallCallback[Call data_callback]
    WaitMsg -->|DISCONNECT| SetDisconnected

    CallCallback --> WaitMsg

    style Start fill:#90EE90, color:#000000
    style CheckWiFi fill:#FFD700, color:#000000
    style WaitEvent fill:#FFD700, color:#000000
    style StateActive fill:#87CEEB, color:#000000
    style StateDisc fill:#FF6B6B, color:#000000
```

## Legend

```mermaid
flowchart LR
    Start([Start/End])
    Process[Process]
    Decision{Decision}
    Critical[Critical state change]

    style Start fill:#90EE90, color:#000000
    style Decision fill:#FFD700, color:#000000
    style Critical fill:#FF6B6B, color:#000000
```

---

**Important Notes:**

- **Green nodes**: Start/end points
- **Yellow nodes**: Decision points
- **Red nodes**: Critical state changes
- **Blue nodes**: Active/stable states

**Flow processing characteristics:**

- WiFi manager auto-retry (5 times with 2s interval)
- MQTT starts only after WiFi is stable for 4s
- All button actions require device (relay) to be ON (except relay toggle)
- Relay state change triggers 500ms delay before sending MQTT status to STM32 (for STM32 boot)
- Exponential backoff for MQTT retry (min 1s, max 60s)
- State synchronization via MQTT retained messages
