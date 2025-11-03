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
#define SD_SEND_INTERVAL_MS 100         // Delay between sending SD records (100ms)
#define DISPLAY_UPDATE_INTERVAL_MS 1000 // Display refresh rate (1s)

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

// Sensor device
ds3231_t g_ds3231;
sht3x_t g_sht3x;
static float sensor_temp = 0.0f;
static float sensor_humidity = 0.0f;

// Periodic fetch timing
static uint32_t last_fetch_ms = 0;
uint32_t next_fetch_ms = 0; // Exposed for cmd_parser
uint32_t periodic_interval_ms = PERIODIC_PRINT_INTERVAL_MS;

// MQTT state
mqtt_state_t mqtt_current_state = MQTT_STATE_DISCONNECTED;

// Display control
bool force_display_update = false;
static uint32_t last_display_update_ms = 0;

// SD buffering control
static uint32_t last_sd_send_ms = 0;
static sd_data_record_t sd_buffer_record; // Moved to global scope (512 bytes)

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

// Task functions (cleaner separation of concerns)
static void Task_HandleUART(void);
static void Task_HandlePeriodicSensorFetch(bool *is_periodic_active);
static void Task_HandleMQTTConnectedMode(void);
static void Task_HandleMQTTDisconnectedMode(void);
static void Task_HandleDisplayUpdate(bool is_periodic_active);

// Helper functions
static uint32_t Get_Current_Timestamp(void);
static void Send_SD_BufferedRecord_If_Available(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Helper: Get current Unix timestamp from DS3231 RTC
 *
 * @return Unix timestamp (seconds since 1970-01-01)
 */
static uint32_t Get_Current_Timestamp(void)
{
    struct tm time;
    if (DS3231_Get_Time(&g_ds3231, &time) == HAL_OK)
    {
        return (uint32_t)mktime(&time);
    }
    return HAL_GetTick() / 1000; // Fallback
}

/**
 * @brief Task: Handle UART communication
 */
static void Task_HandleUART(void)
{
    UART_Handle();
}

/**
 * @brief Task: Handle periodic sensor data fetch
 *
 * @param is_periodic_active Pointer to bool indicating if periodic mode is active
 */
static void Task_HandlePeriodicSensorFetch(bool *is_periodic_active)
{
    if (!SHT3X_IS_PERIODIC_STATE(g_sht3x.currentState))
    {
        *is_periodic_active = false;
        return;
    }

    *is_periodic_active = true;
    uint32_t now = HAL_GetTick();

    // Check if it's time to fetch and prevent duplicate reads
    if ((int32_t)(now - next_fetch_ms) >= 0 && last_fetch_ms != now)
    {
        // Fetch sensor data
        SHT3X_FetchData(&g_sht3x, &sensor_temp, &sensor_humidity);

        // Update data manager with periodic data
        DataManager_UpdatePeriodic(sensor_temp, sensor_humidity);

        // Toggle LED
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        // Record fetch time and schedule next
        last_fetch_ms = now;
        next_fetch_ms = now + periodic_interval_ms;
    }
}

/**
 * @brief Task: Handle data routing when MQTT is CONNECTED
 *
 * @details Responsibilities:
 *          - Send live sensor data via UART
 *          - Send buffered SD data via UART
 */
static void Task_HandleMQTTConnectedMode(void)
{
    // 1. Send live data if ready (uses DataManager_Print internally)
    DataManager_Print();

    // 2. Send buffered SD data (non-blocking)
    Send_SD_BufferedRecord_If_Available();
}

/**
 * @brief Task: Handle data routing when MQTT is DISCONNECTED
 *
 * @details Responsibilities:
 *          - Buffer sensor data to SD card
 *          - DO NOT send data via UART
 */
static void Task_HandleMQTTDisconnectedMode(void)
{
    if (!DataManager_IsDataReady())
        return;

    const data_manager_state_t *state = DataManager_GetState();

    // Get timestamp from RTC
    uint32_t timestamp = Get_Current_Timestamp();

    // Determine mode string
    const char *mode_str = (state->mode == DATA_MANAGER_MODE_SINGLE) ? "SINGLE" : "PERIODIC";

    // Write to SD card buffer
    SDCardManager_WriteData(timestamp, state->sht3x.temperature,
                            state->sht3x.humidity, mode_str);

    // Clear flag to allow next data
    DataManager_ClearDataReady();
}

/**
 * @brief Helper: Send one buffered SD record if available
 *
 * @details Non-blocking, respects SD_SEND_INTERVAL_MS delay between records
 */
static void Send_SD_BufferedRecord_If_Available(void)
{
    uint32_t now_ms = HAL_GetTick();

    // Check buffer count and respect interval
    if (SDCardManager_GetBufferedCount() == 0)
        return;

    if ((now_ms - last_sd_send_ms) < SD_SEND_INTERVAL_MS)
        return;

    // Read one record from SD
    if (!SDCardManager_ReadData(&sd_buffer_record))
        return;

    // Format as JSON
    char json_buffer[128];
    int len = sensor_json_format(json_buffer, sizeof(json_buffer),
                                 sd_buffer_record.mode,
                                 sd_buffer_record.temperature,
                                 sd_buffer_record.humidity,
                                 sd_buffer_record.timestamp);

    // Send to ESP32 via UART
    if (len > 0)
    {
        PRINT_CLI(json_buffer);

        // Remove record after successful send
        SDCardManager_RemoveRecord();
        last_sd_send_ms = now_ms;
    }
}

/**
 * @brief Task: Update TFT display (every 1 second or when forced)
 *
 * @param is_periodic_active Indicates if periodic mode is currently active
 */
static void Task_HandleDisplayUpdate(bool is_periodic_active)
{
    uint32_t now_ms = HAL_GetTick();

    // Check update interval
    if (now_ms - last_display_update_ms < DISPLAY_UPDATE_INTERVAL_MS && !force_display_update)
        return;

    // Get current timestamp from RTC
    time_t current_time = Get_Current_Timestamp();

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

    // Reset flags
    last_display_update_ms = now_ms;
    force_display_update = false;
}

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

    // Initialize UART
    UART_Init(&huart1);

    // Initialize sensors
    SHT3X_Init(&g_sht3x, &hi2c1, SHT3X_I2C_ADDR_GND);
    DS3231_Init(&g_ds3231, &hi2c1);

    // Initialize data manager
    DataManager_Init();

    // Initialize SD Card (with power stabilization delay)
    HAL_Delay(200); // 200ms for SD card power-up
    if (!SDCardManager_Init())
    {
        PRINT_CLI("[WARN] SD Card NOT available! Data will be lost when WiFi disconnected.\r\n");
    }

    // Initialize display
    ILI9225_Init();
    HAL_Delay(50); // Display stabilization
    display_init();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    bool is_periodic_active = false;

    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        // Task 1: Handle UART communication
        Task_HandleUART();

        // Task 2: Handle periodic sensor data fetch
        Task_HandlePeriodicSensorFetch(&is_periodic_active);

        // Task 3: Route data based on MQTT state
        if (mqtt_current_state == MQTT_STATE_CONNECTED)
        {
            Task_HandleMQTTConnectedMode();
        }
        else
        {
            Task_HandleMQTTDisconnectedMode();
        }

        // Task 4: Update display
        Task_HandleDisplayUpdate(is_periodic_active);

        /* USER CODE END 3 */
    }
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
