/*
 * w25qxx_littlefs.h
 *
 *  Created on: Mar 24, 2022
 *      Author: lth
 */

#ifndef SRC_W25QXX_LITTLEFS_H_
#define SRC_W25QXX_LITTLEFS_H_

#include "lfs.h"
#include "w25qxx.h"

#ifdef DEBUGxxx
#define LFS_DBG(...) printf(__VA_ARGS__);
#else
#define LFS_DBG(...) ;
#endif

extern lfs_t littlefs;

int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init, uint32_t reserved_mb);

#endif /* SRC_W25QXX_LITTLEFS_H_ */
