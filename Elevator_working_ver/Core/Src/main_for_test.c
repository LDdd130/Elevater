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
enum elevator_floor
{
    First_floor = 1,
    Second_floor,
    Third_floor
};

/*
 * 시연 기준 초기 위치는 1층으로 설정
 * 버튼을 누른 순간 current_floor를 바꾸지 말고,
 * 실제 도착 시 Stepper_Update() 쪽에서 갱신되도록 두는 것이 안정적임
 */
uint8_t current_floor = 1;
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
    if (huart->Instance == USART1)
    {
        /*
         * STM32F4에서 UART 에러(ORE, NE, FE, PE) 플래그를 안전하게 클리어
         */
        __IO uint32_t tmpreg = 0x00U;

        tmpreg = huart->Instance->SR;
        tmpreg = huart->Instance->DR;
        UNUSED(tmpreg);

        /*
         * 에러로 인해 멈춘 수신 인터럽트 재구동
         */
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

    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

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

    /*
     * 시연 시작 상태 표시가 필요하면 사용
     * 앱 디스플레이 3개를 모두 초기화해서 이전 값이 남지 않게 함
     */
    BT_SetAllDisplays("1F", "READY", "IDLE");

    /* FND / Buzzer 강제 테스트 */
//    first_floor_fnd();
//    start_floor_buzzer(383);
//    HAL_Delay(1000);
//
//    second_floor_fnd();
//    start_floor_buzzer(304);
//    HAL_Delay(1000);
//
//    third_floor_fnd();
//    start_floor_buzzer(255);
//    HAL_Delay(1000);
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

        /*
         * 1. non-blocking 모듈 업데이트
         */
        update_buzzer_nonblocking();
        update_door_nonblocking();

        /*
         * 2. 스텝모터 상태 업데이트
         *    도착 시 current_floor, move_done_flag, elapsed_time이 갱신되어야 함
         */
        Stepper_Update();

        /*
         * 3. 블루투스 입력 처리
         */
        BT_Process();

        /*
         * 4. 블루투스 이동 완료 처리
         */
        BT_MoveDone_Process();

        /*
         * 5. 물리 버튼 입력 처리
         *
         * 시연 기준:
         * - 1층에서 1층 버튼: 문 열림 시연
         * - 2층/3층 버튼: FND + 부저 + 스텝모터 이동
         * - 버튼 누른 순간 current_floor를 바꾸지 않음
         */
        if ((floor1Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
        {
            first_floor_fnd();
            start_floor_buzzer(383);

            if (current_floor == 1)
            {
                /*
                 * 1층에서 1층 버튼:
                 * 문 열림/닫힘 시연
                 */
                open_door_request();
                BT_SetAllDisplays("ARRIVED 1F", "TIME 0s", "DOOR OPEN");
            }
            else
            {
                /*
                 * 다른 층에서 1층 호출:
                 * 1층으로 내려감
                 * 1층 도착 후 Stepper_Update()에서 문 열림
                 */
                Stepper_RequestMoveToFloor(1);
                BT_SetAllDisplays("CALL 1F", "MOVING", "TO 1F");
            }

            HAL_Delay(200);
        }
        else if ((floor2Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
        {
            /*
             * 2층 호출:
             * 문 열림 없음
             * FND + 부저 + 이동만
             */
            second_floor_fnd();
            start_floor_buzzer(304);

            Stepper_RequestMoveToFloor(2);
            BT_SetAllDisplays("CALL 2F", "MOVING", "TO 2F");

            HAL_Delay(200);
        }
        else if ((floor3Pressed != 0U) && (Stepper_GetMoveState() != STEPPER_MOVE_RUNNING))
        {
            /*
             * 3층 호출:
             * 문 열림 없음
             * FND + 부저 + 이동만
             */
            third_floor_fnd();
            start_floor_buzzer(255);

            Stepper_RequestMoveToFloor(3);
            BT_SetAllDisplays("CALL 3F", "MOVING", "TO 3F");

            HAL_Delay(200);
        }
        /*
         * 6. 버튼 edge detection 갱신
         */
        lastFloor1Pin = floor1Pin;
        lastFloor2Pin = floor2Pin;
        lastFloor3Pin = floor3Pin;

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

    /** Configure the main internal regulator output voltage */
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

    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;

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
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
