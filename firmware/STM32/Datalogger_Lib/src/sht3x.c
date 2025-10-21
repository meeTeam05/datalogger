/**
 * @file sht3x.c
 *
 * @brief Source file for the SHT3x sensor driver
 */

/* INCLUDES ------------------------------------------------------------------*/

#include <assert.h>
#include "sht3x.h"
#include "print_cli.h"

/* DEFINES -------------------------------------------------------------------*/

/* Bit positions */
#define SHT3X_STATUS_ALERT_PENDING_Pos 15u /*!< Alert pending bit */
#define SHT3X_STATUS_HEATER_Pos 13u		   /*!< Heater status bit */
#define SHT3X_STATUS_RH_ALERT_Pos 11u	   /*!< Relative humidity alert bit */
#define SHT3X_STATUS_T_ALERT_Pos 10u	   /*!< Temperature alert bit */
#define SHT3X_STATUS_SYS_RESET_Pos 4u	   /*!< System reset detected bit */
#define SHT3X_STATUS_CMD_STATUS_Pos 1u	   /*!< Command status bit */
#define SHT3X_STATUS_WRITE_CRC_Pos 0u	   /*!< Write data checksum status bit */

/* Bit masks */
#define SHT3X_STATUS_ALERT_PENDING (1u << SHT3X_STATUS_ALERT_PENDING_Pos) /*!< Alert pending bit mask */
#define SHT3X_STATUS_HEATER (1u << SHT3X_STATUS_HEATER_Pos)				  /*!< Heater status bit mask */
#define SHT3X_STATUS_RH_ALERT (1u << SHT3X_STATUS_RH_ALERT_Pos)			  /*!< Relative humidity alert bit mask */
#define SHT3X_STATUS_T_ALERT (1u << SHT3X_STATUS_T_ALERT_Pos)			  /*!< Temperature alert bit mask */
#define SHT3X_STATUS_SYS_RESET (1u << SHT3X_STATUS_SYS_RESET_Pos)		  /*!< System reset detected bit mask */
#define SHT3X_STATUS_CMD_STATUS (1u << SHT3X_STATUS_CMD_STATUS_Pos)		  /*!< Command status bit mask */
#define SHT3X_STATUS_WRITE_CRC (1u << SHT3X_STATUS_WRITE_CRC_Pos)		  /*!< Write data checksum status bit mask */

/* Reserved bits */
#define SHT3X_STATUS_RESERVED14 (1u << 14)	/*!< bit 14 */
#define SHT3X_STATUS_RESERVED12 (1u << 12)	/*!< bit 12 */
#define SHT3X_STATUS_RESERVED_9_5 (0x03E0u) /*!< bits [9:5] */
#define SHT3X_STATUS_RESERVED_3_2 (0x000Cu) /*!< bits [3:2] */
#define SHT3X_STATUS_RESERVED_MASK (SHT3X_STATUS_RESERVED14 |   \
									SHT3X_STATUS_RESERVED12 |   \
									SHT3X_STATUS_RESERVED_9_5 | \
									SHT3X_STATUS_RESERVED_3_2) /*!< Reserved bits mask */

/* Macro function status */
#define SHT3X_STATUS_IS_HEATER_ON(x) (((x) & SHT3X_STATUS_HEATER) != 0u)	  /*!< Check if heater is on */
#define SHT3X_STATUS_CMD_FAILED(x) (((x) & SHT3X_STATUS_CMD_STATUS) != 0u)	  /*!< Check if last command failed */
#define SHT3X_STATUS_WRITE_CRC_FAIL(x) (((x) & SHT3X_STATUS_WRITE_CRC) != 0u) /*!< Check if last write CRC failed */
#define SHT3X_STATUS_RESET_DETECTED(x) (((x) & SHT3X_STATUS_SYS_RESET) != 0u) /*!< Check if system reset was detected */
#define SHT3X_STATUS_ALERT_ANY (SHT3X_STATUS_ALERT_PENDING | \
								SHT3X_STATUS_RH_ALERT |      \
								SHT3X_STATUS_T_ALERT) /*!< Any alert condition */

