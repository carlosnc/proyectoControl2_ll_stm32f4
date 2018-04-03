// =============================================================================
#include "estimador.h"
#include "arm_math.h"

// =============================================================================
volatile float32_t dt = 0.0f;
volatile float32_t weight = 0.0f;

volatile float32_t aPastGyroscope_Angle[3] = { 0.0f };
volatile float32_t *pPastGyroscope_Angle = &aPastGyroscope_Angle[0];

volatile float32_t aPastAccel_Angle[3] = { 0.0f };
volatile float32_t *pPastAccel_Angle = &aPastAccel_Angle[0];

volatile float32_t aPastEstimatedGyro[2]   = { 0.0f };
volatile float32_t aPastEstimatedAccel[2]  = { 0.0f };

// =============================================================================
static void eCalc_NormalizeMeasure(float32_t *pMeasure, uint8_t len);
static float32_t eCalc_FastInverse_Sqrt(float32_t x);
static void eCalc_Angles(float32_t *pAccelerometer, float32_t *pGyroscope, float32_t *pResult);
static void eCalc_ComplementaryFilter(float32_t *pGyroscope, float32_t *pFilteredAngles);

// =============================================================================
filter_status_t estimator_init(filter_init_t *filter_InitStruct)
{
  filter_status_t status = FILTER_ERROR;

      dt  = (float32_t)(1.0f/filter_InitStruct->SampleRate);
  weight  = filter_InitStruct->Weight;

  if(mpu9250_readID() == MPU9250_DEVICE_ID)
    status = FILTER_OK;

  return status;
}

filter_status_t estimator_notFilterAngles(float32_t *pAngles)
{
  filter_status_t status = FILTER_OK;

  float32_t aAccelerometer[3] = { 0.0f };
  float32_t aGyroscope[3]     = { 0.0f };

  float32_t *pAccelerometer   = &aAccelerometer[0];
  float32_t *pGyroscope       = &aGyroscope[0];

  if(mpu9250_readData_float(pAccelerometer, pGyroscope) != MPU9250_OK)
    status = FILTER_ERROR;

  eCalc_Angles(pAccelerometer, pGyroscope, pAngles);
  return status;
}

/*==============================================================================
* pAngles[0] = estimated Pitch angle.
* pAngles[1] = estimated Roll angle.
* pAngles[2] = Tilt Z angle.
*-------------------------------------------------------------------------------
*
==============================================================================*/
filter_status_t estimator_filteredAngles(float32_t *pAngles)
{
  filter_status_t status = FILTER_OK;

  float32_t aAccelerometer[3] = { 0.0f };
  float32_t aGyroscope[3]     = { 0.0f };
  float32_t aMeasures[6]      = { 0.0f };

  float32_t *pAccelerometer   = &aAccelerometer[0];
  float32_t *pGyroscope       = &aGyroscope[0];
  float32_t *pMeasures        = &aMeasures[0];

  if(mpu9250_readData_float(pAccelerometer, pGyroscope) != MPU9250_OK)
    status = FILTER_ERROR;

  eCalc_Angles(pAccelerometer, pGyroscope, pMeasures);
  eCalc_ComplementaryFilter(pGyroscope, pAngles);
  pAngles[2] = pMeasures[2];

  for(uint8_t i = 0; i < 2; i++)
    aPastAccel_Angle[i] = pMeasures[i];

  return status;
}

// =============================================================================
static void eCalc_NormalizeMeasure(float32_t *pMeasure, uint8_t len)
{
  float32_t tmp = 0.0f;
  float32_t normG = 0.0f;

  arm_power_f32(pMeasure, len, &tmp);
  normG = eCalc_FastInverse_Sqrt(tmp);

  for(uint8_t i = 0; i < len; i++)
    pMeasure[i] *= normG;
}

static float32_t eCalc_FastInverse_Sqrt(float32_t x)
{
  float32_t xhalf = 0.5f * x;
  int32_t i = *(int32_t *) &x;

  i = 0x5F3759DF - (i >> 1);    // Magic number!!! RTFpaper.
  x = *(float32_t *) &i;

  x = x * (1.5f - (xhalf * x * x));
  x = x * (1.5f - (xhalf * x * x));
  x = x * (1.5f - (xhalf * x * x));
  x = x * (1.5f - (xhalf * x * x));
  x = x * (1.5f - (xhalf * x * x));

  return x;
}

/*==============================================================================
pResult[0] = Pitch angle Accelerometer  |   pResult[3] = X axis angle gyroscope
pResult[1] = Roll angle Accelerometer   |   pResult[4] = X axis angle gyroscope
pResult[2] = Tilt z angle Accelerometer |   pResult[5] = Yaw angle gyroscope
==============================================================================*/
static void eCalc_Angles(float32_t *pAccelerometer, float32_t *pGyroscope, float32_t *pResult)
{
  float32_t tmp = 0.0f;

  eCalc_NormalizeMeasure(pAccelerometer, 3);

  tmp  = pAccelerometer[1]*pAccelerometer[1];
  tmp += pAccelerometer[2]*pAccelerometer[2];
  pResult[0] = atan2f(-pAccelerometer[0], sqrtf(tmp))*(180.0f/PI);

  tmp  = pAccelerometer[0]*pAccelerometer[0];
  tmp += pAccelerometer[2]*pAccelerometer[2];
  pResult[1] = atan2f(pAccelerometer[1], sqrtf(tmp))*(180.0f/PI);

  tmp  = pAccelerometer[0]*pAccelerometer[0];
  tmp += pAccelerometer[1]*pAccelerometer[1];
  pResult[2] = atan2f(sqrtf(tmp), pAccelerometer[2])*(180.0f/PI);

  for(uint8_t i = 0; i < 3; i++)
  {
    pResult[i + 3] = aPastGyroscope_Angle[i] + pGyroscope[i]*dt;
    aPastGyroscope_Angle[i] = pResult[i + 3];
  }
}

/*==============================================================================
pFilteredAngles[0] = filtered Pitch   |   pFilteredAngles[1] = filtered Roll
--------------------------------------------------------------------------------
aEstimatedGyro[i]  = weight*aPastEstimatedGyro[i] + (1 - weight)*pGyroscope[i]*dt
aEstimatedAccel[i] = weight*aPastEstimatedAccel[i] + (1 - weight)*aPastAccel_Angle[i]
==============================================================================*/
static void eCalc_ComplementaryFilter(float32_t *pGyroscope, float32_t *pFilteredAngles)
{
  float32_t aEstimatedAccel[2] = { 0.0f };
  float32_t aEstimatedGyro[2]  = { 0.0f };

  for(uint8_t i = 0; i < 2; i++)
  {
    aEstimatedAccel[i]  = weight*aPastEstimatedAccel[i];
    aEstimatedAccel[i] += (1 - weight)*aPastAccel_Angle[i];
    aPastEstimatedAccel[i] = aEstimatedAccel[i];
  }

  for(uint8_t i = 0; i < 2; i++)
  {
    aEstimatedGyro[i]  = weight*aPastEstimatedGyro[i];
    aEstimatedGyro[i] += (1 - weight)*pGyroscope[i]*dt;
    aPastEstimatedGyro[i] = aEstimatedGyro[i];
  }

  pFilteredAngles[0] = aEstimatedAccel[0] + aEstimatedGyro[1];
  pFilteredAngles[1] = aEstimatedAccel[1] + aEstimatedGyro[0];
}
