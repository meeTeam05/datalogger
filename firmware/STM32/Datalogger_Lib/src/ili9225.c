/**
 * @file ili9225.c
 *
 * @brief ILI9225 TFT LCD Driver Implementation
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "ili9225.h"
#include <string.h>
#include <stdlib.h>

/* DEFINES -------------------------------------------------------------------*/

/* ILI9225 Registers */
#define ILI9225_DRIVER_OUTPUT_CTRL 0x01
#define ILI9225_LCD_AC_DRIVING_CTRL 0x02
#define ILI9225_ENTRY_MODE 0x03
#define ILI9225_DISP_CTRL1 0x07
#define ILI9225_BLANK_PERIOD_CTRL1 0x08
#define ILI9225_FRAME_CYCLE_CTRL 0x0B
#define ILI9225_INTERFACE_CTRL 0x0C
#define ILI9225_OSC_CTRL 0x0F
#define ILI9225_POWER_CTRL1 0x10
#define ILI9225_POWER_CTRL2 0x11
#define ILI9225_POWER_CTRL3 0x12
#define ILI9225_POWER_CTRL4 0x13
#define ILI9225_POWER_CTRL5 0x14
#define ILI9225_VCI_RECYCLING 0x15
#define ILI9225_RAM_ADDR_SET1 0x20
#define ILI9225_RAM_ADDR_SET2 0x21
#define ILI9225_GRAM_DATA_REG 0x22
#define ILI9225_GATE_SCAN_CTRL 0x30
#define ILI9225_VERTICAL_SCROLL_CTRL1 0x31
#define ILI9225_VERTICAL_SCROLL_CTRL2 0x32
#define ILI9225_VERTICAL_SCROLL_CTRL3 0x33
#define ILI9225_PARTIAL_DRIVING_POS1 0x34
#define ILI9225_PARTIAL_DRIVING_POS2 0x35
#define ILI9225_HORIZONTAL_WINDOW_ADDR1 0x36
#define ILI9225_HORIZONTAL_WINDOW_ADDR2 0x37
#define ILI9225_VERTICAL_WINDOW_ADDR1 0x38
#define ILI9225_VERTICAL_WINDOW_ADDR2 0x39
#define ILI9225_GAMMA_CTRL1 0x50
#define ILI9225_GAMMA_CTRL2 0x51
#define ILI9225_GAMMA_CTRL3 0x52
#define ILI9225_GAMMA_CTRL4 0x53
#define ILI9225_GAMMA_CTRL5 0x54
#define ILI9225_GAMMA_CTRL6 0x55
#define ILI9225_GAMMA_CTRL7 0x56
#define ILI9225_GAMMA_CTRL8 0x57
#define ILI9225_GAMMA_CTRL9 0x58
#define ILI9225_GAMMA_CTRL10 0x59

#ifdef ILI9225_USE_CS
#define CS_LOW() HAL_GPIO_WritePin(ILI9225_CS_PORT, ILI9225_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(ILI9225_CS_PORT, ILI9225_CS_PIN, GPIO_PIN_SET)
#else
#define CS_LOW()
#define CS_HIGH()
#endif

#define RS_LOW() HAL_GPIO_WritePin(ILI9225_RS_PORT, ILI9225_RS_PIN, GPIO_PIN_RESET)
#define RS_HIGH() HAL_GPIO_WritePin(ILI9225_RS_PORT, ILI9225_RS_PIN, GPIO_PIN_SET)
#define RST_LOW() HAL_GPIO_WritePin(ILI9225_RST_PORT, ILI9225_RST_PIN, GPIO_PIN_RESET)
#define RST_HIGH() HAL_GPIO_WritePin(ILI9225_RST_PORT, ILI9225_RST_PIN, GPIO_PIN_SET)

#define ABS(x) ((x) > 0 ? (x) : -(x))

#ifdef ILI9225_USE_DMA
static uint16_t dma_buffer[ILI9225_WIDTH * ILI9225_DMA_BUFFER_LINES];
#endif

