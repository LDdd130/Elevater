<div align="center">

# 🛗 Elevator On-Device

### 3층 엘리베이터 모형 · 초음파 층 판정 · Bluetooth 앱 연동

<p>
  <img src="https://img.shields.io/badge/MCU-STM32F411RE-03234B?style=flat-square&logo=stmicroelectronics&logoColor=white" alt="STM32F411RE">
  <img src="https://img.shields.io/badge/Language-C-A8B9CC?style=flat-square&logo=c&logoColor=white" alt="C">
  <img src="https://img.shields.io/badge/IDE-STM32CubeIDE-00A9E0?style=flat-square" alt="STM32CubeIDE">
  <img src="https://img.shields.io/badge/HAL-STM32F4xx-0058A9?style=flat-square" alt="STM32 HAL">
</p>

<p>
  <img src="./Elevator_working_ver/asset/image1.png" width="250" alt="Elevator Model">
</p>

물리 버튼과 스마트폰 앱으로 호출하면, 스텝모터가 카빈을 움직이고 초음파 센서 2채널이 실제 도착을 판정하는 3층 엘리베이터 모형 제어 시스템입니다.

### :movie_camera: 시연 영상


https://github.com/user-attachments/assets/dfed8325-cb74-45d0-a27e-58f5c706b642
</div>

---

## 1. Project Overview

"몇 스텝 돌렸으니 도착했을 것"이라는 개루프 추정 대신, 실제 위치를 센서로 확인하고 정지하는 폐루프 판정으로 설계했습니다. 초음파 센서 2채널이 층 판정 기준입니다.

스텝모터는 층 이동 방향으로 계속 회전하고, 초음파 센서가 목표 층 조건을 연속으로 만족하면 그때 정지합니다. 층별로 요구되는 연속 만족 횟수(hit count)를 다르게 두어 센서 노이즈로 인한 조기 정지를 막았습니다. 문 개폐는 SG90 서보를 상태 머신으로 제어하며, 문이 닫힌 것이 확인되기 전에는 이동을 시작하지 않는 안전 인터록을 두었습니다.

입력은 물리 버튼 3개와 Bluetooth 앱 두 경로를 모두 지원하며, 앱에는 `#id:message` 형식으로 3개 디스플레이를 갱신합니다. 부저는 `HAL_Delay()` 대신 `HAL_GetTick()` 기반 논블로킹 상태 머신으로 만들어, 소리가 나는 동안에도 스텝모터·문·통신이 계속 동작합니다.

| 항목 | 내용 |
|---|---|
| 프로젝트 형태 | 팀 프로젝트 **[인원 수 기재 필요]** |
| 담당 범위 | **[작성 필요 — 예: 스텝모터 이동 제어 / 초음파 층 판정 / Bluetooth 프로토콜]** |
| MCU | STM32F411RE (HSE Bypass + PLL M=4 / N=100 / P=2 → 100 MHz) |
| RTOS | 미사용 — `HAL_GetTick()` 기반 논블로킹 협조적 스케줄링 |
| Language | C |
| Development | STM32CubeIDE, STM32CubeMX, STM32 HAL |
| 주요 인터페이스 | GPIO, TIM Input Capture, TIM PWM, UART, SysTick |
| 액추에이터 | 28BYJ-48 스텝모터(카빈 승강), SG90 서보(문 개폐) |
| 센서 | 초음파 거리 센서 × 2 (상·하 방향) |

> **소스 트리 안내.** 저장소에는 세 개의 `main()` 구현이 있고 기능 범위가 다릅니다. 각 절 설명이 어느 트리를 기준으로 하는지 §12와 §15에 명시했습니다.
>
> | 트리 / 파일 | 범위 | 비고 |
> |---|---|---|
> | `Core/Src/main.c` | 버튼 → FND · 부저 · 문 (PC9 문 열림 포함) | **현재 `Debug/` 빌드 산출물이 이 구성** |
> | `Core/Src/main_for_test.c` | 전체 기능 통합 시연 (초음파·스텝모터·BT) | `Core/Src/stepper.c`의 `HAL_IncTick()` 훅 사용 |
> | `Core_cp/Src/main.c` | 문 인터록 + 보류 요청(`pendingMoveFloor`)까지 반영 | CubeIDE 빌드 대상에는 포함되어 있지 않음 |

---

## 2. Key Features

