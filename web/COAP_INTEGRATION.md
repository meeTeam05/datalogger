# Web Server Integration Guide - CoAP & MQTT

This guide explains how to integrate ESP32 CoAP and MQTT clients with your web server.

## Architecture Overview

```
┌─────────────────────┐
│   Web Dashboard     │
│   (HTML/JS)         │
└──────────┬──────────┘
           │
    ┌──────▼──────┐
    │  Node.js    │
    │  Server     │
    ├─────────────┤
    │ HTTP REST   │ ← Browser communication
    │ CoAP 5683   │ ← ESP32 sensor data
    │ MQTT 1883   │ ← ESP32 control
    └──────┬──────┘
           │
    ┌──────▼───────────┐
    │    Mosquitto    │
    │    MQTT Broker  │
    │    Port 1883    │
    └─────────────────┘
           ▲
           │
    ┌──────┴──────────┐
    │    ESP32        │
    │   WiFi Client   │
    │ MQTT + CoAP     │
    └─────────────────┘
```

## Protocol Comparison

| Protocol | Use Case | Port | Advantages |
|----------|----------|------|------------|
| **MQTT** | Device control, reliable messaging | 1883 | Pub/Sub, QoS, retained messages |
| **CoAP** | Sensor data streaming | 5683 | Lower overhead, UDP, observables |
| **HTTP** | Web dashboard | 80/443 | Browser native, easy debugging |

## Setup: MQTT (Already Working)

Your MQTT setup uses:
- **Broker**: Mosquitto on localhost
- **Port**: 1883 (native), 8083 (WebSocket)
- **Topics**: See `firmware/ESP32/main/main.c`
- **Communication**: Reliable, persistent connections

No changes needed here - it's working well!

## Setup: CoAP (Optional Alternative)

### 1. Install Node CoAP Server

```bash
cd your-web-server-directory
npm install coap
```

### 2. Create CoAP Listener

Create `coap_server.js`:

```javascript
const coap = require('coap');
const fs = require('fs');

// Store sensor data
const database = {
    sensorData: [],
    
    insert(data) {
        this.sensorData.push({
            ...data,
            timestamp: new Date().toISOString()
        });
        
        // Keep only last 1000 records
        if (this.sensorData.length > 1000) {
            this.sensorData.shift();
        }
        
        // Optional: Save to file
        fs.writeFileSync('sensor_data.json', JSON.stringify(this.sensorData, null, 2));
    },
    
    getLastN(n = 50) {
        return this.sensorData.slice(-n);
    }
};

// Create CoAP server
const server = coap.createServer();

server.on('request', (req, res) => {
    const timestamp = new Date().toLocaleTimeString();
    console.log(`[${timestamp}] ${req.method.toUpperCase()} ${req.url}`);
    
    // Handle ESP32 sensor data publication
    if (req.method === 'put' && req.url === '/api/sensor/data') {
        try {
            const data = JSON.parse(req.payload.toString());
            console.log('  → Sensor:', data);
            database.insert(data);
            
            res.code = '2.04';  // Changed
            res.end();
        } catch (e) {
            console.error('  ✗ Parse error:', e.message);
            res.code = '4.00';  // Bad Request
            res.end();
        }
    }
    
    // Handle ESP32 temperature query
    else if (req.method === 'get' && req.url === '/api/last/temperature') {
        const last = database.sensorData[database.sensorData.length - 1];
        if (last) {
            res.write(JSON.stringify({ 
                temperature: last.temperature,
                timestamp: last.timestamp 
            }));
        }
        res.end();
    }
    
    // Handle observable commands (push from server)
    else if (req.url === '/api/command' && req.observe === 0) {
        console.log('  ✓ ESP32 subscribed to commands');
        
        // Send initial command
        res.write(JSON.stringify({ 
            command: 'SYNC',
            time: Date.now()
        }));
    }
    
    else {
        res.code = '4.04';  // Not Found
        res.end();
    }
});

server.listen(5683);
console.log('CoAP server listening on port 5683');
```

### 3. Run Both MQTT and CoAP

```bash
# Terminal 1: MQTT Broker
mosquitto -c broker/config/mosquitto.conf

# Terminal 2: Node.js Web Server with both MQTT and CoAP
node server.js         # Must handle both protocols

# Terminal 3: Flash ESP32
cd firmware/ESP32
idf.py flash monitor
```

### 4. Enable in ESP32 Build

```bash
idf.py menuconfig
# Component config → Component: coap_handler
# ☑ Enable CoAP Protocol Support
# Set IP to your server address

idf.py build
idf.py flash
```

## Web Dashboard Updates

### Display CoAP Data Alongside MQTT

Edit `web/script.js`:

```javascript
// Keep existing MQTT handling
const MQTT_CONFIG = { /* ... existing config ... */ };

// Add CoAP server info (optional, for reference)
const COAP_CONFIG = {
    server: '192.168.1.100',
    port: 5683,
    protocol: 'udp'
};

// Update chart with both MQTT and CoAP data
function updateChart(data) {
    // Data can come from either MQTT or CoAP
    // Both store same JSON format
    
    temperatureData.push(data.temperature);
    humidityData.push(data.humidity);
    
    if (temperatureData.length > maxDataPoints) {
        temperatureData.shift();
        humidityData.shift();
    }
    
    // Redraw chart
    chart.update();
}
```

