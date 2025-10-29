# ESP32 IoT Bridge - Sequence Diagrams

This document illustrates the time-ordered interactions between components in the ESP32 firmware.

## System Initialization Sequence

```mermaid
sequenceDiagram
    participant PWR as Power Supply
    participant ESP32
    participant NVS as NVS Flash
    participant WiFi as WiFi Manager
    participant UART as STM32 UART
    participant LED as LED GPIOs
    participant MQTT as MQTT Handler
    participant Relay
    participant Parser as JSON Parser
    participant Button as Button Handler
    
    PWR->>ESP32: Power ON
    activate ESP32
    
    ESP32->>NVS: nvs_flash_init()
    activate NVS
    NVS->>ESP32: OK
    deactivate NVS
    
    ESP32->>ESP32: esp_event_loop_create_default()
    ESP32->>ESP32: esp_netif_init()
    
    ESP32->>WiFi: WiFi_Manager_Init()
    activate WiFi
    WiFi->>WiFi: Configure SSID, password
    WiFi->>WiFi: Register event handlers
    WiFi->>ESP32: Init complete
    deactivate WiFi
    
    ESP32->>UART: STM32_UART_Init(&stm32_uart)
    activate UART
    UART->>UART: Configure UART1: 115200 baud
    UART->>UART: Allocate ring buffer (512 bytes)
    UART->>UART: Register on_stm32_data_received callback
    UART->>ESP32: Init complete
    deactivate UART
    
    ESP32->>LED: gpio_set_direction(WiFi LED)
    ESP32->>LED: gpio_set_direction(MQTT LED)
    
    ESP32->>WiFi: WiFi_Manager_Connect_Blocking(15s timeout)
    activate WiFi
    WiFi->>WiFi: Start connection process
    
    alt WiFi connected within 15s
        WiFi->>ESP32: WIFI_EVENT_STA_GOT_IP
        ESP32->>ESP32: Mark reconnect time (for 4s delay)
        ESP32->>ESP32: Log: "WiFi connected successfully!"
    else Timeout after 15s
        WiFi->>ESP32: Timeout
        ESP32->>ESP32: Log: "Will retry in background"
    end
    deactivate WiFi
    
    ESP32->>MQTT: MQTT_Handler_Init(&mqtt_handler)
    activate MQTT
    MQTT->>MQTT: Configure broker URL, credentials
    MQTT->>MQTT: Register on_mqtt_data_received callback
    MQTT->>MQTT: Generate unique client ID
    MQTT->>ESP32: Init complete
    deactivate MQTT
    
    ESP32->>Relay: Relay_Init(&relay_control, GPIO_4)
    activate Relay
    Relay->>Relay: Configure GPIO output
    Relay->>Relay: Register on_relay_state_changed callback
    Relay->>Relay: Read initial hardware state
    Relay->>ESP32: Init complete (state=OFF)
    deactivate Relay
    
    ESP32->>Parser: JSON_Parser_Init(&json_parser)
    activate Parser
    Parser->>Parser: Register on_single_sensor_data callback
    Parser->>Parser: Register on_periodic_sensor_data callback
    Parser->>ESP32: Init complete
    deactivate Parser
    
    ESP32->>Button: Button_Init(GPIO_5, relay_callback)
    ESP32->>Button: Button_Init(GPIO_17, single_callback)
    ESP32->>Button: Button_Init(GPIO_16, periodic_callback)
    ESP32->>Button: Button_Init(GPIO_4, interval_callback)
    activate Button
    Button->>Button: Configure GPIOs with pull-up
    Button->>Button: Attach ISR handlers
    Button->>ESP32: All buttons initialized
    deactivate Button
    
    ESP32->>ESP32: Set g_device_on = Relay state
    ESP32->>ESP32: Set g_periodic_active = false
    
    ESP32->>Button: Button_StartTask()
    activate Button
    Button->>Button: Create FreeRTOS task for debouncing
    deactivate Button
    
    ESP32->>UART: STM32_UART_StartTask()
    activate UART
    UART->>UART: Create FreeRTOS RX task
    deactivate UART
    
    ESP32->>ESP32: Enter main monitoring loop
    deactivate ESP32
```

