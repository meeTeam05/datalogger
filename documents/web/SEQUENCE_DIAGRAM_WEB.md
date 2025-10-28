# Web Application - Sequence Diagrams

Comprehensive sequence diagrams documenting time-ordered interactions in the DataLogger web application.

---

## 1. Complete Application Initialization

```mermaid
sequenceDiagram
    participant Browser
    participant HTML
    participant AppJS as app.js
    participant Firebase
    participant MQTT as MQTT Broker
    participant ESP32
    
    Browser->>HTML: Load index.html
    HTML->>Browser: DOM Ready
    Browser->>AppJS: Execute app.js
    
    Note over AppJS: Initialize Feather Icons
    AppJS->>AppJS: feather.replace()
    
    Note over AppJS: Initialize Global Variables
    AppJS->>AppJS: isPeriodic = false<br/>isDeviceOn = false<br/>temperatureData = []<br/>humidityData = []
    
    rect rgb(245, 158, 11, 0.1)
        Note over AppJS,Firebase: Firebase Initialization
        AppJS->>Firebase: firebase.initializeApp(FIREBASE_CONFIG)
        Firebase-->>AppJS: App initialized
        
        AppJS->>Firebase: Get database reference
        Firebase-->>AppJS: Database ref
        
        AppJS->>Firebase: Listen .info/connected
        Firebase-->>AppJS: Connection status
        
        AppJS->>Firebase: Test write permissions<br/>test/connection
        
        alt Write Success
            Firebase-->>AppJS: Write OK
            AppJS->>AppJS: updateFirebaseStatus(true)<br/>Badge: connected
            AppJS->>Firebase: Remove test data
        else Write Failed
            Firebase-->>AppJS: Permission denied
            AppJS->>AppJS: updateFirebaseStatus(false)<br/>Log permission error
        end
    end
    
    rect rgb(139, 92, 246, 0.1)
        Note over AppJS,MQTT: MQTT Initialization
        AppJS->>MQTT: mqtt.connect(ws://127.0.0.1:8083/mqtt)
        
        Note over MQTT: Connecting...
        MQTT-->>AppJS: connect event
        
        AppJS->>AppJS: updateMQTTStatus(true)<br/>Badge: connected
        
        AppJS->>MQTT: Subscribe(systemState, qos=1)
        MQTT-->>AppJS: Subscribed
        
        AppJS->>MQTT: Subscribe(singleData, qos=1)
        MQTT-->>AppJS: Subscribed
        
        AppJS->>MQTT: Subscribe(periodicData, qos=1)
        MQTT-->>AppJS: Subscribed
        
        Note over AppJS: Wait 500ms
        AppJS->>MQTT: Publish(systemState, "REQUEST", qos=1)
        
        MQTT->>ESP32: Forward REQUEST
        ESP32->>ESP32: Get current state
        ESP32->>MQTT: Publish state<br/>{"device":"ON","periodic":"OFF"}
        MQTT->>AppJS: Forward state
        
        AppJS->>AppJS: syncUIWithHardwareState()<br/>Update button colors
    end
    
    rect rgb(6, 182, 212, 0.1)
        Note over AppJS: Chart Initialization
        AppJS->>HTML: Get canvas elements<br/>#tempChart, #humiChart
        
        alt Canvas Found
            HTML-->>AppJS: Canvas contexts
            AppJS->>AppJS: new Chart(tempCtx, config)
            AppJS->>AppJS: new Chart(humiCtx, config)
            AppJS->>AppJS: addStatus("Charts initialized")
        else Canvas Not Found
            HTML-->>AppJS: null
            AppJS->>AppJS: Retry after 500ms<br/>(max 10 attempts)
        end
    end
    
    Note over AppJS: Bind Event Listeners
    AppJS->>HTML: Bind menu navigation
    AppJS->>HTML: Bind quick action buttons
    AppJS->>AppJS: Start clock ticker (1s interval)
    
    AppJS->>Browser: Ready for user interaction
```

