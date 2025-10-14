# SHT31 Temperature Monitor Dashboard

A real-time IoT web dashboard for monitoring SHT31 temperature and humidity sensors via MQTT WebSocket connections with Firebase data persistence.

## Overview

This professional-grade web application provides comprehensive monitoring and control capabilities for IoT sensor networks:

- **Real-time Data Visualization** - Live temperature/humidity charts with statistical analysis
- **MQTT WebSocket Communication** - Bi-directional communication with ESP32 devices  
- **Firebase Cloud Integration** - Persistent data storage and historical analysis
- **Device Control Interface** - Remote relay switching and sampling configuration
- **Responsive Design** - Optimized for desktop, tablet, and mobile devices
- **State Synchronization** - Automatic hardware/software state management

## Architecture

```
Web Dashboard ←→ MQTT Broker ←→ ESP32 ←→ STM32 + SHT31 Sensor
                      ↓
              Firebase Database
```

## Quick Start

### Prerequisites
- MQTT broker with WebSocket support (Mosquitto recommended)
- Modern web browser with WebSocket support
- Optional: Firebase account for data persistence

### 1. Setup Web Server
```bash
# Python (recommended for development)
python -m http.server 8080

# Node.js alternative
npx http-server -p 8080

# Access dashboard at http://localhost:8080
```

### 2. Configure MQTT Connection
Click the settings button (⚙️) and configure:
- **MQTT Broker IP**: Your broker's IP address
- **WebSocket Port**: Default 8083
- **WebSocket Path**: Default `/mqtt`
- **Credentials**: Username/password if authentication enabled

### 3. Firebase Setup (Optional)
For data persistence, configure Firebase:
- **Database URL**: Your Firebase Realtime Database URL
- **API Key**: Firebase project API key  
- **Project ID**: Firebase project identifier

## Deployment Options

### Development Environment
```bash
# Local testing
python -m http.server 8080

# Access via http://[local-ip]:8080 for network testing
```

### Production Deployment

#### Static File Hosting
```bash
# Nginx configuration
server {
    listen 80;
    root /var/www/sht31-dashboard;
    index index.html;
    
    location / {
        try_files $uri $uri/ =404;
    }
}
```

#### Docker Container
```dockerfile
FROM nginx:alpine
COPY . /usr/share/nginx/html
EXPOSE 80
```

```bash
docker build -t sht31-dashboard .
docker run -p 80:80 sht31-dashboard
```

#### Cloud Platforms

**Netlify (Recommended)**
- Drag and drop deployment
- Automatic HTTPS
- CDN distribution

**GitHub Pages**
```bash
# Create repository and push files
git add .
git commit -m "Initial dashboard deployment"
git push origin main

# Enable Pages in repository settings
```

**Vercel**
```bash
npx vercel --prod
```

## Configuration Reference

### MQTT Settings
```javascript
const MQTT_CONFIG = {
    host: '192.168.1.100',      // Broker IP address
    port: 8083,                 // WebSocket port
    path: '/mqtt',              // WebSocket endpoint
    username: 'DataLogger',     // MQTT username
    password: 'your-password'   // MQTT password (optional)
};
```

### Firebase Configuration
```javascript
const FIREBASE_CONFIG = {
    apiKey: "your-firebase-api-key",
    databaseURL: "https://your-project.firebaseio.com/",
    projectId: "your-project-id"
};
```

## MQTT Topic Structure

| Topic | Direction | Purpose | Example Payload |
|-------|-----------|---------|-----------------|
| `esp32/sensor/sht3x/command` | Web → ESP32 | Send sensor commands | `SHT3X SINGLE HIGH` |
| `esp32/control/relay` | Web → ESP32 | Device power control | `RELAY ON` |
| `esp32/sensor/sht3x/periodic/temperature` | ESP32 → Web | Continuous temperature data | `23.5` |
| `esp32/sensor/sht3x/periodic/humidity` | ESP32 → Web | Continuous humidity data | `65.2` |
| `esp32/sensor/sht3x/single/temperature` | ESP32 → Web | Single temperature reading | `24.1` |
| `esp32/sensor/sht3x/single/humidity` | ESP32 → Web | Single humidity reading | `58.7` |
| `esp32/state` | Bi-directional | Device state synchronization | `{"device":"ON","periodic":"OFF","rate":1}` |

## Features

### Real-Time Monitoring
- **Live Charts**: Temperature and humidity visualization with Chart.js
- **Configurable Sampling**: 0.5Hz to 10Hz periodic sampling rates
- **Statistical Analysis**: Real-time min/max/average calculations
- **Current Value Display**: Large, prominent current reading display

### Device Control
- **Power Management**: Remote relay switching for device control
- **Operating Modes**: 
  - Periodic sampling with configurable rates
  - Single-shot readings on demand
- **State Synchronization**: Automatic UI/hardware state management
- **Connection Monitoring**: Real-time MQTT and Firebase status

### Data Management
- **Cloud Storage**: Firebase Realtime Database integration
- **Historical Data**: Load and visualize past measurements
- **Data Export**: Chart data clearing and management
- **Persistent Settings**: Configuration saved across sessions

### User Interface
- **Responsive Design**: Desktop, tablet, and mobile optimized
- **Dark Terminal**: Retro-style status display with color-coded messages
- **Interactive Controls**: Touch-friendly buttons and controls
- **Visual Feedback**: Animations and state indicators
- **Modal Configuration**: Comprehensive settings interface

## Browser Compatibility

### Minimum Requirements
- **Chrome**: Version 80+
- **Firefox**: Version 75+
- **Safari**: Version 13+
- **Edge**: Version 80+