## WiFi Connection and MQTT Start Sequence

```mermaid
sequenceDiagram
    participant Main as Main Loop
    participant WiFi as WiFi Manager
    participant Timer as ESP Timer
    participant MQTT as MQTT Handler
    participant STM32 as STM32 UART
    participant LED
    
    Note over Main: System running, WiFi disconnected
    
    Main->>WiFi: Check wifi_state
    activate WiFi
    WiFi->>Main: WIFI_STATE_FAILED
    deactivate WiFi
    
    Main->>Timer: Get current time
    Timer->>Main: now_ms
    
    Main->>Main: Check if 5s passed since last retry
    
    alt 5s passed
        Main->>WiFi: WiFi_Manager_Connect()
        activate WiFi
        WiFi->>WiFi: Attempt connection (5 auto-retries)
        
        Note over WiFi: Background connection process
        
        WiFi->>Main: on_wifi_event(WIFI_STATE_CONNECTING)
        Main->>LED: Set WiFi LED OFF
        Main->>Main: Log "WiFi: Connecting..."
        
        alt Connection successful
            WiFi->>Main: on_wifi_event(WIFI_STATE_CONNECTED)
            activate Main
            Main->>LED: Set WiFi LED ON
            Main->>WiFi: wifi_manager_get_ip_addr()
            WiFi->>Main: "192.168.1.100"
            Main->>WiFi: wifi_manager_get_rssi()
            WiFi->>Main: -45 dBm
            Main->>Main: Log IP and RSSI
            Main->>Timer: Mark reconnect time
            Timer->>Main: g_wifi_reconnect_time_ms = now
            Main->>Main: Reset disconnect flag
            deactivate Main
        end
        deactivate WiFi
    end
    
    Note over Main: Wait 4 seconds for network stability
    
    loop Check every 100ms
        Main->>Timer: Get current time
        Timer->>Main: now_ms
        Main->>Main: Calculate elapsed = now - reconnect_time
        
        alt elapsed >= 4000ms
            Main->>MQTT: MQTT_Handler_Start(&mqtt_handler)
            activate MQTT
            MQTT->>MQTT: esp_mqtt_client_start()
            MQTT->>Main: Started
            deactivate MQTT
            
            Main->>Main: Set g_mqtt_started = true
            Main->>Main: Clear reconnect timer
            Main->>Main: Log "MQTT starting..."
            
            MQTT->>Main: on_mqtt_event(MQTT_EVENT_CONNECTED)
            activate Main
            Main->>Main: Set mqtt_handler.connected = true
            Main->>MQTT: Subscribe to command topics
            activate MQTT
            MQTT->>MQTT: Subscribe to datalogger/stm32/command
            MQTT->>MQTT: Subscribe to datalogger/esp32/relay/control
            MQTT->>MQTT: Subscribe to datalogger/esp32/system/state
            MQTT->>Main: Subscribed
            deactivate MQTT
            
            Main->>Main: Set g_mqtt_reconnected = true
            Main->>LED: Set MQTT LED ON
            
            Main->>STM32: "MQTT CONNECTED\n"
            activate STM32
            STM32->>STM32: Process command
            STM32->>STM32: Update mqtt_current_state = CONNECTED
            deactivate STM32
            
            Main->>Main: Log "MQTT Connected"
            deactivate Main
        end
    end
```

## Sensor Data Reception and Publishing Sequence