| 기능 | 구현 내용 |
|---|---|
| **센서 기반 층 판정** | 초음파 2채널 거리를 mm 단위로 측정해 목표 층 도달 여부를 실시간 판정 |
| **Hit Count 필터** | 목표 조건을 층별 지정 횟수(1F 22회 / 3F 13회 / 2F 1회) 연속 만족해야 도착 인정 |
| **Non-blocking Stepper** | `Stepper_Process()`가 tick 비교로 2 ms마다 한 스텝씩 구동 (구동 주체는 트리별로 다름 — §5.2) |
| **Door State Machine** | `OPENED → CLOSING → CLOSED` 상태 전이를 `HAL_GetTick()` 기반으로 처리 |
| **Door Interlock** | 문이 닫히지 않았으면 이동 요청을 보류(`pendingMoveFloor`)하고 문부터 닫음 (`Core_cp`) |
| **Move Timeout** | 30초 내 도착하지 못하면 자동 정지 후 `STEPPER_MOVE_TIMEOUT` 반환 |
| **Dual Input** | 물리 버튼 3개(Falling Edge 검출) + Bluetooth 명령(`B0`/`B1`/`B2`) |
| **BT Display Protocol** | `#0` / `#1` / `#2` 3개 디스플레이를 `#id:message\n` 형식으로 갱신 |
| **Floor Tone Buzzer** | 층마다 다른 음(약 260 / 328 / 391 Hz)을 600 ms 논블로킹 출력 |
| **7-Segment Display** | GPIO 직결 세그먼트 제어로 현재 호출 층 표시 |
| **UART Error Recovery** | ORE/NE/FE/PE 발생 시 SR→DR 순서로 읽어 클리어하고 수신 인터럽트를 재구동 |

---

## 3. System Architecture & Control Flow

<p align="center">
  <img src="./Elevator_working_ver/asset/architecture.svg" width="100%"
       alt="Elevator 시스템 아키텍처 — 물리 버튼과 Bluetooth 호출을 문 인터록으로 검사하고 초음파 센서로 도착을 판정해 스텝모터, 문, 부저, 7-Segment와 앱 표시를 제어한다.">
</p>

### Control Flow

1. 물리 버튼(Falling Edge) 또는 Bluetooth 명령으로 목표 층 호출이 들어옵니다.
2. `Stepper_RequestMoveToFloor()`가 문 상태를 확인합니다. 문이 열려 있으면 `close_door_request()`를 호출하고 요청을 `pendingMoveFloor`에 보류합니다.
3. 문이 닫히면 `Stepper_BeginMoveToFloor()`가 초음파로 현재 층을 읽고, 목표 층과 다르면 회전 방향을 정한 뒤(`STEPPER_FLOOR_UP/DOWN_DEGREES`의 부호로 결정 — §5.1) 회전을 시작합니다.
4. `Stepper_Process()`가 tick을 비교해 2 ms마다 한 스텝씩 구동합니다. 이 함수를 부르는 주체는 트리마다 다릅니다 (§5.2).
5. 메인 루프의 `Stepper_Update()` → `Stepper_TickMove()`가 120 ms 주기(또는 64스텝 진행 콜백 시점)로 초음파를 재측정해 도착 여부를 판정합니다.
6. 목표 조건을 층별 hit count만큼 연속 만족하면 정지하고, `RUNNING → ARRIVED` 전이 시점에 `open_door_request()`로 문을 엽니다.
7. `BT_MoveDone_Process()`가 앱 디스플레이에 도착 층과 소요 시간을 전송합니다.

---

## 4. Ultrasonic Floor Detection

### 4.1 Input Capture 기반 거리 측정

TIM3를 1 MHz(`PSC = 100-1`, 100 MHz 기준)로 설정해 1 카운트 = 1 µs가 되게 하고, 두 채널을 Input Capture로 사용합니다. Rising Edge에서 시작 시각을 캡처하고 폴라리티를 Falling으로 바꿔 종료 시각을 캡처하는 방식입니다.

```c
if (IC_Value2_CH1 > IC_Value1_CH1)
    echoTime_CH1 = IC_Value2_CH1 - IC_Value1_CH1;
else if (IC_Value1_CH1 > IC_Value2_CH1)
    echoTime_CH1 = (0xFFFF - IC_Value1_CH1) + IC_Value2_CH1;   // 16-bit wrap 보정

distance_CH1   = echoTime_CH1 / 58;                     // cm
distanceMm_CH1 = ((uint32_t)echoTime_CH1 * 10U) / 58U;  // mm
```

- 카운터가 `0xFFFF`를 넘어 되감긴 경우를 명시적으로 보정하여, 측정 시점에 따라 거리가 튀는 문제를 제거했습니다.
- 측정이 끝나면 해당 채널의 캡처 인터럽트를 비활성화(`__HAL_TIM_DISABLE_IT`)해 불필요한 ISR 진입을 막습니다.
- `Ultrasonic_Trigger()` 진입 시 캡처 플래그와 폴라리티를 **강제로 Rising 대기 상태로 초기화**합니다. 이전 측정에서 Echo가 한쪽만 들어와 상태 머신이 멈춘 경우를 복구하기 위한 처리입니다.

### 4.2 층 판정 규칙

두 센서를 상·하 방향으로 배치해, 층마다 서로 다른 판정 기준을 적용합니다.

| Target Floor | 판정 조건 | 근거 |
|---|---|---|
| **1F** | `CH1` 거리 ≈ `ULTRASONIC_FLOOR1_CH1_TARGET_MM` (23 mm) | 최하층은 한쪽 센서의 절대 거리로 판정 |
| **2F** | `CH1` 거리 ≈ `CH2` 거리 | 중간층은 위아래 거리가 같아지는 지점 |
| **3F** | `CH2` 거리 ≈ `ULTRASONIC_FLOOR3_CH2_TARGET_MM` (26 mm) | 최상층은 반대쪽 센서의 절대 거리로 판정 |

