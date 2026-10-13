#ifndef OUTPUT_H
#define OUTPUT_H

#include <stddef.h>
#include <stdio.h>

#include "frequencyGrid.h"
#include "results.h"

int output_directory_prepare(const char* directory);

int output_open_atom(
	const char* directory,
	size_t atom_index,
	FILE** output
);

int output_write_frequency_block(
	FILE* file,
	double frequency_hz,
	double dt_seconds,
	const double* magnitudes,
	size_t timestep_count
);

int output_close(FILE* file);

int output_write_mean(
	const char* directory,
	const ResultAccumulator* results,
	const FrequencyGrid* frequencies,
	double dt_seconds,
	size_t atom_count
);





#endif // OUTPUT_H
