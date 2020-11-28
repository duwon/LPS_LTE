/**
 ******************************************************************************
 * @file    user.c
 * @author  정두원
 * @date    2020-11-02
 * @brief   메인 구동 함수
 * @details
 */

/**
 * @mainpage LTE 연동 보드
    @section intro 소개
    - IO Board 
 * @section info 정보
    - LTE 모뎀 제어
    - 외부 디바이스 센싱
 * @section  CREATEINFO      작성정보
    - 작성자      :   정두원
    - 작성일      :   2020-11-02
 * @section  MODIFYINFO      수정정보
    - 2020-11-02    :    프로젝트 셋팅
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rtc.h"
#include "user.h"
#include "uart.h"
#include "tim.h"
#include "adc.h"

#define WAKEUP_INTERVAL 600   /*!< 센싱 주기 단위: 초 */
#define SENSING_TIMES 6       /*!< 센싱 정보 저장 횟수 최대 10개 */
#define OPMODE_TIMEOUT 2      /*!< 단위: 초 */
#define RETRANSMISSIONS_CNT 2 /*!< 재전송 횟수 */

//#define DEBUG_PRINT(...) printf(__VA_ARGS__) /* 디버깅 용 */
#define DEBUG_PRINT(...)

typedef enum
{
    BOOTING = 0,
    STANDBY,
    POWERUP,
    RUNNING,
    POWERDOWN,
    POWEROFF,
    SENSING,
    CHECKUSIM,
    SENDING,
    ACKCHECKING,
    WAITING,
    CHECKINGIP,
    CHECKINGNETWORKING,
    WHTTP_POST,
    WHTTP_HEAD,
    WHTTP_DATA,
    WHTTP_SEND,
    TIMEOUT
} OperatingStage; /*!< 운용모드 */

bool flag_UserBtnOn = false;        /*!< 사용자 버튼 누름 상태 */
bool flag_1SecTimerOn = false;      /*!< 1초 플래그 */
bool flag_1mSecTimerOn = false;     /*!< 1m초 플래그 */
bool flag_UartInterruptEnd = false; /*!< LTE 모뎀의 UART 수신 완료 */
bool flag_OpmodeTimeout = false;    /*!< WAIT 모드에서 Timeout 플래그 */

bool falg_Answer = false;
static OperatingStage OPMode, OPModeNext, OPModeLast;

uint32_t sendingCount = 0;  /*!< 전송 횟수 */
uint16_t sensingCount = 0;  /*!< 디바이스 센싱 횟수. SENSING_TIMES 설정 값이 최대 */
uint16_t sendFailCount = 0; /*!< 전송 실패 횟수 */

uint16_t ADCValue[3]; /*!< ADC 값. [0] BAT, [1] DEVICE, [2] REFENCE 3.3V */

void enterStandByMode(uint32_t delaySec);
void ParsingAckMessage(void);
void saveSensingData(uint8_t cntSensing, uint32_t *Vdevice, uint32_t *Vbat, uint8_t Din);
void loadSensingData(uint8_t cntSensing, uint32_t *Vdevice, uint32_t *Vbat, uint8_t *Din);
uint8_t readDINValue(void);

/**
 * @brief 사용자 시작 함수 - 시작시 1회 수행
 *
 */
