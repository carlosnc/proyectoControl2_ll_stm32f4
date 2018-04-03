// Includes ====================================================================
#include "cnc_ll_i2c.h"

// Structures ==================================================================
typedef struct
{
  GPIO_TypeDef *GPIO_PORT;
  uint32_t GPIO_PIN_SCL;
  uint32_t GPIO_PIN_SDA;
  uint32_t GPIO_ALTERNATE;
} i2c_gpio_t;

// Constants ===================================================================
static __I i2c_gpio_t I2C1_PERIPH = { GPIOB, LL_GPIO_PIN_6, LL_GPIO_PIN_7,
    LL_GPIO_AF_4 };

static __I uint32_t TIMEOUT_MAX = 1000;
static __I uint8_t I2C_ACK = 0x01;
static __I uint8_t I2C_NACK = 0x00;

static __I uint8_t I2C_ADD0_READ = (0x01 << 0);
static __I uint8_t I2C_ADD0_WRITE = ~(0x01 << 0);

static __I uint8_t I2C_MODE_WRITE = 0x00;
static __I uint8_t I2C_MODE_READ = 0x01;

// =============================================================================
static __IO uint32_t timeout = 0;

// Private functions prototypes ================================================
static uint8_t cncI2C_Start(
    I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t mode, uint8_t ack);
static uint8_t cncI2C_Stop(I2C_TypeDef *I2Cx);
static uint8_t cncI2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data);
static uint8_t cncI2C_ReadData(I2C_TypeDef *I2Cx, uint8_t ack);

// Public functions ============================================================
/*******************************************************************************
 * @brief   Init I2C bus.
 * @param   I2Cx: I2C instance.
 * @param   freq: Clock frequency.
 * @retval  1 if the device is connected or 0 isn't.
 ******************************************************************************/
uint8_t cncI2C_Init(I2C_TypeDef *I2Cx, uint32_t freq)
{
  // TODO: Implement others I2C instances.
  LL_I2C_InitTypeDef I2C_InitStruct;
  LL_GPIO_InitTypeDef GPIO_InitStruct;

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  GPIO_InitStruct.Pin = (I2C1_PERIPH.GPIO_PIN_SCL | I2C1_PERIPH.GPIO_PIN_SDA);
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = I2C1_PERIPH.GPIO_ALTERNATE;
  LL_GPIO_Init(I2C1_PERIPH.GPIO_PORT, &GPIO_InitStruct);

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
  I2C_InitStruct.PeripheralMode = LL_I2C_MODE_I2C;
  I2C_InitStruct.ClockSpeed = freq;
  I2C_InitStruct.DutyCycle = LL_I2C_DUTYCYCLE_2;
  I2C_InitStruct.OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT;
  I2C_InitStruct.OwnAddress1 = 0x00;
  I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
  LL_I2C_Init(I2Cx, &I2C_InitStruct);

  LL_I2C_SetOwnAddress2(I2Cx, 0x00);
  LL_I2C_DisableOwnAddress2(I2Cx);
  LL_I2C_DisableGeneralCall(I2Cx);
  LL_I2C_EnableClockStretching(I2Cx);

  return 1;
}

/*******************************************************************************
 * @brief   Check if the device is connected on the I2C bus.
 * @param   I2Cx: I2C instance.
 * @param   devAddr: 7 bits device's address.
 * @retval  1 if the device is connected or 0 isn't.
 ******************************************************************************/
uint8_t cncI2C_isDeviceReady(I2C_TypeDef *I2Cx, uint8_t devAddr)
{
  __IO uint8_t tmpAddr = (devAddr << 1);

  /*Check if I2C instance is enabled*/
  if(LL_I2C_IsEnabled(I2Cx) != 1)
    LL_I2C_Enable(I2Cx);

  while(LL_I2C_IsActiveFlag_BUSY(I2Cx) == 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }
  // TODO: Change for cncI2C_Start function.
  LL_I2C_GenerateStartCondition(I2Cx);

  /*Wait for Start condition flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_SB(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
  tmpAddr &= I2C_ADD0_WRITE;
  LL_I2C_TransmitData8(I2Cx, tmpAddr);

  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_ADDR(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  LL_I2C_ClearFlag_ADDR(I2Cx);
  LL_I2C_GenerateStopCondition(I2Cx);

  return 1;
}

/*******************************************************************************
 * @brief   Write single byte to slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   data: byte to be written.
 * @retval  1 if successful.
 ******************************************************************************/
uint8_t cncI2C_WriteByte(I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t data)
{
  /*Check if I2C instance is enabled*/
  if(LL_I2C_IsEnabled(I2Cx) != 1)
    LL_I2C_Enable(I2Cx);

  /*Check if I2C bus is busy*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_BUSY(I2Cx) == 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_WRITE, I2C_NACK) != 1)
    return 0;
  if(cncI2C_WriteData(I2Cx, pDev[1]) != 1)
    return 0;
  if(cncI2C_WriteData(I2Cx, data) != 1)
    return 0;
  if(cncI2C_Stop(I2Cx) != 1)
    return 0;

  return 1;
}

/*******************************************************************************
 * @brief   Write multiple bytes to slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   pData: pointer to byte array to be written.
 * @retval  Number of bytes sent.
 ******************************************************************************/
uint8_t cncI2C_WriteMultipleBytes(
    I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData, uint8_t count)
{
  uint8_t n = 0;

  /*Check if I2C instance is enabled*/
  if(LL_I2C_IsEnabled(I2Cx) != 1)
    LL_I2C_Enable(I2Cx);

  /*Check if I2C bus is busy*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_BUSY(I2Cx) == 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_WRITE, I2C_NACK) != 1)
    return 0;
  if(cncI2C_WriteData(I2Cx, pDev[1]) != 1)
    return 0;

  while(count--)
  {
    if(cncI2C_WriteData(I2Cx, *pData++) == 1)
      n++;
  }

  if(cncI2C_Stop(I2Cx) != 1)
    return 0;

  return n;
}

/*******************************************************************************
 * @brief   Read single byte from slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   data: byte to be read from slave.
 * @retval  1 if sucessful. 0 if fail.
 ******************************************************************************/
uint8_t cncI2C_ReadByte(I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData)
{
  if(LL_I2C_IsEnabled(I2Cx) != 1)
    LL_I2C_Enable(I2Cx);

  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_BUSY(I2Cx) == 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_WRITE, I2C_NACK) != 1)
    return 0;
  if(cncI2C_WriteData(I2Cx, pDev[1]) != 1)
    return 0;
  if(cncI2C_Stop(I2Cx) != 1)
    return 0;
  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_READ, I2C_NACK) != 1)
    return 0;

  *pData = cncI2C_ReadData(I2Cx, I2C_NACK);

  return 1;
}

