#include "mpu9250.h"

#pragma GCC diagnostic ignored "-Wunused-const-variable";

/*==============================================================================
 *                              MPU9250 Address
 *============================================================================*/
static const uint8_t MPU9250_ADDR = 0x68;
static const uint8_t MPU9250_ADDR_ALT = 0x69;

/*==============================================================================
 *                           START REGISTER MAPPING
 *============================================================================*/
/*******************************************************************************
 * Gyroscope Self-test registers - RW - Reset value: 0x00
 * The value in this registers indicates the self test output generated during
 * manufacturing test. This value is to be used to check against subsequent
 * self test outputs performed by the end user.
 * [7:0] - G_ST_DATA
 ******************************************************************************/
static const uint8_t MPU9250_SELF_TEST_X_GYRO_ADDR = 0x00;
static const uint8_t MPU9250_SELF_TEST_Y_GYRO_ADDR = 0x01;
static const uint8_t MPU9250_SELF_TEST_Z_GYRO_ADDR = 0x02;

/*******************************************************************************
 * Accelerometer Self-test registers - RW - Reset value: 0x00
 * The value in this registers indicates the self test output generated during
 * manufacturing test. This value is to be used to check against subsequent
 * self test outputs performed by the end user.
 * [7:0] - A_ST_DATA
 ******************************************************************************/
static const uint8_t MPU9250_SELF_TEST_X_ACCEL_ADDR = 0x0D;
static const uint8_t MPU9250_SELF_TEST_Y_ACCEL_ADDR = 0x0E;
static const uint8_t MPU9250_SELF_TEST_Z_ACCEL_ADDR = 0x0F;

/*******************************************************************************
 * Gyroscope offset registers - RW - Reset value: 0x00
 * These registers are used to remove DC bias from the gyro sensor data output.
 * The values in these registers are subtracted from the gyro sensor values
 * before going into the sensor registers.
 *
 *                      OffsetLSB = (OFFS_USR*4)/(2^FS_SEL)
 *                      OffsetDPS = (OffsetLSB)/Gyro_Sensitivity
 *
 * Nominal:     FS_SEL = 0
 * [Min, Max]:  [-1000dps, 999.969dps]
 * Step:        0.0305dps
 ******************************************************************************/
static const uint8_t MPU9250_XG_OFFSET_H_ADDR = 0x13;
static const uint8_t MPU9250_XG_OFFSET_L_ADDR = 0x14;
static const uint8_t MPU9250_YG_OFFSET_H_ADDR = 0x15;
static const uint8_t MPU9250_YG_OFFSET_L_ADDR = 0x16;
static const uint8_t MPU9250_ZG_OFFSET_H_ADDR = 0x17;
static const uint8_t MPU9250_ZG_OFFSET_L_ADDR = 0x18;

/*******************************************************************************
 * Sample rate divider register - RW - Reset value: 0x00
 * Data should be sampled at or above sample rate; SMPLRT_DIV is only used
 * for 1kHz internal sampling.
 *
 * [7:0] - Divides the internal sample rate to generate the sample rate that
 * controls sensor data output rate, FIFO sample rate.
 * NOTE: This register is only effective when fchoice_b register bits are 0b00,
 * and (0 < dlpf_cfg < 7), such that the average filter’s output is selected.
 *
 *           SAMPLE_RATE = Internal_Sample_Rate / (1 + SMPLRT_DIV)
 ******************************************************************************/
static const uint8_t MPU9250_SAMPLE_RATE_DIV_ADDR = 0x19;

/*******************************************************************************
 * Configuration register - RW - Reset value: 0x00
 *
 * [7] - Reserved!
 * [6] - FIFO_Mode: 1 - When the FIFO is full, additional writes will not be
 *                      written to FIFO.
 *                  0 - When the FIFO is full, additional writes will be
 *                      written to the FIFO, replacing the oldest data.
 * [5:3] - EXT_SYNC_SET: Enables the FSYNC pin data to be sampled.
 *
 *                      ------------------------------------
 *                      | EXT_SYN_SET | FSYNC bit location |
 *                  ----|-------------|--------------------|
 *                  | 0 |  0 | 0 | 0  | Function disabled  |
 *                  | 1 |  0 | 0 | 1  | TEMP_OUT_L[0]      |
 *                  | 2 |  0 | 1 | 0  | GYRO_X_OUT_L[0]    |
 *                  | 3 |  0 | 1 | 1  | GYRO_Y_OUT_L[0]    |
 *                  | 4 |  1 | 0 | 0  | GYRO_Z_OUT_L[0]    |
 *                  | 5 |  1 | 0 | 1  | ACCEL_X_OUT_L[0]   |
 *                  | 6 |  1 | 1 | 0  | ACCEL_Y_OUT_L[0]   |
 *                  | 7 |  1 | 1 | 1  | ACCEL_Z_OUT_L[0]   |
 *                  ----------------------------------------
 * [2:0] - DLPF_CFG: The gyroscope and temperature sensor are filtered
 *                   according to the value of DLPF_CFG and FCHOICE_B.
 *                   BW[Hz] - Delay[ms] - Fs[kHz]
 *
 *                                 ------------------------------------
 *                                 |     Gyroscope     |  Temperature |
 *          -----------------------------------------------------------
 *          | FCHOICE_B | DLPF_CFG |  BW  | Delay | Fs |  BW  | Delay |
 *          -----------------------------------------------------------
 *          |   x | 1   |    x     | 8800 | 0.064 | 32 | 4000 |  0.04 |
 *          |   1 | 0   |    x     | 3600 | 0.11  | 32 | 4000 |  0.04 |
 *          |   0 | 0   |    0     | 250  | 0.97  | 8  | 4000 |  0.04 |
 *          |   0 | 0   |    1     | 184  | 2.9   | 1  | 188  |  1.9  |
 *          |   0 | 0   |    2     | 92   | 3.9   | 1  | 98   |  2.8  |
 *          |   0 | 0   |    3     | 41   | 5.9   | 1  | 42   |  4.8  |
 *          |   0 | 0   |    4     | 20   | 9.9   | 1  | 20   |  8.3  |
 *          |   0 | 0   |    5     | 10   | 17.85 | 1  | 10   |  13.4 |
 *          |   0 | 0   |    6     | 5    | 33.48 | 1  | 5    |  18.6 |
 *          |   0 | 0   |    7     | 3600 | 0.17  | 8  | 4000 |  0.04 |
 *          -----------------------------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_CONFIG_ADDR = 0x1A;

/*******************************************************************************
 * Gyroscope configuration register - RW - Reset value: 0x00
 * [7] - XGYRO_ST_EN: X Gyro self-test enable.
 * [6] - YGYRO_ST_EN: Y Gyro self-test enable.
 * [5] - ZGYRO_ST_EN: Z Gyro self-test enable.
 * [4:3] - GYRO_FS_SEL: Gyroscope full scale select:
 *
 *                      ----------------------------
 *                      | GYRO_FS_SEL | Full Scale |
 *                  ----|-------------|------------|
 *                  | 0 |    0 | 0    |   250dps   |
 *                  | 1 |    0 | 1    |   500dps   |
 *                  | 2 |    1 | 0    |   1000dps  |
 *                  | 3 |    1 | 1    |   2000dps  |
 *                  --------------------------------
 *
 * [2] - Reserved!
 * [1:0] - FCHOISE_B: Used to bypass DLPF as shown in MPU9250_CONFIG register
 *                    above.
 ******************************************************************************/
