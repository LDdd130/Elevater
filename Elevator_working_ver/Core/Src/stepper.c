#include "stepper.h"
#include "ultrasonic.h"

#include <stdio.h>
#include <stdlib.h>


static const uint8_t FULL_STEP_SEQ[4][4] =
{
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 1},
        {1, 0, 0, 1}
};

/* ISR(HAL_IncTick → Stepper_Process)와 main 양쪽에서 접근 → volatile 필수 */
static volatile uint8_t currentStepIndex = 0;
static volatile uint8_t stepperBusy = 0;
static volatile uint8_t activeDirection = STEPPER_DIR_CW;
static volatile uint32_t targetSteps = 0;
static volatile uint32_t movedSteps = 0;
static volatile uint32_t lastStepTick = 0;
static volatile Stepper_ProgressCallback activeProgressCallback = NULL;

static volatile uint8_t stepperProgressFlag = 0;
static volatile uint32_t stepperProgressCurrent = 0;
static volatile uint32_t stepperProgressTotal = 0;

/* Move state machine — main 컨텍스트 전용 */
static Stepper_MoveState moveState = STEPPER_MOVE_IDLE;
static uint8_t moveTargetFloor = 0;
static int32_t moveDegrees = 0;
static uint8_t moveTargetHitMax = 0;
static uint8_t moveTargetHitCount = 0;
static uint32_t moveLastSensorTick = 0;
static uint32_t moveStartTick = 0;

/* HAL_IncTick을 stepper.c에서 재정의하여 it.c/main.c 수정 없이 1ms 인터럽트로 모터 구동 */
extern __IO uint32_t uwTick;
extern HAL_TickFreqTypeDef uwTickFreq;

void HAL_IncTick(void)
{
    uwTick += uwTickFreq;
    Stepper_Process();
}

static void Stepper_OnProgress(uint32_t currentStep, uint32_t totalSteps)
{
    stepperProgressCurrent = currentStep;
    stepperProgressTotal = totalSteps;
    stepperProgressFlag = 1;
}

static uint32_t Stepper_AbsInt32(int32_t value)
{
    if(value < 0)
    {
        return (uint32_t)(-(int64_t)value);
    }

    return (uint32_t)value;
}

uint32_t Stepper_AngleToSteps(uint32_t degrees)
{
    return (uint32_t)((((uint64_t)degrees * STEPPER_STEPS_PER_REVOLUTION) + 180U) / 360U);
}

void Stepper_WriteStep(uint8_t step)
{
    uint8_t seqIndex = step % 4U;

    HAL_GPIO_WritePin(STEPPER_IN1_GPIO_PORT, STEPPER_IN1_GPIO_PIN, FULL_STEP_SEQ[seqIndex][0]);
    HAL_GPIO_WritePin(STEPPER_IN2_GPIO_PORT, STEPPER_IN2_GPIO_PIN, FULL_STEP_SEQ[seqIndex][1]);
    HAL_GPIO_WritePin(STEPPER_IN3_GPIO_PORT, STEPPER_IN3_GPIO_PIN, FULL_STEP_SEQ[seqIndex][2]);
    HAL_GPIO_WritePin(STEPPER_IN4_GPIO_PORT, STEPPER_IN4_GPIO_PIN, FULL_STEP_SEQ[seqIndex][3]);
}

void Stepper_StartRotateSteps(uint32_t steps, uint8_t direction, Stepper_ProgressCallback progressCallback)
{
    targetSteps = steps;
    movedSteps = 0;
    activeDirection = direction;
    activeProgressCallback = progressCallback;
    stepperBusy = (steps > 0U) ? 1U : 0U;

    Stepper_WriteStep(currentStepIndex);
    lastStepTick = HAL_GetTick() + STEPPER_SETTLE_DELAY_MS;
}

void Stepper_StartRotateDegrees(int32_t degrees, Stepper_ProgressCallback progressCallback)
{
    uint8_t direction = (degrees >= 0) ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
    uint32_t absDegrees = Stepper_AbsInt32(degrees);
    uint32_t steps = Stepper_AngleToSteps(absDegrees);

    Stepper_StartRotateSteps(steps, direction, progressCallback);
}

