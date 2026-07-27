#include "BT_Serial.h"

#define BUFFER_SIZE	32

// Flag
uint8_t bt_rx_flag;
uint8_t bt_call_flag;
uint8_t error_flag;

uint8_t bt_rx_byte;
char bt_rx_buf[BUFFER_SIZE];
uint8_t bt_rx_idx;

char bt_cmd[BUFFER_SIZE] = {0};
uint8_t target_floor;

UART_HandleTypeDef *bt_huart;

/*
 * Internal function prototypes
 */

uint8_t BT_button(void);
void BT_HandleError(void);
void BT_HandleBusy(void);
void BT_HandleSameFloor(uint32_t time);
void BT_HandleDifferentFloor(uint32_t time);
void BT_ClearDisplay(uint8_t display_id);
void BT_ClearAllDisplays(void);
void BT_SetAllDisplays(const char *d0, const char *d1, const char *d2);


/*
 * Bluetooth initialize
 */

void BT_Init(UART_HandleTypeDef *huart)
{
    bt_huart = huart;

    bt_rx_flag = 0;
    bt_rx_idx = 0;

    target_floor = 0;
    bt_call_flag = 0;
    error_flag = 0;

    memset(bt_rx_buf, 0, sizeof(bt_rx_buf));
    memset(bt_cmd, 0, sizeof(bt_cmd));
    BT_ClearAllDisplays();
    HAL_UART_Receive_IT(bt_huart, &bt_rx_byte, 1);
}

/*
 * Send message to Bluetooth Serial Connect Display
 * Format: #display_id:message\n
 * Example: #0:CALL 3F\n
 */

void BT_SetDisplay(uint8_t display_id, const char *text)
{
    char tx_buf[64];

    if (bt_huart == NULL)
    {
        return;
    }

    snprintf(tx_buf, sizeof(tx_buf), "#%d:%s\n", display_id, text);
    HAL_UART_Transmit(bt_huart, (uint8_t *)tx_buf, strlen(tx_buf), 100);
}

/*
 * UART RX interrupt callback handler
 * main.c의 HAL_UART_RxCpltCallback()에서 호출
 */

void BT_RxCallback(UART_HandleTypeDef *huart)
{
    if (bt_huart == NULL)
    {
        return;
    }

    if (huart->Instance != bt_huart->Instance)
    {
        return;
    }

    if (bt_rx_byte == '\n' || bt_rx_byte == '\r')
    {
        if (bt_rx_idx > 0)
        {
            bt_rx_buf[bt_rx_idx] = '\0';

            strncpy(bt_cmd, bt_rx_buf, sizeof(bt_cmd) - 1);
            bt_cmd[sizeof(bt_cmd) - 1] = '\0';

            bt_rx_idx = 0;
            memset(bt_rx_buf, 0, sizeof(bt_rx_buf));

            bt_rx_flag = 1;
        }
    }
    else
    {
        if (bt_rx_idx < sizeof(bt_rx_buf) - 1)
        {
            bt_rx_buf[bt_rx_idx++] = bt_rx_byte;
        }
        else
        {
            bt_rx_idx = 0;
            memset(bt_rx_buf, 0, sizeof(bt_rx_buf));
            error_flag = 1;
            BT_SetDisplay(0, "ERR RX");
			BT_ClearAllDisplays();
        }
    }

    HAL_UART_Receive_IT(bt_huart, &bt_rx_byte, 1);
}

/*
 * Parse Bluetooth button command
 * B0 -> 1F
 * B1 -> 2F
 * B2 -> 3F
 */

uint8_t BT_button(void)
{
    target_floor = 0;

    if (strcmp(bt_cmd, "B0") == 0)
    {
        target_floor = 1;
        return 1;
    }
    else if (strcmp(bt_cmd, "B1") == 0)
    {
        target_floor = 2;
        return 1;
    }
    else if (strcmp(bt_cmd, "B2") == 0)
    {
        target_floor = 3;
        return 1;
    }

    return 0;
}

/*
 * Main Bluetooth command process
 */

