# CoAP Handler Component

## Overview

CoAP (Constrained Application Protocol) handler component for ESP32. This component provides a lightweight client implementation for IoT communication over UDP, designed as an alternative to MQTT for resource-constrained environments.

**Key Features:**
- RFC 7252 compliant CoAP protocol
- UDP-based (connectionless, minimal overhead)
- Publish/Subscribe pattern (OBSERVE mechanism)
- JSON payload support
- Automatic retries on failure
- Minimal memory footprint (~5 KB)
- Easy integration with web servers

## Architecture

```
┌─────────────────────────┐
│   ESP32 Application     │
│  ┌───────────────────┐  │
│  │  Your Main Code   │  │
│  └─────────┬─────────┘  │
│            │            │
│  ┌─────────▼─────────┐  │
│  │  CoAP Handler API │  │
│  │  (coap_handler.c) │  │
│  └─────────┬─────────┘  │
│            │            │
│  ┌─────────▼─────────┐  │
│  │ ESP-IDF CoAP Lib  │  │
│  └─────────┬─────────┘  │
│            │            │
│  ┌─────────▼─────────┐  │
│  │    UDP/lwIP       │  │
│  └───────────────────┘  │
└─────────────────────────┘
            │
            │ UDP Port 5683
            ▼
    ┌─────────────────┐
    │  CoAP Server    │
    │  (Web/Node.js)  │
    └─────────────────┘
```

## Comparison: MQTT vs CoAP

| Feature | MQTT | CoAP |
|---------|------|------|
| **Transport** | TCP | UDP |
| **Overhead** | Medium | Very Low |
| **Latency** | Higher | Lower (connectionless) |
| **Reliability** | Guaranteed | Optional (with retries) |
| **Memory** | ~15-20 KB | ~5 KB |
| **Best For** | Message brokers, reliable delivery | Sensors, real-time, low power |
| **Standard Port** | 1883 | 5683 |

## Quick Start

### Initialize & Start

```c
#include "coap_handler.h"

coap_handler_t coap;

void on_coap_data(const char *path, const char *data, int len) {
    ESP_LOGI(TAG, "← %s: %s", path, data);
}

// Initialize
CoAP_Handler_Init(&coap, "192.168.1.100", 5683, on_coap_data);

// Start (when WiFi connected)
CoAP_Handler_Start(&coap);

// Publish JSON data
char json[256];
snprintf(json, sizeof(json), "{\"temp\":%.2f,\"humidity\":%.2f}", 25.5, 60.0);
CoAP_Handler_Publish(&coap, "/api/sensor/data", json, 0, true);

// Subscribe to commands
CoAP_Handler_Subscribe(&coap, "/api/command");

// Check status
if (CoAP_Handler_IsConnected(&coap)) {
    ESP_LOGI(TAG, "CoAP is connected");
}

// Stop
CoAP_Handler_Stop(&coap);
```

### ESP32 Configuration (menuconfig)

```
Component config → Component: coap_handler
  ☑ Enable CoAP Protocol Support
  CoAP Server IP Address: 192.168.1.100
  CoAP Server Port: 5683
```

## Web Server Integration (Node.js)

### 1. Install Dependencies

```bash
npm install coap
```

### 2. Simple CoAP Server

```javascript
const coap = require('coap');

const server = coap.createServer();

server.on('request', (req, res) => {
    console.log(`${req.method.toUpperCase()} ${req.url}`);
    
    // Handle PUT requests (ESP32 publishing)
    if (req.method === 'put' && req.url === '/api/sensor/data') {
        const data = JSON.parse(req.payload.toString());
        console.log('Sensor data:', data);
        
        // Save to database
        database.insert('sensor_data', {
            timestamp: new Date(),
            ...data
        });
        
        res.code = '2.04';  // Changed
        res.end();
    }
});

server.listen(5683);
console.log('CoAP server on port 5683');
```

### 3. Observable Resources (Server → ESP32)

```javascript
const observers = [];

server.on('request', (req, res) => {
    // Handle OBSERVE requests (ESP32 watching for updates)
    if (req.url === '/api/command' && req.observe === 0) {
        observers.push(res);
        res.write(JSON.stringify({ command: 'IDLE' }));
    }
});

// Broadcast command to all watching devices
function broadcastCommand(cmd) {
    observers.forEach(res => {
        res.write(JSON.stringify({ command: cmd, time: Date.now() }));
    });
}

// Send commands periodically
setInterval(() => {
    const hour = new Date().getHours();
    if (hour === 8) {  // 8 AM
        broadcastCommand('PERIODIC_ON');
    }
}, 60000);
```

## Resource Paths (RESTful Convention)

```
/api/
├── sensor/
│   ├── temperature         # Single metric
│   ├── humidity            # Single metric
│   └── data                # Aggregate JSON
├── device/
│   ├── relay               # Control relay
│   └── state               # Device status
└── command                 # Receive commands from server
```

## Performance

| Metric | Value |
|--------|-------|
| Memory (context) | 2-3 KB |
| Memory (session) | 0.5-1 KB |
| UDP packet size | 1024 bytes |
| Max JSON payload | 256-512 bytes |
| Typical latency | 10-50 ms |

## Full Integration Example

See `main.c` for complete integration with WiFi, MQTT, and sensors.

Key points:
- Start CoAP when WiFi connected
- Stop CoAP when WiFi disconnected
- Publish sensor data periodically
- Subscribe to remote commands
- Handle both CoAP and MQTT simultaneously

## Dependencies

- ESP-IDF built-in CoAP library
- FreeRTOS
- lwIP (UDP stack)

## References

- [RFC 7252 - CoAP Specification](https://tools.ietf.org/html/rfc7252)
- [RFC 7641 - OBSERVE](https://tools.ietf.org/html/rfc7641)
- [ESP-IDF CoAP](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/coap.html)

## License

MIT License
