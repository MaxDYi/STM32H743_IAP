/*
*********************************************************************************************************
*
*	ģ������ : �����ļ�
*	�ļ����� : common.c
*	��    �� : V1.0
*	˵    �� : �ַ����������ļ�
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

/*
*********************************************************************************************************
*	�� �� ��: Int2Str
*	����˵��: ������ת�����ַ�
*	��    ��: str �ַ�  intnum ����
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void Int2Str(uint8_t* str, int32_t intnum)
{
	uint32_t i, Div = 1000000000, j = 0, Status = 0;

	for (i = 0; i < 10; i++)
	{
		str[j++] = (intnum / Div) + 48;

		intnum = intnum % Div;
		Div /= 10;
		if ((str[j-1] == '0') & (Status == 0))
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
*	�� �� ��: Str2Int
*	����˵��: ���ַ�ת��������
*	��    ��: inputstr �ַ�  intnum ����
*	�� �� ֵ: 1 ��ȷ 2 ����
*********************************************************************************************************
*/
uint32_t Str2Int(uint8_t *inputstr, int32_t *intnum)
{
	uint32_t i = 0, res = 0;
	uint32_t val = 0;

	if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X'))
	{
		if (inputstr[2] == '\0')
		{
			return 0;
		}
		
		for (i = 2; i < 11; i++)
		{
			if (inputstr[i] == '\0')
			{
				*intnum = val;
				/* return 1; */
				res = 1;
				break;
			}
			
			if (ISVALIDHEX(inputstr[i]))
			{
				val = (val << 4) + CONVERTHEX(inputstr[i]);
			}
			else
			{
				/* Return 0, Invalid input */
				res = 0;
				break;
			}
		}
		/* Over 8 digit hex --invalid */
		if (i >= 11)
		{
			res = 0;
		}
	}
	else /* max 10-digit decimal input */
	{
		for (i = 0;i < 11;i++)
		{
			if (inputstr[i] == '\0')
			{
				*intnum = val;
				/* return 1 */
				res = 1;
				break;
			}
			else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0))
			{
				val = val << 10;
				*intnum = val;
				res = 1;
				break;
			}
			else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0))
			{
				val = val << 20;
				*intnum = val;
				res = 1;
				break;
			}
			else if (ISVALIDDEC(inputstr[i]))
			{
				val = val * 10 + CONVERTDEC(inputstr[i]);
			}
			else
			{
				/* return 0, Invalid input */
				res = 0;
				break;
			}
		}
		/* Over 10 digit decimal --invalid */
		if (i >= 11)
		{
			res = 0;
		}
	}

	return res;
}

/*
*********************************************************************************************************
*	�� �� ��: Str2Int
*	����˵��: ��ȡ��������
*	��    ��: num ����
*	�� �� ֵ: 1 ��ȷ 2 ����
*********************************************************************************************************
*/
uint32_t GetIntegerInput(int32_t * num)
{
	uint8_t inputstr[16];

	while (1)
	{
		GetInputString(inputstr);
		if (inputstr[0] == '\0') continue;
		if ((inputstr[0] == 'a' || inputstr[0] == 'A') && inputstr[1] == '\0')
		{
			SerialPutString("User Cancelled \r\n");
			return 0;
		}

		if (Str2Int(inputstr, num) == 0)
		{
			SerialPutString("Error, Input again: \r\n");
		}
		else
		{
			return 1;
		}
	}
}

/*
*********************************************************************************************************
*	�� �� ��: SerialKeyPressed
*	����˵��: ��ȡ��������
*	��    ��: key ����
*	�� �� ֵ: 1 �������� 2 û�з���
*********************************************************************************************************
*/
uint32_t SerialKeyPressed(uint8_t *key)
{
	if (comGetChar(COM1, key))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*
*********************************************************************************************************
*	�� �� ��: SerialPutChar
*	����˵��: �����ַ�
*	��    ��: c �����ַ�
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void SerialPutChar(uint8_t c)
{
	/* �ȴ����Ϳ� */
	while((USART1->ISR & USART_FLAG_TXE) == 0)
	{}	
	
	USART1->TDR = c;
	
	/* �ȴ����ͽ��� */
	while((USART1->ISR & USART_FLAG_TC) == 0)
	{}
	
	//comSendChar(COM1, c);
}

/*
*********************************************************************************************************
*	�� �� ��: GetKey
*	����˵��: ��ȡ���µİ���
*	��    ��: ��
*	�� �� ֵ: ����ֵ
*********************************************************************************************************
*/
uint8_t GetKey(void)
{
	uint8_t key = 0;

	/* �ȴ��û����� */
	while (1)
	{
		if (SerialKeyPressed((uint8_t*)&key)) break;
	}
	return key;
}

/*
*********************************************************************************************************
*	�� �� ��: Serial_PutString
*	����˵��: �����ַ���
*	��    ��: s �����ַ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void Serial_PutString(uint8_t *s)
{
	while (*s != '\0')
	{
		SerialPutChar(*s);
		s++;
	}
}

/*
*********************************************************************************************************
*	�� �� ��: GetInputString
*	����˵��: �ӳ����ն˻�ȡ������ַ���
*	��    ��: buffP ���յ����ַ���
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void GetInputString (uint8_t * buffP)
{
	uint32_t bytes_read = 0;
	uint8_t c = 0;
	do
	{
		c = GetKey();
		if (c == '\r')
			break;
		
		if (c == '\b') /* Backspace */
		{
			if (bytes_read > 0)
			{
			SerialPutString("\b \b");
			bytes_read --;
			}
			continue;
		}
		
		if (bytes_read >= CMD_STRING_SIZE )
		{
			SerialPutString("Command string size overflow\r\n");
			bytes_read = 0;
			continue;
		}
		
		if (c >= 0x20 && c <= 0x7E)
		{
			buffP[bytes_read++] = c;
			SerialPutChar(c);
		}
	}
	while (1);
	SerialPutString(("\n\r"));
	buffP[bytes_read] = '\0';
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
