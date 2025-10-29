# STM32 Data Logger - Flow Diagrams# STM32 Data Logger - Sơ Đồ Luồng Xử Lý# STM32 Data Logger - Sơ Đồ Luồng Xử Lý



This document describes detailed flow diagrams for processing within the STM32 firmware, including operational sequences and decision-making processes.



## Main Application FlowTài liệu này mô tả chi tiết các sơ đồ luồng xử lý trong firmware STM32, bao gồm các chuỗi hoạt động và quy trình ra quyết định.Tài liệu này mô tả chi tiết các sơ đồ luồng xử lý trong firmware STM32, bao gồm các chuỗi hoạt động và quy trình ra quyết định.



```mermaid

flowchart TD

    A[System Startup] --> B[UART Initialization]## Luồng Ứng Dụng Chính## Luồng Ứng Dụng Chính

    B --> C[SHT3X Sensor Initialization]

    C --> D[DS3231 RTC Initialization]

    D --> E[DataManager Initialization]

    E --> F[SD Card Manager Initialization]```mermaid```mermaid

    F --> G[ILI9225 Display Initialization]

    G --> H{Main Loop}flowchart TDflowchart TD

    

    H --> I[UART_Handle]    A[Khởi Động Hệ Thống] --> B[HAL_Init]    A[Khởi Động Hệ Thống] --> B[Khởi Tạo UART]

    I --> J{Periodic Mode?}

        B --> C[SystemClock_Config]    B --> C[Khởi Tạo Cảm Biến SHT3X]

    J -->|Yes| K{Time to Read?}

    J -->|No| L{MQTT Connected?}    C --> D[GPIO_Init]    C --> D[Khởi Tạo DS3231 RTC]

    K -->|Yes| M[SHT3X_FetchData]

    K -->|No| L    D --> E[I2C1_Init]    D --> E[Khởi Tạo DataManager]

    M --> N[DataManager_UpdatePeriodic]

    N --> L    E --> F[SPI1_Init - Thẻ SD]    E --> F[Khởi Tạo SD Card Manager]

    

    L -->|Yes| O[DataManager_Print Direct Data]    F --> G[SPI2_Init - Màn Hình]    F --> G[Khởi Tạo Màn Hình ILI9225]

    L -->|No| P[SDCardManager_WriteData]

        G --> H[USART1_Init]    G --> H{Vòng Lặp Chính}

    O --> Q{Has Buffered Data?}

    Q -->|Yes| R[SDCardManager_ReadData]    H --> I[UART_Init]    

    Q -->|No| S[Display_Update]

    R --> T[Transmit Buffered JSON]    I --> J[SHT3X_Init]    H --> I[UART_Handle]

    T --> U[SDCardManager_RemoveRecord]

    U --> S    J --> K[DS3231_Init]    I --> J{Chế Độ Định Kỳ?}

    

    P --> S    K --> L[DataManager_Init]    

    S --> H

        L --> M[SDCardManager_Init]    J -->|Có| K{Đến Lúc Đọc?}

    style A fill:#90EE90

    style H fill:#FFD700    M --> N{Thẻ SD Sẵn Sàng?}    J -->|Không| L{MQTT Kết Nối?}

    style L fill:#FFD700

    style P fill:#FF6B6B    N -->|Có| O[display_init]    K -->|Có| M[SHT3X_FetchData]

```

    N -->|Không| P[In Cảnh Báo SD]    K -->|Không| L

## Command Execution Flow

    P --> O    M --> N[DataManager_UpdatePeriodic]