uint8_t Stepper_Process(void)
{
    if(stepperBusy == 0U)
    {
        return 0;
    }

    if((int32_t)(HAL_GetTick() - lastStepTick) < 0)
    {
        return 1;
    }

    if(activeDirection == STEPPER_DIR_CW)
    {
        currentStepIndex = (currentStepIndex + 1U) % 4U;
    }
    else
    {
        currentStepIndex = (currentStepIndex + 3U) % 4U;
    }

    Stepper_WriteStep(currentStepIndex);
    movedSteps++;
    lastStepTick = HAL_GetTick() + STEPPER_STEP_DELAY_MS;

    if(activeProgressCallback != NULL && ((movedSteps % 64U) == 0U || movedSteps == targetSteps))
    {
        activeProgressCallback(movedSteps, targetSteps);
    }

    if(movedSteps >= targetSteps)
    {
        stepperBusy = 0U;
    }

    return stepperBusy;
}

uint8_t Stepper_IsBusy(void)
{
    return stepperBusy;
}

static int32_t Stepper_GetMoveDegrees(uint8_t currentFloor, uint8_t targetFloor)
{
    if(targetFloor > currentFloor)
    {
        return STEPPER_FLOOR_UP_DEGREES;
    }

    return STEPPER_FLOOR_DOWN_DEGREES;
}

static void Stepper_Stop(void)
{
    Stepper_StartRotateSteps(0U, STEPPER_DIR_CW, NULL);
}

Stepper_MoveState Stepper_BeginMoveToFloor(uint8_t targetFloor)
{
    uint16_t distanceCH1 = 0;
    uint16_t distanceCH2 = 0;
    uint8_t currentFloor = 0;

    /* C: 인자 검증 — 잘못된 층은 즉시 거부 */
    if(targetFloor < 1U || targetFloor > 3U)
    {
        printf("Invalid target floor: %u\n\r\n\r", targetFloor);
        moveState = STEPPER_MOVE_INVALID;
        return moveState;
    }

    Ultrasonic_ReadBoth(&distanceCH1, &distanceCH2);
    currentFloor = Ultrasonic_GetCurrentFloor(distanceCH1, distanceCH2);
    printf("CH1: %u mm, CH2: %u mm, current floor: %d\n\r", distanceCH1, distanceCH2, currentFloor);

    if(currentFloor == targetFloor)
    {
        printf("Already floor %d.\n\r\n\r", targetFloor);
        moveState = STEPPER_MOVE_ARRIVED;
        return moveState;
    }

    moveTargetFloor = targetFloor;
    moveDegrees = Stepper_GetMoveDegrees(currentFloor, targetFloor);
    moveTargetHitMax = Ultrasonic_GetTargetHitCount(targetFloor);
    moveTargetHitCount = 0;
    moveLastSensorTick = HAL_GetTick();
    moveStartTick = HAL_GetTick();

    if(moveDegrees < 0)
    {
        printf("Move up CCW to floor %d.\n\r", targetFloor);
    }
    else
    {
        printf("Move down CW to floor %d.\n\r", targetFloor);
    }

    Stepper_StartRotateDegrees(moveDegrees, Stepper_OnProgress);
    moveState = STEPPER_MOVE_RUNNING;
    return moveState;
}

Stepper_MoveState Stepper_TickMove(void)
{
    uint16_t distanceCH1 = 0;
    uint16_t distanceCH2 = 0;

    if(moveState != STEPPER_MOVE_RUNNING)
    {
        return moveState;
    }

    /* B: 타임아웃 — 끼임/센서 사각지대로 도착 못 했을 때 안전 정지 */
    if((HAL_GetTick() - moveStartTick) >= STEPPER_MOVE_TIMEOUT_MS)
    {
        Stepper_Stop();
        printf("Move timeout (%lu ms). Stopped at hit %d/%d.\n\r\n\r",
               (unsigned long)STEPPER_MOVE_TIMEOUT_MS,
               moveTargetHitCount,
               moveTargetHitMax);
        moveState = STEPPER_MOVE_TIMEOUT;
        return moveState;
    }

    /* 회전 한 사이클 끝났으면 재시작 (도착 판정 전까지 계속 회전) */
    if(Stepper_IsBusy() == 0U)
    {
        Stepper_StartRotateDegrees(moveDegrees, Stepper_OnProgress);
    }

    /* 센서 측정 시점: 주기 도달 or 스텝 진행 콜백 플래그 */
    if((HAL_GetTick() - moveLastSensorTick) >= ULTRASONIC_SENSOR_INTERVAL_MS || stepperProgressFlag == 1U)
    {
        stepperProgressFlag = 0;
        moveLastSensorTick = HAL_GetTick();

        Ultrasonic_ReadBoth(&distanceCH1, &distanceCH2);

        if(Ultrasonic_IsTargetReached(moveTargetFloor, distanceCH1, distanceCH2) != 0U)
        {
            moveTargetHitCount++;
            printf("Target hit %d/%d, CH1: %u mm, CH2: %u mm\n\r",
                   moveTargetHitCount,
                   moveTargetHitMax,
                   distanceCH1,
                   distanceCH2);

            if(moveTargetHitCount >= moveTargetHitMax)
            {
                Stepper_Stop();
                printf("Arrived floor %d.\n\r\n\r", moveTargetFloor);
                moveState = STEPPER_MOVE_ARRIVED;
                return moveState;
            }
        }
    }

    return moveState;
}

