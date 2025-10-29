# DATALOGGER Documentation

This directory contains comprehensive technical documentation for the DATALOGGER IoT Environmental Monitoring System. All documentation is organized by system components with detailed flowcharts, sequence diagrams, and architecture diagrams.

## Documentation Structure

The documentation is organized into five main categories, each covering different aspects of the system:

### STM32 Firmware Documentation

Located in: `STM32/`

Documentation for the STM32F103C8T6 data acquisition firmware, covering sensor interfacing, SD card buffering, RTC management, display control, and UART communication.

- **FLOW_DIAGRAM_STM32.md** - 15 flowcharts covering:
  - System initialization and peripheral configuration
  - Sensor measurement and data processing flows
  - SD card buffering and offline data management
  - Command parsing and execution logic
  - Display update and user interface flows
  - Error handling and recovery procedures

- **SEQUENCE_DIAGRAM_STM32.md** - 10 sequence diagrams showing:
  - Hardware initialization sequences
  - Single and periodic measurement workflows
  - UART command reception and processing
  - SD card read/write operations
  - RTC time synchronization
  - Sensor error detection and recovery
  - Display update sequences

- **UML_CLASS_DIAGRAM_STM32.md** - 11 architecture diagrams including:
  - Library module relationships and dependencies
  - HAL driver integration architecture
  - Data manager component structure
  - Command handler class hierarchy
  - SD card manager architecture
  - Memory layout and buffer management
  - Pin configuration and peripheral mapping

Total: 36 comprehensive diagrams for STM32 firmware

### ESP32 Gateway Documentation

Located in: `ESP32/`

Documentation for the ESP32-WROOM-32 WiFi gateway firmware, covering WiFi management, MQTT communication, UART bridging, and relay control.

- **FLOW_DIAGRAM_ESP32.md** - 15 flowcharts covering:
  - System initialization and FreeRTOS task creation
  - WiFi connection and reconnection logic
  - MQTT broker connection and subscription flows
  - UART data reception and transmission
  - JSON parsing and message routing
  - Relay control command processing
  - Error handling and recovery mechanisms

- **SEQUENCE_DIAGRAM_ESP32.md** - 10 sequence diagrams showing:
  - System startup and component initialization
  - WiFi association and IP acquisition
  - MQTT connection and authentication
  - Message publish and subscribe workflows
  - UART-MQTT bidirectional bridging
  - Relay state change sequences
  - Network reconnection procedures

- **UML_CLASS_DIAGRAM_ESP32.md** - 11 architecture diagrams including:
  - Component architecture and relationships
  - FreeRTOS task structure and priorities
  - MQTT client configuration and handlers
  - WiFi manager state machine
  - UART bridge implementation
  - Message queue architecture
  - GPIO and relay control structure

Total: 36 comprehensive diagrams for ESP32 gateway

### Firmware System Documentation

Located in: `firmware/`

High-level documentation covering the complete firmware system, including STM32-ESP32 integration, communication protocols, and coordinated operations.

- **FLOW_DIAGRAM_FIRMWARE.md** - 31 flowcharts covering:
  - Complete system initialization sequence
  - End-to-end data acquisition pipeline
  - Command flow from web to sensors
  - Offline buffering and synchronization
  - Multi-mode operation (single/periodic)
  - Time synchronization mechanisms
  - Relay control integration
  - Error propagation and recovery

- **SEQUENCE_DIAGRAM_FIRMWARE.md** - 15 sequence diagrams showing:
  - Complete system startup coordination
  - STM32-ESP32 communication protocols
  - Data transmission pipelines
  - Command routing and execution
  - Offline data sync procedures
  - Network reconnection workflows
  - Relay control end-to-end flows

- **UML_DIAGRAM_FIRMWARE.md** - 15 architecture diagrams including:
  - System-level component architecture
  - Communication protocol stack
  - Data flow architecture
  - State machine coordination
  - Buffer management strategy
  - Message format specifications
  - Integration patterns

Total: 61 comprehensive diagrams for complete firmware system

### Web Dashboard Documentation