static const uint8_t MPU9250_GYRO_CONFIG_ADDR = 0x1B;

/*******************************************************************************
 * Accelerometer configuration register - RW - Reset value: 0x00
 * [7] - AX_ST_EN: X Accel self-test enable
 * [6] - AY_ST_EN: Y Accel self-test enable
 * [5] - AZ_ST_EN: Z Accel self-test enable
 * [4:3] - ACCEL_FS_SEL: Accelerometer full scale select:
 *
 *                      -----------------------------
 *                      | ACCEL_FS_SEL | Full Scale |
 *                  ----|--------------|------------|
 *                  | 0 |     0 | 0    |     2g     |
 *                  | 1 |     0 | 1    |     4g     |
 *                  | 2 |     1 | 0    |     8g     |
 *                  | 3 |     1 | 1    |     16g    |
 *                  ---------------------------------
 * [2:0] - Reserved!
 ******************************************************************************/
static const uint8_t MPU9250_ACCEL_CONFIG_ADDR = 0x1C;

/*******************************************************************************
 * Accelerometer configuration 2 register - RW - Reset value: 0x00
 * [7:4] - Reserved!
 * [3] - ACCEL_FCHOISE_B:  Used to bypass DLPF.
 * [2:0] - ACCEL_DLPF_CFG: Accelerometer low pass filter setting.
 *                         3dB BW[Hz] - Rate[kHz] - Delay[ms]
 *          ---------------------------------------
 *          | ACCEL     | ACCEL    |____Output____|-----------------
 *          | FCHOISE_B | DLPF_CFG |  3dB  | Rate | Filter | Delay |
 *          |-----------|----------|-------|------|--------|-------|
 *          |     1     |    x     | 1046  |   4  |  Dec1  | 0.503 |
 *          |     0     |    0     | 218.1 |   1  |  DLPF  | 1.88  |
 *          |     0     |    1     | 218.1 |   1  |  DLPF  | 1.88  |
 *          |     0     |    2     | 99    |   1  |  DLPF  | 2.88  |
 *          |     0     |    3     | 44.8  |   1  |  DLPF  | 4.88  |
 *          |     0     |    4     | 21.2  |   1  |  DLPF  | 8.87  |
 *          |     0     |    5     | 10.2  |   1  |  DLPF  | 16.83 |
 *          |     0     |    6     | 5.05  |   1  |  DLPF  | 32.48 |
 *          |     0     |    7     | 420   |   1  |  Dec2  | 1.32  |
 *          --------------------------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_ACCEL_CONFIG2_ADDR = 0x1D;

/*******************************************************************************
 * Low power accelerometer Output Data Rate register - RW - Reset value: 0x00
 * [7:4] - Reserved!
 * [3:0] - LPOSC_CLK_SEL: Sets the frequency of waking up the chip to take a
 *                        sample of accel data – the low power accel Output Data
 *                        Rate.
 *                   ------------------------------------
 *                   | LPOSC_CLK_SEL | Output Freq [Hz] |
 *              -----|---------------|------------------|
 *              |  0 | 0 | 0 | 0 | 0 |      0.24        |
 *              |  1 | 0 | 0 | 0 | 1 |      0.49        |
 *              |  2 | 0 | 0 | 1 | 0 |      0.98        |
 *              |  3 | 0 | 0 | 1 | 1 |      1.95        |
 *              |  4 | 0 | 1 | 0 | 0 |      3.91        |
 *              |  5 | 0 | 1 | 0 | 1 |      7.81        |
 *              |  6 | 0 | 1 | 1 | 0 |      15.63       |
 *              |  7 | 0 | 1 | 1 | 1 |      31.25       |
 *              |  8 | 1 | 0 | 0 | 0 |      62.50       |
 *              |  9 | 1 | 0 | 0 | 1 |      125         |
 *              | 10 | 1 | 0 | 1 | 0 |      200         |
 *              | 11 | 1 | 0 | 1 | 1 |      500         |
 *              -----------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_LP_ACCCEL_ODR_ADDR = 0x1E;

/*******************************************************************************
 * Wake-on Motion Threshold Register - RW - Reset Value 0x00
 *  [7:0] - WOM_THRESHOLD: This register holds the threshold value for the Wake
 *                         on Motion Interrupt for accel x/y/z axes. LSB = 4mg.
 *                         Range is 0mg to 1020mg.
 ******************************************************************************/
static const uint8_t MPU9250_WON_THR_ADDR = 0x1F;

/*******************************************************************************
 * FIFO Enable Register - RW - Reset Value 0x00
 * [7] - TEMP_OUT:  1 - Write TEMP_OUT_H and TEMP_OUT_L to the FIFO at the
 *                      sample rate; If enabled, buffering of data occurs even
 *                      if data path is in standby.
 *                  0 - Function is disabled.
 *
 * [6] - GYRO_XOUT: 1 - Write GYRO_XOUT_H and GYRO_XOUT_L to the FIFO at the
 *                      sample rate; If enabled, buffering of data occurs even
 *                      if data path is in standby.
 *                  0 - Function is disabled.
 * [5] - GYRO_YOUT: 1 - Write GYRO_YOUT_H and GYRO_YOUT_L to the FIFO at the
 *                      sample rate; If enabled, buffering of data occurs even
 *                      if data path is in standby.
 *                  0 - Function is disabled.
 * [4] - GYRO_ZOUT: 1 - Write GYRO_ZOUT_H and GYRO_ZOUT_L to the FIFO at the
 *                      sample rate; If enabled, buffering of data occurs even
 *                      if data path is in standby.
 *                  0 - Function is disabled.
 * [3] - ACCEL:     1 - Write ACCEL_XOUT_H, ACCEL_XOUT_L, ACCEL_YOUT_H,
 *                      ACCEL_YOUT_L, ACCEL_ZOUT_H, and ACCEL_ZOUT_L to the FIFO
 *                      at the sample rate.
 *                  0 - Function is disabled.
 *
 * [2] - SLV2:      1 – Write EXT_SENS_DATA registers associated to SLV_2 to the
 *                      FIFO at the sample rate.
 *                  0 – function is disabled.
 *
 * [1] - SLV1:      1 – Write EXT_SENS_DATA registers associated to SLV_1 to the
 *                      FIFO at the sample rate.
 *                  0 – function is disabled.
 * [0] - SLV0:      1 – Write EXT_SENS_DATA registers associated to SLV_0 to the
 *                      FIFO at the sample rate.
 *                  0 – function is disabled.
 ******************************************************************************/
static const uint8_t MPU9250_FIFO_EN_ADDR = 0x23;

