# Mosquitto MQTT Broker Configuration

This directory contains the configuration files and Docker deployment setup for the Eclipse Mosquitto MQTT broker used in the DATALOGGER system. The broker provides message routing between ESP32 gateway devices and web dashboard clients.

## Directory Structure

```
broker/
├── mosquitto.conf              # Main configuration file
├── config/
│   └── auth/
│       └── passwd.txt          # Bcrypt-hashed user credentials
├── data/                       # Runtime persistence database (excluded from version control)
│   └── mosquitto.db
└── log/                        # Broker log files (excluded from version control)
    └── mosquitto.log
```

Note: The data and log directories are created automatically at runtime and should not be committed to version control.

## Configuration Overview

The mosquitto.conf file defines the broker configuration with the following key settings:

- Port 1883: Standard MQTT protocol for ESP32 device connections
- Port 8083: WebSocket protocol for web dashboard connections
- Authentication: Username and password required for all connections
- Persistence: Message queue state saved to disk for reliability
- Logging: Combined console and file output for monitoring

## Configuration Features

| Feature               | Configuration | Description                             |
| --------------------- | ------------- | --------------------------------------- |
| **MQTT Protocol**     | Port 1883     | Standard MQTT for ESP32 and IoT devices |
| **WebSockets**        | Port 8083     | Web dashboard connectivity              |
| **Authentication**    | Required      | No anonymous connections allowed        |
| **Persistence**       | Enabled       | Messages survive broker restarts        |
| **Logging**           | Dual output   | Console + file logging                  |
| **Connection Limits** | Unlimited     | Configurable message queues             |

## Installation and Setup

### Prerequisites

- Docker Engine version 20.10 or later
- Docker Compose version 1.29 or later (optional)
- Basic familiarity with MQTT protocol
- Network connectivity for ESP32 and web clients

### Step 1: Create Directory Structure

Create the required directories for authentication, persistence, and logging:

```bash
mkdir -p broker/config/auth broker/data broker/log
```

### Step 2: Generate User Credentials

Create a user account with bcrypt password hashing. Replace 'DataLogger' with your desired username:

```bash
docker run --rm -v "$PWD/broker/config/auth:/work" eclipse-mosquitto:2 \
  mosquitto_passwd -c /work/passwd.txt DataLogger
```

You will be prompted to enter and confirm a password. The -c flag creates a new password file.

To add additional users without overwriting the existing file, omit the -c flag:

```bash
docker run --rm -v "$PWD/broker/config/auth:/work" eclipse-mosquitto:2 \
  mosquitto_passwd /work/passwd.txt additional_user
```

### Step 3: Start the Broker

#### Option A: Docker Run Command

Start the broker as a Docker container with volume mounts for configuration and data:

```bash
docker run -d --name mqtt-broker \
  -p 1883:1883 \
  -p 8083:8083 \
  -v "$PWD/broker/mosquitto.conf:/mosquitto/config/mosquitto.conf" \
  -v "$PWD/broker/config/auth:/mosquitto/config/auth" \
  -v "$PWD/broker/data:/mosquitto/data" \
  -v "$PWD/broker/log:/mosquitto/log" \
  eclipse-mosquitto:2
```

#### Option B: Docker Compose

Create a docker-compose.yml file in the project root:

```yaml
version: "3.8"
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: mqtt-broker
    ports:
      - "1883:1883"
      - "8083:8083"
    volumes:
      - ./broker/mosquitto.conf:/mosquitto/config/mosquitto.conf
      - ./broker/config/auth:/mosquitto/config/auth
      - ./broker/data:/mosquitto/data
      - ./broker/log:/mosquitto/log
    restart: unless-stopped
```

Start the broker:

```bash
docker-compose up -d
```

## Configuration Parameters

The mosquitto.conf file contains the following key parameters:

### Network Listeners

```
listener 1883 0.0.0.0
protocol mqtt
```

Binds standard MQTT to port 1883 on all network interfaces.

```
listener 8083 0.0.0.0
protocol websockets
```

Binds MQTT over WebSocket to port 8083 for web browser clients.

### Security Settings

```
allow_anonymous false
password_file /mosquitto/config/auth/passwd.txt
```

