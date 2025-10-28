# STM32 Data Logger - Sequence Diagrams

This document illustrates the time-ordered interactions between components in the STM32 firmware.

## Single Shot Measurement Sequence

```mermaid
sequenceDiagram
    participant User
    participant UART as UART Interface
    participant RingBuf as Ring Buffer
    participant CmdExec as Command Execute
    participant Parser as SINGLE_PARSER
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant DM as DataManager
    participant RTC as DS3231 RTC
    participant MQTT as MQTT State
    
    User->>UART: "SINGLE\n"
    activate UART
    UART->>RingBuf: Store bytes (interrupt)
    deactivate UART
    
    Note over RingBuf: Accumulate until '\n'
    
    RingBuf->>CmdExec: UART_Handle() detects complete line
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["SINGLE"]
    CmdExec->>CmdExec: Find in cmdTable[]
    CmdExec->>Parser: SINGLE_PARSER(argc=1, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>Parser: Set repeat = SHT3X_HIGH (default)
    Parser->>Driver: SHT3X_Single(&g_sht3x, &repeat, &temp, &hum)
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
    
    alt I2C OK and CRC Valid
        Driver->>Driver: Convert raw → 25.50°C, 60.00%
        Driver->>Parser: Return SHT3X_OK, temp=25.50, hum=60.00
    else I2C Error or CRC Failed
        Driver->>Parser: Return SHT3X_ERROR, temp=0.0, hum=0.0
    end
    deactivate Driver
    
    activate Parser
    Parser->>DM: DataManager_UpdateSingle(temp, hum)
    deactivate Parser
    
    activate DM
    DM->>DM: mode = DATA_MANAGER_MODE_SINGLE
    DM->>DM: Store temperature, humidity
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
    DM->>DM: Convert to Unix timestamp: 1729699200
    DM->>DM: Sanitize floats (check NaN/Inf)
    
    DM->>MQTT: Check mqtt_current_state
    activate MQTT
    MQTT->>DM: MQTT_STATE_CONNECTED
    deactivate MQTT
    
    DM->>DM: Format JSON: {"mode":"SINGLE","timestamp":1729699200,...}
    DM->>UART: HAL_UART_Transmit(JSON string)
    activate UART
    UART->>User: {"mode":"SINGLE","timestamp":1729699200,"temperature":25.50,"humidity":60.00}
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
    participant Parser as PERIODIC_ON_PARSER
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant DM as DataManager
    participant MainLoop as Main Loop
    
    User->>UART: "PERIODIC ON\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["PERIODIC", "ON"]
    CmdExec->>Parser: PERIODIC_ON_PARSER(argc=2, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>Parser: Set mode=1MPS (default), repeat=HIGH (default)
    Parser->>Driver: SHT3X_Periodic(&g_sht3x, &mode, &repeat, &temp, &hum)
    deactivate Parser
    
    activate Driver
    Driver->>Driver: Build I2C command: 0x2032 (1MPS HIGH)
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
    Driver->>Driver: HAL_Delay(100ms) - Wait for first measurement
    
    Driver->>I2C: HAL_I2C_Master_Receive(6 bytes)
    activate I2C
    Sensor->>I2C: Send first measurement
    I2C->>Driver: Raw data
    deactivate I2C
    
    alt I2C OK and CRC Valid
        Driver->>Driver: Parse & validate data
        Driver->>Parser: Return SHT3X_OK, temp=25.50, hum=60.00
    else I2C Error or CRC Failed
        Driver->>Parser: Return SHT3X_ERROR, temp=0.0, hum=0.0
    end
    deactivate Driver
    
    activate Parser
    Parser->>Parser: Initialize next_fetch_ms = HAL_GetTick()
    Parser->>DM: DataManager_UpdatePeriodic(temp, hum)
    activate DM
    DM->>DM: mode = DATA_MANAGER_MODE_PERIODIC
    DM->>DM: Store temperature, humidity
    DM->>DM: Set data_ready = true
    DM->>Parser: Return
    deactivate DM
    deactivate Parser
    
    Note over MainLoop: Main loop continues every 5 seconds...
    
    loop Every periodic_interval_ms (default 5000ms)
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
        
        MainLoop->>MainLoop: Toggle PC13 LED
        MainLoop->>MainLoop: Update next_fetch_ms += periodic_interval_ms
        
        MainLoop->>DM: DataManager_Print()
        activate DM
        DM->>UART: Transmit JSON
        activate UART
        UART->>User: {"mode":"PERIODIC","timestamp":...,"temperature":...}
        deactivate UART
        DM->>DM: Clear data_ready = false
        deactivate DM
    end
```

## Periodic Measurement Stop Sequence

