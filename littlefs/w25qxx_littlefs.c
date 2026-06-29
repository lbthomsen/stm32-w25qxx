/*
 * w25qxx_littlefs.c
 *
 *  Created on: Mar 24, 2022
 *      Author: lth
 */


#include "main.h"
#include "lfs.h"
#include "w25qxx.h"
#include "w25qxx_littlefs.h"

int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int littlefs_erase(const struct lfs_config *c, lfs_block_t block);
int littlefs_sync(const struct lfs_config *c);

struct lfs_config littlefs_config = {
    // block device operations
    .read  = littlefs_read,
    .prog  = littlefs_prog,
    .erase = littlefs_erase,
    .sync  = littlefs_sync,

    // block device configuration
    .read_size = 256,
    .prog_size = 256,
    .block_size = 4096,
    .block_count = 256,
    .cache_size = 256,
    .lookahead_size = 8,
    .block_cycles = 100,
};

lfs_t littlefs;
W25QXX_HandleTypeDef *w25qxx_handle;

int w25qxx_littlefs_init(W25QXX_HandleTypeDef *w25qxx_init) {
	LFS_DBG("LittleFS Init");
	w25qxx_handle = w25qxx_init;

	littlefs_config.block_size = w25qxx_handle->sector_size;
	littlefs_config.block_count = w25qxx_handle->sectors_in_block * w25qxx_handle->block_count;

	int err = lfs_mount(&littlefs, &littlefs_config);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&littlefs, &littlefs_config);
        lfs_mount(&littlefs, &littlefs_config);
    }

    return 0;

}

int littlefs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
	LFS_DBG("LittleFS Read b = 0x%04lx o = 0x%04lx s = 0x%04lx", block, off, size);
	if (w25qxx_read(w25qxx_handle, block * w25qxx_handle->sector_size + off, buffer, size) != W25QXX_Ok) return -1;
	return 0;
}

int littlefs_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
	LFS_DBG("LittleFS Prog b = 0x%04lx o = 0x%04lx s = 0x%04lx", block, off, size);
	if (w25qxx_write(w25qxx_handle, block * w25qxx_handle->sector_size + off, (void *)buffer, size) != W25QXX_Ok) return -1;
	return 0;
}

int littlefs_erase(const struct lfs_config *c, lfs_block_t block) {
	LFS_DBG("LittleFS Erase b = 0x%04lx", block);
	if (w25qxx_erase(w25qxx_handle, block * w25qxx_handle->sector_size, w25qxx_handle->sector_size) != W25QXX_Ok) return -1;
	return 0;
}

int littlefs_sync(const struct lfs_config *c) {
	LFS_DBG("LittleFS Sync");
	return 0;
}

/*
 * vim: ts=4 nowrap
 */