---

## 2. User Sends Single Read Command

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant MQTT as MQTT Broker
    participant ESP32
    participant STM32
    participant SHT31
    
    User->>Browser: Click "Single Read" button
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Check isMqttConnected
    
    alt MQTT Not Connected
        AppJS->>User: Show error<br/>"MQTT not connected"
    else MQTT Connected
        AppJS->>AppJS: Check isDeviceOn
        
        alt Device OFF
            AppJS->>User: Show warning<br/>"Device must be ON first"
        else Device ON
            AppJS->>MQTT: Publish(stm32Command,<br/>"SINGLE", qos=1)
            AppJS->>AppJS: addStatus("Single read command sent")
            
            MQTT->>ESP32: Forward "SINGLE"
            ESP32->>STM32: UART transmit<br/>{"cmd":"SINGLE"}
            
            Note over STM32: Process command
            STM32->>SHT31: I2C read request
            SHT31-->>STM32: Temperature & Humidity
            
            STM32->>STM32: Format JSON<br/>{"temperature":X,"humidity":Y,"timestamp":Z}
            STM32->>ESP32: UART transmit JSON
            
            ESP32->>MQTT: Publish(singleData,<br/>JSON, qos=1)
            MQTT->>AppJS: Forward JSON
            
            AppJS->>AppJS: Parse JSON
            AppJS->>AppJS: Check sensor values
            
            alt Sensor Failed (temp=0, humi=0)
                AppJS->>AppJS: Log ERROR "SHT31 sensor failed"
                AppJS->>AppJS: updateComponentHealth(SHT31, false)
            else Sensor OK
                AppJS->>AppJS: currentTemp = data.temp<br/>currentHumi = data.humi
                AppJS->>Browser: Update #currentTemp, #currentHumi
                AppJS->>AppJS: updateComponentHealth(SHT31, true)
            end
            
            alt Firebase Connected
                AppJS->>Firebase: Save record<br/>readings/date/id
                Firebase-->>AppJS: Write complete
            end
            
            AppJS->>AppJS: addToLiveDataTable()
            AppJS->>Browser: Update live data table
            
            AppJS->>User: Display updated values
        end
    end
```

---

## 3. User Toggles Periodic Mode

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant MQTT as MQTT Broker
    participant ESP32
    participant STM32
    
    User->>Browser: Click "Periodic" button
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Check isMqttConnected
    
    alt MQTT Not Connected
        AppJS->>User: Show error<br/>"MQTT not connected"
    else MQTT Connected
        AppJS->>AppJS: Check isDeviceOn
        
        alt Device OFF
            AppJS->>User: Show warning<br/>"Device must be ON first"
        else Device ON
            AppJS->>AppJS: Toggle isPeriodic<br/>isPeriodic = !isPeriodic
            
            alt Enabling Periodic
                AppJS->>MQTT: Publish(stm32Command,<br/>"PERIODIC ON", qos=1)
                AppJS->>Browser: Update button<br/>className = "btn-success"<br/>text = "Periodic ON"
                
                MQTT->>ESP32: Forward "PERIODIC ON"
                ESP32->>STM32: UART transmit<br/>{"cmd":"PERIODIC ON"}
                
                Note over STM32: Start periodic timer
                STM32->>STM32: Enable interrupt<br/>Start measurements
                
                loop Every Interval (default 5s)
                    STM32->>SHT31: I2C read
                    SHT31-->>STM32: Data
                    STM32->>ESP32: UART send JSON
                    ESP32->>MQTT: Publish(periodicData)
                    MQTT->>AppJS: Forward data
                    AppJS->>Browser: Update charts
                end
                
            else Disabling Periodic
                AppJS->>MQTT: Publish(stm32Command,<br/>"PERIODIC OFF", qos=1)
                AppJS->>Browser: Update button<br/>className = "btn-secondary"<br/>text = "Periodic OFF"
                
                MQTT->>ESP32: Forward "PERIODIC OFF"
                ESP32->>STM32: UART transmit<br/>{"cmd":"PERIODIC OFF"}
                
                Note over STM32: Stop periodic timer
                STM32->>STM32: Disable interrupt<br/>Stop measurements
            end
            
            AppJS->>AppJS: addStatus("Periodic X command sent")
            AppJS->>User: Visual feedback
        end
    end
```

