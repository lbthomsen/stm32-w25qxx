/*
 * w25qxx_littlefs.c
 *
 * Automatically configured LittleFS hardware abstraction layer
 * for W25QXX SPI/QSPI Flash chips with dynamic partitioning.
 *
 * Minimal RAM footprint, adaptive lookahead, and stateless callbacks.
 */

#include "w25qxx_littlefs.h"

#include "main.h"
#include "lfs.h"
#include "w25qxx.h"

// Static forward declarations for LittleFS block device operations
static int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int littlefs_erase(const struct lfs_config *c, lfs_block_t block);
static int littlefs_sync(const struct lfs_config *c);

// Dynamic runtime offset to bypass reserved memory space
static uint32_t flash_lfs_offset_bytes = 0;

// Global file system and configuration instances
lfs_t littlefs;
struct lfs_config littlefs_config = {
    // Assign hardware callback function pointers
    .read  = littlefs_read,
    .prog  = littlefs_prog,
    .erase = littlefs_erase,
    .sync  = littlefs_sync,

    // Static tuning flags
    .block_cycles = 500, // Increased to 500 for better wear leveling distribution
    .context = NULL,     // Filled dynamically on init
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

    // 1. Pass the hardware handle into the LittleFS driver context
    littlefs_config.context = w25qxx_init;

    // 2. Map geometry parameters dynamically
    littlefs_config.block_size = w25qxx_init->sector_size;

    // Calculate full physical block capabilities
    uint32_t total_blocks = w25qxx_init->sectors_in_block * w25qxx_init->block_count;

    // Calculate byte and block restrictions based on user reservation parameter
    flash_lfs_offset_bytes = reserved_mb * 1024 * 1024;
    uint32_t reserved_blocks = flash_lfs_offset_bytes / littlefs_config.block_size;

    if (reserved_blocks >= total_blocks) {
        LFS_DBG("LittleFS: Critical Error - Reserved space exceeds or equals flash capacity!\n");
        return LFS_ERR_INVAL;
    }

    // Assign remaining flash blocks exclusively to LittleFS
    littlefs_config.block_count = total_blocks - reserved_blocks;

    // 3. Configure optimal I/O transactional boundaries
    littlefs_config.read_size = 16;                     // Small reads prevent unnecessary SPI bus bloating
    littlefs_config.prog_size = w25qxx_init->page_size; // Matches physical page layout (typically 256B)
    littlefs_config.cache_size = w25qxx_init->page_size;

    // 4. Calculate lookahead size dynamically (1 bit per block, 32-bit aligned, min 8 bytes)
    uint32_t lookahead = (littlefs_config.block_count + 7) / 8; // Convert blocks to bytes
    lookahead = ((lookahead + 3) / 4) * 4;                      // Align up to a multiple of 4 bytes
    if (lookahead < 8) {
        lookahead = 8;                                          // Enforce LittleFS minimum criteria
    }
    littlefs_config.lookahead_size = lookahead;

    LFS_DBG("LittleFS: Blk Size: %lu, Blk Count: %lu, Lookahead: %lu, Offset: %lu Bytes\n",
            littlefs_config.block_size, littlefs_config.block_count, littlefs_config.lookahead_size, flash_lfs_offset_bytes);

    // 5. Try mounting the filesystem
    int err = lfs_mount(&littlefs, &littlefs_config);

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
    W25QXX_HandleTypeDef *w25qxx = (W25QXX_HandleTypeDef *)c->context;
    LFS_DBG("LFS Rd: B=0x%04lx, O=0x%04lx, S=0x%04lx\n", block, off, size);

    // Factor in runtime offset partition shift
    uint32_t raw_address = (block * w25qxx->sector_size) + off + flash_lfs_offset_bytes;

    if (w25qxx_read(w25qxx, raw_address, buffer, size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS low-level program (write) wrapper.
 */
static int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    W25QXX_HandleTypeDef *w25qxx = (W25QXX_HandleTypeDef *)c->context;
    LFS_DBG("LFS Prg: B=0x%04lx, O=0x%04lx, S=0x%04lx\n", block, off, size);

    // Factor in runtime offset partition shift
    uint32_t raw_address = (block * w25qxx->sector_size) + off + flash_lfs_offset_bytes;

    // Safely discard the const attribute to comply with your driver's signature interface
    if (w25qxx_write(w25qxx, raw_address, (void *)buffer, size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS low-level sector erase wrapper.
 */
static int littlefs_erase(const struct lfs_config *c, lfs_block_t block) {
    W25QXX_HandleTypeDef *w25qxx = (W25QXX_HandleTypeDef *)c->context;
    LFS_DBG("LFS Ers: B=0x%04lx\n", block);

    // Factor in runtime offset partition shift
    uint32_t raw_address = (block * w25qxx->sector_size) + flash_lfs_offset_bytes;

    if (w25qxx_erase(w25qxx, raw_address, w25qxx->sector_size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS block device synchronization callback.
 */
static int littlefs_sync(const struct lfs_config *c) {
    // Unused for synchronous SPI flash drivers.
    // If your driver utilizes asynchronous DMA, wait for transmission completion flags here.
    return LFS_ERR_OK;
}

/*
 * vim: ts=4 sw=4 et nowrap
 */