Located in: `web/`

Documentation for the browser-based real-time monitoring dashboard, covering MQTT integration, Firebase storage, chart visualization, and user interface.

- **FLOW_DIAGRAM_WEB.md** - 15 flowcharts covering:
  - Application initialization and setup
  - MQTT WebSocket connection flow
  - Firebase authentication and database initialization
  - Real-time data reception and processing
  - Chart rendering and update logic
  - Command transmission to devices
  - Historical data query and display
  - Data export functionality

- **SEQUENCE_DIAGRAM_WEB.md** - 10 sequence diagrams showing:
  - Dashboard loading and initialization
  - MQTT message subscribe and publish
  - Real-time sensor data reception
  - Firebase data storage and retrieval
  - Chart update sequences
  - User command execution flows
  - Data export workflows

- **UML_DIAGRAM_WEB.md** - 11 architecture diagrams including:
  - JavaScript application architecture
  - Manager class hierarchy and relationships
  - MQTT client integration
  - Firebase SDK integration
  - Chart.js component structure
  - UI controller architecture
  - State management design
  - Event handling flow

Total: 36 comprehensive diagrams for web dashboard

### System-Level Documentation

Located in: `System/`

Complete system documentation covering all four subsystems: STM32 Firmware, ESP32 Gateway, MQTT Broker, and Web Dashboard with their interactions and integration.

- **FLOW_DIAGRAM_SYSTEM.md** - 12 flowcharts covering:
  - Complete system initialization across all layers
  - End-to-end data acquisition and transmission
  - Command processing from web to hardware
  - Offline buffering and cloud synchronization
  - Comprehensive error detection and recovery
  - Periodic monitoring operation
  - Relay control system integration
  - Time synchronization across system
  - Web dashboard initialization
  - Firebase data storage workflows
  - Component health monitoring
  - Data export and download procedures

- **SEQUENCE_DIAGRAM_SYSTEM.md** - 15 sequence diagrams showing:
  - Complete system startup coordination
  - Single read command end-to-end flow
  - Periodic monitoring session lifecycle
  - Offline buffering and sync procedures
  - Relay control complete sequence
  - Time synchronization with NTP
  - Sensor error recovery workflows
  - Firebase query and display operations
  - System health monitoring cycles
  - Real-time chart update sequences
  - CSV data export procedures
  - System state synchronization
  - MQTT broker restart and recovery
  - LCD display update sequences
  - Error cascade and recovery flows

- **UML_DIAGRAM_SYSTEM.md** - 16 architecture diagrams including:
  - Complete system architecture (C4 Context Diagram)
  - System component relationships
  - Deployment architecture
  - Network topology
  - Data flow architecture
  - STM32 state machine
  - ESP32 state machine
  - Web dashboard state machine
  - MQTT topic hierarchy
  - Firebase database schema
  - Hardware pin mapping
  - Power distribution architecture
  - Complete system class diagram
  - Message lifecycle flow
  - Security architecture
  - Performance monitoring architecture

Total: 43 comprehensive diagrams for complete system integration

## Complete Documentation Statistics

Total comprehensive technical diagrams across all categories:

- Flowcharts: 88 diagrams
- Sequence Diagrams: 60 diagrams
- UML and Architecture Diagrams: 64 diagrams

Grand Total: 212 professional technical diagrams

## Documentation Purpose

Each documentation category serves specific purposes:

### For Developers

- **STM32 Documentation**: Understand firmware architecture, library modules, peripheral drivers, and low-level hardware interfacing
- **ESP32 Documentation**: Understand gateway firmware, FreeRTOS tasks, WiFi/MQTT protocols, and UART bridge implementation
- **Firmware Documentation**: Understand complete embedded system integration, communication protocols, and coordinated operations
- **Web Documentation**: Understand dashboard architecture, MQTT WebSocket integration, Firebase usage, and real-time UI updates

### For System Architects

- **System Documentation**: Understand complete system design, component interactions, data flows, deployment architecture, and integration patterns

### For Maintenance Engineers

