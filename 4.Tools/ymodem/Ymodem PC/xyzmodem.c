/*
*********************************************************************************************************
*
*	ģ������ : XYZmodemЭ��
*	�ļ����� : xyzmodem.c
*	��    �� : V1.0
*	˵    �� : xyzmodemЭ��
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
#include "uart.c"

/*
*********************************************************************************************************
*	                                   Ymodem�ļ�����Э�����
*********************************************************************************************************
*/
/*
��1�׶Σ� ͬ��
    �ӻ������ݷ���ͬ���ַ� C

��2�׶Σ����͵�1֡���ݣ������ļ������ļ���С
    �������ͣ�
    ---------------------------------------------------------------------------------------
    | SOH  |  ��� - 0x00 |  ���ȡ�� - 0xff | 128�ֽ����ݣ����ļ������ļ���С�ַ���|CRC0 CRC1|
    |-------------------------------------------------------------------------------------|
    �ӻ����գ�
    ���ճɹ��ظ�ACK��CRC16������ʧ�ܣ�У�����������󣩼����ظ��ַ�C������һ������������ظ�����CA����ֹ���䡣

��3�׶Σ����ݴ���
    �������ͣ�
    ---------------------------------------------------------------------------------------
    | SOH/STX  |  ��0x01��ʼ���  |  ���ȡ�� | 128�ֽڻ���1024�ֽ�                |CRC0 CRC1|
    |-------------------------------------------------------------------------------------|
    �ӻ����գ�
    ���ճɹ��ظ�ACK������ʧ�ܣ�У�����������󣩻����û�����ʧ�ܼ����ظ��ַ�C������һ������������ظ�����CA����ֹ���䡣

��4�׶Σ�����֡
    �������ͣ�����EOT�������䡣
    �ӻ����գ��ظ�ACK��

��5�׶Σ���֡������ͨ��
    �������ͣ�һ֡�����ݡ�
    �ӻ����գ��ظ�ACK��
*/

#define SOH (0x01)   /* start of 128-byte data packet */
#define STX (0x02)   /* start of 1024-byte data packet */
#define EOT (0x04)   /* end of transmission */
#define ACK (0x06)   /* acknowledge */
#define NAK (0x15)   /* negative acknowledge */
#define CA (0x18)    /* two of these in succession aborts transfer */
#define CRC16 (0x43) /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1 (0x41) /* 'A' == 0x41, abort by user */
#define ABORT2 (0x61) /* 'a' == 0x61, abort by user */

#define PACKET_SEQNO_INDEX (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER (3)
#define PACKET_TRAILER (2)
#define PACKET_OVERHEAD (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE (128)
#define PACKET_1K_SIZE (1024)

#define FILE_NAME_LENGTH (256)
#define FILE_SIZE_LENGTH (16)

#define NAK_TIMEOUT (0x100000)
#define MAX_ERRORS (5)

/*
*********************************************************************************************************
*	�� �� ��: Int2Str
*	����˵��: ������ת�����ַ�
*	��    ��: str �ַ�  intnum ����
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void Int2Str(uint8_t *str, int32_t intnum)
{
    uint32_t i, Div = 1000000000, j = 0, Status = 0;

    for (i = 0; i < 10; i++)
    {
        str[j++] = (intnum / Div) + 48;

        intnum = intnum % Div;
        Div /= 10;
        if ((str[j - 1] == '0') & (Status == 0))
        {
            j = 0;
        }
        else
        {
            Status++;
        }
    }
}

/*
*********************************************************************************************************
*	�� �� ��: Ymodem_PrepareIntialPacket
*	����˵��: ׼����һ��Ҫ���͵�����
*	��    ��: data ����
*             fileName �ļ���
*             length   �ļ���С
*	�� �� ֵ: 0
*********************************************************************************************************
*/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t *fileName, uint32_t *length)
{
    uint16_t i, j;
    uint8_t file_ptr[10];

    /* ��һ�����ݵ�ǰ�����ַ�  */
    data[0] = SOH; /* soh��ʾ���ݰ���128�ֽ� */
    data[1] = 0x00;
    data[2] = 0xff;

    /* �ļ��� */
    for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH); i++)
    {
        data[i + PACKET_HEADER] = fileName[i];
    }

    data[i + PACKET_HEADER] = 0x00;

    /* �ļ���Сת�����ַ� */
    Int2Str(file_ptr, *length);
    for (j = 0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0';)
    {
        data[i++] = file_ptr[j++];
    }

    /* ���ಹ0 */
    for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
    {
        data[j] = 0;
    }
}