/*******************************************************************************
 * I2C Master Control Register - RW - Reset Value 0x00
 * [7] - MULT_MST_EN: Enables multi-master capability. When disabled, clocking
 *                    to the I2C_MST_IF can be disabled when not in use and the
 *                    logic to detect lost arbitration is disabled.
 *
 * [6] - WAIT_FOR_ES: Delays the data ready interrupt until external sensor data
 *                    is loaded. If I2C_MST_IF is disabled, the interrupt will
 *                    still occur.
 *
 * [5] - SLV3_FIFO_EN: 1 – Write EXT_SENS_DATA registers associated to SLV_3
 *                         to the FIFO at the sample rate.
 *                     0 – Function is disabled.
 *
 * [4] - I2C_MST_P_NSR: This bit controls the I2C Master’s transition from one
 *                      slave read to the next slave read. If 0, there is a
 *                      restart between reads. If 1, there is a stop between
 *                      reads.
 *
 * [3:0] - I2C_MST_CLK: configures a divider on the MPU-9250 internal 8MHz clock
 *                      It sets the I C master clock speed.
 *
 *              ----------------------------------------------
 *              | I2C_MST_CLK | I2C Speed | 8MHz Clk Divider |
 *              |-------------|-----------|------------------|
 *              |       0     |  348 kHz  |         23       |
 *              |       1     |  333 kHz  |         24       |
 *              |       2     |  320 kHz  |         25       |
 *              |       3     |  308 kHz  |         26       |
 *              |       4     |  296 kHz  |         27       |
 *              |       5     |  286 kHz  |         28       |
 *              |       6     |  276 kHz  |         29       |
 *              |       7     |  267 kHz  |         30       |
 *              |       8     |  258 kHz  |         31       |
 *              |       9     |  500 kHz  |         16       |
 *              |       10    |  471 kHz  |         17       |
 *              |       11    |  444 kHz  |         18       |
 *              |       12    |  421 kHz  |         19       |
 *              |       13    |  400 kHz  |         20       |
 *              |       14    |  381 kHz  |         21       |
 *              |       15    |  364 kHz  |         22       |
 *              ----------------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_I2C_MST_CTRL_ADDR = 0x24;

/*******************************************************************************
 * I2C Slave 0 Address - RW - Reset Value 0x00
 * [7] - I2C_SLV0_RNW: 1 – Transfer is a read
 *                     0 – Transfer is a write
 * [6:0] - I2C_ID_0:   Physical address of I2C slave 0
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV0_ADDRESS_ADR = 0x25;
/*******************************************************************************
 * I2C Slave 0 Register - RW - Reset Value 0x00
 * [7:0] - I2C_SLV0_REG: I2C slave 0 register address from where to begin
 *                       data transfer.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV0_REG_ADDR = 0x26;
/*******************************************************************************
 * I2C Slave 0 Control - RW - Reset Value 0x00
 * [7] - I2C_SLV0_EN: 1 – Enable reading data from this slave at the sample rate
 *                        and storing data at the first available EXT_SENS_DATA
 *                        register, which is always EXT_SENS_DATA_00 for I2C
 *                        slave 0.
 *                    0 – function is disabled for this slave.
 *
 * [6] - I2C_SLV0_BYTE_SW: 1 – Swap bytes when reading both the low and high
 *                             byte of a word. Note there is nothing to swap
 *                             after reading the first byte if I2C_SLV0_REG[0]
 *                             = 1, or if the last byte read has a register
 *                             address LSB = 0.
 *                         0 – no swapping occurs, bytes are written in order
 *                             read.
 *
 * For example, if I2C_SLV0_REG = 0x1, and I2C_SLV0_LENG = 0x4:
 *  1) The first byte read from address 0x1 will be stored at EXT_SENS_DATA_00,
 *  2) The second and third bytes will be read and swapped, so the data read
 *     from address 0x2 will be stored at EXT_SENS_DATA_02 and the data read
 *     from address 0x3 will be stored at EXT_SENS_DATA_01,
 *  3) The last byte read from address 0x4 will be stored at EXT_SENS_DATA_03.
 *
 * [5] - I2C_SLV0_REG_DIS: When set, the transaction does not write a register
 *                         value, it will only read data, or write data.
 *
 * [4] - I2C_SLV0_GRP: 0 indicates slave register addresses 0 and 1 are grouped
 *                     together (odd numbered register ends the group).
 *                     1 indicates slave register addresses 1 and 2 are grouped
 *                     together (even numbered register ends the group). This
 *                     allows byte swapping of registers that are grouped
 *                     starting at any address.
 *
 * [3:0] - I2C_SLV0_LEN: Number of bytes to be read from I2C slave 0
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV0_CTRL_ADDR = 0x27;

// Same I2C Slave 0
static const uint8_t MPU9250_I2C_SLV1_ADDRESS_ADDR = 0x28;
static const uint8_t MPU9250_I2C_SLV1_REG_ADDR = 0x29;
static const uint8_t MPU9250_I2C_SLV1_CTRL_ADDR = 0x2A;

static const uint8_t MPU9250_I2C_SLV2_ADDRESS_ADDR = 0x28;
static const uint8_t MPU9250_I2C_SLV2_REG_ADDR = 0x29;
static const uint8_t MPU9250_I2C_SLV2_CTRL_ADDR = 0x2A;

static const uint8_t MPU9250_I2C_SLV3_ADDRESS_ADDR = 0x28;
static const uint8_t MPU9250_I2C_SLV3_REG_ADDR = 0x29;
static const uint8_t MPU9250_I2C_SLV3_CTRL_ADDR = 0x2A;

/*******************************************************************************
 * I2C Slave 4 Address - RW - Reset Value 0x00
 * [7] - I2C_SLV4_RNW: 1 – Transfer is a read
 *                     0 – Transfer is a write
 * [6:0] - I2C_ID_4:   Physical address of I2C slave 4
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV4_ADDRESS_ADDR = 0x31;

/*******************************************************************************
 * I2C Slave 4 Register - RW - Reset Value 0x00
 * [7:0] - I2C_SLV4_REG: I2C slave 4 register address from where to begin
 *                       data transfer.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV4_REG_ADDR = 0x32;

/*******************************************************************************
 * I2C Slave 4 Data Output Register - RW - Reset Value 0x00
 * [7:0] - I1C_SLV4_DO: Data to be written to I2C Slave 4 if enabled.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV4_DO_ADDR = 0x33;

/*******************************************************************************
 * I2C Slave 4 Control Register - RW - Reset Value 0x00
 * [7] - I2C_SLV4_EN: 1 – Enable data transfer with this slave at the sample
 *                        rate. If read command, store data in I2C_SLV4_DI
 *                        register, if write command, write data stored in
 *                        I2C_SLV4_DO register. Bit is cleared when a single
 *                        transfer is complete. Be sure to write I2C_SLV4_DO
 *                        first.
 *                    0 – function is disabled for this slave.
 *
 * [6] - SLV4_DONE_INT_EN: 1 – Enables the completion of the I2C slave 4 data
 *                             transfer to cause an interrupt.
 *                         0 – Completion of the I2C slave 4 data transfer will
 *                             not cause an interrupt.
 *
 * [5] - I2C_SLV4_REG_DIS: When set, the transaction does not write a register
 *                         value, it will only read data, or write data.
 *
 * [4:0] - I2C_MST_DLY: When enabled via the I2C_MST_DELAY_CTRL, those slaves
 *                      will only be enabled every (1+I2C_MST_DLY) samples
 *                      as determined by the SMPLRT_DIV and DLPF_CFG registers.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV4_CTRL_ADDR = 0x34;

/*******************************************************************************
 * I2C Slave 4 Data Input Register - Read Only - Reset Value 0x00
 * [7:0] - I1C_SLV4_DI: Data read from I2C Slave 4 if enabled.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV4_DI_ADDR = 0x35;

/*******************************************************************************
 * I2C Master status Registers - Read Only - Reset Value 0x00
 * [7] - PASS_THROUGH: Status of FSYNC interrupt – used as a way to pass an
 *                     external interrupt through this chip to the host. If
 *                     enabled in the INT_PIN_CFG register by asserting bit
 *                     FSYNC_INT_EN and if the FSYNC signal transitions from low
 *                     to high, this will cause an interrupt. A read of this
 *                     register clears all status bits in this register.
 * [6] - I2C_SLV4_DONE: Asserted when I2C slave 4’s transfer is complete, will
 *                      cause an interrupt if bit I2C_MST_INT_EN in the
 *                      INT_ENABLE register is asserted, and if the
 *                      SLV4_DONE_INT_EN bit is asserted in the I2C_SLV4_CTRL
 *                      register.
 * [5] - I2C_LOST_ARB: Asserted when I2C slave looses arbitration of the I2C
 *                     bus, will cause an interrupt if bit I2C_MST_INT_EN in the
 *                     INT_ENABLE register is asserted.
 * [4] - I2C_SLV4_NACK: Asserted when slave 4 receives a NACK, will cause an
 *                      interrupt if bit I2C_MST_INT_EN in the INT_ENABLE
 *                      register is asserted.
 * [3] - I2C_SLV3_NACK: Asserted when slave 3 receives a NACK, will cause an
 *                      interrupt if bit I2C_MST_INT_EN in the INT_ENABLE
 *                      register is asserted.
 * [2] - I2C_SLV2_NACK: Asserted when slave 2 receives a NACK, will cause an
 *                      interrupt if bit I2C_MST_INT_EN in the INT_ENABLE
 *                      register is asserted.
 * [1] - I2C_SLV1_NACK: Asserted when slave 1 receives a NACK, will cause an
 *                      interrupt if bit I2C_MST_INT_EN in the INT_ENABLE
 *                      register is asserted.
 * [0] - I2C_SLV0_NACK: Asserted when slave 0 receives a NACK, will cause an
 *                      interrupt if bit I2C_MST_INT_EN in the INT_ENABLE
 *                      register is asserted.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_MST_STATUS_ADDR = 0x36;

/*******************************************************************************
 * INT Pin/Bypass Enable Configuration Register - RW - Reset value 0x00
 * [7] - ACTL: 1 - The logic level of INT pin is active low.
 *             0 - The logic level of INT pin is active high.
 *
 * [6] - OPEN: 1 - INT pin is configured as open drain.
 *             0 - INT pin is configured as push-pull.
 *
 * [5] - LATCH_INT_EN: 1 - INT pin level held until interrupt status is cleared.
 *                     0 - INT pin indicates interrupt pulse’s is width 50us.
 *
 * [4] - INT_ANYRD_2CLEAR: 1 - Interrupt status is cleared if any read operation
 *                             is performed.
 *                         0 - Interrupt status is cleared only by reading
 *                             INT_STATUS register
 *
 * [3] - ACTL_FSYNC: 1 - The logic level for the FSYNC pin as an interrupt is
 *                       active low.
 *                   0 - The logic level for the FSYNC pin as an interrupt is
 *                       active high.
 *
 * [2] - FSYNC_INT_MODE_EN: 1 – This enables the FSYNC pin to be used as an
 *                              interrupt. A transition to the active level
 *                              described by the ACTL_FSYNC bit will cause an
 *                              interrupt. The status of the interrupt is read
 *                              in the I2C Master Status register PASS_THROUGH
 *                              bit.
 *                          0 - This disables the FSYNC pin from causing an
 *                              interrupt.
 *
 * [1] - BYPASS_EN: When asserted, the i2c_master interface pins(ES_CL & ES_DA)
 *                  will go into ‘bypass mode’ when the i2c master interface is
 *                  disabled. The pins will float high due to the internal
 *                  pull-up if not enabled and the i2c master interface is
 *                  disabled.
 *
 * [0] -  Reserved!
 ******************************************************************************/