```mermaid

flowchart TD    O --> Q[Vào Vòng Lặp Chính]    N --> L

    RxChar([UART RX Interrupt]) --> RingBuf[Store in Ring Buffer]

    RingBuf --> UARTHandle[UART_Handle Called]        

    UARTHandle --> CheckNewline{Newline\nDetected?}

        Q --> R[UART_Handle]    L -->|Có| O[DataManager_Print Dữ Liệu Trực Tiếp]

    CheckNewline -->|No| Return1([Return])

    CheckNewline -->|Yes| Tokenize[Tokenize Command String]    R --> S[Kiểm Tra Trạng Thái PERIODIC]    L -->|Không| P[SDCardManager_WriteData]

    Tokenize --> BuildCmd[Build Command from Tokens]

    BuildCmd --> FindCmd{Find in\nCommand Table?}    S --> T{SHT3X_IS_PERIODIC_STATE?}    

    

    FindCmd -->|Not Found| UnknownCmd[Print Unknown Command]    T -->|Không| U[DataManager_Print]    O --> Q{Có Dữ Liệu Buffer?}

    FindCmd -->|Found| CallParser[Call Parser Function]

    UnknownCmd --> Return2([Return])    T -->|Có| V{Đến Lúc Đọc Dữ Liệu?}    Q -->|Có| R[SDCardManager_ReadData]

    

    CallParser --> IsSINGLE{SINGLE\nCommand?}    V -->|Không| U    Q -->|Không| S[Display_Update]

    IsSINGLE -->|Yes| SingleParser[SINGLE_PARSER]

    IsSINGLE -->|No| IsPERIODIC{PERIODIC\nCommand?}    V -->|Có| W[SHT3X_FetchData]    R --> T[Truyền JSON Đã Buffer]

    

    IsPERIODIC -->|ON| PeriodicOnParser[PERIODIC_ON_PARSER]    W --> X[DataManager_UpdatePeriodic]    T --> U[SDCardManager_RemoveRecord]

    IsPERIODIC -->|OFF| PeriodicOffParser[PERIODIC_OFF_PARSER]

    IsPERIODIC -->|No| IsSETTIME{SET TIME\nCommand?}    X --> Y[Chuyển Đổi LED PC13]    U --> S

    

    IsSETTIME -->|Yes| SetTimeParser[SET_TIME_PARSER]    Y --> Z[Cập Nhật next_fetch_ms]    

    IsSETTIME -->|No| IsINTERVAL{SET PERIODIC\nINTERVAL?}

        Z --> U    P --> S

    IsINTERVAL -->|Yes| IntervalParser[SET_PERIODIC_INTERVAL_PARSER]

    IsINTERVAL -->|No| IsMQTT{MQTT\nNotification?}        S --> H

    

    IsMQTT -->|CONNECTED| MQTTConnParser[MQTT_CONNECTED_PARSER]    U --> AA{Dữ Liệu Đã In/Lưu?}    

    IsMQTT -->|DISCONNECTED| MQTTDiscParser[MQTT_DISCONNECTED_PARSER]

    IsMQTT -->|No| OtherCmd[Other Command Handler]    AA -->|Có| BB{Cần Cập Nhật Màn Hình?}    style A fill:#90EE90

    

    SingleParser --> CallDriver1[SHT3X_Single]    AA -->|Không| BB    style H fill:#FFD700

    CallDriver1 --> UpdateSingle[DataManager_UpdateSingle]

    UpdateSingle --> SetFlag[Set data_ready Flag]    BB -->|Có| CC[display_update]    style L fill:#FFD700

    

    PeriodicOnParser --> CallDriver2[SHT3X_Periodic]    BB -->|Không| DD[HAL_Delay 100ms]    style P fill:#FF6B6B

    CallDriver2 --> UpdatePeriodicMode[DataManager_UpdatePeriodic]

    UpdatePeriodicMode --> SetFlag    CC --> DD```

    

    PeriodicOffParser --> CallDriver3[SHT3X_PeriodicStop]    DD --> R

    CallDriver3 --> PrintResult[Print Status]

        ## Command Execution Flow

    SetTimeParser --> CallRTC[DS3231_Set_Time]

    CallRTC --> ForceDisplay[Set force_display_update]    style A fill:#90EE90

    ForceDisplay --> PrintResult

        style Q fill:#FFD700```mermaid

    IntervalParser --> SetInterval[Update periodic_interval_ms]

    SetInterval --> PrintResult    style R fill:#87CEEBflowchart TD

    

    MQTTConnParser --> UpdateMQTTState[mqtt_current_state = CONNECTED]    style U fill:#FF6B6B    RxChar([UART RX Interrupt]) --> RingBuf[Store in Ring Buffer]

    MQTTDiscParser --> UpdateMQTTState2[mqtt_current_state = DISCONNECTED]

    UpdateMQTTState --> PrintResult```    RingBuf --> UARTHandle[UART_Handle Called]

    UpdateMQTTState2 --> PrintResult

        UARTHandle --> CheckNewline{Newline\nDetected?}

    SetFlag --> Return3([Return])

    PrintResult --> Return3## Luồng Thực Thi Lệnh    

    OtherCmd --> Return3

        CheckNewline -->|No| Return1([Return])

    style RxChar fill:#90EE90

    style FindCmd fill:#FFD700```mermaid    CheckNewline -->|Yes| Tokenize[Tokenize Command String]

    style IsSINGLE fill:#FFD700

    style IsMQTT fill:#FFD700flowchart TD    Tokenize --> BuildCmd[Build Command from Tokens]

    style SetFlag fill:#FF6B6B

```    A[Ngắt UART] --> B[Lưu byte vào Ring Buffer]    BuildCmd --> FindCmd{Find in\nCommand Table?}



## SHT3X Single Measurement Flow    B --> C{Nhận được ký tự xuống dòng?}    



```mermaid    C -->|Không| D[Tiếp tục tích lũy]    FindCmd -->|Not Found| UnknownCmd[Print Unknown Command]

flowchart TD

    Start([SINGLE_PARSER]) --> ValidateArgs{argc == 1?}    C -->|Có| E[COMMAND_EXECUTE]    FindCmd -->|Found| CallParser[Call Parser Function]

    ValidateArgs -->|No| Return1([Return])

    ValidateArgs -->|Yes| SetRepeat[Set Repeatability = HIGH]    D --> A    UnknownCmd --> Return2([Return])

    

    SetRepeat --> I2CCmd[Send I2C Command]        

    I2CCmd --> Wait[HAL_Delay Based on Repeatability]

    Wait --> ReadI2C[I2C Read 6 Bytes]    E --> F[Phân tích chuỗi lệnh]    CallParser --> IsSINGLE{SINGLE\nCommand?}

    ReadI2C --> CheckI2C{I2C Success?}

        F --> G[Tìm kiếm trong bảng lệnh]    IsSINGLE -->|Yes| SingleParser[SINGLE_PARSER]

    CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]

    CheckI2C -->|Yes| CheckCRC{CRC Valid?}    G --> H{Tìm thấy lệnh?}    IsSINGLE -->|No| IsPERIODIC{PERIODIC\nCommand?}

    

    CheckCRC -->|No| SetZero    H -->|Không| I[In UNKNOWN COMMAND]    

    CheckCRC -->|Yes| ParseRaw[Parse Raw Temperature & Humidity]

        H -->|Có| J[Thực thi hàm parser]    IsPERIODIC -->|ON| PeriodicOnParser[PERIODIC_ON_PARSER]

    SetZero --> UpdateDM[DataManager_UpdateSingle]

    ParseRaw --> StoreValues[Store in sht3x_t Structure]        IsPERIODIC -->|OFF| PeriodicOffParser[PERIODIC_OFF_PARSER]

    StoreValues --> UpdateDM

    UpdateDM --> SetFlag[Set data_ready Flag]    J --> K{Loại lệnh?}    IsPERIODIC -->|No| IsSETTIME{SET TIME\nCommand?}

    SetFlag --> Return2([Return OK])

        K -->|SINGLE| L[SINGLE_PARSER]    

    style Start fill:#90EE90

    style CheckI2C fill:#FFD700    K -->|PERIODIC ON| M[PERIODIC_ON_PARSER]    IsSETTIME -->|Yes| SetTimeParser[SET_TIME_PARSER]

    style CheckCRC fill:#FFD700

    style UpdateDM fill:#FF6B6B    K -->|PERIODIC OFF| N[PERIODIC_OFF_PARSER]    IsSETTIME -->|No| IsINTERVAL{SET PERIODIC\nINTERVAL?}

