# Button Handler Component

This component provides button input handling functionality for the ESP32 in the DATALOGGER system. It implements debouncing, multiple button support, and callback-based event notification for physical button presses.

## Component Files

```
button_handler/
├── button_handler.h        # Public API header with structure and function declarations
├── button_handler.c        # Implementation of button handling logic
├── button_config.h         # Button configuration parameters (debounce time, etc.)
├── CMakeLists.txt          # ESP-IDF build configuration
├── component.mk            # Legacy build system support
├── Kconfig                 # Configuration menu options
└── README.md               # This file
```

## Overview

The Button Handler component provides a simple and reliable interface for handling physical button inputs on ESP32 GPIO pins. It manages button initialization, debouncing to eliminate spurious presses from mechanical contact bounce, and event notification through callbacks. The component supports multiple buttons simultaneously (up to 4 by default) and runs in a dedicated FreeRTOS task for non-blocking operation.

## Key Features

- Multiple button support (configurable, default 4 buttons)
- Hardware debouncing with configurable debounce time
- Internal pull-up resistor configuration
- Active-low button detection (button connects GPIO to GND)
- Callback-based event notification
- Non-blocking operation via FreeRTOS task
- Thread-safe button state monitoring
- Configurable polling interval
- Low CPU overhead

## Hardware Configuration

Buttons should be connected in active-low configuration:

```
       ESP32
        │
  GPIO ─┤  (Internal pull-up enabled)
        │
        ├─── Button Switch ───┐
        │                     │
        │                    GND
```

When the button is pressed, it connects the GPIO to GND, pulling the pin LOW. When released, the internal pull-up resistor pulls the pin HIGH.

## Button Handler Structure

```c
typedef struct {
    gpio_num_t gpio_num;              // GPIO pin number for button
    button_press_callback_t callback; // Function called on button press
    uint32_t last_press_time;         // Timestamp of last press (for debounce)
    bool initialized;                 // Initialization status flag
} button_handler_t;
```

## Configuration Parameters

```c
#define BUTTON_MAX_HANDLERS 4         // Maximum number of buttons supported

// In button_config.h (typical values):
#define BUTTON_DEBOUNCE_TIME_MS 50    // Debounce time in milliseconds
#define BUTTON_TASK_STACK_SIZE 2048   // Task stack size
#define BUTTON_TASK_PRIORITY 5        // Task priority
#define BUTTON_POLL_INTERVAL_MS 10    // Button polling interval
```

## API Functions

### Initialization

**Button_Init**

```c
bool Button_Init(gpio_num_t gpio_num, button_press_callback_t callback);
```

Initializes a button handler for the specified GPIO pin.

Parameters:

- gpio_num: GPIO pin number where button is connected
- callback: Function to call when button is pressed (can be NULL)

Returns:

- true: Initialization successful
- false: Initialization failed (maximum buttons reached, invalid GPIO, or GPIO configuration error)

This function configures the GPIO pin as input with internal pull-up resistor enabled. The button should connect the GPIO to GND when pressed. Up to BUTTON_MAX_HANDLERS buttons can be initialized.

### Task Management

**Button_StartTask**

```c
bool Button_StartTask(void);
```

Starts the button monitoring task.

Returns:

- true: Task started successfully
- false: Task start failed (task already running or creation error)

Creates a FreeRTOS task that continuously monitors all initialized buttons for state changes. The task polls buttons at the configured interval and triggers callbacks when button presses are detected and debounced.

**Button_StopTask**

```c
void Button_StopTask(void);
```

Stops the button monitoring task.

Terminates the button monitoring task and releases associated resources. Buttons remain configured but will not trigger callbacks until the task is restarted.

## Usage Example

### Basic Single Button

```c
#include "button_handler.h"

// Button press callback
void on_button_press(gpio_num_t gpio_num) {
    printf("Button on GPIO %d pressed!\n", gpio_num);

    // Take action based on button press
    if (gpio_num == GPIO_NUM_0) {
        printf("Boot button pressed - toggling WiFi\n");
        // Toggle WiFi on/off
    }
}

void app_main(void) {
    // Initialize button on GPIO 0 (boot button)
    if (!Button_Init(GPIO_NUM_0, on_button_press)) {
        printf("Failed to initialize button\n");
        return;
    }

    // Start button monitoring task
    if (!Button_StartTask()) {
        printf("Failed to start button task\n");
        return;
    }

    printf("Button handler initialized and running\n");
}
```

### Multiple Buttons with Different Actions

```c
#include "button_handler.h"

// Callback for button 1 (GPIO 0)
void on_button1_press(gpio_num_t gpio_num) {
    printf("Button 1 pressed - Reset WiFi\n");
    wifi_manager_disconnect();
    wifi_manager_connect();
}

// Callback for button 2 (GPIO 2)
void on_button2_press(gpio_num_t gpio_num) {
    printf("Button 2 pressed - Toggle Relay\n");
    Relay_Toggle(&relay);
}

// Callback for button 3 (GPIO 4)
void on_button3_press(gpio_num_t gpio_num) {
    printf("Button 3 pressed - Publish Test Data\n");
    MQTT_Handler_Publish(&mqtt, "datalogger/test", "Button 3 pressed", 0, 0, 0);
}

void app_main(void) {
    // Initialize multiple buttons
    Button_Init(GPIO_NUM_0, on_button1_press);
    Button_Init(GPIO_NUM_2, on_button2_press);
    Button_Init(GPIO_NUM_4, on_button3_press);

    // Start monitoring task (handles all buttons)
    Button_StartTask();

    printf("3 buttons initialized\n");
}
```

