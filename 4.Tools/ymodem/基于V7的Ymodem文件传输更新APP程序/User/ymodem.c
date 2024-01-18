/*
*********************************************************************************************************
*
*	ģ������ : YmodemЭ��
*	�ļ����� : ymodem.c
*	��    �� : V1.0
*	˵    �� : YmodemЭ��
*
*	�޸ļ�¼ :
*		�汾��  ����         ����        ˵��
*		V1.0    2022-08-08  Eric2013     �׷�         
*
*	Copyright (C), 2022-2030, ���������� www.armfly.com
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
*	�� �� ��: Receive_Byte
*	����˵��: ���շ��Ͷ˷������ַ�         
*	��    �Σ�c  �ַ�
*             timeout  ���ʱ��
*	�� �� ֵ: 0 ���ճɹ��� -1 ����ʧ��
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
*	�� �� ��: Send_Byte
*	����˵��: ����һ���ֽ�����         
*	��    �Σ�c  �ַ�
*	�� �� ֵ: 0
*********************************************************************************************************
*/
static uint32_t Send_Byte (uint8_t c)
{
	SerialPutChar(c);
	return 0;
}

/*
*********************************************************************************************************
*	�� �� ��: Receive_Packet
*	����˵��: ����һ������        
*	��    �Σ�data ����
*             length ���ݴ�С
*             timeout  0 �������
*                      -1 ���Ͷ���ֹ����
*                      >0 ���ݰ�����
*	�� �� ֵ: 0  ��������
*             -1 ʱ����������ݰ�����
*             1  �û���ֹ
*********************************************************************************************************
*/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
	uint16_t i, packet_size;
	uint8_t c;
	uint16_t crc;
	
	*length = 0;

	
	/* ����һ���ַ� */
	if (Receive_Byte(&c, timeout) != 0)
	{
		return -1;
	}
	
	switch (c)
	{
		/* SOH��ʾ��������128�ֽ� */
		case SOH:
			packet_size = PACKET_SIZE;
			break;
		
		/* STX��ʾ��������1k�ֽ� */
		case STX:
			packet_size = PACKET_1K_SIZE;
			break;
		
		/* ������� end of transmission */
		case EOT:
			return 0;
		
		/* ����������CA�ź���ֹ���� */
		case CA:
			/* �յ�����������CA�ź� */
			if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
			{
				*length = -1;
				return 0;
			}
			/* ֻ�յ�һ��CA�ź� */
			else
			{
				return -1;
			}
		
		/* �û���ֹ���� */
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
	
	/* ��PACKET_SEQNO_COMP_INDEX������2���ֽ���PACKET_SEQNO_INDEX������1���ֽڵķ��� */
	if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
	{
		return -1;
	}
	
	/* ����CRC */
	crc = data[ packet_size + PACKET_HEADER ] << 8;
	crc += data[ packet_size + PACKET_HEADER + 1 ];
	if (Cal_CRC16(&data[PACKET_HEADER], packet_size) != crc)
	{
		return -1;
	}	
	
	/* ���������� */
	*length = packet_size;
	return 0;
}

