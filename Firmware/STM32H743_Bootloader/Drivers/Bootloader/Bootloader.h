/*
 * @FilePath: \STM32H743_Bootloader\Drivers\Bootloader\Bootloader.h
 * @Author: MaxDYi
 * @Date: 2024-01-15 13:35:21
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-15 21:30:57
 * @Description:
 */
#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "main.h"
#include "stm32h743xx.h"
#include "crc.h"
#include "stdio.h"
#include "string.h"

void BootRun(void);

uint8_t GetBank(void);

uint32_t GetSector(uint32_t Address);

uint32_t EraseBank(uint8_t bank);

void CopyFlash(uint8_t copyDirection);

#endif // __BOOTLOADER_H__