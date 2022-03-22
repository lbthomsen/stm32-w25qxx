/*
 * Minimal w25qxx library
 *
 * Copyright (c) 2022 Lars Boegild Thomsen <lbthomsen@gmail.com>
 *
 * Notice!  The library does _not_ bother to check that sectors have been erased
 * before writing.
 *
 */

#include "main.h"
#include "string.h"
#include "w25qxx.h"

#ifdef DEBUG
#include "stdio.h"
#endif

/**
 * Internal functions
 */

static inline void cs_on(W25QXX_HandleTypeDef *w25qxx) {
	HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_RESET);
}

static inline void cs_off(W25QXX_HandleTypeDef *w25qxx) {
	HAL_GPIO_WritePin(w25qxx->cs_port, w25qxx->cs_pin, GPIO_PIN_SET);
}

/*
 * Transmit data to w25qxx - ignore response
 */
W25QXX_result_t w25qxx_transmit(W25QXX_HandleTypeDef *w25qxx, uint8_t *buf, uint32_t len) {
	W25QXX_result_t ret = W25QXX_Err;
	if (HAL_SPI_Transmit(w25qxx->spiHandle, buf, len, HAL_MAX_DELAY) == HAL_OK) {
		ret = W25QXX_Ok;
	}
	return ret;
}

/*
 * Receive data from w25qxx
 */
W25QXX_result_t w25qxx_receive(W25QXX_HandleTypeDef *w25qxx, uint8_t *buf, uint32_t len) {
	W25QXX_result_t ret = W25QXX_Err;
	if (HAL_SPI_Receive(w25qxx->spiHandle, buf, len, HAL_MAX_DELAY) == HAL_OK) {
		ret = W25QXX_Ok;
	}
	return ret;
}

uint32_t w25qxx_read_id(W25QXX_HandleTypeDef *w25qxx) {
	uint32_t ret = 0;
	uint8_t buf[3];
	cs_on(w25qxx);
	buf[0] = W25QXX_GET_ID;
	if (w25qxx_transmit(w25qxx, buf, 1) == W25QXX_Ok) {
		if (w25qxx_receive(w25qxx, buf, 3) == W25QXX_Ok) {
			ret = (uint32_t) ((buf[0] << 16) | (buf[1] << 8) | (buf[2]));
		}
	}
	cs_off(w25qxx);
	return ret;
}

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

W25QXX_result_t w25qxx_write_enable(W25QXX_HandleTypeDef *w25qxx) {
	W25_DBG("w25qxx_write_enable");
	W25QXX_result_t ret = W25QXX_Err;
	uint8_t buf[1];
	cs_on(w25qxx);
	buf[0] = W25QXX_WRITE_ENABLE;
	if (w25qxx_transmit(w25qxx, buf, 1) == W25QXX_Ok) {
		ret = W25QXX_Ok;
	}
	cs_off(w25qxx);
	return ret;
}

W25QXX_result_t w25qxx_wait_for_ready(W25QXX_HandleTypeDef *w25qxx,
		uint32_t timeout) {
	W25QXX_result_t ret = W25QXX_Ok;
	uint32_t begin = HAL_GetTick();
	uint32_t now = HAL_GetTick();
	while ((now - begin <= timeout) && (w25qxx_get_status(w25qxx) && 0x01 == 0x01)) {
		now = HAL_GetTick();
	}
	if (now - begin == timeout)
		ret = W25QXX_Timeout;
	return ret;
}

