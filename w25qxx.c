/*
 * w25qxx.c
 *
 *  Created on: Jun 2, 2021
 *      Author: lth
 */

#include "main.h"
#include "w25qxx.h"

static inline void cs_on(W25QXX_HandleTypeDef *w25qxx) {
	HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_RESET);
}

static inline void cs_off(W25QXX_HandleTypeDef *w25qxx) {
	HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_SET);
}

uint8_t W25QXX_SPI(W25QXX_HandleTypeDef *w25qxx, uint8_t data) {
	uint8_t	ret;
	if (HAL_SPI_TransmitReceive(w25qxx->spiHandle, &data, &ret, 1, 100) != HAL_OK) {
		Error_Handler();
	}
	return ret;
}

uint32_t W25QXX_read_id(W25QXX_HandleTypeDef *w25qxx)
{
  uint32_t d0 = 0, d1 = 0, d2 = 0;
  cs_on(w25qxx);
  W25QXX_SPI(w25qxx, W25QXX_GET_ID);
  d0 = W25QXX_SPI(w25qxx, W25QXX_DUMMY_BYTE);
  d1 = W25QXX_SPI(w25qxx, W25QXX_DUMMY_BYTE);
  d2 = W25QXX_SPI(w25qxx, W25QXX_DUMMY_BYTE);
  cs_off(w25qxx);
  return (d0 << 16) | (d1 << 8) | (d2);
}

W25QXX_result_t w25qxx_init(W25QXX_HandleTypeDef *w25qxx, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {

	W25QXX_result_t result = W25QXX_Err;

	DBG("w25qxx_init");

	w25qxx->spiHandle = hspi;
	w25qxx->cs_port = cs_port;
	w25qxx->cs_pin = cs_pin;

	W25QXX_read_id(w25qxx);

	return W25QXX_Ok;

}
