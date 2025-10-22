/**
 * @file display.c
 *
 * @brief Display Library Implementation for ILI9225 LCD
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "display.h"
#include "ili9225.h"
#include "fonts.h"
#include <stdio.h>
#include <string.h>

/* DEFINES -------------------------------------------------------------------*/

/* CONFIGURATION -------------------------------------------------------------*/

// Screen dimensions (Landscape mode: 220x176)
#define SCREEN_WIDTH 220
#define SCREEN_HEIGHT 176

// Font dimensions for calculation (OPTIMIZED: Use only smaller fonts)
#define FONT_11X18_WIDTH 11
#define FONT_11X18_HEIGHT 18
#define FONT_7X10_WIDTH 7
#define FONT_7X10_HEIGHT 10

// Layout positions - Perfectly balanced with precise centering
// Total height: 176px
// Top margin: 10px, Bottom margin: ~10px

// ZONE 1: Time & Date (Top - perfectly centered)
#define TIME_Y 10 // Top margin
#define DATE_Y 44 // Below time (10 + 26 + 8 spacing)

// ZONE 2: Temperature & Humidity (Middle - prominent, same row)
#define SENSOR_Y 76          // Sensor row Y (44 + 18 + 14 spacing)
#define TEMP_ICON_X 20       // Temperature icon X
#define TEMP_VALUE_X 35      // Temperature value X (icon + 15px)
#define HUMIDITY_ICON_X 122  // Humidity icon X (right side)
#define HUMIDITY_VALUE_X 137 // Humidity value X (icon + 15px)

// ZONE 3: System Status (Bottom - compact, balanced)
#define STATUS_Y1 118     // First status row (76 + 26 + 16 spacing)
#define STATUS_Y2 140     // Second status row (118 + 22 spacing)
#define STATUS_COL1_X 10  // Left column
#define STATUS_COL2_X 115 // Right column

// Dot position offsets
#define DOT_OFFSET_X 5 // Spacing after text before dot
#define DOT_OFFSET_Y 4 // Vertical centering of dot

// Colors - Harmonious color scheme
#define COLOR_BG ILI9225_BLACK
#define COLOR_TIME ILI9225_CYAN      // Cyan for time - eye-catching
#define COLOR_DATE ILI9225_YELLOW    // Yellow for date - softer contrast
#define COLOR_TEMP ILI9225_WHITE     // White for temperature
#define COLOR_HUMIDITY ILI9225_WHITE // White for humidity
#define COLOR_LABEL ILI9225_WHITE    // White for labels
#define COLOR_ON ILI9225_GREEN       // Green dot for ON
#define COLOR_OFF ILI9225_RED        // Red dot for OFF

/* PRIVATE VARIABLES ---------------------------------------------------------*/

// Previous values for change detection
static char prev_time_str[16] = "";
static char prev_date_str[16] = "";
static char prev_temp_str[16] = "";
static char prev_humi_str[16] = "";
static bool prev_mqtt_on = false;
static bool prev_periodic_on = false;
static char prev_interval_str[16] = "";
static bool first_draw = true;

/* ICON DRAWING ------------------------------------------------------------- */

/**
 * @brief Draw gear icon (8x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_gear(uint16_t x, uint16_t y, uint16_t color)
{
    // Simple gear icon
    ILI9225_DrawRect(x + 2, y + 2, 4, 4, color);
    ILI9225_DrawPixel(x + 1, y + 3, color);
    ILI9225_DrawPixel(x + 1, y + 4, color);
    ILI9225_DrawPixel(x + 6, y + 3, color);
    ILI9225_DrawPixel(x + 6, y + 4, color);
    ILI9225_DrawPixel(x + 3, y + 1, color);
    ILI9225_DrawPixel(x + 4, y + 1, color);
    ILI9225_DrawPixel(x + 3, y + 6, color);
    ILI9225_DrawPixel(x + 4, y + 6, color);
}

/**
 * @brief Draw WiFi icon (8x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_wifi(uint16_t x, uint16_t y, uint16_t color)
{
    // WiFi icon - simplified
    ILI9225_DrawPixel(x + 4, y + 7, color);
    ILI9225_DrawLine(x + 2, y + 5, x + 6, y + 5, color);
    ILI9225_DrawLine(x + 1, y + 3, x + 7, y + 3, color);
    ILI9225_DrawLine(x, y + 1, x + 8, y + 1, color);
}

/**
 * @brief Draw clock icon (8x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_clock(uint16_t x, uint16_t y, uint16_t color)
{
    // Clock icon
    ILI9225_DrawCircle(x + 4, y + 4, 3, color);
    ILI9225_DrawLine(x + 4, y + 4, x + 4, y + 2, color);
    ILI9225_DrawLine(x + 4, y + 4, x + 6, y + 4, color);
}

/**
 * @brief Draw refresh icon (8x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_refresh(uint16_t x, uint16_t y, uint16_t color)
{
    // Circular arrow icon
    ILI9225_DrawCircle(x + 4, y + 4, 3, color);
    ILI9225_DrawLine(x + 7, y + 4, x + 7, y + 2, color);
    ILI9225_DrawLine(x + 7, y + 2, x + 6, y + 2, color);
}

/**
 * @brief Draw thermometer icon (6x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_thermometer(uint16_t x, uint16_t y, uint16_t color)
{
    ILI9225_DrawRect(x + 2, y, 2, 5, color);
    ILI9225_FillCircle(x + 3, y + 6, 2, color);
}

/**
 * @brief Draw water drop icon (6x8)
 *
 * @param x Top-left X
 * @param y Top-left Y
 * @param color RGB565 color
 */