```mermaid
sequenceDiagram
    participant STM32
    participant UART as STM32 UART
    participant RingBuf as Ring Buffer
    participant Task as UART RX Task
    participant Parser as JSON Parser
    participant Callback as Sensor Callback
    participant JSON as JSON Utils
    participant MQTT as MQTT Handler
    participant Broker as MQTT Broker
    
    STM32->>UART: Transmit JSON: {"mode":"PERIODIC","timestamp":1729699200,"temperature":25.50,"humidity":60.00}\n
    activate UART
    UART->>UART: UART ISR receives bytes
    UART->>RingBuf: Write bytes to ring buffer
    deactivate UART
    
    Note over RingBuf: Accumulate until '\n'
    
    Task->>RingBuf: Check for complete line
    activate Task
    RingBuf->>Task: Complete line available
    Task->>Task: Extract line from buffer
    Task->>Callback: on_stm32_data_received(line)
    deactivate Task
    
    activate Callback
    Callback->>Parser: JSON_Parser_ProcessLine(&json_parser, line)
    deactivate Callback
    
    activate Parser
    Parser->>Parser: Parse JSON with cJSON
    Parser->>Parser: Validate required fields
    Parser->>Parser: Extract: mode, timestamp, temperature, humidity
    
    alt JSON valid
        Parser->>Parser: Check mode field
        
        alt mode == "PERIODIC"
            Parser->>Callback: on_periodic_sensor_data(&sensor_data)
            activate Callback
            
            Callback->>JSON: JSON_Utils_CreateSensorData(buffer, "PERIODIC", timestamp, temp, hum)
            activate JSON
            JSON->>JSON: Format JSON string
            JSON->>Callback: Return JSON string
            deactivate JSON
            
            Callback->>MQTT: MQTT_Handler_Publish(&mqtt_handler, "datalogger/stm32/periodic/data", json, qos=0, retain=0)
            activate MQTT
            
            alt MQTT connected
                MQTT->>Broker: Publish message
                Broker->>Broker: Distribute to subscribers
            else MQTT disconnected
                MQTT->>MQTT: Log "Not connected, message lost"
            end
            deactivate MQTT
            
            Callback->>Callback: Log sensor values
            deactivate Callback
        else mode == "SINGLE"
            Parser->>Callback: on_single_sensor_data(&sensor_data)
            activate Callback
            
            Callback->>JSON: JSON_Utils_CreateSensorData(buffer, "SINGLE", timestamp, temp, hum)
            activate JSON
            JSON->>Callback: Return JSON string
            deactivate JSON
            
            Callback->>MQTT: MQTT_Handler_Publish(&mqtt_handler, "datalogger/stm32/single/data", json, qos=0, retain=0)
            activate MQTT
            MQTT->>Broker: Publish message
            deactivate MQTT
            
            Callback->>Callback: Log sensor values
            deactivate Callback
        end
    else JSON invalid
        Parser->>Parser: Log parse error
    end
    deactivate Parser
```

## MQTT Command Reception and Forwarding Sequence

```mermaid
sequenceDiagram
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant MQTT as MQTT Handler
    participant Callback as on_mqtt_data_received
    participant UART as STM32 UART
    participant STM32
    participant State as State Manager
    
    Web->>Broker: Publish "PERIODIC ON" to datalogger/stm32/command
    activate Broker
    Broker->>MQTT: Forward message
    deactivate Broker
    
    activate MQTT
    MQTT->>Callback: on_mqtt_data_received("datalogger/stm32/command", "PERIODIC ON", 11)
    deactivate MQTT
    
    activate Callback
    Callback->>Callback: Check topic == TOPIC_STM32_COMMAND
    
    Callback->>UART: STM32_UART_SendCommand(&stm32_uart, "PERIODIC ON")
    activate UART
    UART->>UART: Append '\n' to command
    UART->>STM32: Transmit "PERIODIC ON\n"
    activate STM32
    STM32->>STM32: Process command
    STM32->>STM32: Start periodic measurement @ 1Hz
    deactivate STM32
    UART->>Callback: Success
    deactivate UART
    
    Callback->>Callback: Check command == "PERIODIC ON"
    Callback->>State: update_and_publish_state(g_device_on, true)
    activate State
    State->>State: Set g_periodic_active = true
    State->>State: state_changed = true
    State->>State: Format JSON state message
    State->>MQTT: MQTT_Handler_Publish("datalogger/esp32/system/state", json, retain=1)
    activate MQTT
    MQTT->>Broker: Publish state update
    activate Broker
    Broker->>Web: Forward state to subscribers
    deactivate Broker
    deactivate MQTT
    State->>State: Log state change
    deactivate State
    
    deactivate Callback
    
    Note over Web: Web dashboard receives state update
```

