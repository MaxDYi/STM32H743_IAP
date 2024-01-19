/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2024 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "usart.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "ymodem.h"
#include "Bootloader.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define B 1
#define R 0
#define LED R
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId taskDefaultHandle;
osThreadId taskIAPHandle;
osThreadId taskKEY1Handle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void TaskDefault(void const *argument);
void TaskIAP(void const *argument);
void TaskKEY1(void const *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* definition and creation of taskDefault */
    osThreadDef(taskDefault, TaskDefault, osPriorityNormal, 0, 128);
    taskDefaultHandle = osThreadCreate(osThread(taskDefault), NULL);

    /* definition and creation of taskIAP */
    osThreadDef(taskIAP, TaskIAP, osPriorityLow, 0, 1024);
    taskIAPHandle = osThreadCreate(osThread(taskIAP), NULL);

    /* definition and creation of taskKEY1 */
    osThreadDef(taskKEY1, TaskKEY1, osPriorityIdle, 0, 128);
    taskKEY1Handle = osThreadCreate(osThread(taskKEY1), NULL);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */
}

/* USER CODE BEGIN Header_TaskDefault */
/**
 * @brief  Function implementing the taskDefault thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_TaskDefault */
void TaskDefault(void const *argument)
{
    /* USER CODE BEGIN TaskDefault */
    /* Infinite loop */
    for (;;)
    {
        if (LED == B)
        {
            HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
        }
        else
        {
            HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
        }
        osDelay(500);
    }
    /* USER CODE END TaskDefault */
}

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
                        // TODO:½ÓÊÕ´íÎó
                    }
                    break;
                }
                else if (YmodemState == YMODEM_STATE_ERROR)
                {
                    // TODO:Éý¼¶Ê§°Ü
                    break;
                }
            }
        }
        osDelay(10);
    }
    /* USER CODE END TaskIAP */
}

/* USER CODE BEGIN Header_TaskKEY1 */
/**
 * @brief Function implementing the taskKEY1 thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_TaskKEY1 */
void TaskKEY1(void const *argument)
{
    /* USER CODE BEGIN TaskKEY1 */
    /* Infinite loop */
    for (;;)
    {
        if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET)
        {
            while ((HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET))
            {
                osDelay(10);
            }
            xTaskNotifyFromISR(taskIAPHandle, pdPASS, eSetValueWithoutOverwrite, NULL);
        }
        osDelay(10);
    }
    /* USER CODE END TaskKEY1 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
