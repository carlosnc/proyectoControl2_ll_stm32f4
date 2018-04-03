#include "cnc_ll_uart.h"
#include "string.h"

// =============================================================================

// =============================================================================
static __I  uint8_t  BASH_SIZE    = 80;
static __I  uint32_t TIMEOUT_MAX  = 1000;
static __IO uint32_t timeout      = 0;

// =============================================================================
static inline uint8_t cncUSART_putChar(USART_TypeDef *USARTx, uint8_t ch)
{
  if(LL_USART_GetDataWidth(USARTx) == LL_USART_DATAWIDTH_8B)
  {
    timeout = TIMEOUT_MAX;
    while(LL_USART_IsActiveFlag_TXE(USARTx) != 1)
    {
      if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
        if(--timeout == 0)
          return 0;
    }

    LL_USART_TransmitData8(USARTx, (ch & 0xFF));

    timeout = TIMEOUT_MAX;
    while(LL_USART_IsActiveFlag_TC(USARTx) != 1)
    {
      if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
        if(--timeout == 0)
          return 0;
    }
  }

  return 1;
}

static inline uint16_t cncUSART_Dec2Int(float32_t x)
{
  return (uint16_t)((x - (int16_t)x)*1000);
}

// =============================================================================
uint8_t cncUSART_init(USART_TypeDef *USARTx)
{
  uint8_t status = 0x00;
  uint32_t periphClock = 0;

  LL_GPIO_InitTypeDef  GPIO_InitStruct;

  if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_UART5) != 1)
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UART5);

  /*!< APB1 clock = SystemCoreClock/4 */
  periphClock = (SystemCoreClock/4);

  LL_USART_SetHWFlowCtrl(UART5, LL_USART_HWCONTROL_NONE);
  LL_USART_SetBaudRate(UART5, periphClock, LL_USART_OVERSAMPLING_16, 115200);
  LL_USART_ConfigCharacter(UART5, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
  LL_USART_SetTransferDirection(UART5, LL_USART_DIRECTION_TX_RX);
  LL_USART_ConfigAsyncMode(UART5);
  LL_USART_Enable(UART5);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_12;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
  LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  return status;
}

uint8_t cncUSART_putString(USART_TypeDef *USARTx, uint8_t *pStr, uint8_t count)
{
  __IO uint8_t len = count;
  __IO uint8_t n = 0;

  /*Check if USART instance is enabled*/
  if (LL_USART_IsEnabled(USARTx) != 1)
    LL_USART_Enable(USARTx);

  /*Check if string in nule or size zero*/
  if ((pStr == 0) || (len == 0))
    return 0;

  /*Send byte until finish or null character*/
  while ((*pStr != '\0') && (len--))
  {
    if (cncUSART_putChar(USARTx, *pStr++) == 1)
      n++;
  }

  /*Return the number of bytes sent*/
  return n;
}

uint8_t cncUSART_send2Bash(USART_TypeDef *USARTx, const bash_cmd_t *cmd, uint8_t *pStr)
{
  uint8_t tmp[BASH_SIZE];
  uint8_t *ptmp = &tmp[0];

  /*Fill vector with null characters*/
  memset(tmp, '\0', BASH_SIZE);
  /*concatenate strings*/
  strncat(ptmp, (uint8_t *)cmd, BASH_SIZE);
  strncat(ptmp, pStr, BASH_SIZE);
  strncat(ptmp, (uint8_t *)bash_Normal, BASH_SIZE);

  /*Send string*/
  if(cncUSART_putString(USARTx, ptmp, BASH_SIZE) == 0)
    return 0;

  return 1;
}

