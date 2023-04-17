/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2022 Lars Boegild Thomsen <lbthomsen@gmail.com>
 * All rights reserved.</center></h2>
 *
 * This software component is licensed under MIT license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/MIT
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
#include <string.h>
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
CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

W25QXX_HandleTypeDef w25qxx; // Handler for all w25qxx operations!

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_CRC_Init(void);
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

// Dump hex to serial console
void dump_hex(char *header, uint32_t start, uint8_t *buf, uint32_t len) {
    uint32_t i = 0;

    printf("%s\n", header);

    for (i = 0; i < len; ++i) {

        if (i % 16 == 0) {
            printf("0x%08lx: ", start);
        }

        printf("%02x ", buf[i]);

        if ((i + 1) % 16 == 0) {
            printf("\n");
        }

        ++start;
    }
}

void fill_buffer(uint8_t pattern, uint8_t *buf, uint32_t len) {
    switch (pattern) {
    case 0:
        memset(buf, 0, len);
        break;
    case 1:
        memset(buf, 0xaa, len); // 10101010
        break;
    case 2:
        for (uint32_t i = 0; i < len; ++i)
            buf[i] = i % 256;
        break;
    default:
        DBG("Programmer is a moron")
        ;
    }
}

bool check_buffer(uint8_t pattern, uint8_t *buf, uint32_t len) {

    bool ret = true;

    switch (pattern) {
    case 0:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != 0)
                ret = false;
        }
        break;
    case 1:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != 0xaa)
                ret = false;
        }
        break;
    case 2:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != i % 256)
                ret = false;
        }
        break;
    default:
        DBG("Programmer is a moron");
    }

    return ret;
}

uint32_t get_sum(uint8_t *buf, uint32_t len) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < len; ++i) {
        sum += buf[i];
    }
    return sum;
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
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

    DBG("\n\n\n--------\nCore and peripherals has been initialised");

    W25QXX_result_t res;

    res = w25qxx_init(&w25qxx, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin);
    if (res == W25QXX_Ok) {
        DBG("W25QXX successfully initialized");
        DBG("Manufacturer       = 0x%2x", w25qxx.manufacturer_id);
        DBG("Device             = 0x%4x", w25qxx.device_id);
        DBG("Block size         = 0x%04lx (%lu)", w25qxx.block_size, w25qxx.block_size);
        DBG("Block count        = 0x%04lx (%lu)", w25qxx.block_count, w25qxx.block_count);
        DBG("Sector size        = 0x%04lx (%lu)", w25qxx.sector_size, w25qxx.sector_size);
        DBG("Sectors per block  = 0x%04lx (%lu)", w25qxx.sectors_in_block, w25qxx.sectors_in_block);
        DBG("Page size          = 0x%04lx (%lu)", w25qxx.page_size, w25qxx.page_size);
        DBG("Pages per sector   = 0x%04lx (%lu)", w25qxx.pages_in_sector, w25qxx.pages_in_sector);
        DBG("Total size (in kB) = 0x%04lx (%lu)", (w25qxx.block_count * w25qxx.block_size) / 1024, (w25qxx.block_count * w25qxx.block_size) / 1024);
    } else {
        DBG("Unable to initialize w25qxx");
    }

    HAL_Delay(10);

    uint8_t buf[w25qxx.page_size]; // Buffer the size of a page

