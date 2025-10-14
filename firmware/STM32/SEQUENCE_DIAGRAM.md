# STM32 Data Logger - Sequence Diagrams

This document illustrates the time-ordered interactions between components in the STM32 firmware.

## Single Shot Measurement Sequence

```mermaid
sequenceDiagram
    participant User
    participant UART as UART Interface
    participant RingBuf as Ring Buffer
    participant CmdExec as Command Execute
    participant Parser as Command Parser
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant DM as DataManager
    participant RTC as DS3231 RTC
    
    User->>UART: "SHT3X SINGLE HIGH\n"
    activate UART
    UART->>RingBuf: Store bytes (interrupt)
    deactivate UART
    
    Note over RingBuf: Accumulate until '\n'
    
    RingBuf->>CmdExec: UART_Handle() detects complete line
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["SHT3X", "SINGLE", "HIGH"]
    CmdExec->>CmdExec: Build command string: "SHT3X SINGLE HIGH"
    CmdExec->>CmdExec: Find in cmdTable[]
    CmdExec->>Parser: SHT3X_Single_Parser(argc=3, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>Parser: Validate argc >= 3
    Parser->>Parser: Parse argv[2]="HIGH" → SHT3X_HIGH
    Parser->>Driver: SHT3X_Single(&g_sht3x, SHT3X_HIGH, &temp, &hum)
    deactivate Parser
    
    activate Driver
    Driver->>Driver: Select I2C command: 0x2400
    Driver->>I2C: HAL_I2C_Master_Transmit(0x2400)
    activate I2C
    I2C->>Sensor: I2C Write [0x24, 0x00]
    activate Sensor
    deactivate I2C
    
    Driver->>Driver: HAL_Delay(15ms)
    Note over Sensor: Measurement in progress
    
    Driver->>I2C: HAL_I2C_Master_Receive(6 bytes)
    activate I2C
    Sensor->>I2C: Send [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]
    deactivate Sensor
    I2C->>Driver: Raw data buffer
    deactivate I2C
    
    Driver->>Driver: Validate CRC for temperature
    Driver->>Driver: Validate CRC for humidity
    Driver->>Driver: Convert raw → 23.45°C, 65.20%
    Driver->>Driver: Store in g_sht3x.temperature, g_sht3x.humidity
    Driver->>Parser: Return SHT3X_OK, temp=23.45, hum=65.20
    deactivate Driver
    
    activate Parser
    Parser->>DM: DataManager_UpdateSingle(23.45, 65.20)
    deactivate Parser
    
    activate DM
    DM->>DM: Store mode = DATALOGGER_MODE_SINGLE
    DM->>DM: Store temperature = 23.45
    DM->>DM: Store humidity = 65.20
    DM->>DM: Set data_ready = true
    deactivate DM
    
    Note over DM: Wait for next main loop iteration
    
    DM->>DM: DataManager_Print() called from main loop
    activate DM
    DM->>DM: Check data_ready == true
    DM->>RTC: DS3231_Get_Time(&time)
    activate RTC
    RTC->>DM: Return time structure
    deactivate RTC
    DM->>DM: Convert to Unix timestamp: 1728930680
    DM->>DM: Sanitize floats (check NaN/Inf)
    DM->>DM: Format JSON: {"mode":"SINGLE","timestamp":1728930680,...}
    DM->>UART: HAL_UART_Transmit(JSON string)
    activate UART
    UART->>User: {"mode":"SINGLE","timestamp":1728930680,"temperature":23.45,"humidity":65.20}
    deactivate UART
    DM->>DM: Clear data_ready = false
    deactivate DM
```

## Periodic Measurement Setup Sequence