```

    K -->|SET TIME| O[SET_TIME_PARSER]    

## SHT3X Periodic Measurement Flow

    K -->|SET PERIODIC INTERVAL| P[SET_PERIODIC_INTERVAL_PARSER]    IsINTERVAL -->|Yes| IntervalParser[SET_PERIODIC_INTERVAL_PARSER]

```mermaid

flowchart TD    K -->|MQTT CONNECTED| Q[MQTT_CONNECTED_PARSER]    IsINTERVAL -->|No| IsMQTT{MQTT\nNotification?}

    Start([PERIODIC_ON_PARSER]) --> ValidateArgs{argc == 2?}

    ValidateArgs -->|No| Return1([Return])    K -->|MQTT DISCONNECTED| R[MQTT_DISCONNECTED_PARSER]    

    ValidateArgs -->|Yes| SetDefaults[Set Rate=1MPS, Repeatability=HIGH]

        K -->|SD CLEAR| S[SD_CLEAR_PARSER]    IsMQTT -->|CONNECTED| MQTTConnParser[MQTT_CONNECTED_PARSER]

    SetDefaults --> BuildI2C[Build I2C Command Word]

    BuildI2C --> SendI2C[Send I2C Periodic Start]    K -->|Lệnh debug khác| T[Parser debug]    IsMQTT -->|DISCONNECTED| MQTTDiscParser[MQTT_DISCONNECTED_PARSER]

    SendI2C --> CheckI2C{I2C Success?}

            IsMQTT -->|No| OtherCmd[Other Command Handler]

    CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]

    CheckI2C -->|Yes| UpdateState[Update currentState]    L --> U[Gọi SHT3X_Single]    

    

    UpdateState --> InitTiming[Initialize next_fetch_ms]    M --> V[Gọi SHT3X_Periodic]    SingleParser --> CallDriver1[SHT3X_Single]

    InitTiming --> FirstFetch[SHT3X_FetchData]

    FirstFetch --> CheckFetch{Fetch Success?}    N --> W[Gọi SHT3X_PeriodicStop]    CallDriver1 --> UpdateSingle[DataManager_UpdateSingle]

    

    CheckFetch -->|No| SetZero    O --> X[Gọi DS3231_Set_Time]    UpdateSingle --> SetFlag[Set data_ready Flag]

    CheckFetch -->|Yes| UpdateDM[DataManager_UpdatePeriodic]

    SetZero --> UpdateDM    P --> Y[Cập nhật periodic_interval_ms]    

    UpdateDM --> SetFlag[Set data_ready Flag]

    SetFlag --> Return2([Return OK])    Q --> Z[Đặt MQTT_STATE_CONNECTED]    PeriodicOnParser --> CallDriver2[SHT3X_Periodic]

    

    style Start fill:#90EE90    R --> AA[Đặt MQTT_STATE_DISCONNECTED]    CallDriver2 --> UpdatePeriodicMode[DataManager_UpdatePeriodic]

    style CheckI2C fill:#FFD700

    style UpdateDM fill:#FF6B6B    S --> AB[SDCardManager_ClearBuffer]    UpdatePeriodicMode --> SetFlag

```

    T --> AC[Các chức năng debug khác]    

## Periodic Measurement Stop Flow

        PeriodicOffParser --> CallDriver3[SHT3X_PeriodicStop]

```mermaid

flowchart TD    U --> AD[DataManager_UpdateSingle]    CallDriver3 --> PrintResult[Print Status]

    Start([PERIODIC_OFF_PARSER]) --> ValidateArgs{argc == 2?}

    ValidateArgs -->|No| Return1([Return])    V --> AD    

    ValidateArgs -->|Yes| StopCmd[Send Stop Command 0x3093]

        W --> AE[In thông báo PERIODIC OFF]    SetTimeParser --> CallRTC[DS3231_Set_Time]

    StopCmd --> CheckI2C{I2C Success?}

    CheckI2C -->|No| PrintError[Print Error]    X --> AF[Đặt force_display_update = true]    CallRTC --> ForceDisplay[Set force_display_update]

    CheckI2C -->|Yes| ResetState[currentState = SHT3X_IDLE]

        Y --> AG[In thông báo cập nhật interval]    ForceDisplay --> PrintResult

    ResetState --> PrintStop[Print Stop Success]

    PrintStop --> Return2([Return])    Z --> AH[Gửi dữ liệu SD đã lưu]    

    PrintError --> Return2

        AA --> AI[In thông báo ngắt kết nối]    IntervalParser --> SetInterval[Update periodic_interval_ms]

    style Start fill:#90EE90

    style CheckI2C fill:#FFD700    AB --> AJ[In thông báo SD đã xóa]    SetInterval --> PrintResult

```

    AC --> AK[In thông tin debug]    

## Data Manager Print Decision Flow

        MQTTConnParser --> UpdateMQTTState[mqtt_current_state = CONNECTED]

```mermaid

