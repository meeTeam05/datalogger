# DATALOGGER - IoT Environmental Monitoring System

A production-grade IoT data acquisition system for environmental monitoring with real-time visualization, cloud storage, and remote device control.

## System Architecture

```
┌─────────────┐    I2C     ┌─────────────┐    UART      ┌─────────────┐    WiFi/MQTT    ┌──────────────┐
│   SHT3X     │────────────│    STM32    │──────────────│    ESP32    │─────────────────│ MQTT Broker  │
│   Sensor    │            │  F103C8T6   │   115200bps  │  Gateway    │  ws://x.x.x.x   │ (Mosquitto)  │
└─────────────┘            └──────┬──────┘              └──────┬──────┘                 └──────┬───────┘
                                  │                            │                               │
                           ┌──────┴──────┐              ┌──────┴──────┐                ┌───────┴────────┐
                           │  DS3231 RTC │              │ Relay GPIO  │                │  Web Dashboard │
                           │  SD Card    │              │   Control   │                │  + Firebase DB │
                           │  ILI9225    │              └─────────────┘                └────────────────┘
                           └─────────────┘
```

**Data flow:**

- **Upstream**: STM32 → UART → ESP32 → MQTT → Web Dashboard / Firebase
- **Downstream**: Web Dashboard → MQTT → ESP32 → UART → STM32
- **Offline**: STM32 buffers up to 204,800 records on SD card, syncs when ESP32 reconnects

## Components

| Component         | Role                                              |
| ----------------- | ------------------------------------------------- |
| **STM32F103C8T6** | Sensor acquisition, SD buffering, local display   |
| **ESP32**         | WiFi/MQTT gateway, relay control, UART bridge     |
| **Mosquitto**     | MQTT broker with WebSocket (ports 1883, 8083)     |
| **Web Dashboard** | Real-time charts, historical data, device control |

## Quick Start

### 1. MQTT Broker

```bash
# Create credentials
docker run --rm -v "$PWD/broker/config/auth:/work" eclipse-mosquitto:2 \
  mosquitto_passwd -c /work/passwd.txt DataLogger

# Start broker
docker run -d --name mqtt-broker \
  -p 1883:1883 -p 8083:8083 \
  -v "$PWD/broker/mosquitto.conf:/mosquitto/config/mosquitto.conf" \
  -v "$PWD/broker/config/auth:/mosquitto/config/auth" \
  -v "$PWD/broker/data:/mosquitto/data" \
  eclipse-mosquitto:2
```

### 2. STM32 Firmware

```bash
cd firmware/STM32/
make clean && make all
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/STM32_DATALOGGER.elf verify reset exit"
```

### 3. ESP32 Firmware

```bash
cd firmware/ESP32/
idf.py menuconfig   # Set WiFi SSID/password and MQTT broker URI
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### 4. Web Dashboard

```bash
cd web/ && python -m http.server 8080
# Open http://localhost:8080 and configure MQTT broker address
```

## Project Structure

```
datalogger/
├── broker/          # Mosquitto MQTT broker config & credentials
├── documents/       # Technical diagrams (flow, sequence, UML)
│   ├── esp32/
│   ├── firmware/
│   ├── stm32/
│   └── web/
├── firmware/
│   ├── STM32/       # STM32F103C8T6 firmware + 18 custom library modules
│   └── ESP32/       # ESP32 firmware + 9 IDF components
└── web/             # Web dashboard (HTML/JS/CSS + Firebase)
```

## Documentation

| Topic                   | Reference                                                                          |
| ----------------------- | ---------------------------------------------------------------------------------- |
| MQTT Broker setup       | [broker/README.md](broker/README.md)                                               |
| Firmware                | [firmware/README.md](firmware/README.md)                                           |
| STM32 firmware & pinout | [firmware/STM32/README.md](firmware/STM32/README.md)                               |
| STM32 library modules   | [firmware/STM32/Datalogger_Lib/README.md](firmware/STM32/Datalogger_Lib/README.md) |
| ESP32 gateway           | [firmware/ESP32/README.md](firmware/ESP32/README.md)                               |
| ESP32 components        | [firmware/ESP32/components/README.md](firmware/ESP32/components/README.md)         |
| Web dashboard           | [web/README.md](web/README.md)                                                     |
| Technical diagrams      | [documents/](documents/)                                                           |

## Video Demo

<div align="center">
<table>
  <tr>
    <td align="center">
      <a href="https://www.youtube.com/watch?v=Vg-FLoMRY9o">
        <img src="https://img.youtube.com/vi/Vg-FLoMRY9o/mqdefault.jpg" width="400"/>
      </a>
      <br/><b>Embedded Data Logger Demo</b>
    </td>
    <td align="center">
      <a href="https://www.youtube.com/watch?v=ait-4wsLwS8">
        <img src="https://img.youtube.com/vi/ait-4wsLwS8/mqdefault.jpg" width="400"/>
      </a>
      <br/><b>WiFi Reconnection Demo</b>
    </td>
  </tr>
</table>
</div>

## License

MIT License — see [LICENSE.md](LICENSE.md) for details.
