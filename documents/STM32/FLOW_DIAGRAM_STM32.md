# STM32 Data Logger - Flow Diagram

This document describes the control flow and decision logic within the STM32 firmware.

## Main Application Flow

```mermaid
flowchart TD
    Start([System Power On]) --> Init[Initialize System]
    Init --> InitUART[Initialize UART]
    InitUART --> InitSHT3X[Initialize SHT3X Sensor]
    InitSHT3X --> InitDS3231[Initialize DS3231 RTC]
    InitDS3231 --> InitDataMgr[Initialize DataManager]
    InitDataMgr --> InitSD[Initialize SD Card Manager]
    InitSD --> InitDisplay[Initialize ILI9225 TFT Display]
    InitDisplay --> MainLoop{Main Loop}
    
    MainLoop --> HandleUART[UART_Handle]
    HandleUART --> CheckPeriodic{In Periodic\nMode?}
    
    CheckPeriodic -->|Yes| CheckTime{Time to\nFetch?}
    CheckPeriodic -->|No| CheckMQTT{MQTT\nConnected?}
    CheckTime -->|Yes| FetchData[SHT3X_FetchData]
    CheckTime -->|No| CheckMQTT
    FetchData --> UpdatePeriodic[DataManager_UpdatePeriodic]
    UpdatePeriodic --> CheckMQTT
    
    CheckMQTT -->|Yes| PrintLive[DataManager_Print Live Data]
    CheckMQTT -->|No| BufferSD[SDCardManager_WriteData]
    
    PrintLive --> SendBuffered{Has Buffered\nData?}
    SendBuffered -->|Yes| ReadSD[SDCardManager_ReadData]
    SendBuffered -->|No| UpdateDisplay
    ReadSD --> TransmitSD[Transmit Buffered JSON]
    TransmitSD --> RemoveSD[SDCardManager_RemoveRecord]
    RemoveSD --> UpdateDisplay[Display_Update]
    
    BufferSD --> UpdateDisplay
    UpdateDisplay --> MainLoop
    
    style Start fill:#90EE90
    style MainLoop fill:#FFD700
    style CheckMQTT fill:#FFD700
    style BufferSD fill:#FF6B6B
```

## Command Execution Flow

