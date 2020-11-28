#ifndef USER_H__
#define USER_H__ 1

#include "main.h"

void userStart(void);
void userLoop(void);

#define SEND_STATUS_INTERVAL 1000U /*!< 상태 전송 주기. 단위 ms */
#define VERSION_MAJOR 0U
#define VERSION_MINOR 1U

#pragma pack(push, 1) /* 1바이트 크기로 정렬  */
typedef struct
{
  uint8_t Do[2];      // 디폴트 설정값  0=Off
  uint8_t Rtd_Cycle;  // 측정주기 sec	  초기값 10
  uint8_t Ai_Cycle;   // 측정주기 sec   초기값 1
  uint8_t Di_Cycle;   // 측정주기 sec   초기값 1
  uint8_t Dps_Cycle;  // 측정주기 sec   초기값 1
  uint8_t Ps_Cycle;   // 측정주기 sec   초기값 1
  uint8_t Pm_Mode;    // 전산적력 측정 방식 0=3상, 1=단상
  uint8_t Pm_Cycle;   // 측정주기 sec   초기값 60
  uint16_t Pm_Volt;   // 파워메터 기준진압                 초기값 220
  uint8_t Pm_Current; // 파워메터 기전전류  100 -> 10.0 A  초기값 50
  uint8_t Pm_Freq;    // 파워메터 기준주파수               초기값 60
} Io_Config_TypeDef;
#pragma pack(pop)

typedef struct
{
  //-------------------------------- PowerMeter
  float Volt;          // 전압
  float Current;       // 전류
  float Cos;           // 역률
  float Active;        // 유효전력
  float Reactive;      // 무효전력
  float Apparent;      // 피상전력
  float Active_Energy; // 유효전력량

  //-------------------------------- Odd I/O 상태값
  uint16_t Rtd;   // MSB=정수, LSB=소수점
  uint16_t Dps;   // MSB=정수, LSB=소수점  차압센서
  uint16_t Ps;    // MSB=정수, LSB=소수점  압력센서
  uint16_t Ai[2]; // MSB=정수, LSB=소수점
  uint8_t Di[4];  // 0=Off, 1=On
  uint8_t Do[2];  // 0=Off, 1=On
} Io_Status_TyeDef;

Io_Config_TypeDef stIOConfig;
Io_Status_TyeDef stIOStatus;

#endif /* USER_H__ */