static const uint8_t MPU9250_INT_PIN_CONFIG_ADDR = 0x37;

/*******************************************************************************
 * Interrupt Enable Register - RW - Reset value: 0x00
 * [7] - Reserved!
 * [6] - WOM_EN: 1 - Enable interrupt for wake on motion to propagate to
 *                   interrupt pin.
 *               0 - Function is disabled.
 * [5] - Reserved!
 * [4] - FIFO_OVERFLOW_EN: 1 - Enable interrupt for fifo overflow to propagate
 *                             to interrupt pin.
 *                         0 - Function is disabled.
 *
 * [3] - FSYNC_INT_EN: 1 - Enable Fsync interrupt to propagate to interrupt pin.
 *                     0 - Function is disabled.
 * [2] - Reserved!
 * [1] - Reserved!
 * [0] - RAW_RDY_EN: 1 – Enable Raw Sensor Data Ready interrupt to propagate to
 *                       interrupt pin. The timing of the interrupt can vary
 *                       depending on the setting in register I2C_MST_CTRL,
 *                       bit [6] WAIT_FOR_ES.
 *                   0 - Function is disabled.
 ******************************************************************************/
static const uint8_t MPU9250_INT_ENABLE_ADDR = 0x38;

/*******************************************************************************
 * Interrupt Status Register - Read Only - Reset value: 0x00
 * [7] - Reserved!
 * [6] - WOM_EN: 1 - Wake on motion interrupt occurred.
 *
 * [5] - Reserved!
 * [4] - FIFO_OVERFLOW_EN: 1 - Fifo Overflow interrupt occurred. Note that the
 *                             oldest data is has been dropped from the fifo.
 *
 * [3] - FSYNC_INT_EN: 1 - Fsync interrupt occurred.
 * [2] - Reserved!
 * [1] - Reserved!
 * [0] - RAW_RDY_EN: 1 – Sensor Register Raw Data sensors are updated and Ready
 *                       to be read. The timing of the interrupt can vary
 *                       depending on the setting in register 36 I2C_MST_CTRL,
 *                       bit [6] WAIT_FOR_ES.
 ******************************************************************************/
static const uint8_t MPU9250_INT_STATUS_ADDR = 0x3A;

/*******************************************************************************
 * Accelerometer Output Registers
 * H - High byte of accelerometer data
 * L - Low byte of accelerometer data
 ******************************************************************************/
static const uint8_t MPU9250_ACCEL_XOUT_H_ADDR = 0x3B;
static const uint8_t MPU9250_ACCEL_XOUT_L_ADDR = 0x3C;
static const uint8_t MPU9250_ACCEL_YOUT_H_ADDR = 0x3D;
static const uint8_t MPU9250_ACCEL_YOUT_L_ADDR = 0x3E;
static const uint8_t MPU9250_ACCEL_ZOUT_H_ADDR = 0x3F;
static const uint8_t MPU9250_ACCEL_ZOUT_L_ADDR = 0x40;

/*******************************************************************************
 * Temperature Sensor Output Registers
 * H - High byte of temperature sensor data
 * L - Low byte of temperature sensor data
 *
 * Temp_Sensitivity = 333.87 LSB/°C
 *  ----------------------------------------------------------------------
 *  | TEMP °C = ((TEMP_OUT – RoomTemp_Offset)/Temp_Sensitivity) + 21 °C  |
 *  ----------------------------------------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_TEMP_H_ADDR = 0x41;
static const uint8_t MPU9250_TEMP_L_ADDR = 0x42;

/*******************************************************************************
 * Gyroscope Output Registers
 * H - High byte of gyroscope data
 * L - Low byte of gyroscope data
 *
 *          -------------------------------------------------
 *          | GYRO_XOUT = Gyro_Sensitivity * X_angular_rate |
 *          -------------------------------------------------
 *
 ******************************************************************************/