/*
*********************************************************************************************************
*	�� �� ��: Ymodem_PreparePacket
*	����˵��: ׼���������ݰ�
*	��    ��: SourceBuf Ҫ���͵�ԭ����
*             data      ����Ҫ���͵����ݰ����Ѿ�������ͷ�ļ���ԭ����
*             pktNo     ���ݰ����
*             sizeBlk   Ҫ����������
*	�� �� ֵ: ��
*********************************************************************************************************
*/
int sendsize = 0;
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
    uint16_t i, size, packetSize;
    uint8_t *file_ptr;

    /* ���ú�Ҫ�������ݰ���ǰ�����ַ�data[0]��data[1]��data[2] */
    /* ����sizeBlk�Ĵ�С�������������ݸ�����ȡ1024�ֽڻ���ȡ128�ֽ�*/
    packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;

    /* ���ݴ�С��һ��ȷ�� */
    size = sizeBlk < packetSize ? sizeBlk : packetSize;

    /* ���ֽڣ�ȷ����1024�ֽڻ�����128�ֽ� */
    if (packetSize == PACKET_1K_SIZE)
    {
        data[0] = STX;
    }
    else
    {
        data[0] = SOH;
    }

    /* ��2���ֽڣ�������� */
    data[1] = pktNo;
    /* ��3���ֽڣ��������ȡ�� */
    data[2] = (~pktNo);
    file_ptr = SourceBuf;

    /* ���Ҫ���͵�ԭʼ���� */
    for (i = PACKET_HEADER; i < size + PACKET_HEADER; i++)
    {
        data[i] = *file_ptr++;
    }

    /* ����Ĳ� EOF (0x1A) �� 0x00 */
    if (size <= packetSize)
    {
        for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
        {
            data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
        }
    }

    sendsize += size;
    printf("SendSize = %d\r\n", sendsize);
}

/*
*********************************************************************************************************
*	�� �� ��: UpdateCRC16
*	����˵��: �ϴμ����CRC��� crcIn �ټ���һ���ֽ����ݼ���CRC
*	��    ��: crcIn ��һ��CRC������
*             byte  ������ֽ�
*	�� �� ֵ: ��
*********************************************************************************************************
*/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte | 0x100;

    do
    {
        crc <<= 1;
        in <<= 1;
        if (in & 0x100)
            ++crc;
        if (crc & 0x10000)
            crc ^= 0x1021;
    } while (!(in & 0x10000));

    return crc & 0xffffu;
}

/*
*********************************************************************************************************
*	�� �� ��: Cal_CRC16
*	����˵��: ����һ�����ݵ�CRC
*	��    ��: data  ����
*             size  ���ݳ���
*	�� �� ֵ: CRC������
*********************************************************************************************************
*/
uint16_t Cal_CRC16(const uint8_t *data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t *dataEnd = data + size;

    while (data < dataEnd)
        crc = UpdateCRC16(crc, *data++);

    crc = UpdateCRC16(crc, 0);
    crc = UpdateCRC16(crc, 0);

    return crc & 0xffffu;
}

/*
*********************************************************************************************************
*	�� �� ��: CalChecksum
*	����˵��: ����һ�������ܺ�
*	��    ��: data  ����
*             size  ���ݳ���
*	�� �� ֵ: �������ĺ�8λ
*********************************************************************************************************
*/
uint8_t CalChecksum(const uint8_t *data, uint32_t size)
{
    uint32_t sum = 0;
    const uint8_t *dataEnd = data + size;

    while (data < dataEnd)
        sum += *data++;

    return (sum & 0xffu);
}