```mermaid
sequenceDiagram
    participant User
    participant UART as UART Interface
    participant CmdExec as Command Execute
    participant Parser as PERIODIC_OFF_PARSER
    participant Driver as SHT3X Driver
    participant I2C as I2C Bus
    participant Sensor as SHT3X Sensor
    participant MainLoop as Main Loop
    
    Note over Sensor: Currently in periodic mode @ 1Hz
    
    User->>UART: "PERIODIC OFF\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>CmdExec: Tokenize: ["PERIODIC", "OFF"]
    CmdExec->>Parser: PERIODIC_OFF_PARSER(argc=2, argv)
    deactivate CmdExec
    
    activate Parser
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
    Parser->>UART: Print "[CMD] PERIODIC OFF"
    activate UART
    UART->>User: "[CMD] PERIODIC OFF\r\n"
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
            Note over CmdTable: Search for: SINGLE, PERIODIC ON/OFF,<br/>SET TIME, SET PERIODIC INTERVAL,<br/>MQTT CONNECTED/DISCONNECTED, SD CLEAR
            CmdTable->>CmdExec: Return result
            deactivate CmdTable
            
            alt Command found
                CmdExec->>Parser: cmdTable[i].func(argc, argv)
                activate Parser
                Parser->>Parser: Execute command logic
                Parser->>CmdExec: Return
                deactivate Parser
            else Command not found
                CmdExec->>Parser: Print "UNKNOWN COMMAND"
                activate Parser
                Parser->>CmdExec: Return
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
    participant MQTT as MQTT State
    participant SD as SD Card Manager
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
        
        DM->>MQTT: Check mqtt_current_state
        activate MQTT
        
        alt MQTT_STATE_CONNECTED
            MQTT->>DM: MQTT_STATE_CONNECTED
            deactivate MQTT
            
            DM->>DM: Format JSON: {"mode":"SINGLE","timestamp":...,"temperature":...}
            
            DM->>DM: Check buffer overflow
            
            alt Buffer OK
                DM->>UART: HAL_UART_Transmit(&huart1, JSON, len, timeout)
                activate UART
                UART->>User: Transmit JSON string
                deactivate UART
            else Buffer overflow
                DM->>UART: Print "JSON BUFFER OVERFLOW"
            end
            
        else MQTT_STATE_DISCONNECTED
            MQTT->>DM: MQTT_STATE_DISCONNECTED
            deactivate MQTT
            
            DM->>SD: SDCardManager_WriteData(timestamp, temp, hum, mode)
            activate SD
            SD->>SD: Write to circular buffer on SD card
            SD->>DM: Return success/failure
            deactivate SD
            
            Note over DM: Data buffered to SD card for later transmission
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
    participant DM as DataManager
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
        Driver->>Parser: Return SHT3X_OK with valid data
        
    else Timeout (no response)
        Note over Sensor: Sensor not responding
        activate I2C
        I2C->>I2C: Wait for timeout (100ms)
        I2C->>HAL: Return HAL_TIMEOUT
        deactivate I2C
        activate HAL
        HAL->>Driver: Return HAL_TIMEOUT
        deactivate HAL
        
        Driver->>Driver: Check return status
        Driver->>Driver: Log error
        Driver->>Parser: Return SHT3X_ERROR, temp=0.0, hum=0.0
        Parser->>DM: DataManager_Update(0.0, 0.0)
        Note over DM: Stores 0.0 values to indicate sensor failure
        
    else NACK received
        activate Sensor
        Sensor->>I2C: NACK (busy/error)
        deactivate Sensor
        activate I2C
        I2C->>HAL: Return HAL_ERROR
        deactivate I2C
        activate HAL
        HAL->>Driver: Return HAL_ERROR
        deactivate HAL
        
        Driver->>Driver: Check return status
        Driver->>HAL: HAL_I2C_GetError(&hi2c1)
        activate HAL
        HAL->>Driver: Return error code
        deactivate HAL
        Driver->>Parser: Return SHT3X_ERROR, temp=0.0, hum=0.0
        Parser->>DM: DataManager_Update(0.0, 0.0)
        
    else CRC mismatch
        activate Sensor
        Sensor->>I2C: Send data with incorrect CRC
        deactivate Sensor
        activate I2C
        I2C->>HAL: Transfer complete
        deactivate I2C
        activate HAL
        HAL->>Driver: Return HAL_OK
        deactivate HAL
        
        Driver->>Driver: Read data buffer
        Driver->>Driver: Calculate CRC on received data
        Driver->>Driver: Compare calculated vs received CRC
        
        alt CRC mismatch detected
            Driver->>Driver: Log CRC error
            Driver->>Parser: Return SHT3X_ERROR, temp=0.0, hum=0.0
            Parser->>DM: DataManager_Update(0.0, 0.0)
            Parser->>UART: Print "[ERROR] CRC VALIDATION FAILED"
        end
    end
```

## MQTT State Change Notification Sequence

