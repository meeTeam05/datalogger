# Display Library

Provides an interface for managing visual output on the system’s TFT LCD display.

## Overview

The Display Library provides high-level display functions for the Datalogger system's ILI9225 TFT LCD (176x220 pixels). It manages the display of sensor data, system status, time, and date with formatted layouts and real-time updates.

## Files

- **display.c**: Display management and update functions
- **display.h**: Display API declarations

## Hardware Configuration

### LCD Specifications

- **Controller**: ILI9225
- **Resolution**: 176 × 220 pixels
- **Interface**: SPI
- **Color Depth**: 16-bit RGB565
- **Orientation**: Portrait mode

### Display Layout

```
+--------------------------------------------------+
|                     14:28:01                     |  ← Time (y = 20)
|                   Th7 25/10/2025                 |  ← Date (y = 40)
|                                                  |
|    28.5°C                     67% RH             |  ← Sensor Data (y = 70)
|                                                  |
|    System       Periodic                         |  ← System Status (y = 110)
|    MQTT         Interval: 1s                     |  ← Communication & Mode Info (y = 130)
|                                                  |
+--------------------------------------------------+
        176 pixels (width) × 220 pixels (height)
```

## Color Definitions

Standard RGB565 colors used:

```c
#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_YELLOW   0xFFE0
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F
```

## Core Functions

### Display Initialization

```c
void DISPLAY_Init(void);
```

Initializes the display hardware and draws initial layout.

**Operations**:

1. Initialize ILI9225 controller
2. Clear screen (black background)
3. Draw title text "DATALOGGER"
4. Draw static labels for temperature, humidity, mode, time, date
5. Set initial values to 0.0

**Usage Example**:

```c
int main(void)
{
    // After HAL and peripheral initialization
    DISPLAY_Init();

    // Display is now ready for updates
}
```

**Typical Execution Time**: 50-100ms (includes full screen clear)

### Display Update

```c
void DISPLAY_Update(void);
```

Updates all dynamic display fields with current sensor data, mode, and time.

**Data Sources**:

- Temperature/Humidity: `DATA_MANAGER_Get_Temperature()`, `DATA_MANAGER_Get_Humidity()`
- Operating mode: `DATA_MANAGER_Get_Mode()`
- Time/Date: `DS3231_ReadTime()`, `DS3231_ReadDate()`

**Update Fields**:

1. Temperature value (format: "Temp: XX.X C")
2. Humidity value (format: "Hum: XX.X %")
3. Operating mode (text: "SINGLE" or "PERIODIC")
4. Current time (format: "HH:MM:SS")
5. Current date (format: "YYYY-MM-DD")

**Usage Example**:

```c
// In main loop
while (1)
{
    // Update display every 500ms
    static uint32_t lastUpdate = 0;
    if (HAL_GetTick() - lastUpdate >= 500)
    {
        lastUpdate = HAL_GetTick();
        DISPLAY_Update();
    }
}
```

**Typical Execution Time**: 20-30ms (text rendering only, no clear)

## Display Layout Details

### Title Section

**Position**: (10, 20)
**Font**: Font_11x18 (large)
**Color**: COLOR_WHITE
**Text**: "DATALOGGER"

```c
ILI9225_DrawText(10, 20, "DATALOGGER", COLOR_WHITE);
```

### Temperature Display

**Position**: (10, 60)
**Font**: Font_11x18
**Color**: COLOR_WHITE
**Format**: "Temp: XX.X C"

**Example Values**:

- "Temp: 25.5 C"
- "Temp: -10.2 C"
- "Temp: 125.0 C"

```c
char buffer[32];
float temp = DATA_MANAGER_Get_Temperature();
snprintf(buffer, sizeof(buffer), "Temp: %.1f C", temp);
ILI9225_DrawText(10, 60, buffer, COLOR_WHITE);
```

### Humidity Display

**Position**: (10, 80)
**Font**: Font_11x18
**Color**: COLOR_WHITE
**Format**: "Hum: XX.X %"

**Example Values**:

- "Hum: 65.2 %"
- "Hum: 100.0 %"
- "Hum: 0.0 %"

