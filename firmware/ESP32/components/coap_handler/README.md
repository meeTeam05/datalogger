# CoAP Handler Component

> **UNDER DEVELOPMENT - NOT YET IMPLEMENTED**
>
> This component is currently in planning/documentation phase and is not yet functional. The CoAP handler will be implemented in a future release. This README serves as a design specification and implementation guide.

This component provides CoAP (Constrained Application Protocol) client functionality for the ESP32 in the DATALOGGER system. It implements a lightweight UDP-based communication protocol designed for resource-constrained IoT devices, offering an alternative to MQTT for scenarios requiring lower overhead and latency.

## Component Files

```
coap_handler/
├── coap_handler.h        # Public API header with structure and function declarations
├── coap_handler.c        # Implementation of CoAP client logic
├── CMakeLists.txt        # ESP-IDF build configuration
├── component.mk          # Legacy build system support
├── Kconfig               # Configuration menu options
└── README.md             # This file
```

## Overview

The CoAP Handler component provides a high-level client interface for communicating with CoAP servers over UDP. It implements RFC 7252 (CoAP) and RFC 7641 (OBSERVE) specifications, supporting resource publication, subscription to observable resources, automatic retries, and message handling callbacks. CoAP is particularly suitable for battery-powered sensors and devices requiring minimal protocol overhead.

## Key Features

- RFC 7252 compliant CoAP client implementation
- UDP-based transport for minimal overhead (connectionless)
- Resource publication using PUT/POST methods
- Observable resource subscription (OBSERVE mechanism per RFC 7641)
- Automatic message retry on failures
- Configurable timeout and retry parameters
- JSON payload support with automatic content-type handling
- Message ID tracking for request-response correlation
- Connection state management
- Data reception callback system
- Low memory footprint (approximately 5 KB)
- RESTful resource path structure

## Protocol Comparison: MQTT vs CoAP

| Feature              | MQTT                                    | CoAP                               |
| -------------------- | --------------------------------------- | ---------------------------------- |
| Transport Protocol   | TCP (reliable)                          | UDP (best-effort)                  |
| Connection           | Persistent connection                   | Connectionless (per-message)       |
| Overhead per Message | Medium (TCP headers)                    | Very Low (UDP headers)             |
| Latency              | Higher (connection setup)               | Lower (direct datagram)            |
| Reliability          | Guaranteed delivery (QoS 1/2)           | Optional (confirmable messages)    |
| Memory Usage         | Approximately 15-20 KB                  | Approximately 5 KB                 |
| Typical Latency      | 50-200 ms                               | 10-50 ms                           |
| Best Use Cases       | Reliable messaging, broker architecture | Sensors, real-time data, low power |
| Standard Port        | 1883 (1883/8883 for TLS)                | 5683 (5684 for DTLS)               |
| Message Pattern      | Publish-Subscribe via broker            | Direct client-server RESTful       |

## Architecture

```
┌──────────────────────────────────────┐
│        ESP32 Application             │
│   ┌──────────────────────────────┐   │
│   │       Application Code       │   │
│   │  (Sensor Reading, Control)   │   │
│   └──────────────┬───────────────┘   │
│                  │                   │
│                  ↓                   │
│   ┌──────────────────────────────┐   │
│   │   CoAP Handler API           │   │
│   │  - CoAP_Handler_Init()       │   │
│   │  - CoAP_Handler_Publish()    │   │
│   │  - CoAP_Handler_Subscribe()  │   │
│   └──────────────┬───────────────┘   │
│                  │                   │
│                  ↓                   │
│   ┌──────────────────────────────┐   │
│   │   ESP-IDF libcoap Library    │   │
│   │  (RFC 7252 Implementation)   │   │
│   └──────────────┬───────────────┘   │
│                  │                   │
│                  ↓                   │
│   ┌──────────────────────────────┐   │
│   │       lwIP UDP Stack         │   │
│   └──────────────────────────────┘   │
└──────────────────┬───────────────────┘
                   │
                   │ UDP Port 5683
                   │
                   ↓
       ┌───────────────────────┐
       │      CoAP Server      │
       │  (Node.js/Python/etc) │
       └───────────────────────┘
```

