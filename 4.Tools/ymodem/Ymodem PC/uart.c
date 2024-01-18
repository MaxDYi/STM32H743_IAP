/*
*********************************************************************************************************
*
*	模块名称 : uart串口收发操作
*	文件名称 : uart.c
*	版    本 : V1.0
*	说    明 : 串口操作
*
*	修改记录
*		版本号    日期         作者        说明
*		V1.0    2022-08-08  Eric2013      首发
*
*	Copyright (C), 2022-2030, 安富莱电子 www.armbbs.cn
*
*********************************************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include "stdint.h"
#include <windows.h>

HANDLE hComm;
OVERLAPPED m_ov;
COMSTAT comstat;
DWORD m_dwCommEvents;

/*
*********************************************************************************************************
*	函 数 名: OpenSerialPort
*	功能说明: 打开串口
*	形    参：PortName 串口COM几
*	返 回 值: 0 失败， 1 成功
*********************************************************************************************************
*/
uint8_t OpenSerialPort(char *PortName)
{
    hComm = CreateFile(PortName,                     /* 串口号 */
                       GENERIC_READ | GENERIC_WRITE, /* 允许读写*/
                       0,                            /* 通讯设备必须以独占方式打开 */
                       0,                            /* 无安全属性 */
                       OPEN_EXISTING,                /* 通讯设备已存在 */
                       FILE_FLAG_OVERLAPPED,         /* 异步I/O */
                       0);                           // 通讯设备不能用模板打开

    if (hComm == INVALID_HANDLE_VALUE)
    {
        CloseHandle(hComm);
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: SetupDcb
*	功能说明: 串口配置
*	形    参：PortName 串口COM几
*	返 回 值: 0 失败， 1 成功
*********************************************************************************************************
*/
uint8_t SetupDcb(int uartrate)
{
    DCB dcb;
    int rate = uartrate;

    memset(&dcb, 0, sizeof(dcb));

    /* 获取当前DCB配置 */
    if (!GetCommState(hComm, &dcb))
    {
        return 0;
    }

    dcb.DCBlength = sizeof(dcb);
    dcb.BaudRate = rate;
    dcb.Parity = NOPARITY;
    dcb.fParity = 0;
    dcb.StopBits = ONESTOPBIT;
    dcb.ByteSize = 8;
    dcb.fOutxCtsFlow = 0;
    dcb.fOutxDsrFlow = 0;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = 0;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutX = 0;
    dcb.fInX = 0;
    dcb.fErrorChar = 0;
    dcb.fBinary = 1;
    dcb.fNull = 0;
    dcb.fAbortOnError = 0;
    dcb.wReserved = 0;
    dcb.XonLim = 2;
    dcb.XoffLim = 4;
    dcb.XonChar = 0x13;
    dcb.XoffChar = 0x19;
    dcb.EvtChar = 0;

    if (!SetCommState(hComm, &dcb))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: SetupTimeout
*	功能说明: 串口配置
*	形    参：----
*	返 回 值: 0 失败， 1 成功
*********************************************************************************************************
*/
uint8_t SetupTimeout(DWORD ReadInterval,
                     DWORD ReadTotalMultiplier,
                     DWORD ReadTotalconstant,
                     DWORD WriteTotalMultiplier,
                     DWORD WriteTotalconstant)
{
    COMMTIMEOUTS timeouts;

    timeouts.ReadIntervalTimeout = ReadInterval;
    timeouts.ReadTotalTimeoutConstant = ReadTotalconstant;
    timeouts.ReadTotalTimeoutMultiplier = ReadTotalMultiplier;
    timeouts.WriteTotalTimeoutConstant = WriteTotalconstant;
    timeouts.WriteTotalTimeoutMultiplier = WriteTotalMultiplier;

    if (!SetCommTimeouts(hComm, &timeouts))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: UART_ReceiveByte
*	功能说明: 接收发送端发来的字符
*	形    参：c  字符
*             timeout  溢出时间
*	返 回 值: 0 接收成功， -1 接收失败
*********************************************************************************************************
*/
uint8_t UART_ReceiveByte(uint8_t *c, uint32_t timeout)
{
    BOOL bRead = TRUE;
    BOOL bResult = TRUE;
    DWORD dwError = 0;
    DWORD BytesRead = 0;
    char RXBuff;
    int cnt = 0;

    timeout = timeout;

    while (1)
    {
        bResult = ClearCommError(hComm, &dwError, &comstat);

        if (comstat.cbInQue == 0)
        {
            Sleep(10);
            if ((timeout--) < 100)
            {
                printf("Receive_Byte failed\r\n");
                return -1;
            }
            continue;
        }

        bResult = ReadFile(hComm,      /* Handle to COMM port */
                           c,          /* RX Buffer Pointer */
                           1,          /* Read one byte */
                           &BytesRead, /* Stores number of bytes read */
                           &m_ov);     /* pointer to the m_ov structure */

        printf("ReceiveByte = 0x%02x\r\n", *c);

        return 0;
    }
}

/*
*********************************************************************************************************
*	函 数 名: UART_ReceivePacket
*	功能说明: 接收发送端发来的字符
*	形    参：data  数据
*             timeout  溢出时间
*	返 回 值: 0 接收成功， -1 接收失败
*********************************************************************************************************
*/
uint8_t UART_ReceivePacket(uint8_t *data, uint16_t length, uint32_t timeout)
{
    BOOL bRead = TRUE;
    BOOL bResult = TRUE;
    DWORD dwError = 0;
    DWORD BytesRead = 0;
    char RXBuff;
    int cnt = 0;

    timeout = timeout;

    while (1)
    {
        bResult = ClearCommError(hComm, &dwError, &comstat);

        if (comstat.cbInQue == 0)
        {
            Sleep(10);
            if ((timeout--) < 100)
            {
                printf("Receive_Byte failed\r\n");
                return -1;
            }
            continue;
        }

        bResult = ReadFile(hComm,      /* Handle to COMM port */
                           data,       /* RX Buffer Pointer */
                           length,     /* Read one byte */
                           &BytesRead, /* Stores number of bytes read */
                           &m_ov);     /* pointer to the m_ov structure */

        printf("ReceivePacket1 = 0x%02x\r\n", data[0]);
        printf("ReceivePacket2 = 0x%02x\r\n", data[1]);

        return 0;
    }
}

/*
*********************************************************************************************************
*	函 数 名: Uart_SendByte
*	功能说明: 发送一个字节数据
*	形    参：c  字符
*	返 回 值: 0
*********************************************************************************************************
*/
void UART_SendByte(uint8_t c)
{
    DWORD BytesSent;

    m_ov.Offset = 0;
    m_ov.OffsetHigh = 0;

    WriteFile(hComm,      /* Handle to COMM Port */
              &c,         /* Pointer to message buffer in calling finction */
              1,          /* Length of message to send */
              &BytesSent, /* Where to store the number of bytes sent */
              &m_ov);     /* Overlapped structure */
}

/*
*********************************************************************************************************
*	函 数 名: UART_SendPacket
*	功能说明: 发送一串数据
*	形    参: data  数据
*             length  数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void UART_SendPacket(uint8_t *data, uint16_t length)
{
    DWORD BytesSent;

    m_ov.Offset = 0;
    m_ov.OffsetHigh = 0;
    WriteFile(hComm,      /* Handle to COMM Port */
              data,       /* Pointer to message buffer in calling finction */
              length,     /* Length of message to send */
              &BytesSent, /* Where to store the number of bytes sent */
              &m_ov);     /* Overlapped structure */
}

/*
*********************************************************************************************************
*	函 数 名: void bsp_UartInit(void)
*	功能说明: 初始化串口
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_UartInit(void)
{
    BOOL OpenStatus;

    OpenStatus = OpenSerialPort("com23");

    if (OpenStatus)
    {
        printf("打开串口号成功\r\n");
    }
    if (SetupDcb(115200))
    {
        printf("建立DCD成功\r\n");
    }

    if (SetupTimeout(0, 0, 0, 0, 0))
    {
        printf("溢出时间设置\r\n");
    }

    /* 当有字符在inbuf中时产生这个事件 */
    SetCommMask(hComm, EV_RXCHAR);

    /* 清除串口的所有操作 */
    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    // UART_SendPacket("1214343", 5);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
