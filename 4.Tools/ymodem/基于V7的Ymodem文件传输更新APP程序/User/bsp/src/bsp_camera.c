/*
*********************************************************************************************************
*
*	模块名称 : 摄像头驱动BSP模块(For OV7670)
*	文件名称 : bsp_camera.c
*	版    本 : V1.0
*	说    明 : OV7670驱动程序。本程序适用于 guanfu_wang  的OV7670摄像头（不带FIFO,不带LDO，不带24M晶振)
*			  安富莱STM32-V5开发板集成了3.0V LDO给OV7670供电，主板集成了24M有源晶振提供给摄像头。
*
*			  本代码参考了 guanfu_wang 提供的例子。http://mcudiy.taobao.com/
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-03-01 armfly  正式发布
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

/*
	安富莱STM32-V7开发板摄像头接口GPIO定义： 【DCIM设备： 摄像头  和串口6, AD7606 模块不能同时使用】
	PA6/DCMI_PIXCLK
	PA4/DCMI_HSYNC/DAC_OUT1
	PC6/DCMI_D0/AD7606_CONVST
	PC7/DCMI_D1/USART6_RX
	PG10/DCMI_D2/NRF24L01_CSN
	PG11/DCMI_D3/ETH_RMII_TX_EN
	PE4/DCMI_D4/NRF24L01_IRQ
	PD3/DCMI_D5
	PE5/DCMI_D6/AD7606_BUSY
	PE6/DCMI_D7/NRF905_CD
	PB7/DCMI_VSYNC
	
	--- I2C总线控制摄像头
	PB6/I2C2_SCL
	PB9/I2C2_SDA
*/

#define DCMI_DR_ADDRESS       	0x50050028
#define OV_REG_NUM  	116  //OV7670


/*　DMA通道定义,可选的有 DMA2_Stream1 +  DMA_Channel_1、 DMA2_Stream7 +  DMA_Channel_1  */

#define DMA_CLOCK              RCC_AHB1Periph_DMA2
#define DMA_STREAM             DMA2_Stream7
#define DMA_CHANNEL            DMA_Channel_1
#define DMA_IRQ                DMA2_Stream7_IRQn
#define DMA_IT_TCIF            DMA_IT_TCIF7
#define DMA_IRQHandler         DMA2_Stream7_IRQHandler


/* 调分辨率

12 17 18 19 1A 03。。。

*/

