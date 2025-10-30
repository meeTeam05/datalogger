# DATALOGGER - Technical Documentation

Comprehensive technical documentation for the DATALOGGER IoT Environmental Monitoring System

## Overview

This directory contains complete technical documentation for the DATALOGGER project - an integrated IoT system combining STM32 sensor acquisition, ESP32 network gateway, and web-based real-time monitoring dashboard. All documentation is organized by system components with detailed flowcharts, sequence diagrams, and architecture diagrams using PlantUML format.

## Documentation Structure

The documentation is organized into four main subsystems, each with three types of diagrams:

```
documents/
├── esp32/              # ESP32 Gateway Firmware
│   ├── FLOW_DIAGRAM_ESP32.md
│   ├── SEQUENCE_DIAGRAM_ESP32.md
│   ├── UML_CLASS_DIAGRAM_ESP32.md
│   └── diagrams/
│       ├── flow_diagrams/
│       ├── sequence_diagrams/
│       └── uml_diagrams/
│
├── firmware/           # Integrated Firmware System
│   ├── FLOW_DIAGRAM_FIRMWARE.md
│   ├── SEQUENCE_DIAGRAM_FIRMWARE.md
│   ├── UML_DIAGRAM_FIRMWARE.md
│   └── diagrams/
│       ├── flow_diagrams/
│       ├── sequence_diagrams/
│       └── uml_diagrams/
│
├── stm32/             # STM32 Sensor Controller
│   ├── FLOW_DIAGRAM_STM32.md
│   ├── SEQUENCE_DIAGRAM_STM32.md
│   ├── UML_CLASS_DIAGRAM_STM32.md
│   └── diagrams/
│       ├── flow_diagrams/
│       ├── sequence_diagrams/
│       └── uml_diagrams/
│
└── web/               # Web Dashboard Application
    ├── FLOW_DIAGRAM_WEB.md
    ├── SEQUENCE_DIAGRAM_WEB.md
    ├── UML_DIAGRAM_WEB.md
    └── diagrams/
        ├── flow_diagrams/
        ├── sequence_diagrams/
        └── uml_diagrams/
```

## Documentation Statistics

| Category     | Flow Diagrams | Sequence Diagrams | UML Diagrams | Total   |
| ------------ | ------------- | ----------------- | ------------ | ------- |
| **ESP32**    | 10            | 7                 | 5            | 22      |
| **STM32**    | 10            | 8                 | 11           | 29      |
| **Firmware** | 10            | 10                | 3            | 23      |
| **Web**      | 15            | 11                | 10           | 36      |
| **TOTAL**    | **45**        | **36**            | **29**       | **110** |

## Documentation by Subsystem

### 1. ESP32 Gateway Documentation

**Location:** `esp32/`

ESP32-WROOM-32 WiFi gateway firmware handling network communication, MQTT bridging, and relay control.

**Coverage:**

- WiFi connection and state management
- MQTT broker communication protocol
- UART bridge with STM32
- Relay control (local and remote)
- Button input handling
- Network reconnection logic
- State synchronization mechanisms

**Key Diagrams:**

- System startup and initialization flows
- MQTT connection state machine
- WiFi reconnection procedures
- Relay control sequences
- Button press processing
- Data routing architecture

### 2. STM32 Sensor Controller Documentation

**Location:** `stm32/`

STM32F103C8T6 firmware for sensor interfacing, data acquisition, SD card logging, and display control.

**Coverage:**

- SHT3X sensor communication (I2C)
- DS3231 RTC management
- SD card buffering and logging
- TFT LCD display control
- UART command interface
- Periodic measurement scheduling
- Error detection and recovery

**Key Diagrams:**

- Main application flow
- Command execution engine
- Single/periodic measurement flows
- MQTT data routing
- Error handling procedures
- Data manager architecture
- Sensor driver components

### 3. Integrated Firmware System Documentation

**Location:** `firmware/`

High-level documentation covering the complete firmware system and STM32-ESP32 integration.

**Coverage:**

- System-wide initialization
- STM32↔ESP32 communication protocol
- End-to-end data pipeline
- Offline data buffering and sync
- Multi-mode operation control
- Coordinated error recovery
- Resource management

**Key Diagrams:**

- Complete system startup
- Communication protocol sequences
- Data transmission pipelines
- SD buffer transmission flow
- System state synchronization
- Memory and resource management
- Unified lifecycle state machine

### 4. Web Dashboard Documentation

**Location:** `web/`