## Relay Control via MQTT Sequence

```mermaid
sequenceDiagram
    participant Web as Web Dashboard
    participant Broker as MQTT Broker
    participant MQTT as MQTT Handler
    participant Callback as on_mqtt_data_received
    participant Relay as Relay Control
    participant GPIO as GPIO Hardware
    participant RelayCallback as on_relay_state_changed
    participant State as State Manager
    participant STM32 as STM32 UART
    
    Web->>Broker: Publish "ON" to datalogger/esp32/relay/control
    Broker->>MQTT: Forward message
    
    MQTT->>Callback: on_mqtt_data_received("datalogger/esp32/relay/control", "ON", 2)
    activate Callback
    
    Callback->>Callback: Check topic == TOPIC_RELAY_CONTROL
    Callback->>Relay: Relay_ProcessCommand(&relay_control, "ON")
    deactivate Callback
    
    activate Relay
    Relay->>Relay: Parse command = "ON"
    Relay->>Relay: Relay_SetState(&relay_control, true)
    Relay->>GPIO: gpio_set_level(GPIO_4, 1)
    activate GPIO
    GPIO->>GPIO: Set pin HIGH
    GPIO->>Relay: Done
    deactivate GPIO
    
    Relay->>Relay: Update relay_control.state = true
    Relay->>RelayCallback: on_relay_state_changed(true)
    deactivate Relay
    
    activate RelayCallback
    RelayCallback->>RelayCallback: Log "Relay: ON"
    RelayCallback->>RelayCallback: new_periodic_state = g_periodic_active (unchanged)
    RelayCallback->>State: update_and_publish_state(true, g_periodic_active)
    activate State
    State->>State: Check if states changed
    State->>State: g_device_on = true (changed)
    State->>State: Format JSON state
    State->>MQTT: Publish state to datalogger/esp32/system/state
    activate MQTT
    MQTT->>Broker: Publish with retain flag
    deactivate MQTT
    deactivate State
    
    RelayCallback->>RelayCallback: Wait 500ms (STM32 boot time)
    
    RelayCallback->>MQTT: Check MQTT_Handler_IsConnected()
    MQTT->>RelayCallback: true
    
    RelayCallback->>STM32: "MQTT CONNECTED\n"
    activate STM32
    STM32->>STM32: Update mqtt_current_state
    deactivate STM32
    
    RelayCallback->>RelayCallback: Log "TX STM32: MQTT CONNECTED (relay toggled)"
    deactivate RelayCallback
```

## Button Press Sequence (Periodic Toggle)

