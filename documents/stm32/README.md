# STM32 Firmware - Technical Documentation

## Overview

This repository contains comprehensive technical documentation for the STM32-based DATALOGGER firmware. The STM32 microcontroller handles data acquisition, sensor interfacing, real-time clock management, SD card storage, and communication with the ESP32 module for network transmission.

## Documentation

* **Flow Diagrams**: [FLOW_DIAGRAM_STM32.md](FLOW_DIAGRAM_STM32.md) — System process flows and data logic
* **Sequence Diagrams**: [SEQUENCE_DIAGRAM_STM32.md](SEQUENCE_DIAGRAM_STM32.md) — Module interaction and execution timing
* **UML Class Diagrams**: [UML_CLASS_DIAGRAM_STM32.md](UML_CLASS_DIAGRAM_STM32.md) — Firmware architecture and component design

### Diagram Categories

#### Flow Diagrams (`diagrams/flow_diagrams/`)

Depict the execution logic, command flows, and decision-making processes:

* `Main_Application_Flow.puml` — System initialization and main loop
* `Command_Execution_Flow.puml` — Command parsing and function dispatch
* `SHT3X_Single_Measurement.puml` — Single measurement process
* `SHT3X_Periodic_Measurement.puml` — Periodic measurement operation
* `Periodic_Fetch_Decision.puml` — Measurement timing and scheduling
* `Periodic_Stop_Flow.puml` — Stopping periodic measurement safely
* `MQTT_Data_Routing.puml` — Data routing and MQTT state handling
* `Data_Manager_Print_Flow.puml` — Sensor data update and CLI print flow
* `Error_Handling_Flow.puml` — Error detection and recovery logic
* `Legend.puml` — Common diagram notation reference

#### Sequence Diagrams (`diagrams/sequence_diagrams/`)

Describe runtime interactions and communication sequences between firmware components:

* `System_Initialization_Sequence.puml` — Power-up and module initialization
* `UART_Interrupt_Command_Dispatch_Sequence.puml` — UART data reception and command handling
* `Single_Measurement_Sequence.puml` — One-shot SHT3X data measurement
* `Periodic_Measurement_Setup_Sequence.puml` — Enabling periodic measurement mode
* `Periodic_Measurement_Stop_Sequence.puml` — Stopping periodic mode operation
* `MQTT_State_Change_Notification_Sequence.puml` — MQTT connection state update via UART
* `I2C_Communication_Error_Recovery_Sequence.puml` — Sensor communication fault recovery
* `DataManager_State_Update_Print_Sequence.puml` — Updating and displaying new measurement results

#### UML Diagrams (`diagrams/uml_diagrams/`)

Show static architecture and inter-component relationships:

* `System_Class_Diagram.puml` — Overall system architecture
* `Command_Processing_Component.puml` — Command parser and execution engine
* `SHT3X_Sensor_Driver_Component.puml` — Sensor communication interface
* `DS3231_RTC_Driver_Component.puml` — Real-time clock driver
* `Data_Manager_Component.puml` — Central data storage and synchronization
* `Display_Component.puml` — LCD rendering and user interface
* `Output_Formatting_Component.puml` — JSON output and CLI printing
* `SD_Card_Manager_Component.puml` — SD card buffering and logging management
* `WiFi_MQTT_Manager_Component.puml` — MQTT connectivity status tracking
* `UART_RingBuffer_Component.puml` — UART communication buffer management
* `Object_Lifecycle.puml` — Initialization and deinitialization dependencies

## Key System Features

* **Sensor Management** — Integrated SHT3X temperature and humidity acquisition
* **RTC Handling** — DS3231-based timekeeping with synchronization via ESP32
* **Data Management** — Centralized buffer for current sensor readings and operating mode
* **Command Execution** — UART-driven CLI command system for configuration and debugging
* **Display Output** — TFT LCD output for real-time monitoring of measurements
* **SD Card Logging** — Buffered data storage with circular buffer architecture
* **MQTT State Tracking** — Remote communication readiness monitoring via ESP32
* **Error Recovery** — Automatic recovery for I2C and peripheral faults

## Using the Documentation

1. Review the markdown documents for overall architecture and flow
2. Study PlantUML diagrams for detailed runtime logic
3. Refer to sequence diagrams for communication event timing
4. Use UML diagrams for architectural and modular understanding

## PlantUML Files

All diagrams are written in PlantUML (`.puml`) format. To view or modify them:

* Use [PlantUML Online Editor](http://www.plantuml.com/plantuml/)
* Install the PlantUML extension in VS Code
* Use the PlantUML CLI tool for local rendering

## Purpose

This documentation serves as:

* Technical reference for firmware implementation
* Design documentation for development and review
* Onboarding material for firmware contributors
* Maintenance and troubleshooting resource

## License

This STM32 firmware is part of the **DATALOGGER Project**.
Refer to the project root **LICENSE.md** for licensing and usage terms.
