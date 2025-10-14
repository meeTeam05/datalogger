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
    InitDataMgr --> MainLoop{Main Loop}
    
    MainLoop --> HandleUART[UART_Handle]
    HandleUART --> CheckData{DataManager\nHas Data?}
    CheckData -->|Yes| PrintData[DataManager_Print]
    CheckData -->|No| CheckPeriodic
    PrintData --> CheckPeriodic{In Periodic\nMode?}
    
    CheckPeriodic -->|Yes| CheckTime{Time to\nFetch?}
    CheckPeriodic -->|No| WFI[Wait For Interrupt]
    CheckTime -->|Yes| FetchData[SHT3X_FetchData]
    CheckTime -->|No| WFI
    FetchData --> UpdatePeriodic[DataManager_UpdatePeriodic]
    UpdatePeriodic --> WFI
    WFI --> MainLoop
    
    style Start fill:#90EE90
    style MainLoop fill:#FFD700
    style WFI fill:#87CEEB
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
    
    CallParser --> IsSHT3X{SHT3X\nCommand?}
    IsSHT3X -->|Single| ParseSingle[Parse Repeatability]
    IsSHT3X -->|Periodic| ParsePeriodic[Parse Rate & Repeatability]
    IsSHT3X -->|Heater| ParseHeater[Parse Enable/Disable]
    IsSHT3X -->|Other| OtherCmd[Other Command Handler]
    
    ParseSingle --> CallDriver1[SHT3X_Single]
    ParsePeriodic --> CallDriver2[SHT3X_Periodic]
    ParseHeater --> CallDriver3[SHT3X_Heater]
    
    CallDriver1 --> UpdateSingle[DataManager_UpdateSingle]
    CallDriver2 --> UpdatePeriodicMode[Update Periodic State]
    CallDriver3 --> PrintResult[Print Status]
    
    UpdateSingle --> SetFlag[Set data_ready Flag]
    UpdatePeriodicMode --> SetFlag
    PrintResult --> Return3([Return])
    SetFlag --> Return3
    OtherCmd --> Return3
    
    style RxChar fill:#90EE90
    style FindCmd fill:#FFD700
    style IsSHT3X fill:#FFD700
    style SetFlag fill:#FF6B6B
```

## SHT3X Single Measurement Flow

```mermaid
flowchart TD
    Start([SHT3X_Single_Parser]) --> ValidateArgs{argc >= 3?}
    ValidateArgs -->|No| PrintUsage[Print Usage Info]
    ValidateArgs -->|Yes| ParseRepeat[Parse Repeatability]
    PrintUsage --> Return1([Return])
    
    ParseRepeat --> IsHigh{HIGH?}
    IsHigh -->|Yes| SetHigh[modeRepeat = SHT3X_HIGH]
    IsHigh -->|No| IsMedium{MEDIUM?}
    IsMedium -->|Yes| SetMedium[modeRepeat = SHT3X_MEDIUM]
    IsMedium -->|No| IsLow{LOW?}
    IsLow -->|Yes| SetLow[modeRepeat = SHT3X_LOW]
    IsLow -->|No| Return2([Return Invalid])
    
    SetHigh --> I2CCmd[Send I2C Command]
    SetMedium --> I2CCmd
    SetLow --> I2CCmd
    
    I2CCmd --> Wait[HAL_Delay Based on Repeatability]
    Wait --> ReadI2C[I2C Read 6 Bytes]
    ReadI2C --> CheckI2C{I2C Success?}
    
    CheckI2C -->|No| PrintError[Print Error]
    CheckI2C -->|Yes| CheckCRC{CRC Valid?}
    PrintError --> Return3([Return ERROR])
    
    CheckCRC -->|No| Return3
    CheckCRC -->|Yes| ParseRaw[Parse Raw Temperature & Humidity]
    ParseRaw --> StoreValues[Store in sht3x_t Structure]
    StoreValues --> UpdateDM[DataManager_UpdateSingle]
    UpdateDM --> Return4([Return OK])
    
    style Start fill:#90EE90
    style CheckI2C fill:#FFD700
    style CheckCRC fill:#FFD700
    style UpdateDM fill:#FF6B6B
