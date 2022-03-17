/*
 * w25qxx_diskio.h
 *
 *  Created on: Mar 17, 2022
 *      Author: lth
 */

#ifndef W25QXX_DISKIO_H_
#define W25QXX_DISKIO_H_

extern Diskio_drvTypeDef  w25qxx_Driver;

DSTATUS w25qxx_diskio_initialize (BYTE pdrv);

DSTATUS w25qxx_diskio_status (BYTE pdrv);

DRESULT w25qxx_diskio_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);

#if _USE_WRITE == 1
  DRESULT w25qxx_diskio_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
#endif /* _USE_WRITE == 1 */

#if _USE_IOCTL == 1
  DRESULT w25qxx_diskio_ioctl (BYTE pdrv, BYTE cmd, void *buff);
#endif /* _USE_IOCTL == 1 */


#endif /* W25QXX_DISKIO_H_ */
