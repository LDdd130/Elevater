#ifndef INC_STEPPER_H_
#define INC_STEPPER_H_

#include "main.h"


#define STEPPER_STEPS_PER_REVOLUTION    2048U    // U는 unsigned int 상수라는 뜻, 1바퀴 스텝 수
#define STEPPER_STEP_DELAY_MS           2U       // U는 unsigned int 상수라는 뜻, 스텝 사이 딜레이(ms)
#define STEPPER_SETTLE_DELAY_MS         10U      // U는 unsigned int 상수라는 뜻, 회전 시작 전 안정화 시간(ms)
#define STEPPER_DIR_CW                  0U       // U는 unsigned int 상수라는 뜻, 시계방향
#define STEPPER_DIR_CCW                 1U       // U는 unsigned int 상수라는 뜻, 반시계방향

#define STEPPER_IN1_GPIO_PORT   GPIOB
#define STEPPER_IN1_GPIO_PIN    GPIO_PIN_1
#define STEPPER_IN2_GPIO_PORT   GPIOB
#define STEPPER_IN2_GPIO_PIN    GPIO_PIN_13
#define STEPPER_IN3_GPIO_PORT   GPIOB
#define STEPPER_IN3_GPIO_PIN    GPIO_PIN_14
#define STEPPER_IN4_GPIO_PORT   GPIOB
#define STEPPER_IN4_GPIO_PIN    GPIO_PIN_15

/* 층 이동 회전량 (음수 = CCW = 상승, 양수 = CW = 하강) */
#define STEPPER_FLOOR_UP_DEGREES        -3600
#define STEPPER_FLOOR_DOWN_DEGREES       3600

/* 1회 이동 안전 타임아웃 (ms). 끼임/센서 사각지대 시 자동 정지 */
#define STEPPER_MOVE_TIMEOUT_MS         30000U

typedef void (*Stepper_ProgressCallback)(uint32_t currentStep, uint32_t totalSteps);

typedef enum
{
    STEPPER_MOVE_IDLE = 0,
    STEPPER_MOVE_RUNNING,
    STEPPER_MOVE_ARRIVED,
    STEPPER_MOVE_TIMEOUT,
    STEPPER_MOVE_INVALID
} Stepper_MoveState;

/* === 저수준 API (모터 제어 원시 함수, 내부용. 직접 호출은 비권장) === */
uint32_t Stepper_AngleToSteps(uint32_t degrees);
void Stepper_WriteStep(uint8_t step);
void Stepper_StartRotateSteps(uint32_t steps, uint8_t direction, Stepper_ProgressCallback progressCallback);
void Stepper_StartRotateDegrees(int32_t degrees, Stepper_ProgressCallback progressCallback);
uint8_t Stepper_Process(void);
uint8_t Stepper_IsBusy(void);

/* === 머지 인터페이스 (비동기 권장) ===
 *  사용 방법:
 *    Stepper_BeginMoveToFloor(target_floor);          // 한 번 호출
 *    while (...) {
 *        Stepper_MoveState s = Stepper_TickMove();    // 주 루프에서 매번 호출
 *        if (s == STEPPER_MOVE_ARRIVED) { move_done_flag = 1; ... }
 *        if (s == STEPPER_MOVE_TIMEOUT) { error_flag = 1; ... }
 *    }
 *  Tick 호출 사이에는 다른 작업(BT/버튼/FND/부저) 자유롭게 가능.
 *  단, 한 Tick 안에서 센서 측정이 필요할 때 80ms 블로킹 발생 (HAL_Delay).
 */
Stepper_MoveState Stepper_BeginMoveToFloor(uint8_t targetFloor);
Stepper_MoveState Stepper_TickMove(void);
Stepper_MoveState Stepper_GetMoveState(void);

/* 동기 래퍼: 도착/타임아웃까지 블로킹. 단순 케이스 및 디버그용 */
Stepper_MoveState Stepper_MoveToFloor(uint8_t targetFloor);

/* [DEBUG] UART 입력으로 임의 각도 회전 확인용 — 머지 시 삭제 가능 */
uint8_t Stepper_ReadAngleFromUart(UART_HandleTypeDef *huart, int32_t *angle);
void Stepper_RotateAngleTest(UART_HandleTypeDef *huart);


#endif /* INC_STEPPER_H_ */