/* STATIC VARIABLES ---------------------------------------------------------*/

// Current rotation
static uint8_t _rotation = ILI9225_ROTATION;

/* STATIC FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Write command to ILI9225
 *
 * @param cmd Command to write
 */
static inline void ILI9225_WriteCommand(uint16_t cmd)
{
    uint8_t data[2] = {cmd >> 8, cmd & 0xFF};
    CS_LOW();
    RS_LOW();
    HAL_SPI_Transmit(&ILI9225_SPI_PORT, data, 2, HAL_MAX_DELAY);
    CS_HIGH();
}

/**
 * @brief Write data to ILI9225
 *
 * @param data Data to write
 */
static inline void ILI9225_WriteData(uint16_t data)
{
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    CS_LOW();
    RS_HIGH();
    HAL_SPI_Transmit(&ILI9225_SPI_PORT, buf, 2, HAL_MAX_DELAY);
    CS_HIGH();
}

/**
 * @brief Write register to ILI9225
 *
 * @param reg Register address
 * @param data Data to write
 */
static void ILI9225_WriteReg(uint16_t reg, uint16_t data)
{
    ILI9225_WriteCommand(reg);
    ILI9225_WriteData(data);
}

/**
 * @brief Write bulk data to ILI9225
 *
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 */
static void ILI9225_WriteDataBulk(uint8_t *data, size_t len)
{
    CS_LOW();
    RS_HIGH();
    while (len > 0)
    {
        uint16_t chunk = (len > 65535) ? 65535 : len;
        HAL_SPI_Transmit(&ILI9225_SPI_PORT, data, chunk, HAL_MAX_DELAY);
        data += chunk;
        len -= chunk;
    }
    CS_HIGH();
}

/**
 * @brief Set drawing window
 *
 * @param x0 Start X
 * @param y0 Start Y
 * @param x1 End X
 * @param y1 End Y
 */
static void ILI9225_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    switch (_rotation)
    {
    case 0: // Portrait (176x220)
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR1, x1);
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR2, x0);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR1, y1);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR2, y0);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET1, x0);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET2, y0);
        break;

    case 1: // Landscape 90° (220x176)
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR1, y1);
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR2, y0);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR1, ILI9225_LCD_HEIGHT - x0 - 1);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR2, ILI9225_LCD_HEIGHT - x1 - 1);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET1, y0);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET2, ILI9225_LCD_HEIGHT - x0 - 1);
        break;

    case 2: // Portrait 180° (176x220)
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR1, ILI9225_LCD_WIDTH - x0 - 1);
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR2, ILI9225_LCD_WIDTH - x1 - 1);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR1, ILI9225_LCD_HEIGHT - y0 - 1);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR2, ILI9225_LCD_HEIGHT - y1 - 1);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET1, ILI9225_LCD_WIDTH - x0 - 1);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET2, ILI9225_LCD_HEIGHT - y0 - 1);
        break;

    case 3: // Landscape 270° (220x176)
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR1, y1);
        ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR2, y0);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR1, x1);
        ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR2, x0);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET1, y0);
        ILI9225_WriteReg(ILI9225_RAM_ADDR_SET2, x0);
        break;
    }
    ILI9225_WriteCommand(ILI9225_GRAM_DATA_REG);
}

/**
 * @brief Perform hardware reset of ILI9225
 */
static void ILI9225_HardReset(void)
{
    RST_HIGH();
    HAL_Delay(10);
    RST_LOW();
    HAL_Delay(50);
    RST_HIGH();
    HAL_Delay(100);
}

/**
 * @brief Draw horizontal line helper
 *
 * @param x_left Left X
 * @param x_right Right X
 * @param y Y coordinate
 * @param color RGB565 color
 */
static void ILI9225_DrawHorizontalLine(int16_t x_left, int16_t x_right, int16_t y, uint16_t color)
{
    if (y < 0 || y >= ILI9225_HEIGHT)
        return;
    if (x_left < 0)
        x_left = 0;
    if (x_right >= ILI9225_WIDTH)
        x_right = ILI9225_WIDTH - 1;
    if (x_left > x_right)
        return;
    ILI9225_FillRect(x_left, y, x_right - x_left + 1, 1, color);
}

