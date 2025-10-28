/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "uart.h"
#include "sht3x.h"
#include "ds3231.h"
#include "data_manager.h"
#include "wifi_manager.h"
#include "sd_card_manager.h"
#include "sensor_json_output.h"
#include "print_cli.h"
#include "ili9225.h"
#include "display.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PERIODIC_PRINT_INTERVAL_MS 5000 // Interval to print periodic data (5 seconds)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

// External variable for display update control
extern bool force_display_update;

// Periodic fetch variables
static uint32_t last_fetch_ms = 0;                          // Track last fetch time to prevent duplicates
uint32_t next_fetch_ms = 0;                                 // Exposed for cmd_parser to reset timing
uint32_t periodic_interval_ms = PERIODIC_PRINT_INTERVAL_MS; // Interval to print periodic data (5 seconds)

// MQTT state tracking - Default to DISCONNECTED (ESP32 will notify if connected)
mqtt_state_t mqtt_current_state = MQTT_STATE_DISCONNECTED;

// Display update flag - Set to true when time is changed via SET TIME command
bool force_display_update = false;

// SHT3X device instance
sht3x_t g_sht3x;
static float outT = 0.0f;
static float outRH = 0.0f;

// DS3231 device instance
ds3231_t g_ds3231;
struct tm time_to_set;
struct tm time_to_get;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  /* USER CODE BEGIN 2 */

  UART_Init(&huart1);

  /* Initialize SHT3X */
  SHT3X_Init(&g_sht3x, &hi2c1, SHT3X_I2C_ADDR_GND);

  /* Initialize DS3231 */
  DS3231_Init(&g_ds3231, &hi2c1);

  /* Initialize DataManager */
  DataManager_Init();

  /* Initialize SD Card Manager - Continue even if SD fails */

  /* CRITICAL: Wait for SD card power stabilization */
  HAL_Delay(200); // 200ms power-up delay for SD card (some cards need more time)

  if (!SDCardManager_Init())
  {
    PRINT_CLI("[WARN] SD Card NOT available! Data will be lost when WiFi disconnected.\r\n");
  }

  /* Initialize ILI9225 TFT Display */
  ILI9225_Init();
  HAL_Delay(50); // Display stabilization delay

  /* Initialize Display Library */
  display_init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  // Display update variables
  static uint32_t last_display_update_ms = 0;
  static bool is_periodic_active = false;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    UART_Handle();

    /* Handle periodic sensor data fetch */
    if (SHT3X_IS_PERIODIC_STATE(g_sht3x.currentState))
    {
      is_periodic_active = true;
      uint32_t now = HAL_GetTick();

      if ((int32_t)(now - next_fetch_ms) >= 0 && last_fetch_ms != now)
      {
        // Fetch data from sensor
        SHT3X_FetchData(&g_sht3x, &outT, &outRH);

        // Update data manager with periodic data
        DataManager_UpdatePeriodic(outT, outRH);

        // Toggle GPIO
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        // Record this fetch time to prevent duplicates
        last_fetch_ms = now;

        // Schedule next fetch
        next_fetch_ms = now + periodic_interval_ms;
      }
    }
    else
    {
      is_periodic_active = false;
    }

    /* MQTT-aware data routing logic */
    if (mqtt_current_state == MQTT_STATE_CONNECTED)
    {
      /* MQTT CONNECTED - Send live data + buffered data */

      // 1. Print current live data if ready (uses centralized DataManager_Print)
      // DataManager_Print() will automatically clear data_ready flag
      DataManager_Print();

      // 2. Send buffered data from SD (non-blocking, one record per loop)
      static uint32_t last_sd_send_ms = 0;
      uint32_t now_ms = HAL_GetTick();

      if (SDCardManager_GetBufferedCount() > 0 && (now_ms - last_sd_send_ms) >= 100) // 100ms delay between SD records
      {
        static sd_data_record_t buffered_record;
        if (SDCardManager_ReadData(&buffered_record))
        {
          // Format buffered record as JSON using the same function DataManager uses
          char json_buffer[128];
          int len = sensor_json_format(json_buffer, sizeof(json_buffer),
                                       buffered_record.mode,
                                       buffered_record.temperature,
                                       buffered_record.humidity,
                                       buffered_record.timestamp);

          // Send to ESP32 via UART
          if (len > 0)
          {
            PRINT_CLI(json_buffer); // Send the JSON string

            // Remove record after successful send
            SDCardManager_RemoveRecord();
            last_sd_send_ms = now_ms;
          }
        }
      }
    }
    else
    {
      /* MQTT DISCONNECTED - Buffer data to SD card (don't print to UART) */

      if (DataManager_IsDataReady())
      {
        const data_manager_state_t *state = DataManager_GetState();

        // Get timestamp from RTC (if available)
        uint32_t timestamp = 0;
        struct tm time;
        if (DS3231_Get_Time(&g_ds3231, &time) == HAL_OK)
        {
          timestamp = (uint32_t)mktime(&time);
        }
        else
        {
          timestamp = HAL_GetTick() / 1000; // Use systick as fallback
        }

        // Determine mode string
        const char *mode_str = (state->mode == DATA_MANAGER_MODE_SINGLE) ? "SINGLE" : "PERIODIC";

        // Write to SD card buffer
        SDCardManager_WriteData(timestamp, state->sht3x.temperature, state->sht3x.humidity, mode_str);

        // Clear flag to allow next data
        DataManager_ClearDataReady();
      }
    }

    /* Update Display (every 1 second for smooth clock update OR when forced) */
    uint32_t now_ms = HAL_GetTick();
    if (now_ms - last_display_update_ms >= 1000 || force_display_update)
    {
      // Get current timestamp from RTC (ALWAYS read from DS3231, not increment)
      time_t current_time = 0;
      struct tm time;
      if (DS3231_Get_Time(&g_ds3231, &time) == HAL_OK)
      {
        current_time = mktime(&time);
      }
      else
      {
        current_time = HAL_GetTick() / 1000; // Fallback to systick
      }

      // Get sensor data from data manager
      const data_manager_state_t *state = DataManager_GetState();
      float display_temp = state->sht3x.valid ? state->sht3x.temperature : 0.0f;
      float display_humi = state->sht3x.valid ? state->sht3x.humidity : 0.0f;

      // Determine MQTT connection status
      bool mqtt_connected = (mqtt_current_state == MQTT_STATE_CONNECTED);

      // Calculate interval in seconds
      int interval_seconds = periodic_interval_ms / 1000;

      // Update display
      display_update(current_time, display_temp, display_humi,
                     mqtt_connected, is_periodic_active, interval_seconds);

      // Reset flags after update
      last_display_update_ms = now_ms;
      force_display_update = false; // Clear force update flag
    }
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin | ILI9225_RST_Pin | ILI9225_RS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ILI9225_CS_GPIO_Port, ILI9225_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin ILI9225_RST_Pin ILI9225_RS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin | ILI9225_RST_Pin | ILI9225_RS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ILI9225_CS_Pin */
  GPIO_InitStruct.Pin = ILI9225_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ILI9225_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
