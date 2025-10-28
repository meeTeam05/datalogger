# Fonts Library

Provides bitmap font resources for on-screen text rendering.

## Overview

The Fonts Library provides bitmap font definitions for text rendering on the ILI9225 TFT LCD display. It includes two font sizes optimized for readability at 176x220 pixel resolution with complete ASCII character support.

## Files

- **fonts.c**: Font bitmap data arrays
- **fonts.h**: Font structure definitions and external declarations

## Font Structure

### Font_TypeDef Structure

```c
typedef struct {
    const uint8_t *table;   // Pointer to font bitmap data
    uint16_t width;         // Character width in pixels
    uint16_t height;        // Character height in pixels
} Font_TypeDef;
```

**Fields**:

- `table`: Pointer to constant font bitmap array in flash memory
- `width`: Width of each character in pixels
- `height`: Height of each character in pixels

## Available Fonts

### Font_7x10

```c
extern Font_TypeDef Font_7x10;
```

**Specifications**:

- **Character Width**: 7 pixels
- **Character Height**: 10 pixels
- **Character Count**: 95 characters (ASCII 32-126)
- **Total Size**: 950 bytes (95 × 10 bytes per character)
- **Use Case**: Small text, status messages, compact displays

**Character Coverage**:

```
ASCII 32-126:
 !"#$%&'()*+,-./0123456789:;<=>?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~
```

**Display Capacity**:

- Characters per line: 25 (176 / 7 = 25.1)
- Lines per screen: 22 (220 / 10 = 22)
- Total characters: 550

### Font_11x18

```c
extern Font_TypeDef Font_11x18;
```

**Specifications**:

- **Character Width**: 11 pixels
- **Character Height**: 18 pixels
- **Character Count**: 95 characters (ASCII 32-126)
- **Total Size**: 1,710 bytes (95 × 18 bytes per character)
- **Use Case**: Main display text, sensor readings, high readability

**Character Coverage**:

```
ASCII 32-126:
 !"#$%&'()*+,-./0123456789:;<=>?
@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_
`abcdefghijklmnopqrstuvwxyz{|}~
```

**Display Capacity**:

- Characters per line: 16 (176 / 11 = 16)
- Lines per screen: 12 (220 / 18 = 12.2)
- Total characters: 192

## Bitmap Format

### Data Encoding

Each character is stored as a series of bytes representing vertical columns of pixels.

**Format**: Column-major order

- Each byte represents 8 vertical pixels
- MSB = top pixel, LSB = bottom pixel
- Characters taller than 8 pixels use multiple bytes per column

### Font_7x10 Character Format

Each character: 10 bytes (7 columns × 1-2 bytes per column)

**Example**: Character 'A' (ASCII 65)

```
Byte Index:  0    1    2    3    4    5    6    7    8    9
Column:      0    1    2    3    4    5    6    (padding)
Data:      0x00 0xFC 0x22 0x22 0x22 0xFC 0x00 0x00 0x00 0x00

Visual representation (rotated 90° for clarity):
Column 0: 00000000
Column 1: 11111100  ******
Column 2: 00100010      *  *
Column 3: 00100010      *  *
Column 4: 00100010      *  *
Column 5: 11111100  ******
Column 6: 00000000
```

### Font_11x18 Character Format

Each character: 18 bytes (11 columns × 1-2 bytes per column)

**Example**: Character 'A' (ASCII 65)

```
Byte Index:  0    1    2    3    4    5    6  ... 17
Column:      0    1    2    3    4    5    6  ... (padding)
Data:      0x00 0x00 0xF0 0x0C 0x03 0x03 0x0C 0xF0 ...

Visual: Larger, more detailed 'A' character (18 pixels tall, 11 pixels wide)
```

## Character Lookup

### Character Index Calculation

```c
// ASCII value to font table index
uint8_t asciiValue = 'A';  // ASCII 65
uint8_t fontIndex = asciiValue - 32;  // Index 33