## CoAP Handler Structure

```c
typedef struct {
    coap_context_t *ctx;              // CoAP context handle from libcoap
    coap_address_t server_addr;       // Server address structure
    coap_data_callback_t data_callback; // Callback for incoming messages
    bool connected;                   // Connection status flag
    char server_ip[16];               // Server IP (for display/logging)
    uint16_t server_port;             // Server port number
} coap_handler_t;
```

## Configuration Parameters

```c
#define COAP_MAX_PATH_LEN 64      // Maximum resource path length
#define COAP_MAX_DATA_LEN 512     // Maximum payload size
#define COAP_DEFAULT_PORT 5683    // Standard CoAP port
#define COAP_TIMEOUT_MS 5000      // Request timeout (5 seconds)
#define COAP_MAX_RETRIES 3        // Maximum retry attempts
```

## API Functions

### Initialization

**CoAP_Handler_Init**

```c
bool CoAP_Handler_Init(coap_handler_t *coap,
                       const char *server_ip,
                       uint16_t server_port,
                       coap_data_callback_t callback);
```

Initializes the CoAP handler with server connection parameters.

Parameters:

- coap: Pointer to CoAP handler structure
- server_ip: CoAP server IP address as string (e.g., "192.168.1.100")
- server_port: CoAP server port (typically 5683)
- callback: Function to call when data is received (can be NULL)

Returns:

- true: Initialization successful
- false: Initialization failed (invalid parameters or memory allocation failure)

This function allocates the CoAP context, sets up server address structures, and registers the data callback. It does not establish a connection (CoAP is connectionless).

### Connection Management

**CoAP_Handler_Start**

```c
bool CoAP_Handler_Start(coap_handler_t *coap);
```

Starts the CoAP client and prepares for communication.

Parameters:

- coap: Pointer to initialized CoAP handler structure

Returns:

- true: Client started successfully
- false: Start failed (handler not initialized or network error)

Call this after WiFi is connected. It sets the connected flag and prepares the context for sending and receiving messages.

**CoAP_Handler_Stop**

```c
void CoAP_Handler_Stop(coap_handler_t *coap);
```

Stops the CoAP client.

Parameters:

- coap: Pointer to CoAP handler structure

Clears the connected flag. Does not free resources (use CoAP_Handler_Deinit for cleanup).

**CoAP_Handler_IsConnected**

```c
bool CoAP_Handler_IsConnected(coap_handler_t *coap);
```

Checks if CoAP client is active.

Parameters:

- coap: Pointer to CoAP handler structure

Returns:

- true: Client is started and ready
- false: Client is not started

### Publishing Data

**CoAP_Handler_Publish**

```c
int CoAP_Handler_Publish(coap_handler_t *coap,
                         const char *path,
                         const char *data,
                         int data_len,
                         bool is_json);
```

Publishes data to a CoAP resource using PUT method.

Parameters:

- coap: Pointer to CoAP handler structure
- path: Resource path (e.g., "/api/sensor/data")
- data: Data payload to send
- data_len: Length of data (pass 0 for null-terminated string)
- is_json: true if data is JSON (sets content-type accordingly)

Returns:

- Message ID (positive integer): Request sent successfully
- -1: Request failed (handler not connected or send error)

The function creates a confirmable PUT request to the specified resource path. If is_json is true, the content-type is set to application/json. The server should respond with a 2.04 Changed code.

### Subscribing to Resources

**CoAP_Handler_Subscribe**

```c
bool CoAP_Handler_Subscribe(coap_handler_t *coap, const char *path);
```

Subscribes to an observable CoAP resource (OBSERVE mechanism).

Parameters:

- coap: Pointer to CoAP handler structure
- path: Resource path to observe (e.g., "/api/command")