Disables anonymous connections and specifies the bcrypt password file location.

### Persistence Configuration

```
persistence true
persistence_location /mosquitto/data
```

Enables message persistence and specifies the database file location.

### Logging Configuration

```
log_type all
log_dest stdout
log_dest file /mosquitto/log/mosquitto.log
```

Enables all log types with output to both console and file.

### Connection Limits

```
max_connections -1
max_inflight_messages 20
max_queued_messages 1000
```

Sets connection and message queue limits. A value of -1 means unlimited connections.

## Testing and Verification

### Verify Broker Status

Check that the Docker container is running:

```bash
docker ps | grep mqtt-broker
```

Expected output should show the container running with ports 1883 and 8083 exposed.

### Test MQTT Protocol Connection

Open two terminal windows for publish/subscribe testing.

**Terminal 1 - Subscribe to test topic:**

```bash
mosquitto_sub -h localhost -p 1883 \
  -u DataLogger \
  -P your_password \
  -t "test/topic" \
  -v
```

**Terminal 2 - Publish to test topic:**

```bash
mosquitto_pub -h localhost -p 1883 \
  -u DataLogger \
  -P your_password \
  -t "test/topic" \
  -m "Hello MQTT"
```

The message should appear in Terminal 1.

### Test WebSocket Connection

The WebSocket endpoint is available at:

```
ws://localhost:8083
```

For remote access, replace localhost with the broker's IP address:

```
ws://192.168.1.100:8083
```

Web clients should connect using MQTT over WebSocket libraries such as MQTT.js.

### Additional Testing Examples

**Subscribe to sensor data:**

```bash
mosquitto_sub -h localhost -p 1883 -u DataLogger -P your_password -t "sensors/temperature" -v
```

**Publish sensor reading:**

```bash
mosquitto_pub -h localhost -p 1883 -u DataLogger -P your_password -t "sensors/temperature" -m "23.5"
```

## Monitoring

### View Broker Logs

Monitor real-time broker activity:

```bash
# View Docker container logs
docker logs -f mqtt-broker

# View file-based logs
tail -f broker/log/mosquitto.log
```

### Check Connection Status

Test broker connectivity:

```bash
mosquitto_pub -h localhost -p 1883 \
  -u DataLogger \
  -P your_password \
  -t "system/ping" \
  -m "test"
```

### Monitor Broker Statistics

Check system statistics using MQTT system topics:

```bash
# Connected clients count
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/clients/connected' -C 1

# Total messages received
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/messages/received' -C 1

# Memory usage
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/load/bytes/received' -C 1
```

### Resource Monitoring

Monitor broker resource usage:

```bash
# CPU and memory usage
docker stats mqtt-broker

# View current configuration
docker exec mqtt-broker cat /mosquitto/config/mosquitto.conf
```

### Restart Broker

Restart the broker container:

```bash
docker restart mqtt-broker
```

### Clean Restart

Remove the container and clear persistent data:

```bash
docker rm -f mqtt-broker
rm -rf broker/data/* broker/log/*
```

Then start the broker again using the docker run or docker-compose command.

## Integration Examples

### ESP32 Client Connection

#### Using ESP-IDF Framework

```c
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define MQTT_BROKER    "mqtt://192.168.1.100:1883"
#define MQTT_USER      "DataLogger"
#define MQTT_PASSWORD  "your_password"

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected");
        esp_mqtt_client_subscribe(client, "datalogger/esp32/command", 0);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Topic: %.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "Data: %.*s", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT Error");
        break;

    default:
        break;
    }
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER,
        .credentials.username = MQTT_USER,
        .credentials.authentication.password = MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
```

**Required Components in CMakeLists.txt:**

```cmake
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt esp_wifi nvs_flash
)
```

#### Using Arduino Framework (PlatformIO/Arduino IDE)

```c
#include <WiFi.h>
#include <PubSubClient.h>

const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;
const char* mqtt_user = "DataLogger";
const char* mqtt_password = "your_password";

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("datalogger/esp32/command");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
```

### Web Dashboard Connection

Example JavaScript code for web applications:

