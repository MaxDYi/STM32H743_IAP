/*
*********************************************************************************************************
*
*	模块名称 : Ymodem协议
*	文件名称 : ymodem.c
*	版    本 : V1.0
*	说    明 : 头文件
*
*	修改记录 :
*		版本号  日期         作者        说明
*		V1.0    2022-08-08  Eric2013     首发         
*
*	Copyright (C), 2022-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#ifndef __YMODEM_H_
#define __YMODEM_H_

#define PACKET_SEQNO_INDEX      (1)
#define PACKET_SEQNO_COMP_INDEX (2)

#define PACKET_HEADER           (3)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (64)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* two of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x100000*5)
#define MAX_ERRORS              (5)

/* 供外部文件调用的函数 */
int32_t Ymodem_Receive (uint8_t *buf, uint32_t appadr);
uint8_t Ymodem_Transmit (uint8_t *,const  uint8_t* , uint32_t );

#endif  /* __YMODEM_H_ */

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
