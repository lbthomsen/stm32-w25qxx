/*
 * w25qxx_littlefs.h
 *
 *  Created on: Mar 24, 2022
 *      Author: lth
 */

#ifndef W25QXX_LITTLEFS_H_
#define W25QXX_LITTLEFS_H_

#include "lfs.h"
#include "w25qxx.h"

#ifdef DEBUGxxx
#define LFS_DBG(...) printf(__VA_ARGS__);
#else
#define LFS_DBG(...) ;
#endif

extern lfs_t littlefs;

int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init);

#endif /* W25QXX_LITTLEFS_H_ */