---

## 4. Periodic Sensor Data Arrives

```mermaid
sequenceDiagram
    participant STM32
    participant ESP32
    participant MQTT as MQTT Broker
    participant AppJS as app.js
    participant Firebase
    participant Browser
    
    Note over STM32: Timer interrupt fires
    STM32->>SHT31: I2C read temperature & humidity
    SHT31-->>STM32: Raw sensor data
    
    STM32->>DS3231: I2C read RTC timestamp
    DS3231-->>STM32: Unix timestamp
    
    STM32->>STM32: Format JSON<br/>{"temperature":25.3,"humidity":65.2,"timestamp":1704067200}
    
    STM32->>ESP32: UART transmit JSON
    ESP32->>ESP32: Validate JSON
    
    ESP32->>MQTT: Publish(periodicData,<br/>JSON, qos=1)
    MQTT->>AppJS: Forward message
    
    AppJS->>AppJS: Check topic == periodicData
    AppJS->>AppJS: Parse JSON
    
    AppJS->>AppJS: Check isDeviceOn && isPeriodic
    
    alt Device OFF or Periodic OFF
        AppJS->>AppJS: Log "Ignored periodic (inactive)"<br/>Return early
    else Active Measurement
        AppJS->>AppJS: Check sensor values
        
        alt Sensor Failed (temp=0, humi=0)
            AppJS->>AppJS: Log ERROR "SHT31 sensor failed"
            AppJS->>Browser: Show error badge<br/>in component card
            AppJS->>AppJS: status = "error"
        else Sensor OK
            AppJS->>AppJS: currentTemp = data.temp<br/>currentHumi = data.humi<br/>lastReadingTimestamp = timestamp
            AppJS->>Browser: Update #currentTemp<br/>#currentHumi<br/>#lastUpdate
            AppJS->>AppJS: status = "success"
        end
        
        AppJS->>AppJS: Check RTC timestamp
        
        alt RTC Failed (timestamp=0)
            AppJS->>AppJS: Log ERROR "DS3231 RTC failed"
            AppJS->>AppJS: Use local browser time
        end
        
        rect rgb(245, 158, 11, 0.1)
            Note over AppJS,Firebase: Save to Firebase
            AppJS->>Firebase: Save record<br/>readings/2025-01-20/1704067200<br/>{temp, humi, mode:"periodic",<br/>sensor:"SHT31", status, time, device}
            Firebase-->>AppJS: Write complete
            AppJS->>AppJS: addStatus("Firebase saved")
        end
        
        rect rgb(6, 182, 212, 0.1)
            Note over AppJS,Browser: Update Charts
            AppJS->>AppJS: pushTemperature(data.temp, true, timestamp)
            AppJS->>AppJS: temperatureData.push({value, time})
            AppJS->>AppJS: Check length > maxDataPoints
            
            alt Exceeds Max
                AppJS->>AppJS: temperatureData.shift()
            end
            
            AppJS->>AppJS: tempChart.data.labels = times
            AppJS->>AppJS: tempChart.data.datasets[0].data = values
            AppJS->>AppJS: tempChart.update('active')
            
            AppJS->>AppJS: pushHumidity(data.humi, true, timestamp)
            AppJS->>AppJS: humiChart.update('active')
            
            AppJS->>AppJS: Calculate statistics<br/>min, max, avg
            AppJS->>Browser: Update #tempMin, #tempMax, #tempAvg<br/>#humiMin, #humiMax, #humiAvg
        end
        
        AppJS->>AppJS: updateComponentHealth(SHT31, !sensorFailed)
        AppJS->>AppJS: updateComponentHealth(DS3231, !rtcFailed)
        AppJS->>Browser: Update component cards
        
        AppJS->>AppJS: addToLiveDataTable({time, temp, humi, mode, status})
        AppJS->>Browser: Render live data table
    end
```