/*  SHT3X COMMAND SET --------------------------------------------------------*/

/* Reset */
#define SHT3X_COMMAND_SOFT_RESET 0x30A2

/* Status */
#define SHT3X_COMMAND_READ_STATUS 0xF32D
#define SHT3X_COMMAND_CLEAR_STATUS 0x3041

/* Heater Control */
#define SHT3X_COMMAND_HEATER_ENABLE 0x306D
#define SHT3X_COMMAND_HEATER_DISABLE 0x3066

/* ART Mode*/
#define SHT3X_COMMAND_ART 0x2B32

/* Periodic Data Flow */
#define SHT3X_COMMAND_FETCH_DATA 0xE000
#define SHT3X_COMMAND_STOP_PERIODIC_MEAS 0x3093

/*  PRIVATE VARIABLES --------------------------------------------------------*/

// Measurement command lookup table

static const uint16_t SHT3X_MEASURE_CMD[6][3] = {
	{0x2400, 0x240b, 0x2416}, /*!< [SINGLE_SHOT][H,M,L] without clock stretching */
	{0x2032, 0x2024, 0x202f}, /*!< [PERIODIC_05][H,M,L] */
	{0x2130, 0x2126, 0x212d}, /*!< [PERIODIC_1 ][H,M,L] */
	{0x2236, 0x2220, 0x222b}, /*!< [PERIODIC_2 ][H,M,L] */
	{0x2334, 0x2322, 0x2329}, /*!< [PERIODIC_4 ][H,M,L] */
	{0x2737, 0x2721, 0x272a}  /*!< [PERIODIC_10][H,M,L] */
};

// Measurement duration lookup table (in milliseconds)
static const uint8_t SHT3X_MEAS_DURATION_MS[3] = {
	15, /*!< HIGH */
	6,	/*!< MEDIUM */
	4	/*!< LOW */
};

/* PRIVATE FUNCTIONS ---------------------------------------------------------*/

/**
 * @brief Combines two uint8_t values into a single uint16_t value
 *
 * @param msb Most significant byte
 * @param lsb Least significant byte
 *
 * @return Combined uint16_t value
 */
static inline uint16_t uint8_to_uint16(uint8_t msb, uint8_t lsb)
{
	return (uint16_t)(((uint16_t)msb << 8) | (uint16_t)lsb);
}

/**
 * @brief Calculates the CRC checksum for a given data array
 *
 * @param data Pointer to the data array
 * @param len Length of the data array
 *
 * @return Calculated CRC checksum
 */
static inline uint8_t SHT3X_CRC(const uint8_t *data, size_t len)
{
	uint8_t crc = 0xFF;

	for (size_t i = 0; i < len; ++i)
	{
		crc ^= data[i];
		for (int b = 0; b < 8; ++b)
		{
			uint8_t msb = crc & 0x80;
			crc <<= 1U;
			if (msb)
			{
				crc ^= 0x31;
			}
		}
	}
	return crc;
}

/**
 * @brief Sends a command to the SHT3x sensor via I2C
 *
 * @param dev Pointer to the SHT3x device structure
 * @param command 16-bit command to be sent
 *
 * @return HAL_StatusTypeDef Status of the I2C transmission (HAL_OK or HAL_ERROR)
 */
static HAL_StatusTypeDef SHT3X_Send_Command(const sht3x_t *dev, uint16_t command)
{
	if (!dev || !dev->hi2c)
	{
		return HAL_ERROR;
	}

	uint8_t command_buffer[2] = {(uint8_t)((command >> 8) & 0xFF), (uint8_t)(command & 0xFF)};

	if (HAL_I2C_Master_Transmit(dev->hi2c,
								(uint16_t)(dev->device_address),		// 7-bit addr
								command_buffer, sizeof(command_buffer), // 2 bytes
								SHT3X_I2C_TIMEOUT) != HAL_OK)
	{
		return HAL_ERROR;
	}

	return HAL_OK;
}