| Parameter | Value | Description |
|---|---:|---|
| `ULTRASONIC_FLOOR1_CH1_TARGET_MM` | 23 mm | 1층 기준 거리 (센서 보정값) |
| `ULTRASONIC_FLOOR3_CH2_TARGET_MM` | 26 mm | 3층 기준 거리 (센서 보정값) |
| `ULTRASONIC_FLOOR_TOLERANCE_MM` | ±2 mm | 모든 판정에 공통 적용되는 허용 오차 |
| `ULTRASONIC_SENSOR_INTERVAL_MS` | 120 ms | 이동 중 거리 재측정 주기 |
| `ULTRASONIC_FLOOR1_HIT_COUNT` | 22회 | 1층 도착 인정에 필요한 연속 만족 횟수 |
| `ULTRASONIC_FLOOR3_HIT_COUNT` | 13회 | 3층 도착 인정에 필요한 연속 만족 횟수 |
| 2F hit count | 1회 | `Ultrasonic_GetTargetHitCount()`의 기본 반환값 |

`Ultrasonic_GetCurrentFloor()`는 1F → 3F → 2F 순으로 판정하고, 어느 조건에도 맞지 않으면 두 센서 거리의 대소만으로(`CH1 < CH2 → 1F`, `CH2 < CH1 → 3F`, 같으면 2F) 위치를 추정하여 항상 유효한 층을 반환합니다.

### 4.3 측정 중 블로킹 최소화

Trigger 후 Echo가 돌아올 때까지 80 ms를 기다려야 하지만, 그 시간에도 문과 스텝모터는 계속 움직여야 합니다.

```c
void Ultrasonic_ReadBoth(uint16_t *distanceCH1, uint16_t *distanceCH2)
{
    Ultrasonic_Trigger();
    startTick = HAL_GetTick();
    while ((HAL_GetTick() - startTick) < 80U)
    {
        update_door_nonblocking();   // 대기 중에도 문 상태 갱신
        Stepper_Process();           // 대기 중에도 스텝 진행
    }
    ...
}
```

단순 `HAL_Delay(80)` 대신 대기 루프 안에서 다른 모듈의 갱신 함수를 호출하여, 센서 측정이 카빈 이동을 멈추지 않도록 했습니다.

---

## 5. Stepper Motor Control

### 5.1 Full-Step 4상 구동

28BYJ-48 스텝모터를 ULN2003 드라이버로 구동하며, 4상 Full-Step 시퀀스를 GPIO 4핀으로 직접 출력합니다.

```c
static const uint8_t FULL_STEP_SEQ[4][4] =
{
    {1, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 1},
    {1, 0, 0, 1}
};
```

방향은 시퀀스 인덱스의 증감 방향으로 결정합니다. `STEPPER_DIR_CW`는 `(idx + 1) % 4`, `STEPPER_DIR_CCW`는 `(idx + 3) % 4`로 계산하여 음수 모듈로 연산을 피했습니다.

| Parameter | Value | Description |
|---|---:|---|
| `STEPPER_STEPS_PER_REVOLUTION` | 2048 | 1회전 스텝 수 (기어비 포함) |
| `STEPPER_STEP_DELAY_MS` | 2 ms | 스텝 간 간격 |
| `STEPPER_SETTLE_DELAY_MS` | 10 ms | 회전 시작 전 안정화 시간 |
| `STEPPER_FLOOR_UP_DEGREES` | −3600° | 상승 방향 (10회전 분량) |
| `STEPPER_FLOOR_DOWN_DEGREES` | +3600° | 하강 방향 |
| `STEPPER_MOVE_TIMEOUT_MS` | 30,000 ms | 이동 안전 타임아웃 |

이동 각도를 층간 정확한 거리로 계산하지 않고 충분히 큰 값(3600°)을 지정한 뒤 센서가 정지 시점을 결정하는 방식입니다. 회전이 한 사이클 끝나도 도착 판정 전이면 `Stepper_TickMove()`가 같은 방향으로 회전을 재시작합니다.

```c
/* 부호 → 방향 매핑은 이 한 줄이 유일한 근거다 */
uint8_t direction = (degrees >= 0) ? STEPPER_DIR_CCW : STEPPER_DIR_CW;
```

> ⚠ 따라서 실제 동작은 **상승(`−3600°`) = `STEPPER_DIR_CW`, 하강(`+3600°`) = `STEPPER_DIR_CCW`** 입니다. `stepper.h`의 주석(`음수 = CCW = 상승, 양수 = CW = 하강`)은 구현과 반대로 적혀 있으므로 신뢰하지 마십시오. 기구상 회전 방향이 반대로 나오면 이 한 줄의 삼항 연산자를 뒤집는 것이 정확한 수정입니다.

### 5.2 스텝 구동 주체

`Stepper_Process()`는 `lastStepTick`과 현재 tick을 비교해 2 ms가 지났을 때만 한 스텝을 진행하는 논블로킹 함수입니다. 이 함수를 무엇이 호출하는지가 두 트리의 가장 큰 차이입니다.