---

## 5. Relay Control (Device ON/OFF)

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant MQTT as MQTT Broker
    participant ESP32
    participant Relay
    
    User->>Browser: Click "Device" button
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Check isMqttConnected
    
    alt MQTT Not Connected
        AppJS->>User: Show error "MQTT not connected"
    else MQTT Connected
        AppJS->>AppJS: Toggle isDeviceOn<br/>isDeviceOn = !isDeviceOn
        
        alt Turning ON
            AppJS->>AppJS: command = "RELAY ON"
            AppJS->>MQTT: Publish(relayControl,<br/>"RELAY ON", qos=1)
            
            MQTT->>ESP32: Forward command
            ESP32->>ESP32: Parse command
            ESP32->>Relay: GPIO write HIGH
            Relay->>Relay: Switch ON
            
            AppJS->>Browser: Update button<br/>className = "btn-success"<br/>text = "Device ON"
            AppJS->>AppJS: addStatus("Device RELAY ON sent")
            
        else Turning OFF
            AppJS->>AppJS: command = "RELAY OFF"
            AppJS->>MQTT: Publish(relayControl,<br/>"RELAY OFF", qos=1)
            
            MQTT->>ESP32: Forward command
            ESP32->>ESP32: Parse command
            ESP32->>Relay: GPIO write LOW
            Relay->>Relay: Switch OFF
            
            AppJS->>Browser: Update button<br/>className = "btn-secondary"<br/>text = "Device OFF"
            
            AppJS->>AppJS: Check isPeriodic
            
            alt Periodic Was ON
                AppJS->>AppJS: isPeriodic = false
                AppJS->>Browser: Update periodic button<br/>className = "btn-secondary"<br/>text = "Periodic OFF"
                
                AppJS->>MQTT: Publish(stm32Command,<br/>"PERIODIC OFF", qos=1)
                AppJS->>AppJS: addStatus("Periodic mode stopped (device OFF)")
            end
            
            AppJS->>AppJS: Reset display<br/>currentTemp = null<br/>currentHumi = null
            AppJS->>Browser: Update #currentTemp = "--"<br/>#currentHumi = "--"<br/>#lastUpdate = "--:--:--"
            
            AppJS->>AppJS: addStatus("Device RELAY OFF sent")
        end
        
        AppJS->>User: Visual feedback
    end
