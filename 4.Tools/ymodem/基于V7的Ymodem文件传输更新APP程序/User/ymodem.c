/*
*********************************************************************************************************
*
*	模块名称 : Ymodem协议
*	文件名称 : ymodem.c
*	版    本 : V1.0
*	说    明 : Ymodem协议
*
*	修改记录 :
*		版本号  日期         作者        说明
*		V1.0    2022-08-08  Eric2013     首发         
*
*	Copyright (C), 2022-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "common.h"
#include "ymodem.h"
#include "string.h"
#include "bsp.h"


extern uint8_t FileName[];
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size);

/*
*********************************************************************************************************
*	函 数 名: Receive_Byte
*	功能说明: 接收发送端发来的字符         
*	形    参：c  字符
*             timeout  溢出时间
*	返 回 值: 0 接收成功， -1 接收失败
*********************************************************************************************************
*/
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
	__IO uint32_t count = timeout;
	
	while (count-- > 0)
	{
		if (SerialKeyPressed(c) == 1)
		{
			return 0;
		}
	}
	
	return -1;
}

/*
*********************************************************************************************************
*	函 数 名: Send_Byte
*	功能说明: 发送一个字节数据         
*	形    参：c  字符
*	返 回 值: 0
*********************************************************************************************************
*/
static uint32_t Send_Byte (uint8_t c)
{
	SerialPutChar(c);
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: Receive_Packet
*	功能说明: 接收一包数据        
*	形    参：data 数据
*             length 数据大小
*             timeout  0 传输结束
*                      -1 发送端终止传输
*                      >0 数据包长度
*	返 回 值: 0  正常返回
*             -1 时间溢出或数据包错误
*             1  用户终止
*********************************************************************************************************
*/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
	uint16_t i, packet_size;
	uint8_t c;
	uint16_t crc;
	
	*length = 0;

	
	/* 接收一个字符 */
	if (Receive_Byte(&c, timeout) != 0)
	{
		return -1;
	}
	
	switch (c)
	{
		/* SOH表示数据区有128字节 */
		case SOH:
			packet_size = PACKET_SIZE;
			break;
		
		/* STX表示数据区有1k字节 */
		case STX:
			packet_size = PACKET_1K_SIZE;
			break;
		
		/* 传输结束 end of transmission */
		case EOT:
			return 0;
		
		/* 连续的两个CA信号终止传输 */
		case CA:
			/* 收到两个连续的CA信号 */
			if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
			{
				*length = -1;
				return 0;
			}
			/* 只收到一个CA信号 */
			else
			{
				return -1;
			}
		
		/* 用户终止传输 */
		case ABORT1:
		case ABORT2:
			return 1;
		
		default:
			return -1;
	}
	
	*data = c;
	for (i = 1; i < (packet_size + PACKET_OVERHEAD); i ++)
	{
		if (Receive_Byte(data + i, timeout) != 0)
		{
			return -1;
		}
	}
	
	/* 第PACKET_SEQNO_COMP_INDEX（数字2）字节是PACKET_SEQNO_INDEX（数字1）字节的反码 */
	if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
	{
		return -1;
	}
	
	/* 计算CRC */
	crc = data[ packet_size + PACKET_HEADER ] << 8;
	crc += data[ packet_size + PACKET_HEADER + 1 ];
	if (Cal_CRC16(&data[PACKET_HEADER], packet_size) != crc)
	{
		return -1;
	}	
	
	/* 数据区长度 */
	*length = packet_size;
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: Receive_Packet
*	功能说明: 按照ymodem协议接收数据       
*	形    参: buf 数据首地址
*	返 回 值: 文件大小
*********************************************************************************************************
*/
uint32_t TotalSize = 0;
int32_t Ymodem_Receive (uint8_t *buf, uint32_t appadr)
{
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
	int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
	uint32_t flashdestination, ramsource;
	uint8_t ucState;
	uint32_t SectorCount = 0;
	uint32_t SectorRemain = 0;

	/* 初始化flash编程首地址 */
	flashdestination = appadr;

	/* 接收数据并进行flash编程 */
	for (session_done = 0, errors = 0, session_begin = 0; ;)
	{
		for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
		{
			switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
			{
				/* 返回0表示接收成功 */
				case 0:
					errors = 0;
					switch (packet_length)
					{
						/* 发送端终止传输 */
						case - 1:
							Send_Byte(ACK);
							return 0;
						
						/* 传输结束 */
						case 0:
							Send_Byte(ACK);
							file_done = 1;
							break;
						
						/* 接收数据 */
						default:
							if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
							{
								Send_Byte(NAK);
							}
							else
							{
								if (packets_received == 0)
								{
									/* 文件名数据包 */
									if (packet_data[PACKET_HEADER] != 0)
									{
										/* 读取文件名 */
										for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
										{
											FileName[i++] = *file_ptr++;
										}
										/* 文件名末尾加结束符 */
										FileName[i++] = '\0';
										
										/* 读取文件大小 */
										for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
										{
											file_size[i++] = *file_ptr++;
										}
										file_size[i++] = '\0';
										
										/* 将文件大小的字符串转换成整型数据 */
										Str2Int(file_size, &size);

										
										/* 检测文件大小是否比flash空间大 */
										if (size > (1024*1024*2 + 1))
										{
											/* 终止传输 */
											Send_Byte(CA);
											Send_Byte(CA);
											return -1;
										}

										/* 擦除用户区flash */
										SectorCount = size/(128*1024);
										SectorRemain = size%(128*1024);	
										
										for(i = 0; i < SectorCount; i++)
										{
											bsp_EraseCpuFlash((uint32_t)(flashdestination + i*128*1024));
										}
										
										if(SectorRemain)
										{
											bsp_EraseCpuFlash((uint32_t)(flashdestination + i*128*1024));
										}
										Send_Byte(ACK);
										Send_Byte(CRC16);
									}
									/* 文件名数据包处理完，终止此部分，开始接收数据 */
									else
									{
										Send_Byte(ACK);
										file_done = 1;
										session_done = 1;
										break;
									}
								}
								
								/* 数据包 */
								else
								{
									/* 读取数据 */
									memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
									ramsource = (uint32_t)buf;
									
									/* 扇区编程 */
									ucState = bsp_WriteCpuFlash((uint32_t)(flashdestination + TotalSize),  (uint8_t *)ramsource, packet_length);
									TotalSize += packet_length;
									
									/* 如果返回非0，表示编程失败 */
									if(ucState != 0)
									{
										/* 终止传输 */
										Send_Byte(CA);
										Send_Byte(CA);
										return -2;
									}
									
									Send_Byte(ACK);
								}
								/* 接收数据包递增 */
								packets_received ++;
								session_begin = 1;
							}
					}
					break;
				
				/* 用户终止传输 */
				case 1:
					Send_Byte(CA);
					Send_Byte(CA);
					return -3;
				
				/* 其它 */
				default:
					if (session_begin > 0)
					{
						errors ++;
					}
					
					if (errors > MAX_ERRORS)
					{
						Send_Byte(CA);
						Send_Byte(CA);
						return 0;
					}
					
					Send_Byte(CRC16);
					break;
			}
			
			if (file_done != 0)
			{
				break;
			}
		}
		
		if (session_done != 0)
		{
			break;
		}
	}
	
	return (int32_t)size;
}

/*
*********************************************************************************************************
*	函 数 名: Ymodem_CheckResponse
*	功能说明: 响应      
*	形    参: c 字符
*	返 回 值: 0
*********************************************************************************************************
*/
int32_t Ymodem_CheckResponse(uint8_t c)
{
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: Ymodem_PrepareIntialPacket
*	功能说明: 准备第一包要发送的数据     
*	形    参: data 数据
*             fileName 文件名
*             length   文件大小
*	返 回 值: 0
*********************************************************************************************************
*/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
	uint16_t i, j;
	uint8_t file_ptr[10];

	/* 第一包数据的前三个字符  */
	data[0] = SOH; /* soh表示数据包是128字节 */
	data[1] = 0x00;
	data[2] = 0xff;

	/* 文件名 */
	for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
	{
		data[i + PACKET_HEADER] = fileName[i];
	}

	data[i + PACKET_HEADER] = 0x00;

	/* 文件大小转换成字符 */
	Int2Str (file_ptr, *length);
	for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
	{
		data[i++] = file_ptr[j++];
	}

	/* 其余补0 */
	for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
	{
		data[j] = 0;
	}
}

/*
*********************************************************************************************************
*	函 数 名: Ymodem_PreparePacket
*	功能说明: 准备发送数据包    
*	形    参: SourceBuf 要发送的原数据
*             data      最终要发送的数据包，已经包含的头文件和原数据
*             pktNo     数据包序号
*             sizeBlk   要发送数据数
*	返 回 值: 无
*********************************************************************************************************
*/
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
	uint16_t i, size, packetSize;
	uint8_t* file_ptr;

	/* 设置好要发送数据包的前三个字符data[0]，data[1]，data[2] */
	/* 根据sizeBlk的大小设置数据区数据个数是取1024字节还是取128字节*/
	packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
	/* 数据大小进一步确定 */
	size = sizeBlk < packetSize ? sizeBlk :packetSize;
	
	/* 首字节：确定是1024字节还是用128字节 */
	if (packetSize == PACKET_1K_SIZE)
	{
		data[0] = STX;
	}
	else
	{
		data[0] = SOH;
	}
	
	/* 第2个字节：数据序号 */
	data[1] = pktNo;
	/* 第3个字节：数据序号取反 */
	data[2] = (~pktNo);
	file_ptr = SourceBuf;

	/* 填充要发送的原始数据 */
	for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
	{
		data[i] = *file_ptr++;
	}
	
	/* 不足的补 EOF (0x1A) 或 0x00 */
	if ( size  <= packetSize)
	{
		for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
		{
			data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: UpdateCRC16
*	功能说明: 上次计算的CRC结果 crcIn 再加上一个字节数据计算CRC
*	形    参: crcIn 上一次CRC计算结果
*             byte  新添加字节
*	返 回 值: 无
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
	if(in & 0x100)
		++crc;
	if(crc & 0x10000)
		crc ^= 0x1021;
  }while(!(in & 0x10000));

  return crc & 0xffffu;
}

/*
*********************************************************************************************************
*	函 数 名: Cal_CRC16
*	功能说明: 计算一串数据的CRC
*	形    参: data  数据
*             size  数据长度
*	返 回 值: CRC计算结果
*********************************************************************************************************
*/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
	uint32_t crc = 0;
	const uint8_t* dataEnd = data+size;

	while(data < dataEnd)
		crc = UpdateCRC16(crc, *data++);

	crc = UpdateCRC16(crc, 0);
	crc = UpdateCRC16(crc, 0);

	return crc&0xffffu;
}

