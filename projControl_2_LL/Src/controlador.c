#include "controlador.h"

// =============================================================================
__IO float32_t h[2][4] = { { 0.0f }, { 0.0f } }; /*!< h: funcion auxiliar de planta */

// =============================================================================
float32_t controlador_planta(float32_t inputReference, uint8_t indice_planta)
{
  __IO float32_t y = 0.0f;
  __I  float32_t k_num[4] = {0.3774f, 0.2662f, 0.3694f, 0.2744f};
  __I  float32_t k_den[3] = {2.1189f, 1.4832f, 0.3483f};

  h[indice_planta][0]  = inputReference;
  h[indice_planta][0] += k_den[0]*h[indice_planta][1];
  h[indice_planta][0] -= k_den[1]*h[indice_planta][2];
  h[indice_planta][0] += k_den[2]*h[indice_planta][3];

  y  = k_num[0]*h[indice_planta][0];
  y -= k_num[1]*h[indice_planta][1];
  y -= k_num[2]*h[indice_planta][2];
  y += k_num[3]*h[indice_planta][3];

  for(uint8_t i = 3; i > 0; i--)
    h[indice_planta][i] = h[indice_planta][i - 1];

  return y;
}

// EOF =========================================================================