```

---

## 6. Time Sync from Internet

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant MQTT as MQTT Broker
    participant ESP32
    participant STM32
    participant DS3231
    
    User->>Browser: Click "Sync from Internet"
    Browser->>AppJS: Click event
    
    AppJS->>Browser: Get current browser time<br/>now = new Date()
    Browser-->>AppJS: Timestamp (ms)
    
    AppJS->>AppJS: Convert to Unix timestamp<br/>timestamp = now.getTime() / 1000
    
    AppJS->>AppJS: Update time picker state<br/>timePickerYear = now.getFullYear()<br/>timePickerMonth = now.getMonth()<br/>timePickerDay = now.getDate()<br/>timePickerHour = now.getHours()<br/>timePickerMinute = now.getMinutes()<br/>timePickerSecond = now.getSeconds()
    
    AppJS->>Browser: renderCalendar()<br/>Highlight current date
    AppJS->>Browser: updateTimeDisplay()<br/>Show HH:MM:SS
    
    AppJS->>AppJS: Add UTC+7 offset<br/>timestampWithOffset = timestamp + 7*3600
    
    AppJS->>AppJS: Build command<br/>"SET TIME {timestampWithOffset}"
    
    AppJS->>AppJS: Check isMqttConnected
    
    alt MQTT Connected
        AppJS->>MQTT: Publish(stm32Command,<br/>"SET TIME X", qos=1)
        
        MQTT->>ESP32: Forward command
        ESP32->>STM32: UART transmit<br/>{"cmd":"SET TIME","value":X}
        
        STM32->>DS3231: I2C write<br/>Set RTC registers<br/>year, month, day,<br/>hour, minute, second
        
        DS3231-->>STM32: Write complete
        STM32->>STM32: Update local time variable
        
        STM32->>ESP32: UART confirm<br/>{"status":"OK","cmd":"SET TIME"}
        ESP32->>MQTT: Publish(systemState)
        MQTT->>AppJS: Confirmation
        
        AppJS->>AppJS: addStatus("Time synced from internet:<br/>{datetime} (UTC+7)")
    else MQTT Not Connected
        AppJS->>AppJS: addStatus("ERROR:<br/>MQTT not connected")
    end
    
    AppJS->>AppJS: Update local device clock<br/>deviceClockMs = timestamp * 1000<br/>deviceClockSetAtMs = Date.now()
    
    AppJS->>AppJS: startDeviceClockTicker()
    
    loop Every 1 second
        AppJS->>AppJS: Calculate display time<br/>elapsed = Date.now() - deviceClockSetAtMs<br/>displayTime = deviceClockMs + elapsed
        AppJS->>Browser: Update #footerTime<br/>Format: HH:MM:SS
    end
    
    AppJS->>User: Time synchronized
```

---

## 7. Manual Time Setting

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant MQTT as MQTT Broker
    participant ESP32
    participant STM32
    participant DS3231
    
    Note over User,Browser: User adjusts time picker
    User->>Browser: Select date in calendar
    Browser->>AppJS: Click event
    AppJS->>AppJS: timePickerDay = selectedDay
    AppJS->>Browser: renderCalendar()<br/>Highlight selected date
    
    User->>Browser: Click hour UP/DOWN
    Browser->>AppJS: adjustTime('hour', delta)
    AppJS->>AppJS: timePickerHour += delta<br/>Wrap 0-23
    AppJS->>Browser: updateTimeDisplay()<br/>Show HH:MM:SS
    
    User->>Browser: Click minute UP/DOWN
    Browser->>AppJS: adjustTime('min', delta)
    AppJS->>AppJS: timePickerMinute += delta<br/>Wrap 0-59
    AppJS->>Browser: updateTimeDisplay()
    
    User->>Browser: Click second UP/DOWN
    Browser->>AppJS: adjustTime('sec', delta)
    AppJS->>AppJS: timePickerSecond += delta<br/>Wrap 0-59
    AppJS->>Browser: updateTimeDisplay()
    
    User->>Browser: Click "Set Time Manually"
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Read time picker state<br/>year, month, day,<br/>hour, minute, second
    
    AppJS->>AppJS: Create Date object<br/>date = new Date(year, month, day,<br/>hour, minute, second)
    
    AppJS->>AppJS: Get Unix timestamp<br/>timestamp = date.getTime() / 1000
    
    AppJS->>AppJS: Add UTC+7 offset<br/>timestampWithOffset = timestamp + 7*3600
    
    AppJS->>AppJS: Build command<br/>"SET TIME {timestampWithOffset}"
    
    AppJS->>AppJS: Check isMqttConnected
    
    alt MQTT Connected
        AppJS->>MQTT: Publish(stm32Command,<br/>"SET TIME X", qos=1)
        
        MQTT->>ESP32: Forward command
        ESP32->>STM32: UART transmit<br/>{"cmd":"SET TIME","value":X}
        
        STM32->>DS3231: I2C write<br/>Set RTC registers
        DS3231-->>STM32: Write complete
        
        STM32->>ESP32: UART confirm
        ESP32->>MQTT: Publish(systemState)
        MQTT->>AppJS: Confirmation
        
        AppJS->>AppJS: addStatus("Manual time set:<br/>{datetime} (UTC+7)")
    else MQTT Not Connected
        AppJS->>AppJS: addStatus("ERROR:<br/>MQTT not connected")
    end
    
    AppJS->>AppJS: Update local device clock<br/>deviceClockMs = timestamp * 1000<br/>deviceClockSetAtMs = Date.now()
    
    AppJS->>AppJS: startDeviceClockTicker()
    AppJS->>Browser: Update #footerTime
    
    AppJS->>User: Time updated
