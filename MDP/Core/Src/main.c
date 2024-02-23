/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gyro.h"
#include "i2c.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;

UART_HandleTypeDef huart3;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for show */
osThreadId_t showHandle;
const osThreadAttr_t show_attributes = {
  .name = "show",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for MotorTask */
osThreadId_t MotorTaskHandle;
const osThreadAttr_t MotorTask_attributes = {
  .name = "MotorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for EncoderTask */
osThreadId_t EncoderTaskHandle;
const osThreadAttr_t EncoderTask_attributes = {
  .name = "EncoderTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for GyroTask */
osThreadId_t GyroTaskHandle;
const osThreadAttr_t GyroTask_attributes = {
  .name = "GyroTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* USER CODE BEGIN PV */
//Queue struct
//to define the nodes for the queue
typedef struct _listnode{
    uint8_t msg[4]; //type for every instruction
    struct _listnode *next;
} ListNode;

typedef ListNode QueueNode; //define QueueNode as a ListNode structure

typedef struct _queue{
   int size; //amount of instructions in the queue
   ListNode *head;
   ListNode *tail;
} Queue;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM8_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM5_Init(void);
void StartDefaultTask(void *argument);
void showMsg(void *argument);
void motors(void *argument);
void encoder(void *argument);
void gryo_task(void *argument);

/* USER CODE BEGIN PFP */

//Prototypes of Interface functions for Queue structure
void enqueue(Queue *qPtr, uint8_t msg[4]);
int dequeue(Queue *qPtr);
void getFront(Queue q);
int isEmptyQueue(Queue q);
void deleteQueue(Queue *qPtr);

void usDelay(uint16_t time){
	__HAL_TIM_SET_COUNTER(&htim5, 0);
	while(__HAL_TIM_GET_COUNTER (&htim5) < time);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t aRxBuffer[50];

uint8_t frontback = 'w';	// Front/back character
uint8_t fb_speed = '0';		// Front/back speed
uint8_t leftright = 'a';	// Left/right character
uint8_t lr_speed = '0';		// Left/right speed

double curAngle = 0; 		// angle via gyro

int encoder_offset = 0; // Adds to motor pwm for straight movement
int encoder_error = 0;	// Current error value for encoder
uint32_t encoder_dist = 0;	// Total distance estimation (counting encoder pulses)

uint16_t pwmVal_servo = 148; // servo centre

Queue q;

int turn_angle_l = 118;	// gyro turn threshold for 90deg
int turn_angle_r = 118;


// GGGGGGGGG
// Ultrasonic Sensor
uint8_t msg[2] = {'W','1'};

uint32_t IC_Val1 = 0;
uint32_t IC_Val2 = 0;
uint32_t Difference = 0;
uint8_t Is_First_Captured = 0;	//1st value captured
uint8_t Distance = 5;

void HCSR04_Read (void)
{
	HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);  // pull the TRIG pin HIGH
	usDelay(10);  // wait for 10 us
	HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);  // pull the TRIG pin low
	__HAL_TIM_ENABLE_IT(&htim5, TIM_IT_CC2);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM8_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART3_UART_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  gyroInit();
  HAL_UART_Receive_IT(&huart3, (uint8_t *)aRxBuffer, 10);
  HAL_TIM_IC_Start_IT(&htim5, TIM_CHANNEL_2);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of show */
  showHandle = osThreadNew(showMsg, NULL, &show_attributes);

  /* creation of MotorTask */
  MotorTaskHandle = osThreadNew(motors, NULL, &MotorTask_attributes);

  /* creation of EncoderTask */
  EncoderTaskHandle = osThreadNew(encoder, NULL, &EncoderTask_attributes);

  /* creation of GyroTask */
  GyroTaskHandle = osThreadNew(gryo_task, NULL, &GyroTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 160;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 0;
  if (HAL_TIM_Encoder_Init(&htim2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_Encoder_InitTypeDef sConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  sConfig.EncoderMode = TIM_ENCODERMODE_TI12;
  sConfig.IC1Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC1Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC1Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC1Filter = 10;
  sConfig.IC2Polarity = TIM_ICPOLARITY_RISING;
  sConfig.IC2Selection = TIM_ICSELECTION_DIRECTTI;
  sConfig.IC2Prescaler = TIM_ICPSC_DIV1;
  sConfig.IC2Filter = 10;
  if (HAL_TIM_Encoder_Init(&htim3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 16-1;
  htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim5.Init.Period = 65535;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim5, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 0;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 7199;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, AIN2_Pin|AIN1_Pin|BIN1_Pin|BIN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OLED_SCL_Pin OLED_SDA_Pin OLED_RST_Pin OLED_DC_Pin
                           LED3_Pin */
  GPIO_InitStruct.Pin = OLED_SCL_Pin|OLED_SDA_Pin|OLED_RST_Pin|OLED_DC_Pin
                          |LED3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : AIN2_Pin AIN1_Pin */
  GPIO_InitStruct.Pin = AIN2_Pin|AIN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BIN1_Pin BIN2_Pin */
  GPIO_InitStruct.Pin = BIN1_Pin|BIN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : TRIG_Pin */
  GPIO_InitStruct.Pin = TRIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TRIG_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){

	UNUSED(huart);

	//enqueue(&q,aRxBuffer);

	HAL_UART_Transmit(&huart3,(uint8_t *) aRxBuffer,10,0xFFFF);
	HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
}



//linked list code
void reset_motorVal(){
	// Reset Values
	frontback = 'w';	// Front/back character
	fb_speed = '0';	// Front/back speed
	leftright = 'a';	// Left/right character
	lr_speed = '0';	// Left/right speed
}

//input stuff into the queue
void enqueue(Queue *qPtr, uint8_t msg[4]){
    QueueNode *newNode;
    newNode = (QueueNode *) malloc(sizeof(QueueNode));
    for(int i=0; i<4; i++){
        newNode->msg[i] = msg[i];
    }
    newNode->next = NULL;

    if(isEmptyQueue(*qPtr))
        qPtr->head=newNode;
    else
        qPtr->tail->next = newNode;

    qPtr->tail = newNode;
    qPtr->size++;
}
int dequeue(Queue *qPtr){
    if(qPtr==NULL || qPtr->head==NULL){ //Queue is empty or NULL pointer
        return 0;
    }
    else{
       QueueNode *temp = qPtr->head;
       qPtr->head = qPtr->head->next;
       if(qPtr->head == NULL) //Queue is emptied
           qPtr->tail = NULL;

       free(temp);
       qPtr->size--;
       return 1;
    }
}

//get the front of the queue (not sure if working)
void getFront(Queue q){
        frontback = (uint8_t)(q.head->msg[0]);
        fb_speed = (uint8_t)(q.head->msg[1]);
        leftright = (uint8_t)(q.head->msg[2]);
        lr_speed = (uint8_t)(q.head->msg[3]);
}

//check if queue is empty (output 1 if empty, 0 if not empty)
int isEmptyQueue(Queue q){
    if(q.size==0) return 1;
    else return 0;
}

//delete the whole queue
void deleteQueue(Queue *qPtr)
{
    while(dequeue(qPtr));
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  uint8_t ch = 'A';
  for(;;)
  {
	HAL_UART_Transmit(&huart3, (uint8_t *)&ch,1, 0xFFFF);
	if(ch<'Z')
		ch++;
	else ch = 'A';

	HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
	osDelay(1000);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_showMsg */
/**
* @brief Function implementing the show thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_showMsg */
void showMsg(void *argument)
{
  /* USER CODE BEGIN showMsg */
  uint8_t test[20] = "testing\0";

  /* Infinite loop */
  for(;;)
  {
   sprintf(test, "%s\0", aRxBuffer);
   OLED_ShowString(10,20,test);
   OLED_Refresh_Gram();
   osDelay(1000);
  }
  /* USER CODE END showMsg */
}

/* USER CODE BEGIN Header_motors */
/**
* @brief Function implementing the MotorTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_motors */
void motors(void *argument)
{
	/* USER CODE BEGIN motor */
	uint16_t servo_max = 5;		// Servo_max * (0-9) = servo_value

	// For differential steering
	double motor_offset_r = 1;
	double motor_offset_l = 1;

	uint16_t pwmVal_motor = 0;	// Current motor pwm value
	uint16_t motor_min = 1000;// Min value for pwm to complete 2 instruction without stopping
	uint16_t motor_increment = 100;
	uint8_t accelerate;

	uint16_t motor_reference;	// Reference pwm value for motor
	uint32_t target_dist;	// Distance to travel

	// PID Values
	uint8_t kp = 3;
	uint8_t ki = 0.8;

	uint8_t kp_back = 2;

	// Distance Values
	uint8_t grad = 160;
	uint8_t y_intercept = 55;

	int16_t eintegral = 0;	// Integral error
	int32_t err;			// To total error for Integral
	int back_angle_threshold;

	// Set servo value to centre
	uint8_t servo_val = pwmVal_servo;

	// Adds to servo middle value to find the goddamn pictures
	uint8_t search_dir = 0;

	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

	int turn_angle;
	int stuck;
	// Initialize your queue
	Queue q = { 0, NULL, NULL };

	// Define your instructions using the uint8_t arrays where:
	// msg[0] - Front/Back instruction ('w' for forward, 's' for backward, 'k' for stop)
	// msg[1] - Distance back and front
	// msg[2] - Left/Right instruction ('a' for left, 'd' for right, '0' for center)
	// msg[3] - Speed for turning

	// Create instructions
	uint8_t reset[] = {'z', '0', '0', '0'}; // Stop
	uint8_t stop[] = {'k', '0', '0', '0'}; // Stop
	//uint8_t F3[] = {'w', '1', '0', '0'}; // Move forward with speed level 3 and turn left with speed level 4
	uint8_t up_right_90[] = {'w', '5', 'd','8'}; // Move forward with speed level 3 and turn left with speed level 4
	uint8_t up_right_45[] = {'w', '5', 'd','5'}; // Move forward with speed level 3 and turn left with speed level 4
	uint8_t up_left_90[] = {'w', '5', 'a','9'}; // Move forward with speed level 3 and turn left with speed level 4
	uint8_t up_left_45[] = {'w', '5', 'a','5'}; // Move forward with speed level 3 and turn left with speed level 4
	uint8_t up_max[] = {'w', '9', '0','0'}; // Move forward with speed level 3 and turn left with speed level 4

	// Enqueue the instructions
	enqueue(&q, reset);
	enqueue(&q, up_right_90);
	enqueue(&q, up_max);
	enqueue(&q, up_left_45);
	enqueue(&q, up_max);
	enqueue(&q, up_right_45);
	enqueue(&q, up_max);
	enqueue(&q, up_right_45);

	/* Infinite loop */
	for (;;) {
		if (isEmptyQueue(q) != 1) {
			stuck = 0;
			uint8_t hello[20];

			// Setting values according to queue head
			getFront(q);
			// Reset Encoder distance measurement
			encoder_dist = 0;
			// Integral error
			eintegral = 0;

			// Default: Start with acceleration
			accelerate = 1;
			target_dist = (int) ((fb_speed - 48) * grad - y_intercept);
			if (target_dist <= 0)
				target_dist = 0;

			// Display direction of movement
			sprintf(hello, "Dir %c : %3d\0", frontback, (fb_speed - 48));
			OLED_ShowString(10, 30, hello);

			// Check if moving straight or turn
			if (lr_speed == '0') {
				//Move faster if straight
				motor_reference = 2800;
			}

			else
				motor_reference = 1600;

			// Turn Servo to desired position
			// Centre - offset for left turn
			if (leftright == 'a') {
				// Decrease in servo's position from center
				// -48 to convert ASCII to integer
				// servo_max declared at 5
				htim1.Instance->CCR4 = pwmVal_servo - 1.1 * (lr_speed - 48) * servo_max;
				// right motor offset
				// right motor spins more due to differential steering
				motor_offset_r = 0.03 * (lr_speed - 48) + 1;
				motor_offset_l = 1;

				// Front Gyro threshold
				if (frontback == 'w')
					// if traveling forward then angle = left turn angle
					turn_angle = turn_angle_l;
				// Back Gyro threshold
				else
					// angle = right turn angle
					turn_angle = turn_angle_r;

			} else if (leftright == 'd') {
				htim1.Instance->CCR4 = pwmVal_servo + 1.73 * (lr_speed - 48) * servo_max;
				// left motor offset
				motor_offset_r = 1;
				motor_offset_l = 0.03 * (lr_speed - 48) + 1;

				// Front Gyro Threshold
				if (frontback == 'w')
					turn_angle = turn_angle_r;
				// Back Gyro threshold
				else
					turn_angle = turn_angle_l;
			}

			pwmVal_motor = motor_min;

			// Move Motor forward (Normal)
			if (frontback == 'w') {
				for (;;) {
					// H-Bridge Circuit for AINx; 1 turn on, the other turns off
					// MOTOR A
					HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);

					// MOTOR B
					HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
					HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);

					// Going straight only
					if (lr_speed == '0') {
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						if (accelerate == 1) {
							// Accelerating
							pwmVal_motor += motor_increment;

							// Accelerate till hit reference speed before maintaining constant speed
							// Reference speed is determined above, based on if it is moving straight or turning
							if (pwmVal_motor >= motor_reference) {
								// Acc flag set to 0
								accelerate = 0;
								// PID for error adjustment
								while (encoder_dist < (int) target_dist * 0.95) {
									// Proportional Error: Output value proportional to current error value

									// Err: Difference between current angle and target angle
									// (0 in this case since it is moving straight)
									err = curAngle - 0;
									// Integral error: Accumulation of past error, removal of residual SSE that occurs with pure proportional control
									eintegral += err;

									// PID equation
									// Controller adjusts servo's position to correct any deviations from straight path
									// Pre-defined kp = 3; ki = 0.8;
									servo_val = (uint8_t) (pwmVal_servo + kp * err + ki * eintegral);

									// Error Correction: Set servo value to correct error
									htim1.Instance->CCR4 = servo_val;

									stuck++;
									if (stuck > 600) {
										encoder_dist = target_dist;
										break;
									}

									osDelay(10);
								}
							}
						}

						// Deceleration and Distance Monitoring
						else {
							// Gradual deceleration
							if (pwmVal_motor > motor_min)
								pwmVal_motor -= 5 * motor_increment;

							// Break movement loop once target distance is covered
							if (encoder_dist >= target_dist)
								break;
						}

					}// End of Straight movement

					// 45 deg turn
					// Distance Values: grad = 160; y_intercept = 55;
					else if (lr_speed < '6') {
						target_dist = (int) (2 * grad - y_intercept);

						gyroStart();
						osDelay(100);

						// Target Angle Threshold: Prevent overshooting
						turn_angle = 0.525 * turn_angle;

						// Set speed
						pwmVal_motor = (int) ((fb_speed - 48) * 400);

						/************ Stage 1 45 deg turn out *********************/
						// Move motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// Continuous check to move angle to threshold
						while (abs(curAngle) < turn_angle) {
							osDelay(10);
						}

						// Once Threshold (45 degrees) reached, turn servo to center
						htim1.Instance->CCR4 = pwmVal_servo;

						// Stop motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

						// curAngle: adjusted to account for overflow/underflow error in turn angle measurement
						// Prep for subsequent movements
						curAngle = curAngle > 0 ? curAngle - turn_angle : curAngle + turn_angle;

						// Wait for full stability
						osDelay(500);
						break;
						/************ Stage 2 Move forward and turn back straight *************/

						// Stage 1: Initial 45 degree turn
						// Stage 2: Forward Movement + Realignment

						/*// Move motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// PID Correction: Error adjustment
						while (encoder_dist < target_dist) {

							// Proportional error
							err = curAngle - 0;
							// Integral error
							eintegral += err;

							// PID equation
							servo_val = (uint8_t) (pwmVal_servo + kp * err + ki * eintegral);

							// Set servo value
							htim1.Instance->CCR4 = servo_val;// Turn servo to correct error
						}

						// Stop motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);*/
					}

					// Turning 90 degree
					else {
						gyroStart();
						osDelay(100);

						// Set speed
						pwmVal_motor = (int) ((fb_speed - 48) * 400);

						// Move motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// Move til angle threshold
						// Did not apply the same fractional adjustment for turn_angle
						while (abs(curAngle) < turn_angle) {
							osDelay(10);
							stuck++;
							if (stuck > 1000) {
								curAngle = curAngle > 0 ? curAngle - turn_angle : curAngle + turn_angle;
								break;
							}
						}

						// Once Threshold reached, turn servo centre
						htim1.Instance->CCR4 = pwmVal_servo;

						// Stop motor
						pwmVal_motor = 0;
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// curAngle: adjusted to account for overflow/underflow error in turn angle measurement
						// Prep for subsequent movements
						curAngle =	curAngle > 0 ? curAngle - turn_angle : curAngle + turn_angle;

						kp = 3;
						ki = 0.8;
						eintegral = 0;	// Integral error

						osDelay(1000);

						// Break movement loop
						break;
					}	// End of Turning

					// To let gyro have the task thread or else OS will only focus on motor thread
					osDelay(10);

				}
			}

			// Move Motor backwards(Normal)
			else if (frontback == 's') {
				for (;;) {
					// MOTOR A
					HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

					// MOTOR B
					HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

					// Going straight only
					if (lr_speed == '0') {
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						if (accelerate == 1) {

							pwmVal_motor += motor_increment;	// Accelerating

							if (pwmVal_motor >= motor_reference) {// Constant speed
								accelerate = 0;
								while (encoder_dist < (int) target_dist * 0.95) {
									// PID for error adjustment
									err = curAngle - 0;	// Proportional error
									eintegral += err;		// Integral error

									// PID equation (opposite correction)
									servo_val = (uint8_t) (pwmVal_servo - kp * err - ki * eintegral);

									// Set servo value
									htim1.Instance->CCR4 = servo_val;// Turn servo to correct error

									stuck++;
									if (stuck > 600) {
										encoder_dist = target_dist;
										break;
									}

									osDelay(10);
								}
							}
						}

						else {		// Decelerate
							if (pwmVal_motor > motor_min)
								pwmVal_motor -= 5 * motor_increment;

							// Break movement loop
							if (encoder_dist >= target_dist)
								break;
						}

					}		// End of Straight movement

					// Turning
					else {
						gyroStart();
						osDelay(100);

						back_angle_threshold = (int) (0.95 * turn_angle);

						pwmVal_motor = (int) ((fb_speed - 48) * 400);

						// Move motor
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// Move til angle threshold
						while (abs(curAngle) < back_angle_threshold) { // Tends to over steer a lot
							osDelay(10);
							stuck++;
							if (stuck > 1000) {
								curAngle = curAngle > 0 ? curAngle - back_angle_threshold : curAngle + back_angle_threshold;
								break;
							}
						}

						// Once Threshold reached, turn servo centre
						htim1.Instance->CCR4 = pwmVal_servo; // Turn servo to the centre

						// Stop motor
						pwmVal_motor = 0;
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, motor_offset_r*pwmVal_motor);
						__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, motor_offset_l*pwmVal_motor);

						// Let overflow be error to account for
						curAngle = curAngle > 0 ? curAngle - back_angle_threshold : curAngle + back_angle_threshold;

						kp = kp_back;
						ki = 0.8;
						eintegral = 0;	// Integral error

						osDelay(1000);

						// Break movement loop
						break;
					}	// End of Turning

					// To let gyro have the task thread or else OS will only focus on motor thread
					osDelay(10);
				}
			}

			// No motor/reset values and start gyro
			else if (frontback == 'k') {
				// Reset all values
				encoder_offset = 0;
				encoder_error = 0;
				curAngle = 0;
				pwmVal_motor = 0;

				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

				osDelay(1500);
				gyroStart();					// Start Gyro Calibration

				osDelay((fb_speed - 48) * 50);	// Additional delay if required
			}

			// Move backwards (Slow)
			else if (frontback == 'y') {

				// E.g. 8cm back movement, target_dist = 80
				target_dist = (int) ((fb_speed - 48) * 10);
				// Slow down reference
				pwmVal_motor = motor_min;

				// MOTOR A
				HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

				// MOTOR B
				HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
				HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

				// Move Motor
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

				while (encoder_dist < (int) target_dist * 0.95) {
					// PID for error adjustment
					err = curAngle - 0;		// Proportional error
					eintegral += err;		// Integral error

					// PID equation (opposite correction)
					servo_val = (uint8_t) (pwmVal_servo - 0.8 * (kp * err + ki * eintegral));

					// Set servo value
					htim1.Instance->CCR4 = servo_val;// Turn servo to correct error

					osDelay(1);
				}

				// Stop motor
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

			}

			// Move forwards (Slow)
			else if (frontback == 'u') {
				target_dist = (int) ((fb_speed - 48) * 10);

				// Slow down reference
				pwmVal_motor = motor_min;

				// MOTOR A
				HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);

				// MOTOR B
				HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);

				// Move motor
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);

				while (encoder_dist < (int) target_dist * 0.95) {
					// PID for error adjustment
					err = curAngle - 0;		// Proportional error
					eintegral += err;		// Integral error

					// PID equation
					servo_val = (uint8_t) (pwmVal_servo + 0.8 * (kp * err + ki * eintegral));

					htim1.Instance->CCR4 = servo_val;

					osDelay(1);
				}

				// Stop motor
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
				__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

			}

			else if (frontback == 'l') {
				// Outside
				if (fb_speed == '1') {
					grad = 142;
					y_intercept = 50;
				}

				else {
					grad = 163;
					y_intercept = 55;
					kp_back = 5;
				}
			}

			// No Image found (Emergency Fail Safe)
			else if (frontback == 'n') {
				encoder_dist = 0;

				int8_t angle_to_turn;

				// Initialise motor
				// MOTOR A
				HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_RESET);

				// MOTOR B
				HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_RESET);

				// Clockwise search
				if (search_dir == 0) {
					// Turn the wheel
					htim1.Instance->CCR4 = pwmVal_servo + 40;// similar to u5a0

					// Move motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 800);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 2600);

					angle_to_turn = curAngle - (int) (turn_angle / 2);

					// Move til angle threshold
					while (curAngle > angle_to_turn) // Turn 45 degree
						osDelay(10);

					// Stop motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

					osDelay(500);

					htim1.Instance->CCR4 = pwmVal_servo;

					// Let it go back same amount
					target_dist = 0.9 * encoder_dist;
					encoder_dist = 0;

					// Reverse Motor
					// MOTOR A
					HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

					// MOTOR B
					HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

					osDelay(10);

					// Move motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 2000);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 2000);

					while (encoder_dist < target_dist) {
						osDelay(10);
					}

					// Stop motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

					search_dir = !search_dir;
					//					// Turned
					//					if(abs(curAngle) > abs(turn_angle)){
				}

				else {
					// Turn the wheel
					htim1.Instance->CCR4 = pwmVal_servo - 30;// similar to u5a0

					// Move motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 2600);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 800);

					angle_to_turn = curAngle + (int) (turn_angle / 2);

					// Move til angle threshold
					while (curAngle < angle_to_turn) // Turn 45 degree
						osDelay(10);

					// Stop motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

					osDelay(500);

					htim1.Instance->CCR4 = pwmVal_servo;

					// Let it go back same amount
					target_dist = 0.9 * encoder_dist;
					encoder_dist = 0;

					// Reverse Motor
					// MOTOR A
					HAL_GPIO_WritePin(GPIOA, AIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, AIN1_Pin, GPIO_PIN_SET);

					// MOTOR B
					HAL_GPIO_WritePin(GPIOA, BIN2_Pin, GPIO_PIN_RESET);
					HAL_GPIO_WritePin(GPIOA, BIN1_Pin, GPIO_PIN_SET);

					osDelay(10);

					// Move motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 2000);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 2000);

					while (encoder_dist < target_dist) {
						osDelay(10);
					}

					// Stop motor
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, 0);
					__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, 0);

					if (curAngle >= turn_angle / 2)
						search_dir = !search_dir;
				}

			}

			// Read Ultrasonic Sensor for distance
			else if (frontback == 'h') {
				osDelay(100);
				HCSR04_Read();

				msg[1] = Distance;
				if (HAL_UART_Transmit(&huart3, (uint8_t*) msg, 2, 0xFFFF) == HAL_OK)
					HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
			}

			// Send Message to Rpi to signify completion
			else if (frontback == 'm') {
				osDelay(100);

				msg[0] = 'e';
				for (int j = 0; j < 4; j++) {
					HAL_UART_Transmit(&huart3, (uint8_t*) msg, 2, 0xFFFF);
					osDelay(100);
				}
				HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);

				msg[0] = 'w';
			}

			else if (frontback == 'z') {
				osDelay(100);
				htim1.Instance->CCR4 = 198;
				osDelay(1000);
				htim1.Instance->CCR4 = 98;
				osDelay(1000);
				htim1.Instance->CCR4 = pwmVal_servo;
				osDelay(500);
				HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin);
			}

			osDelay(10);
			dequeue(&q);

		}	// ENDIF Queue not empty

		// Queue empty
		else {
			reset_motorVal();	//Reset the values
			pwmVal_motor = 0;

			htim1.Instance->CCR4 = pwmVal_servo;	// Reset Servo values

			// Stop motor
			__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_1, pwmVal_motor);
			__HAL_TIM_SetCompare(&htim8, TIM_CHANNEL_2, pwmVal_motor);
		}

	}
	/* USER CODE END motor */

}

