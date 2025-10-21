/**
 * @file ds3231.c
 *
 * @brief Source file for DS3231 RTC driver for STM32 using HAL
 */

/* INCLUDES ------------------------------------------------------------------*/

#include "ds3231.h"
#include <string.h>

/* DEFINES -------------------------------------------------------------------*/

// Register addresses
#define DS3231_ADDR_TIME 0x00
#define DS3231_ADDR_ALARM1 0x07
#define DS3231_ADDR_ALARM2 0x0B
#define DS3231_ADDR_CONTROL 0x0E
#define DS3231_ADDR_STATUS 0x0F
#define DS3231_ADDR_AGING 0x10
#define DS3231_ADDR_TEMP 0x11

// Control register bits
#define DS3231_CTRL_OSCILLATOR 0x80
#define DS3231_CTRL_TEMPCONV 0x20
#define DS3231_CTRL_ALARM_INTS 0x04
#define DS3231_CTRL_ALARM2_INT 0x02
#define DS3231_CTRL_ALARM1_INT 0x01

// Status register bits
#define DS3231_STAT_OSCILLATOR 0x80
#define DS3231_STAT_32KHZ 0x08
#define DS3231_STAT_ALARM_2 0x02
#define DS3231_STAT_ALARM_1 0x01

// Alarm bits
#define DS3231_ALARM_WDAY 0x40
#define DS3231_ALARM_NOTSET 0x80

// Time format bits
#define DS3231_12HOUR_FLAG 0x40
#define DS3231_12HOUR_MASK 0x1F
#define DS3231_PM_FLAG 0x20
#define DS3231_MONTH_MASK 0x1F

// Operation modes
enum
{
    DS3231_SET = 0,
    DS3231_CLEAR,
    DS3231_REPLACE
};

/*  DS3231 STATIC VARIABLES --------------------------------------------------*/