```javascript
const client = mqtt.connect("ws://192.168.1.100:8083", {
  clientId: "WebClient_" + Math.random().toString(16).substr(2, 8),
  username: "DataLogger",
  password: "your_password",
  clean: true,
  connectTimeout: 4000,
  reconnectPeriod: 2000,
});

client.on("connect", function () {
  console.log("MQTT Connected");
  client.subscribe("datalogger/stm32/#");
});

client.on("message", function (topic, payload) {
  console.log("Received:", topic, payload.toString());
});

client.on("error", function (error) {
  console.error("Connection error:", error);
});

client.on("close", function () {
  console.log("Connection closed");
});
```

## Troubleshooting

### Connection Refused Error

**Symptom:** Clients cannot connect to the broker

**Possible causes and solutions:**

1. Verify broker container is running:

```bash
docker ps | grep mqtt-broker
```

2. Check firewall rules allow ports 1883 and 8083:

```bash
# Ubuntu/Debian
sudo ufw status
sudo ufw allow 1883
sudo ufw allow 8083

# CentOS/RHEL
sudo firewall-cmd --list-ports
sudo firewall-cmd --add-port=1883/tcp --permanent
sudo firewall-cmd --add-port=8083/tcp --permanent
sudo firewall-cmd --reload
```

3. Verify broker is listening on all interfaces (0.0.0.0):

```bash
docker exec mqtt-broker netstat -tuln | grep -E '1883|8083'
```

4. Check Docker port mapping:

```bash
docker port mqtt-broker
```

### Authentication Failed Error

**Symptom:** Clients receive authentication errors

**Possible causes and solutions:**

1. Verify username and password are correct

2. Check passwd.txt file exists and is mounted correctly:

```bash
docker exec mqtt-broker cat /mosquitto/config/auth/passwd.txt
```

3. Confirm bcrypt hashing was used (not plaintext passwords):

```bash
# Password entries should look like:
# username:$7$101$randomhash...
```

4. Review broker logs for authentication failure messages:

```bash
docker logs mqtt-broker | grep -i auth
```

5. Regenerate password if needed:

```bash
docker run --rm -v "$PWD/broker/config/auth:/work" eclipse-mosquitto:2 \
  mosquitto_passwd -b /work/passwd.txt DataLogger new_password
docker restart mqtt-broker
```

### WebSocket Connection Failed

**Symptom:** Web dashboard cannot connect via WebSocket

**Possible causes and solutions:**

1. Confirm port 8083 is exposed and not blocked by firewall

2. Verify WebSocket protocol is enabled in mosquitto.conf:

```bash
docker exec mqtt-broker grep -A 1 "listener 8083" /mosquitto/config/mosquitto.conf
```

3. Check browser console for CORS or connection errors

4. Test WebSocket connection:

```bash
# Using wscat (install: npm install -g wscat)
wscat -c ws://localhost:8083
```

5. Verify WebSocket URL format in client code:

```javascript
// Correct format
ws://192.168.1.100:8083

// NOT
ws://192.168.1.100:8083/mqtt
```

### Persistence Not Working

**Symptom:** Messages are lost after broker restart

**Possible causes and solutions:**

1. Verify persistence is enabled in mosquitto.conf:

```bash
docker exec mqtt-broker grep persistence /mosquitto/config/mosquitto.conf
```

2. Check data directory is writable:

```bash
ls -la broker/data/
# Should show mosquitto.db file
```

3. Confirm volume mount for data directory is correct:

```bash
docker inspect mqtt-broker | grep -A 10 Mounts
```

4. Review broker logs for persistence-related errors:

```bash
docker logs mqtt-broker | grep -i persist
```

5. Manually verify persistence database:

```bash
# Check if file exists and is being updated
stat broker/data/mosquitto.db
```

### Missing Log Files

**Symptom:** Log files are not created

**Possible causes and solutions:**

1. Verify log directory volume mount is correct:

```bash
docker inspect mqtt-broker | grep -A 10 Mounts | grep log
```

2. Check log directory permissions:

```bash
ls -la broker/log/
chmod 755 broker/log/
```

3. Confirm log_dest file directive in mosquitto.conf:

```bash
docker exec mqtt-broker grep log_dest /mosquitto/config/mosquitto.conf
```