void BT_Process(void)
{
    uint32_t expected_time = 0;

    if (bt_rx_flag == 0)
    {
        return;
    }

    bt_rx_flag = 0;
    error_flag = 0;

    if (BT_button() == 0)
    {
        BT_HandleError();
        return;
    }

    if (bt_call_flag == 1)
    {
        BT_HandleBusy();
        return;
    }

    if (current_floor == target_floor)
    {
        BT_HandleSameFloor(0);
        return;
    }

    /*
     * 테스트용 예상 시간
     * 층 차이 1층당 2초로 가정
     */
    if (target_floor > current_floor)
    {
        expected_time = (target_floor - current_floor) * 2;
    }
    else
    {
        expected_time = (current_floor - target_floor) * 2;
    }

    BT_HandleDifferentFloor(expected_time);
}
/*
 * Invalid command
 */

void BT_HandleError(void)
{
    error_flag = 1;
    BT_SetDisplay(0, "ERR");
}

/*
 * Busy state
 */

void BT_HandleBusy(void)
{
    BT_SetDisplay(0, "BUSY");
}

/*
 * Same floor case
 */

void BT_HandleSameFloor(uint32_t time)
{
    char msg_floor[32];
    char msg_time[32];

    open_flag = 1;
    elapsed_time = 0;

    BT_SetDisplay(2, "SAME FLOOR");

    snprintf(msg_floor, sizeof(msg_floor), "ARRIVED %dF", current_floor);
    snprintf(msg_time, sizeof(msg_time), "TIME %lus",time);
    BT_SetAllDisplays(msg_floor, msg_time, "SAME FLOOR");


    target_floor = 0;
}

/*
 * Different floor case
 */

void BT_HandleDifferentFloor(uint32_t time)
{
    char msg_floor[32];
    char msg_time[32];

    BT_SetDisplay(2, "DIFFERENT FLOOR");

    toggle_flag[target_floor] = 1;
    bt_call_flag = 1;

    // ★ 여기에 실제 모터 구동 함수를 연동해 주어야 메인 루프와 연동됩니다.
	extern void Stepper_RequestMoveToFloor(uint8_t targetFloor);
	Stepper_RequestMoveToFloor(target_floor);

	snprintf(msg_floor, sizeof(msg_floor), "CALL %dF", target_floor);
	snprintf(msg_time, sizeof(msg_time), "TIME %lus",time);
	BT_SetAllDisplays(msg_floor, msg_time, "DIFFERENT FLOOR");

    snprintf(msg_floor, sizeof(msg_floor), "CALL %dF", target_floor);
    //BT_SetDisplay(0, msg);

    snprintf(msg_time, sizeof(msg_time), "TIME %lus",time);
    //BT_SetDisplay(1, msg_time);
    BT_SetAllDisplays(msg_floor, msg_time, "DIFFERENT FLOOR");  // 한번에!

}

/*
 * Move done process
 * 이동 제어부에서 move_done_flag = 1로 세팅하면 결과 출력
 */

void BT_MoveDone_Process(void)
{
    char msg_floor[32];
    char msg_time[32];

    if (bt_call_flag == 1 && move_done_flag == 1)
    {
        open_flag = 1;

        snprintf(msg_floor, sizeof(msg_floor), "ARRIVED %dF", current_floor);
        BT_SetDisplay(2, msg_floor);

        snprintf(msg_time, sizeof(msg_time), "TIME %lus", elapsed_time);
        BT_SetDisplay(0, msg_time);

        bt_call_flag = 0;
        target_floor = 0;
        move_done_flag = 0;
    }
}
void BT_ClearDisplay(uint8_t display_id)
{
    BT_SetDisplay(display_id, "----");
}

void BT_ClearAllDisplays(void)
{
	HAL_Delay(10);
    BT_ClearDisplay(2);
    BT_ClearDisplay(0);
    BT_ClearDisplay(1);
}


void BT_SetAllDisplays(const char *d0, const char *d1, const char *d2)
{
    char tx_buf[192];
    snprintf(tx_buf, sizeof(tx_buf),
        "#0:%s\n#1:%s\n#2:%s\n", d0, d1, d2);
    HAL_UART_Transmit(bt_huart, (uint8_t *)tx_buf, strlen(tx_buf), 300);
}