```

---

## 8. Data Management Filtering & Export

```mermaid
sequenceDiagram
    participant User
    participant Browser
    participant AppJS as app.js
    participant Firebase
    
    User->>Browser: Navigate to "Data Management" page
    Browser->>AppJS: Page switch event
    AppJS->>AppJS: bindDataManagementButtons()
    
    Note over User,Browser: User sets filters
    User->>Browser: Select date range<br/>From: 2025-01-01<br/>To: 2025-01-20
    User->>Browser: Select mode filter<br/>"periodic"
    User->>Browser: Select status filter<br/>"success"
    User->>Browser: Set temp range<br/>Min: 20, Max: 30
    User->>Browser: Set humidity range<br/>Min: 50, Max: 80
    User->>Browser: Select sort<br/>"time-desc"
    
    User->>Browser: Click "Apply Filters"
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Check isFirebaseConnected
    
    alt Firebase Not Connected
        AppJS->>User: Show error<br/>"Firebase not connected"
    else Firebase Connected
        AppJS->>AppJS: Read all filter inputs<br/>dateFrom, dateTo, mode, status,<br/>tempMin, tempMax, humiMin, humiMax, sortBy
        
        AppJS->>AppJS: Generate date range<br/>dates = []<br/>for d = dateFrom to dateTo<br/>dates.push(YYYY-MM-DD)
        
        AppJS->>AppJS: addStatus("Loading filtered data...")
        
        loop For each date in range
            AppJS->>Firebase: firebase.ref('readings/{date}')<br/>.once('value')
            Firebase-->>AppJS: Snapshot for date
            AppJS->>AppJS: Parse snapshot<br/>Extract records
        end
        
        AppJS->>AppJS: Merge all results<br/>allData = results.flat()
        
        AppJS->>AppJS: Apply mode filter<br/>if (mode && record.mode != mode) remove
        
        AppJS->>AppJS: Apply status filter<br/>if (status && record.status != status) remove
        
        AppJS->>AppJS: Apply temp range filter<br/>if (temp < min || temp > max) remove
        
        AppJS->>AppJS: Apply humidity range filter<br/>if (humi < min || humi > max) remove
        
        AppJS->>AppJS: Apply sorting<br/>Sort by time/temp/humi<br/>asc or desc
        
        AppJS->>AppJS: Cache filtered data<br/>filteredDataCache = filtered
        
        AppJS->>AppJS: renderDataManagementTable(filtered)
        
        AppJS->>Browser: Build HTML table<br/>For each record:<br/>#, Date/Time, Temp, Humi, Mode, Status
        
        AppJS->>AppJS: updateDataStatistics(filtered)
        AppJS->>AppJS: Calculate:<br/>- Total records<br/>- Success rate<br/>- Avg temperature<br/>- Avg humidity
        
        AppJS->>Browser: Update #statsTotal<br/>#statsSuccess<br/>#statsAvgTemp<br/>#statsAvgHumi
        
        AppJS->>AppJS: addStatus("Loaded X records from Y total")
        
        AppJS->>User: Display filtered data
    end
    
    Note over User,Browser: User exports data
    User->>Browser: Click "Export Filtered Data"
    Browser->>AppJS: Click event
    
    AppJS->>AppJS: Check filteredDataCache length
    
    alt No Data
        AppJS->>User: Show warning<br/>"No data to export"
    else Has Data
        AppJS->>AppJS: Build CSV<br/>Headers: #, Date, Time, Temp, Humi, Mode, Status, Sensor, Device
        
        loop For each record in filteredDataCache
            AppJS->>AppJS: Format row<br/>idx, date, time, temp, humi, mode, status, sensor, device
        end
        
        AppJS->>AppJS: Combine to CSV string<br/>csv = headers + rows
        
        AppJS->>Browser: Create Blob(csv, "text/csv")
        AppJS->>Browser: Create download link
        AppJS->>Browser: Trigger download<br/>datalogger_export_{timestamp}.csv
        
        AppJS->>AppJS: addStatus("Exported X records to CSV")
        AppJS->>User: File downloaded
    end
