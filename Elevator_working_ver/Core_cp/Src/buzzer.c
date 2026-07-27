/* buzzer.c */
#include "buzzer.h"

static BuzzerControl my_buzzer = {BUZZER_IDLE, 0, 0};

/**
  * @brief  부저 음을 새로 시작하는 함수 (원래 사용하던 하드웨어 설정 적용)
  */
void start_floor_buzzer(uint32_t psc_value)
{
    // 혹시 기존에 울리던 부저가 있다면 안전하게 먼저 정지
    stop_buzzer_immediately();

    // 상태 및 타이밍 변수 초기화
    my_buzzer.psc_value = psc_value;
    my_buzzer.start_tick = HAL_GetTick();
    my_buzzer.state = BUZZER_SOUND_ON; // 소리 켬 상태로 진입

    // 1. 주파수 변경 및 하드웨어 즉시 갱신
    TIM4->PSC = psc_value;
    TIM4->EGR = TIM_EGR_UG;

    // 2. 원래 설정되어 있던 듀티비(500) 세팅
    TIM4->CCR1 = 500;

    // 3. PWM 물리적 시작
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
}

/**
  * @brief  부저를 하드웨어적으로 즉시 강제 정지하는 함수
  */
void stop_buzzer_immediately(void)
{
    // 원래 사용하시던 "타이머 꺼버리기" 코드 적용
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
    TIM4->CCR1 = 0;
    my_buzzer.state = BUZZER_IDLE;
}

/**
  * @brief  main 함수 while문에서 계속 호출되는 논블로킹 핸들러
  */
void update_buzzer_nonblocking(void)
{
    uint32_t current_tick = HAL_GetTick();

    // 이제 enum에 정의된 모든 상태(IDLE, SOUND_ON)를 switch문이 완벽히 커버하므로 경고가 사라집니다.
    switch (my_buzzer.state)
    {
        case BUZZER_IDLE:
            // 부저가 꺼져있을 때는 아무것도 하지 않고 대기합니다.
            break;

        case BUZZER_SOUND_ON:
            // HAL_Delay(600) 대신 600ms가 경과했는지 체크
            if (current_tick - my_buzzer.start_tick >= 600)
            {
                // 600ms가 지나면 원래 원하셨던 대로 타이머를 아예 완전히 꺼버립니다!
                stop_buzzer_immediately();
            }
            break;
    }
}