### Button with Shared Callback

```c
// Single callback handles multiple buttons
void on_any_button_press(gpio_num_t gpio_num) {
    switch (gpio_num) {
        case GPIO_NUM_0:
            printf("Button A pressed\n");
            // Action A
            break;

        case GPIO_NUM_2:
            printf("Button B pressed\n");
            // Action B
            break;

        case GPIO_NUM_4:
            printf("Button C pressed\n");
            // Action C
            break;

        default:
            printf("Unknown button: GPIO %d\n", gpio_num);
            break;
    }
}

void app_main(void) {
    // All buttons use same callback
    Button_Init(GPIO_NUM_0, on_any_button_press);
    Button_Init(GPIO_NUM_2, on_any_button_press);
    Button_Init(GPIO_NUM_4, on_any_button_press);

    Button_StartTask();
}
```

### Integration with DATALOGGER System

```c
void on_mode_button_press(gpio_num_t gpio_num) {
    static int mode = 0;

    mode = (mode + 1) % 3;  // Cycle through modes

    switch (mode) {
        case 0:
            printf("Mode: Normal Operation\n");
            // Resume normal data logging
            break;

        case 1:
            printf("Mode: Low Power\n");
            // Enter low power mode
            wifi_manager_disconnect();
            break;

        case 2:
            printf("Mode: Test/Debug\n");
            // Enable verbose logging
            break;
    }
}

void on_reset_button_press(gpio_num_t gpio_num) {
    printf("Reset button pressed - Restarting ESP32\n");

    // Publish notification before restart
    MQTT_Handler_Publish(&mqtt, "datalogger/esp32/status",
                        "Restarting by button", 0, 0, 1);

    vTaskDelay(pdMS_TO_TICKS(1000));  // Allow message to send
    esp_restart();
}

void app_main(void) {
    // Initialize WiFi, MQTT, sensors, etc.

    // Initialize buttons for user control
    Button_Init(GPIO_NUM_0, on_mode_button_press);   // Mode button
    Button_Init(GPIO_NUM_2, on_reset_button_press);  // Reset button

    Button_StartTask();
}
```

## Debouncing Mechanism

The component implements time-based debouncing:

1. When a button press is detected (GPIO goes LOW)
2. Current time is compared with last_press_time
3. If time difference < BUTTON_DEBOUNCE_TIME_MS, press is ignored
4. If time difference >= BUTTON_DEBOUNCE_TIME_MS, press is valid:
   - Callback is invoked
   - last_press_time is updated

This eliminates spurious triggers from mechanical contact bounce.

```
Button Press Physical Behavior:
  HIGH  ────┐         ┌────────
            │         │ (bounce)
            │  ┌──────┘
            └──┘
         Press detected here

Debounced Output:
  HIGH  ────┐
            │
            └────────────────
         Single clean press
```

## Task Operation

The button monitoring task operates as follows:

```c
void button_task(void *pvParameters) {
    while (task_running) {
        // Poll all initialized buttons
        for (int i = 0; i < num_buttons; i++) {
            // Read GPIO level
            int level = gpio_get_level(buttons[i].gpio_num);

            // Check for button press (active low)
            if (level == 0) {
                uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

                // Check debounce time
                if ((now - buttons[i].last_press_time) > BUTTON_DEBOUNCE_TIME_MS) {
                    // Valid press - invoke callback
                    if (buttons[i].callback) {
                        buttons[i].callback(buttons[i].gpio_num);
                    }

                    // Update last press time
                    buttons[i].last_press_time = now;
                }
            }
        }

        // Sleep before next poll
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_INTERVAL_MS));
    }
}
```

## Recommended GPIO Pins

Safe GPIO pins for button input on ESP32:

**Recommended:**

- GPIO0: Boot button (already has external pull-up on most boards)
- GPIO2: Safe for input
- GPIO4: Safe for input
- GPIO5: Safe for input
- GPIO12: Safe for input (if not used for flash voltage selection)
- GPIO13: Safe for input
- GPIO14: Safe for input
- GPIO15: Safe for input
- GPIO16-19: Safe for input
- GPIO21-23: Safe for input
- GPIO25-27: Safe for input
- GPIO32-39: Safe for input (note: GPIO34-39 are input-only)

**Avoid:**

- GPIO6-11: Connected to internal flash (do not use)
- GPIO1, GPIO3: UART0 TX/RX (used for programming/debugging)

**Input-Only Pins (no internal pull-up):**

- GPIO34-39: Input only, requires external pull-up resistor

## Configuration via button_config.h

