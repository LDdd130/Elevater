#include "ultrasonic.h"

#include "delay.h"
#include "tim.h"

#include <stdio.h>

#define ULTRASONIC_TRIG_PORT   GPIOA
#define ULTRASONIC_TRIG_PIN    GPIO_PIN_5

static volatile uint16_t IC_Value1_CH1 = 0;
static volatile uint16_t IC_Value2_CH1 = 0;
static volatile uint16_t echoTime_CH1 = 0;
static volatile uint8_t captureFlag_CH1 = 0;
static volatile uint8_t distance_CH1 = 0;
static volatile uint16_t distanceMm_CH1 = 0;

static volatile uint16_t IC_Value1_CH2 = 0;
static volatile uint16_t IC_Value2_CH2 = 0;
static volatile uint16_t echoTime_CH2 = 0;
static volatile uint8_t captureFlag_CH2 = 0;
static volatile uint8_t distance_CH2 = 0;
static volatile uint16_t distanceMm_CH2 = 0;

void Ultrasonic_Init(void)
{
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
}

void Ultrasonic_Trigger(void)
{
  /* 이전 측정에서 echo 한쪽만 들어와 멈춘 경우 대비:
     캡처 상태머신을 RISING-대기 상태로 강제 초기화 */
  captureFlag_CH1 = 0;
  captureFlag_CH2 = 0;
  __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
  __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);

  HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(ULTRASONIC_TRIG_PORT, ULTRASONIC_TRIG_PIN, GPIO_PIN_RESET);

  __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_CC1);
  __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_CC2);
}

uint8_t Ultrasonic_GetDistance(Ultrasonic_Channel channel)
{
  if(channel == ULTRASONIC_CH1)
  {
    return distance_CH1;
  }

  return distance_CH2;
}

uint16_t Ultrasonic_GetDistanceMm(Ultrasonic_Channel channel)
{
  if(channel == ULTRASONIC_CH1)
  {
    return distanceMm_CH1;
  }

  return distanceMm_CH2;
}

void Ultrasonic_ReadBoth(uint16_t *distanceCH1, uint16_t *distanceCH2)
{
  Ultrasonic_Trigger();
  HAL_Delay(80);

  *distanceCH1 = Ultrasonic_GetDistanceMm(ULTRASONIC_CH1);
  *distanceCH2 = Ultrasonic_GetDistanceMm(ULTRASONIC_CH2);
}

static uint8_t Ultrasonic_IsSimilarDistance(uint16_t distanceCH1, uint16_t distanceCH2)
{
  if(distanceCH1 > distanceCH2)
  {
    return ((distanceCH1 - distanceCH2) <= ULTRASONIC_FLOOR_TOLERANCE_MM);
  }

  return ((distanceCH2 - distanceCH1) <= ULTRASONIC_FLOOR_TOLERANCE_MM);
}

static uint8_t Ultrasonic_IsNearTarget(uint16_t distance, uint16_t target)
{
  if(distance > target)
  {
    return ((distance - target) <= ULTRASONIC_FLOOR_TOLERANCE_MM);
  }

  return ((target - distance) <= ULTRASONIC_FLOOR_TOLERANCE_MM);
}

uint8_t Ultrasonic_IsTargetReached(uint8_t targetFloor, uint16_t distanceCH1, uint16_t distanceCH2)
{
  /* 1·3층도 ±ULTRASONIC_FLOOR_TOLERANCE_MM tolerance 적용 (센서 노이즈로 정확 일치 누락 방지)
     hit count(22/13)는 tolerance 적용으로 더 빨리 누적될 수 있음 → 필요 시 재튜닝 */
  if(targetFloor == 1)
  {
    return Ultrasonic_IsNearTarget(distanceCH1, ULTRASONIC_FLOOR1_CH1_TARGET_MM);
  }
  else if(targetFloor == 2)
  {
    return Ultrasonic_IsSimilarDistance(distanceCH1, distanceCH2);
  }
  else if(targetFloor == 3)
  {
    return Ultrasonic_IsNearTarget(distanceCH2, ULTRASONIC_FLOOR3_CH2_TARGET_MM);
  }

  return 0;
}

