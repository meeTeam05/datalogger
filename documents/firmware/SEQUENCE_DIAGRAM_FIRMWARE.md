# Complete Firmware System - Sequence Diagrams

This document illustrates the time-ordered interactions between STM32 and ESP32 firmware components in the complete system.

## Complete System Initialization Sequence

```mermaid
sequenceDiagram
    participant PWR as Power Supply
    participant STM32
    participant SHT3X as SHT3X Sensor
    participant DS3231 as DS3231 RTC
    participant SD as SD Card
    participant Display as ILI9225 Display
    participant UART as UART Link
    participant ESP32
    participant WiFi as WiFi Network
    participant MQTT as MQTT Broker
    
    PWR->>STM32: Power ON
    PWR->>ESP32: Power ON
    
    activate STM32
    STM32->>STM32: HAL_Init, SystemClock_Config
    STM32->>STM32: MX_GPIO_Init
    STM32->>STM32: MX_I2C1_Init (100kHz)
    STM32->>STM32: MX_SPI1_Init (18MHz SD)
    STM32->>STM32: MX_SPI2_Init (36MHz Display)
    STM32->>STM32: MX_USART1_UART_Init (115200)
    
    STM32->>SHT3X: SHT3X_Init(0x44)
    activate SHT3X
    SHT3X->>STM32: SHT3X_OK
    deactivate SHT3X
    
    STM32->>DS3231: DS3231_Init(0x68)
    activate DS3231
    DS3231->>STM32: DS3231_OK
    deactivate DS3231
    
    STM32->>SD: SDCardManager_Init
    activate SD
    SD->>SD: Mount FAT filesystem
    SD->>SD: Create/Open DATALOG.TXT
    SD->>STM32: Init OK (or disabled if failed)
    deactivate SD
    
    STM32->>Display: ILI9225_Init
    activate Display
    Display->>Display: Configure SPI2
    Display->>Display: Send init commands
    Display->>STM32: Display ready
    deactivate Display
    
    STM32->>Display: Display startup screen
    STM32->>STM32: Enter main loop (IDLE mode)
    
    Note over STM32: STM32 Ready - Waiting for commands
    
    activate ESP32
    ESP32->>ESP32: NVS Flash Init
    ESP32->>ESP32: Event Loop Init
    ESP32->>ESP32: Network Interface Init
    
    ESP32->>WiFi: WiFi_Init(config)
    activate WiFi
    WiFi->>ESP32: Init OK
    deactivate WiFi
    
    ESP32->>UART: STM32_UART_Init(UART1, 115200)
    activate UART
    UART->>UART: Allocate ring buffer
    UART->>ESP32: UART ready
    deactivate UART
    
    ESP32->>ESP32: Relay_Init(GPIO4)
    ESP32->>ESP32: JSON_Parser_Init
    ESP32->>ESP32: Button_Init(GPIO5, 16, 17, 4)
    
    ESP32->>WiFi: WiFi_Connect (15s timeout)
    activate WiFi
    WiFi->>WiFi: Scan networks
    WiFi->>WiFi: Connect to AP
    WiFi-->>ESP32: Connection status updates
    
    alt WiFi Connected
        WiFi->>ESP32: WIFI_STATE_CONNECTED
        ESP32->>ESP32: Mark wifi_reconnect_time
        ESP32->>ESP32: Wait 4s for stability
        
        ESP32->>MQTT: MQTT_Handler_Init(config)
        activate MQTT
        MQTT->>ESP32: Init OK
        deactivate MQTT
        
        ESP32->>MQTT: MQTT_Handler_Start
        activate MQTT
        MQTT->>MQTT: Connect to broker
        MQTT->>ESP32: CONNECTED event
        deactivate MQTT
        
        ESP32->>MQTT: Subscribe to topics
        activate MQTT
        MQTT->>MQTT: Subscribe datalogger/stm32/command
        MQTT->>MQTT: Subscribe datalogger/esp32/relay/control
        MQTT->>MQTT: Subscribe datalogger/esp32/system/state
        MQTT->>ESP32: Subscribed
        deactivate MQTT
        
        ESP32->>UART: Send "MQTT CONNECTED\n"
        UART->>STM32: UART RX Interrupt
        STM32->>STM32: Parse: MQTT CONNECTED
        STM32->>STM32: mqtt_current_state = CONNECTED
        STM32->>Display: Update MQTT status icon
        
    else WiFi Failed
        WiFi->>ESP32: WIFI_STATE_FAILED
        ESP32->>ESP32: Log warning, continue
        Note over ESP32: Will retry WiFi in background
    end
    deactivate WiFi
    
    ESP32->>ESP32: Button_StartTask
    ESP32->>UART: STM32_UART_StartTask
    ESP32->>ESP32: Enter main loop
    
    Note over STM32,ESP32: Both systems operational
    
    deactivate STM32
    deactivate ESP32
```

