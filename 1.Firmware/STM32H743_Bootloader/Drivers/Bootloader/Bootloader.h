/*
 * @FilePath: \STM32H743_Bootloader\Drivers\Bootloader\Bootloader.h
 * @Author: MaxDYi
 * @Date: 2024-01-15 13:35:21
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-18 16:25:21
 * @Description:
 */
#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include "main.h"
#include "stm32h743xx.h"
#include "crc.h"
#include "stdio.h"
#include "string.h"
#include "ymodem.h"

void BootRun(void);

uint8_t GetBank(void);

void SwapBank(uint8_t bankNum);

uint32_t EraseBank(uint8_t bank);

uint8_t CheckAppCRC(void);

uint32_t ReadAppCRC(void);

uint8_t CheckAppCRC(void);

#endif // __BOOTLOADER_H__