flowchart TD    AD --> AL[Đặt data_ready = true]    MQTTDiscParser --> UpdateMQTTState2[mqtt_current_state = DISCONNECTED]

    Start([DataManager_Print Called]) --> CheckReady{data_ready\nFlag Set?}

    CheckReady -->|No| Return1([Return false])    AE --> AM[Lệnh hoàn thành]    UpdateMQTTState --> PrintResult

    CheckReady -->|Yes| CheckMode{Current Mode?}

        AF --> AM    UpdateMQTTState2 --> PrintResult

    CheckMode -->|SINGLE| GetTimeSingle[Get Unix Timestamp from DS3231]

    CheckMode -->|PERIODIC| GetTimePeriodic[Get Unix Timestamp from DS3231]    AG --> AM    

    CheckMode -->|UNKNOWN| Return2([Return false])

        AH --> AM    SetFlag --> Return3([Return])

    GetTimeSingle --> SanitizeSingle[Sanitize Float Values]

    GetTimePeriodic --> SanitizePeriodic[Sanitize Float Values]    AI --> AM    PrintResult --> Return3

    

    SanitizeSingle --> CheckMQTT{MQTT\nConnected?}    AJ --> AM    OtherCmd --> Return3

    SanitizePeriodic --> CheckMQTT

        AK --> AM    

    CheckMQTT -->|Yes| FormatJSON[Format JSON String]

    CheckMQTT -->|No| BufferToSD[SDCardManager_WriteData]    AL --> AM    style RxChar fill:#90EE90

    

    FormatJSON --> CheckOverflow{Buffer\nOverflow?}        style FindCmd fill:#FFD700

    BufferToSD --> ClearFlag[Clear data_ready]

        I --> AM    style IsSINGLE fill:#FFD700

    CheckOverflow -->|Yes| PrintOverflow[Print Overflow Error]

    CheckOverflow -->|No| TransmitUART[HAL_UART_Transmit JSON]    AM --> AN[Trở về vòng lặp chính]    style IsMQTT fill:#FFD700

    PrintOverflow --> ClearFlag

            style SetFlag fill:#FF6B6B

    TransmitUART --> ClearFlag

    ClearFlag --> Return3([Return true])    style A fill:#90EE90```

    

    style Start fill:#90EE90    style E fill:#FFD700

    style CheckReady fill:#FFD700

    style CheckMode fill:#FFD700    style K fill:#87CEEB## SHT3X Single Measurement Flow

    style CheckMQTT fill:#FFD700

    style TransmitUART fill:#FF6B6B    style AD fill:#FF6B6B

```

``````mermaid

## Periodic Data Fetch Decision Flow

flowchart TD

```mermaid

flowchart TD## Luồng Đo Đơn SHT3X    Start([SINGLE_PARSER]) --> ValidateArgs{argc == 1?}

    Start([Main Loop Iteration]) --> CheckPeriodicState{In Periodic\nState?}

    CheckPeriodicState -->|No| Skip([Skip Fetch])    ValidateArgs -->|No| Return1([Return])

    CheckPeriodicState -->|Yes| GetNow[now = HAL_GetTick]

    ```mermaid    ValidateArgs -->|Yes| SetRepeat[Set Repeatability = HIGH]

    GetNow --> CheckTime{now >=\nnext_fetch_ms?}

    CheckTime -->|No| Skipflowchart TD    

    CheckTime -->|Yes| CheckDuplicate{now ==\nlast_fetch_ms?}

        A[SINGLE_PARSER được gọi] --> B{Kiểm tra argc}    SetRepeat --> I2CCmd[Send I2C Command]

    CheckDuplicate -->|Yes| Skip

    CheckDuplicate -->|No| FetchData[SHT3X_FetchData]    B -->|Không hợp lệ| C[Dùng mặc định HIGH repeatability]    I2CCmd --> Wait[HAL_Delay Based on Repeatability]

    

    FetchData --> I2CRead[I2C Read Data from Sensor]    B -->|Hợp lệ| D[Phân tích tham số repeat]    Wait --> ReadI2C[I2C Read 6 Bytes]

    I2CRead --> CheckI2C{I2C Success?}

        C --> E[Gọi SHT3X_Single]    ReadI2C --> CheckI2C{I2C Success?}

    CheckI2C -->|No| Skip

    CheckI2C -->|Yes| ParseData[Parse Temperature & Humidity]    D --> E    

    

    ParseData --> UpdateDM[DataManager_UpdatePeriodic]        CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]

    UpdateDM --> ToggleGPIO[Toggle PC13 LED]

    ToggleGPIO --> RecordTime[last_fetch_ms = now]    E --> F[Xây dựng lệnh I2C]    CheckI2C -->|Yes| CheckCRC{CRC Valid?}

    RecordTime --> ScheduleNext[next_fetch_ms = now + interval]

    ScheduleNext --> Done([Continue Main Loop])    F --> G{Mức repeat?}    

    

    style Start fill:#90EE90    G -->|HIGH| H[Dùng 0x2400]    CheckCRC -->|No| SetZero

    style CheckPeriodicState fill:#FFD700

    style CheckTime fill:#FFD700    G -->|MEDIUM| I[Dùng 0x240B]    CheckCRC -->|Yes| ParseRaw[Parse Raw Temperature & Humidity]

    style UpdateDM fill:#FF6B6B

```    G -->|LOW| J[Dùng 0x2416]    



## MQTT-Aware Data Routing Flow        SetZero --> UpdateDM[DataManager_UpdateSingle]