/*
	以下为OV7670 QVGA RGB565参数  (by guanfu_wang)  http://mcudiy.taobao.com

	由于RA8875图形模式下，扫描方向为从左到右，从上到下。
	和wang_guanfu提供的缺省值不同。因此做了适当的调整。
*/
static const unsigned char  OV_reg[][2] =
{

	{0x3a, 0x0c},
	{0x67, 0xc0},
	{0x68, 0x80},

	{0x40, 0xd0},
	{0x12, 0x14},

	{0x32, 0x80},
	
	#if 1  //320*240使用
		{0x17, 0x17},//HSTART start high 8-bit MSB         
		{0x18, 0x05},//5 HSTOP end high 8-bit
		
	#else  //640*480使用
		{0x17, 0x14},//HSTART start high 8-bit MSB         
		{0x18, 0x02},//5 HSTOP end high 8-bit
	#endif
		
	{0x19, 0x03},
	{0x1a, 0x7b},
	{0x03, 0x05},

	{0x0c, 0x00},
	{0x15, 0x00},
	{0x3e, 0x00},
	{0x70, 0x3a},
	{0x71, 0x35},
	{0x72, 0x11},
	{0x73, 0x00},
			
	{0xa2, 0x02},
	{0x7a, 0x20},
	{0x7b, 0x1c},
	{0x7c, 0x28},
			
	{0x7d, 0x3c},
	{0x7e, 0x55},
	{0x7f, 0x68},
	{0x80, 0x76},
	{0x81, 0x80},
			
	{0x82, 0x88},
	{0x83, 0x8f},
	{0x84, 0x96},
	{0x85, 0xa3},
	{0x86, 0xaf},
			
	{0x87, 0xc4},
	{0x88, 0xd7},
	{0x89, 0xe8},
	{0x13, 0xe0},
	{0x00, 0x00},
			
	{0x10, 0x00},
	{0x0d, 0x00},
	{0x14, 0x28},
	{0xa5, 0x05},
	{0xab, 0x07},
			
	{0x24, 0x75},
	{0x25, 0x63},
	{0x26, 0xA5},
	{0x9f, 0x78},
	{0xa0, 0x68},
			
	{0xa1, 0x03},
	{0xa6, 0xdf},
	{0xa7, 0xdf},
	{0xa8, 0xf0},
	{0xa9, 0x90},
			
	{0xaa, 0x94},
	{0x13, 0xe5},
	{0x0e, 0x61},
	{0x0f, 0x4b},
	{0x16, 0x02},
	 
	{0x1e, 0x37},
	{0x21, 0x02},
	{0x22, 0x91},
	{0x29, 0x07},
	{0x33, 0x0b},
			
	{0x35, 0x0b},
	{0x37, 0x1d},
	{0x38, 0x71},
	{0x39, 0x2a},
	{0x3c, 0x78},
			
	{0x4d, 0x40},
	{0x4e, 0x20},
	{0x69, 0x00},
	/*
	emWin的情况下64MHz和48MHz稳定，72MHz不行，快了，容易错误
	6B寄存器  bit6:7 
			00 bypass PLL
			01 X4
			10 X6
			11 X8

	11寄存器  bit7 保留
			  bit6 直接使用外部时钟
			  bit5:0
			  实际内部时钟的输出 = Finputclk / (bit[5:0] + 1)

	摄像头晶振是24MHz ，当前配置24 * 8 / 3 = 64MHz
	*/
	{0x11, 0x02},
	{0x6b, 0xc0},
	{0x74, 0x19},
	{0x8d, 0x4f},
			
	{0x8e, 0x00},
	{0x8f, 0x00},
	{0x90, 0x00},
	{0x91, 0x00},
	{0x92, 0x00},

			
	{0x96, 0x00},
	{0x9a, 0x80},
	{0xb0, 0x84},
	{0xb1, 0x0c},
	{0xb2, 0x0e},
			
	{0xb3, 0x82},
	{0xb8, 0x0a},
	{0x43, 0x14},
	{0x44, 0xf0},
	{0x45, 0x34},
			
	{0x46, 0x58},
	{0x47, 0x28},
	{0x48, 0x3a},
	{0x59, 0x88},
	{0x5a, 0x88},
			
	{0x5b, 0x44},
	{0x5c, 0x67},
	{0x5d, 0x49},
	{0x5e, 0x0e},
	{0x64, 0x04},
	{0x65, 0x20},
			
	{0x66, 0x05},
	{0x94, 0x04},
	{0x95, 0x08},
	{0x6c, 0x0a},
	{0x6d, 0x55},
			
			
	{0x4f, 0x80},
	{0x50, 0x80},
	{0x51, 0x00},
	{0x52, 0x22},
	{0x53, 0x5e},
	{0x54, 0x80},	
			
			
	{0x09, 0x03},

	{0x6e, 0x11},
	{0x6f, 0x9f},
	{0x55, 0x00},
	{0x56, 0x40},
	{0x57, 0x40},

	{0x6a, 0x40},
	{0x01, 0x40},
	{0x02, 0x40},
	{0x13, 0xe7},
	{0x15, 0x00},  


	{0x58, 0x9e},

	{0x41, 0x08},
	{0x3f, 0x00},
	{0x75, 0x05},
	{0x76, 0xe1},
	{0x4c, 0x00},
	{0x77, 0x01},
	{0x3d, 0xc2}, 
	{0x4b, 0x09},
	{0xc9, 0x60},
	{0x41, 0x38},

	{0x34, 0x11},
	{0x3b, 0x02}, 

	{0xa4, 0x89},
	{0x96, 0x00},
	{0x97, 0x30},
	{0x98, 0x20},
	{0x99, 0x30},
	{0x9a, 0x84},
	{0x9b, 0x29},
	{0x9c, 0x03},
	{0x9d, 0x4c},
	{0x9e, 0x3f},
	{0x78, 0x04},

	{0x79, 0x01},
	{0xc8, 0xf0},
	{0x79, 0x0f},
	{0xc8, 0x00},
	{0x79, 0x10},
	{0xc8, 0x7e},
	{0x79, 0x0a},
	{0xc8, 0x80},
	{0x79, 0x0b},
	{0xc8, 0x01},
	{0x79, 0x0c},
	{0xc8, 0x0f},
	{0x79, 0x0d},
	{0xc8, 0x20},
	{0x79, 0x09},
	{0xc8, 0x80},
	{0x79, 0x02},
	{0xc8, 0xc0},
	{0x79, 0x03},
	{0xc8, 0x40},
	{0x79, 0x05},
	{0xc8, 0x30},
	{0x79, 0x26}, 
	{0x09, 0x00}, 
};