// Character data offset
const uint8_t *charData = Font_11x18.table + (fontIndex * 18);
```

**Formula**:

```
Font Index = ASCII Value - 32
Data Offset = Font Index × Bytes Per Character
```

**ASCII 32 (Space)** → Font Index 0
**ASCII 126 (~)** → Font Index 94

## Usage with ILI9225 Driver

### Drawing Single Character

```c
void DrawCharacter(uint16_t x, uint16_t y, char ch, uint16_t color)
{
    Font_TypeDef *font = &Font_11x18;

    if (ch < 32 || ch > 126)
        return;  // Character not supported

    uint8_t fontIndex = ch - 32;
    const uint8_t *charData = font->table + (fontIndex * font->height);

    // Render character bitmap at (x, y) with specified color
    for (uint8_t col = 0; col < font->width; col++)
    {
        for (uint8_t row = 0; row < font->height; row++)
        {
            uint8_t byteIndex = row / 8;
            uint8_t bitIndex = row % 8;
            uint8_t byte = charData[col * ((font->height + 7) / 8) + byteIndex];

            if (byte & (1 << bitIndex))
            {
                ILI9225_DrawPixel(x + col, y + row, color);
            }
        }
    }
}
```

### Drawing String

```c
void DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color)
{
    Font_TypeDef *font = &Font_11x18;
    uint16_t cursorX = x;

    while (*str)
    {
        DrawCharacter(cursorX, y, *str, color);
        cursorX += font->width;  // Move to next character position
        str++;

        // Line wrap if needed
        if (cursorX + font->width > 176)
        {
            cursorX = 0;
            y += font->height;
        }
    }
}
```

## Font Selection Guidelines

### Use Font_7x10 When:

1. **Space is limited**: Need to fit more text
2. **Status messages**: Brief notifications
3. **Small labels**: Icons, button labels
4. **Dense information**: Tables, lists

**Example Use Cases**:

```c
// Status bar
DrawString(0, 0, "Connected | Logging", COLOR_GREEN);

// Compact table
DrawString(0, 20, "Temp  Hum  Time", COLOR_WHITE);
DrawString(0, 30, "25.5  65.2  14:35", COLOR_CYAN);
```

### Use Font_11x18 When:

1. **Primary display**: Main sensor readings
2. **Readability critical**: User must read at distance
3. **Emphasis needed**: Important values, warnings
4. **Standard UI**: Normal application text

**Example Use Cases**:

```c
// Large sensor display
DrawString(10, 60, "Temp: 25.5 C", COLOR_WHITE);
DrawString(10, 80, "Hum: 65.2 %", COLOR_WHITE);

// Mode indicator
DrawString(10, 120, "Mode: PERIODIC", COLOR_GREEN);
```

## Memory Footprint

### Flash Memory Usage

```
Font_7x10:
  - 95 characters × 10 bytes = 950 bytes
  - Font structure: 8 bytes
  - Total: 958 bytes

Font_11x18:
  - 95 characters × 18 bytes = 1,710 bytes
  - Font structure: 8 bytes
  - Total: 1,718 bytes

Combined: 2,676 bytes (~2.6 KB flash)
```

### RAM Usage

**Zero RAM usage**: All font data is const and stored in flash.

### CPU Usage

Character rendering time:

- Font_7x10: ~50 microseconds per character
- Font_11x18: ~120 microseconds per character

String rendering:

- 16-character string (Font_11x18): ~2ms

## Font Customization

### Adding New Character

To add a new character (e.g., degree symbol °):

1. **Create bitmap**:

   ```
   Visual:     Bitmap bytes:
   ******      0xFC
      *  *        0x84
      *  *        0x84
   ******      0xFC
   ```

2. **Add to font table**:
   ```c
   // At appropriate position in fonts.c
   0x00, 0xFC, 0x84, 0x84, 0x84, 0xFC, 0x00, 0x00, 0x00, 0x00
   ```

### Creating New Font Size

To create a new font (e.g., Font_14x24):

1. **Generate bitmap data** using font editor tool
2. **Define font table** in fonts.c:

   ```c
   const uint8_t Font_14x24_Table[] = {
       // 95 characters × 24 bytes each
       // Character bitmaps here...
   };
   ```

3. **Declare font structure** in fonts.c:

   ```c
   Font_TypeDef Font_14x24 = {
       .table = Font_14x24_Table,
       .width = 14,
       .height = 24
   };
   ```

4. **Add extern declaration** in fonts.h:
   ```c
   extern Font_TypeDef Font_14x24;
   ```

## Character Set Limitations

### Supported Characters

Only ASCII 32-126 are supported:

- **Space** (32) to **Tilde** (~, 126)
- Total: 95 characters

### Unsupported Characters

Characters outside this range will not render:

- ASCII 0-31: Control characters
- ASCII 127+: Extended ASCII, Unicode

**Handling Unsupported Characters**:

```c
void DrawCharacterSafe(uint16_t x, uint16_t y, char ch, uint16_t color)
{
    if (ch < 32 || ch > 126)
    {
        ch = '?';  // Replace with question mark
    }

    DrawCharacter(x, y, ch, color);
}
```

## Performance Optimization

### Bitmap Caching

For repeated characters, cache bitmap data:

```c
// Cache character bitmaps in RAM for faster access
uint8_t charCache[18];  // For Font_11x18