```mermaid
sequenceDiagram
    participant User
    participant UART as UART Interface
    participant CmdExec as Command Execute
    participant Parser as Command Parser
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant DM as DataManager
    participant MainLoop as Main Loop
    
    User->>UART: "SHT3X PERIODIC 1 HIGH\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["SHT3X", "PERIODIC", "1", "HIGH"]
    CmdExec->>Parser: SHT3X_Periodic_Parser(argc=4, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>Parser: Parse argv[2]="1" → SHT3X_PERIODIC_1MPS
    Parser->>Parser: Parse argv[3]="HIGH" → SHT3X_HIGH
    Parser->>Driver: SHT3X_Periodic(&g_sht3x, SHT3X_PERIODIC_1MPS, SHT3X_HIGH)
    deactivate Parser
    
    activate Driver
    Driver->>Driver: Build I2C command: 0x2032
    Driver->>I2C: HAL_I2C_Master_Transmit(0x2032)
    activate I2C
    I2C->>Sensor: I2C Write [0x20, 0x32]
    activate Sensor
    Note over Sensor: Enter periodic mode @ 1Hz
    deactivate Sensor
    I2C->>Driver: ACK received
    deactivate I2C
    
    Driver->>Driver: Update currentState = SHT3X_PERIODIC_1MPS
    Driver->>Driver: Update modeRepeat = SHT3X_HIGH
    Driver->>Parser: Return SHT3X_OK
    deactivate Driver
    
    activate Parser
    Parser->>Parser: Initialize next_fetch_ms = HAL_GetTick()
    Parser->>Driver: SHT3X_FetchData(&g_sht3x, &temp, &hum)
    activate Driver
    
    Driver->>I2C: HAL_I2C_Master_Receive(6 bytes)
    activate I2C
    Sensor->>I2C: Send first measurement
    I2C->>Driver: Raw data
    deactivate I2C
    
    Driver->>Driver: Parse & validate data
    Driver->>Parser: Return temp=23.45, hum=65.20
    deactivate Driver
    
    Parser->>DM: DataManager_UpdatePeriodic(23.45, 65.20)
    activate DM
    DM->>DM: Store mode = DATALOGGER_MODE_PERIODIC
    DM->>DM: Store temperature, humidity
    DM->>DM: Set data_ready = true
    DM->>Parser: Return
    deactivate DM
    deactivate Parser
    
    Note over MainLoop: Main loop continues...
    
    loop Every 5 seconds (periodic_interval_ms)
        MainLoop->>MainLoop: Check SHT3X_IS_PERIODIC_STATE()
        MainLoop->>MainLoop: Check (now >= next_fetch_ms)
        MainLoop->>Driver: SHT3X_FetchData(&g_sht3x, &temp, &hum)
        activate Driver
        Driver->>I2C: HAL_I2C_Master_Receive(6 bytes)
        activate I2C
        Sensor->>I2C: Send periodic measurement
        I2C->>Driver: Raw data
        deactivate I2C
        Driver->>Driver: Parse data
        Driver->>MainLoop: Return temp, hum
        deactivate Driver
        
        MainLoop->>DM: DataManager_UpdatePeriodic(temp, hum)
        activate DM
        DM->>DM: Set data_ready = true
        deactivate DM
        
        MainLoop->>DM: DataManager_Print()
        activate DM
        DM->>UART: Transmit JSON
        activate UART
        UART->>User: {"mode":"PERIODIC","timestamp":...,"temperature":...}
        deactivate UART
        DM->>DM: Clear data_ready = false
        deactivate DM
        
        MainLoop->>MainLoop: Update next_fetch_ms += 5000
    end
```

## Periodic Measurement Stop Sequence

