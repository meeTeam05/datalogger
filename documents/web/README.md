# Web Application - Technical Documentation

## Overview

This repository contains the complete technical documentation for the DATALOGGER Web Application, which serves as the monitoring, configuration, and control interface for the IoT firmware system (ESP32 + STM32). The web application enables real-time visualization, relay control, time synchronization, and historical data retrieval via MQTT and Firebase integration.

## Documentation

- **Flow Diagrams**: [FLOW_DIAGRAM_WEB.md](FLOW_DIAGRAM_WEB.md) — Application workflows, page logic, and data processing flows.
- **Sequence Diagrams**: [SEQUENCE_DIAGRAM_WEB.md](SEQUENCE_DIAGRAM_WEB.md) — Event-driven communication and user interaction sequences.
- **UML Diagrams**: [UML_DIAGRAM_WEB.md](UML_DIAGRAM_WEB.md) — Architectural design, data structure, and state management models.

### Diagram Categories

#### Flow Diagrams (`diagrams/flow_diagrams/`)

Illustrate the web application's internal logic, data handling, and user workflows:

- `Application_Initialization_Flow.puml` — Web app startup and component mount process
- `Navigation_Page_Switching_Flow.puml` — Page routing between Dashboard, Logs, and Settings
- `MQTT_Message_Reception_Flow.puml` — Handling of real-time messages from ESP32
- `Live_Data_Table_Management_Flow.puml` — Displaying and updating live sensor data
- `Chart_Data_Management_Flow.puml` — Real-time chart updates and dynamic scaling
- `Data_Management_Filtering_Flow.puml` — Filtering, sorting, and exporting data sets
- `Device_Control_Relay_Flow.puml` — MQTT-based relay ON/OFF control workflow
- `Periodic_Mode_Control_Flow.puml` — Switching periodic measurement mode remotely
- `Single_Read_Command_Flow.puml` — Triggering single measurement from web UI
- `Interval_Configuration_Flow.puml` — Configuring periodic interval values
- `Manual_Time_Setting_Flow.puml` — Manually setting RTC time via web
- `Time_Sync_From_Internet_Flow.puml` — Automatic time synchronization using NTP
- `Load_History_Firebase_Flow.puml` — Fetching historical data from Firebase storage
- `Logs_Page_Management_Flow.puml` — Displaying system logs and filtering events
- `Component_Health_Monitoring_Flow.puml` — Checking MQTT, database, and device health

#### Sequence Diagrams (`diagrams/sequence_diagrams/`)

Describe real-time communication and user interaction processes:

- `Application_Initialization.puml` — Page load, authentication, and MQTT connection
- `User_Toggles_Periodic_Mode.puml` — Periodic mode control through MQTT topic command
- `User_Sends_Single_Read_Command.puml` — On-demand single sensor read event
- `Relay_Control_Device_ON_OFF.puml` — Relay ON/OFF via MQTT message chain
- `Periodic_Sensor_Data_Arrives.puml` — Handling incoming data and updating UI tables
- `Time_Sync_from_Internet.puml` — Web retrieves internet time and syncs device
- `Manual_Time_Setting.puml` — User inputs manual date/time and sends to device
- `Data_Management_Filtering_Export.puml` — User filters data and exports CSV/JSON
- `Live_Data_Table_Realtime_Update.puml` — UI updates triggered by new MQTT payloads
- `Event_Handling_System.puml` — Global event bus propagation between components
- `Error_Handling_Sensor_Failure.puml` — Display of error and recovery feedback

#### UML Diagrams (`diagrams/uml_diagrams/`)

Define system architecture, data flow, and UI structure:

- `System_Architecture.puml` — High-level web architecture overview
- `Deployment_Architecture.puml` — Integration with backend (Firebase + MQTT broker)
- `Application_Class_Diagram.puml` — Core classes, data models, and service hierarchy
- `Data_Flow_Architecture.puml` — End-to-end data flow from MQTT to visualization
- `State_Management_Architecture.puml` — Application-level state and reactivity model
- `UI_Component_Hierarchy.puml` — UI composition tree and reusable modules
- `Chart_Integration_Architecture.puml` — Real-time chart rendering and data binding
- `Firebase_Database_Structure.puml` — Firebase collections and document schema
- `MQTT_Topics_Message_Architecture.puml` — Topic structure and message contracts
- `Settings_Configuration_Management.puml` — App configuration and persistence flow

## Key System Features

- **Real-Time Monitoring** — Display live temperature/humidity and system status via MQTT
- **Relay Control** — Remote control for device relays with feedback
- **Data Logging** — History visualization and export from Firebase or SD buffer
- **Configuration Interface** — Adjust system parameters and intervals dynamically
- **Time Synchronization** — Automatic NTP-based and manual time sync options
- **Error Feedback System** — Display alerts for disconnections, sensor failures, or sync issues
- **Responsive Design** — Adaptive layout for mobile and desktop
- **Chart and Table Visualization** — Real-time chart updates with smooth animation
- **Secure Communication** — MQTT over WebSocket with authentication

## Using the Documentation

1. Review `.md` overview files for high-level understanding of flows and components.
2. Explore individual `.puml` diagrams to visualize system logic and event handling.
3. Reference UML models to understand architecture, state management, and data structures.
4. Follow sequence diagrams to trace communication timing and component interaction.

## PlantUML Files

All diagrams are created in PlantUML (`.puml`) format. To view or modify:

- Use [PlantUML Online Editor](http://www.plantuml.com/plantuml/)
- Or open in Visual Studio Code with the PlantUML extension
- Run local PlantUML CLI for offline rendering

## Purpose

This documentation serves as a reference for:

- Frontend developers extending web functionalities
- Integration engineers working with MQTT and Firebase
- QA teams validating event and UI synchronization
- System architects maintaining overall DATALOGGER ecosystem consistency

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