```c
char buffer[32];
float hum = DATA_MANAGER_Get_Humidity();
snprintf(buffer, sizeof(buffer), "Hum: %.1f %%", hum);
ILI9225_DrawText(10, 80, buffer, COLOR_WHITE);
```

### Mode Display

**Position**: (10, 120)
**Font**: Font_11x18
**Color**:

- COLOR_GREEN for "PERIODIC"
- COLOR_YELLOW for "SINGLE"

**Text**:

- "Mode: PERIODIC" (when in periodic mode)
- "Mode: SINGLE" (when in single mode)

```c
char buffer[32];
uint8_t mode = DATA_MANAGER_Get_Mode();
const char *modeStr = (mode == PERIODIC_MODE) ? "PERIODIC" : "SINGLE";
uint16_t color = (mode == PERIODIC_MODE) ? COLOR_GREEN : COLOR_YELLOW;
snprintf(buffer, sizeof(buffer), "Mode: %s", modeStr);
ILI9225_DrawText(10, 120, buffer, color);
```

### Time Display

**Position**: (10, 160)
**Font**: Font_11x18
**Color**: COLOR_CYAN
**Format**: "HH:MM:SS"

**Example**: "14:35:22"

```c
char buffer[32];
DS3231_Time time;
DS3231_ReadTime(&time);
snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
         time.hour, time.minute, time.second);
ILI9225_DrawText(10, 160, buffer, COLOR_CYAN);
```

### Date Display

**Position**: (10, 180)
**Font**: Font_11x18
**Color**: COLOR_CYAN
**Format**: "YYYY-MM-DD"

**Example**: "2024-03-15"

```c
char buffer[32];
DS3231_Date date;
DS3231_ReadDate(&date);
snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
         date.year, date.month, date.date);
ILI9225_DrawText(10, 180, buffer, COLOR_CYAN);
```

## Font Selection

The display library uses fonts from the fonts library:

### Font_7x10

- **Size**: 7 pixels wide × 10 pixels tall
- **Use**: Small text (not used in current layout)
- **Characters**: ASCII 32-126

### Font_11x18

- **Size**: 11 pixels wide × 18 pixels tall
- **Use**: All main display text (current default)
- **Characters**: ASCII 32-126
- **Readability**: Excellent at 176x220 resolution

**Font Selection Rationale**:

- Font_11x18 provides good readability
- 11 pixels width allows ~16 characters per line (176/11 = 16)
- 18 pixels height allows ~12 lines (220/18 = 12)
- Adequate for displaying sensor values and status

## Text Rendering

### Background Clearing

The display uses **overwrite mode**:

- Old text is cleared by drawing background color
- New text is drawn over cleared area
- No full screen refresh needed

**Method 1**: Clear specific region

```c
// Clear old temperature value area
ILI9225_FillRectangle(10, 60, 166, 78, COLOR_BLACK);

// Draw new value
ILI9225_DrawText(10, 60, "Temp: 26.3 C", COLOR_WHITE);
```

**Method 2**: Fixed-width fields

```c
// Always use same format width
snprintf(buffer, sizeof(buffer), "Temp: %5.1f C", temp);
// "Temp:  25.5 C" - consistent spacing
```

### Anti-Flicker Technique

Current implementation minimizes flicker:

1. Read all data sources first
2. Format all strings in buffers
3. Draw all text updates in sequence
4. No delays between updates

**Example**:

```c
void DISPLAY_Update(void)
{
    // Read all data
    float temp = DATA_MANAGER_Get_Temperature();
    float hum = DATA_MANAGER_Get_Humidity();
    uint8_t mode = DATA_MANAGER_Get_Mode();
    DS3231_Time time;
    DS3231_Date date;
    DS3231_ReadTime(&time);
    DS3231_ReadDate(&date);

    // Format all strings
    char tempBuf[32], humBuf[32], modeBuf[32];
    char timeBuf[32], dateBuf[32];
    snprintf(tempBuf, 32, "Temp: %.1f C", temp);
    snprintf(humBuf, 32, "Hum: %.1f %%", hum);
    // ... format remaining strings ...

    // Draw all updates
    ILI9225_DrawText(10, 60, tempBuf, COLOR_WHITE);
    ILI9225_DrawText(10, 80, humBuf, COLOR_WHITE);
    ILI9225_DrawText(10, 120, modeBuf, COLOR_GREEN);
    ILI9225_DrawText(10, 160, timeBuf, COLOR_CYAN);
    ILI9225_DrawText(10, 180, dateBuf, COLOR_CYAN);
}
```