/*
*********************************************************************************************************
*	�� �� ��: Receive_Packet
*	����˵��: ����ymodemЭ���������       
*	��    ��: buf �����׵�ַ
*	�� �� ֵ: �ļ���С
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

	/* ��ʼ��flash����׵�ַ */
	flashdestination = appadr;

	/* �������ݲ�����flash��� */
	for (session_done = 0, errors = 0, session_begin = 0; ;)
	{
		for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
		{
			switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))
			{
				/* ����0��ʾ���ճɹ� */
				case 0:
					errors = 0;
					switch (packet_length)
					{
						/* ���Ͷ���ֹ���� */
						case - 1:
							Send_Byte(ACK);
							return 0;
						
						/* ������� */
						case 0:
							Send_Byte(ACK);
							file_done = 1;
							break;
						
						/* �������� */
						default:
							if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
							{
								Send_Byte(NAK);
							}
							else
							{
								if (packets_received == 0)
								{
									/* �ļ������ݰ� */
									if (packet_data[PACKET_HEADER] != 0)
									{
										/* ��ȡ�ļ��� */
										for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
										{
											FileName[i++] = *file_ptr++;
										}
										/* �ļ���ĩβ�ӽ����� */
										FileName[i++] = '\0';
										
										/* ��ȡ�ļ���С */
										for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
										{
											file_size[i++] = *file_ptr++;
										}
										file_size[i++] = '\0';
										
										/* ���ļ���С���ַ���ת������������ */
										Str2Int(file_size, &size);

										
										/* ����ļ���С�Ƿ��flash�ռ�� */
										if (size > (1024*1024*2 + 1))
										{
											/* ��ֹ���� */
											Send_Byte(CA);
											Send_Byte(CA);
											return -1;
										}

										/* �����û���flash */
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
									/* �ļ������ݰ������꣬��ֹ�˲��֣���ʼ�������� */
									else
									{
										Send_Byte(ACK);
										file_done = 1;
										session_done = 1;
										break;
									}
								}
								
								/* ���ݰ� */
								else
								{
									/* ��ȡ���� */
									memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
									ramsource = (uint32_t)buf;
									
									/* ������� */
									ucState = bsp_WriteCpuFlash((uint32_t)(flashdestination + TotalSize),  (uint8_t *)ramsource, packet_length);
									TotalSize += packet_length;
									
									/* ������ط�0����ʾ���ʧ�� */
									if(ucState != 0)
									{
										/* ��ֹ���� */
										Send_Byte(CA);
										Send_Byte(CA);
										return -2;
									}
									
									Send_Byte(ACK);
								}
								/* �������ݰ����� */
								packets_received ++;
								session_begin = 1;
							}
					}
					break;
				
				/* �û���ֹ���� */
				case 1:
					Send_Byte(CA);
					Send_Byte(CA);
					return -3;
				
				/* ���� */
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
*	�� �� ��: Ymodem_CheckResponse
*	����˵��: ��Ӧ      
*	��    ��: c �ַ�
*	�� �� ֵ: 0
*********************************************************************************************************
*/
int32_t Ymodem_CheckResponse(uint8_t c)
{
	return 0;
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
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
	uint16_t i, j;
	uint8_t file_ptr[10];

	/* ��һ�����ݵ�ǰ�����ַ�  */
	data[0] = SOH; /* soh��ʾ���ݰ���128�ֽ� */
	data[1] = 0x00;
	data[2] = 0xff;

	/* �ļ��� */
	for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
	{
		data[i + PACKET_HEADER] = fileName[i];
	}

	data[i + PACKET_HEADER] = 0x00;

	/* �ļ���Сת�����ַ� */
	Int2Str (file_ptr, *length);
	for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
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
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
	uint16_t i, size, packetSize;
	uint8_t* file_ptr;

	/* ���ú�Ҫ�������ݰ���ǰ�����ַ�data[0]��data[1]��data[2] */
	/* ����sizeBlk�Ĵ�С�������������ݸ�����ȡ1024�ֽڻ���ȡ128�ֽ�*/
	packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
	/* ���ݴ�С��һ��ȷ�� */
	size = sizeBlk < packetSize ? sizeBlk :packetSize;
	
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
	for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
	{
		data[i] = *file_ptr++;
	}
	
	/* ����Ĳ� EOF (0x1A) �� 0x00 */
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
	if(in & 0x100)
		++crc;
	if(crc & 0x10000)
		crc ^= 0x1021;
  }while(!(in & 0x10000));

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
*	�� �� ��: CalChecksum
*	����˵��: ����һ�������ܺ�
*	��    ��: data  ����
*             size  ���ݳ���
*	�� �� ֵ: �������ĺ�8λ
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
*	�� �� ��: Ymodem_SendPacket
*	����˵��: ����һ������
*	��    ��: data  ����
*             length  ���ݳ���
*	�� �� ֵ: ��
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
*	�� �� ��: Ymodem_Transmit
*	����˵��: �����ļ�
*	��    ��: buf  �ļ�����
*             sendFileName  �ļ���
*             sizeFile    �ļ���С
*	�� �� ֵ: 0  �ļ����ͳɹ�
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

	/* ��ʼ��Ҫ���͵ĵ�һ�����ݰ� */
	Ymodem_PrepareIntialPacket(&packet_data[0], filename, &sizeFile);
  
	do 
	{
		/* �������ݰ� */
		Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* ����CRC16_F����CRC������ͽ���У�� */
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
  
		/* �ȴ� Ack ���ַ� 'C' */
		if (Receive_Byte(&receivedC[0], 10000) == 0)  
		{
			if (receivedC[0] == ACK)
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
	}while (!ackReceived && (errors < 0x0A));
  
	/* ����������������˳� */
	if (errors >=  0x0A)
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
		receivedC[0]= 0;
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
			
			Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
			
			/* ����CRC16_F����CRCУ�������͵Ľ�� */
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

			/* �ȵ�Ack�ź� */
			if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
			{
				ackReceived = 1; 
				/* �޸�buf_ptrλ���Լ�size��С��׼��������һ������ */
				if (size > pktSize)
				{
					buf_ptr += pktSize;  
					size -= pktSize;
					if (blkNumber == (2*1024*1024/128))
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
			
		}while(!ackReceived && (errors < 0x0A));
		
		/* ����10��û���յ�Ӧ����˳� */
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
		
		/* ����EOT�ź� */
		/* �ȴ�AckӦ�� */
		if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
		{
			ackReceived = 1;  
		}
		else
		{
			errors++;
		}
		
	}while (!ackReceived && (errors < 0x0A));
    
	/* ����10��û���յ�Ӧ����˳� */
	if (errors >=  0x0A)
	{
		return errors;
	}
  
	/* ��ʼ�����һ��Ҫ���͵����� */
	ackReceived = 0;
	receivedC[0] = 0x00;
	errors = 0;

	packet_data[0] = SOH;
	packet_data[1] = 0;
	packet_data [2] = 0xFF;

	/* ���ݰ������ݲ���ȫ����ʼ��Ϊ0 */
	for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
	{
		packet_data [i] = 0x00;
	}
  
	do 
	{
		/* �������ݰ� */
		Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);

		/* ����CRC16_F����CRCУ�������͵Ľ�� */
		tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
		Send_Byte(tempCRC >> 8);
		Send_Byte(tempCRC & 0xFF);

		/* �ȴ� Ack ���ַ� 'C' */
		if (Receive_Byte(&receivedC[0], 10000) == 0)  
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
	}while (!ackReceived && (errors < 0x0A));

	/* ����10��û���յ�Ӧ����˳� */
	if (errors >=  0x0A)
	{
		return errors;
	}  
  
	do 
	{
		Send_Byte(EOT);
		/* ����EOT�ź� */
		/* �ȴ�AckӦ�� */
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
	return 0; /* �ļ����ͳɹ� */
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