//    for (uint8_t run = 0; run <= 2; ++run) {
//
//        DBG("\n-------------\nRun %d", run);
//
//        DBG("Reading first page");
//
//        res = w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf));
//        if (res == W25QXX_Ok) {
//            dump_hex("First page at start", 0, (uint8_t*) &buf, sizeof(buf));
//        } else {
//            DBG("Unable to read w25qxx");
//        }
//
//        DBG("Erasing first page");
//        if (w25qxx_erase(&w25qxx, 0, sizeof(buf)) == W25QXX_Ok) {
//            DBG("Reading first page");
//            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
//                dump_hex("After erase", 0, (uint8_t*) &buf, sizeof(buf));
//            }
//        }
//
//        // Create a well known pattern
//        fill_buffer(run, buf, sizeof(buf));
//
//        // Write it to device
//        DBG("Writing first page");
//        if (w25qxx_write(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
//            // now read it back
//            DBG("Reading first page");
//            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
//                //DBG("  - sum = %lu", get_sum(buf, 256));
//                dump_hex("After write", 0, (uint8_t*) &buf, sizeof(buf));
//            }
//        }
//    }
//
//    // Let's do a stress test
//    uint32_t start;
//    uint32_t sectors = w25qxx.block_count * w25qxx.sectors_in_block; // Entire chip
//
//    DBG("Stress testing w25qxx device: sectors = %lu", sectors);
//
//    DBG("Doing chip erase");
//    start = HAL_GetTick();
//    w25qxx_chip_erase(&w25qxx);
//    DBG("Done erasing - took %lu ms", HAL_GetTick() - start);
//
//    fill_buffer(0, buf, sizeof(buf));
//
//    DBG("Writing all zeroes %lu sectors", sectors);
//    start = HAL_GetTick();
//    for (uint32_t i = 0; i < sectors; ++i) {
//        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
//    }
//    DBG("Done writing - took %lu ms", HAL_GetTick() - start);
//
//    DBG("Reading %lu sectors", sectors);
//    start = HAL_GetTick();
//    for (uint32_t i = 0; i < sectors; ++i) {
//        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
//    }
//    DBG("Done reading - took %lu ms", HAL_GetTick() - start);
//
//    DBG("Validating buffer");
//    if (check_buffer(0, buf, sizeof(buf))) {
//        DBG("OK");
//    } else {
//        DBG("Not OK");
//    }
//
//    DBG("Doing chip erase");
//    start = HAL_GetTick();
//    w25qxx_chip_erase(&w25qxx);
//    DBG("Done erasing - took %lu ms", HAL_GetTick() - start);
//
//    fill_buffer(1, buf, sizeof(buf));
//
//    DBG("Writing 10101010 %lu sectors", sectors);
//    start = HAL_GetTick();
//    for (uint32_t i = 0; i < sectors; ++i) {
//        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
//    }
//    DBG("Done writing - took %lu ms", HAL_GetTick() - start);
//
//    DBG("Reading %lu sectors", sectors);
//    start = HAL_GetTick();
//    for (uint32_t i = 0; i < sectors; ++i) {
//        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
//    }
//    DBG("Done reading - took %lu ms", HAL_GetTick() - start);
//
//    DBG("Validating buffer");
//    if (check_buffer(1, buf, sizeof(buf))) {
//        DBG("OK");
//    } else {
//        DBG("Not OK");
//    }
//
//    DBG("Erasing %lu sectors sequentially", sectors);
//    start = HAL_GetTick();
//    for (uint32_t i = 0; i < sectors; ++i) {
//        w25qxx_erase(&w25qxx, i * w25qxx.sector_size, sizeof(buf));
//    }
//    DBG("Done erasing - took %lu ms", HAL_GetTick() - start);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    uint32_t now = 0, last_blink = 0, last_test = 0, offset_address = 0;

    while (1) {

        now = HAL_GetTick();

        if (now - last_blink >= 500) {

            HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);

            last_blink = now;
        }

        if (now - last_test >= 5000) {

            DBG("Reading page at 0x%08lx", offset_address);

            res = w25qxx_read(&w25qxx, offset_address, (uint8_t*) &buf, sizeof(buf));
            if (res == W25QXX_Ok) {
                //dump_hex("First page at start", offset_address, (uint8_t*) &buf, sizeof(buf));
                DBG("Value before: 0x%08lx", HAL_CRC_Calculate(&hcrc, &buf, sizeof(buf)));
            } else {
                DBG("Unable to read w25qxx");
            }

            //        DBG("Erasing first page");
            //        if (w25qxx_erase(&w25qxx, 0, sizeof(buf)) == W25QXX_Ok) {
            //            DBG("Reading first page");
            //            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
            //                dump_hex("After erase", 0, (uint8_t*) &buf, sizeof(buf));
            //            }
            //        }
            //
            //        // Create a well known pattern
            //        fill_buffer(run, buf, sizeof(buf));
            //
            //        // Write it to device
            //        DBG("Writing first page");
            //        if (w25qxx_write(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
            //            // now read it back
            //            DBG("Reading first page");
            //            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
            //                //DBG("  - sum = %lu", get_sum(buf, 256));
            //                dump_hex("After write", 0, (uint8_t*) &buf, sizeof(buf));
            //            }
            //        }

            offset_address += 0x20;

            last_test = now;
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

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