Browser-based monitoring interface with real-time visualization, MQTT integration, and Firebase storage.

**Coverage:**

- Application initialization
- MQTT WebSocket connection
- Firebase authentication and database
- Real-time chart rendering (Chart.js)
- Device control interface
- Historical data retrieval
- Data filtering and export
- Time synchronization

**Key Diagrams:**

- Application initialization flow
- MQTT message reception handling
- Chart data management
- Live data table updates
- Device control workflows
- Time sync mechanisms
- Firebase data structure
- UI component hierarchy

## How to Use This Documentation

### For New Developers

1. **Start with System Overview**

   - Read `firmware/README.md` for overall architecture
   - Understand STM32-ESP32 integration concept

2. **Study Each Subsystem**

   - Review `esp32/README.md` for network gateway
   - Review `stm32/README.md` for sensor controller
   - Review `web/README.md` for dashboard interface

3. **Deep Dive into Details**
   - Study flow diagrams for operational logic
   - Analyze sequence diagrams for interactions
   - Reference UML diagrams for code structure

### For Feature Development

1. Identify which subsystem(s) are affected
2. Review relevant flow diagrams for current logic
3. Study sequence diagrams for interaction protocols
4. Check UML diagrams for architectural constraints
5. Cross-reference with other subsystems for integration

### For Debugging

1. Use **sequence diagrams** to verify message flow
2. Use **flow diagrams** to identify decision points
3. Use **state machines** to confirm valid states
4. Use **architecture diagrams** to understand dependencies

### For System Integration

1. Review **firmware documentation** for complete data flows
2. Study **MQTT architecture** for message routing
3. Review **deployment diagrams** for configuration
4. Study **data flow diagrams** for end-to-end pipeline

## Diagram Conventions

### Flow Diagrams

- **Green nodes:** Start/end points
- **Blue nodes:** Normal processing steps
- **Yellow nodes:** Decision points and conditionals
- **Red nodes:** Error states and failure conditions
- **Orange nodes:** External system interactions
- **Arrows:** Process flow direction

### Sequence Diagrams

- **Participants:** Components ordered by hierarchy
- **Solid arrows:** Synchronous calls and data transfer
- **Dashed arrows:** Asynchronous events and responses
- **Activation boxes:** Processing time
- **Notes:** Additional context and payload details

### UML Diagrams

- **Boxes:** Components and modules
- **Arrows:** Dependencies and data flow
- **Packages:** Logical groupings and subsystems
- **Colors:** Component categories and system layers
- **Stereotypes:** Special component types

## Working with PlantUML

All diagrams use PlantUML syntax (`.puml` files) which can be:

### Viewing Options

- **GitHub:** Direct rendering in markdown viewers
- **VS Code:** Install PlantUML extension
- **Online:** [PlantUML Web Server](http://www.plantuml.com/plantuml/)
- **CLI:** Use PlantUML command-line tool

### Editing Diagrams

```bash
# Install PlantUML CLI
npm install -g node-plantuml

# Generate PNG from PUML
puml generate diagram.puml -o output.png

# Watch for changes
puml watch diagrams/
```

### VS Code Setup

1. Install "PlantUML" extension
2. Install "Markdown Preview Enhanced" (optional)
3. Press `Alt+D` to preview diagram
4. Export with `Ctrl+Shift+E`

## Documentation Maintenance

### When to Update Documentation

Update diagrams when:

- Adding new features or components
- Modifying control flow or logic
- Changing communication protocols
- Refactoring architecture
- Fixing critical bugs that affect flow

### Contributing Guidelines

1. **Follow PlantUML standards** for consistency
2. **Use descriptive labels** for all diagram elements
3. **Maintain color schemes** across similar diagrams
4. **Add explanatory notes** for complex interactions
5. **Update all three diagram types** when appropriate
6. **Test rendering** before committing changes
7. **Update markdown summaries** alongside diagrams

## Related Resources

### Component Implementation

- **Firmware Code:** `../firmware/`
- **STM32 Libraries:** `../firmware/STM32/Datalogger_Lib/`
- **ESP32 Components:** `../firmware/ESP32/components/`
- **Web Application:** `../web/`
- **MQTT Broker Setup:** `../broker/`

### External Documentation

- [PlantUML Language Reference](https://plantuml.com/guide)
- [STM32 HAL Documentation](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [ESP32 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [MQTT Protocol Specification](https://mqtt.org/mqtt-specification/)
- [Chart.js Documentation](https://www.chartjs.org/docs/)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