/* PUBLIC FUNCTIONS ----------------------------------------------------------*/

/**
 * @brief Initialize ILI9225 display
 */
void ILI9225_Init(void)
{
#ifdef ILI9225_USE_DMA
    memset(dma_buffer, 0, sizeof(dma_buffer));
#endif

    ILI9225_HardReset();

    ILI9225_WriteReg(ILI9225_POWER_CTRL1, 0x0000);
    ILI9225_WriteReg(ILI9225_POWER_CTRL2, 0x0000);
    ILI9225_WriteReg(ILI9225_POWER_CTRL3, 0x0000);
    ILI9225_WriteReg(ILI9225_POWER_CTRL4, 0x0000);
    ILI9225_WriteReg(ILI9225_POWER_CTRL5, 0x0000);
    HAL_Delay(40);

    ILI9225_WriteReg(ILI9225_POWER_CTRL2, 0x0018);
    ILI9225_WriteReg(ILI9225_POWER_CTRL3, 0x6121);
    ILI9225_WriteReg(ILI9225_POWER_CTRL4, 0x006F);
    ILI9225_WriteReg(ILI9225_POWER_CTRL5, 0x495F);
    ILI9225_WriteReg(ILI9225_POWER_CTRL1, 0x0800);
    HAL_Delay(10);
    ILI9225_WriteReg(ILI9225_POWER_CTRL2, 0x103B);
    HAL_Delay(50);

    ILI9225_WriteReg(ILI9225_DRIVER_OUTPUT_CTRL, 0x011C);
    ILI9225_WriteReg(ILI9225_LCD_AC_DRIVING_CTRL, 0x0100);
    ILI9225_WriteReg(ILI9225_ENTRY_MODE, 0x1030);
    ILI9225_WriteReg(ILI9225_DISP_CTRL1, 0x0000);
    ILI9225_WriteReg(ILI9225_BLANK_PERIOD_CTRL1, 0x0808);
    ILI9225_WriteReg(ILI9225_FRAME_CYCLE_CTRL, 0x1100);
    ILI9225_WriteReg(ILI9225_INTERFACE_CTRL, 0x0000);
    ILI9225_WriteReg(ILI9225_OSC_CTRL, 0x0D01);
    ILI9225_WriteReg(ILI9225_VCI_RECYCLING, 0x0020);
    ILI9225_WriteReg(ILI9225_RAM_ADDR_SET1, 0x0000);
    ILI9225_WriteReg(ILI9225_RAM_ADDR_SET2, 0x0000);

    ILI9225_WriteReg(ILI9225_GATE_SCAN_CTRL, 0x0000);
    ILI9225_WriteReg(ILI9225_VERTICAL_SCROLL_CTRL1, 0x00DB);
    ILI9225_WriteReg(ILI9225_VERTICAL_SCROLL_CTRL2, 0x0000);
    ILI9225_WriteReg(ILI9225_VERTICAL_SCROLL_CTRL3, 0x0000);
    ILI9225_WriteReg(ILI9225_PARTIAL_DRIVING_POS1, 0x00DB);
    ILI9225_WriteReg(ILI9225_PARTIAL_DRIVING_POS2, 0x0000);
    ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR1, 0x00AF);
    ILI9225_WriteReg(ILI9225_HORIZONTAL_WINDOW_ADDR2, 0x0000);
    ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR1, 0x00DB);
    ILI9225_WriteReg(ILI9225_VERTICAL_WINDOW_ADDR2, 0x0000);

    ILI9225_WriteReg(ILI9225_GAMMA_CTRL1, 0x0000);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL2, 0x0808);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL3, 0x080A);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL4, 0x000A);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL5, 0x0A08);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL6, 0x0808);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL7, 0x0000);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL8, 0x0A00);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL9, 0x0710);
    ILI9225_WriteReg(ILI9225_GAMMA_CTRL10, 0x0710);

    ILI9225_SetRotation(ILI9225_ROTATION);

    ILI9225_WriteReg(ILI9225_DISP_CTRL1, 0x0012);
    HAL_Delay(50);
    ILI9225_WriteReg(ILI9225_DISP_CTRL1, 0x1017);

    ILI9225_FillScreen(ILI9225_BLACK);
}