```

---

## 9. Live Data Table Real-time Update

```mermaid
sequenceDiagram
    participant STM32
    participant ESP32
    participant MQTT as MQTT Broker
    participant AppJS as app.js
    participant Browser
    
    Note over STM32,ESP32: Periodic measurement
    STM32->>ESP32: Send sensor data (JSON)
    ESP32->>MQTT: Publish(periodicData)
    MQTT->>AppJS: Forward message
    
    AppJS->>AppJS: handleMQTTMessage(topic, payload)
    AppJS->>AppJS: Parse JSON
    AppJS->>AppJS: Extract {time, temp, humi, mode, status}
    
    AppJS->>AppJS: addToLiveDataTable(data)
    AppJS->>AppJS: liveDataBuffer.unshift(entry)
    
    AppJS->>AppJS: Check buffer length
    
    alt Length > 100
        AppJS->>AppJS: liveDataBuffer.pop()<br/>Remove oldest entry
    end
    
    AppJS->>AppJS: Get filter selection<br/>filter = #liveDataFilter.value
    
    AppJS->>AppJS: Apply filter
    
    alt Filter = "all"
        AppJS->>AppJS: filtered = liveDataBuffer
    else Filter = "success"
        AppJS->>AppJS: filtered = buffer.filter<br/>(d => d.status == 'success')
    else Filter = "error"
        AppJS->>AppJS: filtered = buffer.filter<br/>(d => d.status == 'error')
    end
    
    AppJS->>AppJS: Check filtered length
    
    alt Empty
        AppJS->>Browser: innerHTML = "No data available"
    else Has Data
        loop For each entry in filtered
            AppJS->>AppJS: Format time<br/>HH:MM:SS
            AppJS->>AppJS: Format status badge
            
            alt Status = "success"
                AppJS->>AppJS: Badge: connected "✓ Success"
            else Status = "error"
                AppJS->>AppJS: Badge: disconnected "✗ Error"
            end
            
            AppJS->>AppJS: Build table row<br/><tr>#, time, temp, humi, mode, badge</tr>
        end
        
        AppJS->>Browser: Render to #liveDataTable<br/>tbody.innerHTML = rows
    end
    
    AppJS->>Browser: Update #liveDataCount<br/>textContent = filtered.length
    
    AppJS->>AppJS: Refresh Feather icons<br/>feather.replace()
    
    AppJS->>Browser: Table updated (real-time)
    
    Note over AppJS,Browser: User interacts with table
    
    alt User changes filter
        Browser->>AppJS: Change event
        AppJS->>AppJS: renderLiveDataTable()<br/>Re-filter and re-render
        AppJS->>Browser: Updated table
    end
    
    alt User clicks "Clear Table"
        Browser->>AppJS: Click event
        AppJS->>AppJS: Confirm dialog
        
        alt User confirms
            AppJS->>AppJS: liveDataBuffer = []
            AppJS->>AppJS: renderLiveDataTable()
            AppJS->>Browser: Empty table<br/>addStatus("Live data cleared")
        end
    end
    
    alt User clicks "Export CSV"
        Browser->>AppJS: Click event
        
        alt Buffer empty
            AppJS->>Browser: Show warning<br/>"No data to export"
        else Buffer has data
            AppJS->>AppJS: Build CSV<br/>Headers + rows
            AppJS->>Browser: Create Blob
            AppJS->>Browser: Trigger download<br/>livedata_{timestamp}.csv
            AppJS->>AppJS: addStatus("Live data exported")
        end
    end