void userStart(void)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);                          /* LED ON */
    HAL_GPIO_WritePin(PWR_RS232_GPIO_Port, PWR_RS232_Pin, GPIO_PIN_SET); /* MAX3232 전원 ON */

    Uart_Init();                   /* UART 초기화 */
    HAL_TIM_Base_Start_IT(&htim6); /* 1ms 타이머 인터럽트 시작 */
    DEBUG_PRINT("\r\nSTART APPLICATION\r\n");

    sendingCount = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR30);                   /* 전송 횟수 불러오기 */
    sensingCount = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR31) & 0xFFFF;          /* 센싱 횟수 불러오기 */
    sendFailCount = (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR31) >> 16) & 0xFFFF; /* 전송 실패 횟수 불러오기 */
    DEBUG_PRINT("sensingCount : %d, sendingCount : %d, sendFailCount : %d\r\n", sensingCount, sendingCount, sendFailCount);

    if (HAL_GPIO_ReadPin(USER_BTN_GPIO_Port, USER_BTN_Pin) == GPIO_PIN_RESET) /* 사용자 버튼 누름상태 체크 */
    {
        flag_UserBtnOn = true;
    }

    OPMode = WAITING;

    HAL_GPIO_WritePin(PWR_BATCHECK_GPIO_Port, PWR_BATCHECK_Pin, GPIO_PIN_SET); /* 배터리 체크를 위한 전압 입력 ON */
    HAL_GPIO_WritePin(PWR_12V_GPIO_Port, PWR_12V_Pin, GPIO_PIN_SET);           /* 외부 디바이스 전력 공급 ON */

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED); /* ADC Calibration */
    HAL_Delay(1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADCValue, 3); /* ADC 시작 */
}

/**
 * @brief 사용자 Loop 함수 
 * 
 */
