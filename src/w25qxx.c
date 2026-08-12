/**
 ******************************************************************************
 * @file           : w25qxx.c
 * @brief          : Minimal W25Qxx SPI Library Source
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 - 2026 Lars Boegild Thomsen <lbthomsen@gmail.com>
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include "w25qxx.h"
#include "main.h"
#include <string.h>

#ifdef DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

/*
 * Internal functions
 */

/**
 * @brief  Enables CS (driving it low) of the W25Qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 */
static inline void cs_on(W25QXX_HandleTypeDef *w25qxx) {
    HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_RESET);
}

/**
 * @brief  Disables CS (driving it high) of the W25Qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 */
static inline void cs_off(W25QXX_HandleTypeDef *w25qxx) {
    HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_SET);
}

/**
 * @brief  Transmit data to w25qxx - ignore returned data
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  buf: pointer to the buffer containing data to transmit
 * @param  len: number of bytes to transmit
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_transmit(W25QXX_HandleTypeDef *w25qxx, uint8_t *buf, uint32_t len) {
    if (HAL_SPI_Transmit(w25qxx->spiHandle, buf, len, 1000) == HAL_OK) {
        return W25QXX_Ok;
    }
    return W25QXX_Err;
}

/**
 * @brief  Receive data from w25qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  buf: pointer to the buffer to store received data
 * @param  len: number of bytes to receive
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_receive(W25QXX_HandleTypeDef *w25qxx, uint8_t *buf, uint32_t len) {
    if (HAL_SPI_Receive(w25qxx->spiHandle, buf, len, 1000) == HAL_OK) {
        return W25QXX_Ok;
    }
    return W25QXX_Err;
}

/**
 * @brief  Transmit and receive data from w25qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @retval ID of the W25Qxx device
 */
uint32_t w25qxx_read_id(W25QXX_HandleTypeDef *w25qxx) {
    uint32_t ret = 0;
    uint8_t buf[3];
    uint8_t cmd = W25QXX_GET_ID;

    cs_on(w25qxx);
    if (w25qxx_transmit(w25qxx, &cmd, 1) == W25QXX_Ok) {
        if (w25qxx_receive(w25qxx, buf, 3) == W25QXX_Ok) {
            ret = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
        }
    }
    cs_off(w25qxx);
    return ret;
}

/**
 * @brief  Get the status register of the W25Qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @retval Status register value
 */
uint8_t w25qxx_get_status(W25QXX_HandleTypeDef *w25qxx) {
    uint8_t ret = 0;
    uint8_t buf = W25QXX_READ_REGISTER_1;
    cs_on(w25qxx);
    if (w25qxx_transmit(w25qxx, &buf, 1) == W25QXX_Ok) {
        if (w25qxx_receive(w25qxx, &buf, 1) == W25QXX_Ok) {
            ret = buf;
        }
    }
    cs_off(w25qxx);
    return ret;
}

