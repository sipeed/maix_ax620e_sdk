/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "cmn.h"
#include "mmc.h"
#include "spinor.h"

#define SD_SECTOR_SIZE		512
#define SD_SECTOR_COUNT		0x800000 //total 4GB
#define SD_BLOCK_SIZE		1 //1 sector = 4KB

#define NOR_SECTOR_SIZE		4096
#define NOR_SECTOR_COUNT	4096 //total 16MB
#define NOR_BLOCK_SIZE		1 //1 sector = 4KB

/* Definitions of physical drive number for each drive */
#define DEV_MMC		0	/* Example: Map MMC/SD card to physical drive 0 */
#define DEV_NOR		1	/* Example: Map NOR flash to physical drive 1 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
#if defined(AX620E_NOR)
	int result;
#endif
	switch (pdrv) {
	case DEV_MMC :
	#if 0
		result = sd_init(24000000, 1);
		if (result < 0)
			return RES_ERROR;
	#endif
		return RES_OK;
#if defined(AX620E_NOR)
	case DEV_NOR :
		result = spinor_init(24000000, 1);
		if (result < 0)
			return RES_ERROR;
		return RES_OK;
#endif
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int result;

	switch (pdrv) {
	case DEV_MMC :
		result = sd_read((char *)buff, sector * SD_SECTOR_SIZE, count * SD_SECTOR_SIZE);
		if (result < 0)
			return RES_ERROR;
		return RES_OK;
#if defined(AX620E_NOR)
	case DEV_NOR :
		result = spinor_read(sector * NOR_SECTOR_SIZE, count * NOR_SECTOR_SIZE, buff);
		if (result < 0)
			return RES_ERROR;
		return RES_OK;
#endif
	}
	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int result;

	switch (pdrv) {
	case DEV_MMC :
		/*
		result = sd_write(sector * SD_SECTOR_SIZE, count * SD_SECTOR_SIZE, buff);
		if (result < 0)
			return RES_ERROR;
		*/
		return RES_OK;

	case DEV_NOR :
		for (; count>0; count--) {
			result = spinor_erase(sector * NOR_SECTOR_SIZE, NOR_SECTOR_SIZE);
			if (result < 0)
				return RES_ERROR;
			result = spinor_write(sector * NOR_SECTOR_SIZE, NOR_SECTOR_SIZE, buff);
			if (result < 0)
				return RES_ERROR;
			sector++;
			buff += NOR_SECTOR_SIZE;
		}
		return RES_OK;
	}

	return RES_PARERR;
}

#endif

DRESULT sd_disk_ioctl (
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (cmd) {
	case CTRL_SYNC :
		return RES_OK;
	case GET_SECTOR_COUNT :
		*(LBA_t*)buff = SD_SECTOR_COUNT;
		return RES_OK;
	case GET_SECTOR_SIZE :
		*(WORD*)buff = SD_SECTOR_SIZE;
		return RES_OK;
	case GET_BLOCK_SIZE :
		*(DWORD*)buff = SD_BLOCK_SIZE;
		return RES_OK;
	case CTRL_TRIM :
		return RES_OK;
	}

	return RES_PARERR;
}

DRESULT nor_disk_ioctl (
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (cmd) {
	case CTRL_SYNC :
		return RES_OK;
	case GET_SECTOR_COUNT :
		*(LBA_t*)buff = NOR_SECTOR_COUNT;
		return RES_OK;
	case GET_SECTOR_SIZE :
		*(WORD*)buff = NOR_SECTOR_SIZE;
		return RES_OK;
	case GET_BLOCK_SIZE :
		*(DWORD*)buff = NOR_BLOCK_SIZE;
		return RES_OK;
	case CTRL_TRIM :
		return RES_OK;
	}

	return RES_PARERR;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;

	switch (pdrv) {
	case DEV_MMC :
		res = sd_disk_ioctl(cmd, buff);
		return res;
#if defined(AX620E_NOR)
	case DEV_NOR :
		res = nor_disk_ioctl(cmd, buff);
		return res;
#endif
	}
	return RES_PARERR;
}

