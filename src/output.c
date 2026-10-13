#include <asm-generic/errno-base.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "output.h"

/*
 * static functions
 */
static int ensure_single_directory(const char* path){
    if (mkdir(path, 0777) == 0) {
	return 0;
    }

    if (errno != EEXIST) {
	return file_error("Creation of directory", path);
    }

    struct stat info;

    if (stat(path, &info) != 0) {
	return file_error("Validation of directory", path);
    }

    if (!S_ISDIR(info.st_mode)) {
	fprintf(stderr, "Error: '%s' exists but is no directory.\n", path);
	return -1;
    }

    return 0;
}


static int output_open_named(
	const char* directory,
	const char* filename,
	FILE** output)
{
    const size_t directory_length = strlen(directory);
    const size_t filename_length = strlen(filename);

    *output = NULL;

    const size_t capacity = directory_length + filename_length + 2;

    char* path = malloc(capacity);

    if (path == NULL) {
	return error_message("Memory for output path could not be allocated.");
    }

    const int written = snprintf(path, capacity, "%s/%s", directory, filename);

    int status = 0;

    if (written < 0 || (size_t)written >= capacity) {
	status = error_message("Could not write output path to string.");
    } else {
	*output = fopen(path, "w");

	if (*output == NULL) {
	    status = file_error("opening", path);
	}
    }

    free(path);

    return status;
}

int output_directory_prepare(const char* directory){
    char* path = strdup(directory);

    if (path == NULL) {
	return error_message("No memory for outputpath.");
    }

    int status = 0;

    for (char* position = path + 1; *position != '\0' && status == 0; ++position) {
	if (*position == '/') {
	    *position = '\0';
	    status = ensure_single_directory(path);
	    *position = '/';
	}
    }

    if (status == 0) {
	status = ensure_single_directory(path);
    }

    free(path);

    return status;
}


int output_open_atom(
	const char* directory,
	size_t atom_index,
	FILE** output)
{
    char filename[64];

    const int written = snprintf(filename, sizeof(filename), "atom_%zu.dat", atom_index + 1);

    if (written < 0 || (size_t)written >= sizeof(filename)) {
	return error_message("Atom filename is too long.");
    }

    if (output_open_named(directory, filename, output) != 0) {
	return -1;
    }

    if (fprintf(*output, "# time_s frequency_hz magnitude\n") < 0) {
	output_close(*output);
	*output = NULL;

	return error_message("Header was not written");
    }

    return 0;
}


int output_write_frequency_block(
	FILE* file,
	double frequency_hz,
	double dt_seconds,
	const double* magnitudes,
	size_t timestep_count)
{
    for (size_t t = 0; t < timestep_count; ++t) {
	if (fprintf(file, "%.12e %.12e %.12e\n", (double)t * dt_seconds, frequency_hz, magnitudes[t]) < 0) {
	    return error_message("failed writing result block to file");
	}
    }

    if (fputc('\n', file) == EOF) {
	return error_message("failed writing block separation");
    }

    return 0;
}


int output_close(FILE* file){
    if (fclose(file) != 0) {
	return error_message("failed to close outputfile.");
    }

    return 0;
}


int output_write_mean(
	const char* directory,
	const ResultAccumulator* results,
	const FrequencyGrid* frequencies,
	double dt_seconds,
	size_t atom_count)
{
    if (atom_count == 0 || frequencies->count != results->frequency_count) {
	return error_message("Invalid dimension for result output.");
    }

    FILE* file = NULL;

    if (output_open_named(directory, "results.dat", &file) != 0) {
	return -1;
    }

    bool success = true;

    if (fprintf(file, "# time_s frequency_hz mean_magnitude\n"
		"# atoms=%zu timesteps=%zu frequencies=%zu\n", atom_count, results->timestep_count, results->frequency_count) < 0) {
	success = false;
    }

    for (size_t k = 0; k < frequencies->count && success; ++k) {
	const double* block = &results->sum_magnitudes[k * results->timestep_count];

	for (size_t t = 0; t < results->timestep_count && success; ++t) {
	    const double mean = block[t] / (double)atom_count;

	    if (fprintf(file, "%.12e %.12e %.12e\n", (double)t * dt_seconds, frequencies->values_hz[k], mean) < 0) {
		success = false;
	    }
	}

	if (success && fputc('\n', file) == EOF) {
	    success = false;
	}
    }

    if (!success) {
	error_message("Failed writing mean values.");
    }

    if (output_close(file) != 0) {
	success = false;
    }

    return success ? 0 : -1;
}




