## Single Measurement End-to-End Sequence

```mermaid
sequenceDiagram
    participant Web as Web Interface
    participant MQTT as MQTT Broker
    participant ESP32
    participant UART as UART Link
    participant STM32
    participant SHT3X as SHT3X Sensor
    participant DS3231 as DS3231 RTC
    participant Display
    
    Web->>MQTT: Publish "SINGLE" to datalogger/stm32/command
    activate MQTT
    MQTT->>ESP32: Forward message
    deactivate MQTT
    
    activate ESP32
    ESP32->>ESP32: on_mqtt_data_received callback
    ESP32->>ESP32: Check topic: datalogger/stm32/command
    ESP32->>UART: STM32_UART_SendCommand("SINGLE\n")
    deactivate ESP32
    
    activate UART
    UART->>STM32: UART RX Interrupt (0x53='S')
    deactivate UART
    
    activate STM32
    STM32->>STM32: Store in ring buffer
    STM32->>STM32: UART_Handle detects '\n'
    STM32->>STM32: Assemble line: "SINGLE"
    STM32->>STM32: COMMAND_EXECUTE("SINGLE")
    STM32->>STM32: Tokenize: argc=1, argv[0]="SINGLE"
    STM32->>STM32: Find in cmdTable
    STM32->>STM32: Call SINGLE_PARSER
    
    STM32->>STM32: Set repeatability = SHT3X_HIGH
    STM32->>SHT3X: SHT3X_Single(&g_sht3x, &repeat, &temp, &hum)
    activate SHT3X
    
    STM32->>SHT3X: I2C Write [0x24, 0x00] to 0x44
    SHT3X->>SHT3X: Start measurement
    STM32->>STM32: HAL_Delay(15ms)
    
    STM32->>SHT3X: I2C Read 6 bytes
    SHT3X->>STM32: [T_MSB, T_LSB, T_CRC, RH_MSB, RH_LSB, RH_CRC]
    deactivate SHT3X
    
    STM32->>STM32: Validate CRC
    
    alt CRC Valid
        STM32->>STM32: Convert: temp=30.59°C, hum=73.97%
    else CRC Failed or I2C Error
        STM32->>STM32: Set: temp=0.0, hum=0.0
    end
    
    STM32->>STM32: DataManager_UpdateSingle(temp, hum)
    STM32->>STM32: mode = SINGLE, data_ready = true
    
    Note over STM32: Wait for next main loop iteration
    
    STM32->>STM32: Main loop: DataManager_Print called
    STM32->>STM32: Check data_ready == true
    STM32->>STM32: Check mqtt_current_state
    
    alt MQTT Connected
        STM32->>DS3231: DS3231_Get_Time
        activate DS3231
        DS3231->>STM32: struct tm time
        deactivate DS3231
        
        STM32->>STM32: Convert to Unix timestamp: 1729699200
        STM32->>STM32: Format JSON:
        Note right of STM32: {"mode":"SINGLE","timestamp":1729699200,<br/>"temperature":30.59,"humidity":73.97}
        
        STM32->>UART: HAL_UART_Transmit(JSON)
        activate UART
        UART->>ESP32: UART RX data
        deactivate UART
        
        activate ESP32
        ESP32->>ESP32: STM32_UART RX task
        ESP32->>ESP32: Assemble line from ring buffer
        ESP32->>ESP32: on_stm32_data_received callback
        ESP32->>ESP32: JSON_Parser_ProcessLine
        ESP32->>ESP32: mode = SINGLE detected
        ESP32->>ESP32: on_single_sensor_data callback
        
        ESP32->>MQTT: MQTT_Handler_Publish
        Note right of ESP32: Topic: datalogger/stm32/single/data<br/>QoS: 0, Retain: 0
        activate MQTT
        MQTT->>Web: Forward sensor data
        deactivate MQTT
        
        Web->>Web: Update UI with sensor values
        deactivate ESP32
        
    else MQTT Disconnected
        STM32->>STM32: SDCardManager_WriteData
        activate STM32
        STM32->>STM32: Format: timestamp,temp,hum,SINGLE
        STM32->>STM32: Write to SD Card circular buffer
        STM32->>Display: Update buffered count
        deactivate STM32
        Note over STM32: Data buffered for later transmission
    end
    
    STM32->>STM32: Clear data_ready flag
    STM32->>Display: Update display with sensor values
    deactivate STM32
```