static void draw_icon_water(uint16_t x, uint16_t y, uint16_t color)
{
    ILI9225_DrawLine(x + 3, y, x + 3, y + 2, color);
    ILI9225_DrawLine(x + 1, y + 3, x + 5, y + 3, color);
    ILI9225_DrawLine(x, y + 4, x + 6, y + 4, color);
    ILI9225_DrawLine(x + 1, y + 5, x + 5, y + 5, color);
    ILI9225_DrawLine(x + 2, y + 6, x + 4, y + 6, color);
    ILI9225_DrawPixel(x + 3, y + 7, color);
}

/**
 * @brief Draw status indicator dot
 *
 * @param x Center X
 * @param y Center Y
 * @param is_on true = ON (green), false = OFF (red)
 */
static void draw_status_dot(uint16_t x, uint16_t y, bool is_on)
{
    uint16_t color = is_on ? COLOR_ON : COLOR_OFF;
    ILI9225_FillCircle(x, y, 3, color);
}

/* HELPER FUNCTIONS --------------------------------------------------------- */

/**
 * @brief Clear text area
 *
 * @param x Start X
 * @param y Start Y
 * @param w Width
 * @param h Height
 *
 * @note Fills area with background color
 */
static void clear_area(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    ILI9225_FillRect(x, y, w, h, COLOR_BG);
}

/**
 * @brief Convert time_t to time string (HH:MM:SS)
 *
 * @param unix_time Unix timestamp
 * @param buffer Output buffer (must be at least 9 bytes)
 *
 * @note Format: "HH:MM:SS"
 */
static void format_time(time_t unix_time, char *buffer)
{
    struct tm *timeinfo = localtime(&unix_time);
    sprintf(buffer, "%02d:%02d:%02d",
            timeinfo->tm_hour,
            timeinfo->tm_min,
            timeinfo->tm_sec);
}

/**
 * @brief Convert time_t to date string (Th4 22/10/2025)
 *
 * @param unix_time Unix timestamp
 * @param buffer Output buffer (must be at least 20 bytes)
 * @note Format: "ThX DD/MM/YYYY"
 */
static void format_date(time_t unix_time, char *buffer)
{
    struct tm *timeinfo = localtime(&unix_time);
    const char *weekdays[] = {"CN", "Th2", "Th3", "Th4", "Th5", "Th6", "Th7"};
    sprintf(buffer, "%s %02d/%02d/%04d",
            weekdays[timeinfo->tm_wday],
            timeinfo->tm_mday,
            timeinfo->tm_mon + 1,
            timeinfo->tm_year + 1900);
}

/**
 * @brief Format interval to human-readable string
 *
 * @param interval_seconds Interval in seconds
 * @param buffer Output buffer (must be at least 8 bytes)
 *
 * @note Format rules:
 *       < 60s: Show as seconds (5s, 30s)
 *       >= 60s: Show as minutes (1m, 10m, 30m, 60m)
 */