Returns:

- true: Subscription request sent successfully
- false: Subscription failed (handler not connected or send error)

Sends a GET request with the OBSERVE option set. The server will send notifications whenever the resource changes. Notifications are delivered to the registered data callback.

### Cleanup

**CoAP_Handler_Deinit**

```c
void CoAP_Handler_Deinit(coap_handler_t *coap);
```

Deinitializes the CoAP handler and frees resources.

Parameters:

- coap: Pointer to CoAP handler structure

Stops the client, frees the CoAP context, and releases allocated memory.

## Usage Example

### Basic Initialization and Data Publishing

```c
#include "coap_handler.h"

coap_handler_t coap;

// Data reception callback
void on_coap_data(const char *path, const char *data, int len) {
    printf("Received from %s: %.*s\n", path, len, data);

    // Parse and handle received data
    // Example: command from server
}

void app_main(void) {
    // Wait for WiFi connection
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    // Initialize CoAP handler
    if (!CoAP_Handler_Init(&coap, "192.168.1.100", 5683, on_coap_data)) {
        printf("CoAP initialization failed!\n");
        return;
    }

    // Start CoAP client
    if (!CoAP_Handler_Start(&coap)) {
        printf("CoAP start failed!\n");
        return;
    }

    printf("CoAP client started\n");

    // Publish sensor data
    char json[256];
    snprintf(json, sizeof(json),
             "{\"temperature\":%.2f,\"humidity\":%.2f,\"timestamp\":%ld}",
             25.5, 60.0, time(NULL));

    int msg_id = CoAP_Handler_Publish(&coap, "/api/sensor/data",
                                       json, 0, true);
    if (msg_id > 0) {
        printf("Data published, message ID: %d\n", msg_id);
    }
}
```

### Subscribing to Observable Resources

```c
void app_main(void) {
    // Initialize and start CoAP
    CoAP_Handler_Init(&coap, "192.168.1.100", 5683, on_coap_data);
    CoAP_Handler_Start(&coap);

    // Subscribe to command resource
    if (CoAP_Handler_Subscribe(&coap, "/api/command")) {
        printf("Subscribed to /api/command\n");
    }

    // Server will send notifications when command resource changes
    // Notifications are delivered to on_coap_data callback
}
```

### Full Integration with Sensor Reading

```c
void sensor_task(void *pvParameters) {
    while (1) {
        if (CoAP_Handler_IsConnected(&coap)) {
            // Read sensor data
            float temp = read_temperature();
            float humidity = read_humidity();

            // Create JSON payload
            char json[256];
            snprintf(json, sizeof(json),
                    "{\"temp\":%.2f,\"hum\":%.2f,\"device\":\"esp32_01\"}",
                    temp, humidity);

            // Publish to server
            CoAP_Handler_Publish(&coap, "/api/sensor/data", json, 0, true);

            printf("Sensor data published: temp=%.2f, humidity=%.2f\n",
                   temp, humidity);
        }

        vTaskDelay(pdMS_TO_TICKS(60000));  // Publish every 60 seconds
    }
}

void app_main(void) {
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    CoAP_Handler_Init(&coap, "192.168.1.100", 5683, on_coap_data);
    CoAP_Handler_Start(&coap);

    xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
}
```

## Server Implementation Examples

### Node.js CoAP Server

Install the CoAP library:

```bash
npm install coap
```

Basic server implementation:

```javascript
const coap = require("coap");

const server = coap.createServer();

server.on("request", (req, res) => {
  console.log(`${req.method.toUpperCase()} ${req.url}`);

  // Handle PUT requests from ESP32
  if (req.method === "put" && req.url === "/api/sensor/data") {
    const payload = req.payload.toString();
    const data = JSON.parse(payload);

    console.log("Received sensor data:", data);

    // Save to database
    saveSensorData(data);

    // Send 2.04 Changed response
    res.code = "2.04";
    res.end();
  }

  // Handle GET requests
  if (req.method === "get" && req.url === "/api/status") {
    res.code = "2.05"; // Content
    res.end(JSON.stringify({ status: "OK", uptime: process.uptime() }));
  }
});

server.listen(5683, () => {
  console.log("CoAP server listening on port 5683");
});
```