/**
 * @brief Set display rotation
 */
void ILI9225_SetRotation(uint8_t rotation)
{
    _rotation = rotation % 4;
    switch (_rotation)
    {
    case 0: // Portrait 0°
        ILI9225_WriteReg(ILI9225_DRIVER_OUTPUT_CTRL, 0x011C);
        ILI9225_WriteReg(ILI9225_ENTRY_MODE, 0x1030);
        break;
    case 1: // Landscape 90°
        ILI9225_WriteReg(ILI9225_DRIVER_OUTPUT_CTRL, 0x021C);
        ILI9225_WriteReg(ILI9225_ENTRY_MODE, 0x1038);
        break;
    case 2: // Portrait 180°
        ILI9225_WriteReg(ILI9225_DRIVER_OUTPUT_CTRL, 0x031C);
        ILI9225_WriteReg(ILI9225_ENTRY_MODE, 0x1030);
        break;
    case 3: // Landscape 270°
        ILI9225_WriteReg(ILI9225_DRIVER_OUTPUT_CTRL, 0x001C);
        ILI9225_WriteReg(ILI9225_ENTRY_MODE, 0x1038);
        break;
    }
}

/**
 * @brief Invert display colors
 */
void ILI9225_InvertDisplay(bool invert)
{
    (void)invert;
}

/**
 * @brief Turn display on/off
 */
void ILI9225_DisplayOn(bool on)
{
    ILI9225_WriteReg(ILI9225_DISP_CTRL1, on ? 0x1017 : 0x0000);
}

/**
 * @brief Set display brightness (if supported)
 */
void ILI9225_SetBrightness(uint8_t brightness) { (void)brightness; }

/**
 * @brief Fill screen with color
 */
void ILI9225_FillScreen(uint16_t color)
{
    ILI9225_FillRect(0, 0, ILI9225_WIDTH, ILI9225_HEIGHT, color);
}

/**
 * @brief Fill rectangle with color
 */
void ILI9225_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ILI9225_WIDTH || y >= ILI9225_HEIGHT)
        return;
    if (x + w > ILI9225_WIDTH)
        w = ILI9225_WIDTH - x;
    if (y + h > ILI9225_HEIGHT)
        h = ILI9225_HEIGHT - y;

    ILI9225_SetWindow(x, y, x + w - 1, y + h - 1);

    uint8_t colorH = color >> 8;
    uint8_t colorL = color & 0xFF;
    uint32_t pixels = (uint32_t)w * h;

    CS_LOW();
    RS_HIGH();

    uint8_t buffer[128];
    for (uint16_t i = 0; i < 128; i += 2)
    {
        buffer[i] = colorH;
        buffer[i + 1] = colorL;
    }

    while (pixels >= 64)
    {
        HAL_SPI_Transmit(&ILI9225_SPI_PORT, buffer, 128, HAL_MAX_DELAY);
        pixels -= 64;
    }

    while (pixels--)
    {
        HAL_SPI_Transmit(&ILI9225_SPI_PORT, &colorH, 1, HAL_MAX_DELAY);
        HAL_SPI_Transmit(&ILI9225_SPI_PORT, &colorL, 1, HAL_MAX_DELAY);
    }

    CS_HIGH();
}

/**
 * @brief Draw single pixel
 */
void ILI9225_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= ILI9225_WIDTH || y >= ILI9225_HEIGHT)
        return;
    ILI9225_SetWindow(x, y, x, y);
    ILI9225_WriteData(color);
}

/**
 * @brief Draw line using Bresenham's algorithm
 */