## Periodic Measurement Setup and Operation Sequence

```mermaid
sequenceDiagram
    participant Web
    participant MQTT
    participant ESP32
    participant UART
    participant STM32
    participant SHT3X
    participant Display
    
    Web->>MQTT: Publish "PERIODIC ON" to datalogger/stm32/command
    MQTT->>ESP32: Message received
    
    activate ESP32
    ESP32->>ESP32: on_mqtt_data_received
    ESP32->>ESP32: Parse command: PERIODIC ON
    ESP32->>ESP32: g_periodic_active = true
    ESP32->>ESP32: update_and_publish_state
    
    ESP32->>MQTT: Publish state update (retain=1)
    Note right of ESP32: {"device":"ON","periodic":"ON","timestamp":...}
    
    ESP32->>UART: Send "PERIODIC ON\n"
    deactivate ESP32
    
    UART->>STM32: UART RX
    activate STM32
    STM32->>STM32: Parse: PERIODIC ON
    STM32->>STM32: PERIODIC_ON_PARSER(argc=2)
    STM32->>STM32: Set mode=1MPS, repeat=HIGH
    
    STM32->>SHT3X: SHT3X_Periodic(&mode, &repeat)
    activate SHT3X
    STM32->>SHT3X: I2C Write [0x20, 0x32] (1MPS HIGH)
    SHT3X->>SHT3X: Enter periodic mode @ 1Hz
    SHT3X->>STM32: ACK
    deactivate SHT3X
    
    STM32->>STM32: currentState = SHT3X_PERIODIC_1MPS
    STM32->>STM32: Initialize next_fetch_ms = HAL_GetTick
    STM32->>STM32: periodic_interval_ms = 5000 (default)
    
    STM32->>SHT3X: Fetch first measurement
    activate SHT3X
    STM32->>SHT3X: I2C Read 6 bytes
    SHT3X->>STM32: Data
    deactivate SHT3X
    
    STM32->>STM32: DataManager_UpdatePeriodic
    STM32->>Display: Update display mode: PERIODIC
    
    loop Every 5 seconds (periodic_interval_ms)
        STM32->>STM32: Main loop checks: now >= next_fetch_ms
        STM32->>SHT3X: SHT3X_FetchData
        activate SHT3X
        SHT3X->>STM32: Latest measurement
        deactivate SHT3X
        
        STM32->>STM32: DataManager_UpdatePeriodic
        STM32->>STM32: data_ready = true
        STM32->>STM32: next_fetch_ms += 5000
        
        alt MQTT Connected
            STM32->>STM32: Format JSON
            STM32->>UART: Send JSON
            UART->>ESP32: Data received
            
            activate ESP32
            ESP32->>ESP32: Parse JSON, mode=PERIODIC
            ESP32->>ESP32: on_periodic_sensor_data
            ESP32->>MQTT: Publish to datalogger/stm32/periodic/data
            MQTT->>Web: Forward data
            Web->>Web: Update real-time chart
            deactivate ESP32
            
        else MQTT Disconnected
            STM32->>STM32: SDCardManager_WriteData
            STM32->>Display: Update buffer count
            Note over STM32: Data buffered to SD
        end
        
        STM32->>Display: Update sensor values on display
    end
    
    Note over STM32,Web: Periodic measurements continue...
    
    Web->>MQTT: Publish "PERIODIC OFF"
    MQTT->>ESP32: Message received
    
    activate ESP32
    ESP32->>ESP32: g_periodic_active = false
    ESP32->>ESP32: update_and_publish_state
    ESP32->>UART: Send "PERIODIC OFF\n"
    deactivate ESP32
    
    UART->>STM32: UART RX
    STM32->>STM32: PERIODIC_OFF_PARSER
    STM32->>SHT3X: SHT3X_PeriodicStop
    activate SHT3X
    STM32->>SHT3X: I2C Write [0x30, 0x93] (Break command)
    SHT3X->>SHT3X: Exit periodic mode
    SHT3X->>STM32: ACK
    deactivate SHT3X
    
    STM32->>STM32: currentState = SHT3X_IDLE
    STM32->>Display: Update mode: IDLE
    deactivate STM32
```

