/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
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
#include "w25qxx.h"
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
QSPI_HandleTypeDef hqspi;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Send printf to uart1
int _write(int fd, char *ptr, int len) {
    HAL_StatusTypeDef hstatus;

    if (fd == 1 || fd == 2) {
        hstatus = HAL_UART_Transmit(&huart1, (uint8_t*) ptr, len, HAL_MAX_DELAY);
        if (hstatus == HAL_OK)
            return len;
        else
            return -1;
    }
    return -1;
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
  MX_QUADSPI_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

    printf("\n\n-----------\nStarting...\n");

    QSPI_CommandTypeDef sCommand;
    static uint8_t id[17] = { 0 };
    static uint8_t tx_buf[0x10] = "Ext Flash";
    static uint8_t rx_buf[0x10] = { 0 };

    sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
    sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /***** Read ID operation*****/
    sCommand.Instruction = 0x9F; //READ ID command code
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_NONE; //Address line width. No address phase
    sCommand.DataMode = QSPI_DATA_1_LINE; //Data line width
    sCommand.NbData = 17; //Read the data length. ID length is 17 bytes
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 0; //No Dummy phase
    //Configuration command (when there is data stage, the command will be sent in the subsequent sending/receiving API call)
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }
    //Execute QSPI reception
    if (HAL_QSPI_Receive(&hqspi, id, 5000) != HAL_OK) {
        Error_Handler();
    }

    /***** Write enable operation (need to make the external memory in the write enable state before block erasing) *****/
    sCommand.Instruction = 0x06; //Write enable command code
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_NONE; //Address line width. No address phase
    sCommand.DataMode = QSPI_DATA_NONE; //Data line width. No data stage
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 0; //No Dummy phase
    //Configure sending command
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }

    /***** Block erase operation*****/
    sCommand.Instruction = 0xD8; //Sector erase command code
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE; //Address line width. No address phase
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS; //Address length
    sCommand.Address = 0; //Any address in the sector to be erased.
    sCommand.DataMode = QSPI_DATA_NONE; //Data line width. No data stage
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 0; //No Dummy phase
    //Configure sending command
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }
    HAL_Delay(3000); //Delay 3s. The unit is SysTick timer interrupt period

    /***** Write enable operation (need to make the external memory in the write enable state before block erasing) *****/
    sCommand.Instruction = 0x06; //Write enable command code
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_NONE; //Address line width. No address phase
    sCommand.DataMode = QSPI_DATA_NONE; //Data line width. No data stage
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 0; //No Dummy phase
    //Configure sending command
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }

    /***** Four-wire fast write operation*****/
    sCommand.Instruction = 0x32; //Quick write command code with four lines
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE; //Address line width
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS; //Address length
    sCommand.Address = 0; //Write the starting address
    sCommand.DataMode = QSPI_DATA_4_LINES; //Data line width
    sCommand.NbData = 10; //write data length
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 0; //No Dummy phase
    //Configuration command (when there is data stage, the command will be sent in the subsequent sending/receiving API call)
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }
    //Execute QSPI reception
    if (HAL_QSPI_Transmit(&hqspi, tx_buf, 5000) != HAL_OK) {
        Error_Handler();
    }
    HAL_Delay(5); //Delay 5ms. The unit is SysTick timer interrupt period

    /***** Four-wire fast read operation*****/
    sCommand.Instruction = 0x6B; //Quick read command code with four lines
    sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE; //Command line width
    sCommand.AddressMode = QSPI_ADDRESS_1_LINE; //Address line width
    sCommand.AddressSize = QSPI_ADDRESS_24_BITS; //Address length
    sCommand.Address = 0; //Start address
    sCommand.DataMode = QSPI_DATA_4_LINES; //Data line width
    sCommand.NbData = 10; //Read data length
    sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE; //No multiplexing byte stage
    sCommand.DummyCycles = 8; //Dummy phase. N25Q128A13EF840F

    //Configuration command (when there is data stage, the command will be sent in the subsequent sending/receiving API call)
    if (HAL_QSPI_Command(&hqspi, &sCommand, 5000) != HAL_OK) {
        Error_Handler();
    }
    //Execute QSPI reception
    if (HAL_QSPI_Receive(&hqspi, rx_buf, 5000) != HAL_OK) {
        Error_Handler();
    }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint32_t now = 0, last_tick = 0;

    while (1) {

        now = HAL_GetTick();

        if (now - last_tick >= 1000) {
            printf("Tick %lu\n", now / 1000);
            last_tick = now;
        }

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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
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
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 10;
  hqspi.Init.FifoThreshold = 1;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
  hqspi.Init.FlashSize = 24;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

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
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED1_Pin|LED2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : B4_Pin B3_Pin */
  GPIO_InitStruct.Pin = B4_Pin|B3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : B2_Pin B1_Pin */
  GPIO_InitStruct.Pin = B2_Pin|B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
    while (1) {
    }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