static void CAM_ConfigCPU(void);
static uint8_t OV_InitReg(void);
static void OV_WriteReg(uint8_t _ucRegAddr, uint8_t _ucRegValue);
static uint8_t OV_ReadReg(uint8_t _ucRegAddr);

CAM_T g_tCam;

DCMI_HandleTypeDef hdcmi = {0};
DMA_HandleTypeDef  hdma_dcmi = {0};

/*
*********************************************************************************************************
*	函 数 名: bsp_InitCamera
*	功能说明: 配置摄像头GPIO和CAMERA设备.
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitCamera(void)
{
	CAM_ConfigCPU();

	#if 1	/* 下面的代码，验证读写寄存器是否正确 */
	{
		uint8_t read;

		read = OV_ReadReg(0x3A);

		OV_WriteReg(0x3A, read + 1);

		read = OV_ReadReg(0x3A);

		OV_WriteReg(0x3A, read + 1);

		read = OV_ReadReg(0x3A);
	}
	#endif

	OV_InitReg();
}

/*
*********************************************************************************************************
*	函 数 名: CAM_ConfigCPU
*	功能说明: 配置摄像头GPIO和CAMERA设备。0V7670的I2C接口配置在 bsp_gpio_i2c.c 文件实现。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void CAM_ConfigCPU(void)
{
 	/* 安富莱STM32-V7开发板采用有源晶振提供24M时钟，因此不用PA8产生时钟 */

	/*
		将DCMI相关的GPIO设置为复用模式  - STM32-V7
			PA4/DCMI_HSYNC/DAC_OUT1		
			PA6/DCMI_PIXCLK
			PB7/DCMI_VSYNC			
			PC6/DCMI_D0/AD7606_CONVST
			PC7/DCMI_D1/USART6_RX
			PD3/DCMI_D5
			PE4/DCMI_D4/NRF24L01_IRQ
			PE5/DCMI_D6/AD7606_BUSY
			PE6/DCMI_D7/NRF905_CD			
			PG10/DCMI_D2/NRF24L01_CSN
			PG11/DCMI_D3/ETH_RMII_TX_EN
	*/
	{  	
		GPIO_InitTypeDef GPIO_InitStruct;
		
		/* 使能DCMI时钟 */
		__HAL_RCC_DCMI_CLK_ENABLE();
		
		__HAL_RCC_GPIOA_CLK_ENABLE();
		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOD_CLK_ENABLE();
		__HAL_RCC_GPIOE_CLK_ENABLE();
		__HAL_RCC_GPIOG_CLK_ENABLE();
		
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
		
		GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_6;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);		
		
		GPIO_InitStruct.Pin = GPIO_PIN_7;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);	

		GPIO_InitStruct.Pin = GPIO_PIN_3;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
		HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);	
		
		GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
		HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);	
	}

	/* 配置 DCMIC 参数 */
	{
		/* 初始DCMI */
		hdcmi.Instance = DCMI;
		hdcmi.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
		hdcmi.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;   
		hdcmi.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;              
		hdcmi.Init.HSPolarity = DCMI_HSPOLARITY_LOW;    
		hdcmi.Init.CaptureRate = DCMI_CR_ALL_FRAME;
		hdcmi.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;                  
		hdcmi.Init.JPEGMode = DCMI_JPEG_DISABLE;  
		hdcmi.Init.ByteSelectMode = DCMI_BSM_ALL;
		hdcmi.Init.ByteSelectStart = DCMI_OEBS_ODD;
		hdcmi.Init.LineSelectMode = DCMI_LSM_ALL;
		hdcmi.Init.LineSelectStart = DCMI_OELS_ODD; 
		if (HAL_DCMI_Init(&hdcmi) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}		
	}
}