## MQTT Connection State Change and Synchronization

```mermaid
sequenceDiagram
    participant STM32
    participant UART
    participant ESP32
    participant WiFi
    participant MQTT
    participant Display
    
    Note over ESP32: System running, MQTT disconnected
    
    ESP32->>WiFi: Monitor WiFi state
    activate WiFi
    WiFi->>ESP32: WIFI_STATE_CONNECTED
    deactivate WiFi
    
    activate ESP32
    ESP32->>ESP32: Check: stable for 4s?
    ESP32->>ESP32: Yes - start MQTT
    
    ESP32->>MQTT: MQTT_Handler_Start
    activate MQTT
    MQTT->>MQTT: Connect to broker
    MQTT-->>ESP32: Connecting...
    
    alt Connection Successful
        MQTT->>ESP32: MQTT_CONNECTED event
        ESP32->>ESP32: connected = true
        ESP32->>ESP32: mqtt_current_state = true
        ESP32->>ESP32: Set mqtt_reconnected flag
        
        ESP32->>MQTT: Subscribe to command topics
        MQTT->>ESP32: Subscriptions confirmed
        
        ESP32->>UART: Send "MQTT CONNECTED\n"
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: Parse: MQTT CONNECTED
        STM32->>STM32: MQTT_CONNECTED_PARSER
        STM32->>STM32: mqtt_current_state = CONNECTED
        STM32->>Display: Update MQTT icon (connected)
        
        STM32->>STM32: Check: Has buffered data?
        
        alt Has Buffered Data
            loop For each buffered record
                STM32->>STM32: SDCardManager_ReadData
                STM32->>STM32: Format buffered JSON
                STM32->>UART: Transmit buffered record
                activate UART
                UART->>ESP32: Data received
                deactivate UART
                
                activate ESP32
                ESP32->>ESP32: Parse JSON
                ESP32->>MQTT: Publish buffered data
                MQTT->>MQTT: Forward to subscribers
                deactivate ESP32
                
                STM32->>STM32: SDCardManager_RemoveRecord
                STM32->>Display: Update buffer count
                STM32->>STM32: Delay 100ms (rate limiting)
            end
            
            STM32->>Display: Buffer empty indicator
            Note over STM32: All buffered data transmitted
        end
        
        deactivate STM32
        
    else Connection Failed
        MQTT->>ESP32: MQTT_ERROR event
        ESP32->>ESP32: Increment retry_count
        ESP32->>ESP32: Calculate backoff: min(60, 2^retry)
        ESP32->>ESP32: Wait backoff delay
        ESP32->>MQTT: Retry connection
        Note over ESP32,MQTT: Exponential backoff retry
    end
    deactivate MQTT
    
    Note over ESP32,STM32: Time passes, WiFi disconnects
    
    WiFi->>ESP32: WIFI_STATE_DISCONNECTED
    activate ESP32
    ESP32->>ESP32: Detect WiFi loss
    ESP32->>MQTT: MQTT_Handler_Stop
    activate MQTT
    MQTT->>MQTT: Disconnect from broker
    MQTT->>ESP32: Stopped
    deactivate MQTT
    
    ESP32->>ESP32: mqtt_current_state = false
    ESP32->>UART: Send "MQTT DISCONNECTED\n"
    activate UART
    UART->>STM32: UART RX
    deactivate UART
    
    activate STM32
    STM32->>STM32: Parse: MQTT DISCONNECTED
    STM32->>STM32: MQTT_DISCONNECTED_PARSER
    STM32->>STM32: mqtt_current_state = DISCONNECTED
    STM32->>Display: Update MQTT icon (disconnected)
    
    Note over STM32: Switch to SD buffering mode
    deactivate STM32
    deactivate ESP32
```

