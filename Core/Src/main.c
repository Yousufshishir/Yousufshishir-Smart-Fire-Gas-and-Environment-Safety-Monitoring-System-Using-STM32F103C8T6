/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Fire, Gas & Environment Safety Monitoring System
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "fonts.h"
#include "ssd1306.h"
#include "stdio.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */
#define TRIG_PIN GPIO_PIN_9
#define TRIG_PORT GPIOA
#define ECHO_PIN GPIO_PIN_8
#define ECHO_PORT GPIOA

#define FLAME_PIN GPIO_PIN_0
#define FLAME_PORT GPIOB

#define BUZZER_PIN GPIO_PIN_10
#define BUZZER_PORT GPIOA

#define DHT11_PORT GPIOB
#define DHT11_PIN GPIO_PIN_9

/* MQ2 Gas Sensor - Digital Output (DO) only, no voltage divider needed */
#define GAS_PIN GPIO_PIN_1
#define GAS_PORT GPIOB

/* LED Definitions */
#define RED_LED_PIN GPIO_PIN_11
#define RED_LED_PORT GPIOA
#define GREEN_LED_PIN GPIO_PIN_12
#define GREEN_LED_PORT GPIOA

#define OBSTACLE_THRESHOLD 50  // 50cm threshold for obstacle detection

uint32_t pMillis;
uint32_t cMillis;
uint32_t Value1 = 0;
uint32_t Value2 = 0;
uint16_t Distance = 0;
uint8_t DistanceValid = 0;
uint8_t obstacleDetected = 0;

// DHT11 variables
uint8_t RHI = 0, RHD = 0, TCI = 0, TCD = 0, SUM = 0;
uint8_t DHT11_Valid = 0;
float tCelsius = 0;
float tFahrenheit = 0;
float RH = 0;
uint8_t TFI = 0;
uint8_t TFD = 0;
uint8_t fireDetected = 0;
uint8_t gasDetected = 0;
char strCopy[32];

// Display control
uint8_t bootScreenShown = 0;
uint32_t displayCounter = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void microDelay(uint16_t delay);
uint8_t DHT11_Start(void);
uint8_t DHT11_Read(void);
void ReadAllSensors(void);
void UpdateDisplay(void);
void ShowBootScreen(void);
void ControlIndicators(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void microDelay(uint16_t delay)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < delay);
}

uint8_t DHT11_Start(void)
{
    uint8_t Response = 0;
    GPIO_InitTypeDef GPIO_InitStructPrivate = {0};

    GPIO_InitStructPrivate.Pin = DHT11_PIN;
    GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    microDelay(30);

    GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
    GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStructPrivate);

    uint32_t timeout = HAL_GetTick() + 2;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (HAL_GetTick() < timeout));

    if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
        timeout = HAL_GetTick() + 2;
        while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (HAL_GetTick() < timeout));
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) {
            timeout = HAL_GetTick() + 2;
            while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (HAL_GetTick() < timeout));
            if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
                Response = 1;
        }
    }
    return Response;
}

uint8_t DHT11_Read(void)
{
    uint8_t a, b = 0;
    for (a = 0; a < 8; a++) {
        uint32_t timeout = HAL_GetTick() + 2;
        while (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (HAL_GetTick() < timeout));
        microDelay(40);
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
            b |= (1 << (7 - a));
        else
            b &= ~(1 << (7 - a));
        timeout = HAL_GetTick() + 2;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && (HAL_GetTick() < timeout));
    }
    return b;
}

void ReadAllSensors(void)
{
    // ===== FLAME SENSOR =====
    fireDetected = !HAL_GPIO_ReadPin(FLAME_PORT, FLAME_PIN);

    // ===== MQ2 GAS SENSOR (Digital Output only) =====
    gasDetected = !HAL_GPIO_ReadPin(GAS_PORT, GAS_PIN);

    // ===== ULTRASONIC =====
    DistanceValid = 0;
    obstacleDetected = 0;

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < 10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    pMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) && (pMillis + 10 > HAL_GetTick()));

    if (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) {
        Value1 = __HAL_TIM_GET_COUNTER(&htim1);
        pMillis = HAL_GetTick();
        while ((HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) && (pMillis + 60 > HAL_GetTick()));
        if (!HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) {
            Value2 = __HAL_TIM_GET_COUNTER(&htim1);
            if (Value2 > Value1) {
                Distance = (Value2 - Value1) * 0.034 / 2;
                DistanceValid = 1;
                if (Distance < OBSTACLE_THRESHOLD) {
                    obstacleDetected = 1;
                }
            }
        }
    }

    // ===== DHT11 =====
    DHT11_Valid = 0;
    if (DHT11_Start()) {
        RHI = DHT11_Read();
        RHD = DHT11_Read();
        TCI = DHT11_Read();
        TCD = DHT11_Read();
        SUM = DHT11_Read();
        if ((RHI + RHD + TCI + TCD) == SUM) {
            tCelsius = (float)TCI + (float)(TCD / 10.0);
            tFahrenheit = tCelsius * 9.0 / 5.0 + 32.0;
            RH = (float)RHI + (float)(RHD / 10.0);
            TFI = (uint8_t)tFahrenheit;
            TFD = (uint8_t)((tFahrenheit - TFI) * 10);
            DHT11_Valid = 1;
        }
    }
}