void ILI9225_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t steep = ABS(y1 - y0) > ABS(x1 - x0);
    if (steep)
    {
        uint16_t t;
        t = x0;
        x0 = y0;
        y0 = t;
        t = x1;
        x1 = y1;
        y1 = t;
    }
    if (x0 > x1)
    {
        uint16_t t;
        t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    int16_t dx = x1 - x0;
    int16_t dy = ABS(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++)
    {
        steep ? ILI9225_DrawPixel(y0, x0, color) : ILI9225_DrawPixel(x0, y0, color);
        err -= dy;
        if (err < 0)
        {
            y0 += ystep;
            err += dx;
        }
    }
}

/**
 * @brief Draw rectangle outline
 */
void ILI9225_DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ILI9225_DrawLine(x, y, x + w - 1, y, color);
    ILI9225_DrawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
    ILI9225_DrawLine(x + w - 1, y + h - 1, x, y + h - 1, color);
    ILI9225_DrawLine(x, y + h - 1, x, y, color);
}

/**
 * @brief Draw circle outline (Midpoint algorithm)
 */
void ILI9225_DrawCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;

    ILI9225_DrawPixel(x0, y0 + r, color);
    ILI9225_DrawPixel(x0, y0 - r, color);
    ILI9225_DrawPixel(x0 + r, y0, color);
    ILI9225_DrawPixel(x0 - r, y0, color);

    while (x < y)
    {
        if (f >= 0)
        {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ILI9225_DrawPixel(x0 + x, y0 + y, color);
        ILI9225_DrawPixel(x0 - x, y0 + y, color);
        ILI9225_DrawPixel(x0 + x, y0 - y, color);
        ILI9225_DrawPixel(x0 - x, y0 - y, color);
        ILI9225_DrawPixel(x0 + y, y0 + x, color);
        ILI9225_DrawPixel(x0 - y, y0 + x, color);
        ILI9225_DrawPixel(x0 + y, y0 - x, color);
        ILI9225_DrawPixel(x0 - y, y0 - x, color);
    }
}

/**
 * @brief Fill circle (Midpoint algorithm)
 */
void ILI9225_FillCircle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    if (r == 0)
        return;
    int16_t x = 0, y = r, d = 3 - 2 * r;

    while (y >= x)
    {
        ILI9225_DrawHorizontalLine(x0 - x, x0 + x, y0 + y, color);
        ILI9225_DrawHorizontalLine(x0 - x, x0 + x, y0 - y, color);
        if (x != y)
        {
            ILI9225_DrawHorizontalLine(x0 - y, x0 + y, y0 + x, color);
            ILI9225_DrawHorizontalLine(x0 - y, x0 + y, y0 - x, color);
        }
        d < 0 ? (d = d + 4 * x + 6) : (d = d + 4 * (x - y) + 10, y--);
        x++;
    }
}

/**
 * @brief Draw triangle outline
 */
