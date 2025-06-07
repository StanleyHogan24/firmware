// steps.h

#ifndef STEPS_H
#define STEPS_H

#include <stdint.h>

// Reset step-counting state
void steps_init(void);

// Sample accel and bump step count
void steps_update(void);

// (optional) Retrieve total steps
uint32_t steps_get_count(void);

#endif // STEPS_H