```mermaid    H --> K[HAL_I2C_Master_Transmit]    ParseRaw --> StoreValues[Store in sht3x_t Structure]

flowchart TD

    Start([Main Loop - After Periodic Fetch]) --> CheckMQTT{MQTT\nConnected?}    I --> K    StoreValues --> UpdateDM

    

    CheckMQTT -->|Yes| PrintLive[DataManager_Print - Send Live Data]    J --> K    UpdateDM --> SetFlag[Set data_ready Flag]

    CheckMQTT -->|No| BufferData[SDCardManager_WriteData - Buffer to SD]

            SetFlag --> Return2([Return OK])

    PrintLive --> CheckBuffered{Has Buffered\nSD Data?}

    BufferData --> UpdateDisplay[Display_Update]    K --> L{Truyền I2C OK?}    

    

    CheckBuffered -->|Yes| CheckDelay{100ms Delay\nPassed?}    L -->|Không| M[Trả về SHT3X_ERROR]    style Start fill:#90EE90

    CheckBuffered -->|No| UpdateDisplay

        L -->|Có| N[HAL_Delay 15ms]    style CheckI2C fill:#FFD700

    CheckDelay -->|Yes| ReadSD[SDCardManager_ReadData]

    CheckDelay -->|No| UpdateDisplay        style CheckCRC fill:#FFD700

    

    ReadSD --> FormatJSON[Format Buffered JSON]    N --> O[HAL_I2C_Master_Receive 6 bytes]    style UpdateDM fill:#FF6B6B

    FormatJSON --> TransmitSD[Transmit via UART]

    TransmitSD --> RemoveSD[SDCardManager_RemoveRecord]    O --> P{Nhận I2C OK?}```

    RemoveSD --> UpdateDisplay

    UpdateDisplay --> Done([Continue Main Loop])    P -->|Không| M

    

    style Start fill:#90EE90    P -->|Có| Q[Kiểm tra CRC nhiệt độ]## SHT3X Periodic Measurement Flow

    style CheckMQTT fill:#FFD700

    style CheckBuffered fill:#FFD700    

    style BufferData fill:#FF6B6B

```    Q --> R{CRC nhiệt độ OK?}```mermaid



## Error Handling Flow    R -->|Không| Mflowchart TD



```mermaid    R -->|Có| S[Kiểm tra CRC độ ẩm]    Start([PERIODIC_ON_PARSER]) --> ValidateArgs{argc == 2?}

flowchart TD

    Start([Error Detected]) --> CheckType{Error Type?}        ValidateArgs -->|No| Return1([Return])

    

    CheckType -->|I2C Timeout| I2CError[HAL_I2C_GetError]    S --> T{CRC độ ẩm OK?}    ValidateArgs -->|Yes| SetDefaults[Set Rate=1MPS, Repeatability=HIGH]

    CheckType -->|CRC Mismatch| CRCError[Discard Data]

    CheckType -->|SD Card Error| SDError[Continue Without Buffering]    T -->|Không| M    

    CheckType -->|Buffer Overflow| BufferError[Report Overflow]

    CheckType -->|Invalid Command| CmdError[Print Unknown]    T -->|Có| U[Phân tích dữ liệu nhiệt độ]    SetDefaults --> BuildI2C[Build I2C Command Word]

    CheckType -->|Invalid Parameter| ParamError[Print Usage]

            BuildI2C --> SendI2C[Send I2C Periodic Start]

    I2CError --> LogI2C[Log Error Code]

    CRCError --> SetZeroData[Set temp=0.0, hum=0.0]    U --> V[Phân tích dữ liệu độ ẩm]    SendI2C --> CheckI2C{I2C Success?}

    SDError --> LogSD[Warn SD Not Available]

    BufferError --> LogBuffer[Log Buffer State]    V --> W[Lưu vào struct sht3x]    

    CmdError --> LogCmd[Log Command String]

    ParamError --> LogParam[Log Parameter]    W --> X[Trả về SHT3X_OK]    CheckI2C -->|No| SetZero[Set temp=0.0, hum=0.0]

    

    LogI2C --> ReturnError([Return Error Status])        CheckI2C -->|Yes| UpdateState[Update currentState]

    SetZeroData --> UpdateWithZero[DataManager_Update with 0.0]

    LogSD --> ContinueOp[Continue Without SD]    M --> Y[Đặt temp = 0.0, hum = 0.0]    

    LogBuffer --> ReturnError

    LogCmd --> ReturnError    X --> Z[DataManager_UpdateSingle]    UpdateState --> InitTiming[Initialize next_fetch_ms]

    LogParam --> ReturnError

        Y --> Z    InitTiming --> FirstFetch[SHT3X_FetchData]

    UpdateWithZero --> PreserveState{Preserve\nState?}

    ContinueOp --> PreserveState        FirstFetch --> CheckFetch{Fetch Success?}

    ReturnError --> PreserveState

        Z --> AA[Đặt mode = SINGLE]    

    PreserveState -->|Yes| KeepState[Maintain Current Mode]

    PreserveState -->|No| ResetState[Reset to IDLE]    AA --> AB[Lưu nhiệt độ, độ ẩm]    CheckFetch -->|No| SetZero

    

    KeepState --> End([Continue Operation])    AB --> AC[Đặt data_ready = true]    CheckFetch -->|Yes| UpdateDM[DataManager_UpdatePeriodic]

    ResetState --> End

        AC --> AD[Trở về parser]    SetZero --> UpdateDM

    style Start fill:#FF6B6B

    style CheckType fill:#FFD700        UpdateDM --> SetFlag[Set data_ready Flag]

    style ReturnError fill:#FF6B6B

```    style A fill:#90EE90    SetFlag --> Return2([Return OK])



## Legend    style E fill:#FFD700    



```mermaid    style K fill:#87CEEB    style Start fill:#90EE90

flowchart LR

    Start([Start/End - Rounded])     style Z fill:#FF6B6B    style CheckI2C fill:#FFD700

    Process[Process - Rectangle]

    Decision{Decision - Diamond}```    style UpdateDM fill:#FF6B6B

    Flag[Critical State Change - Rectangle]

    ```

    style Start fill:#90EE90

    style Decision fill:#FFD700## Luồng Đo Định Kỳ SHT3X

    style Flag fill:#FF6B6B

```## Periodic Measurement Stop Flow



---```mermaid



**Notes:**flowchart TD```mermaid

- Green nodes: Entry/exit points