```mermaid
sequenceDiagram
    participant User
    participant UART as UART Interface
    participant CmdExec as Command Execute
    participant Parser as Command Parser
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant MainLoop as Main Loop
    
    Note over Sensor: Currently in periodic mode @ 1Hz
    
    User->>UART: "SHT3X PERIODIC STOP\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["SHT3X", "PERIODIC", "STOP"]
    CmdExec->>Parser: SHT3X_Periodic_Parser(argc=3, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>Parser: Check argv[2] == "STOP"
    Parser->>Driver: SHT3X_PeriodicStop(&g_sht3x)
    deactivate Parser
    
    activate Driver
    Driver->>Driver: Build stop command: 0x3093
    Driver->>I2C: HAL_I2C_Master_Transmit(0x3093)
    activate I2C
    I2C->>Sensor: I2C Write [0x30, 0x93]
    activate Sensor
    Note over Sensor: Exit periodic mode → IDLE
    deactivate Sensor
    I2C->>Driver: ACK received
    deactivate I2C
    
    Driver->>Driver: Update currentState = SHT3X_IDLE
    Driver->>Parser: Return SHT3X_OK
    deactivate Driver
    
    activate Parser
    Parser->>UART: Print "STOP PERIODIC SUCCEEDED"
    activate UART
    UART->>User: "STOP PERIODIC SUCCEEDED\r\n"
    deactivate UART
    deactivate Parser
    
    Note over MainLoop: Next main loop iteration
    MainLoop->>MainLoop: Check SHT3X_IS_PERIODIC_STATE()
    Note over MainLoop: Returns false, skip fetch
```

## UART Interrupt to Command Dispatch Sequence

```mermaid
sequenceDiagram
    participant HW as UART Hardware
    participant ISR as UART ISR
    participant RingBuf as Ring Buffer
    participant MainLoop as Main Loop
    participant CmdExec as Command Execute
    participant CmdTable as Command Table
    participant Parser as Parser Function
    
    HW->>ISR: Byte received interrupt
    activate ISR
    ISR->>RingBuf: RingBuffer_Write(byte)
    activate RingBuf
    RingBuf->>RingBuf: buf[head] = byte
    RingBuf->>RingBuf: head = (head + 1) % size
    RingBuf->>ISR: Return
    deactivate RingBuf
    ISR->>HW: Re-enable interrupt
    deactivate ISR
    
    Note over MainLoop: Main loop iteration
    MainLoop->>MainLoop: UART_Handle() called
    activate MainLoop
    
    loop Until no complete line
        MainLoop->>RingBuf: RingBuffer_Available()
        activate RingBuf
        RingBuf->>MainLoop: bytes available
        deactivate RingBuf
        
        MainLoop->>RingBuf: RingBuffer_Peek()
        activate RingBuf
        RingBuf->>MainLoop: byte value
        deactivate RingBuf
        
        alt Is newline character
            MainLoop->>MainLoop: Line complete, null terminate
            MainLoop->>CmdExec: COMMAND_EXECUTE(line_buffer)
            activate CmdExec
            
            CmdExec->>CmdExec: Tokenize command string
            CmdExec->>CmdExec: Build command from tokens
            
            CmdExec->>CmdTable: Find matching command
            activate CmdTable
            CmdTable->>CmdTable: Iterate through cmdTable[]
            CmdTable->>CmdTable: Compare command strings
            
            alt Command found
                CmdTable->>CmdExec: Return function pointer
                deactivate CmdTable
                CmdExec->>Parser: cmdTable[i].func(argc, argv)
                activate Parser
                Parser->>Parser: Execute command logic
                Parser->>CmdExec: Return
                deactivate Parser
            else Command not found
                CmdTable->>CmdExec: Return NULL
                deactivate CmdTable
                CmdExec->>Parser: Cmd_Default(argc, argv)
                activate Parser
                Parser->>UART: Print "UNKNOWN COMMAND"
                deactivate Parser
            end
            
            CmdExec->>MainLoop: Return
            deactivate CmdExec
            MainLoop->>MainLoop: Clear line buffer
        else Not newline
            MainLoop->>MainLoop: Append to line buffer
            MainLoop->>RingBuf: RingBuffer_Read()
        end
    end
    
    deactivate MainLoop
```

## DataManager State Update and Print Sequence