Stepper_MoveState Stepper_GetMoveState(void)
{
    return moveState;
}

Stepper_MoveState Stepper_MoveToFloor(uint8_t targetFloor)
{
    Stepper_BeginMoveToFloor(targetFloor);

    while(Stepper_TickMove() == STEPPER_MOVE_RUNNING)
    {
        /* busy-wait — 비동기 호출자는 Begin + Tick을 직접 사용할 것 */
    }

    return moveState;
}

/* ============================================================
 * [DEBUG] UART 기반 동작 확인용 함수 모음
 *  - 머지 시 본 블록(아래 두 함수)을 통째로 주석 처리하거나 삭제해도 됨
 *  - 핵심 기능(Stepper_MoveToFloor 등)에는 영향 없음
 *  - 헤더에서도 함께 빼야 함: Stepper_ReadAngleFromUart, Stepper_RotateAngleTest
 * ============================================================ */

uint8_t Stepper_ReadAngleFromUart(UART_HandleTypeDef *huart, int32_t *angle)
{
    char rxBuffer[16] = {0};
    uint8_t rxByte = 0;
    uint8_t rxIndex = 0;
    uint8_t hasDigit = 0;
    char *endPtr = NULL;

    while(1)
    {
        if(HAL_UART_Receive(huart, &rxByte, 1, HAL_MAX_DELAY) == HAL_OK)
        {
            if(rxByte == '\r' || rxByte == '\n')
            {
                if(rxIndex > 0)
                {
                    rxBuffer[rxIndex] = '\0';
                    break;
                }
            }
            else if(rxByte == '\b' || rxByte == 0x7F)
            {
                if(rxIndex > 0)
                {
                    if(rxBuffer[rxIndex - 1] >= '0' && rxBuffer[rxIndex - 1] <= '9')
                    {
                        hasDigit = 0;
                        for(uint8_t i = 0; i < rxIndex - 1; i++)
                        {
                            if(rxBuffer[i] >= '0' && rxBuffer[i] <= '9')
                            {
                                hasDigit = 1;
                                break;
                            }
                        }
                    }

                    rxIndex--;
                    HAL_UART_Transmit(huart, (uint8_t *)"\b \b", 3, HAL_MAX_DELAY);
                }
            }
            else if((rxByte >= '0' && rxByte <= '9') || (rxByte == '-' && rxIndex == 0))
            {
                if(rxIndex < (sizeof(rxBuffer) - 1))
                {
                    rxBuffer[rxIndex++] = (char)rxByte;
                    if(rxByte >= '0' && rxByte <= '9')
                    {
                        hasDigit = 1;
                    }
                    HAL_UART_Transmit(huart, &rxByte, 1, HAL_MAX_DELAY);
                }
            }
        }
    }

    if(hasDigit == 0)
    {
        return 0;
    }

    *angle = (int32_t)strtol(rxBuffer, &endPtr, 10);

    return (*endPtr == '\0');
}

void Stepper_RotateAngleTest(UART_HandleTypeDef *huart)
{
    int32_t targetAngle = 0;

    printf("Enter angle (+/-): ");
    if(Stepper_ReadAngleFromUart(huart, &targetAngle) != 0 && targetAngle != 0)
    {
        printf("\n\rRotate %ld degree start.\n\r", (long)targetAngle);
        Stepper_StartRotateDegrees(targetAngle, Stepper_OnProgress);
        while(Stepper_IsBusy() != 0U)
        {
        }
        printf("Rotate done.\n\r\n\r");
    }
    else
    {
        printf("\n\rInvalid or 0 degree input.\n\r\n\r");
    }
}

/* ===== END DEBUG ===== */
