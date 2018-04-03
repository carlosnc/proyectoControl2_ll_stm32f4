#ifndef INITHARDWARE_H_
#define INITHARDWARE_H_

// =============================================================================
#include "stm32f4xx.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_utils.h"

#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_exti.h"
#include "stm32f4xx_ll_tim.h"

#include "cnc_ll_uart.h"
#include "mpu9250.h"
#include "estimador.h"
#include "servomotor.h"

// =============================================================================
#ifndef NVIC_PRIORITYGROUP_0
#define NVIC_PRIORITYGROUP_0         ((uint32_t)0x00000007) /*!< 0 bit  for pre-emption priority, 4 bits for subpriority */
#define NVIC_PRIORITYGROUP_1         ((uint32_t)0x00000006) /*!< 1 bit  for pre-emption priority, 3 bits for subpriority */
#define NVIC_PRIORITYGROUP_2         ((uint32_t)0x00000005) /*!< 2 bits for pre-emption priority, 2 bits for subpriority */
#define NVIC_PRIORITYGROUP_3         ((uint32_t)0x00000004) /*!< 3 bits for pre-emption priority, 1 bit  for subpriority */
#define NVIC_PRIORITYGROUP_4         ((uint32_t)0x00000003) /*!< 4 bits for pre-emption priority, 0 bit  for subpriority */
#endif

// =============================================================================
/*!< LEDs and Button - STM32F4-Discovery */
typedef struct
{
    GPIO_TypeDef *GPIO_Port;
    uint32_t      GPIO_Pin;
} board_gpio_t;

// =============================================================================
static const uint8_t SAMPLER_FREQ  = 100;  /*!< Hertz */

static const uint32_t LED_BLINK_FAST    = 150;  /*!< mseconds */
static const uint32_t LED_BLINK_MEDIUM  = 250;  /*!< mseconds */
static const uint32_t LED_BLINK_SLOW    = 500;  /*!< mseconds */

static const board_gpio_t LED_GREEN   = {GPIOD, (0x01 << 12)}; /*!< GPIOD Pin 12 */                                                               /*!< Also TIM4 output */
static const board_gpio_t LED_ORANGE  = {GPIOD, (0x01 << 13)}; /*!< GPIOD Pin 13 */
static const board_gpio_t LED_RED     = {GPIOD, (0x01 << 14)}; /*!< GPIOD Pin 14 */
static const board_gpio_t LED_BLUE    = {GPIOD, (0x01 << 15)}; /*!< GPIOD Pin 15 */
static const board_gpio_t BUTTON      = {GPIOA, (0x01 << 0)};  /*!< GPIOA Pin 0  */

// Public functions prototypes =================================================
void initHardware_InitSystem(void);
void initHardware_TestOutput(void);

void Error_Handler(void);

// In-line functions ===========================================================


#endif /* INITHARDWARE_H_ */
// EOF =========================================================================