```mermaid
sequenceDiagram
    participant Parser as Command Parser
    participant DM as DataManager
    participant State as Internal State
    participant MainLoop as Main Loop
    participant RTC as DS3231 RTC
    participant UART as UART TX
    participant User
    
    Parser->>DM: DataManager_UpdateSingle(temp, hum)
    activate DM
    DM->>State: g_datalogger_state.mode = SINGLE
    DM->>State: g_datalogger_state.sht3x.temperature = temp
    DM->>State: g_datalogger_state.sht3x.humidity = hum
    DM->>State: g_datalogger_state.data_ready = true
    DM->>Parser: Return
    deactivate DM
    
    Note over MainLoop: Main loop continues
    
    MainLoop->>DM: DataManager_Print() called
    activate DM
    DM->>State: Check g_datalogger_state.data_ready
    
    alt data_ready == false
        DM->>MainLoop: Return false
    else data_ready == true
        DM->>RTC: DS3231_Get_Time(&g_ds3231, &current_time)
        activate RTC
        RTC->>RTC: I2C read time registers
        RTC->>DM: Return tm structure
        deactivate RTC
        
        DM->>DM: Convert tm to Unix timestamp
        DM->>DM: timestamp = mktime(&current_time)
        
        DM->>State: Read temperature value
        State->>DM: Return temperature
        DM->>State: Read humidity value
        State->>DM: Return humidity
        
        DM->>DM: Sanitize temperature (check NaN/Inf)
        DM->>DM: Sanitize humidity (check NaN/Inf)
        
        alt mode == SINGLE
            DM->>DM: Format JSON with mode="SINGLE"
        else mode == PERIODIC
            DM->>DM: Format JSON with mode="PERIODIC"
        end
        
        DM->>DM: snprintf(buffer, 128, JSON_format, ...)
        
        alt Buffer overflow
            DM->>UART: Print "JSON BUFFER OVERFLOW"
        else Buffer OK
            DM->>UART: HAL_UART_Transmit(&huart1, buffer, len, timeout)
            activate UART
            UART->>User: Transmit JSON string
            deactivate UART
        end
        
        DM->>State: g_datalogger_state.data_ready = false
        DM->>MainLoop: Return true
    end
    deactivate DM
```

## I2C Communication Error Recovery Sequence

```mermaid
sequenceDiagram
    participant Driver as SHT3X Driver
    participant HAL as HAL I2C
    participant I2C as I2C Hardware
    participant Sensor as SHT3X Sensor
    participant Parser as Command Parser
    participant UART
    
    Driver->>HAL: HAL_I2C_Master_Transmit(&hi2c1, addr, data, size, 100)
    activate HAL
    HAL->>I2C: Configure and start I2C transfer
    activate I2C
    I2C->>Sensor: Send START + ADDRESS + DATA
    
    alt Normal operation
        activate Sensor
        Sensor->>I2C: ACK
        deactivate Sensor
        I2C->>HAL: Transfer complete
        deactivate I2C
        HAL->>Driver: Return HAL_OK
        deactivate HAL
        Driver->>Driver: Continue operation
        
    else Timeout (no response)
        Note over Sensor: Sensor not responding
        I2C->>I2C: Wait for timeout (100ms)
        I2C->>HAL: Return HAL_TIMEOUT
        deactivate I2C
        HAL->>Driver: Return HAL_TIMEOUT
        deactivate HAL
        
        Driver->>Driver: Check return status
        Driver->>Driver: Log error
        Driver->>Parser: Return SHT3X_ERROR
        Parser->>UART: Print "SHT3X SINGLE MODE FAILED"
        Parser->>Parser: Preserve previous state
        
    else NACK received
        Sensor->>I2C: NACK (busy/error)
        I2C->>HAL: Return HAL_ERROR
        deactivate I2C
        HAL->>Driver: Return HAL_ERROR
        deactivate HAL
        
        Driver->>Driver: Check return status
        Driver->>HAL: HAL_I2C_GetError(&hi2c1)
        HAL->>Driver: Return error code
        Driver->>Parser: Return SHT3X_ERROR
        Parser->>UART: Print error message
        
    else CRC mismatch
        Sensor->>I2C: Send data with incorrect CRC
        I2C->>HAL: Transfer complete
        deactivate I2C
        HAL->>Driver: Return HAL_OK
        deactivate HAL
        
        Driver->>Driver: Read data buffer
        Driver->>Driver: Calculate CRC on received data
        Driver->>Driver: Compare calculated vs received CRC
        
        alt CRC mismatch detected
            Driver->>Driver: Log CRC error
            Driver->>Parser: Return SHT3X_ERROR
            Parser->>UART: Print "CRC VALIDATION FAILED"
        end
    end
```

