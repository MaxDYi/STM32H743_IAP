/*
 * @FilePath: \STM32H743_Bootloader\Drivers\Bootloader\Bootloader.c
 * @Author: MaxDYi
 * @Date: 2024-01-15 13:35:21
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-18 17:52:21
 * @Description:
 */

#include "Bootloader.h"

#define BANK1_TO_BANK2 0x01
#define BANK2_TO_BANK1 0x02

#define APP_START_ADDRESS 0x08000000
#define APP_MAX_SIZE 0x00100000
#define APP_CRC_ADDRESS ((uint32_t)(APP_START_ADDRESS + APP_MAX_SIZE - 4))
#define CRC_SUCCESS 1
#define CRC_ERROR 0

FLASH_OBProgramInitTypeDef OBInit;

void BootRun(void)
{
    // printf("-----------------------------\r\n");
    // printf("V1.1.0\r\n");
    // printf("BootLoader Run\r\n");

    HAL_FLASHEx_OBGetConfig(&OBInit);
    OBInit.Banks = FLASH_BANK_1;
    HAL_FLASHEx_OBGetConfig(&OBInit);
    uint8_t bank = GetBank();
    if (bank == FLASH_BANK_1)
    {
        /*Swap to bank2 */
        /*Set OB SWAP_BANK_OPT to swap Bank2*/
    }
    else
    {
        /* Swap to bank1 */
        /*Set OB SWAP_BANK_OPT to swap Bank1*/
        uint32_t targetAddress = 0x08100000;
        uint32_t sourceAddress = 0x08000000;
        for (uint8_t i = 0; i < 8; i++)
        {
            EraseCpuFlash(targetAddress + i * 0x02000);
        }
        uint8_t flashData[1024];
        for (uint16_t i = 0; i < 1024; i++)
        {
            ReadCpuFlash(sourceAddress + i * 1024, flashData, 1024);
            WriteCpuFlash(targetAddress + i * 1024, flashData, 1024);
        }
        SwapBank(1);
        // printf("Reset\r\n");
    }
}

void SwapBank(uint8_t bankNum)
{
    if (bankNum == 1)
    {
        /* Swap to bank1 */
        HAL_FLASHEx_OBGetConfig(&OBInit);
        OBInit.Banks = FLASH_BANK_1;
        HAL_FLASHEx_OBGetConfig(&OBInit);
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        OBInit.OptionType = OPTIONBYTE_USER;
        OBInit.USERType = OB_USER_SWAP_BANK;
        OBInit.USERConfig = OB_SWAP_BANK_DISABLE;
        HAL_FLASHEx_OBProgram(&OBInit);
        HAL_FLASH_OB_Launch();
        HAL_FLASH_OB_Lock();
        NVIC_SystemReset();
    }
    else
    {
        /*Swap to bank2 */
        HAL_FLASHEx_OBGetConfig(&OBInit);
        OBInit.Banks = FLASH_BANK_1;
        HAL_FLASHEx_OBGetConfig(&OBInit);
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        OBInit.OptionType = OPTIONBYTE_USER;
        OBInit.USERType = OB_USER_SWAP_BANK;
        OBInit.USERConfig = OB_SWAP_BANK_ENABLE;
        HAL_FLASHEx_OBProgram(&OBInit);
        HAL_FLASH_OB_Launch();
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        NVIC_SystemReset();
    }
}

uint8_t GetBank(void)
{
    uint32_t bank = 0;
    HAL_FLASHEx_OBGetConfig(&OBInit);
    OBInit.Banks = FLASH_BANK_1;
    HAL_FLASHEx_OBGetConfig(&OBInit);
    if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE)
    {
        bank = FLASH_BANK_1;
    }
    else
    {
        bank = FLASH_BANK_2;
    }
    return bank;
}

uint32_t EraseBank(uint8_t bank)
{
    uint32_t err = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
    if (bank == FLASH_BANK_1)
    {
        EraseInitStruct.Banks = FLASH_BANK_1;
    }
    else
    {
        EraseInitStruct.Banks = FLASH_BANK_2;
    };
    EraseInitStruct.Sector = FLASH_SECTOR_0;
    EraseInitStruct.NbSectors = 8;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    HAL_FLASHEx_Erase(&EraseInitStruct, &err);
    return err;
}

uint32_t GetAppCRC(void)
{
    uint32_t CRC_value = HAL_CRC_Calculate(&hcrc, (uint32_t *)APP_START_ADDRESS,
                                           APP_MAX_SIZE / 4 - 1);
    return CRC_value;
}

uint32_t ReadAppCRC(void)
{
    uint32_t CRC_expected = 0;
    CRC_expected = *(__IO uint32_t *)APP_CRC_ADDRESS;
    return CRC_expected;
}

uint8_t CheckAppCRC(void)
{
    uint32_t CRC_value = GetAppCRC();
    uint32_t CRC_expected = ReadAppCRC();
    printf("CRC Expect: 0x%08X\r\n", CRC_expected);
    printf("CRC Result: 0x%08X\r\n", CRC_value);
    if (CRC_value == CRC_expected)
    {
        return CRC_SUCCESS;
    }
    else
    {
        return CRC_ERROR;
    }
}