uint8_t Ultrasonic_GetTargetHitCount(uint8_t targetFloor)
{
  if(targetFloor == 1)
  {
    return ULTRASONIC_FLOOR1_HIT_COUNT;
  }
  else if(targetFloor == 3)
  {
    return ULTRASONIC_FLOOR3_HIT_COUNT;
  }

  return 1U;
}

uint8_t Ultrasonic_GetCurrentFloor(uint16_t distanceCH1, uint16_t distanceCH2)
{
  if(Ultrasonic_IsTargetReached(1, distanceCH1, distanceCH2) != 0U)
  {
    return 1;
  }
  else if(Ultrasonic_IsTargetReached(3, distanceCH1, distanceCH2) != 0U)
  {
    return 3;
  }
  else if(Ultrasonic_IsTargetReached(2, distanceCH1, distanceCH2) != 0U)
  {
    return 2;
  }

  if(distanceCH1 < distanceCH2)
  {
    return 1;
  }
  else if(distanceCH2 < distanceCH1)
  {
    return 3;
  }

  return 2;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
  {
    if(captureFlag_CH1 == 0)
    {
      IC_Value1_CH1 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);
      captureFlag_CH1 = 1;
      __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else if(captureFlag_CH1 == 1)
    {
      IC_Value2_CH1 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_1);

      if(IC_Value2_CH1 > IC_Value1_CH1)
      {
        echoTime_CH1 = IC_Value2_CH1 - IC_Value1_CH1;
      }
      else if(IC_Value1_CH1 > IC_Value2_CH1)
      {
        echoTime_CH1 = (0xFFFF - IC_Value1_CH1) + IC_Value2_CH1;
      }

      distance_CH1 = echoTime_CH1 / 58;
      distanceMm_CH1 = ((uint32_t)echoTime_CH1 * 10U) / 58U;
      captureFlag_CH1 = 0;
      __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_1, TIM_INPUTCHANNELPOLARITY_RISING);
      __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_CC1);
    }
  }

  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
  {
    if(captureFlag_CH2 == 0)
    {
      IC_Value1_CH2 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);
      captureFlag_CH2 = 1;
      __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else if(captureFlag_CH2 == 1)
    {
      IC_Value2_CH2 = HAL_TIM_ReadCapturedValue(&htim3, TIM_CHANNEL_2);

      if(IC_Value2_CH2 > IC_Value1_CH2)
      {
        echoTime_CH2 = IC_Value2_CH2 - IC_Value1_CH2;
      }
      else if(IC_Value1_CH2 > IC_Value2_CH2)
      {
        echoTime_CH2 = (0xFFFF - IC_Value1_CH2) + IC_Value2_CH2;
      }

      distance_CH2 = echoTime_CH2 / 58;
      distanceMm_CH2 = ((uint32_t)echoTime_CH2 * 10U) / 58U;
      captureFlag_CH2 = 0;
      __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
      __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_CC2);
    }
  }
}

/* ============================================================
 * [DEBUG] UART 거리 출력 — 층별 보정값(mm) 찾기용
 *  - 머지 시 본 블록을 통째로 주석 처리하거나 삭제해도 됨
 *  - 핵심 기능(Ultrasonic_ReadBoth, Ultrasonic_IsTargetReached 등)에는 영향 없음
 *  - 헤더에서도 Ultrasonic_PrintDistanceSamples 선언을 함께 빼야 함
 * ============================================================ */

void Ultrasonic_PrintDistanceSamples(void)
{
  uint16_t distanceCH1 = 0;
  uint16_t distanceCH2 = 0;

  for(uint8_t i = 0; i < 10U; i++)
  {
    Ultrasonic_ReadBoth(&distanceCH1, &distanceCH2);
    printf("[%d/10] CH1: %u mm, CH2: %u mm\n\r", i + 1U, distanceCH1, distanceCH2);
    HAL_Delay(120);
  }

  printf("\n\r");
}

/* ===== END DEBUG ===== */