/**
 * @brief Parses the raw data frame from the SHT3x sensor
 *
 * @param frame Pointer to the raw data frame (6 bytes)
 * @param tC Pointer to store the parsed temperature value (in degrees Celsius)
 * @param rh Pointer to store the parsed relative humidity value (in percentage)
 *
 * @return SHT3X_StatusTypeDef Status of the parsing operation (SHT3X_OK or SHT3X_ERROR)
 */
static SHT3X_StatusTypeDef SHT3X_ParseFrame(const uint8_t frame[SHT3X_RAW_DATA_SIZE], float *tC, float *rh)
{
	/* CRC check for T and RH words */
	if (SHT3X_CRC(&frame[0], 2) != frame[2])
	{
		return SHT3X_ERROR;
	}

	if (SHT3X_CRC(&frame[3], 2) != frame[5])
	{
		return SHT3X_ERROR;
	}

	uint16_t rawT = uint8_to_uint16(frame[0], frame[1]);
	uint16_t rawRH = uint8_to_uint16(frame[3], frame[4]);

	/* Convert per datasheet */
	if (tC)
		*tC = -45.0f + (175.0f * (float)rawT / 65535.0f);
	if (rh)
		*rh = 100.0f * (float)rawRH / 65535.0f;

	return SHT3X_OK;
}

/**
 * @brief Reads the status register from the SHT3x sensor
 *
 * @param dev Pointer to the SHT3x device structure
 * @param state_word Pointer to store the read status word
 *
 * @return SHT3X_StatusTypeDef Status of the read operation (SHT3X_OK or SHT3X_ERROR)
 */
static SHT3X_StatusTypeDef SHT3X_ReadStatus(sht3x_t *dev, uint16_t *state_word)
{
	if (!dev || !dev->hi2c)
	{
		return SHT3X_ERROR;
	}

	uint8_t read_buffer[3]; // [0]=MSB, [1]=LSB, [2]=CRC

	if (HAL_I2C_Mem_Read(dev->hi2c,
						 (uint16_t)(dev->device_address), // 7-bit addr
						 SHT3X_COMMAND_READ_STATUS,		  // 0xF32D
						 I2C_MEMADD_SIZE_16BIT,			  // send [MSB:LSB]
						 read_buffer, sizeof(read_buffer),
						 SHT3X_I2C_TIMEOUT) != HAL_OK)
	{
		return SHT3X_ERROR;
	}

	if (SHT3X_CRC(read_buffer, 2) != read_buffer[2])
	{
		return SHT3X_ERROR;
	}

	*state_word = (uint16_t)((read_buffer[0] << 8) | read_buffer[1]); // MSB-first -> little-endian

	return SHT3X_OK;
}

/* PUBLIC API ----------------------------------------------------------------*/

/**
 * @brief Initialize SHT3x device structure
 */
void SHT3X_Init(sht3x_t *dev, I2C_HandleTypeDef *hi2c, uint8_t addr7bit)
{

	if (!dev || !hi2c)
	{
		return;
	}

	if (addr7bit != SHT3X_I2C_ADDR_GND && addr7bit != SHT3X_I2C_ADDR_VDD)
	{
		return; // invalid 7-bit address
	}

	assert(hi2c->Init.NoStretchMode == I2C_NOSTRETCH_DISABLE);
	if (hi2c->Init.NoStretchMode != I2C_NOSTRETCH_DISABLE)
	{
		return;
	}

	if (hi2c->Init.AddressingMode != I2C_ADDRESSINGMODE_7BIT)
	{
		return;
	}

	dev->hi2c = hi2c;
	dev->device_address = addr7bit;
	dev->temperature = 0.0f;
	dev->humidity = 0.0f;
	dev->currentState = SHT3X_IDLE;
	dev->modeRepeat = SHT3X_HIGH;

	if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(addr7bit),
							  3, SHT3X_I2C_TIMEOUT) != HAL_OK)
	{

		return;
	}

	if (SHT3X_Send_Command(dev, SHT3X_COMMAND_SOFT_RESET) != HAL_OK)
	{
		return;
	}
	HAL_Delay(2);

	if (SHT3X_Send_Command(dev, SHT3X_COMMAND_CLEAR_STATUS) != HAL_OK)
	{
		return;
	}
	HAL_Delay(1);
}

