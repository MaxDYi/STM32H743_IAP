/*
*********************************************************************************************************
*
*	ģ������ : uart�����շ�����
*	�ļ����� : uart.c
*	��    �� : V1.0
*	˵    �� : ���ڲ���
*
*	�޸ļ�¼
*		�汾��    ����         ����        ˵��
*		V1.0    2022-08-08  Eric2013      �׷�
*
*	Copyright (C), 2022-2030, ���������� www.armbbs.cn
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
*	�� �� ��: OpenSerialPort
*	����˵��: �򿪴���
*	��    �Σ�PortName ����COM��
*	�� �� ֵ: 0 ʧ�ܣ� 1 �ɹ�
*********************************************************************************************************
*/
uint8_t OpenSerialPort(char *PortName)
{
    hComm = CreateFile(PortName,                     /* ���ں� */
                       GENERIC_READ | GENERIC_WRITE, /* �����д*/
                       0,                            /* ͨѶ�豸�����Զ�ռ��ʽ�� */
                       0,                            /* �ް�ȫ���� */
                       OPEN_EXISTING,                /* ͨѶ�豸�Ѵ��� */
                       FILE_FLAG_OVERLAPPED,         /* �첽I/O */
                       0);                           // ͨѶ�豸������ģ���

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
*	�� �� ��: SetupDcb
*	����˵��: ��������
*	��    �Σ�PortName ����COM��
*	�� �� ֵ: 0 ʧ�ܣ� 1 �ɹ�
*********************************************************************************************************
*/
uint8_t SetupDcb(int uartrate)
{
    DCB dcb;
    int rate = uartrate;

    memset(&dcb, 0, sizeof(dcb));

    /* ��ȡ��ǰDCB���� */
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
*	�� �� ��: SetupTimeout
*	����˵��: ��������
*	��    �Σ�----
*	�� �� ֵ: 0 ʧ�ܣ� 1 �ɹ�
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
*	�� �� ��: UART_ReceiveByte
*	����˵��: ���շ��Ͷ˷������ַ�
*	��    �Σ�c  �ַ�
*             timeout  ���ʱ��
*	�� �� ֵ: 0 ���ճɹ��� -1 ����ʧ��
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
*	�� �� ��: UART_ReceivePacket
*	����˵��: ���շ��Ͷ˷������ַ�
*	��    �Σ�data  ����
*             timeout  ���ʱ��
*	�� �� ֵ: 0 ���ճɹ��� -1 ����ʧ��
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
*	�� �� ��: Uart_SendByte
*	����˵��: ����һ���ֽ�����
*	��    �Σ�c  �ַ�
*	�� �� ֵ: 0
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
*	�� �� ��: UART_SendPacket
*	����˵��: ����һ������
*	��    ��: data  ����
*             length  ���ݳ���
*	�� �� ֵ: ��
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
*	�� �� ��: void bsp_UartInit(void)
*	����˵��: ��ʼ������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_UartInit(void)
{
    BOOL OpenStatus;

    OpenStatus = OpenSerialPort("com23");

    if (OpenStatus)
    {
        printf("�򿪴��ںųɹ�\r\n");
    }
    if (SetupDcb(115200))
    {
        printf("����DCD�ɹ�\r\n");
    }

    if (SetupTimeout(0, 0, 0, 0, 0))
    {
        printf("���ʱ������\r\n");
    }

    /* �����ַ���inbuf��ʱ��������¼� */
    SetCommMask(hComm, EV_RXCHAR);

    /* ������ڵ����в��� */
    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    // UART_SendPacket("1214343", 5);
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