## Update Strategy

### Update Frequency

**Recommended**: 500ms - 1000ms (1-2 Hz)

**Rationale**:

- Sensor data changes slowly (temperature/humidity)
- Time display: 1-second updates sufficient
- Faster updates waste CPU and power
- LCD response time: < 100ms

**Implementation**:

```c
// Main loop with 500ms update period
while (1)
{
    static uint32_t lastDisplayUpdate = 0;

    if (HAL_GetTick() - lastDisplayUpdate >= 500)
    {
        lastDisplayUpdate = HAL_GetTick();
        DISPLAY_Update();
    }

    // Other tasks
}
```

### Selective Updates

For more efficient updates, update only changed fields:

```c
void DISPLAY_Update_Selective(void)
{
    static float lastTemp = 0.0f;
    static uint8_t lastSecond = 0xFF;

    // Update temperature only if changed
    float temp = DATA_MANAGER_Get_Temperature();
    if (temp != lastTemp)
    {
        lastTemp = temp;
        UpdateTemperatureDisplay(temp);
    }

    // Update time only when second changes
    DS3231_Time time;
    DS3231_ReadTime(&time);
    if (time.second != lastSecond)
    {
        lastSecond = time.second;
        UpdateTimeDisplay(&time);
    }
}
```

## Error Handling

### I2C Communication Failure

If RTC read fails, display error message:

```c
DS3231_Time time;
if (DS3231_ReadTime(&time) != 0)
{
    // Display error
    ILI9225_DrawText(10, 160, "TIME ERROR", COLOR_RED);
    return;
}
```

### Invalid Sensor Data

Check for invalid readings:

```c
float temp = DATA_MANAGER_Get_Temperature();
if (temp < -40.0f || temp > 125.0f)
{
    // Display error
    ILI9225_DrawText(10, 60, "TEMP ERROR", COLOR_RED);
}
else
{
    // Display normal value
    char buffer[32];
    snprintf(buffer, 32, "Temp: %.1f C", temp);
    ILI9225_DrawText(10, 60, buffer, COLOR_WHITE);
}
```

## Power Consumption

### Display On

- Active rendering: 10-20 mA (LED backlight dependent)
- Idle (no updates): 5-10 mA

### Display Off (Power Saving)

To turn off display:

```c
ILI9225_SetDisplayOn(0);  // Turn off display
```

To turn on display:

```c
ILI9225_SetDisplayOn(1);  // Turn on display
```

**Use Case**: Turn off display during sleep mode or long-term SD card logging.

## Performance Characteristics

### Initialization Time

- `DISPLAY_Init()`: 50-100ms
  - SPI initialization: 5ms
  - Screen clear: 30-50ms
  - Initial text rendering: 15-30ms

### Update Time

- `DISPLAY_Update()`: 20-30ms
  - Data retrieval: 5-10ms (I2C reads)
  - Text rendering: 15-20ms (6 text fields)

### CPU Usage

At 500ms update rate:

- Active time: 25ms per update
- Idle time: 475ms
- CPU utilization: 5% (25ms/500ms)

## Integration Example

### Complete System Integration