/**
 * @brief De-initialize SHT3x device structure
 */
void SHT3X_DeInit(sht3x_t *dev)
{
	if (!dev || !dev->hi2c)
	{
		return;
	}

	if (HAL_I2C_IsDeviceReady(dev->hi2c, (uint16_t)(dev->device_address),
							  3, SHT3X_I2C_TIMEOUT) != HAL_OK)
	{
		return;
	}

	if (SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_STOP_PERIODIC_MEAS) != HAL_OK)
		{
			return;
		}
		HAL_Delay(1);

		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_CLEAR_STATUS) != HAL_OK)
		{
			return;
		}
		HAL_Delay(1);

		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_SOFT_RESET) != HAL_OK)
		{
			return;
		}
		HAL_Delay(2);
	}

	dev->hi2c = NULL;
	dev->device_address = 0;
	dev->temperature = 0.0f;
	dev->humidity = 0.0f;
	dev->currentState = SHT3X_IDLE;
	dev->modeRepeat = SHT3X_HIGH;
}

/**
 * @brief Enable or disable the heater on the SHT3x sensor
 */
SHT3X_StatusTypeDef SHT3X_Heater(sht3x_t *dev, const sht3x_heater_mode_t *modeHeater)
{
	if (!dev || !dev->hi2c || !modeHeater)
	{
		return SHT3X_ERROR;
	}

	uint16_t cmd;
	switch (*modeHeater)
	{
	case SHT3X_HEATER_ENABLE:
		cmd = SHT3X_COMMAND_HEATER_ENABLE;
		break;
	case SHT3X_HEATER_DISABLE:
		cmd = SHT3X_COMMAND_HEATER_DISABLE;
		break;
	default:
		return SHT3X_ERROR;
	}

	if (SHT3X_Send_Command(dev, cmd) != HAL_OK)
	{
		return SHT3X_ERROR;
	}

	HAL_Delay(1);

	uint16_t state_word = 0;
	if (SHT3X_ReadStatus(dev, &state_word) != SHT3X_OK)
	{
		return SHT3X_ERROR;
	}

	if (SHT3X_STATUS_CMD_FAILED(state_word) || SHT3X_STATUS_WRITE_CRC_FAIL(state_word))
	{
		return SHT3X_ERROR;
	}

	if ((*modeHeater == SHT3X_HEATER_ENABLE && !SHT3X_STATUS_IS_HEATER_ON(state_word)) ||
		(*modeHeater == SHT3X_HEATER_DISABLE && SHT3X_STATUS_IS_HEATER_ON(state_word)))
	{
		return SHT3X_ERROR;
	}

	return SHT3X_OK;
}

/**
 * @brief Perform a single measurement on the SHT3x sensor
 */