- Yellow nodes: Decision points    A[PERIODIC_ON_PARSER được gọi] --> B{Kiểm tra argc}flowchart TD

- Red nodes: State changes or critical operations

- Blue nodes: Wait states or returns    B -->|argc ≥ 2 hợp lệ| C[Phân tích tham số mode]    Start([PERIODIC_OFF_PARSER]) --> ValidateArgs{argc == 2?}

- All flows are non-blocking except I2C operations and delays
    B -->|Không hợp lệ| D[Dùng mặc định 1MPS HIGH]    ValidateArgs -->|No| Return1([Return])

        ValidateArgs -->|Yes| StopCmd[Send Stop Command 0x3093]

    C --> E{Tham số mode?}    

    E -->|0.5MPS| F[Đặt SHT3X_PERIODIC_05MPS]    StopCmd --> CheckI2C{I2C Success?}

    E -->|1MPS| G[Đặt SHT3X_PERIODIC_1MPS]    CheckI2C -->|No| PrintError[Print Error]

    E -->|2MPS| H[Đặt SHT3X_PERIODIC_2MPS]    CheckI2C -->|Yes| ResetState[currentState = SHT3X_IDLE]

    E -->|4MPS| I[Đặt SHT3X_PERIODIC_4MPS]    

    E -->|10MPS| J[Đặt SHT3X_PERIODIC_10MPS]    ResetState --> PrintStop[Print Stop Success]

    E -->|Không hợp lệ| D    PrintStop --> Return2([Return])

        PrintError --> Return2

    D --> G    

    F --> K{argc ≥ 3?}    style Start fill:#90EE90

    G --> K    style CheckI2C fill:#FFD700

    H --> K```

    I --> K

    J --> K## Data Manager Print Decision Flow

    

    K -->|Có| L[Phân tích tham số repeat]```mermaid

    K -->|Không| M[Dùng mặc định HIGH repeatability]flowchart TD

        Start([DataManager_Print Called]) --> CheckReady{data_ready\nFlag Set?}

    L --> N{Tham số repeat?}    CheckReady -->|No| Return1([Return false])

    N -->|HIGH| O[Đặt SHT3X_HIGH]    CheckReady -->|Yes| CheckMode{Current Mode?}

    N -->|MEDIUM| P[Đặt SHT3X_MEDIUM]    

    N -->|LOW| Q[Đặt SHT3X_LOW]    CheckMode -->|SINGLE| GetTimeSingle[Get Unix Timestamp from DS3231]

    N -->|Không hợp lệ| M    CheckMode -->|PERIODIC| GetTimePeriodic[Get Unix Timestamp from DS3231]

        CheckMode -->|UNKNOWN| Return2([Return false])

    M --> O    

    O --> R[Gọi SHT3X_Periodic]    GetTimeSingle --> SanitizeSingle[Sanitize Float Values]

    P --> R    GetTimePeriodic --> SanitizePeriodic[Sanitize Float Values]

    Q --> R    

        SanitizeSingle --> CheckMQTT{MQTT\nConnected?}

    R --> S[Xây dựng lệnh định kỳ]    SanitizePeriodic --> CheckMQTT

    S --> T[Ánh xạ mode + repeat thành lệnh I2C]    

    T --> U[HAL_I2C_Master_Transmit]    CheckMQTT -->|Yes| FormatJSON[Format JSON String]

        CheckMQTT -->|No| BufferToSD[SDCardManager_WriteData]

    U --> V{Truyền I2C OK?}    

    V -->|Không| W[Trả về SHT3X_ERROR]    FormatJSON --> CheckOverflow{Buffer\nOverflow?}

    V -->|Có| X[Cập nhật currentState]    BufferToSD --> ClearFlag[Clear data_ready]

        

    X --> Y[Cập nhật modeRepeat]    CheckOverflow -->|Yes| PrintOverflow[Print Overflow Error]

    Y --> Z[HAL_Delay 100ms]    CheckOverflow -->|No| TransmitUART[HAL_UART_Transmit JSON]

    Z --> AA[SHT3X_FetchData - đo lần đầu]    PrintOverflow --> ClearFlag

        

    AA --> BB{Đo lần đầu OK?}    TransmitUART --> ClearFlag

    BB -->|Không| W    ClearFlag --> Return3([Return true])

    BB -->|Có| CC[Trả về SHT3X_OK]    

        style Start fill:#90EE90

    W --> DD[DataManager_UpdatePeriodic với giá trị 0.0]    style CheckReady fill:#FFD700

    CC --> EE[DataManager_UpdatePeriodic với giá trị thực]    style CheckMode fill:#FFD700

        style CheckMQTT fill:#FFD700

    DD --> FF[Đặt mode = PERIODIC]    style TransmitUART fill:#FF6B6B

    EE --> FF```

    FF --> GG[Đặt data_ready = true]

    GG --> HH[Khởi tạo next_fetch_ms]## Periodic Data Fetch Decision Flow

    HH --> II[Trở về parser]

    ```mermaid

    style A fill:#90EE90flowchart TD

    style R fill:#FFD700    Start([Main Loop Iteration]) --> CheckPeriodicState{In Periodic\nState?}

    style U fill:#87CEEB    CheckPeriodicState -->|No| Skip([Skip Fetch])

    style EE fill:#FF6B6B    CheckPeriodicState -->|Yes| GetNow[now = HAL_GetTick]

```    

    GetNow --> CheckTime{now >=\nnext_fetch_ms?}

## Luồng In Dữ Liệu Data Manager    CheckTime -->|No| Skip

    CheckTime -->|Yes| CheckDuplicate{now ==\nlast_fetch_ms?}

```mermaid    

