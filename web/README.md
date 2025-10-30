# Web Dashboard

This directory contains the web-based monitoring and control interface for the DATALOGGER system. The dashboard provides real-time visualization of sensor data from STM32/ESP32 devices via MQTT WebSocket connections, with optional Firebase cloud storage integration.

## Directory Contents

```
web/
├── index.html                  # Main HTML structure and dashboard layout
├── style.css                   # All CSS styling and responsive design
├── app.js                      # JavaScript application logic and MQTT handling
└── README.md                   # This file
```

The application architecture follows a modular separation of concerns:

- index.html: HTML structure (855 lines)
- style.css: CSS styling (609 lines)
- app.js: JavaScript logic (2148 lines)

Note: index_original_backup.html is retained for reference but is not used in the current implementation.

## System Architecture

The web dashboard communicates with the data logger hardware through the MQTT broker:

```
Web Browser (Dashboard) ←→ MQTT Broker (WebSocket Port 8083) ←→ ESP32 Gateway ←→ STM32 Sensor Node
         ↓
Firebase Cloud Database (Optional)
```

Communication protocols:

- HTTP/HTTPS: Static dashboard file delivery
- WebSocket (MQTT): Real-time bidirectional sensor data and control commands
- Firebase API: Cloud data persistence and historical retrieval

## Configuration

### Running the Web Server

The dashboard consists of static HTML, CSS, and JavaScript files that can be served by any web server.

#### Method 1: Python HTTP Server (Recommended for Development)

```bash
cd web
python -m http.server 8080
```

Access the dashboard at http://localhost:8080

#### Method 2: Node.js HTTP Server

```bash
npx http-server -p 8080
```

#### Method 3: PHP Built-in Server

```bash
php -S localhost:8080
```

### Configuring MQTT Connection

Click the settings gear icon in the dashboard and configure:

- MQTT Broker IP Address: IP address of the Mosquitto broker (example: 192.168.1.100)
- WebSocket Port: MQTT WebSocket port (default: 8083)
- WebSocket Path: MQTT WebSocket endpoint path (default: /mqtt)
- Username: MQTT authentication username
- Password: MQTT authentication password

Click Save Configuration to apply settings.

### Configuring Firebase (Optional)

For cloud data persistence, configure Firebase settings:

