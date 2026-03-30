/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// 1602 I2C address
// 1602 I2C address
#define I2C_ADDR 0x27 // I2C address of the PCF8574
// 1602 dimensions
#define LCD_ROWS 2 // Number of rows on the LCD
#define LCD_COLS 16 // Number of columns on the LCD
// 1602 message bit numbers
#define DC_BIT 0 // Data/Command bit (register select bit)
#define EN_BIT 2 // Enable bit
#define BL_BIT 3 // Back light bit
#define D4_BIT 4 // Data 4 bit
#define D5_BIT 5 // Data 5 bit
#define D6_BIT 6 // Data 6 bit
#define D7_BIT 7 // Data 7 bit
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
uint32_t rawADC = 0;
uint32_t percentage = 0;
char rawADCString[20];
char lcdBuffer[20];

volatile uint8_t buttonPressed = 0;

typedef enum {
    DISPLAY_WATER = 0,
    DISPLAY_TEMP  = 1
} DisplayMode_t;


volatile DisplayMode_t displayMode = DISPLAY_WATER;

uint32_t lastDHTReadTick = 0;
uint8_t dhtValid = 0;

uint8_t Rh = 0;
uint8_t Temp = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void CharLCD_Write_Nibble (uint8_t nibble, uint8_t dc);
void CharLCD_Send_Cmd(uint8_t cmd);
void CharLCD_Send_Data(uint8_t data);
void CharLCD_Init();
void CharLCD_Write_String(char *str);
void CharLCD_Set_Cursor(uint8_t row, uint8_t column);
void CharLCD_Clear(void);
uint8_t DHT11_Start(void);
uint8_t DHT11_Read(void);
void delay_us(uint16_t us);

void DHT11_Update(void);
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
  MX_ADC1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // Test display
    // Start the microsecond timer for DHT11 sensor timing
    HAL_TIM_Base_Start(&htim2);

    // Start the ADC for the water level sensor
    HAL_ADC_Start(&hadc1);

    // 2. INITIALIZE DISPLAY
    CharLCD_Init();
    CharLCD_Set_Cursor(0,0);
    CharLCD_Write_String("ECEN_260");
    CharLCD_Set_Cursor(1,0);
    CharLCD_Write_String("FINAL_PROJ");

    // Show splash screen for 3 seconds
    HAL_Delay(3000);

    // Clear the screen so it's ready for the live data loop
    CharLCD_Clear();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  // Handle button press: toggle display mode
	  if (buttonPressed)
	  {
		  buttonPressed = 0;

		  if (displayMode == DISPLAY_WATER)
			  displayMode = DISPLAY_TEMP;
		  else
			  displayMode = DISPLAY_WATER;

		  CharLCD_Clear();
	  }

	  // --- READ WATER SENSOR OFTEN ---
	  HAL_ADC_Start(&hadc1);
	  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
	  {
		  rawADC = HAL_ADC_GetValue(&hadc1);
		  percentage = (rawADC * 100) / 4095;   // 12-bit ADC should use 4095
		  if (percentage > 100) percentage = 100;
	  }
	  HAL_ADC_Stop(&hadc1);

	  // --- UPDATE DHT11 PERIODICALLY ---
	  // DHT11 shouldn't be read too fast; about once every 1-2 seconds is safe
	  if (HAL_GetTick() - lastDHTReadTick >= 2000)
	  {
		  lastDHTReadTick = HAL_GetTick();
		  DHT11_Update();
	  }

	  // --- DISPLAY ONLY ONE SENSOR AT A TIME ---
	  switch (displayMode)
	  {
		  case DISPLAY_WATER:
			  CharLCD_Set_Cursor(0,0);
			  CharLCD_Write_String("Water Sensor    ");

			  snprintf(lcdBuffer, sizeof(lcdBuffer), "Level: %3lu%%    ", percentage);
			  CharLCD_Set_Cursor(1,0);
			  CharLCD_Write_String(lcdBuffer);
			  break;

		  case DISPLAY_TEMP:
			  CharLCD_Set_Cursor(0,0);
			  CharLCD_Write_String("Temp/Humidity   ");

			  if (dhtValid)
			  {
				  snprintf(lcdBuffer, sizeof(lcdBuffer), "T:%02dC H:%02d%%   ", Temp, Rh);
			  }
			  else
			  {
				  snprintf(lcdBuffer, sizeof(lcdBuffer), "Sensor Error!   ");
			  }

			  CharLCD_Set_Cursor(1,0);
			  CharLCD_Write_String(lcdBuffer);
			  break;
	  }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      HAL_Delay(200);
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : TEMP_SENSOR_BUTTON_Pin */
  GPIO_InitStruct.Pin = TEMP_SENSOR_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TEMP_SENSOR_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TEMP_SENSOR_Pin */
  GPIO_InitStruct.Pin = TEMP_SENSOR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TEMP_SENSOR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 * @brief Write a 4-bit nibble to the LCD via I2C
 * @param nibble: 4-bit data to send (lower 4 bits)
 * @param dc: data/command (1 = data, 0 = command)
 * @retval None
 */
void CharLCD_Write_Nibble(uint8_t nibble, uint8_t dc) {
 uint8_t data = nibble << D4_BIT; // Shift nibble to D4-D7 position
 data |= dc << DC_BIT; // Set DC bit for data/command selection
 data |= 1 << BL_BIT; // Include backlight state in data
 data |= 1 << EN_BIT; // Set enable bit high
 HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, 100); // Send data with EN high
 HAL_Delay(1); // Wait for data setup
 data &= ~(1 << EN_BIT); // Clear enable bit (falling edge triggers LCD)
 HAL_I2C_Master_Transmit(&hi2c1, I2C_ADDR << 1, &data, 1, 100); // Send data with EN low
}

