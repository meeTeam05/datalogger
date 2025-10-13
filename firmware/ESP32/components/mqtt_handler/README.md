# MQTT Handler Component

MQTT client wrapper for ESP32 with support for MQTT v5.0 protocol.

## Features
- MQTT 5.0 protocol support
- Auto-reconnect capability
- QoS level support (0-2)
- Topic subscription management
- Callback system for incoming messages

## Configuration

Configure in your project:
- Broker URL (e.g., "mqtt://broker.example.com")
- Client credentials (optional username/password)
- QoS levels for publish/subscribe
- Message callback handler

## Usage

1. Initialize MQTT handler with broker settings
2. Register message callback function
3. Connect to MQTT broker
4. Subscribe to desired topics
5. Publish/receive messages as needed

## Key Functions

- `MQTT_Handler_Init()` - Initialize with broker URL and credentials
- `MQTT_Handler_Start()` - Connect to broker
- `MQTT_Handler_Subscribe()` - Subscribe to topic
- `MQTT_Handler_Publish()` - Send message to topic
- `MQTT_Handler_IsConnected()` - Check connection status

## License

MIT License - see project root for details.