**`Core/Src/stepper.c` — SysTick 훅**

`HAL_IncTick()`을 재정의하여, `stm32f4xx_it.c`나 `main.c`를 수정하지 않고 1 ms 인터럽트에 모터 구동을 연결했습니다.

```c
extern __IO uint32_t uwTick;
extern HAL_TickFreqTypeDef uwTickFreq;

void HAL_IncTick(void)
{
    uwTick += uwTickFreq;   // 원래 HAL 동작 유지
    Stepper_Process();      // 1 ms마다 스텝 진행 여부 판단
}
```

ISR과 메인 컨텍스트가 함께 접근하는 상태 변수(`currentStepIndex`, `stepperBusy`, `movedSteps`, `lastStepTick`, `targetSteps`, `activeDirection`, 진행 콜백 관련 변수)는 모두 `volatile`로 선언했습니다.

**`Core_cp/Src/stepper.c` — 메인 루프 호출**

이 트리에는 `HAL_IncTick()` 재정의가 없고 상태 변수도 `volatile`이 아닙니다. 대신 두 경로에서 폴링합니다.

- 메인 루프의 `Stepper_Update()` 선두
- `Ultrasonic_ReadBoth()`의 80 ms 대기 루프 안 (§4.3)

문 인터록과 `pendingMoveFloor` 보류 큐는 이 트리에만 있습니다. 두 트리를 통합할 때는 **`HAL_IncTick()` 훅 + `volatile` 선언**과 **문 인터록**을 함께 가져와야 합니다.

### 5.3 Move State Machine

<p align="center">
  <img src="./Elevator_working_ver/asset/move_fsm.svg" width="100%"
       alt="Elevator 이동 상태 머신 — 문이 열려 있으면 요청을 보류하고 닫힌 뒤 이동하며 초음파 hit count 충족 시 ARRIVED, 30초 초과 시 TIMEOUT으로 전이한다.">
</p>

- 목표 층이 1~3 범위를 벗어나면 즉시 `STEPPER_MOVE_INVALID`를 반환합니다.
- 이미 목표 층에 있으면 회전 없이 `STEPPER_MOVE_ARRIVED`로 처리합니다.
- 이동 중(`STEPPER_MOVE_RUNNING`) 재호출은 `Stepper_RequestMoveToFloor()` 진입부에서 즉시 반려합니다.
- `RUNNING → ARRIVED` **전이 시점에만** 문을 여는 Edge 판정(`previousMoveState`)을 적용해, 도착 후 매 루프마다 문 열림 요청이 반복되는 것을 막았습니다.

---

## 6. Door Control

SG90 서보를 TIM10 CH1 PWM(`PSC = 100-1`, `Period = 20000-1` → 1 µs 분해능 / 50 Hz)으로 제어합니다.

| State | 동작 | CCR / 시간 |
|---|---|---|
| `DOOR_CLOSED` | 대기 | `CCR = 500` (0.5 ms 펄스) |
| `DOOR_OPENED` | 열린 상태 유지 | `CCR = 2500` (2.5 ms 펄스), `DOOR_OPEN_HOLD_MS` 3000 ms |
| `DOOR_CLOSING` | 닫히는 중 | `CCR = 500`, `DOOR_CLOSE_TIME_MS` 2000 ms |

> `DoorState` enum에는 `DOOR_OPENING`도 정의되어 있지만 현재 구현은 이 상태로 진입하지 않습니다. `update_door_nonblocking()`에서 `DOOR_OPENED`와 같은 분기로 묶어 두었습니다.

```c
uint8_t is_door_closed(void)
{
    return ((my_door.state == DOOR_CLOSED) && (door_cycle_active == 0U)) ? 1U : 0U;
}
```

- 한 번의 개폐 사이클이 진행 중(`door_cycle_active`)이면 중복 열림 요청을 무시하여, 반복 호출로 문이 계속 열린 채 유지되는 상황을 막았습니다.
- 만료 시각 비교를 `(int32_t)(current_tick - due_tick) >= 0`으로 작성해 `HAL_GetTick()` 오버플로 시에도 판정이 뒤집히지 않도록 했습니다.
- `is_door_closed()`를 스텝모터 이동 조건으로 사용하여, **문이 열린 채 카빈이 움직이는 상황을 구조적으로 차단**했습니다.

---

## 7. Bluetooth Command Protocol

### 7.1 수신 — 층 호출

USART1(9600 bps, 8N1) 1-byte 인터럽트 수신으로 개행 문자까지 문자열을 조립합니다.

| Command | Target Floor |
|---|---|
| `B0` | 1F |
| `B1` | 2F |
| `B2` | 3F |

```c
void BT_RxCallback(UART_HandleTypeDef *huart)
{
    if (bt_rx_byte == '\n' || bt_rx_byte == '\r') { /* 명령 확정 → bt_rx_flag = 1 */ }
    else if (bt_rx_idx < sizeof(bt_rx_buf) - 1)    { bt_rx_buf[bt_rx_idx++] = bt_rx_byte; }
    else                                            { /* 버퍼 오버플로 → 리셋 + ERR RX 표시 */ }

    HAL_UART_Receive_IT(bt_huart, &bt_rx_byte, 1);  // 다음 바이트 수신 재등록
}
```