static const uint8_t MPU9250_GYRO_XOUT_H_ADDR = 0x43;
static const uint8_t MPU9250_GYRO_XOUT_L_ADDR = 0x44;
static const uint8_t MPU9250_GYRO_YOUT_H_ADDR = 0x45;
static const uint8_t MPU9250_GYRO_YOUT_L_ADDR = 0x46;
static const uint8_t MPU9250_GYRO_ZOUT_H_ADDR = 0x47;
static const uint8_t MPU9250_GYRO_ZOUT_L_ADDR = 0x48;

/*******************************************************************************
 * External Sensor Data
 * [7:0] - DATA: Sensor data read from external I2C devices via the I2C master
 *               interface.
 ******************************************************************************/
static const uint8_t MPU9250_EXT_SENS_DATA_00_ADDR = 0x49;
static const uint8_t MPU9250_EXT_SENS_DATA_01_ADDR = 0x4A;
static const uint8_t MPU9250_EXT_SENS_DATA_02_ADDR = 0x4B;
static const uint8_t MPU9250_EXT_SENS_DATA_03_ADDR = 0x4C;
static const uint8_t MPU9250_EXT_SENS_DATA_04_ADDR = 0x4D;
static const uint8_t MPU9250_EXT_SENS_DATA_05_ADDR = 0x4E;
static const uint8_t MPU9250_EXT_SENS_DATA_06_ADDR = 0x4F;
static const uint8_t MPU9250_EXT_SENS_DATA_07_ADDR = 0x50;
static const uint8_t MPU9250_EXT_SENS_DATA_08_ADDR = 0x51;
static const uint8_t MPU9250_EXT_SENS_DATA_09_ADDR = 0x52;
static const uint8_t MPU9250_EXT_SENS_DATA_10_ADDR = 0x53;
static const uint8_t MPU9250_EXT_SENS_DATA_11_ADDR = 0x54;
static const uint8_t MPU9250_EXT_SENS_DATA_12_ADDR = 0x55;
static const uint8_t MPU9250_EXT_SENS_DATA_13_ADDR = 0x56;
static const uint8_t MPU9250_EXT_SENS_DATA_14_ADDR = 0x57;
static const uint8_t MPU9250_EXT_SENS_DATA_15_ADDR = 0x58;
static const uint8_t MPU9250_EXT_SENS_DATA_16_ADDR = 0x59;
static const uint8_t MPU9250_EXT_SENS_DATA_17_ADDR = 0x5A;
static const uint8_t MPU9250_EXT_SENS_DATA_18_ADDR = 0x5B;
static const uint8_t MPU9250_EXT_SENS_DATA_19_ADDR = 0x5C;
static const uint8_t MPU9250_EXT_SENS_DATA_20_ADDR = 0x5D;
static const uint8_t MPU9250_EXT_SENS_DATA_21_ADDR = 0x5E;
static const uint8_t MPU9250_EXT_SENS_DATA_22_ADDR = 0x5F;
static const uint8_t MPU9250_EXT_SENS_DATA_23_ADDR = 0x60;

/*******************************************************************************
 * I2C Slave Data Output - RW
 * [7:0] - DO: Data out when slave is set to write.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_SLV0_DO_ADDR = 0x63;
static const uint8_t MPU9250_I2C_SLV1_DO_ADDR = 0x64;
static const uint8_t MPU9250_I2C_SLV2_DO_ADDR = 0x65;
static const uint8_t MPU9250_I2C_SLV3_DO_ADDR = 0x66;

/*******************************************************************************
 * I2C Master Delay Control Register - RW - Reset Value 0x00
 * [7] - DELAY_ES_SHADOW: Delays shadowing of external sensor data until all
 *                        data is received.
 * [6:5] - Reserved!
 * [4] - I2C_SLV4_DLY_EN: When enabled, slave 4 will only be accessed
 *                        (1+I2C_MST_DLY) samples as determined by SMPLRT_DIV
 *                        and DLPF_CFG. *
 * [3] - I2C_SLV3_DLY_EN: Idem.
 * [2] - I2C_SLV2_DLY_EN: Idem.
 * [1] - I2C_SLV1_DLY_EN: Idem.
 * [0] - I2C_SLV0_DLY_EN: Idem.
 ******************************************************************************/
static const uint8_t MPU9250_I2C_MST_DELAY_CTRL_ADDR = 0x67;

/*******************************************************************************
 * Signal Path Reset Register - RW - Reset Value 0x00
 * [7:3] - Reserved!
 * [2] - GYRO_RST: Reset gyroscope digital signal path.
 * [1] - ACCEL_RST:Reset accelerometer digital signal path.
 * [0] - TEMP_RST:Reset temperature digital signal path.
 *
 * Note: Sensor registers are not cleared. Use SIG_COND_RST to clear sensor
 *       registers.
 ******************************************************************************/
static const uint8_t MPU9250_SIGNAL_PATH_RESET_ADDR = 0x68;

/*******************************************************************************
 * Accelerometer Interrupt Control Register - RW - Reset Value 0x00
 * [7] - ACCEL_INTEL_EN: This bit enables the Wake-on-Motion detection logic.
 * [6] - ACCEL_INTEL_MODE: 1 - Compare the current sample with the previous
 *                             sample.
 *                         0 - Not used.
 * [5:0] - Reserved!
 ******************************************************************************/
static const uint8_t MPU9250_ACCEL_INTEL_CTRL_ADDR = 0x69;

/*******************************************************************************
 * User Control Register - RW - Reset Value 0x00
 * [7] - Reserved!
 * [6] - FIFO_EN: 1 – Enable FIFO operation mode.
 *                0 – Disable FIFO access from serial interface. To disable FIFO
 *                    writes by DMA, use FIFO_EN register. To disable possible
 *                    FIFO writes from DMP, disable the DMP.
 *
 * [5] - I2C_MST_EN: 1 – Enable the I2C Master I/F module; pins ES_DA and ES_SCL
 *                       are isolated from pins SDA/SDI and SCL/ SCLK.
 *                   0 – Disable I2C Master I/F module; pins ES_DA and ES_SCL
 *                       are logically driven by pins SDA/SDI and SCL/ SCLK.
 *                   NOTE: DMP will run when enabled, even if all internal
 *                   sensors are disabled, except when the sample rate is set
 *                   to 8Khz.
 *
 * [4] - I2C_IF_DIS: 1 – Disable I2C Slave module and put the serial interface
 *                       in SPI mode only.
 * [3] - Reserved!
 * [2] - FIFO_RST: 1 – Reset FIFO module. Reset is asynchronous. This bit auto
 *                     clears after one clock cycle.
 * [1] - I2C_MST_RST: 1 – Reset I2C Master module. Reset is asynchronous. This
 *                        bit auto clears after one clock cycle.
 *                    NOTE: This bit should only be set when the I2C master has
 *                    hung. If this bit is set during an active I2C master
 *                    transaction, the I2C slave will hang, which will require
 *                    the host to reset the slave.
 * [0] - SIG_COND_RST: 1 – Reset all gyro digital signal path, accel digital
 *                         signal path, and temp digital signal path. This bit
 *                         also clears all the sensor registers.
 ******************************************************************************/