/*
*********************************************************************************************************
*	函 数 名: OV_InitReg
*	功能说明: 复位OV7670, 配置OV7670的寄存器，QVGA
*	形    参: 无
*	返 回 值: 0 表示正确，1表示失败
*********************************************************************************************************
*/
static uint8_t OV_InitReg(void)
{
	uint8_t i;

  	CAM_ConfigCPU();
	
	//bsp_InitI2C();		/* 配置I2C总线, 在 bsp.c 文件执行了 */

	OV_WriteReg(0x12, 0x80); 	/* Reset SCCB */

	bsp_DelayMS(5);

	for (i = 0; i < sizeof(OV_reg)/sizeof(OV_reg[0]); i++)
  	{
		OV_WriteReg(OV_reg[i][0], OV_reg[i][1]);
  	}
	
	return 0;
}

/*
*********************************************************************************************************
*	函 数 名: CAM_Start
*	功能说明: 启动DMA和DCMI，开始传送图像数据到LCD显存
*	形    参: _uiDispMemAddr 显存地址
*	返 回 值: 无
*********************************************************************************************************
*/
void CAM_Start(uint32_t _uiDispMemAddr)
{	
	/* 使能DCMI */
	__HAL_RCC_DMA1_CLK_ENABLE();

	hdma_dcmi.Instance                 = DMA1_Stream7;           /* 使用的DMA1 Stream7 */
	hdma_dcmi.Init.Request             = DMA_REQUEST_DCMI;       /* DMA请求来自DCMI */  
	hdma_dcmi.Init.Direction           = DMA_PERIPH_TO_MEMORY;   /* 传输方向是从DCMI到存储器 */ 
	hdma_dcmi.Init.PeriphInc           = DMA_PINC_DISABLE;       /* 外设地址自增禁止 */  
	hdma_dcmi.Init.MemInc              = DMA_MINC_ENABLE;        /* 存储器地址自增使能 */  
	hdma_dcmi.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;    /* 外设数据传输位宽选择字，即32bit */  
	hdma_dcmi.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;    /* 存储器数据传输位宽选择字，即32bit */
	hdma_dcmi.Init.Mode                = DMA_NORMAL;             /* 正常模式模式，传输一帧后停止 */   
	hdma_dcmi.Init.Priority            = DMA_PRIORITY_LOW;       /* 优先级低 */  
	hdma_dcmi.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;   /* 使能FIFO*/
	hdma_dcmi.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;/* 用于设置阀值 */
	hdma_dcmi.Init.MemBurst            = DMA_MBURST_SINGLE;      /* 单次突发，用于存储器突发 */
	hdma_dcmi.Init.PeriphBurst         = DMA_PBURST_SINGLE;      /* 单次突发，用于外设突发 */
	
	/* 初始化DMA */
	if (HAL_DMA_Init(&hdma_dcmi) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* 摄像头句柄关联DMA句柄 */
	__HAL_LINKDMA(&hdcmi, DMA_Handle, hdma_dcmi);

	/* 使能DMA1 Stream7 中断 */
	HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);		
	
	/* 
	   启动DCMI的DMA传输，传输一帧后停止，一帧大小120*320*4字节 = 153600字节
       注意DMA1和DMA2最大传输65536次，这里是120*320 = 38400次，每次位宽4字节。
	*/
	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)_uiDispMemAddr, 120*320);

	g_tCam.CaptureOk = 0;		/* 全局标志 */
}

