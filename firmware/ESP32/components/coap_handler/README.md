# CoAP Handler Component

## Overview

CoAP (Constrained Application Protocol) handler component for ESP32. This component provides a lightweight implementation for IoT communication over UDP, designed as an alternative to MQTT for resource-constrained environments.

## Features

- Lightweight Protocol: UDP-based communication with minimal overhead
- RESTful API: GET, POST, PUT, DELETE methods support
- Small Footprint: Optimized for memory-constrained devices
- Easy Integration: Simple API for ESP32 applications
- Resource Discovery: Built-in service discovery support

## Status

Under Development - This component is planned for future implementation as an alternative protocol to MQTT.

## Planned Architecture

```
┌─────────────┐
│   ESP32     │
│             │
│  ┌───────┐  │
│  │ CoAP  │  │──► CoAP Server
│  │Handler│  │    (Port 5683)
│  └───────┘  │
│      │      │
│  ┌───────┐  │
│  │  UDP  │  │
│  └───────┘  │
└─────────────┘
```

## Usage (Planned)

```c
#include "coap_handler.h"

// Initialize CoAP handler
coap_handler_t coap;
CoAP_Handler_Init(&coap, "coap://server:5683", callback);

// Start CoAP client
CoAP_Handler_Start(&coap);

// Send GET request
CoAP_Handler_Get(&coap, "/sensor/temperature");

// Send POST request
CoAP_Handler_Post(&coap, "/actuator/relay", "ON");
```

## Configuration

Component can be enabled/disabled in `menuconfig`:

```
Component config → Enable CoAP → [*] Enable CoAP Handler
```

## API Reference

### Initialization
- `CoAP_Handler_Init()` - Initialize CoAP client
- `CoAP_Handler_Start()` - Start CoAP communication
- `CoAP_Handler_Stop()` - Stop CoAP client
- `CoAP_Handler_Deinit()` - Cleanup resources

### Communication
- `CoAP_Handler_Get()` - Send GET request
- `CoAP_Handler_Post()` - Send POST request
- `CoAP_Handler_Put()` - Send PUT request
- `CoAP_Handler_Delete()` - Send DELETE request

### Status
- `CoAP_Handler_IsConnected()` - Check connection status

## Dependencies

- ESP-IDF CoAP library
- FreeRTOS
- lwIP (UDP stack)

## Advantages over MQTT

| Feature | CoAP | MQTT |
|---------|------|------|
| Transport | UDP | TCP |
| Overhead | Low | Medium |
| Reliability | Optional | Guaranteed |
| Power | Very Low | Low |
| Complexity | Simple | Moderate |

## Use Cases

- Battery-powered sensors
- Lossy networks (LoRa, Zigbee)
- Real-time data streaming
- IoT edge devices
- Mesh networks

## Future Roadmap

- [ ] Implement CoAP client library
- [ ] Add DTLS security support
- [ ] Implement resource discovery
- [ ] Add observe mechanism
- [ ] Block-wise transfer support
- [ ] Integration with main application

## References

- [RFC 7252 - CoAP Specification](https://tools.ietf.org/html/rfc7252)
- [ESP-IDF CoAP Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/coap.html)

## License

MIT License - see project root for details.

## Contributing

This component is part of the IoT Datalogger project. Contributions welcome!