### Chart.js Integration

```html
<canvas id="sensorChart" width="400" height="100"></canvas>
<script>
const chart = new Chart(document.getElementById('sensorChart'), {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            {
                label: 'Temperature (°C)',
                data: [],
                borderColor: 'rgb(255, 99, 132)',
                tension: 0.1
            },
            {
                label: 'Humidity (%)',
                data: [],
                borderColor: 'rgb(54, 162, 235)',
                tension: 0.1
            }
        ]
    },
    options: {
        responsive: true,
        scales: {
            y: {
                beginAtZero: false,
                max: 100
            }
        }
    }
});
</script>
```

## Using Both MQTT and CoAP

### MQTT Use Cases (Already Implemented)
- ✅ Reliable device control (relay on/off)
- ✅ Persistent state (device ON/OFF)
- ✅ Publish/Subscribe patterns
- ✅ Retained messages

### CoAP Use Cases (Optional)
- Lightweight sensor streaming (real-time)
- Low power consumption
- Reduced network overhead
- Observable resources (push notifications)

### Best Practice: Hybrid Approach

```javascript
// MQTT: For control and state
MQTT_Topics = {
    control: 'datalogger/esp32/relay/control',      // MQTT (reliable)
    state: 'datalogger/esp32/system/state',         // MQTT (persistent)
};

// CoAP: For sensor data
CoAP_Paths = {
    temperature: '/api/sensor/temperature',         // CoAP (efficient)
    humidity: '/api/sensor/humidity',               // CoAP (efficient)
    aggregate: '/api/sensor/data'                   // CoAP (JSON)
};
```

## Monitoring & Debugging

### Check MQTT Broker

```bash
# View all connected clients
mosquitto_clients -h localhost

# Monitor topic traffic
mosquitto_sub -h localhost -v -t '#'
```

### Check CoAP Traffic

```bash
# On macOS/Linux
sudo tcpdump -i any -n "udp port 5683" -A

# Windows: Use Wireshark
# Filter: udp.port == 5683
```

### ESP32 Serial Monitor

```bash
idf.py monitor

# Look for:
# I (xxxx) MQTT_HANDLER: Connected
# I (xxxx) COAP_HANDLER: Started
# I (xxxx) CoAP published: /api/sensor/data
```

## Docker Deployment (Optional)

### docker-compose.yml

```yaml
version: '3'
services:
  mosquitto:
    image: eclipse-mosquitto:latest
    ports:
      - "1883:1883"
      - "8083:8083"
    volumes:
      - ./broker/config:/mosquitto/config
      - ./broker/data:/mosquitto/data

  web-server:
    build: .
    ports:
      - "3000:3000"
      - "5683:5683/udp"  # CoAP
    depends_on:
      - mosquitto
    environment:
      - MQTT_HOST=mosquitto
      - MQTT_PORT=1883
      - COAP_PORT=5683
```

### Build and Run

```bash
docker-compose up -d

# Check logs
docker-compose logs -f web-server
```

## API Reference

### REST Endpoints (HTTP)

```
GET  /api/chart/data        # Get sensor history
GET  /api/device/status     # Get device state
POST /api/device/relay      # Control relay
GET  /api/system/health     # Check system health
```

### MQTT Topics

See `firmware/ESP32/main/main.c` for complete list.

### CoAP Resources

```
PUT  /api/sensor/data       # Publish sensor data
GET  /api/last/temperature  # Query last reading
OBS  /api/command           # Observe server commands
```

## Performance Notes

| Metric | MQTT | CoAP |
|--------|------|------|
| Connection setup | ~100ms | ~50ms |
| Sensor data publish | ~10ms | ~5ms |
| Reliability | Very High | Good (with retries) |
| Bandwidth | Medium | Low |

**Recommendation**: Use both!
- MQTT for critical control messages (relay, state)
- CoAP for high-frequency sensor data (temperature, humidity)

## Troubleshooting

### ESP32 not connecting to server

1. Check IP address is correct
   ```c
   CONFIG_COAP_SERVER_IP = "192.168.1.100"  // Your server IP
   ```

2. Check firewall allows UDP 5683 and TCP 1883
   ```bash
   sudo ufw allow 5683/udp
   sudo ufw allow 1883/tcp
   ```

3. Check WiFi is connected
   ```
   Look for "WiFi: Connected" in serial monitor
   ```

### Dashboard not updating

1. Check browser console for JavaScript errors
2. Verify MQTT/CoAP server is running
3. Check network connectivity
4. Enable debug logging in both ESP32 and web server

## References

- MQTT: https://mosquitto.org/
- CoAP: https://tools.ietf.org/html/rfc7252
- Node.js CoAP: https://www.npmjs.com/package/coap

## Next Steps

1. **Test MQTT** (already working) - verify all control works
2. **Enable CoAP** (optional) - for better sensor performance
3. **Monitor performance** - choose which protocol fits best
4. **Scale up** - add more sensors or devices as needed

Happy coding! 🚀