```mermaid
sequenceDiagram
    participant User
    participant GPIO as GPIO Hardware
    participant ISR as GPIO ISR
    participant Task as Button Task
    participant Callback as Button Callback
    participant UART as STM32 UART
    participant State as State Manager
    participant MQTT
    
    User->>GPIO: Press button GPIO_16
    GPIO->>ISR: GPIO interrupt triggered
    activate ISR
    ISR->>ISR: Disable interrupt temporarily
    ISR->>Task: Notify button task via queue
    deactivate ISR
    
    activate Task
    Task->>Task: Debounce: wait 50ms
    Task->>GPIO: Read GPIO level
    GPIO->>Task: Level LOW (pressed)
    Task->>Task: Wait for release
    Task->>GPIO: Poll GPIO level
    GPIO->>Task: Level HIGH (released)
    Task->>Callback: on_button_periodic_pressed(GPIO_16)
    Task->>ISR: Re-enable interrupt
    deactivate Task
    
    activate Callback
    Callback->>Callback: Check g_device_on == true
    
    alt Device ON
        Callback->>Callback: Calculate new_periodic_state = !g_periodic_active
        Callback->>Callback: new_periodic_state = true
        Callback->>Callback: Format command = "PERIODIC ON"
        
        Callback->>UART: STM32_UART_SendCommand(&stm32_uart, "PERIODIC ON")
        activate UART
        UART->>UART: Transmit "PERIODIC ON\n"
        deactivate UART
        
        Callback->>State: update_and_publish_state(g_device_on, true)
        activate State
        State->>State: g_periodic_active = true (changed)
        State->>State: state_changed = true
        State->>State: Format JSON: {"device":"ON","periodic":"ON"}
        State->>MQTT: MQTT_Handler_Publish("datalogger/esp32/system/state", json, retain=1)
        State->>State: Log "Periodic state changed: ON"
        State->>State: Log "State published"
        deactivate State
    else Device OFF
        Callback->>Callback: Log "Button: PERIODIC ignored (device OFF)"
    end
    
    deactivate Callback
```

## WiFi Disconnection and Reconnection Sequence

```mermaid
sequenceDiagram
    participant Main as Main Loop
    participant WiFi as WiFi Manager
    participant MQTT as MQTT Handler
    participant LED
    participant STM32 as STM32 UART
    participant Timer
    
    Note over WiFi: WiFi connection lost (router reboot, signal loss, etc.)
    
    WiFi->>Main: on_wifi_event(WIFI_STATE_DISCONNECTED)
    activate Main
    Main->>LED: Set WiFi LED OFF
    Main->>Main: Log "WiFi: Disconnected"
    deactivate Main
    
    Note over Main: Main loop detects WiFi state change
    
    Main->>WiFi: Check wifi_manager_is_connected()
    WiFi->>Main: false
    
    Main->>Main: Detect WiFi changed from connected to disconnected
    Main->>MQTT: MQTT_Handler_Stop(&mqtt_handler)
    activate MQTT
    MQTT->>MQTT: esp_mqtt_client_stop()
    MQTT->>MQTT: Set connected = false
    MQTT->>Main: Stopped
    deactivate MQTT
    
    Main->>LED: Set MQTT LED OFF
    Main->>Main: Clear g_wifi_reconnect_time_ms
    Main->>Main: Log "Stopping MQTT (WiFi lost)"
    
    Note over WiFi: WiFi Manager auto-retry (5 attempts, 2s interval)
    
    loop Auto-retry attempts (up to 5)
        WiFi->>WiFi: Attempt reconnection
        
        alt Reconnection successful
            WiFi->>Main: on_wifi_event(WIFI_STATE_CONNECTED)
            activate Main
            Main->>LED: Set WiFi LED ON
            Main->>Main: Log "WiFi restored, network stabilizing..."
            Main->>Timer: Mark reconnect time
            Main->>Main: g_wifi_reconnect_time_ms = now
            deactivate Main
            
            Note over Main: Wait 4 seconds for network stability
            
            Main->>Main: Check elapsed time >= 4000ms
            Main->>MQTT: MQTT_Handler_Start(&mqtt_handler)
            activate MQTT
            MQTT->>MQTT: esp_mqtt_client_start()
            MQTT->>Main: Started
            deactivate MQTT
            
            Main->>Main: Log "Starting MQTT (network stable)"
            
            Note over MQTT: MQTT reconnects to broker
            
            MQTT->>Main: on_mqtt_event(MQTT_EVENT_CONNECTED)
            Main->>LED: Set MQTT LED ON
            
            Main->>STM32: "MQTT CONNECTED\n"
            activate STM32
            STM32->>STM32: Update state, resume normal operation
            deactivate STM32
            
            Main->>Main: Log "MQTT Connected"
        end
    end
    
    alt All auto-retries failed
        WiFi->>Main: on_wifi_event(WIFI_STATE_FAILED)
        Main->>Main: Log "WiFi: Failed (all retries exhausted)"
        
        Note over Main: Manual retry every 5 seconds
        
        loop Every 5 seconds
            Main->>Timer: Check if 5s passed
            Timer->>Main: true
            Main->>WiFi: WiFi_Manager_Connect()
            WiFi->>WiFi: Restart connection process
        end
    end
```

