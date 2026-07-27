/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fnd.h"
#include "buzzer.h"
#include "door.h"
#include "stepper.h"
#include "ultrasonic.h"
#include "Header.h"
#include "BT_Serial.h"
#include "elevator.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

enum elevator_floor{
	First_floor = 1,
	Second_floor,
	Third_floor
};
uint8_t current_floor = 0;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    BT_RxCallback(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) // 블루투스가 연결된 USART 채널 (huart1)
    {
        /* STM32F4에서 UART 에러(ORE, NE, FE, PE) 플래그를 안전하게 클리어하는 코드 */
        __IO uint32_t tmpreg = 0x00U;
        tmpreg = huart->Instance->SR;  // SR 레지스터를 읽고
        tmpreg = huart->Instance->DR;  // DR 레지스터를 읽으면 에러 플래그가 하드웨어적으로 클리어됩니다.
        UNUSED(tmpreg);                // 컴파일러 미사용 변수 경고 방지

        // 에러로 인해 멈춘 수신 인터럽트 재구동
        extern uint8_t bt_rx_byte;
        HAL_UART_Receive_IT(huart, &bt_rx_byte, 1);
    }
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM11_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);
  HAL_TIM_Base_Start(&htim11);
  Ultrasonic_Init();
  BT_Init(&huart1);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  static GPIO_PinState lastFloor1Pin = GPIO_PIN_SET;
	  static GPIO_PinState lastFloor2Pin = GPIO_PIN_SET;
	  static GPIO_PinState lastFloor3Pin = GPIO_PIN_SET;
	  GPIO_PinState floor1Pin = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
	  GPIO_PinState floor2Pin = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_6);
	  GPIO_PinState floor3Pin = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_8);
	  uint8_t floor1Pressed = ((floor1Pin != GPIO_PIN_SET) && (lastFloor1Pin == GPIO_PIN_SET)) ? 1U : 0U;
	  uint8_t floor2Pressed = ((floor2Pin != GPIO_PIN_SET) && (lastFloor2Pin == GPIO_PIN_SET)) ? 1U : 0U;
	  uint8_t floor3Pressed = ((floor3Pin != GPIO_PIN_SET) && (lastFloor3Pin == GPIO_PIN_SET)) ? 1U : 0U;

	  update_buzzer_nonblocking();
	  update_door_nonblocking();
	  Stepper_Update();
	  BT_Process();

	  if((floor3Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
	  {
		  third_floor_fnd();

		  start_floor_buzzer(255);

		  Stepper_RequestMoveToFloor(3);

		  current_floor = Third_floor;
	  }
	  else if((floor2Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
	  {
		  second_floor_fnd();

		  start_floor_buzzer(304);

		  Stepper_RequestMoveToFloor(2);

		  current_floor = Second_floor;
	  }
	  else if((floor1Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
	  {
		  first_floor_fnd();

		  start_floor_buzzer(383);

		  Stepper_RequestMoveToFloor(1);

		  current_floor = First_floor;
	  }

	  lastFloor1Pin = floor1Pin;
	  lastFloor2Pin = floor2Pin;
	  lastFloor3Pin = floor3Pin;

	  BT_MoveDone_Process();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