ISR에서는 문자 수집과 플래그 세팅만 수행하고, 실제 명령 해석과 모터 제어는 메인 루프의 `BT_Process()`가 담당합니다. 수신 버퍼는 32 byte(`BUFFER_SIZE`)입니다.

### 7.2 송신 — 앱 디스플레이

```text
#0:<메시지>\n#1:<메시지>\n#2:<메시지>\n
```

`BT_SetAllDisplays(d0, d1, d2)`가 세 줄을 한 번에 보내고, `BT_SetDisplay(id, text)`는 한 줄만 보냅니다. 실제 코드가 보내는 문자열은 다음과 같습니다.

| 상황 | 함수 | `#0` | `#1` | `#2` |
|---|---|---|---|---|
| 부팅 (`main_for_test.c`) | `BT_SetAllDisplays` | `1F` | `READY` | `IDLE` |
| `BT_Init()` 직후 | `BT_ClearAllDisplays` | `----` | `----` | `----` |
| BT 이동 시작 | `BT_HandleDifferentFloor` | `CALL 3F` | `TIME 4s` | `DIFFERENT FLOOR` |
| 버튼 이동 시작 (`main_for_test.c`) | `BT_SetAllDisplays` | `CALL 3F` | `MOVING` | `TO 3F` |
| BT 같은 층 호출 | `BT_HandleSameFloor` | `ARRIVED 2F` | `TIME 0s` | `SAME FLOOR` |
| 버튼 1F 같은 층 (`main_for_test.c`) | `BT_SetAllDisplays` | `ARRIVED 1F` | `TIME 0s` | `DOOR OPEN` |
| 도착 보고 | `BT_MoveDone_Process` | `TIME 12s` | (미갱신) | `ARRIVED 1F` |
| 이동 중 재호출 | `BT_HandleBusy` | `BUSY` | (미갱신) | (미갱신) |
| 잘못된 명령 | `BT_HandleError` | `ERR` | (미갱신) | (미갱신) |
| RX 버퍼 오버플로 | `BT_RxCallback` | `ERR RX` → `----` | `----` | `----` |

예상 소요 시간은 `|목표층 − 현재층| × 2`초로 계산해 `TIME <n>s` 형식으로 보냅니다.

> ⚠ **알려진 불일치.** `BT_MoveDone_Process()`만 층 문자열을 `#2`로, 시간을 `#0`으로 보냅니다. 다른 모든 경로는 층을 `#0`, 시간을 `#1`에 넣습니다. 앱 화면에서 도착 시점에만 배치가 뒤바뀌어 보인다면 원인은 이것입니다. `BT_HandleBusy()` / `BT_HandleError()`도 `#0`만 갱신하므로 `#1` / `#2`에는 직전 상태가 남습니다.

### 7.3 UART 오류 복구

```c
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        __IO uint32_t tmpreg = 0x00U;
        tmpreg = huart->Instance->SR;   // SR → DR 순서로 읽어 ORE/NE/FE/PE 클리어
        tmpreg = huart->Instance->DR;
        UNUSED(tmpreg);

        extern uint8_t bt_rx_byte;
        HAL_UART_Receive_IT(huart, &bt_rx_byte, 1);   // 멈춘 수신 인터럽트 재구동
    }
}
```

Overrun 한 번으로 이후 모든 Bluetooth 명령이 수신되지 않는 문제를 이 콜백으로 해결했습니다. (`Core_cp/Src/main.c`, `Core/Src/main_for_test.c` 양쪽에 동일하게 구현되어 있습니다.)

---

## 8. Floor Indication (FND & Buzzer)

### 7-Segment

7세그먼트를 GPIO에 직결하여 세그먼트 단위로 제어합니다. 숫자 변경 시 `clear_fnd()`로 전 세그먼트를 소등한 뒤 필요한 세그먼트만 점등합니다.

| Segment | A | B | C | D | E | F | G |
|---|---|---|---|---|---|---|---|
| Pin | `PA12` | `PA11` | `PB12` | `PA8` | `PB10` | `PB4` | `PB5` |

`first_floor_fnd()` ~ `third_floor_fnd()` 외에 `fourth_floor_fnd()`가 있으며, `Core/Src/main.c`에서 PC9(문 열림 단독 요청) 버튼의 표시로 사용합니다.

### Floor Tone Buzzer

TIM4 CH1(PB6) PWM의 Prescaler를 바꿔 층마다 다른 음을 냅니다. `Period = 1000-1`, `CCR = 500`(듀티 50 %) 고정입니다.

| Floor | `PSC` | 출력 주파수 (약) | 음 |
|---|---:|---:|---|
| 1F | 383 | 260 Hz | C4 |
| 2F | 304 | 328 Hz | E4 |
| 3F | 255 | 391 Hz | G4 |

세 층의 음을 C–E–G 화음 구성음으로 배치해 층이 올라갈수록 음이 높아지게 했습니다.

