/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 Lars Boegild Thomsen <lbthomsen@gmail.com>
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
#include <string.h>
#include "w25qxx.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PAGE_SIZE 4096
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

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
        DBG("Programmer is a moron");
    }
}

uint8_t check_buffer(uint8_t pattern, uint8_t *buf, uint32_t len) {

    uint8_t ret = 1;

    switch (pattern) {
    case 0:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != 0)
                ret = 0;
        }
        break;
    case 1:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != 0xaa)
                ret = 0;
        }
        break;
    case 2:
        for (uint32_t i = 0; i < len; ++i) {
            if (buf[i] != i % 256)
                ret = 0;
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

    DBG("\n\n\n--------\nCore and peripherals has been initialized\n");

    HAL_Delay(10); // Wait a bit to make sure the w25qxx is ready

    W25QXX_result_t res;

    res = w25qxx_init(&w25qxx, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin);

    if (res == W25QXX_Ok) {
        DBG("W25QXX successfully initialized\n");
        DBG("Manufacturer       = 0x%2x\n", w25qxx.manufacturer_id);
        DBG("Device             = 0x%4x\n", w25qxx.device_id);
        DBG("Block size         = 0x%04lx (%lu)\n", w25qxx.block_size, w25qxx.block_size);
        DBG("Block count        = 0x%04lx (%lu)\n", w25qxx.block_count, w25qxx.block_count);
        DBG("Sector size        = 0x%04lx (%lu)\n", w25qxx.sector_size, w25qxx.sector_size);
        DBG("Sectors per block  = 0x%04lx (%lu)\n", w25qxx.sectors_in_block, w25qxx.sectors_in_block);
        DBG("Page size          = 0x%04lx (%lu)\n", w25qxx.page_size, w25qxx.page_size);
        DBG("Pages per sector   = 0x%04lx (%lu)\n", w25qxx.pages_in_sector, w25qxx.pages_in_sector);
        DBG("Total size (in kB) = 0x%04lx (%lu)\n", (w25qxx.block_count * w25qxx.block_size) / 1024, (w25qxx.block_count * w25qxx.block_size) / 1024);
    } else {
        DBG("Unable to initialize w25qxx\n");
    }

    HAL_Delay(2000);

    uint8_t buf[PAGE_SIZE]; // Buffer the size of a page

    for (uint8_t run = 0; run <= 2; ++run) {

        DBG("\n-------------\nRun %d\n", run);

        DBG("Reading first page");

        res = w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf));
        if (res == W25QXX_Ok) {
            dump_hex("First page at start", 0, (uint8_t*) &buf, sizeof(buf));
        } else {
            DBG("Unable to read w25qxx\n");
        }

        DBG("Erasing first page");
        if (w25qxx_erase(&w25qxx, 0, sizeof(buf)) == W25QXX_Ok) {
            DBG("Reading first page\n");
            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
                dump_hex("After erase", 0, (uint8_t*) &buf, sizeof(buf));
            }
        }

        // Create a well known pattern
        fill_buffer(run, buf, sizeof(buf));

        // Write it to device
        DBG("Writing first page\n");
        if (w25qxx_write(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
            // now read it back
            DBG("Reading first page\n");
            if (w25qxx_read(&w25qxx, 0, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
                //DBG("  - sum = %lu", get_sum(buf, 256));
                dump_hex("After write", 0, (uint8_t*) &buf, sizeof(buf));
            }
        }
    }

    // Let's do a stress test
    uint32_t start;
    uint32_t sectors = w25qxx.block_count * w25qxx.sectors_in_block; // Entire chip

    DBG("Stress testing w25qxx device: sectors = %lu\n", sectors);

    DBG("Doing chip erase\n");
    start = HAL_GetTick();
    w25qxx_chip_erase(&w25qxx);
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

    fill_buffer(0, buf, sizeof(buf));

    DBG("Writing all zeroes %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done writing - took %lu ms\n", HAL_GetTick() - start);

    DBG("Reading %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done reading - took %lu ms\n", HAL_GetTick() - start);

    DBG("Validating buffer .... ");
    if (check_buffer(0, buf, sizeof(buf))) {
        DBG("OK\n");
    } else {
        DBG("Not OK\n");
    }

    DBG("Doing chip erase\n");
    start = HAL_GetTick();
    w25qxx_chip_erase(&w25qxx);
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

    fill_buffer(1, buf, sizeof(buf));

    DBG("Writing 10101010 %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_write(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done writing - took %lu ms\n", HAL_GetTick() - start);

    DBG("Reading %lu sectors\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_read(&w25qxx, i * w25qxx.sector_size, buf, sizeof(buf));
    }
    DBG("Done reading - took %lu ms\n", HAL_GetTick() - start);

    DBG("Validating buffer ... ");
    if (check_buffer(1, buf, sizeof(buf))) {
        DBG("OK\n");
    } else {
        DBG("Not OK\n");
    }

    DBG("Erasing %lu sectors sequentially\n", sectors);
    start = HAL_GetTick();
    for (uint32_t i = 0; i < sectors; ++i) {
        w25qxx_erase(&w25qxx, i * w25qxx.sector_size, sizeof(buf));
        if ((i > 0) && (i % 100 == 0)) {
            DBG("Done %4lu sectors - total time = %3lu s\n", i, (HAL_GetTick() - start) / 1000);
        }
    }
    DBG("Done erasing - took %lu ms\n", HAL_GetTick() - start);

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

        if (now - last_test >= 10000) {

            DBG("---------------\nReading page at address     : 0x%08lx\n", offset_address);

            res = w25qxx_read(&w25qxx, offset_address, (uint8_t*) &buf, sizeof(buf));
            if (res == W25QXX_Ok) {
                //dump_hex("First page at start", offset_address, (uint8_t*) &buf, sizeof(buf));
                DBG("Reading old value           : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t* )&buf, sizeof(buf) / 4));
            } else {
                DBG("Unable to read w25qxx\n");
            }

            // DBG("Erasing page");
            if (w25qxx_erase(&w25qxx, offset_address, sizeof(buf)) == W25QXX_Ok) {
                if (w25qxx_read(&w25qxx, offset_address, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
                    DBG("After erase                 : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t* )&buf, sizeof(buf) / 4));
                }
            }

            // Create a well known pattern
            fill_buffer(2, buf, sizeof(buf));

            // Write it to device
            DBG("Writing page value          : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t* )&buf, sizeof(buf) / 4));
            if (w25qxx_write(&w25qxx, offset_address, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
                // now read it back
                //DBG("Reading page");
                if (w25qxx_read(&w25qxx, offset_address, (uint8_t*) &buf, sizeof(buf)) == W25QXX_Ok) {
                    DBG("Reading back                : 0x%08lx\n", HAL_CRC_Calculate(&hcrc, (uint32_t* )&buf, sizeof(buf) / 4));
                }
            }

            DBG("Test time                   : %lu ms\n", HAL_GetTick() - now);

            offset_address += PAGE_SIZE / 4;

            if (offset_address + PAGE_SIZE > w25qxx.block_count * w25qxx.block_size)
                offset_address = 0;

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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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

  LL_SPI_InitTypeDef SPI_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**SPI1 GPIO Configuration
  PA5   ------> SPI1_SCK
  PA6   ------> SPI1_MISO
  PA7   ------> SPI1_MOSI
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5|LL_GPIO_PIN_6|LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2;
  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
  SPI_InitStruct.CRCPoly = 10;
  LL_SPI_Init(SPI1, &SPI_InitStruct);
  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
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
  huart1.Init.BaudRate = 2000000;
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
  __HAL_RCC_GPIOB_CLK_ENABLE();

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
    while (1)
    {
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
