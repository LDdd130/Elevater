/* buzzer.h */
#ifndef __BUZZER_H__
#define __BUZZER_H__

#include "main.h"
#include "tim.h"

// 3개였던 단계를 딱 2개(대기 상태, 소리 출력 상태)로 축소
typedef enum {
    BUZZER_IDLE = 0,
    BUZZER_SOUND_ON
} BuzzerState;

typedef struct {
    BuzzerState state;    // 현재 상태
    uint32_t start_tick;  // 시작 시간
    uint32_t psc_value;   // 주파수 값
} BuzzerControl;

void start_floor_buzzer(uint32_t psc_value);
void stop_buzzer_immediately(void);
void update_buzzer_nonblocking(void);

#endif /* __BUZZER_H__ */
