
#include "servomotor.h"

// =============================================================================
static servo_channel_t servo[3] = {SERVO_CHANNEL_1, SERVO_CHANNEL_2, SERVO_CHANNEL_3};

static volatile uint32_t posZero        = 0;
static volatile uint32_t servoStep      = 0;
static volatile int16_t  maxAngle       = 0;
static volatile int16_t  minAngle       = 0;

// =============================================================================
static uint32_t eServo_angle_to_pwm(float32_t angle);

// =============================================================================
void cncServo_init(servo_initStruct_t *servo_initStruct)
{
  maxAngle = servo_initStruct->Max_Angle;
  minAngle = servo_initStruct->Min_Angle;
  servoStep = 10;

  posZero = (uint32_t)(600 + servoStep*servo_initStruct->Pos_Zero);
}

void cncServo_check(servo_channel_t servo_channel)
{
  float32_t tmpAngle = 0.0f;
  uint16_t step = (maxAngle - minAngle)/4;

  if(servo_channel == SERVO_CHANNEL_ALL)
  {
    for(uint8_t i = 0; i < 3; i++)
    {
      tmpAngle = minAngle;

      for(uint8_t u = 0; u < 4; u++)
      {
        cncServo_updatePosition(tmpAngle, servo[i]);
        tmpAngle += step;
        LL_mDelay(250);
      }
    }
  }
  else
  {
    tmpAngle = minAngle;
    for (uint8_t u = 0; u < 4; u++)
    {
      cncServo_updatePosition(tmpAngle, servo_channel);
      tmpAngle += step;
      LL_mDelay(250);
    }
  }

  cncServo_zeroPosition(servo_channel);
}

void cncServo_zeroPosition(servo_channel_t servo_channel)
{
  float32_t servoPosition = 0.0f;
  cncServo_updatePosition(servoPosition, servo_channel);
}

void cncServo_updatePosition(float32_t angle, servo_channel_t servo_channel)
{
  uint32_t servoPosition = 0;
  servoPosition = eServo_angle_to_pwm(angle);

  switch (servo_channel)
  {
    case SERVO_CHANNEL_1:
      LL_TIM_OC_SetCompareCH1(TIM4, servoPosition);
      break;
    case SERVO_CHANNEL_2:
      LL_TIM_OC_SetCompareCH2(TIM4, servoPosition);
      break;
    case SERVO_CHANNEL_3:
      LL_TIM_OC_SetCompareCH3(TIM4, servoPosition);
      break;
    case SERVO_CHANNEL_ALL:
      LL_TIM_OC_SetCompareCH1(TIM4, servoPosition);
      LL_TIM_OC_SetCompareCH2(TIM4, servoPosition);
      LL_TIM_OC_SetCompareCH3(TIM4, servoPosition);
      break;
    default:
      break;
  }
}

// =============================================================================
static uint32_t eServo_angle_to_pwm(float32_t angle)
{
  float32_t int_degree = 0;
  uint32_t  pwm_pos    = 0;

  int_degree = (angle > maxAngle) ? (maxAngle) : (angle);
  int_degree = (angle < minAngle) ? (minAngle) : (angle);

  pwm_pos = (uint32_t)(posZero + (servoStep * int_degree));

  return pwm_pos;
}