```

## SHT3X Periodic Measurement Flow

```mermaid
flowchart TD
    Start([SHT3X_Periodic_Parser]) --> CheckStop{argv[2] ==\nSTOP?}
    CheckStop -->|Yes| StopCmd[Send Stop Command]
    CheckStop -->|No| ValidateArgs{argc >= 4?}
    
    StopCmd --> ResetState[currentState = SHT3X_IDLE]
    ResetState --> PrintStop[Print Stop Success]
    PrintStop --> Return1([Return])
    
    ValidateArgs -->|No| PrintUsage[Print Usage]
    ValidateArgs -->|Yes| ParseRate[Parse Rate: 0.5/1/2/4/10]
    PrintUsage --> Return2([Return])
    
    ParseRate --> ParseRepeat[Parse Repeatability: HIGH/MEDIUM/LOW]
    ParseRepeat --> BuildI2C[Build I2C Command Word]
    BuildI2C --> SendI2C[Send I2C Periodic Start]
    SendI2C --> CheckI2C{I2C Success?}
    
    CheckI2C -->|No| PrintError[Print Error]
    CheckI2C -->|Yes| UpdateState[Update currentState]
    PrintError --> Return3([Return ERROR])
    
    UpdateState --> InitTiming[Initialize next_fetch_ms]
    InitTiming --> FirstFetch[SHT3X_FetchData]
    FirstFetch --> CheckFetch{Fetch Success?}
    
    CheckFetch -->|No| Return4([Return ERROR])
    CheckFetch -->|Yes| UpdateDM[DataManager_UpdatePeriodic]
    UpdateDM --> Return5([Return OK])
    
    style Start fill:#90EE90
    style CheckStop fill:#FFD700
    style CheckI2C fill:#FFD700
    style UpdateDM fill:#FF6B6B
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
    
    SanitizeSingle --> FormatSingleJSON[Format Single JSON]
    SanitizePeriodic --> FormatPeriodicJSON[Format Periodic JSON]
    
    FormatSingleJSON --> CheckOverflow{Buffer\nOverflow?}
    FormatPeriodicJSON --> CheckOverflow
    
    CheckOverflow -->|Yes| PrintOverflow[Print Overflow Error]
    CheckOverflow -->|No| TransmitUART[HAL_UART_Transmit JSON]
    PrintOverflow --> ClearFlag[Clear data_ready]
    
    TransmitUART --> ClearFlag
    ClearFlag --> Return3([Return true])
    
    style Start fill:#90EE90
    style CheckReady fill:#FFD700
    style CheckMode fill:#FFD700
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
    UpdateDM --> RecordTime[last_fetch_ms = now]
    RecordTime --> ScheduleNext[next_fetch_ms = now + interval]
    ScheduleNext --> Done([Continue Main Loop])
    
    style Start fill:#90EE90
    style CheckPeriodicState fill:#FFD700
    style CheckTime fill:#FFD700
    style UpdateDM fill:#FF6B6B
```

## Error Handling Flow

```mermaid
flowchart TD
    Start([Error Detected]) --> CheckType{Error Type?}
    
    CheckType -->|I2C Timeout| I2CError[HAL_I2C_GetError]
    CheckType -->|CRC Mismatch| CRCError[Discard Data]
    CheckType -->|Buffer Overflow| BufferError[Report Overflow]
    CheckType -->|Invalid Command| CmdError[Print Unknown]
    CheckType -->|Invalid Parameter| ParamError[Print Usage]
    
    I2CError --> LogI2C[Log Error Code]
    CRCError --> LogCRC[Log CRC Failure]
    BufferError --> LogBuffer[Log Buffer State]
    CmdError --> LogCmd[Log Command String]
    ParamError --> LogParam[Log Parameter]
    
    LogI2C --> ReturnError([Return Error Status])
    LogCRC --> ReturnError
    LogBuffer --> ReturnError
    LogCmd --> ReturnError
    LogParam --> ReturnError
    
    ReturnError --> PreserveState{Preserve\nState?}
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
