#ifndef INC_BT_SERIAL_H_
#define INC_BT_SERIAL_H_

#include "main.h"
#include "Header.h"
#include "elevator.h"
#include "stepper.h"
#include "fnd.h"
#include "door.h"
/*
 * Bluetooth control variables
 */

extern uint8_t bt_rx_flag;    // 0: 수신 없음, 1: 수신 완료
extern char bt_cmd[32];                // 수신 명령 문자열

extern uint8_t target_floor;           // 목표층
extern uint8_t bt_call_flag;           // 0: 호출 가능, 1: 호출 처리 중
extern uint8_t error_flag;             // 0: 정상, 1: 오류

void BT_Init(UART_HandleTypeDef *huart);
void BT_Process(void);
void BT_MoveDone_Process(void);
void BT_RxCallback(UART_HandleTypeDef *huart);
void BT_SetDisplay(uint8_t display_id, const char *text);
void BT_ClearDisplay(uint8_t display_id);
void BT_ClearAllDisplays(void);
void BT_HandleDifferentFloor(uint8_t floor, uint32_t time);
void BT_HandleSameFloor(uint32_t	time);
void BT_SetAllDisplays(const char *d0, const char *d1, const char *d2);
#endif