### Required Features
- WebSocket API support
- ES6 JavaScript compatibility
- HTML5 Canvas for charts
- CSS Grid and Flexbox
- Local Storage API

## Security Considerations

### Production Security
```javascript
// Secure WebSocket configuration
const SECURE_MQTT = {
    protocol: 'wss://',           // Encrypted WebSocket
    host: 'your-broker.com',
    port: 443,                    // Standard HTTPS port
    username: process.env.MQTT_USER,
    password: process.env.MQTT_PASS
};
```

### Best Practices
- **HTTPS/WSS Only**: Use encrypted connections in production
- **Strong Authentication**: Implement robust MQTT broker authentication
- **CORS Configuration**: Properly configure broker CORS settings
- **Input Validation**: Validate all sensor data and user inputs
- **Rate Limiting**: Implement MQTT topic rate limiting
- **Secure Headers**: Add security headers for web deployment

## Troubleshooting

### Connection Issues

**MQTT WebSocket Connection Failed**
```bash
# Verify broker WebSocket listener
netstat -tlnp | grep 8083

# Test WebSocket connectivity
curl -i -N -H "Connection: Upgrade" \
     -H "Upgrade: websocket" \
     -H "Sec-WebSocket-Key: test" \
     -H "Sec-WebSocket-Version: 13" \
     http://localhost:8083/mqtt
```

**CORS Errors**
```conf
# Add to mosquitto.conf
listener 8083
protocol websockets
allow_anonymous false
password_file /mosquitto/config/auth/passwd.txt

# For development only (not recommended for production)
# http_dir /usr/share/mosquitto/www
```

**Firebase Connection Issues**
- Verify Firebase project configuration
- Check database rules and permissions
- Ensure API key has proper permissions
- Verify network connectivity to Firebase

### Data Issues

**Charts Not Updating**
- Check browser console for JavaScript errors
- Verify MQTT message reception
- Confirm Chart.js library loading
- Check canvas element initialization

**Missing Historical Data**
- Verify Firebase authentication
- Check database read/write permissions
- Confirm data structure matches expected format
- Test Firebase connectivity independently

### Performance Issues

**High Memory Usage**
```javascript
// Limit chart data points
const maxDataPoints = 50;  // Reduce if needed

// Use efficient chart updates
chart.update('none');  // Skip animations
```

**Slow Chart Rendering**
- Reduce chart animation duration
- Limit concurrent data points
- Implement data throttling for high-frequency updates

## Development

### Project Structure
```
web/
├── index.html          # Main dashboard interface
├── style.css           # Responsive styling and animations
├── script.js           # Application logic and MQTT handling
└── Web.md              # This documentation
```

### Key Components

**MQTT Client Management**
- Connection handling with automatic reconnection
- Topic subscription and message routing
- State synchronization with hardware

**Chart Visualization**
- Dual-chart layout for temperature/humidity
- Real-time data streaming
- Statistical calculations and display

**Firebase Integration**
- Cloud data persistence
- Historical data retrieval
- Connection state management

**Device Control Interface**
- Relay power switching
- Sampling mode configuration
- Frame rate adjustment

### Adding New Features

**Additional Sensor Support**
1. Extend MQTT topic configuration
2. Add new chart instances
3. Update message parsing logic
4. Modify Firebase data structure

**Enhanced Visualization**
1. Create new Chart.js configurations
2. Add statistical analysis functions
3. Implement data export capabilities
4. Add customizable chart options

**Mobile App Integration**
1. Implement PWA manifest
2. Add service worker for offline support
3. Optimize touch interactions
4. Add push notification support

## Performance Optimization

### Resource Management
- **Memory**: ~10-15MB typical usage
- **Network**: ~1-2KB/minute at 1Hz sampling
- **Storage**: ~1MB/day Firebase data
- **CPU**: Minimal impact with optimized rendering

### Best Practices
- Limit chart data points (default: 50)
- Use chart animation skipping for real-time data
- Implement data throttling for high-frequency sensors
- Cache Firebase queries when possible
- Minimize DOM manipulation frequency

## API Reference

### MQTT Commands
```javascript
// Device control
publishMQTT('esp32/control/relay', 'RELAY ON');
publishMQTT('esp32/control/relay', 'RELAY OFF');

// Sensor commands
publishMQTT('esp32/sensor/sht3x/command', 'SHT3X SINGLE HIGH');
publishMQTT('esp32/sensor/sht3x/command', 'SHT3X PERIODIC 1 HIGH');
publishMQTT('esp32/sensor/sht3x/command', 'SHT3X PERIODIC STOP');

// State synchronization
publishMQTT('esp32/state', 'REQUEST');
```

### Firebase Data Structure
```json
{
  "sht31": {
    "temperature": {
      "timestamp": {
        "value": 23.5,
        "timestamp": 1640995200000,
        "date": "2021-12-31T12:00:00.000Z",
        "source": "periodic"
      }
    },
    "humidity": {
      "timestamp": {
        "value": 65.2,
        "timestamp": 1640995200000,
        "date": "2021-12-31T12:00:00.000Z",
        "source": "periodic"
      }
    }
  }
}
```

## Contributing

### Development Setup
1. Fork the repository
2. Clone locally: `git clone <your-fork>`
3. Create feature branch: `git checkout -b feature-name`
4. Make changes and test thoroughly
5. Submit pull request with detailed description

### Code Style
- Use consistent JavaScript ES6+ syntax
- Follow responsive CSS design patterns
- Include comprehensive error handling
- Add appropriate code comments
- Test across multiple browsers and devices

## License

MIT License - see project root for details.