### Observable Resources (Server Push)

```javascript
const observers = [];

server.on("request", (req, res) => {
  // Handle OBSERVE requests (ESP32 subscribing)
  if (req.url === "/api/command" && req.headers.Observe === 0) {
    console.log("New observer for /api/command");

    observers.push(res);

    // Send initial response
    res.write(JSON.stringify({ command: "IDLE", timestamp: Date.now() }));

    // Keep connection alive for notifications
  }
});

// Broadcast command to all observing devices
function broadcastCommand(command) {
  const message = JSON.stringify({ command, timestamp: Date.now() });

  observers.forEach((res) => {
    res.write(message);
  });

  console.log(`Broadcast command: ${command} to ${observers.length} devices`);
}

// Example: Trigger command based on time or event
setInterval(() => {
  const hour = new Date().getHours();

  if (hour === 8) {
    broadcastCommand("TURN_ON");
  } else if (hour === 22) {
    broadcastCommand("TURN_OFF");
  }
}, 60000); // Check every minute
```

### Python CoAP Server (aiocoap)

Install aiocoap:

```bash
pip install aiocoap
```

Basic server:

```python
import asyncio
import aiocoap
import aiocoap.resource as resource
import json

class SensorDataResource(resource.Resource):
    async def render_put(self, request):
        payload = request.payload.decode('utf-8')
        data = json.loads(payload)

        print(f"Received sensor data: {data}")

        # Save to database
        save_sensor_data(data)

        return aiocoap.Message(code=aiocoap.CHANGED)

async def main():
    root = resource.Site()
    root.add_resource(['api', 'sensor', 'data'], SensorDataResource())

    await aiocoap.Context.create_server_context(root, bind=('0.0.0.0', 5683))

    print("CoAP server running on port 5683")
    await asyncio.get_running_loop().create_future()

if __name__ == "__main__":
    asyncio.run(main())
```

## RESTful Resource Path Conventions

Recommended resource path structure:

```
/api/
├── sensor/
│   ├── temperature         # Single temperature reading
│   ├── humidity            # Single humidity reading
│   ├── pressure            # Single pressure reading
│   └── data                # Aggregate sensor data (JSON)
├── device/
│   ├── relay               # Relay control (PUT ON/OFF)
│   ├── state               # Device status (GET)
│   └── info                # Device information (GET)
├── command                 # Remote commands (OBSERVE)
└── status                  # System status (GET)
```

Example usage:

```c
// Publish aggregate data
CoAP_Handler_Publish(&coap, "/api/sensor/data", json, 0, true);

// Publish single metric
CoAP_Handler_Publish(&coap, "/api/sensor/temperature", "25.5", 0, false);

// Control relay
CoAP_Handler_Publish(&coap, "/api/device/relay", "ON", 0, false);

// Subscribe to commands
CoAP_Handler_Subscribe(&coap, "/api/command");
```

## CoAP Message Types

CoAP supports four message types:

**Confirmable (CON)**

- Requires acknowledgment from receiver
- Provides reliability
- Used by CoAP_Handler_Publish (default)

**Non-confirmable (NON)**

- No acknowledgment required
- Fire-and-forget
- Lower overhead for non-critical data

**Acknowledgment (ACK)**

- Confirms receipt of CON message
- Automatically sent by libcoap

**Reset (RST)**

- Indicates error or rejection
- Automatically handled by libcoap

## Response Codes

Common CoAP response codes:

**Success (2.xx)**

- 2.01 Created: Resource created
- 2.02 Deleted: Resource deleted
- 2.03 Valid: Resource is valid
- 2.04 Changed: Resource updated (typical PUT response)
- 2.05 Content: Resource content returned (typical GET response)

