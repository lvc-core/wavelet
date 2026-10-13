#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "frequencyGrid.h"


int frequency_grid_create(
	const FrequencyConfig* config,
	FrequencyGrid* output)
{
    size_t bytes = 0;

    if (!checked_multiply(config->count, sizeof(*output->values_hz), &bytes)) {
	return error_message("Frequency grid is too large");
    }

    output->values_hz = malloc(bytes);

    if (output->values_hz == NULL) {
	return error_message("Could not allocate memory for frequency grid");
    }

    output->count = config->count;

    for (size_t k=0; k<output->count; ++k) {
	const double fraction = (double)k / (double)(output->count -1);

	if (config->spacing == FREQUENCY_LINEAR) {
	    output->values_hz[k] = config->min_hz + fraction * (config->max_hz - config->min_hz);	    
	} else {
	    output->values_hz[k] = exp( (1.0 - fraction) * log(config->min_hz) + fraction * log(config->max_hz));
	}
    }

    for (size_t k=1; k<output->count; ++k) {
	if (!isfinite(output->values_hz[k]) || !(output->values_hz[k] > output->values_hz[k-1])) {
	    return error_message("Frequency grid is numerically not strictly increasing");
	}
    }

    return 0;
};


void frequency_grid_destroy(FrequencyGrid* grid){
    free(grid->values_hz);
    *grid = (FrequencyGrid){0};
}
