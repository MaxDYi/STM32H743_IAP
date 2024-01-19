# STM32H743使用双BANK设计串口IAP

STM32H7系列拥有双BANK，可更方便的设计Bootloader。在ST的F4、F7、L0、L4等系列中均有双BANK，但不同的是H7默认使用双BANK，寄存器设置与其他系列有较大不同。

# 0 目录

1. 嵌入式Flash说明
2. 设计思路
3. 存储区交换设计
4. Flash操作
5. Ymodem协议
6. 测试

# 1 嵌入式Flash说明

## 1.1 Flash架构

Flash 为 266 位的 Flash 字存储器，用于存储代码和数据常量。每个字包括：

- 一个 Flash 字（8 个字、32 个字节或 256 位）。
- 10 个 ECC 位。

Flash 分为两个独立的存储区。每个存储区的构成如下：

- 1 MB 的用户 Flash 块，包括八个 128 KB 的用户扇区（4 K Flash 字）。
- 128 KB 的系统 Flash，器件可从该 Flash 启动：此区域包含根安全服务 (root secure services, RSS) 和自举程序，两者分别用于通过USART、USB (DFU)、I2C、SPI 或以太网接口进行安全或非安全 Flash 编程。系Flash 留给意法半导体使用。它由意法半导体在器件制造期间编程并加以保护以防止误编程/误擦除操作。更多详细信息，请参见 http://www.st.com 上提供的应用笔记 AN2606《STM32 微控制器系统 Flash 启动模式》。
- 2 KB（64 个 Flash 字）的用户选项字节，用于进行用户配置：此区域仅在存储区 1 中可用。与用户 Flash 和系统 Flash 不同，该区域并未映射到任何存储器地址，并且仅可通过 Flash 寄存器接口进行访问。

