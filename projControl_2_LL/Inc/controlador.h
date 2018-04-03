#ifndef CONTROLADOR_H_
#define CONTROLADOR_H_

#include "stm32f4xx.h"
#include "stm32f4xx_ll_system.h"
#include "stm32f4xx_ll_cortex.h"

// =============================================================================
typedef float float32_t;

// =============================================================================
float32_t controlador_planta(float32_t inputReference, uint8_t indice_planta);

// =============================================================================

#endif /* CONTROLADOR_H_ */
// EOF =========================================================================