```mermaid
flowchart TD
    RxChar([UART RX Interrupt]) --> RingBuf[Store in Ring Buffer]
    RingBuf --> UARTHandle[UART_Handle Called]
    UARTHandle --> CheckNewline{Newline\nDetected?}
    
    CheckNewline -->|No| Return1([Return])
    CheckNewline -->|Yes| Tokenize[Tokenize Command String]
    Tokenize --> BuildCmd[Build Command from Tokens]
    BuildCmd --> FindCmd{Find in\nCommand Table?}
    
    FindCmd -->|Not Found| UnknownCmd[Print Unknown Command]
    FindCmd -->|Found| CallParser[Call Parser Function]
    UnknownCmd --> Return2([Return])
    
    CallParser --> IsSINGLE{SINGLE\nCommand?}
    IsSINGLE -->|Yes| SingleParser[SINGLE_PARSER]
    IsSINGLE -->|No| IsPERIODIC{PERIODIC\nCommand?}
    
    IsPERIODIC -->|ON| PeriodicOnParser[PERIODIC_ON_PARSER]
    IsPERIODIC -->|OFF| PeriodicOffParser[PERIODIC_OFF_PARSER]
    IsPERIODIC -->|No| IsSETTIME{SET TIME\nCommand?}
    
    IsSETTIME -->|Yes| SetTimeParser[SET_TIME_PARSER]
    IsSETTIME -->|No| IsINTERVAL{SET PERIODIC\nINTERVAL?}
    
    IsINTERVAL -->|Yes| IntervalParser[SET_PERIODIC_INTERVAL_PARSER]
    IsINTERVAL -->|No| IsMQTT{MQTT\nNotification?}
    
    IsMQTT -->|CONNECTED| MQTTConnParser[MQTT_CONNECTED_PARSER]
    IsMQTT -->|DISCONNECTED| MQTTDiscParser[MQTT_DISCONNECTED_PARSER]
    IsMQTT -->|No| OtherCmd[Other Command Handler]
    
    SingleParser --> CallDriver1[SHT3X_Single]
    CallDriver1 --> UpdateSingle[DataManager_UpdateSingle]
    UpdateSingle --> SetFlag[Set data_ready Flag]
    
    PeriodicOnParser --> CallDriver2[SHT3X_Periodic]
    CallDriver2 --> UpdatePeriodicMode[DataManager_UpdatePeriodic]
    UpdatePeriodicMode --> SetFlag
    
    PeriodicOffParser --> CallDriver3[SHT3X_PeriodicStop]
    CallDriver3 --> PrintResult[Print Status]
    
    SetTimeParser --> CallRTC[DS3231_Set_Time]
    CallRTC --> ForceDisplay[Set force_display_update]
    ForceDisplay --> PrintResult
    
    IntervalParser --> SetInterval[Update periodic_interval_ms]
    SetInterval --> PrintResult
    
    MQTTConnParser --> UpdateMQTTState[mqtt_current_state = CONNECTED]
    MQTTDiscParser --> UpdateMQTTState2[mqtt_current_state = DISCONNECTED]
    UpdateMQTTState --> PrintResult
    UpdateMQTTState2 --> PrintResult
    
    SetFlag --> Return3([Return])
    PrintResult --> Return3
    OtherCmd --> Return3
    
    style RxChar fill:#90EE90
    style FindCmd fill:#FFD700
    style IsSINGLE fill:#FFD700
    style IsMQTT fill:#FFD700
    style SetFlag fill:#FF6B6B
```

## SHT3X Single Measurement Flow

```mermaid
flowchart TD
    Start([SINGLE_PARSER]) --> ValidateArgs{argc == 1?}
    ValidateArgs -->|No| Return1([Return])
    ValidateArgs -->|Yes| SetRepeat[Set Repeatability = HIGH]
    
    SetRepeat --> I2CCmd[Send I2C Command]
    I2CCmd --> Wait[HAL_Delay Based on Repeatability]
    Wait --> ReadI2C[I2C Read 6 Bytes]
    ReadI2C --> CheckI2C{I2C Success?}
    
    CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]
    CheckI2C -->|Yes| CheckCRC{CRC Valid?}
    
    CheckCRC -->|No| SetZero
    CheckCRC -->|Yes| ParseRaw[Parse Raw Temperature & Humidity]
    
    SetZero --> UpdateDM[DataManager_UpdateSingle]
    ParseRaw --> StoreValues[Store in sht3x_t Structure]
    StoreValues --> UpdateDM
    UpdateDM --> SetFlag[Set data_ready Flag]
    SetFlag --> Return2([Return OK])
    
    style Start fill:#90EE90
    style CheckI2C fill:#FFD700
    style CheckCRC fill:#FFD700
    style UpdateDM fill:#FF6B6B
```

## SHT3X Periodic Measurement Flow

```mermaid
flowchart TD
    Start([PERIODIC_ON_PARSER]) --> ValidateArgs{argc == 2?}
    ValidateArgs -->|No| Return1([Return])
    ValidateArgs -->|Yes| SetDefaults[Set Rate=1MPS, Repeatability=HIGH]
    
    SetDefaults --> BuildI2C[Build I2C Command Word]
    BuildI2C --> SendI2C[Send I2C Periodic Start]
    SendI2C --> CheckI2C{I2C Success?}
    
    CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]
    CheckI2C -->|Yes| UpdateState[Update currentState]
    
    UpdateState --> InitTiming[Initialize next_fetch_ms]
    InitTiming --> FirstFetch[SHT3X_FetchData]
    FirstFetch --> CheckFetch{Fetch Success?}
    
    CheckFetch -->|No| SetZero
    CheckFetch -->|Yes| UpdateDM[DataManager_UpdatePeriodic]
    SetZero --> UpdateDM
    UpdateDM --> SetFlag[Set data_ready Flag]
    SetFlag --> Return2([Return OK])
    
    style Start fill:#90EE90
    style CheckI2C fill:#FFD700
    style UpdateDM fill:#FF6B6B
```

