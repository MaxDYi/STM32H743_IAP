/*
 * @FilePath: \Ymodem PC\test.c
 * @Author: MaxDYi
 * @Date: 2024-01-16 13:59:45
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-17 21:01:48
 * @Description:
 */
/*
*********************************************************************************************************
*
*	ģ������ : ������
*	�ļ����� : test.c
*	��    �� : V1.0
*	˵    �� : ������
*
*	�޸ļ�¼
*		�汾��    ����         ����      ˵��
*		V1.0    2022-08-08  Eric2013    �׷�
*
*	Copyright (C), 2022-2030, ���������� www.armbbs.cn
*
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include <windows.h>
#include "xyzmodem.c"

/* Դ�ļ�·�� */
char SourceFile[] =
    "E:\\STM32H743_IAP\\1.Firmware\\STM32H743_Bootloader\\EWARM\\STM32H743_Bootloader\\Exe\\STM32H743_Bootloader.bin";
int main()
{

    bsp_UartInit();

    xymodem_send(SourceFile);

    while (1)
    {
    }
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