static const uint8_t MPU9250_USER_CTRL_ADDR = 0x6A;

/*******************************************************************************
 * Power Management Control Register - RW - Reset Value 0x00
 * [7] - H_RST: 1 – Reset the internal registers and restores the default
 *                  settings. Write a 1 to set the reset, the bit will auto
 *                  clear.
 * [6] - SLEEP: When set, the chip is set to sleep mode (After OTP loads, the
 *              PU_SLEEP_MODE bit will be written here).
 *
 * [5] - CYCLE: When set, and SLEEP and STANDBY are not set, the chip will cycle
 *              between sleep and taking a single sample at a rate determined by
 *              LP_ACCEL_ODR register.
 *              NOTE: When all accelerometer axis are disabled via PWR_MGMT_2
 *              register bits and cycle is enabled, the chip will wake up at the
 *              rate determined by the respective registers above, but will not
 *              take any samples.
 * [4] - GYRO_STANDBY: When set, the gyro drive and pll circuitry are enabled,
 *                     but the sense paths are disabled. This is a low power
 *                     mode that allows quick enabling of the gyros.
 * [3] - PD_PTAT: Power down internal PTAT voltage generator and PTAT ADC.
 * [2:0] - CLK_SEL: Clock source select.
 *
 *                      ------------------------------
 *                      |   Code    |  Clock Source  |
 *                  ----|-----------|----------------|
 *                  | 0 | 0 | 0 | 0 | Internal 20MHz |
 *                  | 1 | 0 | 0 | 1 | Auto Select    |
 *                  | 2 | 0 | 1 | 0 | Auto Select    |
 *                  | 3 | 0 | 1 | 1 | Auto Select    |
 *                  | 4 | 1 | 0 | 0 | Auto Select    |
 *                  | 5 | 1 | 0 | 1 | Auto Select    |
 *                  | 6 | 1 | 1 | 0 | Auto Select    |
 *                  | 7 | 1 | 1 | 1 | Stop           |
 *                  ----------------------------------
 ******************************************************************************/
static const uint8_t MPU9250_PWR_MGMT1_ADDR = 0x6B;

/*******************************************************************************
 * Power Management 2 Control Register - RW - Reset Value 0x00
 * [7:6] - Reserved!
 * [5] - DISABLE_XA: 1 - X accelerometer is disabled.
 *                   0 - X accelerometer On.
 * [4] - DISABLE_YA: Idem.
 * [3] - DISABLE_ZA: Idem.
 * [2] - DISABLE_XG: Idem.
 * [1] - DISABLE_YG: Idem.
 * [0] - DISABLE_ZG: Idem.
 ******************************************************************************/
static const uint8_t MPU9250_PWR_MGMT2_ADDR = 0x6C;

/******************************************************************************
 * Digital Motion Process Registers - RW
 ******************************************************************************/
static const uint8_t MPU9250_DMP_BANK = 0x6D;
static const uint8_t MPU9250_DMP_RW_PNT = 0x6E;
static const uint8_t MPU9250_DMP_REG = 0x6F;
static const uint8_t MPU9250_DMP_REG_1 = 0x70;
static const uint8_t MPU9250_DMP_REG_2 = 0x71;

/*******************************************************************************
 * FIFO Count Registers - Read Only - Reset Value 0x00
 * H - [4:0] - High Bits, count indicates the number of written bytes in the
 *             FIFO.
 * L - [7:0] - Low Bits, count indicates the number of written bytes in the
 *             FIFO.
 * Note: Reading FIFO_COUNTH latches the data for both FIFO_COUNTH, and
 *       FIFO_COUNTL.
 ******************************************************************************/
static const uint8_t MPU9250_FIFO_COUNTH_ADDR = 0x72;
static const uint8_t MPU9250_FIFO_COUNTL_ADDR = 0x73;

/*******************************************************************************
 * FIFO Read Write Register - RW - Reset Value 0x00
 * [7:0] - D: RW command provides Read or Write operation for the FIFO.
 ******************************************************************************/
static const uint8_t MPU9250_FIFO_RW_ADDR = 0x74;

/*******************************************************************************
 * Who Am I Register - Read Only - Reset Value 0x71
 * [7:0] - WHOAMI: Register to indicate to user which device is being accessed.
 ******************************************************************************/
static const uint8_t MPU9250_WHO_AM_I_ADDR = 0x75;

/*******************************************************************************
 * Accelerometer Offset Registers - RW - Reset Value 0x00
 * H - [7:0] - Upper bits of the accelerometer offset cancellation. +/- 16g
 *             Offset cancellation in all Full Scale modes, 15 bit 0.98mg steps
 * L - [7:1] - Lower bits of the accelerometer offset cancellation. +/- 16g
 *             Offset cancellation in all Full Scale modes, 15 bit 0.98mg steps
 ******************************************************************************/
static const uint8_t MPU9250_XA_OFFSET_H_ADDR = 0x77;
static const uint8_t MPU9250_XA_OFFSET_L_ADDR = 0x78;
static const uint8_t MPU9250_YA_OFFSET_H_ADDR = 0x7A;
static const uint8_t MPU9250_YA_OFFSET_L_ADDR = 0x7B;
static const uint8_t MPU9250_ZA_OFFSET_H_ADDR = 0x7D;
static const uint8_t MPU9250_ZA_OFFSET_L_ADDR = 0x7E;
/*==============================================================================
 *                            END REGISTER MAPPING
 *============================================================================*/

// Global Variables ============================================================
static volatile uint8_t mpuAddr = 0x00;
static volatile float32_t gResolution = 0.0f;
static volatile uint16_t aResolution = 0;

// Private functions ===========================================================
static mpu9250_status_t mpu9250_isReady(void);
static mpu9250_status_t mpu9250_getStatus(void);
static mpu9250_status_t mpu9250_writeReg(const uint8_t regAddr, uint8_t data);
static mpu9250_status_t mpu9250_readReg(const uint8_t regAddr, uint8_t *pData);
static mpu9250_status_t mpu9250_readBytes(
    const uint8_t regAddr, uint8_t *pData, uint8_t count);

// Public Functions ============================================================
/*******************************************************************************
 * Configure and init MPU9250 IMU (Accelerometer and Gyroscope)
 * ToDo:
 *      implement SPI protocol.
 ******************************************************************************/
