/* door.h */
#ifndef __DOOR_H__
#define __DOOR_H__

#include "main.h"
#include "tim.h"

// 문의 현재 상태 정의
typedef enum {
    DOOR_CLOSED = 0,    // 문 닫힘 (CCR1 = 1000 또는 500)
    DOOR_OPENING,       // 문 열리는 중
    DOOR_OPENED,        // 문 완전히 열림 (CCR1 = 2000 또는 2500)
    DOOR_CLOSING        // 문 닫히는 중
} DoorState;

typedef struct {
    DoorState state;      // 문의 현재 상태
    uint32_t start_tick;  // 시간 측정용 틱 수
} DoorControl;

// 외부에서 사용할 함수 선언
void open_door_request(void);
void update_door_nonblocking(void);

#endif /* __DOOR_H__ */
