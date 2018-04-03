#ifndef MPU9250_H_
#define MPU9250_H_

#include "cnc_ll_i2c.h"

// =============================================================================
typedef float float32_t;

typedef enum
{
  MPU9250_INTERFACE_I2C = 0, MPU9250_INTERFACE_SPI,
} mpu9250_Interface_t;

typedef enum
{
  MPU9250_ERROR = 0, MPU9250_OK,
} mpu9250_status_t;

typedef enum
{
  MPU9250_GYRO_FULLSCALE_250DPS = 0,
  MPU9250_GYRO_FULLSCALE_500DPS,
  MPU9250_GYRO_FULLSCALE_1000DPS,
  MPU9250_GYRO_FULLSCALE_2000DPS,
} mpu9250_Gyro_Scale_t;

typedef enum
{
  MPU9250_GYRO_X_ENABLE = 0x03,
  MPU9250_GYRO_Y_ENABLE = 0x05,
  MPU9250_GYRO_Z_ENABLE = 0x06,
  MPU9250_GYRO_XYZ_ENABLE = 0x00,
} mpu9250_Gyro_Axes_t;

typedef enum
{
  MPU9250_GYRO_LPF_DISABLE = -1,
  MPU9250_GYRO_LPF_250HZ = 0x00u,
  MPU9250_GYRO_LPF_184HZ,
  MPU9250_GYRO_LPF_92HZ,
  MPU9250_GYRO_LPF_41HZ,
  MPU9250_GYRO_LPF_20HZ,
  MPU9250_GYRO_LPF_10HZ,
  MPU9250_GYRO_LPF_5HZ,
  MPU9250_GYRO_LPF_3600HZ,
} mpu9250_Gyro_LowPassFilter_t;

typedef enum
{
  MPU9250_ACCEL_FULLSCALE_2G = 0,
  MPU9250_ACCEL_FULLSCALE_4G,
  MPU9250_ACCEL_FULLSCALE_8G,
  MPU9250_ACCEL_FULLSCALE_16G,
} mpu9250_Accel_Scale_t;

typedef enum
{
  MPU9250_ACCEL_X_ENABLE = 0x18,
  MPU9250_ACCEL_Y_ENABLE = 0x28,
  MPU9250_ACCEL_Z_ENABLE = 0x30,
  MPU9250_ACCEL_XYZ_ENABLE = 0x00,
} mpu9250_Accel_Axes_t;

typedef enum
{
  MPU9250_ACCEL_LPF_DISABLE = -1,
  MPU9250_ACCEL_LPF_218_1HZ = 0,
  MPU9250_ACCEL_LPF_99HZ = 2,
  MPU9250_ACCEL_LPF_44_8HZ,
  MPU9250_ACCEL_LPF_21_2HZ,
  MPU9250_ACCEL_LPF_10_2HZ,
  MPU9250_ACCEL_LPF_5_05HZ,
  MPU9250_ACCEL_LPF_420HZ,
} mpu9250_Accel_LowPassFilter_t;

// Structures ==================================================================
typedef struct
{
    mpu9250_Interface_t Interface;
    uint16_t SampleRate;
    mpu9250_Gyro_Axes_t Gyro_Axes;
    mpu9250_Gyro_Scale_t Gyro_Scale;
    mpu9250_Gyro_LowPassFilter_t Gyro_LPF;
    mpu9250_Accel_Axes_t Accel_Axes;
    mpu9250_Accel_Scale_t Accel_Scale;
    mpu9250_Accel_LowPassFilter_t Accel_LPF;
} mpu9250_InitStruct_t;

// Public constants ============================================================
static const uint8_t MPU9250_DEVICE_ID = 0x71;
static const uint8_t MPU9250_CMD_RESET = 0x80;
static const uint8_t MPU9250_INT_DATA_READY_MSK = 0x01;

// Function Prototypes =========================================================
/*******************************************************************************
 * Configure and init MPU9250 IMU (Accelerometer and Gyroscope)
 * ToDo:
 *      implement SPI protocol.
 ******************************************************************************/
mpu9250_status_t mpu9250_init(mpu9250_InitStruct_t* mpu9250_Init);

/*******************************************************************************
 * init interrupt pin only if internal sample rate == 1kHz
 ******************************************************************************/
mpu9250_status_t mpu9250_initInterrupt(uint16_t samplerate);

/*******************************************************************************
 * Reset device
 ******************************************************************************/
mpu9250_status_t mpu9250_reset(void);

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getBias_int16(
    uint8_t samples, int16_t *pAccel, int16_t *pGyro);
mpu9250_status_t mpu9250_getBias_float(
    uint8_t samples, float32_t *pAccel, float32_t *pGyro);

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getResolution_int16(int16_t *pResolution);
mpu9250_status_t mpu9250_getResolution_float(float32_t *pResolution);

/*******************************************************************************
 * Return device id --> MPU9250_DEVICE_ID == 0x71
 ******************************************************************************/
uint8_t mpu9250_readID(void);

/*******************************************************************************
 * Read device's temperature: int16_t --> raw data.
 * float32_t:
 *          temperature = [(rawData - RoomTemp_Offset)/ 333.87f] + 21.0f
 ******************************************************************************/
mpu9250_status_t mpu9250_readTemperature_int16(int16_t *pTemperature);
mpu9250_status_t mpu9250_readTemperature_float(float32_t *pTemperature);

/*******************************************************************************
 * Return Accelerometer data. Default 16.384 LSB/g
 ******************************************************************************/
mpu9250_status_t mpu9250_readAccelData_int16(int16_t *pAccel);
mpu9250_status_t mpu9250_readAccelData_float(float32_t *pAccel);

/*******************************************************************************
 * Return Gyroscope data. Default 131.0 LSB/dps
 ******************************************************************************/
mpu9250_status_t mpu9250_readGyroData_int16(int16_t *pGyro);
mpu9250_status_t mpu9250_readGyroData_float(float32_t *pGyro);

/*******************************************************************************
 * Return Accelerometer and Gyroscope data
 ******************************************************************************/
mpu9250_status_t mpu9250_readData_int16(int16_t *pAccel, int16_t *pGyro);
mpu9250_status_t mpu9250_readData_float(float32_t *pAccel, float32_t *pGyro);

#endif /* MPU9250_H_ */
// EOF =========================================================================