SHT3X_StatusTypeDef SHT3X_Single(sht3x_t *dev, sht3x_repeat_t *modeRepeat, float *outT, float *outRH)
{
	// Initialize output to 0.0 (default for sensor failure)
	if (outT)
		*outT = 0.0f;
	if (outRH)
		*outRH = 0.0f;

	if (!dev || !dev->hi2c || !modeRepeat)
	{
		return SHT3X_ERROR;
	}

	sht3x_mode_t savedMode = dev->currentState;
	sht3x_repeat_t savedRepeat = dev->modeRepeat;

	if (SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_STOP_PERIODIC_MEAS) != HAL_OK)
		{
			return SHT3X_ERROR;
		}
		// Critical: Sensor needs minimum 10ms to stop periodic mode completely
		// per SHT3X datasheet. Shorter delay causes unreliable single measurements.
		HAL_Delay(10);

		// Clear any pending status to ensure clean state
		SHT3X_Send_Command(dev, SHT3X_COMMAND_CLEAR_STATUS);
		HAL_Delay(1);

		dev->currentState = SHT3X_IDLE;
	}

	if (SHT3X_Send_Command(dev, SHT3X_MEASURE_CMD[0][*modeRepeat]) != HAL_OK)
	{
		return SHT3X_ERROR;
	}

	HAL_Delay(SHT3X_MEAS_DURATION_MS[*modeRepeat]);

	uint8_t frame[SHT3X_RAW_DATA_SIZE] = {0};
	if (HAL_I2C_Master_Receive(dev->hi2c, (uint16_t)(dev->device_address),
							   frame, sizeof(frame), SHT3X_I2C_TIMEOUT) != HAL_OK)
	{
		// I2C error, output remains 0.0
		return SHT3X_ERROR;
	}

	float tC = 0.0f, rh = 0.0f;
	if (SHT3X_ParseFrame(frame, &tC, &rh) != SHT3X_OK)
	{
		// Parse error (CRC failure), output remains 0.0
		return SHT3X_ERROR;
	}

	// Valid data, update device state and output
	dev->temperature = tC;
	dev->humidity = rh;

	if (outT)
	{
		*outT = tC;
	}
	if (outRH)
	{
		*outRH = rh;
	}

	// sensor_json_output_send("SINGLE", g_sht3x.temperature, g_sht3x.humidity);

	// Restore periodic mode if it was active before single measurement
	if (SHT3X_IS_PERIODIC_STATE(savedMode))
	{
		// Add delay after single measurement before restarting periodic
		// This prevents race condition where sensor hasn't finished processing
		HAL_Delay(5);

		// Clear status register before restarting periodic mode
		// This ensures sensor is in clean state after single measurement
		SHT3X_Send_Command(dev, SHT3X_COMMAND_CLEAR_STATUS);
		HAL_Delay(1);

		// Restart periodic mode manually to avoid fetching stale data
		// Don't use SHT3X_Periodic() which calls FetchData immediately
		uint8_t row;
		switch (savedMode)
		{
		case SHT3X_PERIODIC_05MPS:
			row = 1;
			break;
		case SHT3X_PERIODIC_1MPS:
			row = 2;
			break;
		case SHT3X_PERIODIC_2MPS:
			row = 3;
			break;
		case SHT3X_PERIODIC_4MPS:
			row = 4;
			break;
		case SHT3X_PERIODIC_10MPS:
			row = 5;
			break;
		default:
			dev->currentState = SHT3X_IDLE;
			return SHT3X_OK; // Single measurement succeeded, just can't restart periodic
		}

		// Send periodic command and update state
		if (SHT3X_Send_Command(dev, SHT3X_MEASURE_CMD[row][savedRepeat]) == HAL_OK)
		{
			dev->currentState = savedMode;
			dev->modeRepeat = savedRepeat;
		}
		else
		{
			// Periodic restart failed, but single measurement succeeded
			dev->currentState = SHT3X_IDLE;
		}
		// PRINT_CLI("currentState: %d, modeRepeat: %d\r\n", dev->currentState, dev->modeRepeat);
	}
	else
	{
		dev->currentState = SHT3X_SINGLE_SHOT;
		dev->modeRepeat = *modeRepeat;
		// PRINT_CLI("currentState: %d, modeRepeat: %d\r\n", dev->currentState, dev->modeRepeat);
	}

	return SHT3X_OK;
}

/**
 * @brief Start periodic measurements on the SHT3x sensor
 */