W25QXX_result_t w25qxx_init(W25QXX_HandleTypeDef *w25qxx,
		SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {

	W25QXX_result_t result = W25QXX_Ok;

	W25_DBG("w25qxx_init");

	w25qxx->spiHandle = hspi;
	w25qxx->cs_port = cs_port;
	w25qxx->cs_pin = cs_pin;

	cs_off(w25qxx);

	uint32_t id = w25qxx_read_id(w25qxx);
	if (id) {
		w25qxx->manufacturer_id = (uint8_t) (id >> 16);
		w25qxx->device_id = (uint16_t) (id & 0xFFFF);

		switch (w25qxx->manufacturer_id) {
		case W25QXX_MANUFACTURER_WINBOND:

			w25qxx->block_size = 0x10000;
			w25qxx->sector_size = 0x1000;
			w25qxx->sectors_in_block = 0x10;
			w25qxx->page_size = 0x100;
			w25qxx->pages_in_sector = 0x10;

			switch (w25qxx->device_id) {
			case 0x4018:
				w25qxx->block_count = 0x100;
				break;
			default:
				W25_DBG("Unknown Winbond device");
				result = W25QXX_Err;
			}

			break;
		default:
			W25_DBG("Unknown manufacturer");
			result = W25QXX_Err;
		}
	} else {
		result = W25QXX_Err;
	}

	if (result == W25QXX_Err) {
		// Zero the handle so it is clear initialization failed!
		memset(w25qxx, 0, sizeof(W25QXX_HandleTypeDef));
	}

	return result;

}

W25QXX_result_t w25qxx_read(W25QXX_HandleTypeDef *w25qxx, uint32_t address,
		uint8_t *buf, uint32_t len) {

	W25_DBG("w25qxx_read - address: 0x%08lx, lengh: 0x%04lx", address, len);

	// Transmit buffer holding command and address
	uint8_t tx[4] = {
	W25QXX_READ_DATA, (uint8_t) (address >> 16), (uint8_t) (address >> 8),
			(uint8_t) (address), };

	// First wait for device to get ready
	if (w25qxx_wait_for_ready(w25qxx, HAL_MAX_DELAY) != W25QXX_Ok) {
		return W25QXX_Err;
	}

	cs_on(w25qxx);
	if (w25qxx_transmit(w25qxx, tx, 4) == W25QXX_Ok) { // size will always be fixed
		if (w25qxx_receive(w25qxx, buf, len) != W25QXX_Ok) {
			cs_off(w25qxx);
			return W25QXX_Err;
		}
	}
	cs_off(w25qxx);

	return W25QXX_Ok;
}

W25QXX_result_t w25qxx_write(W25QXX_HandleTypeDef *w25qxx, uint32_t address,
		uint8_t *buf, uint32_t len) {

	W25_DBG("w25qxx_write - address 0x%08lx len 0x%04lx", address, len);

	// Let's determine the pages
	uint32_t first_page = address / w25qxx->page_size;
	uint32_t last_page = (address + len - 1) / w25qxx->page_size;

	W25_DBG("w25qxx_write %lu pages from %lu to %lu", 1 + last_page - first_page, first_page, last_page);

	uint32_t buffer_offset = 0;
	uint32_t start_address = address;

	for (uint32_t page = first_page; page <= last_page; ++page) {

		uint32_t end_address = page < last_page ? start_address + w25qxx->page_size : address + len;
		uint32_t write_len = end_address - start_address;

		W25_DBG("w25qxx_write: handling page %lu start_address = 0x%08lx end_address = 0x%08lx buffer_offset = 0x%08lx len = %04lx", page, start_address, end_address, buffer_offset, write_len);

		// First wait for device to get ready
		if (w25qxx_wait_for_ready(w25qxx, HAL_MAX_DELAY) != W25QXX_Ok) {
			return W25QXX_Err;
		}

		if (w25qxx_write_enable(w25qxx) == W25QXX_Ok) {

			uint8_t tx[4] = {
					W25QXX_PAGE_PROGRAM,
					(uint8_t) (start_address >> 16),
					(uint8_t) (start_address >> 8),
					(uint8_t) (start_address),
			};

			cs_on(w25qxx);
			if (w25qxx_transmit(w25qxx, tx, 4) == W25QXX_Ok) { // size will always be fixed
				// Now write the buffer
				if (w25qxx_transmit(w25qxx, buf, write_len) != W25QXX_Ok) {
					cs_off(w25qxx);
					return W25QXX_Err;
				}
			}
			cs_off(w25qxx);

		}

		start_address += w25qxx->page_size;
		buffer_offset += w25qxx->page_size;

	}

	return W25QXX_Ok;
}

W25QXX_result_t w25qxx_erase(W25QXX_HandleTypeDef *w25qxx, uint32_t address,
		uint32_t len) {

	W25_DBG("w25qxx_erase, address = 0x%08lx len = 0x%04lx", address, len);

	W25QXX_result_t ret = W25QXX_Ok;

	// Let's determine the sector start
	uint32_t first_sector = address / w25qxx->sector_size;
	uint32_t last_sector = (address + len - 1) / w25qxx->sector_size;

	W25_DBG("w25qxx_erase: first sector: 0x%04lx", first_sector);
	W25_DBG("w25qxx_erase: last sector : 0x%04lx", last_sector);

	for (uint32_t sector = first_sector; sector <= last_sector; ++sector) {

		W25_DBG("Erasing sector %lu, starting at: 0x%08lx", sector,
				sector * w25qxx->sector_size);

		// First we have to ensure the device is not busy
		if (w25qxx_wait_for_ready(w25qxx, HAL_MAX_DELAY) == W25QXX_Ok) {
			if (w25qxx_write_enable(w25qxx) == W25QXX_Ok) {

				uint32_t sector_start_address = sector * w25qxx->sector_size;

				uint8_t tx[4] = {
				W25QXX_SECTOR_ERASE, (uint8_t) (sector_start_address >> 16),
						(uint8_t) (sector_start_address >> 8),
						(uint8_t) (sector_start_address), };

				cs_on(w25qxx);
				if (w25qxx_transmit(w25qxx, tx, 4) != W25QXX_Ok) {
					ret = W25QXX_Err;
				}
				cs_off(w25qxx);
			}
		} else {
			ret = W25QXX_Timeout;
		}

	}

	return ret;
}

W25QXX_result_t w25qxx_chip_erase(W25QXX_HandleTypeDef *w25qxx) {
	if (w25qxx_write_enable(w25qxx) == W25QXX_Ok) {
		uint8_t tx[1] = {
				W25QXX_CHIP_ERASE
		};
		cs_on(w25qxx);
		if (w25qxx_transmit(w25qxx, tx, 1) != W25QXX_Ok) {
			return W25QXX_Err;
		}
		cs_off(w25qxx);
		if (w25qxx_wait_for_ready(w25qxx, HAL_MAX_DELAY) != W25QXX_Ok) {
			return W25QXX_Err;
		}
	}
	return W25QXX_Ok;
}

/*
 * vim: ts=4 nowrap
 */