## Relay Control from Web Interface

```mermaid
sequenceDiagram
    participant Web
    participant MQTT
    participant ESP32
    participant Relay
    participant UART
    participant STM32
    participant Display
    
    Web->>Web: User clicks "Turn ON" button
    Web->>MQTT: Publish "ON" to datalogger/esp32/relay/control
    
    activate MQTT
    MQTT->>ESP32: Message received
    deactivate MQTT
    
    activate ESP32
    ESP32->>ESP32: on_mqtt_data_received callback
    ESP32->>ESP32: Topic: datalogger/esp32/relay/control
    ESP32->>ESP32: Data: "ON"
    ESP32->>ESP32: Relay_ProcessCommand(&g_relay, "ON")
    
    ESP32->>Relay: Relay_SetState(true)
    activate Relay
    Relay->>Relay: GPIO4 = HIGH
    Relay->>Relay: state = true
    Relay->>ESP32: Relay callback: on_relay_state_changed(true)
    deactivate Relay
    
    ESP32->>ESP32: g_device_on = true
    ESP32->>ESP32: Log: Relay turned ON
    ESP32->>ESP32: update_and_publish_state
    
    ESP32->>MQTT: Publish state
    activate MQTT
    Note right of ESP32: Topic: datalogger/esp32/system/state<br/>Payload: {"device":"ON","periodic":"OFF",...}<br/>Retain: 1
    MQTT->>Web: Forward state update
    deactivate MQTT
    
    Web->>Web: Update UI: Device ON badge
    
    ESP32->>ESP32: Wait 500ms for STM32 boot
    Note over ESP32: STM32 needs time to initialize after relay ON
    
    ESP32->>ESP32: Check MQTT state
    
    alt MQTT Connected
        ESP32->>UART: Send "MQTT CONNECTED\n"
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: Parse: MQTT CONNECTED
        STM32->>STM32: mqtt_current_state = CONNECTED
        STM32->>Display: Update MQTT icon
        deactivate STM32
        
    else MQTT Disconnected
        ESP32->>UART: Send "MQTT DISCONNECTED\n"
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: Parse: MQTT DISCONNECTED
        STM32->>STM32: mqtt_current_state = DISCONNECTED
        STM32->>Display: Update MQTT icon
        deactivate STM32
    end
    
    deactivate ESP32
    
    Note over Web,STM32: Later, user turns OFF relay
    
    Web->>MQTT: Publish "OFF" to datalogger/esp32/relay/control
    MQTT->>ESP32: Message received
    
    activate ESP32
    ESP32->>Relay: Relay_SetState(false)
    activate Relay
    Relay->>Relay: GPIO4 = LOW
    Relay->>ESP32: Callback: on_relay_state_changed(false)
    deactivate Relay
    
    ESP32->>ESP32: g_device_on = false
    ESP32->>ESP32: Force: g_periodic_active = false
    ESP32->>UART: Send "PERIODIC OFF\n"
    
    activate UART
    UART->>STM32: UART RX
    deactivate UART
    
    activate STM32
    STM32->>STM32: Stop periodic measurements if active
    deactivate STM32
    
    ESP32->>ESP32: update_and_publish_state
    ESP32->>MQTT: Publish state (device OFF, periodic OFF)
    MQTT->>Web: Update UI
    deactivate ESP32
```

## Button Press Sequence (ESP32 Buttons)