/*******************************************************************************
 * @brief   Read multiple bytes from slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   pData: pointer to data array to store bytes from slave.
 * @retval  1 if successful. 0 if fail.
 ******************************************************************************/
uint8_t cncI2C_ReadMultipleBytes(
    I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData, uint8_t count)
{
  if(LL_I2C_IsEnabled(I2Cx) != 1)
    LL_I2C_Enable(I2Cx);

  /*Check if I2C bus is busy*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_BUSY(I2Cx) == 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_WRITE, I2C_ACK) != 1)
    return 0;
  if(cncI2C_WriteData(I2Cx, pDev[1]) != 1)
    return 0;
  if(cncI2C_Start(I2Cx, pDev[0], I2C_MODE_READ, I2C_ACK) != 1)
    return 0;

  while(count--)
  {
    if(count != 0)
      *pData++ = cncI2C_ReadData(I2Cx, I2C_NACK);
    else
      *pData++ = cncI2C_ReadData(I2Cx, I2C_ACK);
  }

  return 1;
}

// Private functions ===========================================================
/*******************************************************************************
 * @brief   Generate start condition.
 * @param   I2Cx: I2C instance.
 * @param   devAddr: 7 bits device's address.
 * @param   Mode: I2C_MODE_READ or I2C_MODE_WRITE.
 * @param   ack: Acknowledge type.
 * @retval  1 for successful.
 ******************************************************************************/
static uint8_t cncI2C_Start(
    I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t mode, uint8_t ack)
{
  __IO uint8_t tmpAddr = (devAddr << 1);

  LL_I2C_GenerateStartCondition(I2Cx);
  /*Wait for Start condition flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_SB(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(ack == I2C_ACK)
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
  else
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);

  (mode == I2C_MODE_WRITE) ? (tmpAddr &= I2C_ADD0_WRITE) : (tmpAddr |=
                                 I2C_ADD0_READ);
  LL_I2C_TransmitData8(I2Cx, tmpAddr);

  /*Wait for address sent flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_ADDR(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  /*Clear address sent flag*/
  LL_I2C_ClearFlag_ADDR(I2Cx);

  return 1;
}

/*******************************************************************************
 * @brief   Generate stop condition.
 * @param   I2Cx: I2C instance.
 * @retval  None.
 ******************************************************************************/
static uint8_t cncI2C_Stop(I2C_TypeDef *I2Cx)
{
  /*Wait for TXE(Transmit data register is Empty) flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_TXE(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  /*Wait for BTF(Byte Transfer Finished) flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_BTF(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  LL_I2C_GenerateStopCondition(I2Cx);

  return 1;
}

/*******************************************************************************
 * @brief   Write single byte.
 * @param   I2Cx: I2C instance.
 * @param   data: Data to be written.
 * @retval  1 if successful. 0 if fail.
 ******************************************************************************/
static uint8_t cncI2C_WriteData(I2C_TypeDef *I2Cx, uint8_t data)
{
  /*Wait for TXE(Transmit data register is Empty) flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_TXE(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout)
        return 0;
    }
  }

  LL_I2C_TransmitData8(I2Cx, data);

  return 1;
}

/*******************************************************************************
 * @brief   Write single byte.
 * @param   I2Cx: I2C instance.
 * @param   ack: Acknowledge type.
 * @retval  Data from slave. 0 if fail.
 ******************************************************************************/
static uint8_t cncI2C_ReadData(I2C_TypeDef *I2Cx, uint8_t ack)
{
  /*Wait for RXNE(RX data register is Not Empty) flag*/
  timeout = TIMEOUT_MAX;
  while(LL_I2C_IsActiveFlag_RXNE(I2Cx) != 1)
  {
    if((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0)
    {
      if(--timeout == 0)
        return 0;
    }
  }

  if(ack == I2C_ACK)
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
  else
  {
    LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
    LL_I2C_GenerateStopCondition(I2Cx);
  }

  return LL_I2C_ReceiveData8(I2Cx);
}

// EOF =========================================================================
