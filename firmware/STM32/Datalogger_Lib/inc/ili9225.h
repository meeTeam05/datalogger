/**
 * @file ili9225.h
 *
 * @brief ILI9225 TFT LCD Driver for STM32
 *
 * @note 176x220 2.0" Display, SPI Interface
 */

#ifndef ILI9225_H
#define ILI9225_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* DEFINES ------------------------------------------------------------------*/

/* CONFIGURATION OPTIONS ----------------------------------------------------*/

// SPI Port
#define ILI9225_SPI_PORT hspi2
extern SPI_HandleTypeDef ILI9225_SPI_PORT;

// DMA Support (comment to disable and save RAM)
// #define ILI9225_USE_DMA
#ifdef ILI9225_USE_DMA
#define ILI9225_DMA_MIN_SIZE 16    // Min bytes to use DMA
#define ILI9225_DMA_BUFFER_LINES 5 // Buffer size (lines)
#endif

// Font Support (comment to disable and save memory)
#define ILI9225_USE_FONTS
#ifdef ILI9225_USE_FONTS
#include "fonts.h"
#endif

// Display Resolution
#define ILI9225_LCD_WIDTH 176
#define ILI9225_LCD_HEIGHT 220

// Display Rotation (0-3)
// #define ILI9225_ROTATION 0    // 0=Portrait (176x220)
// #define ILI9225_ROTATION 1    // 1=Landscape (220x176)
// #define ILI9225_ROTATION 2    // 2=Portrait180 (176x220)
#define ILI9225_ROTATION 3 // 3=Landscape180 (220x176)

// CS Control (comment if CS tied to GND)
#define ILI9225_USE_CS

// Pin Definitions
#define ILI9225_RST_PORT ILI9225_RST_GPIO_Port
#define ILI9225_RST_PIN ILI9225_RST_Pin
#define ILI9225_RS_PORT ILI9225_RS_GPIO_Port // Register Select (DC)
#define ILI9225_RS_PIN ILI9225_RS_Pin
#ifdef ILI9225_USE_CS
#define ILI9225_CS_PORT ILI9225_CS_GPIO_Port
#define ILI9225_CS_PIN ILI9225_CS_Pin
#endif

/* DISPLAY PARAMETERS ------------------------------------------------------ */

#if ILI9225_ROTATION == 0 || ILI9225_ROTATION == 2
#define ILI9225_WIDTH ILI9225_LCD_WIDTH
#define ILI9225_HEIGHT ILI9225_LCD_HEIGHT
#else
#define ILI9225_WIDTH ILI9225_LCD_HEIGHT
#define ILI9225_HEIGHT ILI9225_LCD_WIDTH
#endif

/* COLOR DEFINITIONS (RGB565) ---------------------------------------------- */

#define ILI9225_BLACK 0x0000
#define ILI9225_WHITE 0xFFFF
#define ILI9225_RED 0xF800
#define ILI9225_GREEN 0x07E0
#define ILI9225_BLUE 0x001F
#define ILI9225_YELLOW 0xFFE0
#define ILI9225_CYAN 0x07FF
#define ILI9225_MAGENTA 0xF81F
#define ILI9225_ORANGE 0xFD20
#define ILI9225_GRAY 0x8410
#define ILI9225_DARKGRAY 0x4208
#define ILI9225_LIGHTGRAY 0xC618
#define ILI9225_BROWN 0xBC40
#define ILI9225_DARKBLUE 0x01CF
#define ILI9225_LIGHTBLUE 0x7D7C
#define ILI9225_LIGHTGREEN 0x841F

/* PUBLIC API -------------------------------------------------------------- */

// Initialization & Control

/**
 * @brief Initialize ILI9225 display
 */
void ILI9225_Init(void);

/**
 * @brief Set display rotation
 *
 * @param rotation 0: Portrait, 1: Landscape, 2: Portrait 180°, 3: Landscape 180°
 */