```mermaid
sequenceDiagram
    participant User
    participant Button as Button GPIO
    participant ESP32
    participant UART
    participant STM32
    participant MQTT
    
    User->>Button: Press GPIO5 (Relay Toggle)
    activate Button
    Button->>ESP32: GPIO Interrupt
    deactivate Button
    
    activate ESP32
    ESP32->>ESP32: Button task detects press
    ESP32->>ESP32: Software debounce (200ms)
    ESP32->>ESP32: Check last_press_time
    ESP32->>ESP32: on_button_relay_pressed(GPIO5)
    ESP32->>ESP32: Toggle g_device_on
    ESP32->>ESP32: Relay_Toggle(&g_relay)
    ESP32->>ESP32: See Relay Control sequence...
    deactivate ESP32
    
    User->>Button: Press GPIO16 (Single Measurement)
    activate Button
    Button->>ESP32: GPIO Interrupt
    deactivate Button
    
    activate ESP32
    ESP32->>ESP32: on_button_single_pressed(GPIO16)
    ESP32->>ESP32: Check: g_device_on == true?
    
    alt Device ON
        ESP32->>ESP32: Log: Button single pressed
        ESP32->>UART: Send "SINGLE\n"
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: Execute single measurement
        Note over STM32: See Single Measurement sequence...
        deactivate STM32
        
    else Device OFF
        ESP32->>ESP32: Log: Ignored - device OFF
        Note over ESP32: No action taken
    end
    deactivate ESP32
    
    User->>Button: Press GPIO17 (Periodic Toggle)
    activate Button
    Button->>ESP32: GPIO Interrupt
    deactivate Button
    
    activate ESP32
    ESP32->>ESP32: on_button_periodic_pressed(GPIO17)
    ESP32->>ESP32: Check: g_device_on == true?
    
    alt Device ON
        ESP32->>ESP32: Toggle g_periodic_active
        ESP32->>ESP32: new_state = !g_periodic_active
        
        alt Periodic ON
            ESP32->>UART: Send "PERIODIC ON\n"
            ESP32->>ESP32: update_and_publish_state(device=true, periodic=true)
        else Periodic OFF
            ESP32->>UART: Send "PERIODIC OFF\n"
            ESP32->>ESP32: update_and_publish_state(device=true, periodic=false)
        end
        
        ESP32->>MQTT: Publish state update
        
    else Device OFF
        ESP32->>ESP32: Log: Ignored - device OFF
    end
    deactivate ESP32
    
    User->>Button: Press GPIO4 (Interval Adjust)
    activate Button
    Button->>ESP32: GPIO Interrupt
    deactivate Button
    
    activate ESP32
    ESP32->>ESP32: on_button_interval_pressed(GPIO4)
    ESP32->>ESP32: Check: g_device_on == true?
    
    alt Device ON
        ESP32->>ESP32: Cycle g_interval_index: 0→1→2→3→4→0
        ESP32->>ESP32: intervals = [5, 10, 15, 30, 60] seconds
        ESP32->>ESP32: new_interval = intervals[g_interval_index]
        ESP32->>ESP32: Format: "SET PERIODIC INTERVAL {value}\n"
        ESP32->>UART: Send command
        
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: Parse: SET PERIODIC INTERVAL
        STM32->>STM32: periodic_interval_ms = value * 1000
        STM32->>STM32: Log: Interval updated
        deactivate STM32
        
    else Device OFF
        ESP32->>ESP32: Log: Ignored - device OFF
    end
    deactivate ESP32
```

## WiFi Disconnection and Reconnection

