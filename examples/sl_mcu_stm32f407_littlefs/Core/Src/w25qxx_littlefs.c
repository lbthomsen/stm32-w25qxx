/**
 ******************************************************************************
 * @file           : w25qxx_littlefs.c
 * @brief          : Littlefs on top of w25qxx source
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

#include "w25qxx_littlefs.h"
#include "main.h"
#include "lfs.h"
#include "w25qxx.h"

// Define a structured context to cleanly isolate instance hardware and configuration offsets
typedef struct {
    W25QXX_HandleTypeDef *hw_handle;
    uint32_t flash_offset;
} LFS_DriverContext_t;

// Static driver context tracking runtime configurations
static LFS_DriverContext_t lfs_ctx;

// Maximum expected size for lookahead buffer tracking block usage states.
#define LFS_LOOKAHEAD_MAX_SIZE 16
static uint8_t lfs_lookahead_buf[LFS_LOOKAHEAD_MAX_SIZE];

// Static forward declarations for LittleFS block device operations
static int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int littlefs_erase(const struct lfs_config *c, lfs_block_t block);
static int littlefs_sync(const struct lfs_config *c);

// Global file system and configuration instances
lfs_t littlefs;
struct lfs_config littlefs_config = {
        // Assign hardware callback function pointers
        .read = littlefs_read,
        .prog = littlefs_prog,
        .erase = littlefs_erase,
        .sync = littlefs_sync,

        // Static tuning flags
        .block_cycles = 500, // Increased to 500 for better wear leveling distribution
        };

/**
 * @brief  Initializes and mounts LittleFS using the hardware driver dimensions,
 * leaving a specified amount of megabytes untouched at the start of the flash.
 * @param  w25qxx_init: Pointer to the initialized hardware handle.
 * @param  reserved_mb: Number of Megabytes to reserve at the beginning of the flash (e.g., 2).
 * @retval 0 on success, or a negative LittleFS error code on failure.
 */
int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init, uint32_t reserved_mb) {
    if (w25qxx_init == NULL) {
        return LFS_ERR_INVAL;
    }

    LFS_DBG("LittleFS: Auto-configuring geometry...\n");

    // 1. Populate custom context pointer boundaries
    lfs_ctx.hw_handle = w25qxx_init;
    lfs_ctx.flash_offset = reserved_mb * 1024 * 1024;
    littlefs_config.context = &lfs_ctx;

    // 2. Map geometry parameters dynamically
    littlefs_config.block_size = w25qxx_init->sector_size;

    // Calculate full physical block capabilities
    uint32_t total_blocks = w25qxx_init->sectors_in_block * w25qxx_init->block_count;
    uint32_t reserved_blocks = lfs_ctx.flash_offset / littlefs_config.block_size;

    if (reserved_blocks >= total_blocks) {
        LFS_DBG("LittleFS: Critical Error - Reserved space exceeds or equals flash capacity!\n");
        return LFS_ERR_INVAL;
    }

    // Assign remaining flash blocks exclusively to LittleFS
    littlefs_config.block_count = total_blocks - reserved_blocks;

    // 3. Configure optimal I/O transactional boundaries
    littlefs_config.read_size = 16;                     // Small reads prevent unnecessary SPI bus bloating
    littlefs_config.prog_size = w25qxx_init->page_size; // Matches physical page layout (typically 256B), safe due to multi-page driver architecture
    littlefs_config.cache_size = w25qxx_init->page_size;

    // 4. Calculate lookahead size dynamically (1 bit per block, 32-bit aligned, min 8 bytes)
    uint32_t lookahead = (littlefs_config.block_count + 7) / 8; // Convert blocks to bytes
    lookahead = ((lookahead + 3) / 4) * 4;                      // Align up to a multiple of 4 bytes
    if (lookahead < 8) {
        lookahead = 8;                                          // Enforce LittleFS minimum criteria
    }

    // Bounds check calculated lookahead up to allocated static limits
    if (lookahead > LFS_LOOKAHEAD_MAX_SIZE) {
        lookahead = LFS_LOOKAHEAD_MAX_SIZE;
    }

    littlefs_config.lookahead_size = lookahead;
    littlefs_config.lookahead_buffer = lfs_lookahead_buf;      // Assign the static lookup allocation map

    LFS_DBG("LittleFS: Blk Size: %lu, Blk Count: %lu, Lookahead: %lu, Offset: %lu Bytes\n",
            littlefs_config.block_size, littlefs_config.block_count, littlefs_config.lookahead_size, lfs_ctx.flash_offset);

    // 5. Try mounting the filesystem
    int err = lfs_mount(&littlefs, &littlefs_config);

#ifdef LITTLEFS_FORMAT_ON_ERROR // defined in header
    // Reformat automatically if the filesystem is unreadable (unformatted or blank flash)
    if (err) {
        LFS_DBG("LittleFS: Mount failed (%d). Formatting flash partition...\n", err);
        err = lfs_format(&littlefs, &littlefs_config);
        if (err) {
            LFS_DBG("LittleFS: Format critical failure (%d)\n", err);
            return err;
        }

        // Remount following fresh layout formatting
        err = lfs_mount(&littlefs, &littlefs_config);
    }
#endif

    if (err == LFS_ERR_OK) {
        LFS_DBG("LittleFS: Filesystem mounted successfully.\n");
    } else {
        LFS_DBG("LittleFS: Initialization failed (%d)\n", err);
    }

    return err;
}

