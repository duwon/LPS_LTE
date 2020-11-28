#ifndef UART_H__
#define UART_H__ 1

#include "main.h"
#include "usart.h"

#define UART_BUFFER_SIZE 600U
#define MESSAGE_MAX_SIZE 300U

#define MESSAGE_STX 0x02U
#define MESSAGE_ETX 0x03U

typedef enum
{
    START = 0,
    MSGID = 1,
    LENGTH = 10,
    DATA = 2,
    PARSING = 3,
    CHECKSUM = 4,
    SEND = 5,
    RESEND = 6,
    STOP = 7,
    WRITE = 8,
    END = 9
} MessageStage; /*!< 메시지 패킷 분석 단계 */

typedef struct
{
    uint16_t in;
    uint16_t out;
    uint16_t count;
    uint8_t buff[UART_BUFFER_SIZE];
    uint8_t rxCh;
    uint8_t buffCh;
} uartFIFO_TypeDef; /*!< 수신 패킷 저장 버퍼 구조체 */

typedef struct
{
    MessageStage lastStage;
    MessageStage nextStage;
    uint8_t sendingCount;
    uint8_t msgID;
    uint8_t datasize;
    uint8_t data[MESSAGE_MAX_SIZE];
    uint8_t checksum;
    void (*parsing)(void);
} message_TypeDef; /*!< 수신 메시지 구조체 */

/* Extern variables ---------------------------------------------------------*/
extern uartFIFO_TypeDef uart1Buffer; /*!< UART1 링 버퍼 구조체 - RS232 */
extern uartFIFO_TypeDef uart2Buffer; /*!< UART2 링 버퍼 구조체 - RS485 */

extern message_TypeDef uart1Message; /*!< UART2 메시지 구조체 - RS485 */
extern message_TypeDef uart2Message; /*!< UART4 메시지 구조체- Raspberry Pi */

extern uint8_t uartTimeOutCount;

/* Extern functions ---------------------------------------------------------*/
void Uart_Init(void);                                                          /*!< UART 관련 설정 초기화 */
ErrorStatus getByteFromBuffer(volatile uartFIFO_TypeDef *buffer, uint8_t *ch); /*!< 버퍼에서 1Byte 읽기 */
void initBuffer(uartFIFO_TypeDef *buffer);

#endif /* UART_H__ */