/* USER CODE BEGIN Header_encoder */
/**
* @brief Function implementing the EncoderTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_encoder */
void encoder(void *argument)
{
  /* USER CODE BEGIN encoder */

  //Initializes Timer 2 to start counting pulses from encoder on input channels
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);

  //Store encoder counts at different times
  int cnt1, cnt3;
  //hold difference in encoder counts, used to calculate speed
  int diffa=0, diffb=0;
  //store timestamp for timing purpose
  uint32_t tick;
  //format strings for OLED
  uint8_t msg[20];

  //obtain current system time in ms
  tick = HAL_GetTick();

  /* Infinite loop */
  //checks encoder's position and calculating speed based on position change over time
  for(;;)
  {
		// Every 1000 ticks, get reading(How fast wheel turn)
		if(HAL_GetTick()-tick > 10){

			// At rising edge, counter increase by 1
			cnt1 = __HAL_TIM_GET_COUNTER(&htim2);
			cnt3 = __HAL_TIM_GET_COUNTER(&htim3);

			/* Motor A */
			// Counting up; Motor moving forward
			// 32500 is the max tick
			if (cnt1 - 32500 > 0) {
				diffa = cnt1 - 65535;
			}
			// Counting down; Motor moving backward
			else if (cnt1 - 32500 < 0) {
				diffa = cnt1;
			}

			/* Motor B */
			// Counting up; Motor moving backward
			if (cnt3 - 32500 > 0) {
				diffb = (cnt3 - 65535);
			}
			// Counting down; Motor moving forward
			else if (cnt3 - 32500 < 0) {
				diffb = cnt3;
			}

			encoder_error = diffa + diffb;
			// Updating of total distance
			encoder_dist += (abs(diffa) + abs(diffb));

			//sprintf(msg, "Diff:%3d\0", encoder_error);
			//OLED_ShowString(10, 30, msg);

			//Distance (meters)= (PPR×Gear Ratio/Encoder Counts)*Wheel Circumference (meters)
			sprintf(msg, "Dist:%3d\0", encoder_dist);
			OLED_ShowString(10, 40, msg);

			// Reset base tick
			__HAL_TIM_SET_COUNTER(&htim2, 0);
			__HAL_TIM_SET_COUNTER(&htim3, 0);

			tick = HAL_GetTick();

			osDelay(10);
		}
	}
  /* USER CODE END encoder */
}

/* USER CODE BEGIN Header_gryo_task */
/**
* @brief Function implementing the GyroTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_gryo_task */
void gryo_task(void *argument)
{
	/* USER CODE BEGIN gyro_task */
	/* Infinite loop */
	uint8_t val[2] = { 0, 0 }; // To store ICM gyro values
	gyroInit();
	int16_t angular_speed = 0;
	int16_t angle = 0;

	curAngle = 0;
	uint32_t tick = HAL_GetTick();
	osDelay(100);
	for (;;) {
		uint8_t msg[8];
		readByte(0x37, val);
		angular_speed = (val[0] << 8) | val[1];	// appending the 2 bytes together
		angle = ((double) (angular_speed * (100)) / 10000.0); //1.69

		curAngle += angle;

		sprintf(msg, "gyro : %3d\0", (int) curAngle);
		OLED_ShowString(10, 20, msg);

		osDelay(100);
	}
	/* USER CODE END gyro_task */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
