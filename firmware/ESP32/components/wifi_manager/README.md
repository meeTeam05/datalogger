# WiFi Manager Component

A comprehensive WiFi management library for ESP32 with advanced configuration options and power management features.

## Features

- Easy WiFi connection with retry mechanism
- Power save mode support
- IPv6 support
- Signal strength monitoring
- Configurable scan methods and security thresholds
- Event callback system
- Comprehensive Kconfig integration

## Project Structure

```
wifi_manager/
├── CMakeLists.txt      # Component build configuration
├── Kconfig.projbuild   # WiFi configuration options
├── wifi_manager.c      # Implementation
├── wifi_manager.h      # Public API
└── README.md          # This file
```

## Configuration

Configure WiFi settings using menuconfig:

```bash
idf.py menuconfig
```

Navigate to **WiFi Connection Configuration** and configure:

### Basic Settings
- **WiFi SSID**: Your network name
- **WiFi Password**: Your network password  
- **Maximum WiFi Retry Attempts**: Connection retry limit (default: 5)

### Advanced Settings
- **WiFi Scan Method**: Fast scan (0) or All channel scan (1)
- **WiFi AP Sort Method**: Sort by RSSI (0) or Security (1)
- **WiFi Minimum RSSI**: Minimum signal strength threshold
- **WiFi Security Mode Threshold**: Minimum security level
- **WiFi Listen Interval**: Power save listen interval
- **WiFi Power Save Mode**: Enable/disable power saving
- **IPv6 Support**: Enable IPv6 connectivity
- **Connection Timeout**: Maximum connection wait time

## Usage

1. Configure WiFi credentials in menuconfig
2. Initialize WiFi manager with default or custom config
3. Call connect function to establish connection
4. Use callback system to handle connection events
5. Check connection status and get network information as needed

## API Functions

- `wifi_manager_init()` - Initialize WiFi manager
- `wifi_manager_connect()` - Connect to WiFi
- `wifi_manager_disconnect()` - Disconnect from WiFi  
- `wifi_manager_is_connected()` - Check connection status
- `wifi_manager_wait_connected()` - Wait for connection
- `wifi_manager_get_rssi()` - Get signal strength
- `wifi_manager_get_ip_addr()` - Get IP address
- `wifi_manager_deinit()` - Cleanup resources

## Build Commands

```bash
# Configure project
idf.py menuconfig

# Build project  
idf.py build

# Flash and monitor (Windows)
idf.py -p COM3 flash monitor

# Flash and monitor (Linux/Mac)
idf.py -p /dev/ttyUSB0 flash monitor
```

## License

MIT License - see project root for details.