void ILI9225_SetRotation(uint8_t rotation);

/**
 * @brief Invert display colors
 *
 * @param invert true to invert, false for normal
 */
void ILI9225_InvertDisplay(bool invert);

/**
 * @brief Turn display on/off
 *
 * @param on true for on, false for off
 */
void ILI9225_DisplayOn(bool on);

/**
 * @brief Set display brightness (if supported)
 *
 * @param brightness 0-255
 */
void ILI9225_SetBrightness(uint8_t brightness);

// Basic Drawing

/**
 * @brief Fill screen with color
 *
 * @param color RGB565 color
 */
void ILI9225_FillScreen(uint16_t color);

/**
 * @brief Fill rectangle with color
 *
 * @param x Start X
 * @param y Start Y
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void ILI9225_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw single pixel
 *
 * @param x X coordinate
 * @param y Y coordinate
 * @param color RGB565 color
 */
void ILI9225_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

// Lines & Shapes

/**
 * @brief Draw line (Bresenham algorithm)
 *
 * @param x0 Start X
 * @param y0 Start Y
 * @param x1 End X
 * @param y1 End Y
 * @param color RGB565 color
 */
void ILI9225_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);

/**
 * @brief Draw rectangle outline
 *
 * @param x Start X
 * @param y Start Y
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void ILI9225_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Draw circle outline (Midpoint algorithm)
 *
 * @param x0 Center X
 * @param y0 Center Y
 * @param r Radius
 * @param color RGB565 color
 */
void ILI9225_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Fill circle
 *
 * @param x0 Center X
 * @param y0 Center Y
 * @param r Radius
 * @param color RGB565 color
 */
void ILI9225_FillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

/**
 * @brief Draw triangle outline
 *
 * @param x1 Vertex 1 X
 * @param y1 Vertex 1 Y
 * @param x2 Vertex 2 X
 * @param y2 Vertex 2 Y
 * @param x3 Vertex 3 X
 * @param y3 Vertex 3 Y
 * @param color RGB565 color
 */
void ILI9225_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t x3, uint16_t y3, uint16_t color);

/**
 * @brief Fill triangle
 *
 * @param x1 Vertex 1 X
 * @param y1 Vertex 1 Y
 * @param x2 Vertex 2 X
 * @param y2 Vertex 2 Y
 * @param x3 Vertex 3 X
 * @param y3 Vertex 3 Y
 * @param color RGB565 color
 */
void ILI9225_FillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t x3, uint16_t y3, uint16_t color);

// Image Drawing

/**
 * @brief Draw bitmap image
 *
 * @param x Start X
 * @param y Start Y
 * @param w Width
 * @param h Height
 * @param data RGB565 data pointer
 */
void ILI9225_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

// Text Functions (if fonts enabled)
#ifdef ILI9225_USE_FONTS

/**
 * @brief Write single character
 *
 * @param x Start X
 * @param y Start Y
 * @param ch Character
 * @param font Font definition
 * @param color Foreground RGB565
 * @param bgcolor Background RGB565
 */
void ILI9225_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font,
                       uint16_t color, uint16_t bgcolor);

/**
 * @brief Write string with word wrap
 *
 * @param x Start X
 * @param y Start Y
 * @param str String pointer
 * @param font Font definition
 * @param color Foreground RGB565
 * @param bgcolor Background RGB565
 */
void ILI9225_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font,
                         uint16_t color, uint16_t bgcolor);
#endif

// Utility Functions

/**
 * @brief Convert RGB to RGB565
 *
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 *
 * @return RGB565 value
 */
uint16_t ILI9225_Color565(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Run test sequence (colors, text, shapes)
 *
 * @note FLASH OPTIMIZATION: Commented out to save ~1KB
 */
// void ILI9225_Test(void);

// Validation
#ifndef ILI9225_ROTATION
#error "ILI9225_ROTATION must be defined (0-3)"
#endif

#endif /* ILI9225_H */
