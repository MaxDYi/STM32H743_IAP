/*
*********************************************************************************************************
*
*	ģ������ : cpu�ڲ�falsh����ģ��(for STM32H743 H750)
*	�ļ����� : bsp_cpu_flash.c
*	��    �� : V1.1
*	˵    �� : �ṩ��дCPU�ڲ�Flash�ĺ���
*	�޸ļ�¼ :
*		�汾��  ����        ����     ˵��
*		V1.0    2019-09-20  armfly  ��ʽ����
*		V1.1    2019-10-03  armfly  дflash�������������Ȳ���32�ֽ���������bug��ĩβ��0д�롣
*									���HAL�⺯���� HAL_FLASH_Program������bug��
*									���������Ǵ������������ʧ�ܣ������ָ��˳��� PGSERR1�� PGSERR2��
*	Copyright (C), 2019-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"




/*
*********************************************************************************************************
*	�� �� ��: bsp_GetSector
*	����˵��: ���ݵ�ַ���������׵�ַ
*	��    ��: ��
*	�� �� ֵ: �����ţ�0-7)
*********************************************************************************************************
*/
uint32_t bsp_GetSector(uint32_t Address)
{
	uint32_t sector = 0;

	if (((Address < ADDR_FLASH_SECTOR_1_BANK1) && (Address >= ADDR_FLASH_SECTOR_0_BANK1)) || \
		((Address < ADDR_FLASH_SECTOR_1_BANK2) && (Address >= ADDR_FLASH_SECTOR_0_BANK2)))    
	{
		sector = FLASH_SECTOR_0;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_2_BANK1) && (Address >= ADDR_FLASH_SECTOR_1_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_2_BANK2) && (Address >= ADDR_FLASH_SECTOR_1_BANK2)))    
	{
		sector = FLASH_SECTOR_1;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_3_BANK1) && (Address >= ADDR_FLASH_SECTOR_2_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_3_BANK2) && (Address >= ADDR_FLASH_SECTOR_2_BANK2)))    
	{
		sector = FLASH_SECTOR_2;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_4_BANK1) && (Address >= ADDR_FLASH_SECTOR_3_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_4_BANK2) && (Address >= ADDR_FLASH_SECTOR_3_BANK2)))    
	{
		sector = FLASH_SECTOR_3;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_5_BANK1) && (Address >= ADDR_FLASH_SECTOR_4_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_5_BANK2) && (Address >= ADDR_FLASH_SECTOR_4_BANK2)))    
	{
		sector = FLASH_SECTOR_4;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_6_BANK1) && (Address >= ADDR_FLASH_SECTOR_5_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_6_BANK2) && (Address >= ADDR_FLASH_SECTOR_5_BANK2)))    
	{
		sector = FLASH_SECTOR_5;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_7_BANK1) && (Address >= ADDR_FLASH_SECTOR_6_BANK1)) || \
	  ((Address < ADDR_FLASH_SECTOR_7_BANK2) && (Address >= ADDR_FLASH_SECTOR_6_BANK2)))    
	{
		sector = FLASH_SECTOR_6;  
	}
	else if (((Address < ADDR_FLASH_SECTOR_0_BANK2) && (Address >= ADDR_FLASH_SECTOR_7_BANK1)) || \
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
*********************************************************************************************************
*	�� �� ��: bsp_ReadCpuFlash
*	����˵��: ��ȡCPU Flash������
*	��    ��:  _ucpDst : Ŀ�껺����
*			 _ulFlashAddr : ��ʼ��ַ
*			 _ulSize : ���ݴ�С����λ���ֽڣ�
*	�� �� ֵ: 0=�ɹ���1=ʧ��
*********************************************************************************************************
*/
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize)
{
	uint32_t i;

	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return 1;
	}

	/* ����Ϊ0ʱ����������,������ʼ��ַΪ���ַ����� */
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

/*
*********************************************************************************************************
*	�� �� ��: bsp_CmpCpuFlash
*	����˵��: �Ƚ�Flashָ����ַ������.
*	��    ��: _ulFlashAddr : Flash��ַ
*			 _ucpBuf : ���ݻ�����
*			 _ulSize : ���ݴ�С����λ���ֽڣ�
*	�� �� ֵ:
*			FLASH_IS_EQU		0   Flash���ݺʹ�д���������ȣ�����Ҫ������д����
*			FLASH_REQ_WRITE		1	Flash����Ҫ������ֱ��д
*			FLASH_REQ_ERASE		2	Flash��Ҫ�Ȳ���,��д
*			FLASH_PARAM_ERR		3	������������
*********************************************************************************************************
*/
uint8_t bsp_CmpCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpBuf, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucIsEqu;	/* ��ȱ�־ */
	uint8_t ucByte;

	/* ���ƫ�Ƶ�ַ����оƬ�������򲻸�д��������� */
	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return FLASH_PARAM_ERR;		/*��������������*/
	}

	/* ����Ϊ0ʱ������ȷ */
	if (_ulSize == 0)
	{
		return FLASH_IS_EQU;		/* Flash���ݺʹ�д���������� */
	}

	ucIsEqu = 1;			/* �ȼ��������ֽںʹ�д���������ȣ���������κ�һ������ȣ�������Ϊ 0 */
	for (i = 0; i < _ulSize; i++)
	{
		ucByte = *(uint8_t *)_ulFlashAddr;

		if (ucByte != *_ucpBuf)
		{
			if (ucByte != 0xFF)
			{
				return FLASH_REQ_ERASE;		/* ��Ҫ��������д */
			}
			else
			{
				ucIsEqu = 0;	/* ����ȣ���Ҫд */
			}
		}

		_ulFlashAddr++;
		_ucpBuf++;
	}

	if (ucIsEqu == 1)
	{
		return FLASH_IS_EQU;	/* Flash���ݺʹ�д���������ȣ�����Ҫ������д���� */
	}
	else
	{
		return FLASH_REQ_WRITE;	/* Flash����Ҫ������ֱ��д */
	}
}

