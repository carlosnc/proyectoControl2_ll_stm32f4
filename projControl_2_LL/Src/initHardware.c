#include "initHardware.h"

// =============================================================================
static void initHardware_Nvic(void);
static void initHardware_SystemClock(void);
static void initHardware_GPIO(void);
static void initHardware_COM(void);
static void initHardware_PWM(void);
static void initHardware_Sampler(void);
static void initHardware_Platform(void);

// Public functions ============================================================
/*******************************************************************************
 * @brief   Init System.
 * @retval  None.
 ******************************************************************************/
void initHardware_InitSystem(void)
{
  initHardware_Nvic();
  initHardware_SystemClock();
  initHardware_GPIO();
  initHardware_COM();
  initHardware_PWM();
  initHardware_Sampler();
  initHardware_Platform();
}

/*******************************************************************************
 * @brief   Test LED outputs.
 * @retval  None.
 ******************************************************************************/
void initHardware_TestOutput(void)
{
  /*!< pin = LED_GREEN */
  uint32_t pin = LED_GREEN.GPIO_Pin;

  for(uint8_t i = 0; i < 8; i++)
  {
    LL_GPIO_TogglePin(GPIOD, pin);
    LL_mDelay(LED_BLINK_MEDIUM);

    if(i == 3)
      pin = LED_GREEN.GPIO_Pin;
    else
      pin = (pin << 1);
  }
}

/*******************************************************************************
 * @brief   Infinite Loop.
 * @retval  None.
 ******************************************************************************/
void Error_Handler(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct;
  GPIO_InitStruct.Pin = LED_BLUE.GPIO_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(LED_BLUE.GPIO_Port, &GPIO_InitStruct);

  while(1)
  {
    LL_GPIO_TogglePin(LED_BLUE.GPIO_Port, LED_BLUE.GPIO_Pin);
    LL_mDelay(LED_BLINK_FAST);
  }
}

// =============================================================================
/*******************************************************************************
 * @brief Configure interrupts and priority.
 * @retval None.
 ******************************************************************************/
static void initHardware_Nvic(void)
{
  uint32_t nvic_priority = 0;
  nvic_priority = NVIC_EncodePriority(NVIC_PRIORITYGROUP_1, 0, 0);

  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_1);
  NVIC_SetPriority(MemoryManagement_IRQn, nvic_priority);
  NVIC_SetPriority(BusFault_IRQn, nvic_priority);
  NVIC_SetPriority(UsageFault_IRQn, nvic_priority);
  NVIC_SetPriority(SVCall_IRQn, nvic_priority);
  NVIC_SetPriority(DebugMonitor_IRQn, nvic_priority);
  NVIC_SetPriority(PendSV_IRQn, nvic_priority);
  NVIC_SetPriority(SysTick_IRQn, nvic_priority);
}

/*******************************************************************************
 * @brief System clock and peripherals configuration.
 * @retval None.
 * @note Hclk = 168MHz, APB1 = 42MHz, APB2 = 84MHz.
 ******************************************************************************/
static void initHardware_SystemClock(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5)
    Error_Handler();

  /*!< Voltage Regulator 1 for max frequency */
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();
  while(LL_RCC_HSE_IsReady() != 1)
    __NOP();

  /*!< High Speed External Crystal = 8MHz, PLLM = 8, PLLN = 336, PLLP = 2 */
  /*!< HCLK = ( HSE / PLLM )*( PLLN / PLLP) = 168MHz */
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336,
                              LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1)
    __NOP();

  /*!< AHB  max. frequency = 168MHz --> HCLK/DIV_1 */
  /*!< APB1 max. frequency = 42MHz  --> HCLK/DIV_4 */
  /*!< APB2 max. frequency = 84MHz  --> HCLK/DIV_2 */
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

  /*!< System clock source: PLL output */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
    __NOP();

  LL_SetSystemCoreClock(168000000);
  LL_Init1msTick(SystemCoreClock);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
}

/*******************************************************************************
 * @brief Configure inputs and outputs.
 * @retval None.
 ******************************************************************************/
static void initHardware_GPIO(void)
{
  uint32_t IRQPriority = 0;

  LL_GPIO_InitTypeDef GPIO_InitStruct;
  LL_EXTI_InitTypeDef EXTI_InitStruct;

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOH);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);

  LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);

  /*!< LED Blue configuration */
  GPIO_InitStruct.Pin = LED_BLUE.GPIO_Pin;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  LL_GPIO_Init(LED_BLUE.GPIO_Port, &GPIO_InitStruct);

  /*!< Button interrupt configuration */
  EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
  EXTI_InitStruct.LineCommand = ENABLE;
  EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
  EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
  LL_EXTI_Init(&EXTI_InitStruct);

  LL_GPIO_SetPinPull(BUTTON.GPIO_Port, BUTTON.GPIO_Pin, LL_GPIO_PULL_NO);
  LL_GPIO_SetPinMode(BUTTON.GPIO_Port, BUTTON.GPIO_Pin, LL_GPIO_MODE_INPUT);

  IRQPriority = NVIC_EncodePriority(NVIC_PRIORITYGROUP_1, 0, 2);
  NVIC_SetPriority(EXTI0_IRQn, IRQPriority);
  NVIC_EnableIRQ(EXTI0_IRQn);
}

