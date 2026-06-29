/*
 * w25qxx_littlefs.c
 *
 * Automatically configured LittleFS hardware abstraction layer
 * for W25QXX SPI/QSPI Flash chips.
 *
 * Minimal RAM footprint, adaptive lookahead, and stateless callbacks.
 */

#include "main.h"
#include "lfs.h"
#include "w25qxx.h"
#include "w25qxx_littlefs.h"

// Static forward declarations for LittleFS block device operations
static int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int littlefs_erase(const struct lfs_config *c, lfs_block_t block);
static int littlefs_sync(const struct lfs_config *c);

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
 * @brief  Initializes and mounts LittleFS using the hardware driver dimensions.
 * @param  w25qxx_init: Pointer to the initialized hardware handle.
 * @retval 0 on success, or a negative LittleFS error code on failure.
 */
int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init) {
    if (w25qxx_init == NULL) {
        return LFS_ERR_INVAL;
    }

    LFS_DBG("LittleFS: Auto-configuring geometry...\n");

    // 1. Pass the hardware handle into the LittleFS driver context
    littlefs_config.context = w25qxx_init;

    // 2. Map geometry parameters dynamically
    // Note: LittleFS "block" size matches the hardware "sector" size (minimum erasable unit)
    littlefs_config.block_size = w25qxx_init->sector_size;
    littlefs_config.block_count = w25qxx_init->sectors_in_block * w25qxx_init->block_count;

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

    LFS_DBG("LittleFS: Blk Size: %lu, Blk Count: %lu, Lookahead: %lu\n",
            littlefs_config.block_size, littlefs_config.block_count, littlefs_config.lookahead_size);

    // 5. Try mounting the filesystem
    int err = lfs_mount(&littlefs, &littlefs_config);

    // Reformat automatically if the filesystem is unreadable (unformatted or blank flash)
    if (err) {
        LFS_DBG("LittleFS: Mount failed (%d). Formatting flash memory...\n", err);
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

    uint32_t raw_address = (block * w25qxx->sector_size) + off;

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

    uint32_t raw_address = (block * w25qxx->sector_size) + off;

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

    uint32_t raw_address = block * w25qxx->sector_size;

    if (w25qxx_erase(w25qxx, raw_address, w25qxx->sector_size) != W25QXX_Ok) {
        return LFS_ERR_IO;
    }
    return LFS_ERR_OK;
}

/**
 * @brief  LittleFS block device synchronization synchronization callback.
 */
static int littlefs_sync(const struct lfs_config *c) {
    // Unused for synchronous SPI flash drivers.
    // If your driver utilizes asynchronous DMA, wait for transmission completion flags here.
    return LFS_ERR_OK;
}

/*
 * vim: ts=4 sw=4 et nowrap
 */
