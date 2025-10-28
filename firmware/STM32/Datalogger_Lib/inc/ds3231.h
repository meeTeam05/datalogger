/**
 * @file ds3231.h
 *
 * @brief Header file for DS3231 RTC driver for STM32 using HAL
 */

#ifndef DS3231_H
#define DS3231_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stm32f1xx_hal.h>

/* DEFINES -------------------------------------------------------------------*/

#define DS3231_ADDR (0x68 << 1) // I2C address (shifted for HAL)
#define DS3231_TIMEOUT 100      // I2C timeout in ms

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * DS3231 device handle
 */
typedef struct
{
    I2C_HandleTypeDef *hi2c;
} ds3231_t;

/**
 * Alarms
 */
typedef enum
{
    DS3231_ALARM_NONE = 0, // No alarms
    DS3231_ALARM_1 = 1,    // First alarm
    DS3231_ALARM_2 = 2,    // Second alarm
    DS3231_ALARM_BOTH = 3  // Both alarms
} ds3231_alarm_t;

/**
 * First alarm rate
 */
typedef enum
{
    DS3231_ALARM1_EVERY_SECOND = 0,    // Every second
    DS3231_ALARM1_MATCH_SEC,           // Match seconds
    DS3231_ALARM1_MATCH_SECMIN,        // Match seconds and minutes
    DS3231_ALARM1_MATCH_SECMINHOUR,    // Match seconds, minutes, and hours
    DS3231_ALARM1_MATCH_SECMINHOURDAY, // Match seconds, minutes, hours, and day
    DS3231_ALARM1_MATCH_SECMINHOURDATE // Match seconds, minutes, hours, and date
} ds3231_alarm1_rate_t;

/**
 * Second alarm rate
 */
typedef enum
{
    DS3231_ALARM2_EVERY_MIN = 0,    // Every minute
    DS3231_ALARM2_MATCH_MIN,        // Match minutes
    DS3231_ALARM2_MATCH_MINHOUR,    // Match minutes and hours
    DS3231_ALARM2_MATCH_MINHOURDAY, // Match minutes, hours, and day
    DS3231_ALARM2_MATCH_MINHOURDATE // Match minutes, hours, and date
} ds3231_alarm2_rate_t;

/**
 * Squarewave frequency
 */
typedef enum
{
    DS3231_SQWAVE_1HZ = 0x00,    // 1 Hz
    DS3231_SQWAVE_1024HZ = 0x08, // 1024 Hz
    DS3231_SQWAVE_4096HZ = 0x10, // 4096 Hz
    DS3231_SQWAVE_8192HZ = 0x18  // 8192 Hz
} ds3231_sqwave_freq_t;

/* VARIABLES -----------------------------------------------------------------*/

// Global DS3231 device handle
extern ds3231_t g_ds3231;

// Time structure for setting time
extern struct tm time_to_set;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize DS3231 device
 *
 * @param dev Device handle
 * @param hi2c I2C handle
 */
void DS3231_Init(ds3231_t *dev, I2C_HandleTypeDef *hi2c);

/**
 * @brief Deinitialize DS3231 device
 *
 * @param dev Device handle
 * @param hi2c I2C handle
 */
void DS3231_DeInit(ds3231_t *dev);

/**
 * @brief Set the time on the RTC
 *
 * @param dev Device handle
 * @param time Time structure
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Set_Time(ds3231_t *dev, struct tm *time);

/**
 * @brief Get the time from the RTC
 *
 * @param dev Device handle
 * @param time Time structure to store result
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Time(ds3231_t *dev, struct tm *time);

/**
 * @brief Set alarms
 *
 * @param dev Device handle
 * @param alarms Which alarm(s) to set
 * @param time1 Time for alarm 1
 * @param option1 Alarm 1 rate
 * @param time2 Time for alarm 2
 * @param option2 Alarm 2 rate
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Set_Alarm(ds3231_t *dev, ds3231_alarm_t alarms,
                                   struct tm *time1, ds3231_alarm1_rate_t option1,
                                   struct tm *time2, ds3231_alarm2_rate_t option2);

/**
 * @brief Check if oscillator has previously stopped
 *
 * @param dev Device handle
 * @param flag Stop flag output
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Oscillator_Stop_Flag(ds3231_t *dev, bool *flag);

/**
 * @brief Clear the oscillator stopped flag
 *
 * @param dev Device handle
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Clear_Oscillator_Stop_Flag(ds3231_t *dev);

/**
 * @brief Check which alarm(s) have triggered
 *
 * @param dev Device handle
 * @param alarms Alarm flags output
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Alarm_Flags(ds3231_t *dev, ds3231_alarm_t *alarms);

/**
 * @brief Clear alarm flag(s)
 *
 * @param dev Device handle
 * @param alarms Which alarm(s) to clear
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Clear_Alarm_Flags(ds3231_t *dev, ds3231_alarm_t alarms);

/**
 * @brief Enable alarm interrupts
 *
 * @param dev Device handle
 * @param alarms Which alarm(s) to enable
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Enable_Alarm_Ints(ds3231_t *dev, ds3231_alarm_t alarms);

/**
 * @brief Disable alarm interrupts
 *
 * @param dev Device handle
 * @param alarms Which alarm(s) to disable
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Disable_Alarm_Ints(ds3231_t *dev, ds3231_alarm_t alarms);

/**
 * @brief Enable 32kHz output
 *
 * @param dev Device handle
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Enable_32khz(ds3231_t *dev);

/**
 * @brief Disable 32kHz output
 *
 * @param dev Device handle
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Disable_32khz(ds3231_t *dev);

/**
 * @brief Enable squarewave output
 *
 * @param dev Device handle
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Enable_Squarewave(ds3231_t *dev);

/**
 * @brief Disable squarewave output
 *
 * @param dev Device handle
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Disable_Squarewave(ds3231_t *dev);

/**
 * @brief Set squarewave frequency
 *
 * @param dev Device handle
 * @param freq Frequency to set
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Set_Squarewave_Freq(ds3231_t *dev, ds3231_sqwave_freq_t freq);

/**
 * @brief Get squarewave frequency
 *
 * @param dev Device handle
 * @param freq Frequency output
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Squarewave_Freq(ds3231_t *dev, ds3231_sqwave_freq_t *freq);

/**
 * @brief Get raw temperature value
 *
 * @param dev Device handle
 * @param temp Raw temperature output
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Raw_Temp(ds3231_t *dev, int16_t *temp);

/**
 * @brief Get temperature as integer
 *
 * @param dev Device handle
 * @param temp Temperature in degrees Celsius
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Temp_Integer(ds3231_t *dev, int8_t *temp);

/**
 * @brief Get temperature as float
 *
 * @param dev Device handle
 * @param temp Temperature in degrees Celsius
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Temp_Float(ds3231_t *dev, float *temp);

/**
 * @brief Set aging offset
 *
 * @param dev Device handle
 * @param age Aging offset [-128, 127]
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Set_Aging_Offset(ds3231_t *dev, int8_t age);

/**
 * @brief Get aging offset
 *
 * @param dev Device handle
 * @param age Aging offset output
 *
 * @return HAL_OK on success
 */
HAL_StatusTypeDef DS3231_Get_Aging_Offset(ds3231_t *dev, int8_t *age);

#endif /* DS3231_H */