void ControlIndicators(void)
{
    // RED LED = Warning (Fire, Gas, or Obstacle)
    // GREEN LED = System Safe (No warnings)
    // Buzzer = Warning (Fire, Gas, or Obstacle)

    if (fireDetected || gasDetected || obstacleDetected) {
        // ===== WARNING STATE =====
        HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);      // RED ON
        HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET); // GREEN OFF
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_SET);         // BUZZER ON
    } else {
        // ===== SAFE STATE =====
        HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);     // RED OFF
        HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);   // GREEN ON
        HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);       // BUZZER OFF
    }
}

void ShowBootScreen(void)
{
    SSD1306_Clear();

    // Using smaller font (Font_7x10) for boot screen
    SSD1306_GotoXY(5, 5);
    SSD1306_Puts("Smart Fire,", &Font_7x10, 1);
    SSD1306_GotoXY(5, 20);
    SSD1306_Puts("Gas & Env", &Font_7x10, 1);
    SSD1306_GotoXY(5, 35);
    SSD1306_Puts("Monitor", &Font_7x10, 1);

    // Version info
    SSD1306_GotoXY(5, 50);
    SSD1306_Puts("v1.0", &Font_7x10, 1);

    SSD1306_UpdateScreen();
    HAL_Delay(3000);
    bootScreenShown = 1;
}

