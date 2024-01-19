/*
 * @FilePath: \STM32H743_Bootloader\Drivers\ymodem\Ymodem.c
 * @Author: MaxDYi
 * @Date: 2024-01-16 20:24:55
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-19 11:34:41
 * @Description:Ymodem协议实现
 */

#include "ymodem.h"
#include "usart.h"
#include "crc.h"

#define YMODEM_SOH 0x01 // 128字节长度包
#define YMODEM_STX 0x02 // 1024字节长度包
#define YMODEM_EOT 0x04 // 文件传输结束命令
#define YMODEM_ACK 0x06 // 接受正确应答命令
#define YMODEM_NAK 0x15 // 重传当前数据包请求命令
#define YMODEM_CAN 0x18 // 取消传输命令，连发5个该命令
#define YMODEM_C 0x43   // 请求数据包

#define YMODEM_CRC_ENABLE 1

#define YMODEM_TX_TIMEOUT 0xFF
#define YMODEM_RX_TIMEOUT 0xFFFF
#define YMODEM_OK 1
#define YMODEM_ERROR 0

#define YMODEM_BLOCK_SIZE 1024
#define YMODEM_BUFFER_SIZE (1024 + 5)

#define FLASH_ERASE_SIZE 0x00020000

#define YMODEM_ERROR_NONE 0
#define YMODEM_ERROR_CRC 1
#define YMODEM_ERROR_HEAD 2
#define YMODEM_ERROR_FRAME_NUM 3
#define YMODEM_ERROR_FILE_NAME 4
#define YMODEM_ERROR_FILE_SIZE 5

uint8_t Ymodem_State = YMODEM_STATE_IDLE;
YmodemInfo yinfo;
YmodemData ydata;
uint8_t Ymodem_RxBuffer[YMODEM_BUFFER_SIZE];
uint8_t Ymodem_TxBuffer[YMODEM_BUFFER_SIZE];

/**
 * @description:发送数据
 * @param {uint8_t} *TxBuffer
 * @param {uint16_t} Length
 * @param {uint32_t} Timeout
 * @return {数据发送状态}
 */
uint8_t Ymodem_Send(uint8_t *TxBuffer, uint16_t Length, uint32_t Timeout)
{
    if (HAL_UART_Transmit(&huart1, TxBuffer, Length, Timeout) == HAL_OK)
    {
        return YMODEM_OK;
    }
    else
    {
        return YMODEM_ERROR;
    }
}

/**
 * @description:接收数据
 * @param {uint8_t} *RxBuffer
 * @param {uint16_t} Length
 * @param {uint32_t} Timeout
 * @return {数据接收状态}
 */
uint8_t Ymodem_Receive(uint8_t *RxBuffer, uint16_t Length, uint32_t Timeout)
{
    if (HAL_UART_Receive(&huart1, RxBuffer, Length, Timeout) == HAL_OK)
    {
        return YMODEM_OK;
    }
    else
    {
        return YMODEM_ERROR;
    }
}

/**
 * @description:获取文件保存地址
 * @return {文件保存地址}
 */
uint32_t Ymodem_GetFileSaveAddress(void)
{
    uint32_t Address = 0x08100000;
    return Address;
}

/**
 * @description: 擦除Flash
 * @param {uint32_t} StartAddress
 * @return {*}
 */
void Ymodem_FlashErase(uint32_t StartAddress)
{
    EraseCpuFlash(StartAddress);
}

/**
 * @description: 文件写入Flash
 * @param {uint32_t} Address
 * @param {uint8_t} *Data
 * @param {uint32_t} Length
 * @return {*}
 */
void Ymodem_FlashWrite(uint32_t Address, uint8_t *Data, uint32_t Length)
{
    WriteCpuFlash(Address, Data, Length);
}

/**
 * @description: Ymodem接收文件
 * @param {uint8_t} state
 * @return {*}
 */
