#ifndef ESTIMADOR_H_
#define ESTIMADOR_H_

#include "mpu9250.h"

// =============================================================================
typedef float   float32_t;

typedef enum { FILTER_ERROR = 0, FILTER_OK } filter_status_t;

typedef struct
{
  uint16_t SampleRate;
  float32_t Weight;
} filter_init_t;

// =============================================================================
filter_status_t estimator_init(filter_init_t *filter_InitStruct);
filter_status_t estimator_notFilteredAngles(float32_t *pAngles);
filter_status_t estimator_filteredAngles(float32_t *pAngles);

#endif /* ESTIMADOR_H_ */
// EOF =========================================================================