void UpdateDisplay(void)
{
    SSD1306_Clear();
    displayCounter++;

    if (fireDetected) {
        // ===== FULL SCREEN FIRE ALERT =====
        SSD1306_GotoXY(5, 0);
        SSD1306_Puts("!!! FIRE !!!", &Font_11x18, 1);
        SSD1306_GotoXY(5, 20);
        SSD1306_Puts("EVACUATE NOW!", &Font_11x18, 1);

        // Show sensor values
        SSD1306_GotoXY(0, 42);
        if (DHT11_Valid)
            sprintf(strCopy, "T:%dC H:%d%%", TCI, RHI);
        else
            sprintf(strCopy, "T:--C H:--%%");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        SSD1306_GotoXY(0, 52);
        if (DistanceValid)
            sprintf(strCopy, "D:%dcm", Distance);
        else
            sprintf(strCopy, "D:---cm");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        // Blinking border
        if ((displayCounter / 5) % 2 == 0) {
            for (int i = 0; i < 128; i++) {
                SSD1306_DrawPixel(i, 0, 1);
                SSD1306_DrawPixel(i, 63, 1);
            }
            for (int i = 0; i < 64; i++) {
                SSD1306_DrawPixel(0, i, 1);
                SSD1306_DrawPixel(127, i, 1);
            }
        }

    } else if (gasDetected) {
        // ===== FULL SCREEN GAS ALERT =====
        SSD1306_GotoXY(5, 0);
        SSD1306_Puts("!! GAS LEAK !!", &Font_11x18, 1);
        SSD1306_GotoXY(5, 20);
        SSD1306_Puts("GAS DETECTED", &Font_11x18, 1);

        SSD1306_GotoXY(0, 42);
        if (DHT11_Valid)
            sprintf(strCopy, "T:%dC H:%d%%", TCI, RHI);
        else
            sprintf(strCopy, "T:--C H:--%%");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        SSD1306_GotoXY(0, 52);
        if (DistanceValid)
            sprintf(strCopy, "D:%dcm", Distance);
        else
            sprintf(strCopy, "D:---cm");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        if ((displayCounter / 5) % 2 == 0) {
            for (int i = 0; i < 128; i++) {
                SSD1306_DrawPixel(i, 0, 1);
                SSD1306_DrawPixel(i, 63, 1);
            }
            for (int i = 0; i < 64; i++) {
                SSD1306_DrawPixel(0, i, 1);
                SSD1306_DrawPixel(127, i, 1);
            }
        }

    } else if (obstacleDetected) {
        // ===== OBSTACLE ALERT - FIXED TEXT OVERFLOW =====
        SSD1306_GotoXY(0, 0);
        SSD1306_Puts("! OBSTACLE !", &Font_11x18, 1);

        // Distance on next line with proper spacing
        SSD1306_GotoXY(0, 22);
        if (DistanceValid) {
            sprintf(strCopy, "Dist: %dcm", Distance);
            SSD1306_Puts(strCopy, &Font_11x18, 1);
        } else {
            SSD1306_Puts("Dist: ---cm", &Font_11x18, 1);
        }

        // Show other sensor values at bottom
        SSD1306_GotoXY(0, 44);
        if (DHT11_Valid) {
            sprintf(strCopy, "T:%dC H:%d%%", TCI, RHI);
        } else {
            sprintf(strCopy, "T:--C H:--%%");
        }
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        SSD1306_GotoXY(0, 54);
        if (fireDetected)
            SSD1306_Puts("FIRE!", &Font_7x10, 1);
        else if (gasDetected)
            SSD1306_Puts("GAS!", &Font_7x10, 1);
        else
            SSD1306_Puts("System: SAFE", &Font_7x10, 1);

        // Blinking border
        if ((displayCounter / 5) % 2 == 0) {
            for (int i = 0; i < 128; i++) {
                SSD1306_DrawPixel(i, 0, 1);
                SSD1306_DrawPixel(i, 63, 1);
            }
            for (int i = 0; i < 64; i++) {
                SSD1306_DrawPixel(0, i, 1);
                SSD1306_DrawPixel(127, i, 1);
            }
        }

    } else {
        // ===== NORMAL DISPLAY =====
        // Line 1: Distance
        SSD1306_GotoXY(0, 0);
        if (DistanceValid) {
            sprintf(strCopy, "Dist: %3dcm", Distance);
        } else {
            sprintf(strCopy, "Dist: ---cm");
        }
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        // Line 2: Temperature
        SSD1306_GotoXY(0, 12);
        if (DHT11_Valid)
            sprintf(strCopy, "Temp: %2d.%dC", TCI, TCD);
        else
            sprintf(strCopy, "Temp: --.-C");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        // Line 3: Humidity
        SSD1306_GotoXY(0, 24);
        if (DHT11_Valid)
            sprintf(strCopy, "Hum: %2d.%d%%", RHI, RHD);
        else
            sprintf(strCopy, "Hum: --.-%%");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        // Line 4: Fahrenheit
        SSD1306_GotoXY(0, 36);
        if (DHT11_Valid)
            sprintf(strCopy, "F: %d.%dF", TFI, TFD);
        else
            sprintf(strCopy, "Status: Normal");
        SSD1306_Puts(strCopy, &Font_7x10, 1);

        // Line 5: Fire / Gas Status
        SSD1306_GotoXY(0, 48);
        if (fireDetected) {
            SSD1306_Puts("> FIRE DETECTED <", &Font_7x10, 1);
        } else if (gasDetected) {
            SSD1306_Puts("> GAS DETECTED <", &Font_7x10, 1);
        } else {
            SSD1306_Puts("System: SAFE", &Font_7x10, 1);
        }
    }

    SSD1306_UpdateScreen();
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
    HAL_TIM_Base_Start(&htim1);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);

    // Initialize DHT11 pin
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);

    // Initialize OLED
    SSD1306_Init();
    SSD1306_Clear();
    SSD1306_UpdateScreen();

    // Show boot screen
    ShowBootScreen();

    // LED Test
    HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(RED_LED_PORT, RED_LED_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GREEN_LED_PORT, GREEN_LED_PIN, GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
        // Read sensors every 500ms
        static uint32_t lastReadTime = 0;
        uint32_t currentTime = HAL_GetTick();

        if (currentTime - lastReadTime >= 500) {
            lastReadTime = currentTime;
            ReadAllSensors();

            // Control indicators (LEDs, Buzzer)
            ControlIndicators();

            // Update display
            UpdateDisplay();
        }

        HAL_Delay(10);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
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
  hi2c1.Init.ClockSpeed = 400000;
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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);  // BUZZER LOW
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);  // RED LED LOW
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);    // GREEN LED HIGH
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET);     // DHT11 HIGH

  /*Configure GPIO pins : PB0 - FLAME SENSOR */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB1 - GAS SENSOR */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA8 - ECHO */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA9 - TRIG */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA10 - BUZZER */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA11 - RED LED */
  GPIO_InitStruct.Pin = GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA12 - GREEN LED */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB9 - DHT11 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