```c
void update_buzzer_nonblocking(void)
{
    switch (my_buzzer.state)
    {
        case BUZZER_IDLE:
            break;
        case BUZZER_SOUND_ON:
            if (HAL_GetTick() - my_buzzer.start_tick >= 600)   // HAL_Delay(600) 대체
                stop_buzzer_immediately();
            break;
    }
}
```

`HAL_Delay(600)`을 상태 머신으로 바꿔, 부저가 울리는 600 ms 동안에도 스텝모터·문·통신이 정상 동작합니다.

---

## 9. Hardware and Peripheral Mapping

| Peripheral | Pin | Usage |
|---|---|---|
| STM32F411RE | — | HSE Bypass, PLL(M=4, N=100, P=2) → 100 MHz |
| Ultrasonic TRIG | `PA5` | 두 센서 공통 트리거 (10 µs 펄스, `delay_us()` 사용) |
| Ultrasonic ECHO CH1 | `PA6` (`TIM3_CH1`) | Input Capture, 1 µs 분해능 |
| Ultrasonic ECHO CH2 | `PA7` (`TIM3_CH2`) | Input Capture, 1 µs 분해능 |
| Stepper IN1~IN4 | `PB1`, `PB13`, `PB14`, `PB15` | 28BYJ-48 4상 Full-Step 출력 |
| Door Servo | `PB8` (`TIM10_CH1`) | 50 Hz PWM (`PSC = 100-1`, `Period = 20000-1`) |
| Buzzer | `PB6` (`TIM4_CH1`) | 가변 주파수 PWM (`Period = 1000-1`, `CCR = 500`) |
| Button 1F | `PC5` | Falling Edge 검출 |
| Button 2F | `PC6` | Falling Edge 검출 |
| Button 3F | `PC8` | Falling Edge 검출 |
| Button (문 열림) | `PC9` (`BUTTONC9_Pin`) | `Core/Src/main.c`에서만 사용 — FND `4` 표시 + `open_door_request()` |
| 7-Segment A~G | `PA8`, `PA11`, `PA12`, `PB4`, `PB5`, `PB10`, `PB12` | 현재 층 표시 |
| USART1 | `PA9` / `PA10` | Bluetooth 모듈, 9600 bps 8N1, RX 인터럽트 |
| USART2 | — | 디버그 로그 출력용 (9600 bps) |
| TIM3 | — | `PSC = 100-1` → 1 MHz, 초음파 Input Capture |
| TIM10 | — | 문 서보 PWM |
| TIM11 | — | `PSC = 100-1` → `delay_us()` 용 자유 실행 1 MHz 카운터 |
| SysTick | — | `Core/Src/stepper.c`에서 `HAL_IncTick()` 재정의 → 1 ms 스텝모터 구동 |

---

## 10. Troubleshooting

| Problem | Cause | Applied Solution |
|---|---|---|
| 스텝 수 기반 이동이 층마다 어긋남 | 개루프 제어라 미끄러짐·부하 편차가 누적 | 초음파 거리 판정으로 정지 시점을 결정하는 폐루프 구조로 변경 |
| 센서 노이즈로 목표 층을 지나쳐 정지 | 1회 조건 만족만으로 도착 판정 | 층별 hit count(1F 22회 / 3F 13회) 연속 만족 시에만 도착 인정 |
| 정확히 일치하는 거리값이 안 잡혀 도착 판정 누락 | `==` 비교로 판정 | 모든 층 판정에 `±2 mm` tolerance 적용 |
| 측정 시점에 따라 거리가 튐 | TIM3 16-bit 카운터가 되감김 | `(0xFFFF - start) + end` 로 wrap 보정 |
| Echo 한쪽만 수신되면 이후 측정이 전부 멈춤 | 캡처 상태 머신이 Falling 대기에 고정 | `Ultrasonic_Trigger()`에서 폴라리티와 플래그를 Rising 대기로 강제 초기화 |
| 초음파 측정 80 ms 동안 카빈이 멈춤 | `HAL_Delay(80)` 블로킹 대기 | 대기 루프 안에서 `update_door_nonblocking()`·`Stepper_Process()` 계속 호출 |
| 부저가 울리는 동안 모터·통신 정지 | `HAL_Delay(600)` 블로킹 | `HAL_GetTick()` 기반 `BUZZER_SOUND_ON` 상태 머신으로 전환 |
| 문이 열린 상태에서 카빈이 이동 | 이동 요청과 문 상태가 독립적으로 처리 | `is_door_closed()` 인터록 + `pendingMoveFloor` 보류 큐 도입 (`Core_cp`) |
| 도착 후 문 열림 요청이 매 루프 반복 | 상태값만 보고 문을 염 | `previousMoveState`로 `RUNNING → ARRIVED` 전이 Edge에서만 실행 |
| 센서 사각지대·끼임 시 무한 회전 | 정지 조건이 센서 판정뿐 | `STEPPER_MOVE_TIMEOUT_MS`(30 s) 초과 시 강제 정지 |
| Overrun 한 번 후 Bluetooth 수신 불가 | ORE 플래그가 클리어되지 않아 RX 인터럽트 정지 | `HAL_UART_ErrorCallback()`에서 SR→DR 순서로 읽고 `Receive_IT` 재등록 |
| 긴 명령 입력 시 버퍼 오버런 | 수신 인덱스 상한 검사 없음 | 32 byte 버퍼 상한 검사 후 초과 시 리셋 및 `ERR RX` 표시 |
| 버튼 1회 입력이 여러 번 처리 | 레벨 기준으로 판정 | 이전 핀 상태를 저장해 Falling Edge에서만 1회 처리 |
| 이동 중 재호출로 상태가 꼬임 | 중복 이동 요청 허용 | `STEPPER_MOVE_RUNNING` 상태에서 요청을 즉시 반려하고 앱에는 `BUSY` 표시 |
| `it.c` 수정 없이 1 ms 모터 구동 필요 | CubeMX 재생성 시 사용자 코드 유실 위험 | `Core/Src/stepper.c`에서 `HAL_IncTick()`을 재정의해 생성 코드 밖에서 훅 |