void userLoop(void)
{
    if (flag_1SecTimerOn) /* 1초 주기마다 실행 */
    {
        flag_1SecTimerOn = false;
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin); /* LED 토글 */
    }

    if (flag_UartInterruptEnd) /* LTE 모뎀의 수신 데이터가 있으면 메시지 분석*/
    {
        flag_UartInterruptEnd = false;
        ParsingAckMessage();
    }

    /* DEBUG 포트 입력 데이터를 LTE UART 포트로 전송 UART1:LTE모뎀, UART2:DEBUG - 디버그용 */
    if (getByteFromBuffer(&uart2Buffer, &uart2Buffer.buffCh) == SUCCESS) /* UART2 버퍼에 데이터가 있으면 */
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)&uart2Buffer.buffCh, 1, 0xFFFF); /* UART1로 데이터 전송 */
        //HAL_UART_Transmit(&huart2, (uint8_t *)&uart2Buffer.buffCh, 1, 0xFFFF);
    }

    char *tmpTxData;                         /*!< LTE모뎀으로 전송을 위한 데이터 버퍼 */
    char arrTxBuffer[300];                   /*!< 서버에 사용자 데이터 전송을 위한 버퍼 */
    int tmpTxDataLength;                     /*!< 사용자 데이터 길이 저장용 */
    static int resendCount = 0;              /*!< 재전송 횟수 */
    uint8_t DINValue[SENSING_TIMES];         /*!< 서버에 보낼 때 데이터 저장용 */
    float ADCVoltageValue[SENSING_TIMES][2]; /*!< 서버에 보낼 때 데이터 저장용 */
    float VoltageBAT, VoltageDevice;

    switch (OPMode)
    {
    case BOOTING:
        HAL_GPIO_WritePin(LTE_WAKEUP_GPIO_Port, LTE_WAKEUP_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        char *tmpData = "ATE0\r\n"; /* LTE 모뎀의 UART ECHO OFF */
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpData, strlen(tmpData), 0xFFFF);

        tmpTxData = "AT*CPIN?\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = CHECKINGNETWORKING;
        OPMode = WAITING;
        break;
    case CHECKINGNETWORKING:
        tmpTxData = "AT+CEREG?\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = CHECKINGIP;
        OPMode = WAITING;
        break;
    case CHECKINGIP:
        tmpTxData = "AT*WWANIP?\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = WHTTP_POST;
        OPMode = WAITING;
        break;
    case WHTTP_POST:
        tmpTxData = "AT*WHTTP=0,POST,dbos.co.kr/smlf_api_v_2_1/test/set\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = WHTTP_HEAD;
        OPMode = WAITING;
        break;
    case WHTTP_HEAD:
        tmpTxData = "AT*WHTTP=2,HEAD,Content-Type: application/x-www-form-urlencoded\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = WHTTP_DATA;
        OPMode = WAITING;
        break;
    case WHTTP_DATA:
        for (int i = 0; i < SENSING_TIMES; i++)
        {
            loadSensingData(i, (uint32_t *)&ADCVoltageValue[i][0], (uint32_t *)&ADCVoltageValue[i][1], &DINValue[i]);
        }
        tmpTxDataLength = sprintf(arrTxBuffer, "AT*WHTTP=2,DATA,send=%ld\\&Fail=%d\\&V1=%.2f\\&V2=%.2f\\&V3=%.2f\\&V4=%.2f\\&V5=%.2f\\&V6=%.2f\\&B1=%.2f\\&B2=%.2f\\&B3=%.2f\\&B4=%.2f\\&B5=%.2f\\&B6=%.2f\\&D1=0x%x\\&D2=0x%x\\&D3=0x%x\\&D4=0x%x\\&D5=0x%x\\&D6=0x%x\r\n", sendingCount, sendFailCount, ADCVoltageValue[0][0], ADCVoltageValue[1][0], ADCVoltageValue[2][0], ADCVoltageValue[3][0], ADCVoltageValue[4][0], ADCVoltageValue[5][0], ADCVoltageValue[0][1], ADCVoltageValue[1][1], ADCVoltageValue[2][1], ADCVoltageValue[3][1], ADCVoltageValue[4][1], ADCVoltageValue[5][1], DINValue[0], DINValue[1], DINValue[2], DINValue[3], DINValue[4], DINValue[5]);
        HAL_UART_Transmit(&huart1, (uint8_t *)arrTxBuffer, (uint16_t)tmpTxDataLength, 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = WHTTP_SEND;
        OPMode = WAITING;
        break;
    case WHTTP_SEND:
        tmpTxData = "AT*WHTTP=3\r\n\0";
        HAL_UART_Transmit(&huart1, (uint8_t *)tmpTxData, strlen(tmpTxData), 0xFFFF);
        OPModeLast = OPMode;
        OPModeNext = ACKCHECKING;
        OPMode = WAITING;
        break;
    case ACKCHECKING:
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR30, ++sendingCount);
        OPModeLast = WHTTP_SEND;
        OPModeNext = POWEROFF;
        OPMode = WAITING;
        break;
    case SENSING:
        DEBUG_PRINT("sensing.......\r\n");

        VoltageBAT = 1.2f * 4096.0f / ADCValue[2];
        VoltageDevice = ADCValue[1] * ADCValue[2] / 4096;

        saveSensingData(sensingCount, (uint32_t *)&VoltageDevice, (uint32_t *)&VoltageBAT, readDINValue());

        sensingCount++;
        if (sensingCount >= SENSING_TIMES) /* 설정된 센싱 횟수이면 BOOTING 모드로 전환하여 정보 전송 */
        {
            OPMode = BOOTING;
            sensingCount = 0;
        }
        else
        {
            OPMode = POWEROFF;
        }

        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR31, (sendFailCount << 16) + sensingCount);
        break;
    case POWEROFF:
        DEBUG_PRINT("POWER OFF\r\n");
        if (!flag_UserBtnOn) /* 부팅 시 사용자 버튼이 눌리지 않았을 경우 저전력 모드 실행 */
        {
            enterStandByMode(WAKEUP_INTERVAL);
        }
        break;
    case TIMEOUT:
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR31, ((++sendFailCount) << 16) + sensingCount);
        resendCount = 0;
        OPMode = POWEROFF;
        break;
    case WAITING:
        if (falg_Answer == true) /* LTE 모뎀의 응답이 있으면 */
        {
            falg_Answer = false;
            OPMode = OPModeNext;
        }

        if (flag_OpmodeTimeout) /* OPMODE_TIMEOUT 설정 값 안에 LTE 모뎀 응답이 없으면 */
        {
            flag_OpmodeTimeout = false;
            resendCount++;
            if (resendCount >= RETRANSMISSIONS_CNT)
            {
                OPMode = TIMEOUT;
            }
            else
            {
                OPMode = OPModeLast;
                OPModeLast = WAITING;
            }
            DEBUG_PRINT("TimeOut\r\n");
        }
        break;
    default:
        break;
    }
}