Customize button behavior by editing button_config.h:

```c
// Debounce time in milliseconds
#define BUTTON_DEBOUNCE_TIME_MS 50

// Task configuration
#define BUTTON_TASK_STACK_SIZE 2048
#define BUTTON_TASK_PRIORITY 5
#define BUTTON_POLL_INTERVAL_MS 10

// Maximum number of buttons
#define BUTTON_MAX_HANDLERS 4
```

Adjust debounce time based on button quality:

- High-quality buttons: 20-30 ms
- Standard buttons: 50 ms (default)
- Low-quality buttons: 100 ms

## Performance Characteristics

- Initialization Time: Less than 1 ms per button
- Polling Interval: Configurable (default 10 ms)
- Debounce Time: Configurable (default 50 ms)
- Callback Execution Context: Button task (keep processing brief)
- Memory Usage: Approximately 32 bytes per button
- CPU Overhead: Minimal (< 0.1% with 10 ms polling)
- Maximum Buttons: Configurable (default 4, can be increased)

## Callback Execution Context

Important considerations for callback functions:

1. Callbacks execute in the button task context
2. Keep callback processing brief (< 10 ms recommended)
3. Avoid blocking operations in callbacks
4. Use queues or event groups for inter-task communication
5. Thread-safety: Callbacks may need synchronization if accessing shared data

Example of non-blocking callback:

```c
QueueHandle_t button_event_queue;

void on_button_press(gpio_num_t gpio_num) {
    // Send event to queue (non-blocking)
    xQueueSend(button_event_queue, &gpio_num, 0);
}

void main_task(void *pvParameters) {
    while (1) {
        gpio_num_t gpio;

        // Wait for button event
        if (xQueueReceive(button_event_queue, &gpio, portMAX_DELAY)) {
            // Process button press in main task context
            printf("Processing button %d\n", gpio);
            // Perform lengthy operations here
        }
    }
}
```

## Error Handling

Handle initialization and runtime errors:

```c
// Check initialization
if (!Button_Init(GPIO_NUM_0, callback)) {
    printf("Button init failed - possible causes:\n");
    printf("  - Maximum buttons already initialized\n");
    printf("  - Invalid GPIO number\n");
    printf("  - GPIO configuration error\n");
    return;
}

// Check task start
if (!Button_StartTask()) {
    printf("Button task start failed - possible causes:\n");
    printf("  - Task already running\n");
    printf("  - Insufficient memory for task creation\n");
    return;
}
```

## Limitations

- Maximum number of buttons limited by BUTTON_MAX_HANDLERS (default 4)
- Polling-based detection (not interrupt-based, slight latency)
- Callbacks execute in button task context (avoid blocking operations)
- No long-press or multi-click detection (single press only)
- No button release event notification
- Active-low configuration only (requires pull-up)

## Extending Functionality

To add long-press detection:

```c
typedef struct {
    gpio_num_t gpio_num;
    button_press_callback_t short_press_callback;
    button_press_callback_t long_press_callback;
    uint32_t press_start_time;
    bool press_in_progress;
} button_handler_extended_t;

// In task loop:
if (level == 0) {
    if (!button.press_in_progress) {
        button.press_start_time = now;
        button.press_in_progress = true;
    }
} else {
    if (button.press_in_progress) {
        uint32_t press_duration = now - button.press_start_time;

        if (press_duration > LONG_PRESS_THRESHOLD_MS) {
            button.long_press_callback(gpio_num);
        } else {
            button.short_press_callback(gpio_num);
        }

        button.press_in_progress = false;
    }
}
```

To add multi-click detection:

```c
typedef struct {
    uint32_t last_press_time;
    uint32_t click_count;
} button_click_tracker_t;

// Detect double-click
if ((now - tracker.last_press_time) < MULTI_CLICK_WINDOW_MS) {
    tracker.click_count++;

    if (tracker.click_count == 2) {
        // Double-click detected
        double_click_callback(gpio_num);
        tracker.click_count = 0;
    }
} else {
    tracker.click_count = 1;
}

tracker.last_press_time = now;
```

## Debugging

Enable debug logging for button events:

```c
// In sdkconfig or via menuconfig
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y

// In button_handler.c
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
```

Monitor button activity:

```bash
idf.py monitor | grep "BUTTON"
```

Test button hardware:

```c
// Simple GPIO test (before using button handler)
void test_button_gpio(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    while (1) {
        int level = gpio_get_level(GPIO_NUM_0);
        printf("GPIO 0 level: %d\n", level);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

## Configuration via Menuconfig

Configure button parameters in ESP-IDF menuconfig:

```bash
idf.py menuconfig
```

Navigate to: **Component config → Button Handler Configuration**

Available options:

- Relay button GPIO pin number
- Single command button GPIO pin number
- Periodic toggle button GPIO pin number
- Interval cycle button GPIO pin number
- Button debounce time (milliseconds)

## Dependencies

- ESP-IDF GPIO driver (driver/gpio.h)
- FreeRTOS (for task and timing)
- ESP-IDF logging (esp_log.h)

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