/**
 * @brief Send command to LCD
 * @param cmd: 8-bit command to send to LCD controller
 * @retval None
 */
void CharLCD_Send_Cmd(uint8_t cmd) {
 uint8_t upper_nibble = cmd >> 4; // Extract upper 4 bits
 uint8_t lower_nibble = cmd & 0x0F; // Extract lower 4 bits
 CharLCD_Write_Nibble(upper_nibble, 0); // Send upper nibble (DC=0 for command)
 CharLCD_Write_Nibble(lower_nibble, 0); // Send lower nibble (DC=0 for command)
 if (cmd == 0x01 || cmd == 0x02) { // Clear display or return home commands
 HAL_Delay(2); // These commands need extra time
 }
}

/**
 * @brief Send data (character) to LCD
 * @param data: 8-bit character data to display
 * @retval None
 */
void CharLCD_Send_Data(uint8_t data) {
 uint8_t upper_nibble = data >> 4; // Extract upper 4 bits
 uint8_t lower_nibble = data & 0x0F; // Extract lower 4 bits
 CharLCD_Write_Nibble(upper_nibble, 1); // Send upper nibble (DC=1 for data)
 CharLCD_Write_Nibble(lower_nibble, 1); // Send lower nibble (DC=1 for data)
}
/**
 * @brief Initialize LCD in 4-bit mode via I2C
 * @param None
 * @retval None
 */
void CharLCD_Init() {
 HAL_Delay(50); // Wait for LCD power-on reset (>40ms)
 CharLCD_Write_Nibble(0x03, 0); // Function set: 8-bit mode (first attempt)
 HAL_Delay(5); // Wait >4.1ms
 CharLCD_Write_Nibble(0x03, 0); // Function set: 8-bit mode (second attempt)
 HAL_Delay(1); // Wait >100us
 CharLCD_Write_Nibble(0x03, 0); // Function set: 8-bit mode (third attempt)
 HAL_Delay(1); // Wait >100us
 CharLCD_Write_Nibble(0x02, 0); // Function set: switch to 4-bit mode
 CharLCD_Send_Cmd(0x28); // Function set: 4-bit, 2 lines, 5x8 font
 CharLCD_Send_Cmd(0x0C); // Display control: display on/cursor off/blink off
 CharLCD_Send_Cmd(0x06); // Entry mode: increment cursor, no shift
 CharLCD_Send_Cmd(0x01); // Clear display
 HAL_Delay(2); // Wait for clear display command
}
/**
 * @brief Write string to LCD at current cursor position
 * @param str: Pointer to null-terminated string
 * @retval None
 */
void CharLCD_Write_String(char *str) {
 while (*str) { // Loop until null terminator
 CharLCD_Send_Data(*str++); // Send each character and increment pointer
 }
}
/**
 * @brief Set cursor position on LCD
 * @param row: Row number (0 or 1 for 2-line display)
 * @param column: Column number (0 to display width - 1)
 * @retval None
 */
void CharLCD_Set_Cursor(uint8_t row, uint8_t column) {
 uint8_t address;
 switch (row) {
 case 0:
 address = 0x00; break; // First line starts at address 0x00
 case 1:
 address = 0x40; break; // Second line starts at address 0x40
 default:
 address = 0x00; // Default to first line for invalid row
 }
 address += column; // Add column offset
 CharLCD_Send_Cmd(0x80 | address); // Set DDRAM address command (0x80 + address)
}
/**
 * @brief Clear LCD display and return cursor to home position
 * @param None
 * @retval None
 */
void CharLCD_Clear(void) {
 CharLCD_Send_Cmd(0x01); // Clear display command
 HAL_Delay(2); // Wait for command execution
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == TEMP_SENSOR_BUTTON_Pin) // Match the label in CubeMX
    {
    	buttonPressed = 1; // for button/display state machine code
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); // Uses the standard LD2 label
    }
}

void delay_us(uint16_t us) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us);
}

uint8_t DHT11_Start(void) {
    uint8_t Response = 0;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = TEMP_SENSOR_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(TEMP_SENSOR_GPIO_Port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin, 0);
    HAL_Delay(18);
    HAL_GPIO_WritePin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin, 1);
    delay_us(20);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(TEMP_SENSOR_GPIO_Port, &GPIO_InitStruct);
    delay_us(40);
    if (!(HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin))) {
        delay_us(80);
        if ((HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin))) Response = 1;
    }
    while ((HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin)));
    return Response;
}

uint8_t DHT11_Read(void) {
    uint8_t i = 0, j;
    for (j = 0; j < 8; j++) {
        while (!(HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin)));
        delay_us(40);
        if (!(HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin))) i &= ~(1 << (7 - j));
        else i |= (1 << (7 - j));
        while ((HAL_GPIO_ReadPin(TEMP_SENSOR_GPIO_Port, TEMP_SENSOR_Pin)));
    }
    return i;
}

void DHT11_Update(void)
{
    if (DHT11_Start())
    {
        uint8_t rh_byte1   = DHT11_Read();
        uint8_t rh_byte2   = DHT11_Read();
        uint8_t temp_byte1 = DHT11_Read();
        uint8_t temp_byte2 = DHT11_Read();
        uint8_t checksum   = DHT11_Read();

        if (checksum == (uint8_t)(rh_byte1 + rh_byte2 + temp_byte1 + temp_byte2))
        {
            Rh = rh_byte1;
            Temp = temp_byte1;
            dhtValid = 1;
        }
        else
        {
            dhtValid = 0;
        }
    }
    else
    {
        dhtValid = 0;
    }
}
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
