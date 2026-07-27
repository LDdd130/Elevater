#include "elevator.h"
#include "fnd.h"
#include "buzzer.h"

/*
 * Elevator common state variables
 */

extern uint8_t current_floor;                 // 초기 위치 1층
uint8_t open_flag = 0;             // 0: 문 닫힘/요청 없음, 1: 문 열림 요청
uint8_t move_done_flag = 0;        // 0: 이동 중/완료 아님, 1: 이동 완료
uint32_t elapsed_time = 0;         // 이동 소요시간
uint8_t toggle_flag[4] = {0};      // toggle_flag[1], [2], [3] 사용
uint32_t	expected_time = 0;

void Elevator_State_Init(void)
{
	open_flag = 0;
    move_done_flag = 0;
    elapsed_time = 0;
    expected_time = 0;

    toggle_flag[0] = 0;
    toggle_flag[1] = 0;
    toggle_flag[2] = 0;
    toggle_flag[3] = 0;
}

void Elevator_OutputFloorCall(uint8_t floor)
{
    switch (floor)
    {
    case 1:
        first_floor_fnd();
        start_floor_buzzer(383);
        break;

    case 2:
        second_floor_fnd();
        start_floor_buzzer(304);
        break;

    case 3:
        third_floor_fnd();
        start_floor_buzzer(255);
        break;

    default:
        break;
    }
}
