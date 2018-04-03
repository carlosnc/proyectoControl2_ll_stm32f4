#ifndef SERVOMOTOR_H_
#define SERVOMOTOR_H_

// =============================================================================
#include "stm32f4xx.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_cortex.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_pwr.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_utils.h"

#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_tim.h"

// =============================================================================
typedef float float32_t;

typedef enum
{
  SERVO_CHANNEL_1 = (0x01 << 0),
  SERVO_CHANNEL_2 = (0x01 << 4),
  SERVO_CHANNEL_3 = (0x01 << 8),
  SERVO_CHANNEL_ALL = 0x00,
} servo_channel_t;

typedef struct
{
     int16_t Pos_Zero;
     int16_t Min_Angle;
     int16_t Max_Angle;
} servo_initStruct_t;


// =============================================================================

// =============================================================================
void cncServo_init(servo_initStruct_t *servo_initStruct);
void cncServo_check(servo_channel_t servo_channel);
void cncServo_zeroPosition(servo_channel_t servo_channel);
void cncServo_updatePosition(float32_t angle, servo_channel_t servo_channel);

// =============================================================================
static inline void cncServo_start(void)
{
  LL_TIM_EnableCounter(TIM4);
}

static inline void cncServo_stop(void)
{
  LL_TIM_DisableCounter(TIM4);
}

#endif /* SERVOMOTOR_H_ */
// EOF =========================================================================
