/*
 * @FilePath: \STM32H743_Bootloader\Drivers\ymodem\ymodem.h
 * @Author: MaxDYi
 * @Date: 2024-01-16 20:24:55
 * @LastEditors: MaxDYi
 * @LastEditTime: 2024-01-19 11:16:43
 * @Description:
 */
#ifndef __YMODEM_H__
#define __YMODEM_H__

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "InternalFlash.h"

#define YMODEM_STATE_IDLE 0x00
#define YMODEM_STATE_WAIT_START 0x01
#define YMODEM_STATE_RECEIVE_INFO 0x02
#define YMODEM_STATE_RECEIVE_DATA 0x03
#define YMODEM_STATE_RECEIVE_END 0x04
#define YMODEM_STATE_RECEIVE_STOP 0x05
#define YMODEM_STATE_RECEIVE_FINISH 0x06
#define YMODEM_STATE_ERROR 0xFF

typedef struct Ymodem_Info
{
    uint8_t FrameNumber;
    uint8_t FileName[128];
    uint32_t FileSize;
    uint8_t CRCHigh;
    uint8_t CRCLow;
    uint16_t CRC16;
    uint8_t Error;
} YmodemInfo;

typedef struct Ymodem_Data
{
    uint8_t FrameNumber;
    uint16_t DataSize;
    uint8_t Data[1024];
    uint8_t CRCHigh;
    uint8_t CRCLow;
    uint16_t CRC16;
    uint8_t Error;
} YmodemData;


uint8_t Ymodem_Send(uint8_t *TxBuffer, uint16_t Length, uint32_t Timeout);

uint8_t Ymodem_Receive(uint8_t *RxBuffer, uint16_t Length, uint32_t Timeout);

uint32_t Ymodem_GetFileSaveAddress(void);

void Ymodem_FlashErase(uint32_t StartAddress);

void Ymodem_FlashWrite(uint32_t Address, uint8_t *Data, uint32_t Length);

uint8_t Ymodem_ReceiveFile(uint8_t state);

YmodemInfo Ymodem_ParseInfo(uint8_t *RxBuffer);

YmodemData Ymodem_ParseData(uint8_t *RxBuffer);

uint16_t Ymodem_CalCRC16(uint8_t *data, uint32_t size);

uint16_t Ymodem_UpdateCRC16(uint16_t crcIn, uint8_t byte);

void Ymodem_SendACK(void);

void Ymodem_SendNAK(void);

void Ymodem_SendCAN(void);

void Ymodem_SendC(void);
#endif // __YMODEM_H__