/* door.c */
#include "door.h"

// 문의 상태를 관리하는 전역 변수
static DoorControl my_door = {DOOR_CLOSED, 0};

/**
  * @brief  각 층에서 문을 열고 싶을 때 호출하는 함수 (인터럽트 기능 포함)
  */
void open_door_request(void)
{
    // 문이 완전히 닫혀있거나(CLOSED), 심지어 닫히는 도중(CLOSING)이더라도
    // 요청이 들어오면 즉시 문을 여는 단계로 전환합니다! (핵심 기능)
    if (my_door.state == DOOR_CLOSED || my_door.state == DOOR_CLOSING)
    {
        my_door.state = DOOR_OPENING;
        my_door.start_tick = HAL_GetTick(); // 시간 리셋

        // 문 열기 시작 (180도 방향으로 모터 제어)
        TIM11->CCR1 = 2500;
    }
}

/**
  * @brief  main 함수의 while문에서 계속 호출되며 문 타이밍을 제어하는 함수
  */
void update_door_nonblocking(void)
{
    uint32_t current_tick = HAL_GetTick();

    switch (my_door.state)
    {
        case DOOR_CLOSED:
            // 문이 완전히 닫힌 상태에서는 아무것도 하지 않고 대기합니다.
            break;

        case DOOR_OPENING:
            // 문이 열리기 시작한 후 2초(2000ms) 동안 모터가 다 돌 때까지 대기
            if (current_tick - my_door.start_tick >= 2000)
            {
                my_door.state = DOOR_OPENED;
                my_door.start_tick = current_tick; // 열린 채로 유지할 시간 측정 시작
            }
            break;

        case DOOR_OPENED:
            // 문이 완전히 열린 상태로 2초(2000ms) 동안 머무릅니다.
            if (current_tick - my_door.start_tick >= 2000)
            {
                my_door.state = DOOR_CLOSING;
                my_door.start_tick = current_tick; // 닫히는 시간 측정 시작

                // 문 닫기 시작 (0도 방향으로 모터 제어)
                TIM11->CCR1 = 500;
            }
            break;

        case DOOR_CLOSING:
            // 문이 완전히 닫힐 때까지 2초(2000ms) 동안 대기
            if (current_tick - my_door.start_tick >= 2000)
            {
                my_door.state = DOOR_CLOSED; // 완전히 닫힘 상태로 복귀
            }
            break;
    }
}
