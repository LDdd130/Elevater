/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ULTRASONIC_Pin GPIO_PIN_5
#define ULTRASONIC_GPIO_Port GPIOA
#define ULTRASONIC_TIM_Pin GPIO_PIN_6
#define ULTRASONIC_TIM_GPIO_Port GPIOA
#define ULTRASONIC_TIMA7_Pin GPIO_PIN_7
#define ULTRASONIC_TIMA7_GPIO_Port GPIOA
#define BUTTON_Pin GPIO_PIN_5
#define BUTTON_GPIO_Port GPIOC
#define STEPPER_Pin GPIO_PIN_1
#define STEPPER_GPIO_Port GPIOB
#define FND_Pin GPIO_PIN_10
#define FND_GPIO_Port GPIOB
#define FNDB12_Pin GPIO_PIN_12
#define FNDB12_GPIO_Port GPIOB
#define STEPPERB13_Pin GPIO_PIN_13
#define STEPPERB13_GPIO_Port GPIOB
#define STEPPERB14_Pin GPIO_PIN_14
#define STEPPERB14_GPIO_Port GPIOB
#define STEPPERB15_Pin GPIO_PIN_15
#define STEPPERB15_GPIO_Port GPIOB
#define BUTTONC6_Pin GPIO_PIN_6
#define BUTTONC6_GPIO_Port GPIOC
#define BUTTONC8_Pin GPIO_PIN_8
#define BUTTONC8_GPIO_Port GPIOC
#define BUTTONC9_Pin GPIO_PIN_9
#define BUTTONC9_GPIO_Port GPIOC
#define FNDA8_Pin GPIO_PIN_8
#define FNDA8_GPIO_Port GPIOA
#define FNDA11_Pin GPIO_PIN_11
#define FNDA11_GPIO_Port GPIOA
#define FNDA12_Pin GPIO_PIN_12
#define FNDA12_GPIO_Port GPIOA
#define FNDB3_Pin GPIO_PIN_3
#define FNDB3_GPIO_Port GPIOB
#define FNDB4_Pin GPIO_PIN_4
#define FNDB4_GPIO_Port GPIOB
#define FNDB5_Pin GPIO_PIN_5
#define FNDB5_GPIO_Port GPIOB
#define BUZZER_TIM_Pin GPIO_PIN_6
#define BUZZER_TIM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