```mermaid
sequenceDiagram
    participant STM32
    participant UART
    participant ESP32
    participant WiFi
    participant MQTT
    
    Note over ESP32: System running, WiFi connected
    
    WiFi->>ESP32: WIFI_STATE_DISCONNECTED event
    activate ESP32
    ESP32->>ESP32: on_wifi_event(DISCONNECTED)
    ESP32->>ESP32: Log: WiFi disconnected
    ESP32->>ESP32: Set WiFi LED OFF
    
    ESP32->>ESP32: Check MQTT state
    
    alt MQTT was connected
        ESP32->>MQTT: MQTT_Handler_Stop
        activate MQTT
        MQTT->>MQTT: Disconnect from broker
        MQTT->>ESP32: Stopped
        deactivate MQTT
        
        ESP32->>ESP32: mqtt_current_state = false
        ESP32->>UART: Send "MQTT DISCONNECTED\n"
        activate UART
        UART->>STM32: UART RX
        deactivate UART
        
        activate STM32
        STM32->>STM32: mqtt_current_state = DISCONNECTED
        STM32->>STM32: Switch to SD buffering mode
        deactivate STM32
    end
    
    ESP32->>ESP32: Enter retry loop
    deactivate ESP32
    
    loop Retry attempts 1-5
        ESP32->>ESP32: Wait 2 seconds
        ESP32->>WiFi: WiFi_Connect
        activate WiFi
        WiFi->>WiFi: Scan and connect
        
        alt Connection successful
            WiFi->>ESP32: WIFI_STATE_CONNECTED
            ESP32->>ESP32: Mark wifi_reconnect_time
            ESP32->>ESP32: Set WiFi LED ON
            ESP32->>ESP32: Break retry loop
            Note over ESP32: Proceed to MQTT restart
        else Connection failed
            WiFi->>ESP32: WIFI_STATE_FAILED
            ESP32->>ESP32: Increment retry count
            Note over ESP32: Continue to next retry
        end
        deactivate WiFi
    end
    
    alt Max retries (5) exceeded
        ESP32->>ESP32: Enter manual retry mode
        loop Manual retry every 5s
            ESP32->>ESP32: Wait 5 seconds
            ESP32->>WiFi: WiFi_Connect
            WiFi->>ESP32: Result
            Note over ESP32: Continue until successful
        end
    end
    
    Note over ESP32: WiFi reconnected
    
    activate ESP32
    ESP32->>ESP32: Wait 4s for network stability
    ESP32->>MQTT: MQTT_Handler_Start
    activate MQTT
    MQTT->>MQTT: Connect to broker
    MQTT->>ESP32: MQTT_CONNECTED event
    deactivate MQTT
    
    ESP32->>ESP32: mqtt_current_state = true
    ESP32->>ESP32: Set mqtt_reconnected flag
    ESP32->>UART: Send "MQTT CONNECTED\n"
    activate UART
    UART->>STM32: UART RX
    deactivate UART
    
    activate STM32
    STM32->>STM32: mqtt_current_state = CONNECTED
    
    STM32->>STM32: Check for buffered data
    
    loop Transmit buffered data
        STM32->>STM32: SDCardManager_ReadData
        STM32->>UART: Send buffered JSON
        activate UART
        UART->>ESP32: Data received
        deactivate UART
        
        ESP32->>MQTT: Publish buffered data
        activate MQTT
        MQTT->>MQTT: Forward to subscribers
        deactivate MQTT
        
        STM32->>STM32: SDCardManager_RemoveRecord
        STM32->>STM32: Delay 100ms
    end
    
    STM32->>STM32: Resume normal operation
    deactivate STM32
    deactivate ESP32
```

## SD Card Buffer Transmission After Reconnection

```mermaid
sequenceDiagram
    participant STM32
    participant SD as SD Card
    participant UART
    participant ESP32
    participant MQTT
    participant Web
    
    Note over STM32: MQTT reconnected, has buffered data
    
    STM32->>STM32: Main loop iteration
    STM32->>STM32: Check: mqtt_current_state == CONNECTED?
    STM32->>STM32: Yes - check buffered data
    
    STM32->>SD: SDCardManager_HasData()
    activate SD
    SD->>STM32: true (records available)
    deactivate SD
    
    loop While buffered data exists
        STM32->>SD: SDCardManager_ReadData(&record)
        activate SD
        SD->>SD: Read from current read pointer
        SD->>STM32: record data
        deactivate SD
        
        STM32->>STM32: Validate record format
        
        alt Record valid
            STM32->>STM32: Format JSON from record
            Note right of STM32: {"mode":"PERIODIC","timestamp":1729699100,<br/>"temperature":25.5,"humidity":60.0}
            
            STM32->>UART: HAL_UART_Transmit(JSON)
            activate UART
            UART->>ESP32: UART RX data
            deactivate UART
            
            activate ESP32
            ESP32->>ESP32: Assemble line
            ESP32->>ESP32: JSON_Parser_ProcessLine
            ESP32->>ESP32: Detect mode (SINGLE or PERIODIC)
            
            ESP32->>MQTT: MQTT_Handler_Publish
            activate MQTT
            Note right of ESP32: Topic: datalogger/stm32/periodic/data<br/>or datalogger/stm32/single/data
            MQTT->>Web: Forward buffered data
            deactivate MQTT
            
            Web->>Web: Display data (marked as historical)
            deactivate ESP32
            
            STM32->>SD: SDCardManager_RemoveRecord()
            activate SD
            SD->>SD: Advance read pointer
            SD->>SD: Decrement record count
            SD->>STM32: Record removed
            deactivate SD
            
            STM32->>STM32: Update display: buffer count decreased
            STM32->>STM32: HAL_Delay(100) - rate limiting
            
        else Record corrupted
            STM32->>SD: SDCardManager_RemoveRecord()
            activate SD
            SD->>SD: Skip corrupted record
            SD->>STM32: Removed
            deactivate SD
            
            STM32->>STM32: Log: Corrupted record skipped
        end
        
        STM32->>SD: Check more data
        activate SD
        SD->>STM32: Has more / No more data
        deactivate SD
    end
    
    STM32->>STM32: All buffered data transmitted
    STM32->>STM32: Display: Buffer empty
    STM32->>STM32: Resume normal live transmission
    
    Note over STM32,Web: System back to real-time operation
```