static void format_interval(int interval_seconds, char *buffer)
{
    if (interval_seconds < 60)
    {
        // Show as seconds (5s, 30s, etc.)
        sprintf(buffer, "%ds", interval_seconds);
    }
    else
    {
        // Show as minutes (1m, 10m, 30m, 60m, etc.)
        int minutes = interval_seconds / 60;
        sprintf(buffer, "%dm", minutes);
    }
}

/* PUBLIC API --------------------------------------------------------------- */

/**
 * @brief Initialize display system
 */
void display_init(void)
{
    // Clear screen
    ILI9225_FillScreen(COLOR_BG);

    /* Draw static temperature & humidity icons & units */
    // Temperature side (left)
    draw_icon_thermometer(TEMP_ICON_X, SENSOR_Y + 3, COLOR_TEMP);

    // Humidity side (right)
    draw_icon_water(HUMIDITY_ICON_X, SENSOR_Y + 3, COLOR_HUMIDITY);

    /* ZONE 3: Draw static status labels and icons */
    // Row 1: System (left) & Periodic (right)
    draw_icon_gear(STATUS_COL1_X, STATUS_Y1, COLOR_LABEL);
    ILI9225_WriteString(STATUS_COL1_X + 12, STATUS_Y1, "System",
                        Font_7x10, COLOR_LABEL, COLOR_BG);

    draw_icon_clock(STATUS_COL2_X, STATUS_Y1, COLOR_LABEL);
    ILI9225_WriteString(STATUS_COL2_X + 12, STATUS_Y1, "Periodic",
                        Font_7x10, COLOR_LABEL, COLOR_BG);

    // Row 2: MQTT (left) & Interval (right)
    draw_icon_wifi(STATUS_COL1_X, STATUS_Y2, COLOR_LABEL);
    ILI9225_WriteString(STATUS_COL1_X + 12, STATUS_Y2, "MQTT",
                        Font_7x10, COLOR_LABEL, COLOR_BG);

    draw_icon_refresh(STATUS_COL2_X, STATUS_Y2, COLOR_LABEL);
    ILI9225_WriteString(STATUS_COL2_X + 12, STATUS_Y2, "Interval:",
                        Font_7x10, COLOR_LABEL, COLOR_BG);

    // Reset previous values
    memset(prev_time_str, 0, sizeof(prev_time_str));
    memset(prev_date_str, 0, sizeof(prev_date_str));
    memset(prev_temp_str, 0, sizeof(prev_temp_str));
    memset(prev_humi_str, 0, sizeof(prev_humi_str));
    memset(prev_interval_str, 0, sizeof(prev_interval_str));
    first_draw = true;
}

/**
 * @brief Update all display data
 */
