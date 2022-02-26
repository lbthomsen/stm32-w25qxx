/*
 * w25qxx.h
 *
 *  Created on: Jun 2, 2021
 *      Author: lth
 */

#ifndef W25QXX_H_
#define W25QXX_H_

#ifndef DBG
#define DBG(...) printf(__VA_ARGS__);\
                 printf("\n");
#else
#define DBG(...)
#endif

#define W25QXX_DUMMY_BYTE         0xA5
#define W25QXX_GET_ID             0x9F

typedef struct {
	SPI_HandleTypeDef *spiHandle;
	GPIO_TypeDef *cs_port;
	uint16_t cs_pin;
} W25QXX_HandleTypeDef;

typedef enum {
        W25QXX_Err,
        W25QXX_Ok
} W25QXX_result_t;

W25QXX_result_t w25qxx_init(W25QXX_HandleTypeDef *w25qxx, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

#endif /* W25QXX_H_ */
