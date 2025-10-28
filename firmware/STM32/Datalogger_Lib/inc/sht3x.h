/**
 * @file sht3x.h
 *
 * @brief Header file for the SHT3x sensor driver
 */

#ifndef SHT3X_H
#define SHT3X_H

/* INCLUDES ------------------------------------------------------------------*/

#include <stdint.h>
#include <stm32f1xx_hal.h>

/* DEFINES -------------------------------------------------------------------*/

// Define I2C timeout in ms
#define SHT3X_I2C_TIMEOUT 100

// Define address of the SHT3x sensor (7-bit)
#define SHT3X_I2C_ADDR_GND 0x44 << 1
#define SHT3X_I2C_ADDR_VDD 0x45 << 1

// Define size of raw data frame: 2 bytes T + 1 byte CRC + 2 bytes RH + 1 byte CRC
#define SHT3X_RAW_DATA_SIZE 6

/* MACROS --------------------------------------------------------------------*/

// Define macros to check the current state
#define SHT3X_IS_PERIODIC_STATE(s) ((s) == SHT3X_PERIODIC_05MPS || \
									(s) == SHT3X_PERIODIC_1MPS ||  \
									(s) == SHT3X_PERIODIC_2MPS ||  \
									(s) == SHT3X_PERIODIC_4MPS ||  \
									(s) == SHT3X_PERIODIC_10MPS)

/* TYPEDEFS ------------------------------------------------------------------*/

/**
 * @typedef enum SHT3X_StatusTypeDef
 *
 * @brief Status return values for SHT3X functions
 */
typedef enum
{
	SHT3X_OK = 0, /*!< Operation successful */
	SHT3X_ERROR	  /*!< Operation failed */
} SHT3X_StatusTypeDef;

/**
 * @typedef enum sht3x_heater_mode_t
 *
 * @brief Enumeration for heater modes of the SHT3x sensor
 */
typedef enum
{
	SHT3X_HEATER_ENABLE = 0, /*!< Enable heater */
	SHT3X_HEATER_DISABLE	 /*!< Disable heater */
} sht3x_heater_mode_t;

/**
 * @typedef enum sht3x_repeat_t
 *
 * @brief Enumeration for repeatability modes of the SHT3x sensor
 */
typedef enum
{
	SHT3X_HIGH = 0, /*!< HIGH */
	SHT3X_MEDIUM,	/*!< MEDIUM */
	SHT3X_LOW		/*!< LOW */
} sht3x_repeat_t;

/**
 * @typedef enum sht3x_mode_t
 *
 * @brief Enumeration for operation modes of the SHT3x sensor
 */
typedef enum
{
	SHT3X_IDLE = 0,		  /*!< initial state */
	SHT3X_SINGLE_SHOT,	  /*!< one single measurement */
	SHT3X_PERIODIC_05MPS, /*!< periodic with 0.5 measurements per second (mps) */
	SHT3X_PERIODIC_1MPS,  /*!< periodic with   1 measurements per second (mps) */
	SHT3X_PERIODIC_2MPS,  /*!< periodic with   2 measurements per second (mps) */
	SHT3X_PERIODIC_4MPS,  /*!< periodic with   4 measurements per second (mps) */
	SHT3X_PERIODIC_10MPS  /*!< periodic with  10 measurements per second (mps) */
} sht3x_mode_t;

/**
 * @typedef struct sht3x_t
 *
 * @brief Structure representing the SHT3x sensor device
 */
typedef struct
{
	I2C_HandleTypeDef *hi2c;	 /*!< I2C handle pointer */
	uint8_t device_address;		 /*!< 7-bit I2C address */
	float temperature, humidity; /*!< last measured values */
	sht3x_mode_t currentState;	 /*!< current mode (idle, single shot, periodic) */
	sht3x_repeat_t modeRepeat;	 /*!< current repeatability (high, medium, low) */
} sht3x_t;

/* EXTERNAL VARIABLES --------------------------------------------------------*/

// Global instance of the SHT3x sensor
extern sht3x_t g_sht3x;

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initializes the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param hi2c Pointer to the I2C handle
 * @param addr7bit 7-bit I2C address of the sensor (SHT3X_I2C_ADDR_GND or SHT3X_I2C_ADDR_VDD)
 */
void SHT3X_Init(sht3x_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7bit);

/**
 * @brief De-initializes the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 */
void SHT3X_DeInit(sht3x_t *dev);

/**
 * @brief Controls the heater of the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param modeHeater Pointer to the heater mode (enable or disable)
 *
 * @return SHT3X_StatusTypeDef Status of the operation (SHT3X_OK or SHT3X_ERROR)
 */
SHT3X_StatusTypeDef SHT3X_Heater(sht3x_t *dev, const sht3x_heater_mode_t *modeHeater);

/**
 * @brief Performs a single measurement with the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param modeRepeat Pointer to the repeatability mode (HIGH, MEDIUM, LOW)
 * @param outT Pointer to store the measured temperature value (in degrees Celsius), can be NULL
 * @param outRH Pointer to store the measured relative humidity value (in percentage), can be NULL
 *
 * @return SHT3X_StatusTypeDef Status of the operation (SHT3X_OK or SHT3X_ERROR)
 */
SHT3X_StatusTypeDef SHT3X_Single(sht3x_t *dev, sht3x_repeat_t *modeRepeat, float *outT, float *outRH);

/**
 * @brief Starts periodic measurements with the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param modePeriodic Pointer to the periodic measurement mode (05MPS, 1MPS, 2MPS, 4MPS, 10MPS)
 * @param modeRepeat Pointer to the repeatability mode (HIGH, MEDIUM, LOW)
 *
 * @return SHT3X_StatusTypeDef Status of the operation (SHT3X_OK or SHT3X_ERROR)
 */
SHT3X_StatusTypeDef SHT3X_Periodic(sht3x_t *dev, sht3x_mode_t *modePeriodic, sht3x_repeat_t *modeRepeat,
								   float *outT, float *outRH);

/** @brief Performs a self-test on the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 *
 * @return SHT3X_StatusTypeDef Status of the operation (SHT3X_OK or SHT3X_ERROR)
 */
SHT3X_StatusTypeDef SHT3X_ART(sht3x_t *dev);

/** @brief Stops periodic measurements with the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 *
 * @return SHT3X_StatusTypeDef Status of the operation (SHT3X_OK or SHT3X_ERROR)
 */
SHT3X_StatusTypeDef SHT3X_Stop_Periodic(sht3x_t *dev);

/** @brief Fetches the latest measurement data from the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param outT Pointer to store the fetched temperature value (in degrees Celsius)
 * @param outRH Pointer to store the fetched relative humidity value (in percentage)
 */
void SHT3X_FetchData(sht3x_t *dev, float *outT, float *outRH);

#endif /* SHT3X_H */