```mermaid
sequenceDiagram
    participant ESP32
    participant UART as UART Interface
    participant CmdExec as Command Execute
    participant Parser as MQTT Parser
    participant MQTTState as mqtt_current_state
    participant MainLoop as Main Loop
    participant DM as DataManager
    participant SD as SD Card Manager
    
    Note over ESP32: ESP32 detects MQTT connection
    
    ESP32->>UART: "MQTT CONNECTED\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>Parser: MQTT_CONNECTED_PARSER(argc, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>MQTTState: mqtt_current_state = MQTT_STATE_CONNECTED
    Parser->>UART: Print "[MQTT] CONNECTED"
    deactivate Parser
    
    Note over MainLoop: Main loop detects MQTT connected
    
    loop While buffered data exists
        MainLoop->>SD: SDCardManager_GetBufferedCount()
        activate SD
        SD->>MainLoop: Return count > 0
        deactivate SD
        
        MainLoop->>MainLoop: Wait 100ms between records
        
        MainLoop->>SD: SDCardManager_ReadData(&record)
        activate SD
        SD->>SD: Read from circular buffer
        SD->>MainLoop: Return buffered record
        deactivate SD
        
        MainLoop->>DM: sensor_json_format(buffer, record data)
        activate DM
        DM->>MainLoop: Return formatted JSON
        deactivate DM
        
        MainLoop->>UART: Transmit buffered JSON
        activate UART
        UART->>ESP32: {"mode":"PERIODIC","timestamp":...,"temperature":...}
        deactivate UART
        
        MainLoop->>SD: SDCardManager_RemoveRecord()
        activate SD
        SD->>SD: Increment read_index, decrement count
        SD->>MainLoop: Return success
        deactivate SD
    end
    
    Note over MainLoop: All buffered data sent
    
    Note over ESP32: ESP32 loses MQTT connection
    
    ESP32->>UART: "MQTT DISCONNECTED\n"
    activate UART
    UART->>CmdExec: Complete command received
    deactivate UART
    
    activate CmdExec
    CmdExec->>Parser: MQTT_DISCONNECTED_PARSER(argc, argv)
    deactivate CmdExec
    
    activate Parser
    Parser->>MQTTState: mqtt_current_state = MQTT_STATE_DISCONNECTED
    Parser->>UART: Print "[MQTT] DISCONNECTED"
    deactivate Parser
    
    Note over MainLoop: Future data will be buffered to SD
```

## System Initialization Sequence

```mermaid
sequenceDiagram
    participant PWR as Power Supply
    participant MCU as STM32 MCU
    participant HAL as HAL Library
    participant I2C as I2C Peripheral
    participant SPI as SPI Peripherals
    participant UART as UART Peripheral
    participant SHT3X as SHT3X Sensor
    participant RTC as DS3231 RTC
    participant SD as SD Card Manager
    participant Display as ILI9225 Display
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
    MCU->>MCU: Configure PLL: 72MHz
    
    MCU->>I2C: MX_I2C1_Init()
    activate I2C
    I2C->>I2C: Configure I2C1: 100kHz, PB6/PB7
    I2C->>MCU: Return
    deactivate I2C
    
    MCU->>SPI: MX_SPI1_Init()
    activate SPI
    SPI->>SPI: Configure SPI1: 18MHz (SD Card)
    SPI->>MCU: Return
    deactivate SPI
    
    MCU->>SPI: MX_SPI2_Init()
    activate SPI
    SPI->>SPI: Configure SPI2: 36MHz (Display)
    SPI->>MCU: Return
    deactivate SPI
    
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
    DM->>DM: mode = DATA_MANAGER_MODE_IDLE
    DM->>DM: data_ready = false
    DM->>DM: temperature = 0.0
    DM->>DM: humidity = 0.0
    DM->>MCU: Return
    deactivate DM
    
    MCU->>MCU: HAL_Delay(200ms) - SD power stabilization
    
    MCU->>SD: SDCardManager_Init()
    activate SD
    SD->>SD: Initialize SPI & SD card
    SD->>SD: Read metadata from block 1
    
    alt SD card initialized successfully
        SD->>MCU: Return true
    else SD card failed
        SD->>UART: Print "[WARN] SD Card NOT available!"
        SD->>MCU: Return false (continue anyway)
    end
    deactivate SD
    
    MCU->>Display: ILI9225_Init()
    activate Display
    Display->>Display: Configure SPI2 pins
    Display->>Display: Send initialization commands
    Display->>MCU: Return
    deactivate Display
    
    MCU->>MCU: HAL_Delay(50ms) - Display stabilization
    
    MCU->>Display: display_init()
    activate Display
    Display->>Display: Clear screen
    Display->>Display: Set default layout
    Display->>MCU: Return
    deactivate Display
    
    MCU->>Main: Enter main loop (while(1))
    deactivate MCU
    
    activate Main
    Note over Main: System ready - MQTT_STATE_DISCONNECTED by default
    Main->>Main: UART_Handle()
    Main->>Main: Check periodic state
    Main->>Main: DataManager_Print() or buffer to SD
    Main->>Main: Display_Update()
    deactivate Main
```

---

**Key Points:**
- Sequences show time-ordered interactions between components
- Activation bars indicate when a component is active/processing
- Loops represent periodic or repeated operations
- Alt blocks show conditional execution paths
- Notes provide additional context