uint8_t Ymodem_ReceiveFile(uint8_t state)
{

    static uint32_t PackNum = 0;
    static uint32_t ReceiveFileSize = 0;
    static uint32_t FlashWriteAddress = 0;
    static uint8_t errCount = 0;
    Ymodem_State = state;
    switch (Ymodem_State)
    {
    case (YMODEM_STATE_IDLE): // 空闲
    {
        errCount = 0;
        Ymodem_State = YMODEM_STATE_WAIT_START;
        memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
        memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
        break;
    }
    case (YMODEM_STATE_WAIT_START): // 发送开始字符
    {
        errCount = 0;
        memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
        memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
        PackNum = 0;
        ReceiveFileSize = 0;
        Ymodem_State = YMODEM_STATE_RECEIVE_INFO;
        Ymodem_SendC();
        break;
    }
    case (YMODEM_STATE_RECEIVE_INFO): // 接收起始帧
    {
        if (Ymodem_Receive(Ymodem_RxBuffer, 3 + 128 + 2, YMODEM_RX_TIMEOUT) == YMODEM_OK)
        {
            if (Ymodem_RxBuffer[0] == YMODEM_STX)
            {
                Ymodem_Receive(&Ymodem_RxBuffer[3 + 128 + 2], 1024 - 128, YMODEM_RX_TIMEOUT);
            }
            yinfo = Ymodem_ParseInfo(Ymodem_RxBuffer);
            if (yinfo.Error == YMODEM_ERROR_NONE)
            {
                uint8_t FlashEraseNum = yinfo.FileSize / FLASH_ERASE_SIZE + 1;
                FlashWriteAddress = Ymodem_GetFileSaveAddress();
                for (uint8_t i = 0; i < FlashEraseNum; i++)
                {
                    Ymodem_FlashErase(FlashWriteAddress + i * FLASH_ERASE_SIZE);
                }
                memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
                memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
                Ymodem_State = YMODEM_STATE_RECEIVE_DATA;
                Ymodem_SendACK();
                Ymodem_SendC();
            }
            else
            {
                errCount++;
                if (errCount >= 5)
                {
                    Ymodem_State = YMODEM_STATE_ERROR;
                }
                Ymodem_SendNAK();
            }
        }
        break;
    }
    case (YMODEM_STATE_RECEIVE_DATA): // 接收数据帧
    {
        if (Ymodem_Receive(Ymodem_RxBuffer, YMODEM_BUFFER_SIZE, YMODEM_RX_TIMEOUT) == YMODEM_OK)
        {
            ydata = Ymodem_ParseData(Ymodem_RxBuffer);
            if (ydata.Error == YMODEM_ERROR_NONE)
            {
                PackNum++;
                ReceiveFileSize = ReceiveFileSize + ydata.DataSize;

                Ymodem_FlashWrite(FlashWriteAddress, ydata.Data, ydata.DataSize);
                FlashWriteAddress = FlashWriteAddress + ydata.DataSize;
                Ymodem_SendACK();

                if (ReceiveFileSize >= yinfo.FileSize) // 判断是最后一包
                {
                    Ymodem_State = YMODEM_STATE_RECEIVE_END;
                }
                memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
                memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
            }
            else
            {
                memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
                memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
                errCount++;
                if (errCount >= 5)
                {
                    Ymodem_State = YMODEM_STATE_ERROR;
                }
                Ymodem_SendNAK();
            }
        }
        break;
    }
    case (YMODEM_STATE_RECEIVE_END):
    {
        if (Ymodem_Receive(Ymodem_RxBuffer, 1, YMODEM_RX_TIMEOUT) == YMODEM_OK)
        {
            if (Ymodem_RxBuffer[0] == YMODEM_EOT)
            {
                Ymodem_SendNAK();
                Ymodem_State = YMODEM_STATE_RECEIVE_STOP;
            }
            else
            {
                Ymodem_State = YMODEM_STATE_ERROR;
            }
        }
        break;
    }
    case (YMODEM_STATE_RECEIVE_STOP):
    {
        if (Ymodem_Receive(Ymodem_RxBuffer, 1, YMODEM_RX_TIMEOUT) == YMODEM_OK)
        {
            if (Ymodem_RxBuffer[0] == YMODEM_EOT)
            {
                Ymodem_SendACK();
                Ymodem_SendC();
                if (Ymodem_Receive(Ymodem_RxBuffer, 133, YMODEM_RX_TIMEOUT) == YMODEM_OK)
                {
                    Ymodem_SendACK();
                    Ymodem_SendCAN();
                    Ymodem_SendCAN();
                    Ymodem_SendCAN();
                    Ymodem_SendCAN();
                    Ymodem_SendCAN();
                }
                Ymodem_State = YMODEM_STATE_RECEIVE_FINISH;
            }
            else
            {
                Ymodem_State = YMODEM_STATE_ERROR;
            }
        }
        break;
    }
    case (YMODEM_STATE_RECEIVE_FINISH):
    {
        Ymodem_State = YMODEM_STATE_IDLE;
        break;
    }
    case (YMODEM_STATE_ERROR):
    {
        memset(Ymodem_RxBuffer, 0, sizeof(Ymodem_RxBuffer));
        memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
        errCount = 0;
        PackNum = 0;
        ReceiveFileSize = 0;
        Ymodem_State = YMODEM_STATE_IDLE;
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        break;
    }
    default:
    {
        errCount = 0;
        PackNum = 0;
        ReceiveFileSize = 0;
        Ymodem_State = YMODEM_STATE_ERROR;
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        Ymodem_SendCAN();
        break;
    }
    }
    return Ymodem_State;
}

