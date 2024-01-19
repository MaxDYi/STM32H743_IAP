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

#define APP_START_ADDRESS 0x08100000
#define APP_MAX_SIZE 0x00100000
#define APP_CRC_ADDRESS ((uint32_t)(APP_START_ADDRESS + APP_MAX_SIZE - 4))

#define CRC_SUCCESS 1
#define CRC_ERROR 0

void BootRun(void);

uint8_t GetBank(void);

void SwapBank(uint8_t bankNum);

uint32_t EraseBank(uint8_t bank);

uint32_t GetAppCRC(uint32_t startAddress, uint32_t size);

uint32_t ReadAppCRC(uint32_t crcAddr);

uint8_t CheckAppCRC(uint32_t startAddress, uint32_t appSize, uint32_t crcAddr);

#endif // __BOOTLOADER_H__