## Periodic Measurement Stop Flow

```mermaid
flowchart TD
    Start([PERIODIC_OFF_PARSER]) --> ValidateArgs{argc == 2?}
    ValidateArgs -->|No| Return1([Return])
    ValidateArgs -->|Yes| StopCmd[Send Stop Command 0x3093]
    
    StopCmd --> CheckI2C{I2C Success?}
    CheckI2C -->|No| PrintError[Print Error]
    CheckI2C -->|Yes| ResetState[currentState = SHT3X_IDLE]
    
    ResetState --> PrintStop[Print Stop Success]
    PrintStop --> Return2([Return])
    PrintError --> Return2
    
    style Start fill:#90EE90
    style CheckI2C fill:#FFD700
```

## Data Manager Print Decision Flow

```mermaid
flowchart TD
    Start([DataManager_Print Called]) --> CheckReady{data_ready\nFlag Set?}
    CheckReady -->|No| Return1([Return false])
    CheckReady -->|Yes| CheckMode{Current Mode?}
    
    CheckMode -->|SINGLE| GetTimeSingle[Get Unix Timestamp from DS3231]
    CheckMode -->|PERIODIC| GetTimePeriodic[Get Unix Timestamp from DS3231]
    CheckMode -->|UNKNOWN| Return2([Return false])
    
    GetTimeSingle --> SanitizeSingle[Sanitize Float Values]
    GetTimePeriodic --> SanitizePeriodic[Sanitize Float Values]
    
    SanitizeSingle --> CheckMQTT{MQTT\nConnected?}
    SanitizePeriodic --> CheckMQTT
    
    CheckMQTT -->|Yes| FormatJSON[Format JSON String]
    CheckMQTT -->|No| BufferToSD[SDCardManager_WriteData]
    
    FormatJSON --> CheckOverflow{Buffer\nOverflow?}
    BufferToSD --> ClearFlag[Clear data_ready]
    
    CheckOverflow -->|Yes| PrintOverflow[Print Overflow Error]
    CheckOverflow -->|No| TransmitUART[HAL_UART_Transmit JSON]
    PrintOverflow --> ClearFlag
    
    TransmitUART --> ClearFlag
    ClearFlag --> Return3([Return true])
    
    style Start fill:#90EE90
    style CheckReady fill:#FFD700
    style CheckMode fill:#FFD700
    style CheckMQTT fill:#FFD700
    style TransmitUART fill:#FF6B6B
```

## Periodic Data Fetch Decision Flow

```mermaid
flowchart TD
    Start([Main Loop Iteration]) --> CheckPeriodicState{In Periodic\nState?}
    CheckPeriodicState -->|No| Skip([Skip Fetch])
    CheckPeriodicState -->|Yes| GetNow[now = HAL_GetTick]
    
    GetNow --> CheckTime{now >=\nnext_fetch_ms?}
    CheckTime -->|No| Skip
    CheckTime -->|Yes| CheckDuplicate{now ==\nlast_fetch_ms?}
    
    CheckDuplicate -->|Yes| Skip
    CheckDuplicate -->|No| FetchData[SHT3X_FetchData]
    
    FetchData --> I2CRead[I2C Read Data from Sensor]
    I2CRead --> CheckI2C{I2C Success?}
    
    CheckI2C -->|No| Skip
    CheckI2C -->|Yes| ParseData[Parse Temperature & Humidity]
    
    ParseData --> UpdateDM[DataManager_UpdatePeriodic]
    UpdateDM --> ToggleGPIO[Toggle PC13 LED]
    ToggleGPIO --> RecordTime[last_fetch_ms = now]
    RecordTime --> ScheduleNext[next_fetch_ms = now + interval]
    ScheduleNext --> Done([Continue Main Loop])
    
    style Start fill:#90EE90
    style CheckPeriodicState fill:#FFD700
    style CheckTime fill:#FFD700
    style UpdateDM fill:#FF6B6B
```

