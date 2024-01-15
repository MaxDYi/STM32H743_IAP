/*
 * @FilePath: \STM32H743_Bootloader\Drivers\Bootloader\Bootloader.c
 * @Author: MaxDYi
 * @Date: 2024-01-15 13:35:21
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-15 21:52:23
 * @Description:
 */
#include "Bootloader.h"

#define BANK1_TO_BANK2 0x01
#define BANK2_TO_BANK1 0x02

FLASH_OBProgramInitTypeDef OBInit;

void BootRun(void)
{
    printf("-----------------------------\r\n");
    printf("BootLoader Run\r\n");
    HAL_FLASH_Unlock();
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBGetConfig(&OBInit);
    OBInit.Banks = FLASH_BANK_1;
    HAL_FLASHEx_OBGetConfig(&OBInit);

    if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE)
    {
        /*Swap to bank2 */
        /*Set OB SWAP_BANK_OPT to swap Bank2*/
        printf("Boot Bank is Bank1\r\n");
        OBInit.OptionType = OPTIONBYTE_USER;
        OBInit.USERType = OB_USER_SWAP_BANK;
        OBInit.USERConfig = OB_SWAP_BANK_ENABLE;
        HAL_FLASHEx_OBProgram(&OBInit);
        HAL_FLASH_OB_Launch();
        printf("Set Bank2 as Boot Bank\r\n");
    }
    else
    {
        /* Swap to bank1 */
        /*Set OB SWAP_BANK_OPT to swap Bank1*/
        printf("Boot Bank is Bank2\r\n");
        printf("Copy Bank2 to Bank1\r\n");
        printf("Copy Bank2 to Bank1 Done\r\n");
        OBInit.Banks = FLASH_BANK_2;
        OBInit.OptionType = OPTIONBYTE_USER;
        OBInit.USERType = OB_USER_SWAP_BANK;
        OBInit.USERConfig = OB_SWAP_BANK_DISABLE;
        HAL_FLASHEx_OBProgram(&OBInit);
        HAL_FLASH_OB_Launch();
        printf("Set Bank1 as Boot Bank\r\n");
    }
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
    printf("Reset\r\n");
    NVIC_SystemReset();
}

uint8_t GetBank(void)
{
    uint32_t bank = 0;
    if ((FLASH->OPTCR & FLASH_OPTCR_SWAP_BANK) == FLASH_OPTCR_SWAP_BANK)
    {
        bank = FLASH_BANK_1;
    }
    else
    {
        bank = FLASH_BANK_2;
    }
    return bank;
}

uint32_t GetSector(uint32_t Address)
{
    uint32_t sector = 0;

    if (((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) ||
        ((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))
    {
        sector = FLASH_SECTOR_0;
    }
    else if (((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))
    {
        sector = FLASH_SECTOR_1;
    }
    else if (((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))
    {
        sector = FLASH_SECTOR_2;
    }
    else if (((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))
    {
        sector = FLASH_SECTOR_3;
    }
    else if (((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))
    {
        sector = FLASH_SECTOR_4;
    }
    else if (((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))
    {
        sector = FLASH_SECTOR_5;
    }
    else if (((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) ||
             ((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))
    {
        sector = FLASH_SECTOR_6;
    }
    else if (((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) ||
             ((Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
    {
        sector = FLASH_SECTOR_7;
    }
    else
    {
        sector = FLASH_SECTOR_7;
    }

    return sector;
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

void CopyFlash(uint8_t copyDirection)
{
    if (copyDirection == BANK1_TO_BANK2)
    {
        EraseBank(FLASH_BANK_2);
        for (uint64_t i = 0; i < FLASH_BANK_SIZE; i += 4)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              FLASH_BANK_2 + i, (FLASH_BANK_1 + i));
        }
    }
    else
    {
        EraseBank(FLASH_BANK_1);
        for (uint64_t i = 0; i < FLASH_BANK_SIZE; i += 4)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD,
                              FLASH_BANK_1 + i, (FLASH_BANK_2 + i));
        }
    }
}