void ILI9225_DrawTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t x3, uint16_t y3, uint16_t color)
{
    ILI9225_DrawLine(x1, y1, x2, y2, color);
    ILI9225_DrawLine(x2, y2, x3, y3, color);
    ILI9225_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief Fill triangle
 */
void ILI9225_FillTriangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t x3, uint16_t y3, uint16_t color)
{
    int16_t deltax = ABS(x2 - x1), deltay = ABS(y2 - y1);
    int16_t x = x1, y = y1, xinc1, xinc2, yinc1, yinc2, den, num, numadd, numpixels;

    xinc1 = xinc2 = (x2 >= x1) ? 1 : -1;
    yinc1 = yinc2 = (y2 >= y1) ? 1 : -1;

    if (deltax >= deltay)
    {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    }
    else
    {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (int16_t curpixel = 0; curpixel <= numpixels; curpixel++)
    {
        ILI9225_DrawLine(x, y, x3, y3, color);
        num += numadd;
        if (num >= den)
        {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

/**
 * @brief Draw image from buffer
 */
void ILI9225_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if (x >= ILI9225_WIDTH || y >= ILI9225_HEIGHT)
        return;
    if (x + w > ILI9225_WIDTH || y + h > ILI9225_HEIGHT)
        return;
    ILI9225_SetWindow(x, y, x + w - 1, y + h - 1);
    ILI9225_WriteDataBulk((uint8_t *)data, w * h * 2);
}

#ifdef ILI9225_USE_FONTS
/**
 * @brief Write character using specified font
 */
void ILI9225_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font,
                       uint16_t color, uint16_t bgcolor)
{
    if (x + font.width > ILI9225_WIDTH || y + font.height > ILI9225_HEIGHT)
        return;
    ILI9225_SetWindow(x, y, x + font.width - 1, y + font.height - 1);

    CS_LOW();
    RS_HIGH();

    for (uint16_t i = 0; i < font.height; i++)
    {
        uint16_t line = font.data[(ch - 32) * font.height + i];
        for (uint16_t j = 0; j < font.width; j++)
        {
            uint16_t pixel_color = (line & (0x8000 >> j)) ? color : bgcolor;
            uint8_t data[] = {pixel_color >> 8, pixel_color & 0xFF};
            HAL_SPI_Transmit(&ILI9225_SPI_PORT, data, 2, HAL_MAX_DELAY);
        }
    }
    CS_HIGH();
}

/**
 * @brief Write string using specified font
 */
void ILI9225_WriteString(uint16_t x, uint16_t y, const char *str, FontDef font,
                         uint16_t color, uint16_t bgcolor)
{
    while (*str)
    {
        if (x + font.width > ILI9225_WIDTH)
        {
            x = 0;
            y += font.height;
            if (y + font.height > ILI9225_HEIGHT)
                break;
            if (*str == ' ')
            {
                str++;
                continue;
            }
        }
        ILI9225_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }
}
#endif

/**
 * @brief Convert RGB888 to RGB565
 */
uint16_t ILI9225_Color565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// FLASH OPTIMIZATION: Comment out test function to save ~1KB
// Uncomment if you need to test display functionality
/*
void ILI9225_Test(void)
{
    const uint16_t colors[] = {ILI9225_RED, ILI9225_GREEN, ILI9225_BLUE,
        ILI9225_YELLOW, ILI9225_CYAN, ILI9225_MAGENTA, ILI9225_WHITE, ILI9225_BLACK};

    // Color test
    for (uint8_t i = 0; i < 8; i++)
    {
        ILI9225_FillScreen(colors[i]);
        HAL_Delay(500);
    }

    #ifdef ILI9225_USE_FONTS
    // Text test (OPTIMIZED: Use Font_11x18 instead of Font_16x26)
    ILI9225_FillScreen(ILI9225_BLACK);
    ILI9225_WriteString(10, 10, "ILI9225 Test", Font_11x18, ILI9225_WHITE, ILI9225_BLACK);
    ILI9225_WriteString(10, 35, "176x220 Display", Font_11x18, ILI9225_CYAN, ILI9225_BLACK);
    ILI9225_WriteString(10, 60, "2.0 inch TFT", Font_7x10, ILI9225_YELLOW, ILI9225_BLACK);
    HAL_Delay(2000);
    #endif

    // Shape test
    ILI9225_FillScreen(ILI9225_BLACK);
    ILI9225_DrawRect(10, 10, 80, 60, ILI9225_GREEN);
    ILI9225_FillCircle(150, 40, 25, ILI9225_RED);
    ILI9225_DrawTriangle(40, 100, 80, 140, 60, 160, ILI9225_YELLOW);
    ILI9225_FillRect(120, 100, 70, 50, ILI9225_BLUE);
    HAL_Delay(2000);

    // Gradient test
    ILI9225_FillScreen(ILI9225_BLACK);
    for (uint16_t i = 0; i < ILI9225_HEIGHT; i++)
    {
        uint8_t brightness = (uint8_t)((i * 255) / ILI9225_HEIGHT);
        uint16_t gradient_color = ILI9225_Color565(brightness, 0, 255 - brightness);
        ILI9225_DrawLine(0, i, ILI9225_WIDTH - 1, i, gradient_color);
    }
    HAL_Delay(2000);
}
*/