/**
 * @description: 解析起始帧
 * @param {uint8_t} *RxBuffer
 * @return {起始帧信息}
 */
YmodemInfo Ymodem_ParseInfo(uint8_t *RxBuffer)
{
    YmodemInfo Y_Info;
    if (RxBuffer[0] == YMODEM_SOH || RxBuffer[0] == YMODEM_STX)
    {
        uint16_t FrameLength = 0;
        if (RxBuffer[0] == YMODEM_SOH)
        {
            FrameLength = 128;
        }
        else
        {
            FrameLength = 1024;
        }
        Y_Info.FrameNumber = RxBuffer[1];
        if (Y_Info.FrameNumber == 0x00)
        {
            uint8_t i = 3;
            memset(Y_Info.FileName, 0, sizeof(Y_Info.FileName));
            while (RxBuffer[i] != 0x00)
            {
                Y_Info.FileName[i - 3] = RxBuffer[i];
                i++;
                if (i >= 128 + 3)
                {
                    Y_Info.Error = YMODEM_ERROR_FILE_NAME;
                    return Y_Info;
                }
            }
            Y_Info.FileName[i - 3] = '\0';
            i++;
            Y_Info.FileSize = 0;
            while (RxBuffer[i] >= '0' && RxBuffer[i] <= '9')
            {
                Y_Info.FileSize = Y_Info.FileSize * 10 + RxBuffer[i] - '0';
                i++;
                if (i >= 128 + 3)
                {
                    Y_Info.Error = YMODEM_ERROR_FILE_SIZE;
                    return Y_Info;
                }
            }

            Y_Info.CRCHigh = RxBuffer[FrameLength + 3];
            Y_Info.CRCLow = RxBuffer[FrameLength + 3 + 1];
            Y_Info.CRC16 = Y_Info.CRCHigh << 8 | Y_Info.CRCLow;

            if (YMODEM_CRC_ENABLE)
            {
                uint16_t crc16 = Ymodem_CalCRC16(&RxBuffer[3], FrameLength);
                if (crc16 != Y_Info.CRC16)
                {
                    Y_Info.Error = YMODEM_ERROR_CRC;
                }
                else
                {
                    Y_Info.Error = YMODEM_ERROR_NONE;
                }
            }
            else
            {
                Y_Info.Error = YMODEM_ERROR_NONE;
            }
            return Y_Info;
        }
        else
        {
            Y_Info.Error = YMODEM_ERROR_FRAME_NUM;
            return Y_Info;
        }
    }
    else
    {
        Y_Info.Error = YMODEM_ERROR_HEAD;
        return Y_Info;
    }
}