![image-20240119101552278](https://raw.githubusercontent.com/MaxDYi/PicGo/main/202401191015331.png)

## 1.2 Flash双存储区构成

### 1.2.1 存储区域1

|        Flash区域        |  起始地址   |  结束地址   | 大小（字节） |
| :---------------------: | :---------: | :---------: | :----------: |
| 存储区域1用户Flash扇区0 | 0x0800_0000 | 0x0801_FFFF |     128K     |
| 存储区域1用户Flash扇区1 | 0x0802_0000 | 0x0803_FFFF |     128K     |
| 存储区域1用户Flash扇区2 | 0x0804_0000 | 0x0805_FFFF |     128K     |
| 存储区域1用户Flash扇区3 | 0x0806_0000 | 0x0807_FFFF |     128K     |
| 存储区域1用户Flash扇区4 | 0x0808_0000 | 0x0809_FFFF |     128K     |
| 存储区域1用户Flash扇区5 | 0x080A_0000 | 0x080B_FFFF |     128K     |
| 存储区域1用户Flash扇区6 | 0x080C_0000 | 0x080D_FFFF |     128K     |
| 存储区域1用户Flash扇区7 | 0x080E_0000 | 0x080F_FFFF |     128K     |
|   存储区域1系统Flash    | 0x1FF0_0000 | 0x1FF1_FFFF |     128K     |

### 1.2.2 存储区域2

|        Flash区域        |  起始地址   |  结束地址   | 大小（字节） |
| :---------------------: | :---------: | :---------: | :----------: |
| 存储区域2用户Flash扇区0 | 0x0810_0000 | 0x0811_FFFF |     128K     |
| 存储区域2用户Flash扇区1 | 0x0812_0000 | 0x0813_FFFF |     128K     |
| 存储区域2用户Flash扇区2 | 0x0814_0000 | 0x0815_FFFF |     128K     |
| 存储区域2用户Flash扇区3 | 0x0816_0000 | 0x0817_FFFF |     128K     |
| 存储区域2用户Flash扇区4 | 0x0818_0000 | 0x0819_FFFF |     128K     |
| 存储区域2用户Flash扇区5 | 0x081A_0000 | 0x081B_FFFF |     128K     |
| 存储区域2用户Flash扇区6 | 0x081C_0000 | 0x081D_FFFF |     128K     |
| 存储区域2用户Flash扇区7 | 0x081E_0000 | 0x081F_FFFF |     128K     |
|   存储区域2系统Flash    | 0x1FF4_0000 | 0x1FF5_FFFF |     128K     |

# 2 设计思路

双BANK设计串口IAP流程图如下：

![IAP设计流程](https://raw.githubusercontent.com/MaxDYi/PicGo/main/202401191054352.png)

其中，固件的整体校验使用CRC-32/MPEG-2，可在IAR中直接设置，使校验码放置于Flash区域的最后4字节。详见[使用IAR Embedded Workbench和MCU的CRC模块来检查代码的完整性 | IAR](https://www.iar.com/cn/knowledge/support/technical-notes/general/verifying-integrity-by-iar-embedded-workbench-and-crc/)。

# 3 存储区交换设计

Flash 接口允许交换存储区 1 和存储区 2 的存储器映射。固件升级后可使用此功能使器件在系统复位后以新固件重启。存储区交换是由 FLASH_OPTCR 寄存器中的 SWAP_BANK 位控制的。下表按 SWAP_BANK 选项位配置列出了两个 AXI 从 Flash 接口可使用的存储器映射。

## 3.1 AXI 接口存储器映射

**AXI 接口存储器映射 SWAP_BANK =“0”**

|        区域         |   AXI 1   |   AXI 2   |
| :-----------------: | :-------: | :-------: |
|  用户扇区存储区 1   | 有^（1）^ |    无     |
|  用户扇区存储区 2   |    无     | 有^（1）^ |
| 系统 Flash 存储区 1 | 有^（2）^ |    无     |
| 系统 Flash 存储区 2 |    无     | 有^（2）^ |

1. 用户存储区1和用户存储区2分别从0x0800 0000映射到0x080F FFFF，从0x0810 0000 映射到0x081F FFFF。
2. 系统Flash存储区1和系统Flash存储区2分别从0x1FF0 0000映射到0x1FF1 FFFF，从0x1FF4 0000映射到0x1FF5 FFFF。

**AXI 接口存储器映射 SWAP_BANK =“1”**

|        区域         |   AXI 1   |   AXI 2   |
| :-----------------: | :-------: | :-------: |
|  用户扇区存储区 1   |    无     | 有^（1）^ |
|  用户扇区存储区 2   | 有^（1）^ |    无     |
| 系统 Flash 存储区 1 | 有^（2）^ |    无     |
| 系统 Flash 存储区 2 |    无     | 有^（2）^ |

1. 用户存储区1和用户存储区2分别从0x0810 0000映射到0x081F FFFF，从0x0800 0000 映射到0x080F FFFF。
2. 系统Flash存储区1和系统Flash存储区2分别从0x1FF0 0000映射到0x1FF1 FFFF，从0x1FF4 0000映射到0x1FF5 FFFF。

## 3.2 存储区交换步骤

![image-20240119102640074](https://raw.githubusercontent.com/MaxDYi/PicGo/main/202401191026121.png)

## 3.3 存储区交换代码

```C
/**
 * @description: 转换bank区域
 * @param {uint8_t} bankNum
 * @return {*}
 */
void SwapBank(uint8_t bankNum);

/**
 * @description: 获取当前运行的bank区域
 * @return {*}
 */
uint8_t GetBank(void);
```

# 4 Flash操作

Flash读、写（编程）、擦除操作已进行封装，详见**InternalFlash.c**文件。

## 4.1 读协议

Flash 接口支持以下访问类型： 

- 双字（64 位）
- 单字（32 位）
- 半字（16 位）
- 字节（8 位）

```C
/*
*	函 数 名: ReadCpuFlash
*	功能说明: 读取CPU Flash的内容
*	形    参:  _ucpDst : 目标缓冲区
*			 _ulFlashAddr : 起始地址
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0=成功，1=失败
*/
uint8_t ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize);
```

## 4.2 Flash编程

Flash 接口支持多种编程操作：

- 写入用户扇区
- 更改用户选项字节

```C
/*
*	函 数 名: WriteCpuFlash
*	功能说明: 写数据到CPU 内部Flash。 必须按32字节整数倍写。不支持跨扇区。扇区大小128KB. \
*			  写之前需要擦除扇区. 长度不是32字节整数倍时，最后几个字节末尾补0写入.
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpSrc : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节, 必须是32字节整数倍）
*	返 回 值: 0-成功，1-数据长度或地址溢出，2-写Flash出错(估计Flash寿命到)
*/
uint8_t WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize);
```

## 4.3 Flash擦除

Flash 接口支持多种擦除操作：

- 擦除用户扇区
- 擦除存储区 1、存储区 2 或同时擦除两个存储区

```C
/*
*	函 数 名: EraseCpuFlash
*	功能说明: 擦除CPU FLASH一个扇区 （128KB)
*	形    参: _ulFlashAddr : Flash地址
*	返 回 值: 0 成功， 1 失败
*			  HAL_OK       = 0x00,
*			  HAL_ERROR    = 0x01,
*			  HAL_BUSY     = 0x02,
*			  HAL_TIMEOUT  = 0x03
*
*/
uint8_t EraseCpuFlash(uint32_t _ulFlashAddr);
```

# 5 Ymodem协议

Ymodem协议可以查看[YMODEM - Wikipedia](https://en.wikipedia.org/wiki/YMODEM)和[textfiles.com/programming/ymodem.txt](http://textfiles.com/programming/ymodem.txt)。

使用状态机进行Ymodem协议实现，通信接口选择串口。

## 5.1 Ymodem协议命令

|  **命令**  | **命令码** |            **备注**             |
| :--------: | :--------: | :-----------------------------: |
| YMODEM_SOH |    0x01    |          128字节长度帧          |
| YMODEM_STX |    0x02    |         1024字节长度帧          |
| YMODEM_EOT |    0x04    |        文件传输结束命令         |
| YMODEM_ACK |    0x06    |        接收正确应答命令         |
| YMODEM_NAK |    0x15    |     重传当前数据包请求命令      |
| YMODEM_CAN |    0x18    | 取消传输命令，连续发送5个该命令 |
|  YMODEM_C  |    0x43    |              字符C              |

## 5.2 Ymodem协议状态

|         状态机状态          | 代码 |    备注    |
| :-------------------------: | :--: | :--------: |
|      YMODEM_STATE_IDLE      | 0x00 |    空闲    |
|   YMODEM_STATE_WAIT_START   | 0x01 |  等待开始  |
|  YMODEM_STATE_RECEIVE_INFO  | 0x02 | 接收起始帧 |
|  YMODEM_STATE_RECEIVE_DATA  | 0x03 | 接收数据包 |
|  YMODEM_STATE_RECEIVE_END   | 0x04 |  接收结束  |
|  YMODEM_STATE_RECEIVE_STOP  | 0x05 |  接收终止  |
| YMODEM_STATE_RECEIVE_FINISH | 0x06 |  接收完成  |
|     YMODEM_STATE_ERROR      | 0xFF |  传输错误  |

## 5.3 Ymodem接收代码

```C
/**
 * @brief Function implementing the taskIAP thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_TaskIAP */
void TaskIAP(void const *argument)
{
    /* USER CODE BEGIN TaskIAP */
    BaseType_t xResult;
    uint32_t ulValue;
    /* Infinite loop */
    for (;;)
    {
        xResult = xTaskNotifyWait(0, 0, &ulValue, portMAX_DELAY);
        if (xResult == pdPASS)
        {
            uint8_t YmodemState = Ymodem_ReceiveFile(YMODEM_STATE_IDLE);
            while (1)
            {
                YmodemState = Ymodem_ReceiveFile(YmodemState);
                if (YmodemState == YMODEM_STATE_RECEIVE_FINISH)
                {
                    if (CheckAppCRC(APP_START_ADDRESS, APP_MAX_SIZE, APP_CRC_ADDRESS) == CRC_SUCCESS)
                    {
                        SwapBank(2);
                    }
                    else
                    {
                        // TODO:接收错误
                    }
                    break;
                }
                else if (YmodemState == YMODEM_STATE_ERROR)
                {
                    // TODO:升级失败
                    break;
                }
            }
        }
        osDelay(10);
    }
    /* USER CODE END TaskIAP */
}
```

# 6 测试

板载3颗LED，分别为红、绿、蓝色，两版固件分别为红色LED闪烁和蓝色LED闪烁。绿色LED用作升级指示，当绿色LED长亮时，代表系统正在重启升级固件。

| 颜色  | 引脚 |
| ----- | ---- |
| RED   | PB0  |
| GREEN | PB1  |
| BLUE  | PA3  |

下载固件使用**SecureCRT**工具。配置按键KEY1，当按下KEY1时，通过FreeRTOS的任务通知进入Ymodem任务，执行IAP。

![image-20240119120005260](https://raw.githubusercontent.com/MaxDYi/PicGo/main/202401191200331.png)

配置串口波特率为921600,1M的固件下载大约需要1min，Flash擦除操作消耗的时间较多。

![0b0c51dcb2e4dae454ef83f3f2cf664](https://raw.githubusercontent.com/MaxDYi/PicGo/main/202401191200179.png)
