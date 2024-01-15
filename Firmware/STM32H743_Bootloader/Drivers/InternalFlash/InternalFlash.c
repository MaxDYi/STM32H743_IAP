#include "InternalFlash.h"

/*
 ******************************************************************************************************
 *    函 数 名: bsp_EraseCpuFlash
 *    功能说明: 擦除CPU FLASH一个扇区 （128KB)
 *    形    参: _ulFlashAddr : Flash地址
 *    返 回 值: 0 成功， 1 失败
 *              HAL_OK       = 0x00,
 *              HAL_ERROR    = 0x01,
 *              HAL_BUSY     = 0x02,
 *              HAL_TIMEOUT  = 0x03
 *
 ******************************************************************************************************
 */
uint8_t bsp_EraseCpuFlash(uint32_t _ulFlashAddr)
{
    uint32_t FirstSector = 0, NbOfSectors = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SECTORError = 0;
    uint8_t re;

    /* 解锁 */
    HAL_FLASH_Unlock();

    /* 获取此地址所在的扇区 */
    FirstSector = bsp_GetSector(_ulFlashAddr);

    /* 固定1个扇区 */
    NbOfSectors = 1;

    /* 擦除扇区配置 */
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (_ulFlashAddr >= ADDR_FLASH_SECTOR_0_BANK2)

    {
        EraseInitStruct.Banks = FLASH_BANK_2;
    }
    else

    {
        EraseInitStruct.Banks = FLASH_BANK_1;
    }

    EraseInitStruct.Sector = FirstSector;
    EraseInitStruct.NbSectors = NbOfSectors;

    /* 扇区擦除 */
    re = HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);

    /* 擦除完毕后，上锁 */
    HAL_FLASH_Lock();

    return re;
}

/*
*********************************************************************************************************
*    函 数 名: bsp_GetSector
*    功能说明: 根据地址计算扇区首地址
*    形    参: 无
*    返 回 值: 扇区号（0-7)
*********************************************************************************************************
*/
uint32_t bsp_GetSector(uint32_t Address)
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
             ((Address < CPU_FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_7_BANK2)))
    {
        sector = FLASH_SECTOR_7;
    }
    else
    {
        sector = FLASH_SECTOR_7;
    }

    return sector;
}

/*
******************************************************************************************************
*    函 数 名: bsp_WriteCpuFlash
*    功能说明: 写数据到CPU 内部Flash。 必须按32字节整数倍写。不支持跨扇区。扇区大小128KB. \
*              写之前需要擦除扇区. 长度不是32字节整数倍时，最后几个字节末尾补0写入.
*    形    参: _ulFlashAddr : Flash地址
*             _ucpSrc : 数据缓冲区
*             _ulSize : 数据大小（单位是字节, 必须是32字节整数倍）
*    返 回 值: 0-成功，1-数据长度或地址溢出，2-写Flash出错(估计Flash寿命到)
******************************************************************************************************
*/
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
    uint32_t i;
    uint8_t ucRet;

    /* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
    if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)

    {
        return 1;
    }

    /* 长度为0时不继续操作  */
    if (_ulSize == 0)

    {
        return 0;
    }

    ucRet = bsp_CmpCpuFlash(_ulFlashAddr, _ucpSrc, _ulSize);

    if (ucRet == FLASH_IS_EQU)

    {
        return 0;
    }

    __set_PRIMASK(1); /* 关中断 */

    /* FLASH 解锁 */
    HAL_FLASH_Unlock();

    for (i = 0; i < _ulSize / 32; i++)

    {
        uint64_t FlashWord[4];

        memcpy((char *)FlashWord, _ucpSrc, 32);
        _ucpSrc += 32;

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr,
                              (uint64_t)((uint32_t)FlashWord)) == HAL_OK)

        {
            _ulFlashAddr = _ulFlashAddr + 32; /* 递增，操作下一个256bit */
        }
        else

        {
            goto err;
        }
    }

    /* 长度不是32字节整数倍 */
    if (_ulSize % 32)

    {
        uint64_t FlashWord[4];

        FlashWord[0] = 0;
        FlashWord[1] = 0;
        FlashWord[2] = 0;
        FlashWord[3] = 0;
        memcpy((char *)FlashWord, _ucpSrc, _ulSize % 32);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr,
                              (uint64_t)((uint32_t)FlashWord)) == HAL_OK)

        {
            ; // _ulFlashAddr = _ulFlashAddr + 32;
        }
        else

        {
            goto err;
        }
    }

    /* Flash 加锁，禁止写Flash控制寄存器 */
    HAL_FLASH_Lock();

    __set_PRIMASK(0); /* 开中断 */

    return 0;

err:
    ./* Flash 加锁，禁止写Flash控制寄存器 */
        HAL_FLASH_Lock();

    __set_PRIMASK(0); /* 开中断 */

    return 1;
}

/*
*********************************************************************************************************
*    函 数 名: bsp_ReadCpuFlash
*    功能说明: 读取CPU Flash的内容
*    形    参:  _ucpDst : 目标缓冲区
*             _ulFlashAddr : 起始地址
*             _ulSize : 数据大小（单位是字节）
*    返 回 值: 0=成功，1=失败
*********************************************************************************************************
*/
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize)
{
    uint32_t i;

    if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
    {
        return 1;
    }

    /* 长度为0时不继续操作,否则起始地址为奇地址会出错 */
    if (_ulSize == 0)
    {
        return 1;
    }

    for (i = 0; i < _ulSize; i++)
    {
        *_ucpDst++ = *(uint8_t *)_ulFlashAddr++;
    }

    return 0;
}