mpu9250_status_t mpu9250_init(mpu9250_InitStruct_t* mpu9250_Init)
{
  mpu9250_status_t status = MPU9250_ERROR;
  uint8_t tmpData = 0x00;

  if(mpu9250_isReady() != MPU9250_OK)
    return status;

  mpu9250_reset();
  LL_mDelay(1000);

  if(mpu9250_writeReg(MPU9250_PWR_MGMT1_ADDR, 0x01) != MPU9250_OK)
    return status;

  tmpData = mpu9250_Init->Gyro_LPF;
  if(mpu9250_writeReg(MPU9250_CONFIG_ADDR, tmpData) != MPU9250_OK)
    return status;

  if(mpu9250_readReg(MPU9250_GYRO_CONFIG_ADDR, &tmpData) != MPU9250_OK)
    return status;

  tmpData &= ~0x03;
  tmpData &= ~0x18;
  tmpData |= (mpu9250_Init->Gyro_Scale << 3);
  if(mpu9250_writeReg(MPU9250_GYRO_CONFIG_ADDR, tmpData) != MPU9250_OK)
    return status;

  if(mpu9250_readReg(MPU9250_ACCEL_CONFIG_ADDR, &tmpData) != MPU9250_OK)
    return status;

  tmpData &= ~0x18;
  tmpData = (mpu9250_Init->Accel_Scale << 3);
  if(mpu9250_writeReg(MPU9250_ACCEL_CONFIG_ADDR, tmpData) != MPU9250_OK)
    return status;

  if(mpu9250_readReg(MPU9250_ACCEL_CONFIG2_ADDR, &tmpData) != MPU9250_OK)
    return status;

  tmpData &= ~0x0F;
  tmpData |= mpu9250_Init->Accel_LPF;
  if(mpu9250_writeReg(MPU9250_ACCEL_CONFIG2_ADDR, tmpData) != MPU9250_OK)
    return status;

  switch(mpu9250_Init->Gyro_Scale)
  {
    case MPU9250_GYRO_FULLSCALE_250DPS:
      gResolution = 131.0f;
      break;
    case MPU9250_GYRO_FULLSCALE_500DPS:
      gResolution = 65.5f;
      break;
    case MPU9250_GYRO_FULLSCALE_1000DPS:
      gResolution = 32.8f;
      break;
    case MPU9250_GYRO_FULLSCALE_2000DPS:
      gResolution = 16.4f;
      break;
    default:
      gResolution = 131.0f;
      break;
  }

  switch(mpu9250_Init->Accel_Scale)
  {
    case MPU9250_ACCEL_FULLSCALE_2G:
      aResolution = 16384;
      break;
    case MPU9250_ACCEL_FULLSCALE_4G:
      aResolution = 8192;
      break;
    case MPU9250_ACCEL_FULLSCALE_8G:
      aResolution = 4096;
      break;
    case MPU9250_ACCEL_FULLSCALE_16G:
      aResolution = 2048;
      break;
    default:
      aResolution = 16384;
      break;
  }

  status = MPU9250_OK;
  return status;
}

/*******************************************************************************
 * init interrupt pin only if internal sample rate == 1kHz
 ******************************************************************************/
mpu9250_status_t mpu9250_initInterrupt(uint16_t sampleRate)
{
  mpu9250_status_t status = MPU9250_OK;

  uint8_t tmpData[2] = { 0 };
  uint8_t *ptmpData = &tmpData[0];

  uint8_t sampleRate_Div = 0;

  if(mpu9250_readReg(MPU9250_GYRO_CONFIG_ADDR, ptmpData++) != MPU9250_OK)
    status = MPU9250_ERROR;

  if(mpu9250_readReg(MPU9250_CONFIG_ADDR, ptmpData) != MPU9250_OK)
    status = MPU9250_ERROR;

  sampleRate_Div = (uint8_t) ((1000 / sampleRate) - 1);

  sampleRate_Div = (sampleRate_Div < 3) ? (3) : (sampleRate_Div);

  if(mpu9250_writeReg(MPU9250_SAMPLE_RATE_DIV_ADDR, sampleRate_Div)
      != MPU9250_OK)
    status = MPU9250_ERROR;

  tmpData[0] = 0x00;
  if(mpu9250_writeReg(MPU9250_INT_PIN_CONFIG_ADDR, tmpData[0]) != MPU9250_OK)
    status = MPU9250_ERROR;

  tmpData[0] = 0x01;
  if(mpu9250_writeReg(MPU9250_INT_ENABLE_ADDR, tmpData[0]) != MPU9250_OK)
    status = MPU9250_ERROR;

  return status;
}

/*******************************************************************************
 * Reset device
 ******************************************************************************/
mpu9250_status_t mpu9250_reset(void)
{
  mpu9250_status_t status = MPU9250_ERROR;

  if(mpu9250_writeReg(MPU9250_PWR_MGMT1_ADDR, MPU9250_CMD_RESET) != MPU9250_OK)
    status = MPU9250_OK;

  LL_mDelay(100);

  if(mpu9250_writeReg(MPU9250_PWR_MGMT1_ADDR, 0x00) != MPU9250_OK)
    status = MPU9250_OK;

  return status;
}

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getBias_int16(
    uint8_t samples, int16_t *pAccel, int16_t *pGyro)
{
  mpu9250_status_t status = MPU9250_OK;

  int16_t accelData[3] = { 0 };
  int16_t *pAccelData = &accelData[0];
  int16_t gyroData[3] = { 0 };
  int16_t *pGyroData = &gyroData[0];

  int32_t accelTmp[3] = { 0 };
  int32_t gyroTmp[3] = { 0 };

  for(uint8_t i = 0; i < samples; i++)
  {
    if(mpu9250_readData_int16(pAccelData, pGyroData) == MPU9250_OK)
    {
      for(uint8_t u = 0; u < 3; u++)
      {
        accelTmp[u] += pAccelData[u];
        gyroTmp[u] += pGyroData[u];
      }
    }
    else
      status = MPU9250_ERROR;
  }

  for(uint8_t i = 0; i < 3; i++)
  {
    pAccel[i] = (int16_t) (accelTmp[i] / samples);
    pGyro[i] = (int16_t) (gyroTmp[i] / samples);
  }

  return status;
}

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getBias_float(
    uint8_t samples, float32_t *pAccel, float32_t *pGyro)
{
  mpu9250_status_t status = MPU9250_OK;

  float32_t accelData[3] = { 0 };
  float32_t *pAccelData = &accelData[0];
  float32_t gyroData[3] = { 0 };
  float32_t *pGyroData = &gyroData[0];

  float32_t accelTmp[3] = { 0.0f };
  float32_t gyroTmp[3] = { 0.0f };

  for(uint8_t i = 0; i < samples; i++)
  {
    if(mpu9250_readData_float(pAccelData, pGyroData) == MPU9250_OK)
    {
      for(uint8_t u = 0; u < 3; u++)
      {
        accelTmp[u] += pAccelData[u];
        gyroTmp[u] += pGyroData[u];
      }
    }
    else
      status = MPU9250_ERROR;
  }

  for(uint8_t i = 0; i < 3; i++)
  {
    pAccel[i] = (accelTmp[i] / samples);
    pGyro[i] = (gyroTmp[i] / samples);
  }

  return status;
}

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getResolution_int16(int16_t *pResolution)
{
  mpu9250_status_t status = MPU9250_OK;

  pResolution[0] = aResolution;
  pResolution[1] = (int16_t) gResolution;

  return status;
}

/*******************************************************************************
 *
 ******************************************************************************/
mpu9250_status_t mpu9250_getResolution_float(float32_t *pResolution)
{
  mpu9250_status_t status = MPU9250_OK;

  pResolution[1] = (float32_t) aResolution;
  pResolution[2] = gResolution;

  return status;
}

/*******************************************************************************
 * Return device id --> MPU9250_DEVICE_ID == 0x71
 ******************************************************************************/
uint8_t mpu9250_readID(void)
{
  uint8_t devID = 0x00;
  uint8_t *pDevID = &devID;

  if(mpuAddr == 0x00)
    mpu9250_isReady();

  mpu9250_readReg(MPU9250_WHO_AM_I_ADDR, pDevID);

  return devID;
}

