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
*	模块名称 : 主函数
*	文件名称 : test.c
*	版    本 : V1.0
*	说    明 : 主功能
*
*	修改记录
*		版本号    日期         作者      说明
*		V1.0    2022-08-08  Eric2013    首发
*
*	Copyright (C), 2022-2030, 安富莱电子 www.armbbs.cn
*
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include <windows.h>
#include "xyzmodem.c"

/* 源文件路径 */
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

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
