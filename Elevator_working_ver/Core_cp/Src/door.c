/* door.c */
#include "door.h"

#define DOOR_OPEN_CCR        2500U
#define DOOR_CLOSED_CCR      500U
#define DOOR_OPEN_HOLD_MS    3000U
#define DOOR_CLOSE_TIME_MS   2000U

static DoorControl my_door = {DOOR_CLOSED, 0};
static uint8_t door_cycle_active = 0U;
static uint8_t door_close_command_sent = 0U;
static uint32_t door_close_due_tick = 0U;
static uint32_t door_closed_due_tick = 0U;

static void door_set_servo(uint32_t ccr)
{
    __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1, ccr);
}

void open_door_request(void)
{
    if (door_cycle_active == 0U)
    {
        my_door.state = DOOR_OPENED;
        my_door.start_tick = HAL_GetTick();
        door_close_due_tick = my_door.start_tick + DOOR_OPEN_HOLD_MS;
        door_closed_due_tick = door_close_due_tick + DOOR_CLOSE_TIME_MS;
        door_close_command_sent = 0U;
        door_cycle_active = 1U;
        door_set_servo(DOOR_OPEN_CCR);
    }
}

void close_door_request(void)
{
    if (my_door.state != DOOR_CLOSED)
    {
        my_door.state = DOOR_CLOSING;
        my_door.start_tick = HAL_GetTick();
        door_close_command_sent = 1U;
        door_closed_due_tick = my_door.start_tick + DOOR_CLOSE_TIME_MS;
        door_set_servo(DOOR_CLOSED_CCR);
    }
}

uint8_t is_door_closed(void)
{
    return ((my_door.state == DOOR_CLOSED) && (door_cycle_active == 0U)) ? 1U : 0U;
}

void update_door_nonblocking(void)
{
    uint32_t current_tick = HAL_GetTick();

    if(door_cycle_active != 0U)
    {
        if((door_close_command_sent == 0U) && ((int32_t)(current_tick - door_close_due_tick) >= 0))
        {
            my_door.state = DOOR_CLOSING;
            my_door.start_tick = current_tick;
            door_close_command_sent = 1U;
            door_set_servo(DOOR_CLOSED_CCR);
        }

        if((door_close_command_sent != 0U) && ((int32_t)(current_tick - door_closed_due_tick) >= 0))
        {
            my_door.state = DOOR_CLOSED;
            door_cycle_active = 0U;
        }

        return;
    }

    switch (my_door.state)
    {
        case DOOR_CLOSED:
            break;

        case DOOR_OPENING:
        case DOOR_OPENED:
            if ((current_tick - my_door.start_tick) >= DOOR_OPEN_HOLD_MS)
            {
                close_door_request();
            }
            break;

        case DOOR_CLOSING:
            if ((current_tick - my_door.start_tick) >= DOOR_CLOSE_TIME_MS)
            {
                my_door.state = DOOR_CLOSED;
            }
            break;
    }
}