**Client Error (4.xx)**

- 4.00 Bad Request: Malformed request
- 4.01 Unauthorized: Authentication required
- 4.04 Not Found: Resource does not exist
- 4.05 Method Not Allowed: Method not supported for resource

**Server Error (5.xx)**

- 5.00 Internal Server Error: Server failure
- 5.03 Service Unavailable: Server temporarily unavailable

## Performance Characteristics

- Initialization Time: Less than 10 ms
- Message Send Time: 10-50 ms (including network round-trip)
- Subscription Setup Time: 10-50 ms
- Memory Usage per Context: Approximately 2-3 KB
- Memory Usage per Session: Approximately 0.5-1 KB
- Maximum Payload Size: 512 bytes (configurable via COAP_MAX_DATA_LEN)
- UDP Packet Size: Typically 1024 bytes maximum
- Typical Latency: 10-50 ms (local network)

## Debugging

Enable CoAP debug logging:

```c
// In sdkconfig or via menuconfig
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

Monitor CoAP activity:

```bash
idf.py monitor | grep "COAP"
```

Use network tools to inspect CoAP traffic:

```bash
# Capture CoAP packets (Linux/Mac)
tcpdump -i wlan0 -n udp port 5683

# Test CoAP server with coap-client (libcoap-bin package)
coap-client -m put coap://192.168.1.100:5683/api/test -e "test data"
```

## Error Handling

Handle CoAP failures gracefully:

```c
if (!CoAP_Handler_Init(&coap, server_ip, 5683, callback)) {
    printf("Init failed - check server IP and memory\n");
    return;
}

if (!CoAP_Handler_Start(&coap)) {
    printf("Start failed - check network connectivity\n");
    return;
}

int msg_id = CoAP_Handler_Publish(&coap, "/api/sensor/data", json, 0, true);
if (msg_id < 0) {
    printf("Publish failed - server unreachable or timeout\n");
    // Retry or queue data for later transmission
}
```

## Integration with DATALOGGER System

CoAP complements MQTT in the DATALOGGER architecture:

```c
void app_main(void) {
    // Initialize WiFi
    wifi_manager_init(NULL);
    wifi_manager_connect();
    wifi_manager_wait_connected(30000);

    // Initialize both MQTT and CoAP
    MQTT_Handler_Init(&mqtt, &mqtt_cfg);
    CoAP_Handler_Init(&coap, "192.168.1.100", 5683, on_coap_data);

    MQTT_Handler_Connect(&mqtt);
    CoAP_Handler_Start(&coap);

    // Use MQTT for reliable command delivery
    // Use CoAP for real-time sensor data with lower latency
}
```

## Limitations

- UDP-based protocol (no guaranteed delivery without confirmable messages)
- No built-in broker architecture (direct client-server only)
- Limited payload size (typically 512 bytes maximum)
- No built-in authentication in standard CoAP (use DTLS for security)
- Callback executes in network context (keep processing brief)
- Observable resources require persistent server state

## Configuration via Menuconfig

Configure CoAP parameters in ESP-IDF menuconfig:

```bash
idf.py menuconfig
```

Navigate to: **Component config → CoAP Handler Configuration**

Available options:

- Enable CoAP Protocol Support (default: disabled)
- CoAP Server IP Address
- CoAP Server Port (default: 5683)
- Maximum Retries (default: 3)
- Timeout (default: 5000 ms)

## Dependencies

- ESP-IDF libcoap library (espressif\_\_coap component)
- ESP-IDF logging (esp_log.h)
- lwIP UDP stack
- FreeRTOS (for task synchronization)

## References

- RFC 7252: The Constrained Application Protocol (CoAP)
- RFC 7641: Observing Resources in the Constrained Application Protocol (CoAP)
- ESP-IDF CoAP Documentation: https://github.com/espressif/idf-extra-components/tree/master/coap

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