flowchart TD    CheckDuplicate -->|Yes| Skip

    A[DataManager_Print được gọi] --> B{Cờ data_ready?}    CheckDuplicate -->|No| FetchData[SHT3X_FetchData]

    B -->|false| C[Trả về false - không có gì để in]    

    B -->|true| D[DS3231_Get_Time]    FetchData --> I2CRead[I2C Read Data from Sensor]

        I2CRead --> CheckI2C{I2C Success?}

    D --> E{Đọc RTC OK?}    

    E -->|Không| F[Dùng thời gian mặc định]    CheckI2C -->|No| Skip

    E -->|Có| G[Chuyển tm thành Unix timestamp]    CheckI2C -->|Yes| ParseData[Parse Temperature & Humidity]

        

    F --> H[Đọc dữ liệu cảm biến từ trạng thái]    ParseData --> UpdateDM[DataManager_UpdatePeriodic]

    G --> H    UpdateDM --> ToggleGPIO[Toggle PC13 LED]

        ToggleGPIO --> RecordTime[last_fetch_ms = now]

    H --> I[Làm sạch dữ liệu nhiệt độ]    RecordTime --> ScheduleNext[next_fetch_ms = now + interval]

    I --> J[Làm sạch dữ liệu độ ẩm]    ScheduleNext --> Done([Continue Main Loop])

    J --> K[Kiểm tra mqtt_current_state]    

        style Start fill:#90EE90

    K --> L{Trạng thái MQTT?}    style CheckPeriodicState fill:#FFD700

    L -->|MQTT_STATE_CONNECTED| M[Định dạng JSON xuất ra]    style CheckTime fill:#FFD700

    L -->|MQTT_STATE_DISCONNECTED| N[SDCardManager_WriteData]    style UpdateDM fill:#FF6B6B

    ```

    M --> O[Gọi sensor_json_format]

    O --> P{Buffer JSON OK?}## MQTT-Aware Data Routing Flow

    P -->|Không| Q[In BUFFER OVERFLOW]

    P -->|Có| R[HAL_UART_Transmit JSON]```mermaid

    flowchart TD

    N --> S{Ghi SD OK?}    Start([Main Loop - After Periodic Fetch]) --> CheckMQTT{MQTT\nConnected?}

    S -->|Không| T[In lỗi ghi SD]    

    S -->|Có| U[Dữ liệu đã lưu vào SD]    CheckMQTT -->|Yes| PrintLive[DataManager_Print - Send Live Data]

        CheckMQTT -->|No| BufferData[SDCardManager_WriteData - Buffer to SD]

    Q --> V[Đặt data_ready = false]    

    R --> V    PrintLive --> CheckBuffered{Has Buffered\nSD Data?}

    T --> V    BufferData --> UpdateDisplay[Display_Update]

    U --> V    

        CheckBuffered -->|Yes| CheckDelay{100ms Delay\nPassed?}

    V --> W[Trả về true - dữ liệu đã xử lý]    CheckBuffered -->|No| UpdateDisplay

    C --> X[Trở về caller]    

    W --> X    CheckDelay -->|Yes| ReadSD[SDCardManager_ReadData]

        CheckDelay -->|No| UpdateDisplay

    style A fill:#90EE90    

    style K fill:#FFD700    ReadSD --> FormatJSON[Format Buffered JSON]

    style M fill:#87CEEB    FormatJSON --> TransmitSD[Transmit via UART]

    style N fill:#FF6B6B    TransmitSD --> RemoveSD[SDCardManager_RemoveRecord]

```    RemoveSD --> UpdateDisplay

    UpdateDisplay --> Done([Continue Main Loop])

## Luồng Định Tuyến Dữ Liệu Nhận Biết MQTT    

    style Start fill:#90EE90

```mermaid    style CheckMQTT fill:#FFD700

flowchart TD    style CheckBuffered fill:#FFD700

    A[Dữ liệu cảm biến có sẵn] --> B[DataManager được gọi]    style BufferData fill:#FF6B6B

    B --> C[Kiểm tra mqtt_current_state]```

    

    C --> D{Trạng thái MQTT?}## Error Handling Flow

    D -->|MQTT_STATE_CONNECTED| E[Định dạng JSON ngay lập tức]

    D -->|MQTT_STATE_DISCONNECTED| F[Lưu buffer vào thẻ SD]```mermaid

    flowchart TD

    E --> G[HAL_UART_Transmit đến ESP32]    Start([Error Detected]) --> CheckType{Error Type?}

    F --> H[SDCardManager_WriteData]    

        CheckType -->|I2C Timeout| I2CError[HAL_I2C_GetError]

    G --> I[Dữ liệu gửi đến MQTT broker]    CheckType -->|CRC Mismatch| CRCError[Discard Data]

    H --> J[Dữ liệu lưu trong buffer vòng tròn]    CheckType -->|SD Card Error| SDError[Continue Without Buffering]

        CheckType -->|Buffer Overflow| BufferError[Report Overflow]

    I --> K[Hoạt động bình thường tiếp tục]    CheckType -->|Invalid Command| CmdError[Print Unknown]

    J --> K    CheckType -->|Invalid Parameter| ParamError[Print Usage]

        

    %% Kịch bản kết nối lại MQTT    I2CError --> LogI2C[Log Error Code]

    K --> L{MQTT kết nối lại?}    CRCError --> SetZeroData[Set temp=0.0, hum=0.0]

    L -->|Có| M[Nhận lệnh MQTT CONNECTED]    SDError --> LogSD[Warn SD Not Available]

    L -->|Không| N[Tiếp tục lưu buffer]    BufferError --> LogBuffer[Log Buffer State]

        CmdError --> LogCmd[Log Command String]

    M --> O[Đặt mqtt_current_state = CONNECTED]    ParamError --> LogParam[Log Parameter]

    O --> P[Vòng lặp chính: Gửi dữ liệu đã lưu]    

        LogI2C --> ReturnError([Return Error Status])

    P --> Q{Tồn tại dữ liệu đã lưu?}    SetZeroData --> UpdateWithZero[DataManager_Update with 0.0]

    Q -->|Không| R[Tiếp tục truyền trực tiếp bình thường]    LogSD --> ContinueOp[Continue Without SD]

    Q -->|Có| S[SDCardManager_ReadData]    LogBuffer --> ReturnError

        LogCmd --> ReturnError

    S --> T[Định dạng bản ghi đã lưu thành JSON]    LogParam --> ReturnError

    T --> U[HAL_UART_Transmit dữ liệu đã lưu]    

    U --> V[SDCardManager_RemoveRecord]    UpdateWithZero --> PreserveState{Preserve\nState?}

    V --> W[HAL_Delay 100ms giữa các bản ghi]    ContinueOp --> PreserveState

    W --> Q    ReturnError --> PreserveState

        

    N --> X[Tiếp tục lưu buffer SD]    PreserveState -->|Yes| KeepState[Maintain Current Mode]

    R --> Y[Chế độ truyền dữ liệu trực tiếp]    PreserveState -->|No| ResetState[Reset to IDLE]

    X --> K    

    Y --> K    KeepState --> End([Continue Operation])

        ResetState --> End

    style A fill:#90EE90    

    style D fill:#FFD700    style Start fill:#FF6B6B

    style E fill:#87CEEB    style CheckType fill:#FFD700

    style F fill:#FF6B6B    style ReturnError fill:#FF6B6B

    style P fill:#DDA0DD```

