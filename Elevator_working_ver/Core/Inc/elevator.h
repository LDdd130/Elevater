#ifndef ELEVATOR_STATE_H
#define ELEVATOR_STATE_H

#include "main.h"
#include "Header.h"

/*
 * Elevator common state variables
 * 다른 모듈들이 공통으로 사용하는 엘리베이터 상태값
 */

extern uint8_t current_floor;              // 현재층: 1, 2, 3
extern uint8_t open_flag;          // 문 열림 요청/상태
extern uint8_t move_done_flag;     // 이동 완료 플래그
extern uint32_t elapsed_time;      // 이동 소요시간
extern uint8_t toggle_flag[4];     // 1~3층 호출 플래그
extern uint32_t	expected_time;

void Elevator_State_Init(void);
void Elevator_OutputFloorCall(uint8_t floor);

#endif
