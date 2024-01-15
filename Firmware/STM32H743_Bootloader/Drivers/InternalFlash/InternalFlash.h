#ifndef __INTERNALFLASH_H__
#define __INTERNALFLASH_H__

#include "main.h"

uint8_t bsp_EraseCpuFlash(uint32_t _ulFlashAddr);
uint32_t bsp_GetSector(uint32_t Address);
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize);
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize);

#endif // __INTERNALFLASH_H__