---

## 11. Repository Structure

```text
Elevater/
└── Elevator_working_ver/               # STM32CubeIDE 프로젝트
    ├── 111_ELEVATOR.ioc                # STM32CubeMX 프로젝트 설정
    ├── 111_ELEVATOR.launch             # 디버그 실행 구성
    ├── STM32F411RETX_FLASH.ld / _RAM.ld
    │
    ├── Core/                           # CubeIDE 빌드 대상 트리
    │   ├── Inc/
    │   │   ├── elevator.h              # 공통 상태 변수 (current_floor, flag 등)
    │   │   ├── stepper.h               # 스텝모터 상수 및 Move State 정의
    │   │   ├── ultrasonic.h            # 층별 거리 임계값 및 hit count
    │   │   ├── door.h                  # 문 상태 enum 및 제어 API
    │   │   ├── buzzer.h                # 부저 상태 머신 정의
    │   │   ├── fnd.h / BT_Serial.h / delay.h / Header.h
    │   │   └── main.h                  # CubeMX 핀 매핑 define
    │   ├── Src/
    │   │   ├── main.c                  # 버튼 → FND/부저/문 축소 버전 (현재 Debug/ 빌드 구성)
    │   │   ├── main_for_test.c         # 전체 기능 통합 시연 버전
    │   │   ├── stepper.c               # HAL_IncTick 재정의 + volatile, Move State Machine
    │   │   ├── ultrasonic.c            # TIM3 Input Capture, 층 판정
    │   │   ├── door.c                  # 서보 개폐 상태 머신
    │   │   ├── buzzer.c                # 논블로킹 층별 음 출력
    │   │   ├── BT_Serial.c             # Bluetooth 명령 파싱 및 디스플레이 전송
    │   │   ├── elevator.c              # 공통 상태 초기화 및 층 호출 출력
    │   │   ├── fnd.c                   # 7세그먼트 표시
    │   │   ├── delay.c                 # TIM11 기반 delay_us()
    │   │   └── tim.c / gpio.c / usart.c / stm32f4xx_it.c ...
    │   └── Startup/
    │
    ├── Core_cp/                        # 문 인터록·pendingMoveFloor 반영 버전
    │                                   # (Core와 동일 구조, 빌드 대상에는 미포함)
    ├── Drivers/                        # STM32F4 HAL + CMSIS (CubeMX 생성)
    ├── Debug/                          # CubeIDE 빌드 산출물 (.elf/.map/.list/.o)
    ├── asset/image.png, image1.png     # 장치 사진
    └── code.tar.gz                     # 소스 아카이브
```

> `Debug/objects.list` 기준으로 마지막 빌드에 포함된 소스는 `Core/Src/{main, door, buzzer, fnd, gpio, tim, usart, ...}.c`이며 `stepper.c` / `ultrasonic.c` / `BT_Serial.c`는 포함되어 있지 않습니다. 전체 기능을 빌드하려면 `main.c`를 `main_for_test.c`로 교체(또는 한쪽을 빌드에서 제외)하고 나머지 소스를 빌드에 포함시켜야 합니다. `main()`이 세 파일에 각각 정의되어 있으므로 **동시에 빌드할 수는 없습니다.**

---

## 12. Key Source Files

기능 설명의 기준 트리를 각 항목에 표기했습니다.

