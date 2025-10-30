# ESP32 Firmware - Technical Documentation

## Overview

This repository contains comprehensive technical documentation for an ESP32-based IoT control system. The system integrates WiFi connectivity, MQTT protocol, relay control, button handling, and STM32 communication for sensor data processing.

## Documentation

- **Flow Diagrams**: [FLOW_DIAGRAM_ESP32.md](FLOW_DIAGRAM_ESP32.md) - Process flows and system logic documentation
- **Sequence Diagrams**: [SEQUENCE_DIAGRAM_ESP32.md](SEQUENCE_DIAGRAM_ESP32.md) - Component interactions and timing diagrams
- **UML Class Diagrams**: [UML_CLASS_DIAGRAM_ESP32.md](UML_CLASS_DIAGRAM_ESP32.md) - System architecture and design patterns

### Diagram Categories

#### Flow Diagrams (`diagrams/flow_diagrams/`)

Process flows and system logic:

- `System_Startup_Flow.puml` - System initialization sequence
- `Main_Loop_Flow.puml` - Main program loop
- `MQTT_Connection_State_Machine.puml` - MQTT connection states
- `WiFi_State_Change_Flow.puml` - WiFi connection handling
- `Button_Press_Processing.puml` - Button event handling
- `Relay_State_Change_Flow.puml` - Relay control logic
- `MQTT_Message_Processing.puml` - MQTT message handling
- `STM32_Data_Processing.puml` - STM32 data reception
- `State_Update_and_Publish.puml` - State synchronization
- `Legend.puml` - Diagram notation reference

#### Sequence Diagrams (`diagrams/sequence_diagrams/`)

Component interactions and timing:

- `System_Initialization_Sequence.puml` - Startup sequence
- `Wifi_Connection_and_Mqtt_Startup.puml` - Network initialization
- `Wifi_Disconnection_and_Reconnection.puml` - Connection recovery
- `Mqtt_Reconnection_with_State_Sync.puml` - MQTT recovery with state sync
- `Relay_Control_via_Mqtt.puml` - Remote relay control
- `Button_Press_Sequence.puml` - Local button control
- `Sensor_Data_Reception_and_Publishing.puml` - Data flow from STM32

#### UML Diagrams (`diagrams/uml_diagrams/`)

System architecture and design:

- `Overall_System_Class.puml` - Complete class structure
- `Overall_State_Machine.puml` - System state transitions
- `Configuration_Constants.puml` - System configuration
- `Retry_Logic.puml` - Retry mechanisms
- `Object_Lifecycle.puml` - Object lifecycle management

## Key System Features

- **WiFi Management** - Automatic connection and reconnection handling
- **MQTT Communication** - Reliable pub/sub messaging with state synchronization
- **Relay Control** - Both local (button) and remote (MQTT) control
- **STM32 Integration** - Serial communication for sensor data
- **State Management** - Persistent state tracking and recovery

## Using the Documentation

1. Start with the markdown files for high-level understanding
2. Reference PlantUML diagrams for detailed technical flows
3. Use sequence diagrams to understand component interactions
4. Consult UML diagrams for architecture and design patterns

## PlantUML Files

All diagrams are written in PlantUML format (`.puml`). To view or edit:

- Use [PlantUML Online Editor](http://www.plantuml.com/plantuml/)
- Install PlantUML extension in VS Code
- Use PlantUML command-line tools

## Purpose

This documentation serves as:

- Technical reference for system implementation
- Design documentation for code reviews
- Onboarding material for new developers
- Maintenance and troubleshooting guide

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