/*
*********************************************************************************************************
*	�� �� ��: Ymodem_Transmit
*	����˵��: �����ļ�
*	��    ��: buf  �ļ�����
*             sendFileName  �ļ���
*             sizeFile    �ļ���С
*	�� �� ֵ: 0  �ļ����ͳɹ�
*********************************************************************************************************
*/
uint8_t Ymodem_Transmit(uint8_t *buf, const uint8_t *sendFileName, uint32_t sizeFile)
{
    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t filename[FILE_NAME_LENGTH];
    uint8_t *buf_ptr, tempCheckSum;
    uint16_t tempCRC;
    uint16_t blkNumber;
    uint8_t receivedC[2], CRC16_F = 0, i;
    uint32_t errors, ackReceived, size = 0, pktSize;

    errors = 0;
    ackReceived = 0;
    for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
    {
        filename[i] = sendFileName[i];
    }

    CRC16_F = 1;

    printf("���͵ĵ�һ�����ݰ�\r\n");

    /* ��ʼ��Ҫ���͵ĵ�һ�����ݰ� */
    Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);

    do
    {
        /* �������ݰ� */
        UART_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

        /* ����CRC16_F����CRC������ͽ���У�� */
        if (CRC16_F)
        {
            tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
            UART_SendByte(tempCRC >> 8);
            UART_SendByte(tempCRC & 0xFF);
        }
        else
        {
            tempCheckSum = CalChecksum(&packet_data[3], PACKET_SIZE);
            UART_SendByte(tempCheckSum);
        }

        /* �ȴ� Ack ���ַ� 'C' */
        if (UART_ReceivePacket(&receivedC[0], 2, 10000) == 0)
        {
            if ((receivedC[0] == ACK) && (receivedC[1] == CRC16))
            {
                /* ���յ�Ӧ�� */
                ackReceived = 1;
            }
        }
        /* û�еȵ� */
        else
        {
            errors++;
        }
        /* �������ݰ�����յ�Ӧ�����û�еȵ����Ƴ� */
    } while (!ackReceived && (errors < 0x0A));

    /* ����������������˳� */
    if (errors >= 0x0A)
    {
        return errors;
    }

    buf_ptr = buf;
    size = sizeFile;
    blkNumber = 0x01;

    /* ����ʹ�õ��Ƿ���1024�ֽ����ݰ� */
    /* Resend packet if NAK  for a count of 10 else end of communication */
    while (size)
    {
        /* ׼����һ������ */
        Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
        ackReceived = 0;
        receivedC[0] = 0;
        errors = 0;
        do
        {
            /* ������һ������ */
            if (size >= PACKET_1K_SIZE)
            {
                pktSize = PACKET_1K_SIZE;
            }
            else
            {
                pktSize = PACKET_SIZE;
            }

            UART_SendPacket(packet_data, pktSize + PACKET_HEADER);

            /* ����CRC16_F����CRCУ�������͵Ľ�� */
            if (CRC16_F)
            {
                tempCRC = Cal_CRC16(&packet_data[3], pktSize);
                UART_SendByte(tempCRC >> 8);
                UART_SendByte(tempCRC & 0xFF);
            }
            else
            {
                tempCheckSum = CalChecksum(&packet_data[3], pktSize);
                UART_SendByte(tempCheckSum);
            }

            /* �ȵ�Ack�ź� */
            if ((UART_ReceiveByte(&receivedC[0], 100000) == 0) && (receivedC[0] == ACK))
            {
                ackReceived = 1;
                /* �޸�buf_ptrλ���Լ�size��С��׼��������һ������ */
                if (size > pktSize)
                {
                    buf_ptr += pktSize;
                    size -= pktSize;
                    if (blkNumber == ((2 * 1024 * 1024) / 128))
                    {
                        return 0xFF; /* ���� */
                    }
                    else
                    {
                        blkNumber++;
                    }
                }
                else
                {
                    buf_ptr += pktSize;
                    size = 0;
                }
            }
            else
            {
                errors++;
            }

        } while (!ackReceived && (errors < 0x0A));

        /* ����10��û���յ�Ӧ����˳� */
        if (errors >= 0x0A)
        {
            return errors;
        }
    }

    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;
    do
    {
        UART_SendByte(EOT);

        /* ����EOT�ź� */
        /* �ȴ�AckӦ�� */
        if ((UART_ReceiveByte(&receivedC[0], 10000) == 0))
        {
            if (receivedC[0] == ACK)
            {
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }

    } while (!ackReceived && (errors < 0x0A));

    if (errors >= 0x0A)
    {
        return errors;
    }

    printf("���ͽ����ź�\r\n");

#if 1
    /* ��ʼ�����һ��Ҫ���͵����� */
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;

    packet_data[0] = SOH;
    packet_data[1] = 0;
    packet_data[2] = 0xFF;

    /* ���ݰ������ݲ���ȫ����ʼ��Ϊ0 */
    for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
    {
        packet_data[i] = 0x00;
    }

    do
    {
        /* �������ݰ� */
        UART_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

        /* ����CRC16_F����CRCУ�������͵Ľ�� */
        tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
        UART_SendByte(tempCRC >> 8);
        UART_SendByte(tempCRC & 0xFF);

        /* �ȴ� Ack */
        if (UART_ReceiveByte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                /* ���ݰ����ͳɹ� */
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    printf("�������\r\n");

    /* ����10��û���յ�Ӧ����˳� */
    if (errors >= 0x0A)
    {
        return errors;
    }
#endif

    return 0; /* �ļ����ͳɹ� */
}

/*
*********************************************************************************************************
*	�� �� ��: xymodem_send
*	����˵��: �����ļ�
*	��    ��: filename  �ļ���
*	�� �� ֵ: 0  �ļ����ͳɹ�
*********************************************************************************************************
*/
int xymodem_send(const char *filename)
{
    size_t len;
    int ret, fd;
    FILE *fin, *fout;
    int skip_payload = 0;

    int size = 0;
    int bw = 0;
    int readcount = 0, remain = 0;

    fin = fopen(filename, "rb");
    if (fin != NULL)
    {
        /* �ļ��򿪳ɹ� */
        printf("���ļ��ɹ�\r\n");
    }
    else
    {
        printf("���ļ�ʧ��, �����ļ�������\r\n");
    }

    fseek(fin, 0, SEEK_END);
    size = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    printf("�ļ���С = %d\r\n", size);

    char *buf = malloc(size);

    do
    {
        bw = fread(&buf[readcount], 1, size, fin);
        readcount += bw;
    } while (readcount < size);

    printf("readcount = %d\r\n", readcount);
    fclose(fin);

    Ymodem_Transmit(buf, "app.bin", size);

    return 0;
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