```

---

## 10. Error Handling - Sensor Failure

```mermaid
sequenceDiagram
    participant STM32
    participant SHT31
    participant ESP32
    participant MQTT as MQTT Broker
    participant AppJS as app.js
    participant Browser
    
    Note over STM32: Periodic measurement triggered
    STM32->>SHT31: I2C read request
    
    alt Sensor Disconnected/Failed
        SHT31-->>STM32: No response / NACK
        
        STM32->>STM32: Detect I2C error<br/>Set temp = 0.0, humi = 0.0
        
        STM32->>DS3231: I2C read RTC (still works)
        DS3231-->>STM32: Valid timestamp
        
        STM32->>STM32: Format JSON<br/>{"temperature":0.0,"humidity":0.0,"timestamp":X}
        
        STM32->>ESP32: UART transmit error JSON
        ESP32->>MQTT: Publish(periodicData)
        MQTT->>AppJS: Forward message
        
        AppJS->>AppJS: Parse JSON
        AppJS->>AppJS: Check: temp == 0.0 && humi == 0.0
        
        AppJS->>AppJS: sensorFailed = true
        AppJS->>AppJS: addStatus("ERROR: SHT31 sensor hardware failed<br/>(disconnected or wiring issue)")
        AppJS->>AppJS: addStatus("WARNING: Sensor hardware failed")
        
        AppJS->>AppJS: updateComponentHealth(SHT31, false)
        AppJS->>AppJS: componentHealth.SHT31.healthy = false<br/>errorCount++
        
        AppJS->>Browser: Update component card<br/>#component-SHT31
        AppJS->>Browser: Badge: disconnected "Error"
        AppJS->>Browser: #sht31-temp = "Sensor Failed"<br/>#sht31-humi = "Check Wiring"<br/>#sht31-time = "Error"
        
        AppJS->>AppJS: Save to Firebase<br/>status = "error"<br/>error = "sensor_fail"
        
        AppJS->>Firebase: Save record with error flag
        Firebase-->>AppJS: Write complete
        
        AppJS->>AppJS: Skip chart update<br/>(Don't push 0 values)
        
        AppJS->>AppJS: addToLiveDataTable<br/>status = "error"
        AppJS->>Browser: Render error row<br/>Red badge "✗ Error"
        
        AppJS->>Browser: Console shows error logs
    end
```

---

## System Characteristics

### Timing Analysis
- **MQTT Message Latency**: 50-100ms (local network)
- **Firebase Write Latency**: 200-500ms (network dependent)
- **Chart Update Animation**: 750ms (easeInOutQuart)
- **State Sync Delay**: 500ms after MQTT connect
- **Clock Ticker Interval**: 1000ms (1 second)

### Message Flow Patterns
- **Command Flow**: User → App → MQTT → ESP32 → STM32 → Hardware
- **Data Flow**: Hardware → STM32 → ESP32 → MQTT → App → UI
- **Sync Flow**: App → MQTT → ESP32 (state query) → MQTT → App (state response)

### Error Recovery
- **MQTT Disconnect**: Auto-reconnect every 2 seconds
- **Firebase Disconnect**: Auto-reconnect via .info/connected listener
- **Sensor Failure**: Log error, skip chart update, save with error flag
- **RTC Failure**: Use local browser time as fallback
- **Chart Init Failure**: Retry 10 times with 500ms delay

### Data Integrity
- **MQTT QoS**: Level 1 (at least once delivery)
- **Firebase Write**: Automatic retry on network failure
- **Buffer Management**: FIFO with configurable max size
- **Timestamp Source**: Prefer device RTC, fallback to browser time

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-XX  
**Total Sequence Diagrams**: 10  
**Coverage**: Complete interaction flows