## System Initialization Sequence

```mermaid
sequenceDiagram
    participant PWR as Power Supply
    participant MCU as STM32 MCU
    participant HAL as HAL Library
    participant I2C as I2C Peripheral
    participant UART as UART Peripheral
    participant SHT3X as SHT3X Sensor
    participant RTC as DS3231 RTC
    participant DM as DataManager
    participant Main as Main Loop
    
    PWR->>MCU: Power ON
    activate MCU
    MCU->>MCU: Reset & Boot
    MCU->>HAL: HAL_Init()
    activate HAL
    HAL->>HAL: Configure NVIC, SysTick
    HAL->>MCU: Return
    deactivate HAL
    
    MCU->>MCU: SystemClock_Config()
    MCU->>MCU: Configure PLL: 64MHz
    
    MCU->>I2C: MX_I2C1_Init()
    activate I2C
    I2C->>I2C: Configure I2C1: 100kHz, PB6/PB7
    I2C->>MCU: Return
    deactivate I2C
    
    MCU->>UART: MX_USART1_UART_Init()
    activate UART
    UART->>UART: Configure UART1: 115200 baud, PA9/PA10
    UART->>UART: Enable RX interrupt
    UART->>MCU: Return
    deactivate UART
    
    MCU->>UART: UART_Init(&huart1)
    activate UART
    UART->>UART: Initialize ring buffer
    UART->>UART: Clear line buffer
    UART->>MCU: Return
    deactivate UART
    
    MCU->>SHT3X: SHT3X_Init(&g_sht3x, &hi2c1, 0x44)
    activate SHT3X
    SHT3X->>SHT3X: Store I2C handle
    SHT3X->>SHT3X: Store device address
    SHT3X->>SHT3X: currentState = SHT3X_IDLE
    SHT3X->>SHT3X: Clear temperature, humidity
    SHT3X->>I2C: Soft reset command (optional)
    I2C->>SHT3X: ACK
    SHT3X->>MCU: Return
    deactivate SHT3X
    
    MCU->>RTC: DS3231_Init(&g_ds3231, &hi2c1)
    activate RTC
    RTC->>RTC: Store I2C handle
    RTC->>I2C: Read status register
    I2C->>RTC: Return status
    RTC->>MCU: Return
    deactivate RTC
    
    MCU->>DM: DataManager_Init()
    activate DM
    DM->>DM: Clear g_datalogger_state
    DM->>DM: mode = DATALOGGER_MODE_IDLE
    DM->>DM: data_ready = false
    DM->>DM: temperature = 0.0
    DM->>DM: humidity = 0.0
    DM->>MCU: Return
    deactivate DM
    
    MCU->>Main: Enter main loop (while(1))
    deactivate MCU
    
    activate Main
    Note over Main: System ready for commands
    Main->>Main: UART_Handle()
    Main->>Main: DataManager_Print()
    Main->>Main: Check periodic state
    Main->>Main: __WFI()
    deactivate Main
```

---

**Key Points:**
- Sequences show time-ordered interactions between components
- Activation bars indicate when a component is active/processing
- Loops represent periodic or repeated operations
- Alt blocks show conditional execution paths
- Notes provide additional context