```

## Legend

## Luồng Xử Lý Lỗi

```mermaid

```mermaidflowchart LR

flowchart TD    Start([Start/End - Rounded]) 

    A[Bắt đầu thao tác] --> B{Loại thao tác?}    Process[Process - Rectangle]

    B -->|Giao tiếp I2C| C[Gọi HAL_I2C_*]    Decision{Decision - Diamond}

    B -->|Giao tiếp SPI| D[Gọi HAL_SPI_*]    Flag[Critical State Change - Rectangle]

    B -->|Giao tiếp UART| E[Gọi HAL_UART_*]    

    B -->|Thao tác thẻ SD| F[Gọi hàm thẻ SD]    style Start fill:#90EE90

        style Decision fill:#FFD700

    C --> G{Trạng thái HAL?}    style Flag fill:#FF6B6B

    D --> H{Trạng thái HAL?}```

    E --> I{Trạng thái HAL?}

    F --> J{Trạng thái thao tác SD?}---

    

    G -->|HAL_OK| K[Kiểm tra tính toàn vẹn dữ liệu]**Notes:**

    G -->|HAL_ERROR| L[Ghi lỗi I2C]- Green nodes: Entry/exit points

    G -->|HAL_TIMEOUT| M[Ghi timeout I2C]- Yellow nodes: Decision points

    - Red nodes: State changes or critical operations

    H -->|HAL_OK| N[Thao tác SPI thành công]- Blue nodes: Wait states or returns

    H -->|HAL_ERROR| O[Ghi lỗi SPI]- All flows are non-blocking except I2C operations and delays

    H -->|HAL_TIMEOUT| P[Ghi timeout SPI]
    
    I -->|HAL_OK| Q[Thao tác UART thành công]
    I -->|HAL_ERROR| R[Ghi lỗi UART]
    I -->|HAL_TIMEOUT| S[Ghi timeout UART]
    
    J -->|Thành công| T[Thao tác SD thành công]
    J -->|Lỗi| U[Ghi lỗi SD]
    
    K --> V{Dữ liệu hợp lệ?}
    V -->|Có| W[Trả về thành công với dữ liệu]
    V -->|Không| X[Trả về lỗi - CRC/validation thất bại]
    
    L --> Y[Trả về I2C_ERROR]
    M --> Y
    O --> Z[Trả về SPI_ERROR]
    P --> Z
    R --> AA[Trả về UART_ERROR]
    S --> AA
    U --> AB[Trả về SD_ERROR]
    
    N --> AC[Tiếp tục thao tác]
    Q --> AC
    T --> AC
    W --> AC
    
    Y --> AD[Đặt giá trị cảm biến về 0.0]
    Z --> AE[Đặt trạng thái hiển thị mặc định]
    AA --> AF[Dùng dữ liệu trước đó hoặc mặc định]
    AB --> AG[Tiếp tục không có buffer SD]
    X --> AD
    
    AD --> AH[Cập nhật DataManager với trạng thái lỗi]
    AE --> AI[Tiếp tục với chức năng hạn chế]
    AF --> AJ[In thông báo cảnh báo]
    AG --> AK[In cảnh báo SD không khả dụng]
    
    AH --> AL[Trạng thái lỗi đã được thông báo]
    AI --> AL
    AJ --> AL
    AK --> AL
    AC --> AM[Hoạt động bình thường tiếp tục]
    AL --> AN[Hệ thống tiếp tục với chức năng giảm]
    AM --> AO[Trở về caller]
    AN --> AO
    
    style A fill:#90EE90
    style B fill:#FFD700
    style G fill:#87CEEB
    style V fill:#FF6B6B
    style AD fill:#FFA500
```

---

**Điểm Chính:**
- Tất cả luồng bắt đầu từ khởi tạo khi cấp nguồn hoặc nhận lệnh
- Đo định kỳ hoạt động độc lập trong vòng lặp chính
- Trạng thái MQTT quyết định định tuyến dữ liệu (truyền trực tiếp vs lưu buffer SD)
- Xử lý lỗi cung cấp sự suy giảm nhẹ nhàng
- Thẻ SD hoạt động như buffer offline khi MQTT không khả dụng
- Cập nhật hiển thị được kích hoạt bởi thay đổi dữ liệu hoặc lệnh rõ ràng