- Database URL: Firebase Realtime Database URL (example: https://your-project.firebaseio.com/)
- API Key: Firebase project API key
- Project ID: Firebase project identifier

Firebase integration enables:

- Historical data storage
- Data persistence across browser sessions
- Remote data access

## Deployment

### Local Development

For local testing and development:

```bash
cd web
python -m http.server 8080
```

Access via http://localhost:8080 or http://[local-ip]:8080 for network testing.

### Production Deployment with Nginx

Create Nginx configuration file:

```nginx
server {
    listen 80;
    server_name datalogger.example.com;
    root /var/www/datalogger/web;
    index index.html;

    location / {
        try_files $uri $uri/ =404;
    }
}
```

Deploy files:

```bash
sudo cp -r web/* /var/www/datalogger/web/
sudo systemctl restart nginx
```

### Docker Containerization

Create Dockerfile:

```dockerfile
FROM nginx:alpine
COPY . /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
```

Build and run:

```bash
docker build -t datalogger-web .
docker run -d -p 80:80 datalogger-web
```

### Cloud Platform Deployment

**Netlify**

- Drag and drop the web directory to Netlify dashboard
- Automatic HTTPS and CDN distribution provided

**GitHub Pages**

```bash
git add web/*
git commit -m "Deploy dashboard"
git push origin main
```

Enable Pages in repository settings, select main branch and /web directory.

**Vercel**

```bash
cd web
npx vercel --prod
```

## MQTT Topic Structure

The dashboard subscribes to and publishes messages on the following MQTT topics:

### Subscribed Topics (Dashboard receives data)

| Topic                          | Data Type | Description                                  |
| ------------------------------ | --------- | -------------------------------------------- |
| `datalogger/esp32/sensor/data` | JSON      | Complete sensor data with timestamp and mode |
| `datalogger/esp32/status`      | JSON      | System status including relay and WiFi state |
| `datalogger/esp32/relay/state` | String    | Relay state changes (ON/OFF)                 |

Example sensor data message:

```json
{
  "mode": "PERIODIC",
  "timestamp": 1729699200,
  "temperature": 25.5,
  "humidity": 60.0
}
```

Example status message:

```json
{
  "device": "ON",
  "periodic": "OFF"
}
```

### Published Topics (Dashboard sends commands)

| Topic                      | Payload Example              | Description                   |
| -------------------------- | ---------------------------- | ----------------------------- |
| `datalogger/esp32/command` | `SINGLE`                     | Sensor measurement commands   |
| `datalogger/esp32/command` | `PERIODIC ON`                | Start periodic measurements   |
| `datalogger/esp32/command` | `PERIODIC OFF`               | Stop periodic measurements    |
| `datalogger/esp32/command` | `SET PERIODIC INTERVAL 5000` | Set measurement interval (ms) |
| `datalogger/esp32/relay`   | `ON`                         | Relay control commands        |
| `datalogger/esp32/relay`   | `OFF`                        | Relay control commands        |

### Command Examples

Single measurement:

```javascript
Topic: datalogger / esp32 / command;
Payload: SINGLE;
```

Start periodic measurement:

```javascript
Topic: datalogger/esp32/command
Payload: PERIODIC ON
```

Stop periodic measurement:

```javascript
Topic: datalogger/esp32/command
Payload: PERIODIC OFF
```

Set measurement interval to 10 seconds:

```javascript
Topic: datalogger/esp32/command
Payload: SET PERIODIC INTERVAL 10000
```

Relay control:

```javascript
Topic: datalogger / esp32 / relay;
Payload: ON; // or OFF
```

Set RTC time:

```javascript
Topic: datalogger/esp32/command
Payload: SET TIME 2025 10 23 14 30 00
```

Control sensor heater:

```javascript
Topic: datalogger/esp32/command
Payload: SHT3X HEATER ENABLE  // or DISABLE
```

## Dashboard Features and Usage

### Real-Time Data Visualization

The dashboard displays two synchronized charts:

- Temperature chart (left): Shows temperature readings in degrees Celsius
- Humidity chart (right): Shows relative humidity percentage

Charts automatically update when new data is received via MQTT. Historical data points are limited to 50 entries per chart to maintain performance.

### Statistical Analysis

Current statistics displayed for each measurement:

- Current Value: Most recent sensor reading
- Minimum: Lowest recorded value in current session
- Maximum: Highest recorded value in current session
- Average: Mean of all recorded values in current session

Statistics reset when charts are cleared.

### Device Control Panel

**Relay Control**

- DEVICE ON button: Activates relay output (GPIO4 on ESP32)
- DEVICE OFF button: Deactivates relay output

**Measurement Control**

- PERIODIC button: Starts periodic measurements at configured interval
- SINGLE SHOT button: Requests single immediate measurement
- Interval selector: Choose sampling period from 1 second to 60 minutes

### Connection Status Monitoring

Status indicators show real-time connection state:

- MQTT Status: WebSocket connection to MQTT broker
- Firebase Status: Connection to Firebase cloud database

Color-coded messages in terminal display:

- Green: Successful operations
- Yellow: Warnings or state changes
- Red: Errors or connection failures
- Blue: Informational messages

### Data Management

- Clear Charts button: Removes all data points and resets statistics
- Firebase integration: Automatically stores data if configured
- Browser local storage: Saves MQTT and Firebase configuration settings

## Browser Requirements

Minimum supported browser versions:

- Chrome 80 or later
- Firefox 75 or later
- Safari 13 or later
- Microsoft Edge 80 or later

Required browser features:

- WebSocket API
- ES6 JavaScript support
- HTML5 Canvas element
- CSS Grid and Flexbox
- Local Storage API
- Fetch API

## Security Considerations

### Production Deployment Security

For production environments, implement the following security measures:

**Use Encrypted Connections**

```javascript
// Configure WebSocket Secure (WSS) instead of WS
const mqttBrokerUrl = "wss://your-broker.example.com:443/mqtt";
```

**Enable TLS/SSL on MQTT Broker**
Configure Mosquitto with TLS certificates:

```
listener 8084
protocol websockets
cafile /path/to/ca.crt
certfile /path/to/server.crt
keyfile /path/to/server.key
```

**Implement Strong Authentication**

- Use strong passwords for MQTT broker authentication
- Consider implementing OAuth or JWT-based authentication
- Rotate credentials periodically

**Configure CORS Properly**
Restrict broker WebSocket access to authorized origins only.

**Input Validation**
All sensor data and user inputs are validated before processing.

**Rate Limiting**
Implement MQTT message rate limiting to prevent abuse.

## Troubleshooting

### MQTT Connection Issues

**WebSocket Connection Failed**

Symptoms: Cannot connect to MQTT broker, connection timeout

Solutions:

- Verify MQTT broker is running and WebSocket listener is active on port 8083
- Check broker IP address and port configuration in dashboard settings
- Test WebSocket connectivity using browser developer console
- Verify firewall allows WebSocket connections on port 8083
- Check broker authentication credentials

Command to verify broker WebSocket listener:

```bash
netstat -tlnp | grep 8083
```

**CORS Errors**

Symptoms: Browser console shows Cross-Origin Resource Sharing errors

Solutions:

- Verify mosquitto.conf has proper WebSocket configuration
- For development, ensure broker allows connections from dashboard origin
- Check browser developer console for specific CORS error messages
- Verify mosquitto.conf includes WebSocket protocol listener

**Authentication Failures**

Symptoms: Connection rejected by broker

Solutions:

- Verify username and password in dashboard settings match broker configuration
- Check broker/config/auth/passwd.txt file contains correct credentials
- Ensure allow_anonymous is set to false in mosquitto.conf
- Test authentication with mosquitto_pub command line tool

### Data Display Issues

**Charts Not Updating**

Symptoms: No new data points appear on charts

Solutions:

- Check browser JavaScript console for errors
- Verify MQTT connection status indicator shows connected
- Confirm ESP32 is sending data (check ESP32 serial monitor)
- Verify correct MQTT topics are subscribed
- Check Chart.js library loaded successfully

**Missing Historical Data**

Symptoms: Firebase data not loading

Solutions:

- Verify Firebase configuration in dashboard settings
- Check Firebase database rules allow read/write access
- Confirm Firebase API key has proper permissions
- Test Firebase connectivity independently
- Check browser console for Firebase authentication errors

### Performance Issues

**High Memory Usage**

Solutions:

- Reduce maximum chart data points (default 50) in app.js
- Clear charts periodically to free memory
- Disable Firebase integration if not needed
- Close unused browser tabs

**Slow Chart Rendering**

Solutions:

- Reduce chart animation duration
- Lower periodic sampling frequency
- Implement data throttling for high-rate updates
- Use chart update without animation: `chart.update('none')`

## Firebase Data Structure

When Firebase integration is enabled, sensor data is stored in the following structure:

```json
{
  "readings": {
    "YYYY-MM-DD": {
      "<created_at_timestamp_ms>": {
        "created_at": <timestamp_ms>,
        "device": "ESP32_01",
        "sensor": "SHT31",
        "mode": "periodic",
        "status": "success",
        "temp": 29.51,
        "humi": 76.35,
        "time": 946659605
      }
    }
  }
}
```

Each sensor data entry includes:

- temperature: Temperature reading in degrees Celsius (float)
- humidity: Relative humidity percentage (float)
- timestamp: Unix timestamp in seconds (integer)
- mode: Measurement mode (SINGLE or PERIODIC)

System status includes:

- device: Relay state (ON or OFF)
- periodic: Periodic measurement state (ON or OFF)
- wifi: WiFi connection status
- mqtt: MQTT connection status
- lastUpdate: Last update timestamp

## Performance Characteristics

| Metric               | Typical Value                           |
| -------------------- | --------------------------------------- |
| Memory Usage         | 10-15 MB                                |
| Network Bandwidth    | 1-2 KB per minute at 1 Hz sampling      |
| Firebase Storage     | Approximately 1 MB per day              |
| CPU Usage            | Minimal impact with optimized rendering |
| Maximum Chart Points | 50 data points per chart                |

Performance optimization tips:

- Limit chart data points to reduce memory usage
- Use chart update without animation for real-time data
- Implement data throttling for high-frequency sensors
- Cache Firebase queries when possible
- Minimize DOM manipulation frequency

## License

This web dashboard is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
