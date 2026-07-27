#ifndef INC_ULTRASONIC_H_
#define INC_ULTRASONIC_H_

#include "main.h"

/* 층별 거리 임계값 (mm) — 센서 보정값 */
#define ULTRASONIC_FLOOR1_CH1_TARGET_MM  23U
#define ULTRASONIC_FLOOR3_CH2_TARGET_MM  26U
#define ULTRASONIC_FLOOR_TOLERANCE_MM    2U
#define ULTRASONIC_SENSOR_INTERVAL_MS    120U
#define ULTRASONIC_FLOOR1_HIT_COUNT      22U
#define ULTRASONIC_FLOOR3_HIT_COUNT      13U

typedef enum
{
  ULTRASONIC_CH1 = 0,
  ULTRASONIC_CH2
} Ultrasonic_Channel;

void Ultrasonic_Init(void);
void Ultrasonic_Trigger(void);
uint8_t Ultrasonic_GetDistance(Ultrasonic_Channel channel);
uint16_t Ultrasonic_GetDistanceMm(Ultrasonic_Channel channel);

void Ultrasonic_ReadBoth(uint16_t *distanceCH1, uint16_t *distanceCH2);
uint8_t Ultrasonic_IsTargetReached(uint8_t targetFloor, uint16_t distanceCH1, uint16_t distanceCH2);
uint8_t Ultrasonic_GetTargetHitCount(uint8_t targetFloor);
uint8_t Ultrasonic_GetCurrentFloor(uint16_t distanceCH1, uint16_t distanceCH2);

/* [DEBUG] UART로 10회 연속 거리 출력 — 보정값 찾기용, 머지 시 삭제 가능 */
void Ultrasonic_PrintDistanceSamples(void);

#endif /* INC_ULTRASONIC_H_ */
