#ifndef RESULTS_H
#define RESULTS_H

#include <stddef.h>

typedef struct {
    size_t frequency_count;
    size_t timestep_count;

    double* sum_magnitudes;
} ResultAccumulator;


int result_accumulator_init(
	ResultAccumulator* results,
	size_t frequency_count,
	size_t timestep_count
);

int result_accumulator_add(
	ResultAccumulator* results,
	size_t frequency_index,
	const double* magnitudes
);

void result_accumulator_destroy(
	ResultAccumulator* results
);



#endif // RESULTS_H