/*
*********************************************************************************************************
*	函 数 名: CalChecksum
*	功能说明: 计算一串数据总和
*	形    参: data  数据
*             size  数据长度
*	返 回 值: 计算结果的后8位
*********************************************************************************************************
*/
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t* dataEnd = data+size;

  while(data < dataEnd )
    sum += *data++;

  return (sum & 0xffu);
}

/*
*********************************************************************************************************
*	函 数 名: Ymodem_SendPacket
*	功能说明: 发送一串数据
*	形    参: data  数据
*             length  数据长度
*	返 回 值: 无
*********************************************************************************************************
*/
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
	uint16_t i;
	i = 0;
	
	while (i < length)
	{
		Send_Byte(data[i]);
		i++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: Ymodem_Transmit
*	功能说明: 发送文件
*	形    参: buf  文件数据
*             sendFileName  文件名
*             sizeFile    文件大小
*	返 回 值: 0  文件发送成功
*********************************************************************************************************
*/
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
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

	/* 初始化要发送的第一个数据包 */
	Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  
	do 
	{
		/* 发送数据包 */
		Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* 根据CRC16_F发送CRC或者求和进行校验 */
		if (CRC16_F)
		{
			tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
			Send_Byte(tempCRC >> 8);
			Send_Byte(tempCRC & 0xFF);
		}
		else
		{
			tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
			Send_Byte(tempCheckSum);
		}
  
		/* 等待 Ack 和字符 'C' */
		if (Receive_Byte(&receivedC[0], 10000) == 0)  
		{
			if (receivedC[0] == ACK)
			{ 
				/* 接收到应答 */
				ackReceived = 1;
			}
		}
		/* 没有等到 */
		else
		{
			errors++;
		}
	/* 发送数据包后接收到应答或者没有等到就推出 */
	}while (!ackReceived && (errors < 0x0A));
  
	/* 超过最大错误次数就退出 */
	if (errors >=  0x0A)
	{
		return errors;
	}
	
	buf_ptr = buf;
	size = sizeFile;
	blkNumber = 0x01;
	
	/* 下面使用的是发送1024字节数据包 */
	/* Resend packet if NAK  for a count of 10 else end of communication */
	while (size)
	{
		/* 准备下一包数据 */
		Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
		ackReceived = 0;
		receivedC[0]= 0;
		errors = 0;
		do
		{
			/* 发送下一包数据 */
			if (size >= PACKET_1K_SIZE)
			{
				pktSize = PACKET_1K_SIZE;
			}
			else
			{
				pktSize = PACKET_SIZE;
			}
			
			Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
			
			/* 根据CRC16_F发送CRC校验或者求和的结果 */
			if (CRC16_F)
			{
				tempCRC = Cal_CRC16(&packet_data[3], pktSize);
				Send_Byte(tempCRC >> 8);
				Send_Byte(tempCRC & 0xFF);
			}
			else
			{
				tempCheckSum = CalChecksum (&packet_data[3], pktSize);
				Send_Byte(tempCheckSum);
			}

			/* 等到Ack信号 */
			if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
			{
				ackReceived = 1; 
				/* 修改buf_ptr位置以及size大小，准备发送下一包数据 */
				if (size > pktSize)
				{
					buf_ptr += pktSize;  
					size -= pktSize;
					if (blkNumber == (2*1024*1024/128))
					{
						return 0xFF; /* 错误 */
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
			
		}while(!ackReceived && (errors < 0x0A));
		
		/* 超过10次没有收到应答就退出 */
		if (errors >=  0x0A)
		{
			return errors;
		} 
	}
	
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;
	do 
	{
		Send_Byte(EOT);
		
		/* 发送EOT信号 */
		/* 等待Ack应答 */
		if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
		{
			ackReceived = 1;  
		}
		else
		{
			errors++;
		}
		
	}while (!ackReceived && (errors < 0x0A));
    
	/* 超过10次没有收到应答就退出 */
	if (errors >=  0x0A)
	{
		return errors;
	}
  
	/* 初始化最后一包要发送的数据 */
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;

	packet_data[0] = SOH;
	packet_data[1] = 0;
	packet_data [2] = 0xFF;

	/* 数据包的数据部分全部初始化为0 */
	for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
	{
		packet_data [i] = 0x00;
	}
  
	do 
	{
		/* 发送数据包 */
		Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* 根据CRC16_F发送CRC校验或者求和的结果 */
		tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
		Send_Byte(tempCRC >> 8);
		Send_Byte(tempCRC & 0xFF);

		/* 等待 Ack 和字符 'C' */
		if (Receive_Byte(&receivedC[0], 10000) == 0)  
		{
			if (receivedC[0] == ACK)
			{ 
				/* 数据包发送成功 */
				ackReceived = 1;
			}
		}
		else
		{
			errors++;
		}
	}while (!ackReceived && (errors < 0x0A));

	/* 超过10次没有收到应答就退出 */
	if (errors >=  0x0A)
	{
		return errors;
	}  
  
	do 
	{
		Send_Byte(EOT);
		/* 发送EOT信号 */
		/* 等待Ack应答 */
		if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
		{
			ackReceived = 1;  
		}
		else
		{
			errors++;
		}
	}while (!ackReceived && (errors < 0x0A));

	if (errors >=  0x0A)
	{
		return errors;
	}
	return 0; /* 文件发送成功 */
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