| File | Description |
|---|---|
| [`Core/Src/stepper.c`](./Elevator_working_ver/Core/Src/stepper.c) | `HAL_IncTick()` 재정의, `volatile` 공유 상태, Move State Machine |
| [`Core_cp/Src/stepper.c`](./Elevator_working_ver/Core_cp/Src/stepper.c) | 위 + 문 인터록 및 `pendingMoveFloor` 보류 요청 (SysTick 훅 없음) |
| [`Core_cp/Src/ultrasonic.c`](./Elevator_working_ver/Core_cp/Src/ultrasonic.c) | TIM3 Input Capture, wrap 보정, 층별 도착 판정 |
| [`Core_cp/Src/door.c`](./Elevator_working_ver/Core_cp/Src/door.c) | 서보 개폐 상태 머신, `is_door_closed()` 인터록 |
| [`Core_cp/Src/BT_Serial.c`](./Elevator_working_ver/Core_cp/Src/BT_Serial.c) | Bluetooth 수신 ISR, 명령 파싱, 3-디스플레이 프로토콜 |
| [`Core_cp/Src/buzzer.c`](./Elevator_working_ver/Core_cp/Src/buzzer.c) | 층별 음 PWM, 논블로킹 600 ms 출력 |
| [`Core_cp/Src/main.c`](./Elevator_working_ver/Core_cp/Src/main.c) | 버튼 Edge 검출, 모듈 통합 루프, `HAL_UART_ErrorCallback()` |
| [`Core/Src/main_for_test.c`](./Elevator_working_ver/Core/Src/main_for_test.c) | 시연용 통합 시나리오 (BT 디스플레이 초기화 포함) |
| [`Core/Src/main.c`](./Elevator_working_ver/Core/Src/main.c) | 축소 버전 — 버튼·FND·부저·문(PC9 포함), 현재 빌드 구성 |
| [`Core_cp/Inc/ultrasonic.h`](./Elevator_working_ver/Core_cp/Inc/ultrasonic.h) | 층별 거리 임계값·tolerance·hit count 튜닝 파라미터 |
| [`Core_cp/Inc/stepper.h`](./Elevator_working_ver/Core_cp/Inc/stepper.h) | 스텝 수, 딜레이, 회전량, 타임아웃 상수 |
| [`Core_cp/Src/fnd.c`](./Elevator_working_ver/Core_cp/Src/fnd.c) | 7세그먼트 층 표시 |

---

## 13. Result and Learning

### Result

- 스텝 수 추정이 아닌 **초음파 실측 기반 층 판정**으로 반복 이동 시에도 위치 오차가 누적되지 않는 구조 구현
- hit count와 tolerance 조합으로 센서 노이즈와 실제 도착을 구분
- 문–모터 인터록으로 "문이 열린 채 이동"이라는 위험 상태를 구조적으로 차단
- 물리 버튼과 Bluetooth 앱 두 입력 경로를 동일한 이동 API로 통합
- 부저·문·초음파 대기 구간을 모두 논블로킹화하여 RTOS 없이 다중 동작 병행
- 30초 타임아웃과 UART 오류 복구로 비정상 상황에서도 시스템이 멈추지 않도록 처리

### What I Learned

- Input Capture로 펄스 폭을 재려면 16-bit 카운터가 한 바퀴 도는 경우를 따로 보정해야 했습니다.
- 폴라리티 전환 방식 캡처는 엣지를 한 번 놓치면 상태가 어긋난 채로 계속 갑니다. 복구 경로를 넣어야 했습니다.
- 개루프 스텝 제어만으로는 층에 정확히 서지 못해, 센서 피드백으로 정지 시점을 판정하도록 바꿨습니다.
- `HAL_IncTick()`을 재정의하면 CubeMX가 생성한 코드를 건드리지 않고도 1 ms 주기 작업을 넣을 수 있습니다.
- ISR과 메인 컨텍스트가 함께 보는 상태에 `volatile`을 빼먹으면 최적화 단계에서 갱신이 사라집니다.
- `HAL_Delay()`를 tick 비교 상태 머신으로 바꾸는 리팩터링을 부저·문·스텝모터에 반복 적용했습니다.
- UART Overrun 같은 오류 플래그는 클리어 절차를 지켜야 수신 인터럽트가 다시 살아납니다.
- 도착 알림은 상태 전이 엣지(`previous == RUNNING && current == ARRIVED`)로 잡아야 매 루프마다 재발행되지 않습니다.
- 소스 트리를 복제해 실험할 때 어느 트리가 실제 빌드 대상인지 문서에 적어두지 않으면 설명과 코드가 어긋납니다.

---

## 14. Future Improvements

- `Core`와 `Core_cp` 트리 통합 — `HAL_IncTick()` 훅 + `volatile`(Core)과 문 인터록 + `pendingMoveFloor`(Core_cp)를 한쪽으로 합치고, `main_for_test.c`는 빌드 구성으로 분리
- `stepper.h`의 방향 주석(`음수 = CCW = 상승`)을 실제 구현(`음수 → CW`)에 맞게 정정
- `BT_MoveDone_Process()`의 `#0` / `#2` 디스플레이 배치를 다른 경로와 통일
- 다중 층 호출을 저장하고 진행 방향에 맞춰 순차 처리하는 **호출 큐 및 스케줄링** 도입
- `__io_putchar()`를 재정의해 `stepper.c`의 `printf` 디버그 로그를 USART2로 실제 출력
- 리미트 스위치 또는 엔코더를 추가해 초음파 사각지대에서도 위치 확인 가능하도록 이중화
- 문 사이에 물체 감지 센서를 추가해 닫히는 중 재열림 처리
- 이동 중 가감속 프로파일 적용으로 진동 감소
- 층별 거리 임계값을 하드코딩 대신 부팅 시 캘리브레이션으로 산출
- 초음파 대기 80 ms를 타이머 인터럽트 기반 비동기 측정으로 전환

---

<div align="center">

GitHub: [@LDdd130](https://github.com/LDdd130)

</div>
