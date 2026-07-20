/**
 ******************************************************************************
 * @file           : w25qxx_littlefs.h
 * @brief          : Littlefs on top of w25qxx header
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 Lars Boegild Thomsen <lbthomsen@gmail.com>
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#ifndef SRC_W25QXX_LITTLEFS_H_
#define SRC_W25QXX_LITTLEFS_H_

#include "lfs.h"
#include "w25qxx.h"

#define LITTLEFS_FORMAT_ON_ERROR 1

#ifdef DEBUGxxx
#define LFS_DBG(...) printf(__VA_ARGS__);
#else
#define LFS_DBG(...) ;
#endif

extern lfs_t littlefs;

int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init, uint32_t reserved_mb);

#endif /* SRC_W25QXX_LITTLEFS_H_ */

/*
 * vim: ts=4 sw=4 et nowrap
 */