4. Review Docker container logs for error messages:

```bash
docker logs mqtt-broker --tail 50
```

### High Memory Usage

**Symptom:** Broker consuming excessive memory

**Possible causes and solutions:**

1. Check queued messages:

```bash
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/messages/stored' -C 1
```

2. Reduce message queue limits in mosquitto.conf:

```
max_queued_messages 1000
max_inflight_messages 20
```

3. Monitor client connections:

```bash
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/clients/connected' -C 1
```

4. Clear persistence database if needed:

```bash
docker stop mqtt-broker
rm broker/data/mosquitto.db
docker start mqtt-broker
```

### Common Commands

```bash
# Check broker status
docker ps | grep mqtt-broker

# View live logs
docker logs -f mqtt-broker

# Restart broker
docker restart mqtt-broker

# Stop broker
docker stop mqtt-broker

# Remove broker (keeps data)
docker rm mqtt-broker

# Clean restart (removes all data)
docker rm -f mqtt-broker && rm -rf broker/data/* broker/log/*

# Test connectivity
mosquitto_pub -h localhost -p 1883 -u DataLogger -P your_password -t "test/ping" -m "alive"
```

## Security

### Password Management

- **Use strong passwords** with mixed case, numbers, and special characters
- **Rotate passwords periodically** according to security policy
- **Store passwd.txt securely** and limit file permissions:

```bash
chmod 600 broker/config/auth/passwd.txt
```

- **Never commit passwd.txt** to public version control

### Network Security

- **Use TLS/SSL certificates** for encrypted connections in production
- **Implement firewall rules** to restrict broker access
- **Consider using VPN** for remote broker access
- **Enable topic-based access control lists (ACLs)** for fine-grained permissions

### Production Deployment

For production environments, enhance security with:

#### 1. TLS/SSL Encryption

Add to mosquitto.conf:

```
listener 8883 0.0.0.0
protocol mqtt
cafile /mosquitto/config/certs/ca.crt
certfile /mosquitto/config/certs/server.crt
keyfile /mosquitto/config/certs/server.key
require_certificate false
```

Generate certificates:

```bash
# Generate CA certificate
openssl req -new -x509 -days 3650 -extensions v3_ca \
  -keyout ca.key -out ca.crt

# Generate server certificate
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
  -CAcreateserial -out server.crt -days 3650
```

#### 2. Topic-Based Access Control (ACL)

Create acl.conf file:

```
# Read/write access for DataLogger user
user DataLogger
topic readwrite datalogger/#

# Read-only access for monitoring user
user monitor
topic read $SYS/#
topic read datalogger/#

# Pattern-based access
pattern readwrite datalogger/%u/#
```

Add to mosquitto.conf:

```
acl_file /mosquitto/config/acl.conf
```

#### 3. Connection Rate Limiting

```
max_connections 100
persistent_client_expiration 2h
```

#### 4. Additional Security Measures

- Enable log analysis and monitoring
- Regular security updates:

```bash
docker pull eclipse-mosquitto:2
docker-compose down
docker-compose up -d
```

- Use Docker secrets for sensitive data
- Implement network segmentation
- Enable audit logging

## Performance Tuning

### High-Traffic Scenarios

For systems with many clients or high message rates:

1. **Increase message queue limits:**

```
max_queued_messages 5000
max_inflight_messages 50
```

2. **Optimize persistence settings:**

```
persistence true
autosave_interval 300
autosave_on_changes false
```

3. **Adjust connection timeouts:**

```
keepalive_interval 60
max_keepalive 120
```

4. **Enable message compression** in client libraries

5. **Monitor memory usage** and adjust queue sizes accordingly:

```bash
docker stats mqtt-broker
```

### Resource Monitoring

Monitor broker resource usage:

```bash
# CPU and memory usage
docker stats mqtt-broker --no-stream

# Connection count
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/clients/connected' -C 1

# Message throughput
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/messages/received' -C 1
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/messages/sent' -C 1

# Subscription count
docker exec mqtt-broker mosquitto_sub -t '$SYS/broker/subscriptions/count' -C 1
```

### Optimization Tips