/**
 * @description: 解析数据帧
 * @param {uint8_t} *RxBuffer
 * @return {数据帧信息}
 */
YmodemData Ymodem_ParseData(uint8_t *RxBuffer)
{
    YmodemData Y_Data;
    if (RxBuffer[0] == YMODEM_STX || RxBuffer[0] == YMODEM_SOH)
    {
        if (RxBuffer[0] == YMODEM_STX)
        {
            Y_Data.DataSize = 1024;
        }
        else
        {
            Y_Data.DataSize = 128;
        }
        if (RxBuffer[1] + RxBuffer[2] == 0xFF)
        {
            Y_Data.FrameNumber = RxBuffer[1];
            for (uint16_t i = 0; i < Y_Data.DataSize; i++)
            {
                Y_Data.Data[i] = RxBuffer[3 + i];
            }
            Y_Data.CRCHigh = RxBuffer[3 + Y_Data.DataSize];
            Y_Data.CRCLow = RxBuffer[3 + Y_Data.DataSize + 1];
            Y_Data.CRC16 = Y_Data.CRCHigh << 8 | Y_Data.CRCLow;
            if (YMODEM_CRC_ENABLE)
            {
                uint16_t crc16 = Ymodem_CalCRC16(&RxBuffer[3], Y_Data.DataSize);
                if (crc16 != Y_Data.CRC16)
                {
                    Y_Data.Error = YMODEM_ERROR_CRC;
                }
                else
                {
                    Y_Data.Error = YMODEM_ERROR_NONE;
                }
            }
            else
            {
                Y_Data.Error = YMODEM_ERROR_NONE;
            }
            return Y_Data;
        }
        else
        {
            Y_Data.Error = YMODEM_ERROR_FRAME_NUM;
            return Y_Data;
        }
    }
    else
    {
        Y_Data.Error = YMODEM_ERROR_HEAD;
        return Y_Data;
    }
}

/**
 * @description: CRC16计算
 * @param {uint8_t} *data
 * @param {uint32_t} size
 * @return {CRC16计算结果}
 */
uint16_t Ymodem_CalCRC16(uint8_t *data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t *dataEnd = data + size;

    while (data < dataEnd)
        crc = Ymodem_UpdateCRC16(crc, *data++);

    crc = Ymodem_UpdateCRC16(crc, 0);
    crc = Ymodem_UpdateCRC16(crc, 0);

    return crc & 0xffffu;
}

/**
 * @description: 更新CRC16
 * @param {uint16_t} crcIn
 * @param {uint8_t} byte
 * @return {CRC16计算结果}
 */
uint16_t Ymodem_UpdateCRC16(uint16_t crcIn, uint8_t byte)
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

/**
 * @description: 发送ACK
 * @return {*}
 */
void Ymodem_SendACK(void)
{
    memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
    sprintf((char *)Ymodem_TxBuffer, "%c", YMODEM_ACK);
    Ymodem_Send(Ymodem_TxBuffer, 1, YMODEM_TX_TIMEOUT);
}

/**
 * @description: 发送NAK
 * @return {*}
 */
void Ymodem_SendNAK(void)
{
    memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
    sprintf((char *)Ymodem_TxBuffer, "%c", YMODEM_NAK);
    Ymodem_Send(Ymodem_TxBuffer, 1, YMODEM_TX_TIMEOUT);
}

/**
 * @description: 发送CAN
 * @return {*}
 */
void Ymodem_SendCAN(void)
{
    memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
    sprintf((char *)Ymodem_TxBuffer, "%c", YMODEM_CAN);
    Ymodem_Send(Ymodem_TxBuffer, 1, YMODEM_TX_TIMEOUT);
}

/**
 * @description: 发送C
 * @return {*}
 */
void Ymodem_SendC(void)
{
    memset(Ymodem_TxBuffer, 0, sizeof(Ymodem_TxBuffer));
    sprintf((char *)Ymodem_TxBuffer, "%c", YMODEM_C);
    Ymodem_Send(Ymodem_TxBuffer, 1, YMODEM_TX_TIMEOUT);
}