// Days per month
static const int days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int days_per_month_leap[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* STATIC FUNCTIONs ----------------------------------------------------------*/

/**
 * @brief Convert BCD to decimal
 *
 * @param val BCD value
 *
 * @return Decimal value
 */
static uint8_t bcd2dec(uint8_t val)
{
    return (val >> 4) * 10 + (val & 0x0F);
}

/**
 * @brief Convert decimal to BCD
 *
 * @param val Decimal value
 *
 * @return BCD value
 */
static uint8_t dec2bcd(uint8_t val)
{
    return ((val / 10) << 4) + (val % 10);
}

/**
 * @brief Calculate days since January 1st of the year
 *
 * @param year Year since 1900
 * @param month Month (0-11)
 * @param day Day of month (1-31)
 *
 * @return Number of days since January 1st
 */
static int days_since_january_1st(int year, int month, int day)
{
    int days = day - 1;
    const int *ptr = days_per_month;

    // Check leap year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        ptr = days_per_month_leap;

    // Add days from previous months
    for (int i = 0; i < month; i++)
        days += ptr[i];

    return days;
}

/**
 * @brief Write to register
 *
 * @param dev Pointer to DS3231 device structure
 * @param reg Register address
 * @param data Pointer to data buffer
 * @param len Length of data
 *
 * @return HAL status
 */
static HAL_StatusTypeDef DS3231_Write_Reg(ds3231_t *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    return HAL_I2C_Mem_Write(dev->hi2c, DS3231_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, DS3231_TIMEOUT);
}

/**
 * @brief Read from register
 *
 * @param dev Pointer to DS3231 device structure
 * @param reg Register address
 * @param data Pointer to data buffer
 * @param len Length of data
 *
 * @return HAL status
 */
static HAL_StatusTypeDef DS3231_Read_Reg(ds3231_t *dev, uint8_t reg, uint8_t *data, uint8_t len)
{
    return HAL_I2C_Mem_Read(dev->hi2c, DS3231_ADDR, reg, I2C_MEMADD_SIZE_8BIT, data, len, DS3231_TIMEOUT);
}

/**
 * @brief Get flag from register
 *
 * @param dev Pointer to DS3231 device structure
 * @param addr Register address
 * @param mask Bit mask
 * @param flag Pointer to store flag value
 *
 * @return HAL status
 */
static HAL_StatusTypeDef DS3231_Get_Flag(ds3231_t *dev, uint8_t addr, uint8_t mask, uint8_t *flag)
{
    uint8_t data;
    HAL_StatusTypeDef res = DS3231_Read_Reg(dev, addr, &data, 1);
    if (res == HAL_OK)
        *flag = (data & mask);
    return res;
}

/**
 * @brief Set/Clear/Replace flag in register
 *
 * @param dev Pointer to DS3231 device structure
 * @param addr Register address
 * @param bits Bits to set/clear/replace
 * @param mode Operation mode (DS3231_SET, DS3231_CLEAR, DS3231_REPLACE)
 *
 * @return HAL status
 */
static HAL_StatusTypeDef DS3231_Set_Flag(ds3231_t *dev, uint8_t addr, uint8_t bits, uint8_t mode)
{
    uint8_t data;
    HAL_StatusTypeDef res = DS3231_Read_Reg(dev, addr, &data, 1);
    if (res != HAL_OK)
        return res;

    if (mode == DS3231_REPLACE)
        data = bits;
    else if (mode == DS3231_SET)
        data |= bits;
    else
        data &= ~bits;

    return DS3231_Write_Reg(dev, addr, &data, 1);
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize DS3231 device structure
 */
void DS3231_Init(ds3231_t *dev, I2C_HandleTypeDef *hi2c)
{
    if (!dev || !hi2c)
    {
        return;
    }

    dev->hi2c = hi2c;
}

/**
 * @brief Deinitialize DS3231 device structure
 */
void DS3231_DeInit(ds3231_t *dev)
{
    if (!dev)
    {
        return;
    }

    dev->hi2c = NULL;
}

/**
 * @brief Set the time on the RTC
 */
HAL_StatusTypeDef DS3231_Set_Time(ds3231_t *dev, struct tm *time)
{
    if (!dev || !time)
        return HAL_ERROR;

    uint8_t data[7];

    data[0] = dec2bcd(time->tm_sec);
    data[1] = dec2bcd(time->tm_min);
    data[2] = dec2bcd(time->tm_hour);
    data[3] = dec2bcd(time->tm_wday + 1);
    data[4] = dec2bcd(time->tm_mday);
    data[5] = dec2bcd(time->tm_mon + 1);
    data[6] = dec2bcd(time->tm_year - 100);

    return DS3231_Write_Reg(dev, DS3231_ADDR_TIME, data, 7);
}

/**
 * @brief Get the time from the RTC
 */
HAL_StatusTypeDef DS3231_Get_Time(ds3231_t *dev, struct tm *time)
{
    if (!dev || !time)
        return HAL_ERROR;

    uint8_t data[7];
    HAL_StatusTypeDef res = DS3231_Read_Reg(dev, DS3231_ADDR_TIME, data, 7);
    if (res != HAL_OK)
        return res;

    time->tm_sec = bcd2dec(data[0]);
    time->tm_min = bcd2dec(data[1]);

    if (data[2] & DS3231_12HOUR_FLAG)
    {
        // 12-hour mode
        time->tm_hour = bcd2dec(data[2] & DS3231_12HOUR_MASK) - 1;
        if (data[2] & DS3231_PM_FLAG)
            time->tm_hour += 12;
    }
    else
    {
        // 24-hour mode
        time->tm_hour = bcd2dec(data[2]);
    }

    time->tm_wday = bcd2dec(data[3]) - 1;
    time->tm_mday = bcd2dec(data[4]);
    time->tm_mon = bcd2dec(data[5] & DS3231_MONTH_MASK) - 1;
    time->tm_year = bcd2dec(data[6]) + 100;
    time->tm_isdst = 0;
    time->tm_yday = days_since_january_1st(time->tm_year, time->tm_mon, time->tm_mday);

    return HAL_OK;
}

/**
 * @brief Set alarms
 */
HAL_StatusTypeDef DS3231_Set_Alarm(ds3231_t *dev, ds3231_alarm_t alarms,
                                   struct tm *time1, ds3231_alarm1_rate_t option1,
                                   struct tm *time2, ds3231_alarm2_rate_t option2)
{
    if (!dev)
        return HAL_ERROR;

    uint8_t data[7];
    int i = 0;

    // Alarm 1
    if (alarms != DS3231_ALARM_2)
    {
        if (!time1)
            return HAL_ERROR;

        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SEC ? dec2bcd(time1->tm_sec) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMIN ? dec2bcd(time1->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 >= DS3231_ALARM1_MATCH_SECMINHOUR ? dec2bcd(time1->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option1 == DS3231_ALARM1_MATCH_SECMINHOURDAY ? (dec2bcd(time1->tm_wday + 1) | DS3231_ALARM_WDAY) : (option1 == DS3231_ALARM1_MATCH_SECMINHOURDATE ? dec2bcd(time1->tm_mday) : DS3231_ALARM_NOTSET));
    }

    // Alarm 2
    if (alarms != DS3231_ALARM_1)
    {
        if (!time2)
            return HAL_ERROR;

        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MIN ? dec2bcd(time2->tm_min) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 >= DS3231_ALARM2_MATCH_MINHOUR ? dec2bcd(time2->tm_hour) : DS3231_ALARM_NOTSET);
        data[i++] = (option2 == DS3231_ALARM2_MATCH_MINHOURDAY ? (dec2bcd(time2->tm_wday + 1) | DS3231_ALARM_WDAY) : (option2 == DS3231_ALARM2_MATCH_MINHOURDATE ? dec2bcd(time2->tm_mday) : DS3231_ALARM_NOTSET));
    }

    uint8_t addr = (alarms == DS3231_ALARM_2) ? DS3231_ADDR_ALARM2 : DS3231_ADDR_ALARM1;
    return DS3231_Write_Reg(dev, addr, data, i);
}

/**
 * @brief Get oscillator stop flag
 */
HAL_StatusTypeDef DS3231_Get_Oscillator_Stop_Flag(ds3231_t *dev, bool *flag)
{
    if (!dev || !flag)
        return HAL_ERROR;

    uint8_t f;
    HAL_StatusTypeDef res = DS3231_Get_Flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, &f);
    if (res == HAL_OK)
        *flag = (f != 0);

    return res;
}

/**
 * @brief Clear oscillator stop flag
 */
HAL_StatusTypeDef DS3231_Clear_Oscillator_Stop_Flag(ds3231_t *dev)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_OSCILLATOR, DS3231_CLEAR);
}