/*
*********************************************************************************************************
*	�� �� ��: bsp_EraseCpuFlash
*	����˵��: ����CPU FLASHһ������ ��128KB)
*	��    ��: _ulFlashAddr : Flash��ַ
*	�� �� ֵ: 0 �ɹ��� 1 ʧ��
*			  HAL_OK       = 0x00,
*			  HAL_ERROR    = 0x01,
*			  HAL_BUSY     = 0x02,
*			  HAL_TIMEOUT  = 0x03
*
*********************************************************************************************************
*/
uint8_t bsp_EraseCpuFlash(uint32_t _ulFlashAddr)
{
	uint32_t FirstSector = 0, NbOfSectors = 0;
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SECTORError = 0;
	uint8_t re;

	/* ���� */
	HAL_FLASH_Unlock();
	
	/* ��ȡ�˵�ַ���ڵ����� */
	FirstSector = bsp_GetSector(_ulFlashAddr);
	
	/* �̶�1������ */
	NbOfSectors = 1;	

	/* ������������ */
	EraseInitStruct.TypeErase     = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_3;
	
	if (_ulFlashAddr >= ADDR_FLASH_SECTOR_0_BANK2)
	{
		EraseInitStruct.Banks         = FLASH_BANK_2;
	}
	else
	{
		EraseInitStruct.Banks         = FLASH_BANK_1;
	}
	
	EraseInitStruct.Sector        = FirstSector;
	EraseInitStruct.NbSectors     = NbOfSectors;
	
	/* �������� */	
	re = HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError);
	
	/* ������Ϻ����� */
	HAL_FLASH_Lock();	
	
	return re;
}

/*
*********************************************************************************************************
*	�� �� ��: bsp_WriteCpuFlash
*	����˵��: д���ݵ�CPU �ڲ�Flash�� ���밴32�ֽ�������д����֧�ֿ�������������С128KB. \
*			  д֮ǰ��Ҫ��������. ���Ȳ���32�ֽ�������ʱ����󼸸��ֽ�ĩβ��0д��.
*	��    ��: _ulFlashAddr : Flash��ַ
*			 _ucpSrc : ���ݻ�����
*			 _ulSize : ���ݴ�С����λ���ֽ�, ������32�ֽ���������
*	�� �� ֵ: 0-�ɹ���1-���ݳ��Ȼ��ַ�����2-дFlash����(����Flash������)
*********************************************************************************************************
*/
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
	uint32_t i;
	uint8_t ucRet;

	/* ���ƫ�Ƶ�ַ����оƬ�������򲻸�д��������� */
	if (_ulFlashAddr + _ulSize > CPU_FLASH_BASE_ADDR + CPU_FLASH_SIZE)
	{
		return 1;
	}

	/* ����Ϊ0ʱ����������  */
	if (_ulSize == 0)
	{
		return 0;
	}

	ucRet = bsp_CmpCpuFlash(_ulFlashAddr, _ucpSrc, _ulSize);

	if (ucRet == FLASH_IS_EQU)
	{
		return 0;
	}

	__set_PRIMASK(1);  		/* ���ж� */

	/* FLASH ���� */
	HAL_FLASH_Unlock();

	for (i = 0; i < _ulSize / 32; i++)	
	{
		uint64_t FlashWord[4];
		
		memcpy((char *)FlashWord, _ucpSrc, 32);
		_ucpSrc += 32;
		
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
		{
			_ulFlashAddr = _ulFlashAddr + 32; /* ������������һ��256bit */				
		}		
		else
		{
			goto err;
		}
	}
	
	/* ���Ȳ���32�ֽ������� */
	if (_ulSize % 32)
	{
		uint64_t FlashWord[4];
		
		FlashWord[0] = 0;
		FlashWord[1] = 0;
		FlashWord[2] = 0;
		FlashWord[3] = 0;
		memcpy((char *)FlashWord, _ucpSrc, _ulSize % 32);
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, _ulFlashAddr, (uint64_t)((uint32_t)FlashWord)) == HAL_OK)
		{
			; // _ulFlashAddr = _ulFlashAddr + 32;		
		}		
		else
		{
			goto err;
		}
	}
	
  	/* Flash ��������ֹдFlash���ƼĴ��� */
  	HAL_FLASH_Lock();

  	__set_PRIMASK(0);  		/* ���ж� */

	return 0;
	
err:
  	/* Flash ��������ֹдFlash���ƼĴ��� */
  	HAL_FLASH_Lock();

  	__set_PRIMASK(0);  		/* ���ж� */

	return 1;
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