/* 后续启动传输，仅需调用函数HAL_DCMI_Start_DMA即可 */
void CAM_Start1(uint32_t _uiDispMemAddr)
{
	HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)_uiDispMemAddr, 120*320);	

	g_tCam.CaptureOk = 0;		/* 全局标志 */
}

/*
*********************************************************************************************************
*	函 数 名: CAM_Stop
*	功能说明: 停止DMA和DCMI
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void CAM_Stop(void)
{
	HAL_DCMI_Stop(&hdcmi);
}

void DMA1_Stream7_IRQHandler(void)
{
	/* 传输完成中断 */
	if((DMA1->HISR & DMA_FLAG_TCIF3_7) != RESET)
	{
		
		/* 关闭摄像 */
		CAM_Stop();
		g_tCam.CaptureOk = 1;		/* 表示DMA传输结束 */

		/* 清除标志 */
		DMA1->HIFCR = DMA_FLAG_TCIF3_7;
	}

	/* 半传输完成中断 */    
	if((DMA1->HISR & DMA_FLAG_HTIF3_7) != RESET)
	{

		/* 清除标志 */
		DMA1->HIFCR = DMA_FLAG_HTIF3_7;
	}

	/* 传输错误中断 */
	if((DMA1->HISR & DMA_FLAG_TEIF3_7) != RESET)
	{
		/* 清除标志 */
		DMA1->HIFCR = DMA_FLAG_TEIF3_7;
	}

	/* 直接模式错误中断 */
	if((DMA1->HISR & DMA_FLAG_DMEIF3_7) != RESET)
	{
		/* 清除标志 */
		DMA1->HIFCR = DMA_FLAG_DMEIF3_7;
	}
}

/*
*********************************************************************************************************
*	函 数 名: OV_WriteReg
*	功能说明: 写0V7670寄存器
*	形    参: _ucRegAddr  : 寄存器地址
*			  _ucRegValue : 寄存器值
*	返 回 值: 无
*********************************************************************************************************
*/
static void OV_WriteReg(uint8_t _ucRegAddr, uint8_t _ucRegValue)
{
    i2c_Start();							/* 总线开始信号 */

    i2c_SendByte(OV7670_SLAVE_ADDRESS);		/* 发送设备地址+写信号 */
	i2c_WaitAck();

    i2c_SendByte(_ucRegAddr);				/* 发送寄存器地址 */
	i2c_WaitAck();

    i2c_SendByte(_ucRegValue);				/* 发送寄存器数值 */
	i2c_WaitAck();

    i2c_Stop();                   			/* 总线停止信号 */
}

/*
*********************************************************************************************************
*	函 数 名: OV_ReadReg
*	功能说明: 读0V7670寄存器
*	形    参: _ucRegAddr  : 寄存器地址
*	返 回 值: 寄存器值
*********************************************************************************************************
*/
static uint8_t OV_ReadReg(uint8_t _ucRegAddr)
{
	uint16_t usRegValue;

	i2c_Start();                  			/* 总线开始信号 */
	i2c_SendByte(OV7670_SLAVE_ADDRESS);		/* 发送设备地址+写信号 */
	i2c_WaitAck();
	i2c_SendByte(_ucRegAddr);				/* 发送地址 */
	i2c_WaitAck();

	i2c_Stop();			/* 0V7670 需要插入 stop, 否则读取寄存器失败 */

	i2c_Start();                  			/* 总线开始信号 */
	i2c_SendByte(OV7670_SLAVE_ADDRESS + 1);/* 发送设备地址+读信号 */
	i2c_WaitAck();

	usRegValue = i2c_ReadByte();       		/* 读出高字节数据 */
	i2c_NAck();
	i2c_Stop();                  			/* 总线停止信号 */

	return usRegValue;
}

/*
*********************************************************************************************************
*	函 数 名: OV_ReadID
*	功能说明: 读0V7670的芯片ID
*	形    参: 无
*	返 回 值: 芯片ID. 正常应该返回 0x7673
*********************************************************************************************************
*/
uint16_t OV_ReadID(void)
{
	uint8_t idh,idl;

	idh = OV_ReadReg(0x0A);
	idl = OV_ReadReg(0x0B);
	return (idh << 8) + idl;
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
