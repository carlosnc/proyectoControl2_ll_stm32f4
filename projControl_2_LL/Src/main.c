#include "initHardware.h"
#include "controlador.h"

// =============================================================================
__IO uint8_t state = 0;
__IO uint32_t cycles_count = 0;

// =============================================================================
static void initApp(void);
static void updateData(void);

// Main function ===============================================================
int main(void)
{
  CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  initHardware_InitSystem();
  cncUSART_send2Bash(UART5, bash_ClearScreen, (uint8_t *)"\r");

  while (1)
    __WFI();

  updateData();
  return 0;
}

// =============================================================================
static void initApp(void)
{
  uint8_t helloMsg[35] = "\t\t\tSTM32F4 Discovery - Carlosnc\n\r";
  cncUSART_send2Bash(UART5, bash_Cursor2Home, (uint8_t *)"\r");
  cncUSART_send2Bash(UART5, bash_LightBlue, helloMsg);

  if(MPU9250_DEVICE_ID == mpu9250_readID())
  {
    cncUSART_send2Bash(UART5, bash_LightGreen, (uint8_t *)"MPU9250 conectado\n\n\r");
    cncUSART_send2Bash(UART5, bash_White, (uint8_t *)"iPitch\toPitch\tiRoll\toRoll\n\r");
    LL_TIM_EnableCounter(TIM7);
    LL_TIM_GenerateEvent_UPDATE(TIM7);
    LL_TIM_EnableIT_UPDATE(TIM7);

  }
  else
    cncUSART_send2Bash(UART5, bash_LightRed, (uint8_t *)"MPU9250 desconectado\n\r");
}

static void updateData(void)
{
  DWT->CYCCNT = 0;
  float32_t outputs[2] = { 0.0f };
  float32_t serialData[4] = { 0.0f };

  float32_t filteredAngles[2] = { 0.0f };
  float32_t *pFilteredAngles = &filteredAngles[0];

  estimator_filteredAngles(pFilteredAngles);
  outputs[0] = controlador_planta(filteredAngles[0], 0);
  outputs[1] = controlador_planta(filteredAngles[1], 1);

  cncServo_updatePosition(outputs[0], SERVO_CHANNEL_1);
  cncServo_updatePosition(outputs[0], SERVO_CHANNEL_2);
  cncServo_updatePosition(outputs[1], SERVO_CHANNEL_3);

  for(uint8_t i = 0; i < 2; i++)
  {
    serialData[2*i] = filteredAngles[i];
    serialData[2*i + 1] = outputs[i];
  }

  cncUSART_sendData_float(UART5, &serialData[0], 4, (UART_DATA_LOG | UART_DATA_FORMAT_TAB));
  cycles_count = DWT->CYCCNT;
  __NOP();
}

// =============================================================================
void EXTI0_IRQHandler(void)
{
  LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
  LL_mDelay(300);

  if(0 == state)
  {
    state++;
    initApp();
  }
}

void TIM7_IRQHandler(void)
{
  if(LL_TIM_IsActiveFlag_UPDATE(TIM7) == 1)
    LL_TIM_ClearFlag_UPDATE(TIM7);

  updateData();
}
// EOF =========================================================================