## MQTT Reconnection with State Sync Sequence

```mermaid
sequenceDiagram
    participant MQTT as MQTT Handler
    participant Main as Main Loop
    participant State as State Manager
    participant Broker as MQTT Broker
    participant Web as Web Dashboard
    
    Note over MQTT: MQTT disconnected (broker restart, network glitch)
    
    MQTT->>Main: on_mqtt_event(MQTT_EVENT_DISCONNECTED)
    activate Main
    Main->>Main: Set mqtt_handler.connected = false
    Main->>Main: Increment retry_count
    Main->>Main: Calculate backoff = min(60s, 2^retry * 1s)
    Main->>Main: Log "MQTT disconnected, retry in {backoff}s"
    deactivate Main
    
    Note over MQTT: Wait backoff period
    
    MQTT->>MQTT: Automatic reconnection attempt
    
    alt Reconnection successful
        MQTT->>Main: on_mqtt_event(MQTT_EVENT_CONNECTED)
        activate Main
        Main->>Main: Set mqtt_handler.connected = true
        Main->>Main: Reset retry_count = 0
        Main->>Main: Set g_mqtt_reconnected = true
        
        Main->>MQTT: Subscribe to command topics
        activate MQTT
        MQTT->>Broker: SUBSCRIBE datalogger/stm32/command
        MQTT->>Broker: SUBSCRIBE datalogger/esp32/relay/control
        MQTT->>Broker: SUBSCRIBE datalogger/esp32/system/state
        Broker->>MQTT: SUBACK
        deactivate MQTT
        
        Main->>Main: Log "MQTT reconnected"
        deactivate Main
        
        Note over Web: Web dashboard detects MQTT reconnection
        
        Web->>Broker: Publish "REQUEST" to datalogger/esp32/system/state
        Broker->>MQTT: Forward state sync request
        
        MQTT->>Main: on_mqtt_data_received("datalogger/esp32/system/state", "REQUEST", 7)
        activate Main
        Main->>Main: Check g_mqtt_reconnected == true
        
        Main->>State: publish_current_state()
        activate State
        State->>State: Format JSON: {"device":"ON","periodic":"ON"}
        State->>MQTT: MQTT_Handler_Publish("datalogger/esp32/system/state", json, retain=1)
        activate MQTT
        MQTT->>Broker: Publish state
        Broker->>Web: Forward to subscribers
        deactivate MQTT
        State->>State: Log "State published"
        deactivate State
        
        Main->>Main: Set g_mqtt_reconnected = false
        Main->>Main: Log "State sync completed"
        deactivate Main
        
        Note over Web: Web dashboard updates UI with current state
    end
```

---

**Key Sequence Characteristics:**

1. **Asynchronous Communication**: UART RX, MQTT events, and button presses all use interrupt-driven, non-blocking I/O with FreeRTOS queues
2. **State Synchronization**: ESP32 maintains authoritative state and publishes to MQTT with retain flag for persistence
3. **Boot Delay Handling**: 500ms delay after relay toggle ensures STM32 receives MQTT status after reboot
4. **Network Stability**: 4-second delay after WiFi connection before starting MQTT ensures network stack is fully ready
5. **Exponential Backoff**: MQTT retries use exponential backoff (min 1s, max 60s) to avoid overwhelming broker
6. **Button Debouncing**: Hardware and software debouncing (50ms) prevents spurious button presses
7. **Ring Buffer**: UART uses 512-byte ring buffer for reliable byte-level reception
8. **Callback Chain**: Data flows through callbacks: Hardware ISR → Task → Parser → Sensor Callback → MQTT Publish