/**
 * @brief  LittleFS low-level read wrapper.
 */
static int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    LFS_DriverContext_t *ctx = (LFS_DriverContext_t*) c->context;
    LFS_DBG("LFS Rd: B=0x%04lx, O=0x%04lx, S=0x%04lx\n", block, off, size);

    // Defensive Guardrail: Ensure requested operations do not spill outside allocated block partitions
    if (block >= c->block_count) {
        return LFS_ERR_INVAL;
    }

    // Factor in runtime offset partition shift
    uint32_t raw_address = (block * c->block_size) + off + ctx->flash_offset;

    if (w25qxx_read(ctx->hw_handle, raw_address, buffer, size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS low-level program (write) wrapper.
 */
static int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    LFS_DriverContext_t *ctx = (LFS_DriverContext_t*) c->context;
    LFS_DBG("LFS Prg: B=0x%04lx, O=0x%04lx, S=0x%04lx\n", block, off, size);

    // Defensive Guardrail: Ensure requested operations do not spill outside allocated block partitions
    if (block >= c->block_count) {
        return LFS_ERR_INVAL;
    }

    // Factor in runtime offset partition shift
    uint32_t raw_address = (block * c->block_size) + off + ctx->flash_offset;

    // Safely discard the const attribute to comply with driver's signature interface
    if (w25qxx_write(ctx->hw_handle, raw_address, (uint8_t*) buffer, size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS low-level sector erase wrapper.
 */
static int littlefs_erase(const struct lfs_config *c, lfs_block_t block) {
    LFS_DriverContext_t *ctx = (LFS_DriverContext_t*) c->context;
    LFS_DBG("LFS Ers: B=0x%04lx\n", block);

    // Defensive Guardrail: Ensure requested operations do not spill outside allocated block partitions
    if (block >= c->block_count) {
        return LFS_ERR_INVAL;
    }

    // Force exact absolute hardware alignment starting positions
    uint32_t target_sector_address = (block * c->block_size) + ctx->flash_offset;

    // Clear exactly c->block_size (4096B) to safely align driver tracking parameters
    if (w25qxx_erase(ctx->hw_handle, target_sector_address, c->block_size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS block device synchronization callback.
 */
static int littlefs_sync(const struct lfs_config *c) {
    LFS_DriverContext_t *ctx = (LFS_DriverContext_t*) c->context;

    // Explicitly check and block until physical chip completes internal operations
    // to safeguard critical power-loss safe synchronization calls.
    if (w25qxx_wait_for_ready(ctx->hw_handle, 1000) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/*
 * vim: ts=4 sw=4 et nowrap
 */