/**
 * @brief ADC 완료 인터럽트
 * 
 * @param hadc 
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    OPMode = SENSING;
}

/**
 * @brief LTE 모뎀의 ACK 메시지 분석
 * 
 */
void ParsingAckMessage(void)
{
    char *Query[6] = {"+CPIN", "+CEREG", "*WWANIP", "*WHTTPR", "*WHTTP", "$$MSTIME?"};
    uint8_t QueryIndex = 0;

    char rxMessage[500];
    memcpy(rxMessage, uart1Buffer.buff, uart1Buffer.count);
    rxMessage[uart1Buffer.count] = '\0';
    initBuffer(&uart1Buffer);

    DEBUG_PRINT("*****\r\n%s\r\n*****\r\n", rxMessage);

    for (uint8_t i = 0; i < 6; i++)
    {
        if (strstr(rxMessage, Query[i]) != NULL)
        {
            QueryIndex = i;
            DEBUG_PRINT("%s\r\n", Query[i]);
            break;
        }
    }

    char *anserString = NULL;
    switch (QueryIndex)
    {
    case 0: //+CPIN
        anserString = strtok((char *)rxMessage, "READY");
        if (anserString != NULL)
        {
            falg_Answer = true;
        }
        else
        {
            falg_Answer = false;
        }
        break;
    case 1: //+CEREG
        anserString = strtok((char *)rxMessage, "0,1");
        if (anserString != NULL)
        {
            falg_Answer = true;
        }
        else
        {
            falg_Answer = false;
        }
        break;
    case 2: //*WWANIP
        anserString = strtok((char *)rxMessage, "2001");
        if (anserString != NULL)
        {
            falg_Answer = true;
        }
        else
        {
            falg_Answer = false;
        }
        break;
    case 3: //*WHTTPR
        anserString = strstr((char *)rxMessage, "START");
        char *anserString2 = strstr((char *)rxMessage, "COMPLETED");
        if ((anserString != NULL) || (anserString2 != NULL))
        {
            falg_Answer = true;
        }
        else
        {
            falg_Answer = false;
        }
        break;
    case 4: //*WHTTP
        anserString = strtok((char *)rxMessage, "OK");
        if (anserString != NULL)
        {
            falg_Answer = true;
        }
        else
        {
            falg_Answer = false;
        }
        break;
    case 5: //$$MSTIME? 시간 확인
        break;
    default:
        strtok((char *)rxMessage, ":");
        char *messageType = strtok(NULL, "\r\n"); /* READY */
        if (messageType != NULL)
            DEBUG_PRINT("%s\r\n", messageType);
        break;
    }
}

/**
 * @brief DIN 값 반환
 * 
 * @return uint8_t: [3:0] DIN3~0
 */
uint8_t readDINValue(void)
{
#define DINPORTn 4 /*!< DIN 포트 갯수 */
    GPIO_TypeDef *DIN_PORT[DINPORTn] = {DIN0_GPIO_Port, DIN1_GPIO_Port, DIN2_GPIO_Port, DIN3_GPIO_Port};
    const uint16_t DIN_PIN[DINPORTn] = {DIN0_Pin, DIN1_Pin, DIN2_Pin, DIN3_Pin};

    uint8_t DINValue = 0;
    for (int i = 0; i < DINPORTn; i++)
    {
        if (HAL_GPIO_ReadPin(DIN_PORT[i], DIN_PIN[i]) == GPIO_PIN_SET)
        {
            DINValue |= (1 << i);
        }
    }

    return DINValue;
}