uint8_t cncUSART_sendData_float(USART_TypeDef *USARTx, float32_t *pData, uint8_t vector_len, uart_data_t mode)
{
  uint8_t tmp[10], separator, decimal;
  uint8_t *ptmp = &tmp[0];
  uint8_t *pSeparator = &separator;

  switch (mode & ~0x0001)
  {
    case UART_DATA_FORMAT_TAB:
      pSeparator = (uint8_t *)"\t";
      break;
    case UART_DATA_FORMAT_SPACE:
      pSeparator = (uint8_t *)" ";
      break;
    case UART_DATA_FORMAT_LF:
      pSeparator = (uint8_t *)"\n";
      break;
    case UART_DATA_FORMAT_BS:
      pSeparator = (uint8_t *)"/";
      break;
  }

  if(mode & UART_DATA_LOG)
  {
    for(uint8_t i = 0; i < vector_len; i++)
    {
      memset(ptmp, '\0', 10);
      itoa((int16_t)pData[i], ptmp, 10);
      cncUSART_putString(USARTx, ptmp, 10);
      cncUSART_putString(USARTx, (uint8_t *)".", 10);
      decimal = cncUSART_Dec2Int(pData[i]);
      itoa(decimal, ptmp, 10);
      cncUSART_putString(USARTx, ptmp, 10);
      cncUSART_putString(USARTx, pSeparator, 1);
    }
    if(pSeparator != (uint8_t *)"\n")
      cncUSART_putString(USARTx, (uint8_t *)"\n\r", 2);
  }
  else
  {
    if(pSeparator == (uint8_t *)"\n")
      pSeparator = (uint8_t *)"\t";
    
    cncUSART_send2Bash(USARTx, bash_EraseLine, (uint8_t *)"\r");
    for(uint8_t i = 0; i < vector_len; i++)
    {
      memset(ptmp, '\0', 10);
      itoa((int16_t)pData[i], ptmp, 10);

      if(pData[i] >= 0.0f)
      {
        cncUSART_send2Bash(USARTx, bash_Green, ptmp);
        decimal = cncUSART_Dec2Int(pData[i]);
        itoa(decimal, ptmp, 10);
        cncUSART_putString(USARTx, (uint8_t *)".", 1);
        cncUSART_send2Bash(USARTx, bash_Green, ptmp);
      }
      else
      {
        cncUSART_send2Bash(USARTx, bash_Red, ptmp);
        decimal = cncUSART_Dec2Int(pData[i]);
        itoa(decimal, ptmp, 10);
        cncUSART_putString(USARTx, (uint8_t *)".", 1);
        cncUSART_send2Bash(USARTx, bash_Red, ptmp);

      }

      if(i < (vector_len - 1))
        cncUSART_putString(USARTx, pSeparator, 1);
    }
  }

  return 1;
}

uint8_t cncUSART_sendData_int16(USART_TypeDef *USARTx, int16_t *pData, uint8_t vector_len, uart_data_t mode)
{
    uint8_t tmp[10], separator;
  uint8_t *ptmp = &tmp[0];
  uint8_t *pSeparator = &separator;

  switch (mode & ~0x0001)
  {
    case UART_DATA_FORMAT_TAB:
      pSeparator = (uint8_t *)"\t";
      break;
    case UART_DATA_FORMAT_SPACE:
      pSeparator = (uint8_t *)" ";
      break;
    case UART_DATA_FORMAT_LF:
      pSeparator = (uint8_t *)"\n";
      break;
    case UART_DATA_FORMAT_BS:
      pSeparator = (uint8_t *)"/";
      break;
  }

  if(mode & UART_DATA_LOG)
  {
    for(uint8_t i = 0; i < vector_len; i++)
    {
      memset(ptmp, '\0', 10);
      itoa((int16_t)pData[i], ptmp, 10);
      cncUSART_putString(USARTx, ptmp, 10);
      cncUSART_putString(USARTx, pSeparator, 1);
    }
    if(pSeparator != (uint8_t *)"\n")
      cncUSART_putString(USARTx, (uint8_t *)"\n\r", 2);
  }
  else
  {
    if(pSeparator == (uint8_t *)"\n")
      pSeparator = (uint8_t *)"\t";
    
    cncUSART_send2Bash(USARTx, bash_EraseLine, (uint8_t *)"\r");
    for(uint8_t i = 0; i < vector_len; i++)
    {
      memset(ptmp, '\0', 10);
      itoa((int16_t)pData[i], ptmp, 10);

      if(pData[i] >= 0)
      {
        cncUSART_send2Bash(USARTx, bash_Green, ptmp);
      }
      else
      {
        cncUSART_send2Bash(USARTx, bash_Red, ptmp);
      }

      if(i < (vector_len - 1))
        cncUSART_putString(USARTx, pSeparator, 1);
    }
  }

  return 1;
}

uint8_t cncUSART_receiveData(USART_TypeDef *USARTx, uint8_t *pData, uint8_t len)
{
  return 1;
}

// IRQ Handlers ================================================================


// EOF =========================================================================