/*******************************************************************************
 * Read device's temperature: int16_t --> raw data.
 *  float32_t:
 *          temperature = [(rawData - RoomTemp_Offset)/ 333.87f] + 21.0f
 ******************************************************************************/
mpu9250_status_t mpu9250_readTemperature_int16(int16_t *pTemperature)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t rawData[2] = { 0 };
  uint8_t *pRawData = &rawData[0];

  if(mpu9250_readBytes(MPU9250_TEMP_H_ADDR, pRawData, 2) == MPU9250_OK)
    status = MPU9250_OK;

  *pTemperature = (int16_t) (((int16_t) rawData[0] << 8) | rawData[1]);

  return status;
}

/*******************************************************************************
 * Return Accelerometer data. Default 16.384 LSB/g
 ******************************************************************************/
mpu9250_status_t mpu9250_readAccelData_int16(int16_t *pAccel)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t rawData[6] = { 0 };
  uint8_t *pRawData = &rawData[0];

  if(mpu9250_readBytes(MPU9250_ACCEL_XOUT_H_ADDR, pRawData, 6) == MPU9250_OK)
    status = MPU9250_OK;

  for(uint8_t i = 0; i < 3; i++)
    *pAccel++ =
        (int16_t) (((int16_t) rawData[2 * i] << 8) | rawData[2 * i + 1]);

  return status;
}

/*******************************************************************************
 * Return Gyroscope data. Default 131.0 LSB/dps
 ******************************************************************************/
mpu9250_status_t mpu9250_readGyroData_int16(int16_t *pGyro)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t rawData[6] = { 0 };
  uint8_t *pRawData = &rawData[0];

  if(mpu9250_readBytes(MPU9250_GYRO_XOUT_H_ADDR, pRawData, 6) == MPU9250_OK)
    status = MPU9250_OK;

  for(uint8_t i = 0; i < 3; i++)
    *pGyro++ = (int16_t) (((int16_t) rawData[2 * i] << 8) | rawData[2 * i + 1]);

  return status;
}

/*******************************************************************************
 * Return Accelerometer and Gyroscope data
 ******************************************************************************/
mpu9250_status_t mpu9250_readData_int16(int16_t *pAccel, int16_t *pGyro)
{
  mpu9250_status_t status = MPU9250_OK;

  if(mpu9250_getStatus() == MPU9250_OK)
  {
    if(mpu9250_readAccelData_int16(pAccel) != MPU9250_OK)
      status = MPU9250_ERROR;

    if(mpu9250_readGyroData_int16(pGyro) != MPU9250_OK)
      status = MPU9250_ERROR;
  }

  return status;
}

/*******************************************************************************
 * Read device's temperature: int16_t --> raw data.
 *  float32_t:
 *          temperature = [(rawData - RoomTemp_Offset)/ 333.87f] + 21.0f
 ******************************************************************************/
mpu9250_status_t mpu9250_readTemperature_float(float32_t *pTemperature)
{
  mpu9250_status_t status = MPU9250_OK;

  int16_t temp = 0;
  int16_t *pTemp = &temp;

  if(mpu9250_readTemperature_int16(pTemp) != MPU9250_OK)
    status = MPU9250_ERROR;

  *pTemperature = ((float32_t) temp / 333.87f) + 21.0f;

  return status;
}

/*******************************************************************************
 * Return Accelerometer data. Default 16.384 LSB/g
 ******************************************************************************/
mpu9250_status_t mpu9250_readAccelData_float(float32_t *pAccel)
{
  mpu9250_status_t status = MPU9250_OK;

  int16_t accData[3] = { 0 };
  int16_t *pAccData = &accData[0];

  if(mpu9250_readAccelData_int16(pAccData) != MPU9250_OK)
    status = MPU9250_ERROR;

  for(uint8_t i = 0; i < 3; i++)
    *pAccel++ = ((float32_t) (accData[i]) / aResolution);

  return status;
}

/*******************************************************************************
 * Return Gyroscope data. Default 131.0 LSB/dps
 ******************************************************************************/
mpu9250_status_t mpu9250_readGyroData_float(float32_t *pGyro)
{
  mpu9250_status_t status = MPU9250_OK;

  int16_t gyroData[3] = { 0 };
  int16_t *pGyroData = &gyroData[0];

  if(mpu9250_readGyroData_int16(pGyroData) != MPU9250_OK)
    status = MPU9250_ERROR;

  for(uint8_t i = 0; i < 3; i++)
    *pGyro++ = ((float32_t) (gyroData[i]) / gResolution);

  return status;
}

/*******************************************************************************
 * Return Accelerometer and Gyroscope data
 ******************************************************************************/
mpu9250_status_t mpu9250_readData_float(float32_t *pAccel, float32_t *pGyro)
{
  mpu9250_status_t status = MPU9250_OK;

  if(mpu9250_getStatus() == MPU9250_OK)
  {
    if(mpu9250_readAccelData_float(pAccel) != MPU9250_OK)
      status = MPU9250_ERROR;

    if(mpu9250_readGyroData_float(pGyro) != MPU9250_OK)
      status = MPU9250_ERROR;
  }

  return status;
}

// Private Functions ===========================================================
static mpu9250_status_t mpu9250_isReady(void)
{
  mpu9250_status_t status = MPU9250_ERROR;
  uint8_t devAddr = MPU9250_ADDR;

  if(cncI2C_isDeviceReady(I2C1, devAddr++))
  {
    mpuAddr = MPU9250_ADDR;
    status = MPU9250_OK;
  }
  else if(cncI2C_isDeviceReady(I2C1, devAddr))
  {
    mpuAddr = MPU9250_ADDR_ALT;
    status = MPU9250_OK;
  }

  return status;
}

static mpu9250_status_t mpu9250_getStatus(void)
{
  uint8_t status = MPU9250_ERROR;

  uint8_t mpuStatus = 0x00;
  uint8_t *pMpuStatus = &mpuStatus;

  if(mpu9250_readReg(MPU9250_INT_STATUS_ADDR, pMpuStatus) == MPU9250_OK)
    status = MPU9250_OK;

  status =
      (mpuStatus & MPU9250_INT_DATA_READY_MSK) ? (MPU9250_OK) : (MPU9250_ERROR);

  return status;
}

static mpu9250_status_t mpu9250_writeReg(const uint8_t regAddr, uint8_t data)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t aDev[2] = { mpuAddr, regAddr };
  uint8_t *pDev = &aDev[0];

  if(cncI2C_WriteByte(I2C1, pDev, data))
    status = MPU9250_OK;

  return status;
}

static mpu9250_status_t mpu9250_readReg(const uint8_t regAddr, uint8_t *pData)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t aDev[2] = { mpuAddr, regAddr };
  uint8_t *pDev = &aDev[0];

  if(cncI2C_ReadByte(I2C1, pDev, pData))
    status = MPU9250_OK;

  return status;
}

static mpu9250_status_t mpu9250_readBytes(
    const uint8_t regAddr, uint8_t *pData, uint8_t count)
{
  mpu9250_status_t status = MPU9250_ERROR;

  uint8_t len = count;
  uint8_t memAddr = regAddr;

  while(len--)
    if(mpu9250_readReg(memAddr++, pData++) == MPU9250_OK)
      status = MPU9250_OK;

  return status;
}

// EOF =========================================================================