## MQTT-Aware Data Routing Flow

```mermaid
flowchart TD
    Start([Main Loop - After Periodic Fetch]) --> CheckMQTT{MQTT\nConnected?}
    
    CheckMQTT -->|Yes| PrintLive[DataManager_Print - Send Live Data]
    CheckMQTT -->|No| BufferData[SDCardManager_WriteData - Buffer to SD]
    
    PrintLive --> CheckBuffered{Has Buffered\nSD Data?}
    BufferData --> UpdateDisplay[Display_Update]
    
    CheckBuffered -->|Yes| CheckDelay{100ms Delay\nPassed?}
    CheckBuffered -->|No| UpdateDisplay
    
    CheckDelay -->|Yes| ReadSD[SDCardManager_ReadData]
    CheckDelay -->|No| UpdateDisplay
    
    ReadSD --> FormatJSON[Format Buffered JSON]
    FormatJSON --> TransmitSD[Transmit via UART]
    TransmitSD --> RemoveSD[SDCardManager_RemoveRecord]
    RemoveSD --> UpdateDisplay
    UpdateDisplay --> Done([Continue Main Loop])
    
    style Start fill:#90EE90
    style CheckMQTT fill:#FFD700
    style CheckBuffered fill:#FFD700
    style BufferData fill:#FF6B6B
```

## Error Handling Flow

```mermaid
flowchart TD
    Start([Error Detected]) --> CheckType{Error Type?}
    
    CheckType -->|I2C Timeout| I2CError[HAL_I2C_GetError]
    CheckType -->|CRC Mismatch| CRCError[Discard Data]
    CheckType -->|SD Card Error| SDError[Continue Without Buffering]
    CheckType -->|Buffer Overflow| BufferError[Report Overflow]
    CheckType -->|Invalid Command| CmdError[Print Unknown]
    CheckType -->|Invalid Parameter| ParamError[Print Usage]
    
    I2CError --> LogI2C[Log Error Code]
    CRCError --> SetZeroData[Set temp=0.0, hum=0.0]
    SDError --> LogSD[Warn SD Not Available]
    BufferError --> LogBuffer[Log Buffer State]
    CmdError --> LogCmd[Log Command String]
    ParamError --> LogParam[Log Parameter]
    
    LogI2C --> ReturnError([Return Error Status])
    SetZeroData --> UpdateWithZero[DataManager_Update with 0.0]
    LogSD --> ContinueOp[Continue Without SD]
    LogBuffer --> ReturnError
    LogCmd --> ReturnError
    LogParam --> ReturnError
    
    UpdateWithZero --> PreserveState{Preserve\nState?}
    ContinueOp --> PreserveState
    ReturnError --> PreserveState
    
    PreserveState -->|Yes| KeepState[Maintain Current Mode]
    PreserveState -->|No| ResetState[Reset to IDLE]
    
    KeepState --> End([Continue Operation])
    ResetState --> End
    
    style Start fill:#FF6B6B
    style CheckType fill:#FFD700
    style ReturnError fill:#FF6B6B
```

## Legend

```mermaid
flowchart LR
    Start([Start/End - Rounded]) 
    Process[Process - Rectangle]
    Decision{Decision - Diamond}
    Flag[Critical State Change - Rectangle]
    
    style Start fill:#90EE90
    style Decision fill:#FFD700
    style Flag fill:#FF6B6B
```

---

**Notes:**
- Green nodes: Entry/exit points
- Yellow nodes: Decision points
- Red nodes: State changes or critical operations
- Blue nodes: Wait states or returns
- All flows are non-blocking except I2C operations and delays