/**
 * @brief Get alarm flags
 */
HAL_StatusTypeDef DS3231_Get_Alarm_Flags(ds3231_t *dev, ds3231_alarm_t *alarms)
{
    if (!dev || !alarms)
        return HAL_ERROR;

    return DS3231_Get_Flag(dev, DS3231_ADDR_STATUS, DS3231_ALARM_BOTH, (uint8_t *)alarms);
}

/**
 * @brief Clear alarm flags
 */
HAL_StatusTypeDef DS3231_Clear_Alarm_Flags(ds3231_t *dev, ds3231_alarm_t alarms)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_STATUS, alarms, DS3231_CLEAR);
}

/**
 * @brief Enable alarm interrupts
 */
HAL_StatusTypeDef DS3231_Enable_Alarm_Ints(ds3231_t *dev, ds3231_alarm_t alarms)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL,
                           DS3231_CTRL_ALARM_INTS | alarms, DS3231_SET);
}

/**
 * @brief Disable alarm interrupts
 */
HAL_StatusTypeDef DS3231_Disable_Alarm_Ints(ds3231_t *dev, ds3231_alarm_t alarms)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL, alarms, DS3231_CLEAR);
}

/**
 * @brief Enable 32kHz output
 */
HAL_StatusTypeDef DS3231_Enable_32khz(ds3231_t *dev)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_SET);
}

/**
 * @brief Disable 32kHz output
 */
HAL_StatusTypeDef DS3231_Disable_32khz(ds3231_t *dev)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_STATUS, DS3231_STAT_32KHZ, DS3231_CLEAR);
}

/**
 * @brief Enable squarewave output
 */