static void initHardware_COM(void)
{
  cncI2C_Init(I2C1, 400000);
  cncUSART_init(UART5);
}

/*******************************************************************************
 * @brief Config Timer4 for PWM output. Period: 50 Hz.
 * @retval None.
 ******************************************************************************/
static void initHardware_PWM(void)
{
  const uint32_t TIMER_CLOCK = (SystemCoreClock / 2);
  uint32_t prescaler_val  = 0;
  uint32_t autoreload_val = 0;

  LL_TIM_InitTypeDef    TIM_InitStruct;
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct;
  LL_GPIO_InitTypeDef   GPIO_InitStruct;

  /*!< Timer 4 configuration */
  if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_TIM4) != 1)
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);

  /*!< Event frequency = TIMER_CLOCK / [(1 + prescaler)(1 + autoreload)]*/
  prescaler_val  = 83;
  autoreload_val = 19999;

  TIM_InitStruct.Prescaler = (uint16_t)prescaler_val;
  TIM_InitStruct.Autoreload = autoreload_val;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;

  if(LL_TIM_Init(TIM4, &TIM_InitStruct) != SUCCESS)
    Error_Handler();

  LL_TIM_SetClockSource(TIM4, LL_TIM_CLOCKSOURCE_INTERNAL);
  LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_EnablePreload(TIM4, LL_TIM_CHANNEL_CH3);

  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;

  /*!< All outputs have the same configuration */
  if(LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct) != SUCCESS)
    Error_Handler();
  if(LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct) != SUCCESS)
    Error_Handler();
  if(LL_TIM_OC_Init(TIM4, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct) != SUCCESS)
    Error_Handler();

  LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_DisableFast(TIM4, LL_TIM_CHANNEL_CH3);

  LL_TIM_SetTriggerOutput(TIM4, LL_TIM_TRGO_RESET);
  LL_TIM_DisableMasterSlaveMode(TIM4);

  /*!< PWM Outputs configuration */
  /*!< LEDs GPIO: output TIM4 */
  GPIO_InitStruct.Pin = (LED_GREEN.GPIO_Pin | LED_ORANGE.GPIO_Pin | LED_RED.GPIO_Pin);
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
  LL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

/*******************************************************************************
 * @brief Init Timer 7 for sampling. 100 Hz
 * @retval None.
 ******************************************************************************/
static void initHardware_Sampler(void)
{
  const uint32_t TIMER_CLOCK = (SystemCoreClock / 2);
  uint32_t nvic_priority = 0;
  uint32_t prescaler_val  = 0;
  uint32_t autoreload_val = 0;

  LL_TIM_InitTypeDef TIM_InitStruct;

  /*!< Timer 7 configuration */
  if(LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_TIM7) != 1)
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM7);

  /*!< Event frequency = TIMER_CLOCK / [(1 + PSR)(1 + ARR)]*/
  prescaler_val  = 83;
  autoreload_val = 9999;

  TIM_InitStruct.Prescaler = prescaler_val;
  TIM_InitStruct.Autoreload = autoreload_val;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;

  if(LL_TIM_Init(TIM7, &TIM_InitStruct) != SUCCESS)
    Error_Handler();

  /*!> Enable Timer7 interrupt */
  nvic_priority = NVIC_EncodePriority(NVIC_PRIORITYGROUP_1, 0, 3);
  NVIC_ClearPendingIRQ(TIM7_IRQn);
  NVIC_SetPriority(TIM7_IRQn, nvic_priority);
  NVIC_EnableIRQ(TIM7_IRQn);
}

static void initHardware_Platform(void)
{
  mpu9250_InitStruct_t mpu9250_InitStruct;
  mpu9250_InitStruct.SampleRate = 100;
  mpu9250_InitStruct.Interface = MPU9250_INTERFACE_I2C;
  mpu9250_InitStruct.Accel_Axes = MPU9250_ACCEL_XYZ_ENABLE;
  mpu9250_InitStruct.Accel_LPF = MPU9250_ACCEL_LPF_99HZ;
  mpu9250_InitStruct.Accel_Scale = MPU9250_ACCEL_FULLSCALE_2G;
  mpu9250_InitStruct.Gyro_Axes = MPU9250_GYRO_XYZ_ENABLE;
  mpu9250_InitStruct.Gyro_LPF = MPU9250_GYRO_LPF_92HZ;
  mpu9250_InitStruct.Gyro_Scale = MPU9250_GYRO_FULLSCALE_250DPS;
  mpu9250_init(&mpu9250_InitStruct);

  filter_init_t filter_InitStruct;
  filter_InitStruct.SampleRate = (uint16_t)SAMPLER_FREQ;
  filter_InitStruct.Weight = 0.90f;
  estimator_init(&filter_InitStruct);

  servo_initStruct_t servo_InitStruct;
  servo_InitStruct.Max_Angle = 90;
  servo_InitStruct.Min_Angle = -90;
  servo_InitStruct.Pos_Zero = 90;
  cncServo_init(&servo_InitStruct);
}

// EOF =========================================================================
