#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "results.h"


int result_accumulator_init(
	ResultAccumulator* results,
	size_t frequency_count,
	size_t timestep_count)
{
    size_t element_count = 0;
    size_t bytes = 0;

    if (!checked_multiply(frequency_count, timestep_count, &element_count) ||
	    !checked_multiply(element_count, sizeof(*results->sum_magnitudes), &bytes)) {
	return error_message("result matrix is too large.");
    }

    results->sum_magnitudes = calloc(element_count, sizeof(*results->sum_magnitudes));

    if (results->sum_magnitudes == NULL) {
	return error_message("Could not allocated memory for result matrix.");
    }

    results->frequency_count = frequency_count;
    results->timestep_count = timestep_count;

    return 0;
}



int result_accumulator_add(
	ResultAccumulator* results,
	size_t frequency_index,
	const double* magnitudes)
{
    if (frequency_index >= results->frequency_count) {
	return error_message("Invalid frequency index.");
    }

    double* block = &results->sum_magnitudes[frequency_index * results->timestep_count];

    for (size_t t = 0; t < results->timestep_count; ++t) {
	const double sum = block[t] + magnitudes[t];

	if (!isfinite(sum)) {
	    return error_message("overflow while adding results.");
	}

	block[t] = sum;
    }

    return 0;
}


void result_accumulator_destroy(ResultAccumulator *results){
    free(results->sum_magnitudes);
    *results = (ResultAccumulator) {0};
}


