/**
 * @brief  Enable write operations on the W25Qxx
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_write_enable(W25QXX_HandleTypeDef *w25qxx) {
    W25_DBG("w25qxx_write_enable");
    W25QXX_result_t ret = W25QXX_Err;
    uint8_t buf = W25QXX_WRITE_ENABLE;
    cs_on(w25qxx);
    if (w25qxx_transmit(w25qxx, &buf, 1) == W25QXX_Ok) {
        ret = W25QXX_Ok;
    }
    cs_off(w25qxx);
    return ret;
}

/**
 * @brief  Initialize any generic JEDEC-compliant W25Qxx device
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  hspi: pointer to the SPI handle
 * @param  cs_port: GPIO port for chip select
 * @param  cs_pin: GPIO pin for chip select
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_init(W25QXX_HandleTypeDef *w25qxx, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    W25QXX_result_t result = W25QXX_Ok;
    W25_DBG("w25qxx_init: %s", W25QXX_VERSION);

    w25qxx->spiHandle = hspi;
    w25qxx->cs_port = cs_port;
    w25qxx->cs_pin = cs_pin;
    cs_off(w25qxx);

    uint32_t id = w25qxx_read_id(w25qxx);

    // Check for invalid ID response (0x000000 or 0xFFFFFF)
    if (id == 0 || id == 0xFFFFFF) {
        W25_DBG("Failed to communicate with Flash chip (ID: 0x%06lX)", id);
        memset(w25qxx, 0, sizeof(W25QXX_HandleTypeDef));
        return W25QXX_Err;
    }

    w25qxx->manufacturer_id = (uint8_t)(id >> 16);
    w25qxx->device_id = (uint16_t)(id & 0xFFFF);

    // Standard baseline memory organization
    w25qxx->block_size = 0x10000;    // 64 KB (65,536 Bytes)
    w25qxx->sector_size = 0x1000;    // 4 KB  (4,096 Bytes)
    w25qxx->sectors_in_block = 0x10; // 16 sectors per 64KB block
    w25qxx->page_size = 0x100;       // 256 Bytes
    w25qxx->pages_in_sector = 0x10;  // 16 pages per sector

    // Extract Capacity ID (low byte of device_id / 3rd byte of JEDEC ID)
    uint8_t capacity_id = (uint8_t)(w25qxx->device_id & 0xFF);

    // Validate standard Capacity ID range (0x10 = 512Kb up to 0x1A = 512Mb)
    if (capacity_id >= 0x10 && capacity_id <= 0x1B) {
        // Dynamic formula: 64KB Block Count = 2^(capacity_id - 16)
        w25qxx->block_count = (1 << (capacity_id - 0x10));

        W25_DBG("Flash Init OK! MFR: 0x%02X, Capacity ID: 0x%02X, Blocks: %lu", w25qxx->manufacturer_id, capacity_id, w25qxx->block_count);
    } else {
        W25_DBG("Unknown Flash Capacity ID: 0x%02X", capacity_id);
        result = W25QXX_Err;
    }

    if (result == W25QXX_Err) {
        memset(w25qxx, 0, sizeof(W25QXX_HandleTypeDef));
    }

    return result;
}

/**
 * @brief  Read data from the W25Qxx device
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  address: address to read from
 * @param  buf: buffer to store the read data
 * @param  len: number of bytes to read
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_read(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint8_t *buf, uint32_t len) {
    W25_DBG("w25qxx_read - address: 0x%08lx, length: 0x%04lx", address, len);

    if (w25qxx_wait_for_ready(w25qxx, 1000) != W25QXX_Ok) {
        return W25QXX_Err;
    }

    uint8_t tx[4] = {
        W25QXX_READ_DATA,
        (uint8_t)(address >> 16),
        (uint8_t)(address >> 8),
        (uint8_t)(address)
    };

    W25QXX_result_t ret = W25QXX_Err;
    cs_on(w25qxx);
    if (w25qxx_transmit(w25qxx, tx, 4) == W25QXX_Ok) {
        ret = w25qxx_receive(w25qxx, buf, len);
    }
    cs_off(w25qxx);

    return ret;
}

/**
 * @brief  Write data to the W25Qxx device
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  address: address to write to
 * @param  buf: buffer containing the data to write
 * @param  len: number of bytes to write
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_write(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint8_t *buf, uint32_t len) {
    W25_DBG("w25qxx_write - address 0x%08lx len 0x%04lx", address, len);

    uint32_t buffer_offset = 0;

    while (len > 0) {
        uint32_t page_offset = address % w25qxx->page_size;
        uint32_t write_len = w25qxx->page_size - page_offset;
        if (len < write_len) {
            write_len = len;
        }

        if (w25qxx_wait_for_ready(w25qxx, 1000) != W25QXX_Ok) {
            return W25QXX_Err;
        }

        if (w25qxx_write_enable(w25qxx) != W25QXX_Ok) {
            return W25QXX_Err;
        }

        uint8_t tx[4] = {
            W25QXX_PAGE_PROGRAM,
            (uint8_t)(address >> 16),
            (uint8_t)(address >> 8),
            (uint8_t)(address)
        };

        cs_on(w25qxx);
        if (w25qxx_transmit(w25qxx, tx, 4) == W25QXX_Ok) {
            if (w25qxx_transmit(w25qxx, buf + buffer_offset, write_len) != W25QXX_Ok) {
                cs_off(w25qxx);
                return W25QXX_Err;
            }
        }
        cs_off(w25qxx);

        address += write_len;
        buffer_offset += write_len;
        len -= write_len;
    }

    return W25QXX_Ok;
}

/**
 * @brief  Erase a sector of the W25Qxx device
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  address: address of the sector to erase
 * @param  len: number of bytes to erase (must be a multiple of sector size)
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_erase(W25QXX_HandleTypeDef *w25qxx, uint32_t address, uint32_t len) {
    W25_DBG("w25qxx_erase, address = 0x%08lx len = 0x%04lx", address, len);

    uint32_t first_sector = address / w25qxx->sector_size;
    uint32_t last_sector = (address + len - 1) / w25qxx->sector_size;

    for (uint32_t sector = first_sector; sector <= last_sector; ++sector) {
        if (w25qxx_wait_for_ready(w25qxx, 1000) != W25QXX_Ok) {
            return W25QXX_Timeout;
        }

        if (w25qxx_write_enable(w25qxx) == W25QXX_Ok) {
            uint32_t sector_start_address = sector * w25qxx->sector_size;
            uint8_t tx[4] = {
                W25QXX_SECTOR_ERASE,
                (uint8_t)(sector_start_address >> 16),
                (uint8_t)(sector_start_address >> 8),
                (uint8_t)(sector_start_address)
            };

            cs_on(w25qxx);
            if (w25qxx_transmit(w25qxx, tx, 4) != W25QXX_Ok) {
                cs_off(w25qxx);
                return W25QXX_Err;
            }
            cs_off(w25qxx);
        }
    }

    return W25QXX_Ok;
}

/**
 * @brief  Erase the entire W25Qxx chip
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_chip_erase(W25QXX_HandleTypeDef *w25qxx) {
    if (w25qxx_write_enable(w25qxx) != W25QXX_Ok) {
        return W25QXX_Err;
    }

    uint8_t tx = W25QXX_CHIP_ERASE;
    cs_on(w25qxx);
    if (w25qxx_transmit(w25qxx, &tx, 1) != W25QXX_Ok) {
        cs_off(w25qxx);
        return W25QXX_Err;
    }
    cs_off(w25qxx);

    return w25qxx_wait_for_ready(w25qxx, 60000); // 60s max hardware limit bound
}

/**
 * @brief  Wait for the W25Qxx device to be ready
 * @param  w25qxx: pointer to a W25QXX_HandleTypeDef structure
 * @param  timeout: maximum time to wait in milliseconds
 * @retval W25QXX_result_t: result of the operation
 */
W25QXX_result_t w25qxx_wait_for_ready(W25QXX_HandleTypeDef *w25qxx, uint32_t timeout) {
    uint32_t begin = HAL_GetTick();

    while ((HAL_GetTick() - begin) <= timeout) {
        if ((w25qxx_get_status(w25qxx) & 0x01) == 0) {
            return W25QXX_Ok;
        }
    }
    return W25QXX_Timeout;
}

// vim: ts=4 et nowrap autoindent