/**
 * @brief 센싱 정보 RTC 백업레지스터에 저장. float형을 uint32으로 변환하여 저장.
 * 
 * @param cntSensing: 센싱 횟수 0~9
 * @param Vdevice: 외부 디바이스 ADC 전압 값
 * @param Vbat: 배터리 ADC 전압 값
 */
void saveSensingData(uint8_t cntSensing, uint32_t *Vdevice, uint32_t *Vbat, uint8_t Din)
{
    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing, *Vdevice);
    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing + 10U, *Vbat);
    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing + 20U, (uint32_t)(Din));
}

/**
 * @brief 
 * 
 * @param cntSensing: 센싱값 회차 0~9
 * @param Vdevice: 반환될 외부 디바이스 전압 값
 * @param Vbat: 반한될 배터리 전압 값
 */
void loadSensingData(uint8_t cntSensing, uint32_t *Vdevice, uint32_t *Vbat, uint8_t *Din)
{
    *Vdevice = HAL_RTCEx_BKUPRead(&hrtc, cntSensing);
    *Vbat = HAL_RTCEx_BKUPRead(&hrtc, cntSensing + 10U);
    *Din = (uint8_t)(HAL_RTCEx_BKUPRead(&hrtc, cntSensing + 20U));

    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing, 0U); /* 읽고 난 후 0으로 초기화 */
    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing + 10U, 0U);
    HAL_RTCEx_BKUPWrite(&hrtc, cntSensing + 20U, 0U);
}

/**
 * @brief  Timer 인터럽트 함수. 이 함수는 HAL_TIM_IRQHandler() 내부에서 TIM6 인터럽트가 발생했을 때 호출됨.
 * @note   TIM6 - 1ms Timer
 *
 * @param  htim : TIM handle
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint32_t count_1s = 0;
    static uint32_t count_OpmodeTimeout = 0;

    if (htim->Instance == TIM6) /* 1ms 타이머 인터럽트 */
    {
        flag_1mSecTimerOn = true;

        count_1s++;
        if (count_1s > 1000) /* 1초 */
        {
            flag_1SecTimerOn = true;
            count_1s = 0;
        }

        if (uartTimeOutCount == 1) /* UART1 (LTE 모뎀) 데이터 수신 시 10으로 셋팅. */
        {
            flag_UartInterruptEnd = true; /* UART 인터럽트가 10ms 이상 없으면 데이터 다 받은 것으로 판단 */
        }
        if (uartTimeOutCount)
        {
            uartTimeOutCount--;
        }

        if (OPMode == WAITING) /* WAITING 모드에서 타임아웃 설정 */
        {
            count_OpmodeTimeout++;
            if (count_OpmodeTimeout > (OPMODE_TIMEOUT << 10))
            {
                flag_OpmodeTimeout = true;
                count_OpmodeTimeout = 0;
            }
        }
        else if (OPMode == POWEROFF) /* POWEROFF 모드에서 타임아웃 설정 */
        {
            count_OpmodeTimeout++;
            if (count_OpmodeTimeout > 20000)
            {
                flag_UserBtnOn = false;
                count_OpmodeTimeout = 0;
            }
        }
        else
        {
            count_OpmodeTimeout = 0;
        }
    }
}

/**
 * @brief 지정된 시간동안 STANDBY 모드 진입
 *
 * @param delayTime: 시간 : 초
 */
void enterStandByMode(uint32_t delaySec)
{
    /* 시스템이 대기 모드에서 재시작되었는지 확인 */
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_SB) != RESET)
    {
        /* Standby flag 클리어 */
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    }

    /* 저전력 모드에서 BOR 및 PVD 공급 모니터링 활성화 */
    HAL_PWREx_EnableBORPVD_ULP();

    /* 모든 wake-up 소스 비활성화 */
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    /* 모든 wake-up 플래그 초기화 */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, delaySec, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);

    /* 스탠바이 모드 진입 */
    HAL_PWR_EnterSTANDBYMode();
}
