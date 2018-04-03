#ifndef CNC_LL_UART_H_
#define CNC_LL_UART_H_

// Includes ====================================================================
#include "stm32f4xx.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_utils.h"

#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_usart.h"

// =============================================================================
typedef uint8_t bash_cmd_t;
typedef float float32_t;

typedef enum
{
    UART_DATA_DISPLAY = 0,
    UART_DATA_LOG = (0x01 << 0),
    UART_DATA_FORMAT_TAB = (0x01 << 1),
    UART_DATA_FORMAT_SPACE = (0x01 << 2),
    UART_DATA_FORMAT_LF = (0x01 << 3),
    UART_DATA_FORMAT_BS = (0x01 << 4),
} uart_data_t;

// Linux bash sequences ========================================================
static const bash_cmd_t bash_ClearScreen[] = "\033[2J";
static const bash_cmd_t bash_EraseLine[]   = "\033[k";
static const bash_cmd_t bash_Cursor2Home[] = "\033[0;0H";
static const bash_cmd_t bash_Normal[]      = "\033[0m";
static const bash_cmd_t bash_Black[]       = "\033[0;30m";
static const bash_cmd_t bash_DarkGray[]    = "\033[1;30m";
static const bash_cmd_t bash_LightGray[]   = "\033[0;37m";
static const bash_cmd_t bash_White[]       = "\033[1;37m";
static const bash_cmd_t bash_Red[]         = "\033[0;31m";
static const bash_cmd_t bash_LightRed[]    = "\033[1;31m";
static const bash_cmd_t bash_Green[]       = "\033[0;32m";
static const bash_cmd_t bash_LightGreen[]  = "\033[1;32m";
static const bash_cmd_t bash_Blue[]        = "\033[0;34m";
static const bash_cmd_t bash_LightBlue[]   = "\033[1;34m";
static const bash_cmd_t bash_Purple[]      = "\033[0;35m";
static const bash_cmd_t bash_LightPurple[] = "\033[1;35m";
static const bash_cmd_t bash_Cyan[]        = "\033[0;36m";
static const bash_cmd_t bash_LightCyan[]   = "\033[1;36m";
static const bash_cmd_t bash_Brown[]       = "\033[0;33m";
static const bash_cmd_t bash_Yellow[]      = "\033[1;33m";

// =============================================================================
uint8_t cncUSART_init(USART_TypeDef *USARTx);
uint8_t cncUSART_putString(USART_TypeDef *USARTx, uint8_t *pStr, uint8_t count);
uint8_t cncUSART_send2Bash(USART_TypeDef *USARTx, const bash_cmd_t *cmd, uint8_t *pStr);
uint8_t cncUSART_sendData_float(USART_TypeDef *USARTx, float32_t *pData, uint8_t vector_len, uart_data_t mode);
uint8_t cncUSART_sendData_int16(USART_TypeDef *USARTx, int16_t *pData, uint8_t vector_len, uart_data_t mode);
uint8_t cncUSART_receiveData(USART_TypeDef *USARTx, uint8_t *pData, uint8_t len);

// =============================================================================
static inline void cncUSART_enable(USART_TypeDef *USARTx)
{
    if(LL_USART_IsEnabled(USARTx) != 1)
        LL_USART_Enable(USARTx);
}

static inline void cncUSART_disable(USART_TypeDef *USARTx)
{
    if(LL_USART_IsEnabled(USARTx) == 1)
        LL_USART_Disable(USARTx);
}

#endif /* CNC_LL_UART_H_ */
// EOF =========================================================================