- Use QoS 0 for non-critical messages to reduce overhead
- Implement message batching in clients when possible
- Use retained messages wisely (they consume memory)
- Clean up disconnected clients regularly
- Monitor and optimize topic structure (avoid deep hierarchies)

## Backup and Recovery

### Manual Backup

#### Backup Persistent Data

Create dated backups of the persistence database:

```bash
cp broker/data/mosquitto.db backups/mosquitto_$(date +%Y%m%d_%H%M%S).db
```

#### Backup User Credentials

Create dated backups of the password file:

```bash
cp broker/config/auth/passwd.txt backups/passwd_$(date +%Y%m%d_%H%M%S).txt
```

#### Backup Configuration

```bash
cp broker/mosquitto.conf backups/mosquitto_$(date +%Y%m%d_%H%M%S).conf
```

### Automated Backup Script

Create a backup script for periodic execution:

```bash
#!/bin/bash
# backup_mqtt.sh

BACKUP_DIR="backups/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Backup persistence database
if [ -f "broker/data/mosquitto.db" ]; then
    cp broker/data/mosquitto.db "$BACKUP_DIR/"
    echo "Database backed up"
fi

# Backup password file
if [ -f "broker/config/auth/passwd.txt" ]; then
    cp broker/config/auth/passwd.txt "$BACKUP_DIR/"
    echo "Password file backed up"
fi

# Backup configuration
if [ -f "broker/mosquitto.conf" ]; then
    cp broker/mosquitto.conf "$BACKUP_DIR/"
    echo "Configuration backed up"
fi

# Compress backup
tar -czf "$BACKUP_DIR.tar.gz" "$BACKUP_DIR"
rm -rf "$BACKUP_DIR"

echo "Backup completed: $BACKUP_DIR.tar.gz"

# Keep only last 30 backups
ls -t backups/*.tar.gz | tail -n +31 | xargs -r rm
```

Make it executable and add to crontab:

```bash
chmod +x backup_mqtt.sh

# Run daily at 2 AM
crontab -e
# Add line:
0 2 * * * /path/to/backup_mqtt.sh
```

### Restore from Backup

Stop the broker and restore files:

```bash
# Stop broker
docker stop mqtt-broker

# Extract backup
tar -xzf backups/20241028_120000.tar.gz

# Restore files
cp backups/20241028_120000/mosquitto.db broker/data/
cp backups/20241028_120000/passwd.txt broker/config/auth/
cp backups/20241028_120000/mosquitto.conf broker/

# Restart broker
docker start mqtt-broker

# Verify
docker logs mqtt-broker
```

### Disaster Recovery

Complete disaster recovery procedure:

```bash
# 1. Install Docker (if needed)
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh

# 2. Restore directory structure
mkdir -p broker/config/auth broker/data broker/log

# 3. Restore from backup
tar -xzf mqtt_backup.tar.gz
cp mqtt_backup/* broker/config/auth/
cp mqtt_backup/mosquitto.db broker/data/

# 4. Start broker
docker run -d --name mqtt-broker \
  -p 1883:1883 \
  -p 8083:8083 \
  -v "$PWD/broker/mosquitto.conf:/mosquitto/config/mosquitto.conf" \
  -v "$PWD/broker/config/auth:/mosquitto/config/auth" \
  -v "$PWD/broker/data:/mosquitto/data" \
  -v "$PWD/broker/log:/mosquitto/log" \
  eclipse-mosquitto:2

# 5. Verify functionality
mosquitto_pub -h localhost -p 1883 -u DataLogger -P your_password -t "test" -m "recovery test"
```

## Runtime Data

- **Persistence Database**: `broker/data/mosquitto.db`
- **Authentication File**: `broker/config/auth/passwd.txt`
- **Log Files**: `broker/log/mosquitto.log`
- **Configuration**: `broker/mosquitto.conf`

## Additional Resources

- [Eclipse Mosquitto Documentation](https://mosquitto.org/documentation/)
- [MQTT Protocol Specification](https://mqtt.org/)
- [Docker Documentation](https://docs.docker.com/)
- [MQTT.js Library](https://github.com/mqttjs/MQTT.js)
- [ESP-IDF MQTT Component](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
