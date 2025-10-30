# Firmware - Technical Documentation

## Overview

This repository contains integrated technical documentation for the **DATALOGGER Firmware System**, which combines both **ESP32 (Network Gateway)** and **STM32 (Sensor Controller)** subsystems. Together, they form the complete IoT data acquisition and communication layer of the DATALOGGER platform.

The firmware is responsible for sensor data collection, storage, communication, and synchronization between local hardware (STM32) and remote services (ESP32 → MQTT broker → Web server).

## Documentation

* **Flow Diagrams**: [FLOW_DIAGRAM_FIRMWARE.md](FLOW_DIAGRAM_FIRMWARE.md) — Overall process and logic flows across the entire firmware system.
* **Sequence Diagrams**: [SEQUENCE_DIAGRAM_FIRMWARE.md](SEQUENCE_DIAGRAM_FIRMWARE.md) — Runtime interactions between STM32, ESP32, and external systems.
* **UML Diagrams**: [UML_DIAGRAM_FIRMWARE.md](UML_DIAGRAM_FIRMWARE.md) — System-wide architecture and lifecycle models.

### Diagram Categories

#### Flow Diagrams (`diagrams/flow_diagrams/`)

Illustrate firmware workflows, inter-module logic, and data flow between microcontrollers:

* `System_Startup_Initialization.puml` — Power-up and hardware configuration process
* `Single_Measurement_Flow.puml` — On-demand data acquisition cycle
* `Periodic_Measurement_Flow.puml` — Continuous sensor sampling with SD buffering
* `Button_Control_Flow.puml` — Local button interaction and mode toggling
* `Relay_Control_Flow.puml` — Relay state management from MQTT and physical input
* `MQTT_Connection_State_Management.puml` — ESP32 MQTT connection supervision
* `System_State_Synchronization.puml` — Data/state synchronization between STM32 and ESP32
* `SD_Card_Buffering_Transmission.puml` — Offline data logging and recovery upload
* `Error_Handling_Recovery.puml` — Fault detection and automatic recovery handling
* `Legend.puml` — Common notation and diagram symbol references

#### Sequence Diagrams (`diagrams/sequence_diagrams/`)

Describe timing, communication, and control sequences across firmware layers:

* `System_Initialization_Sequence.puml` — Complete startup flow from power-on to ready state
* `Communication_Protocol_Sequence.puml` — UART-based STM32–ESP32 message exchange
* `Single_Measurement_Sequence.puml` — Trigger–read–transmit process for single sensor command
* `Periodic_Measurement_Sequence.puml` — Automated multi-cycle measurement routine
* `Button_Press_Sequence.puml` — Button input event through ESP32 task processing
* `Relay_Control_Web_Interface.puml` — Remote relay control via MQTT/Web command
* `SD_Buffer_Transmission.puml` — SD buffer upload to cloud through ESP32 bridge
* `MQTT_Connection_State_Sync.puml` — MQTT connection updates sent to STM32
* `WiFi_Disconnection_Reconnection.puml` — ESP32 WiFi connection recovery and notification
* `Error_Recovery_Sensor_Failure.puml` — Sensor communication failure and recovery

#### UML Diagrams (`diagrams/uml_diagrams/`)

Define overall firmware architecture, object relationships, and resource flow:

* `System_Class_Diagram.puml` — Unified class and module relationship model
* `System_Lifecycle_State_Machine.puml` — Full operational lifecycle from startup to shutdown
* `Memory_Resource_Management.puml` — Shared memory and buffer management across modules

## Key System Features

* **Dual-MCU Architecture** — ESP32 (network/MQTT) + STM32 (sensor and SD logging)
* **Data Acquisition** — SHT3X temperature/humidity sensor management
* **Real-Time Clock** — DS3231 for precise timestamps
* **Storage System** — Circular-buffer SD logging and sync
* **Display Interface** — TFT LCD for local real-time visualization
* **Command Interface** — UART-based CLI for testing and configuration
* **MQTT Integration** — ESP32 handles publish/subscribe communication
* **Error Handling** — Automatic I2C, UART, and SD card fault recovery
* **State Synchronization** — Continuous STM32↔ESP32 data consistency mechanism

## Using the Documentation

1. Start from the main markdown files for an overview of system flow.
2. Open individual PlantUML diagrams for visual interpretation of process logic.
3. Analyze sequence diagrams to understand multi-device interactions.
4. Use UML diagrams for detailed architectural dependencies and resource relationships.

## PlantUML Files

All diagrams are written in `.puml` format. To view or edit them:

* Use [PlantUML Online Editor](http://www.plantuml.com/plantuml/)
* Install the PlantUML VS Code extension
* Or use command-line tools for local generation

## Purpose

This documentation serves as the central reference for:

* Understanding the **interaction between STM32 and ESP32 firmware layers**
* Supporting **development, debugging, and maintenance**
* Assisting in **code reviews and feature expansion**
* Onboarding new developers into the DATALOGGER firmware ecosystem

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