## Error Recovery - Sensor Failure Detection

```mermaid
sequenceDiagram
    participant STM32
    participant SHT3X
    participant Display
    participant UART
    participant ESP32
    participant MQTT
    participant Web
    
    Note over STM32: Single/Periodic measurement requested
    
    STM32->>SHT3X: I2C Read measurement
    activate SHT3X
    
    alt I2C Timeout/NACK
        SHT3X-->>STM32: HAL_TIMEOUT / HAL_ERROR
        STM32->>STM32: Log: I2C communication failed
        STM32->>STM32: Set temp=0.0, hum=0.0
        
    else CRC Check Failed
        SHT3X->>STM32: Data received
        STM32->>STM32: Validate CRC
        STM32->>STM32: CRC mismatch detected
        STM32->>STM32: Set temp=0.0, hum=0.0
        
    else Sensor Disconnected
        SHT3X-->>STM32: No response
        STM32->>STM32: Set temp=0.0, hum=0.0
    end
    deactivate SHT3X
    
    STM32->>STM32: DataManager_Update (with 0.0 values)
    STM32->>Display: Show error indicator: temp=0.0, hum=0.0
    Display->>Display: Display shows "Sensor Error" or red values
    
    alt MQTT Connected
        STM32->>STM32: Format JSON with error values
        Note right of STM32: {"mode":"SINGLE","timestamp":1729699200,<br/>"temperature":0.0,"humidity":0.0}
        
        STM32->>UART: Transmit error JSON
        UART->>ESP32: Data received
        
        activate ESP32
        ESP32->>ESP32: Parse JSON
        ESP32->>ESP32: JSON_Parser_IsSensorFailed() returns true
        ESP32->>ESP32: Log: Sensor failure detected
        
        ESP32->>MQTT: Publish error data
        MQTT->>Web: Forward
        
        Web->>Web: Display error state
        Web->>Web: Show alert: "Sensor communication error"
        deactivate ESP32
        
    else MQTT Disconnected
        STM32->>STM32: Buffer error data to SD
        Note over STM32: Error records also buffered
    end
    
    Note over STM32: System continues operation
    
    STM32->>STM32: Next measurement cycle
    STM32->>SHT3X: Retry I2C communication
    
    alt Sensor Recovered
        activate SHT3X
        SHT3X->>STM32: Valid data
        deactivate SHT3X
        STM32->>STM32: Parse valid temp & humidity
        STM32->>Display: Clear error indicator
        STM32->>STM32: Resume normal operation
        Note over STM32,Web: System automatically recovered
        
    else Sensor Still Failed
        STM32->>STM32: Continue with 0.0 values
        Note over STM32: Retry on each measurement
    end
```

---

## Key Sequence Patterns

### Communication Protocol
- **UART**: Line-based, newline-terminated, 115200 baud
- **Commands**: Plain text (SINGLE, PERIODIC ON/OFF, MQTT CONNECTED/DISCONNECTED, etc.)
- **Data**: JSON format with mode, timestamp, sensor values

### Timing Sequences
- **WiFi Stabilization**: 4-second wait before MQTT start
- **STM32 Boot**: 500ms delay after relay ON before sending MQTT state
- **Debouncing**: 200ms for button presses
- **Rate Limiting**: 100ms between buffered data transmissions
- **Retry Delays**: 2s for WiFi (auto), 5s (manual), exponential for MQTT

### State Synchronization
- **Bidirectional**: ESP32 ↔ STM32 via UART commands
- **MQTT Retained**: State published with retain flag for web sync
- **Callback Chains**: Event → Callback → State Update → Publish

### Error Handling
- **Graceful Degradation**: Use 0.0 values for failed sensors
- **Automatic Retry**: WiFi and MQTT reconnection logic
- **Buffer Fallback**: SD card buffering when MQTT unavailable
- **Recovery Detection**: Automatic resume on sensor/connection recovery
