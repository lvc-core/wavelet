#ifndef FREQUENCYGRID_H
#define FREQUENCYGRID_H

#include <stddef.h>

typedef enum {
    FREQUENCY_LINEAR,
    FREQUENCY_LOGARITHMIC
} FrequencySpacing;


typedef struct {
    double min_hz;
    double max_hz;
    size_t count;
    FrequencySpacing spacing;
} FrequencyConfig;

typedef struct {
    size_t count;
    double* values_hz;
} FrequencyGrid;



int frequency_grid_create(
	const FrequencyConfig* config,
	FrequencyGrid* output
);

void frequency_grid_destroy(FrequencyGrid* grid);

#endif // FREQUENCYGRID_H