- All flowcharts show operational procedures, error handling, and recovery mechanisms
- All sequence diagrams show interaction protocols and timing requirements
- All architecture diagrams show component dependencies and system structure

### For Quality Assurance

- Diagrams serve as specification for test case development
- Sequence diagrams define expected interaction patterns
- State machines define valid system states and transitions

## Diagram Conventions

All diagrams follow standard conventions for clarity and consistency:

### Flowchart Conventions

- **Green nodes**: Start and end points of processes
- **Blue nodes**: Normal processing steps
- **Yellow nodes**: Decision points and conditionals
- **Red nodes**: Error states and failure conditions
- **Arrows**: Process flow direction and data movement

### Sequence Diagram Conventions

- **Participants**: System components ordered left to right by hierarchy
- **Solid arrows**: Synchronous calls and data transfer
- **Dashed arrows**: Asynchronous events and responses
- **Activation boxes**: Component processing time
- **Notes**: Additional context and payload details

### Architecture Diagram Conventions

- **Boxes**: Components and modules
- **Arrows**: Dependencies and data flow
- **Subgraphs**: Logical groupings and subsystems
- **Colors**: Component categories and system layers
- **Line styles**: Different relationship types

## How to Use This Documentation

### For New Developers

1. Start with **System Documentation** to understand overall architecture
2. Read **Firmware Documentation** to understand STM32-ESP32 integration
3. Study **STM32 Documentation** for data acquisition details
4. Study **ESP32 Documentation** for communication gateway details
5. Review **Web Documentation** for dashboard implementation

### For Specific Feature Development

1. Identify the component involved (STM32, ESP32, Web)
2. Review relevant flowcharts for operational logic
3. Study sequence diagrams for interaction protocols
4. Reference architecture diagrams for code structure
5. Cross-reference with System documentation for integration impact

### For Debugging

1. Use sequence diagrams to verify message flow
2. Use flowcharts to identify decision points
3. Use state machines to confirm valid states
4. Use architecture diagrams to understand dependencies

### For System Integration

1. Review System documentation for complete data flows
2. Study MQTT topic hierarchy for message routing
3. Review deployment diagrams for configuration
4. Study network topology for connectivity requirements

## Documentation Maintenance

This documentation is actively maintained alongside code development:

- All diagrams use Mermaid syntax for version control and easy updates
- Diagrams are generated from text source for consistency
- Each code change should be reflected in relevant diagrams
- Documentation reviews are part of the code review process

## Related Documentation

For component-specific implementation details, refer to:

- **Firmware Implementation**: `../firmware/README.md`
- **STM32 Firmware Details**: `../firmware/STM32/README.md`
- **STM32 Library Modules**: `../firmware/STM32/Datalogger_Lib/README.md`
- **ESP32 Firmware Details**: `../firmware/ESP32/README.md`
- **ESP32 Components**: `../firmware/ESP32/components/README.md`
- **MQTT Broker Setup**: `../broker/README.md`
- **Web Dashboard Details**: `../web/README.md`

## Contributing to Documentation

When contributing to this documentation:

1. **Follow Mermaid syntax standards** for diagram consistency
2. **Use clear, descriptive labels** for all diagram elements
3. **Maintain consistent color schemes** across similar diagrams
4. **Add notes and comments** for complex interactions
5. **Update all three diagram types** when adding new features:
   - Add flowchart showing operational logic
   - Add sequence diagram showing interactions
   - Update architecture diagrams if structure changes
6. **Cross-reference between documents** for related information
7. **Test diagram rendering** before committing changes

## Documentation Tools

All diagrams in this documentation use Mermaid syntax, which can be:

- Rendered directly in GitHub markdown viewers
- Previewed in VS Code with Mermaid extensions
- Generated into images using Mermaid CLI
- Embedded in static documentation sites

## Contact and Support

For documentation questions or suggestions:

- Review existing diagrams in relevant category
- Check component-specific README files
- Refer to code comments for implementation details
- Create issue for documentation improvements

## License

This documentation is part of the DATALOGGER project and is licensed under the MIT License. See the root LICENSE.md file for details.