void display_update(time_t time_unix, float temperature, float humidity,
                    bool mqtt_on, bool periodic_on, int interval)
{
    char buffer[32];
    uint16_t text_x;

    /* ZONE 1: TIME & DATE (Top, perfectly centered, colored) */
    // TIME - Display in CYAN for visibility (HH:MM:SS = 8 chars)
    format_time(time_unix, buffer);
    if (strcmp(buffer, prev_time_str) != 0 || first_draw)
    {
        // Calculate perfect center: (screen_width - text_width) / 2
        text_x = (SCREEN_WIDTH - (8 * FONT_11X18_WIDTH)) / 2; // Optimized: Use Font_11x18
        clear_area(0, TIME_Y, SCREEN_WIDTH, 20);
        ILI9225_WriteString(text_x, TIME_Y, buffer,
                            Font_11x18, COLOR_TIME, COLOR_BG);
        strcpy(prev_time_str, buffer);
    }

    // DATE - Display in YELLOW for contrast (e.g., "Th4 22/10/2025" = 14 chars)
    format_date(time_unix, buffer);
    if (strcmp(buffer, prev_date_str) != 0 || first_draw)
    {
        // Calculate perfect center based on actual string length
        uint16_t date_len = strlen(buffer);
        text_x = (SCREEN_WIDTH - (date_len * FONT_7X10_WIDTH)) / 2; // Optimized: Use Font_7x10
        clear_area(0, DATE_Y, SCREEN_WIDTH, 12);
        ILI9225_WriteString(text_x, DATE_Y, buffer,
                            Font_7x10, COLOR_DATE, COLOR_BG);
        strcpy(prev_date_str, buffer);
    }

    /* ZONE 2: TEMPERATURE & HUMIDITY (Center, LARGE, WHITE) */
    // Temperature (Left side) - WHITE for clarity
    sprintf(buffer, "%.1f", temperature);
    if (strcmp(buffer, prev_temp_str) != 0 || first_draw)
    {
        // Clear temperature area
        clear_area(TEMP_ICON_X, SENSOR_Y, 85, 20);

        // Draw thermometer icon
        draw_icon_thermometer(TEMP_ICON_X, SENSOR_Y + 2, COLOR_TEMP);

        // Display temperature VALUE - Optimized: Use Font_11x18
        ILI9225_WriteString(TEMP_VALUE_X, SENSOR_Y, buffer,
                            Font_11x18, COLOR_TEMP, COLOR_BG);

        // Add degree symbol and C unit
        uint16_t deg_x = TEMP_VALUE_X + (strlen(buffer) * FONT_11X18_WIDTH) + 2;
        ILI9225_DrawCircle(deg_x, SENSOR_Y + 2, 2, COLOR_TEMP);
        ILI9225_WriteString(deg_x + 5, SENSOR_Y + 4, "C",
                            Font_7x10, COLOR_TEMP, COLOR_BG);

        strcpy(prev_temp_str, buffer);
    }

    // Humidity (Right side) - WHITE for clarity
    sprintf(buffer, "%.0f", humidity);
    if (strcmp(buffer, prev_humi_str) != 0 || first_draw)
    {
        // Clear humidity area
        clear_area(HUMIDITY_ICON_X, SENSOR_Y, 80, 20);

        // Draw water drop icon
        draw_icon_water(HUMIDITY_ICON_X, SENSOR_Y + 2, COLOR_HUMIDITY);

        // Display humidity VALUE - Optimized: Use Font_11x18
        ILI9225_WriteString(HUMIDITY_VALUE_X, SENSOR_Y, buffer,
                            Font_11x18, COLOR_HUMIDITY, COLOR_BG);

        // Add % RH label
        uint16_t unit_x = HUMIDITY_VALUE_X + (strlen(buffer) * FONT_11X18_WIDTH) + 2;
        ILI9225_WriteString(unit_x, SENSOR_Y + 4, "% RH",
                            Font_7x10, COLOR_HUMIDITY, COLOR_BG);

        strcpy(prev_humi_str, buffer);
    }

    /* ZONE 3: SYSTEM STATUS (Bottom, compact with colored dots) */

    // Row 1, Column 1: System (always ON)
    if (first_draw)
    {
        // Calculate dot position: after "System" text + spacing
        uint16_t dot_x = STATUS_COL1_X + 12 + (6 * 7) + DOT_OFFSET_X;
        draw_status_dot(dot_x, STATUS_Y1 + DOT_OFFSET_Y, true);
    }

    // Row 1, Column 2: Periodic status
    if (periodic_on != prev_periodic_on || first_draw)
    {
        // Calculate dot position: after "Periodic" text + spacing
        uint16_t dot_x = STATUS_COL2_X + 12 + (8 * 7) + DOT_OFFSET_X;
        clear_area(dot_x - 2, STATUS_Y1, 10, 10);
        draw_status_dot(dot_x, STATUS_Y1 + DOT_OFFSET_Y, periodic_on);
        prev_periodic_on = periodic_on;
    }

    // Row 2, Column 1: MQTT status
    if (mqtt_on != prev_mqtt_on || first_draw)
    {
        // Calculate dot position: after "MQTT" text + spacing
        uint16_t dot_x = STATUS_COL1_X + 12 + (4 * 7) + DOT_OFFSET_X;
        clear_area(dot_x - 2, STATUS_Y2, 10, 10);
        draw_status_dot(dot_x, STATUS_Y2 + DOT_OFFSET_Y, mqtt_on);
        prev_mqtt_on = mqtt_on;
    }

    // Row 2, Column 2: Interval value
    format_interval(interval, buffer);
    if (strcmp(buffer, prev_interval_str) != 0 || first_draw)
    {
        // Position after "Interval:" label
        uint16_t interval_x = STATUS_COL2_X + 12 + (9 * 7) + 2;
        clear_area(interval_x, STATUS_Y2, 35, 10);
        ILI9225_WriteString(interval_x, STATUS_Y2, buffer,
                            Font_7x10, COLOR_LABEL, COLOR_BG);
        strcpy(prev_interval_str, buffer);
    }

    first_draw = false;
}

/**
 * @brief Clear entire display
 */
void display_clear(void)
{
    ILI9225_FillScreen(COLOR_BG);
    first_draw = true;
}
