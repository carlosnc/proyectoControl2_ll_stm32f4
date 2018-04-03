/*******************************************************************************
 * @file    cnc_ll_i2c.h
 * @author  Contrera Carlos - carlos.n.contrera@gmail.com
 * @brief   Driver for I2C using STM32 Low Layer API.
 * -----------------------------------------------------------------------------
  @verbatim
  ==============================================================================

  ==============================================================================

  @endverbatim
 * -----------------------------------------------------------------------------
 * @attention
 ******************************************************************************/
#ifndef CNC_LL_I2C_H_
#define CNC_LL_I2C_H_

//Includes =====================================================================
#include "stm32f4xx.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_utils.h"

#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_gpio.h"

// Enum & structs ==============================================================
// TODO: Replace funcion returns with enum I2C_STATE

// Public functions ============================================================
/*******************************************************************************
 * @brief   Init I2C bus.
 * @param   I2Cx: I2C instance.
 * @param   freq: Clock frequency.
 * @retval  1 if the device is connected or 0 isn't.
 ******************************************************************************/
uint8_t cncI2C_Init(I2C_TypeDef *I2Cx, uint32_t freq);
/*******************************************************************************
 * @brief   Check if the device is connected on the I2C bus.
 * @param   I2Cx: I2C instance.
 * @param   devAddr: 7 bits device's address.
 * @retval  1 if the device is connected or 0 isn't.
 ******************************************************************************/
uint8_t cncI2C_isDeviceReady(I2C_TypeDef *I2Cx, uint8_t devAddr);
/*******************************************************************************
 * @brief   Write single byte to slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   data: byte to be written.
 * @retval  1 if successful.
 ******************************************************************************/
uint8_t cncI2C_WriteByte(I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t data);
/*******************************************************************************
 * @brief   Write multiple bytes to slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   pData: pointer to byte array to be written.
 * @retval  Number of bytes sent.
 ******************************************************************************/
uint8_t cncI2C_WriteMultipleBytes(
    I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData, uint8_t count);
/*******************************************************************************
 * @brief   Read single byte from slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   data: byte to be read from slave.
 * @retval  1 if successful. 0 if fail.
 ******************************************************************************/
uint8_t cncI2C_ReadByte(I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData);
/*******************************************************************************
 * @brief   Read multiple bytes from slave.
 * @param   I2Cx: I2C instance.
 * @param   pDev: pDev[0]: 7 bits device's address. pDev[1]: register address.
 * @param   pData: pointer to data array to store bytes from slave.
 * @retval  1 if successful. 0 if fail.
 ******************************************************************************/
uint8_t cncI2C_ReadMultipleBytes(
    I2C_TypeDef *I2Cx, uint8_t *pDev, uint8_t *pData, uint8_t count);

// In-line functions ===========================================================
/*******************************************************************************
 * @brief   Enable I2C Instance.
 * @param   I2Cx: I2C instance.
 * @retval  None.
 ******************************************************************************/
static inline void cncI2C_Enable(I2C_TypeDef *I2Cx)
{
  LL_I2C_Enable(I2Cx);
}
/*******************************************************************************
 * @brief   Disable I2C Instance.
 * @param   I2Cx: I2C instance.
 * @retval  None.
 ******************************************************************************/
static inline void cncI2C_Disable(I2C_TypeDef *I2Cx)
{
  LL_I2C_Disable(I2Cx);
}

#endif /* CNC_LL_I2C_H_ */
// EOF =========================================================================