```c
int main(void)
{
    // HAL initialization
    HAL_Init();
    SystemClock_Config();

    // Peripheral initialization
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    // Component initialization
    DS3231_Init();
    SHT3X_Init();
    DATA_MANAGER_Set_Mode(SINGLE_MODE);

    // Display initialization (last)
    DISPLAY_Init();

    // Main loop
    while (1)
    {
        // Update display every 500ms
        static uint32_t lastUpdate = 0;
        if (HAL_GetTick() - lastUpdate >= 500)
        {
            lastUpdate = HAL_GetTick();
            DISPLAY_Update();
        }

        // Handle periodic measurements
        if (Flag_Periodic)
        {
            Flag_Periodic = 0;

            // Measure sensor
            float temp, hum;
            SHT3X_Read_TempHumi(&temp, &hum);
            DATA_MANAGER_Update_Sensor(temp, hum);

            // Display will update on next cycle
        }

        // Handle UART commands
        if (Flag_UART)
        {
            Flag_UART = 0;
            COMMAND_EXECUTE((char *)buff);

            // Force immediate display update after command
            DISPLAY_Update();
        }
    }
}
```

## Dependencies

### Required Libraries

This library depends on:

- **ili9225**: Low-level LCD driver (SPI communication, drawing primitives)
- **fonts**: Font definitions (Font_7x10, Font_11x18)
- **data_manager**: Sensor data and mode access
- **ds3231**: RTC time/date reading

### Used By

This library is used by:

- **main.c**: Main loop calls `DISPLAY_Update()` periodically

## Thread Safety

The display library is **NOT thread-safe**:

- Should only be called from main loop
- Do NOT call from ISR or multiple threads
- SPI peripheral is not reentrant

**Correct Usage**:

```c
// Main loop only
while (1)
{
    DISPLAY_Update();  // OK
}
```

**Incorrect Usage**:

```c
// ISR context
void TIM2_IRQHandler(void)
{
    DISPLAY_Update();  // WRONG - Do not call from ISR
}
```

## Best Practices

### 1. Update at Appropriate Rate

```c
// Good: 500ms - 1000ms updates
static uint32_t lastUpdate = 0;
if (HAL_GetTick() - lastUpdate >= 500)
{
    DISPLAY_Update();
}

// Bad: Too fast, wastes CPU
DISPLAY_Update();  // Every main loop iteration
```

### 2. Initialize Last

```c
// Initialize display after all data sources are ready
DS3231_Init();
SHT3X_Init();
DATA_MANAGER_Init();
DISPLAY_Init();  // Last - will read from above sources
```

### 3. Force Update After Mode Change

```c
// After command that changes mode
DATA_MANAGER_Set_Mode(PERIODIC_MODE);
DISPLAY_Update();  // Immediate update to show mode change
```

### 4. Handle Errors Gracefully

```c
void DISPLAY_Update(void)
{
    float temp = DATA_MANAGER_Get_Temperature();

    if (temp < -40.0f || temp > 125.0f)
    {
        // Show error instead of invalid value
        ILI9225_DrawText(10, 60, "TEMP ERROR", COLOR_RED);
    }
    else
    {
        // Normal display
        char buffer[32];
        snprintf(buffer, 32, "Temp: %.1f C", temp);
        ILI9225_DrawText(10, 60, buffer, COLOR_WHITE);
    }
}
```

## Customization

### Change Layout Positions

Edit coordinates in display.c:

```c
// Current positions
#define TITLE_X     10
#define TITLE_Y     20
#define TEMP_X      10
#define TEMP_Y      60
#define HUM_X       10
#define HUM_Y       80
// ... etc
```

### Add New Display Elements

Example: Add battery voltage display:

```c
void DISPLAY_Update(void)
{
    // ... existing updates ...

    // Add battery voltage
    float voltage = ADC_ReadBatteryVoltage();
    char buffer[32];
    snprintf(buffer, 32, "Batt: %.2f V", voltage);
    ILI9225_DrawText(10, 140, buffer, COLOR_MAGENTA);
}
```

### Change Colors

Edit color values in function calls:

```c
// Change temperature color to red
ILI9225_DrawText(10, 60, tempBuf, COLOR_RED);

// Change mode color to blue
ILI9225_DrawText(10, 120, modeBuf, COLOR_BLUE);
```

## Summary

The Display Library provides:

- High-level display management for ILI9225 LCD
- Real-time sensor data visualization
- System status indication (mode, time, date)
- Efficient update strategy (500ms cycle)
- Integration with Data Manager and RTC
- Clear, readable layout for user monitoring

This abstraction layer simplifies display management and provides a consistent user interface for the Datalogger system.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