SHT3X_StatusTypeDef SHT3X_Periodic(sht3x_t *dev, sht3x_mode_t *modePeriodic, sht3x_repeat_t *modeRepeat,
								   float *outT, float *outRH)
{
	// Initialize output to 0.0 (default for sensor failure)
	if (outT)
		*outT = 0.0f;
	if (outRH)
		*outRH = 0.0f;

	if (!dev || !dev->hi2c || !modePeriodic || !modeRepeat)
	{
		return SHT3X_ERROR;
	}

	if (SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_STOP_PERIODIC_MEAS) != HAL_OK)
		{
			return SHT3X_ERROR;
		}
		HAL_Delay(1);
	}

	uint8_t row;
	switch (*modePeriodic)
	{
	case SHT3X_PERIODIC_05MPS:
		row = 1;
		break;
	case SHT3X_PERIODIC_1MPS:
		row = 2;
		break;
	case SHT3X_PERIODIC_2MPS:
		row = 3;
		break;
	case SHT3X_PERIODIC_4MPS:
		row = 4;
		break;
	case SHT3X_PERIODIC_10MPS:
		row = 5;
		break;
	default:
		return HAL_ERROR;
	}

	// Set state BEFORE sending command so periodic fetch continues even if sensor fails
	dev->currentState = *modePeriodic;
	dev->modeRepeat = *modeRepeat;

	// Try to send periodic measurement command to sensor
	// If this fails, state is already set so main loop will continue fetching 0.0/0.0
	SHT3X_Send_Command(dev, SHT3X_MEASURE_CMD[row][*modeRepeat]);

	// Wait for first measurement to be ready (use measurement duration based on repeatability)
	HAL_Delay(SHT3X_MEAS_DURATION_MS[*modeRepeat]);

	// Fetch first measurement (will be 0.0/0.0 if sensor fails)
	SHT3X_FetchData(dev, outT, outRH);

	return SHT3X_OK;
}

/**
 * @brief Start ART mode on the SHT3x sensor
 */
SHT3X_StatusTypeDef SHT3X_ART(sht3x_t *dev)
{
	if (!dev || !dev->hi2c)
	{
		return SHT3X_ERROR;
	}

	if (SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		if (SHT3X_Send_Command(dev, SHT3X_COMMAND_STOP_PERIODIC_MEAS) != HAL_OK)
		{
			return SHT3X_ERROR;
		}
		HAL_Delay(1);
	}

	if (SHT3X_Send_Command(dev, SHT3X_COMMAND_ART) != HAL_OK)
	{
		return SHT3X_ERROR;
	}

	dev->currentState = SHT3X_PERIODIC_4MPS;
	dev->modeRepeat = SHT3X_HIGH;

	return SHT3X_OK;
}

/**
 * @brief Stop periodic measurements on the SHT3x sensor
 */
SHT3X_StatusTypeDef SHT3X_Stop_Periodic(sht3x_t *dev)
{
	if (!dev || !dev->hi2c)
	{
		return SHT3X_ERROR;
	}
	if (!SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		dev->currentState = SHT3X_IDLE;
		return SHT3X_OK; /* nothing to stop */
	}

	if (SHT3X_Send_Command(dev, SHT3X_COMMAND_STOP_PERIODIC_MEAS) != HAL_OK)
	{
		return SHT3X_ERROR;
	}
	HAL_Delay(1);

	dev->currentState = SHT3X_IDLE;

	return SHT3X_OK;
}

/**
 * @brief Fetch the latest measurement data from the SHT3x sensor in periodic mode
 */
void SHT3X_FetchData(sht3x_t *dev, float *outT, float *outRH)
{
	// Initialize output to 0.0 (default for sensor failure)
	if (outT)
		*outT = 0.0f;
	if (outRH)
		*outRH = 0.0f;

	if (!dev || !dev->hi2c)
	{
		return;
	}

	if (!SHT3X_IS_PERIODIC_STATE(dev->currentState))
	{
		return;
	}

	uint8_t frame[SHT3X_RAW_DATA_SIZE] = {0};
	if (HAL_I2C_Mem_Read(dev->hi2c,
						 (uint16_t)(dev->device_address),
						 SHT3X_COMMAND_FETCH_DATA, I2C_MEMADD_SIZE_16BIT,
						 frame, sizeof(frame), SHT3X_I2C_TIMEOUT) != HAL_OK)
	{
		// I2C error, output remains 0.0
		return;
	}

	float tC = 0.0f, rh = 0.0f;
	if (SHT3X_ParseFrame(frame, &tC, &rh) != SHT3X_OK)
	{
		// Parse error (CRC failure), output remains 0.0
		return;
	}

	// Valid data, update device state and output
	dev->temperature = tC;
	dev->humidity = rh;

	if (outT)
	{
		*outT = tC;
	}
	if (outRH)
	{
		*outRH = rh;
	}
}
