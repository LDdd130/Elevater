#include "stepper.h"
#include "door.h"
#include "ultrasonic.h"

#include <stdio.h>


static const uint8_t FULL_STEP_SEQ[4][4] =
{
        {1, 1, 0, 0},
        {0, 1, 1, 0},
        {0, 0, 1, 1},
        {1, 0, 0, 1}
};

static uint8_t currentStepIndex = 0;
static uint8_t stepperBusy = 0;
static uint8_t activeDirection = STEPPER_DIR_CW;
static uint32_t targetSteps = 0;
static uint32_t movedSteps = 0;
static uint32_t lastStepTick = 0;
static Stepper_ProgressCallback activeProgressCallback = NULL;

static uint8_t stepperProgressFlag = 0;
static uint32_t stepperProgressCurrent = 0;
static uint32_t stepperProgressTotal = 0;

/* Move state machine — main 컨텍스트 전용 */
static Stepper_MoveState moveState = STEPPER_MOVE_IDLE;
static uint8_t moveTargetFloor = 0;
static int32_t moveDegrees = 0;
static uint8_t moveTargetHitMax = 0;
static uint8_t moveTargetHitCount = 0;
static uint32_t moveLastSensorTick = 0;
static uint32_t moveStartTick = 0;
static Stepper_MoveState previousMoveState = STEPPER_MOVE_IDLE;
static uint8_t pendingMoveFloor = 0U;

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

static void Stepper_Release(void)
{
    HAL_GPIO_WritePin(STEPPER_IN1_GPIO_PORT, STEPPER_IN1_GPIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN2_GPIO_PORT, STEPPER_IN2_GPIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN3_GPIO_PORT, STEPPER_IN3_GPIO_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(STEPPER_IN4_GPIO_PORT, STEPPER_IN4_GPIO_PIN, GPIO_PIN_RESET);
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
    Stepper_Release();
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

void Stepper_Update(void)
{
    Stepper_MoveState currentMoveState = STEPPER_MOVE_IDLE;

    Stepper_Process();
    currentMoveState = Stepper_TickMove();

    if((currentMoveState == STEPPER_MOVE_ARRIVED) && (previousMoveState == STEPPER_MOVE_RUNNING))
    {
        open_door_request();
    }
    previousMoveState = currentMoveState;

    if((pendingMoveFloor != 0U) && (is_door_closed() != 0U) && (moveState != STEPPER_MOVE_RUNNING))
    {
        currentMoveState = Stepper_BeginMoveToFloor(pendingMoveFloor);
        if(currentMoveState == STEPPER_MOVE_ARRIVED)
        {
            open_door_request();
        }

        pendingMoveFloor = 0U;
    }
}

void Stepper_RequestMoveToFloor(uint8_t targetFloor)
{
    Stepper_MoveState currentMoveState = STEPPER_MOVE_IDLE;

    if(moveState == STEPPER_MOVE_RUNNING)
    {
        return;
    }

    if(is_door_closed() != 0U)
    {
        currentMoveState = Stepper_BeginMoveToFloor(targetFloor);
        if(currentMoveState == STEPPER_MOVE_ARRIVED)
        {
            open_door_request();
        }
    }
    else
    {
        pendingMoveFloor = targetFloor;
        close_door_request();
    }
}

Stepper_MoveState Stepper_MoveToFloor(uint8_t targetFloor)
{
    Stepper_BeginMoveToFloor(targetFloor);

    while(Stepper_TickMove() == STEPPER_MOVE_RUNNING)
    {
        update_door_nonblocking();
        Stepper_Process();
        /* busy-wait — 비동기 호출자는 Begin + Tick을 직접 사용할 것 */
    }

    return moveState;
}

