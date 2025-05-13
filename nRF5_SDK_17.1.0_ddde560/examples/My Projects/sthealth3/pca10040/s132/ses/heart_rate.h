#ifndef HEART_RATE_H
#define HEART_RATE_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the heart rate module
void heart_rate_init(void);

// Update heart rate calculation with a new sample
// Returns the current BPM value
uint16_t heart_rate_update(float newSample);

// Get the latest BPM value
uint16_t heart_rate_get_bpm(void);

#endif // HEART_RATE_H