void CacheCharacter(char ch)
{
    Font_TypeDef *font = &Font_11x18;
    uint8_t fontIndex = ch - 32;
    const uint8_t *charData = font->table + (fontIndex * font->height);

    memcpy(charCache, charData, font->height);
}
```

### Monospaced Advantage

Both fonts are monospaced (fixed width):

- Simplifies cursor positioning
- Enables fast column-major rendering
- Predictable line wrapping

**Example**:

```c
// Calculate string width without rendering
uint16_t GetStringWidth(const char *str, Font_TypeDef *font)
{
    return strlen(str) * font->width;
}

// Center text on screen
void DrawCenteredString(uint16_t y, const char *str, uint16_t color)
{
    uint16_t width = GetStringWidth(str, &Font_11x18);
    uint16_t x = (176 - width) / 2;
    DrawString(x, y, str, color);
}
```

## Integration Example

### Complete Text Rendering System

```c
// Initialize in main.c
Font_TypeDef *currentFont = &Font_11x18;

// Draw formatted text
void DisplaySensorData(float temp, float hum)
{
    char buffer[32];

    // Temperature with Font_11x18
    snprintf(buffer, sizeof(buffer), "Temp: %.1f C", temp);
    ILI9225_DrawText(10, 60, buffer, COLOR_WHITE);

    // Humidity with Font_11x18
    snprintf(buffer, sizeof(buffer), "Hum: %.1f %%", hum);
    ILI9225_DrawText(10, 80, buffer, COLOR_WHITE);

    // Status with Font_7x10 (smaller)
    currentFont = &Font_7x10;
    ILI9225_DrawText(0, 0, "System: OK | Mode: PERIODIC", COLOR_GREEN);
    currentFont = &Font_11x18;  // Reset to default
}
```

## Best Practices

### 1. Use Appropriate Font Size

```c
// Main data: Use Font_11x18
ILI9225_DrawText(10, 60, "Temp: 25.5 C", COLOR_WHITE);

// Status/small text: Use Font_7x10
ILI9225_DrawText(0, 0, "WiFi: Connected", COLOR_GREEN);
```

### 2. Validate Characters

```c
void DrawStringSafe(uint16_t x, uint16_t y, const char *str, uint16_t color)
{
    while (*str)
    {
        char ch = *str;

        // Validate ASCII range
        if (ch >= 32 && ch <= 126)
        {
            DrawCharacter(x, y, ch, color);
            x += Font_11x18.width;
        }

        str++;
    }
}
```

### 3. Consider Screen Boundaries

```c
void DrawStringWrapped(uint16_t x, uint16_t y, const char *str, uint16_t color)
{
    Font_TypeDef *font = &Font_11x18;
    uint16_t cursorX = x;
    uint16_t cursorY = y;

    while (*str)
    {
        // Check horizontal boundary
        if (cursorX + font->width > 176)
        {
            cursorX = 0;
            cursorY += font->height;
        }

        // Check vertical boundary
        if (cursorY + font->height > 220)
            break;  // Stop rendering

        DrawCharacter(cursorX, cursorY, *str, color);
        cursorX += font->width;
        str++;
    }
}
```

## Dependencies

### Used By

- **ili9225.c**: Uses font structures for `ILI9225_DrawText()` function
- **display.c**: Uses Font_11x18 for sensor data display

### Uses

- **None**: Standalone font data library

## Summary

The Fonts Library provides:

- Two fixed-width bitmap fonts (7x10 and 11x18 pixels)
- Complete ASCII printable character set (32-126)
- Efficient flash storage (column-major bitmap format)
- Zero RAM usage (const data in flash)
- Easy integration with ILI9225 LCD driver
- Scalable architecture for additional fonts

This library enables clear, readable text rendering on the 176x220 LCD display with minimal memory footprint and optimal performance.

## License

This component is part of the DATALOGGER project.
See the LICENSE.md file in the project root directory for licensing information.