HAL_StatusTypeDef DS3231_Enable_Squarewave(ds3231_t *dev)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_CLEAR);
}

/**
 * @brief Disable squarewave output
 */
HAL_StatusTypeDef DS3231_Disable_Squarewave(ds3231_t *dev)
{
    if (!dev)
        return HAL_ERROR;

    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_ALARM_INTS, DS3231_SET);
}

/**
 * @brief Set squarewave frequency
 */
HAL_StatusTypeDef DS3231_Set_Squarewave_Freq(ds3231_t *dev, ds3231_sqwave_freq_t freq)
{
    if (!dev)
        return HAL_ERROR;

    uint8_t flag;
    HAL_StatusTypeDef res = DS3231_Get_Flag(dev, DS3231_ADDR_CONTROL, 0xFF, &flag);
    if (res != HAL_OK)
        return res;

    flag &= ~DS3231_SQWAVE_8192HZ;
    flag |= freq;

    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL, flag, DS3231_REPLACE);
}

/**
 * @brief Get squarewave frequency
 */
HAL_StatusTypeDef DS3231_Get_Squarewave_Freq(ds3231_t *dev, ds3231_sqwave_freq_t *freq)
{
    if (!dev || !freq)
        return HAL_ERROR;

    uint8_t flag;
    HAL_StatusTypeDef res = DS3231_Get_Flag(dev, DS3231_ADDR_CONTROL, 0xFF, &flag);
    if (res == HAL_OK)
    {
        flag &= DS3231_SQWAVE_8192HZ;
        *freq = (ds3231_sqwave_freq_t)flag;
    }

    return res;
}

/**
 * @brief Get raw temperature value
 */
HAL_StatusTypeDef DS3231_Get_Raw_Temp(ds3231_t *dev, int16_t *temp)
{
    if (!dev || !temp)
        return HAL_ERROR;

    uint8_t data[2];
    HAL_StatusTypeDef res = DS3231_Read_Reg(dev, DS3231_ADDR_TEMP, data, 2);
    if (res == HAL_OK)
        *temp = (int16_t)(int8_t)data[0] << 2 | data[1] >> 6;

    return res;
}

/**
 * @brief Get temperature as integer
 */
HAL_StatusTypeDef DS3231_Get_Temp_Integer(ds3231_t *dev, int8_t *temp)
{
    if (!temp)
        return HAL_ERROR;

    int16_t t_int;
    HAL_StatusTypeDef res = DS3231_Get_Raw_Temp(dev, &t_int);
    if (res == HAL_OK)
        *temp = t_int >> 2;

    return res;
}

/**
 * @brief Get temperature as float
 */
HAL_StatusTypeDef DS3231_Get_Temp_Float(ds3231_t *dev, float *temp)
{
    if (!temp)
        return HAL_ERROR;

    int16_t t_int;
    HAL_StatusTypeDef res = DS3231_Get_Raw_Temp(dev, &t_int);
    if (res == HAL_OK)
        *temp = t_int * 0.25f;

    return res;
}

/**
 * @brief Set aging offset
 */
HAL_StatusTypeDef DS3231_Set_Aging_Offset(ds3231_t *dev, int8_t age)
{
    if (!dev)
        return HAL_ERROR;

    uint8_t age_u8 = (uint8_t)age;
    HAL_StatusTypeDef res = DS3231_Write_Reg(dev, DS3231_ADDR_AGING, &age_u8, 1);
    if (res != HAL_OK)
        return res;

    // Trigger temperature conversion to apply aging offset immediately
    return DS3231_Set_Flag(dev, DS3231_ADDR_CONTROL, DS3231_CTRL_TEMPCONV, DS3231_SET);
}

/**
 * @brief Get aging offset
 */
HAL_StatusTypeDef DS3231_Get_Aging_Offset(ds3231_t *dev, int8_t *age)
{
    if (!dev || !age)
        return HAL_ERROR;

    uint8_t age_u8;
    HAL_StatusTypeDef res = DS3231_Read_Reg(dev, DS3231_ADDR_AGING, &age_u8, 1);
    if (res == HAL_OK)
        *age = (int8_t)age_u8;

    return res;
}
