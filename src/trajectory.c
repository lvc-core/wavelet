#include <float.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "common.h"
#include "trajectory.h"

/*
 * static functions
 */

static double select_signal_component(
	double x,
	double y,
	double z,
	SignalComponent component)
{
    switch (component) {
	case SIGNAL_X:
	    return x;
	case SIGNAL_Y:
	    return y;
	case SIGNAL_Z:
	    return z;
	case SIGNAL_MAGNITUDE:
	    return hypot(hypot(x, y), z);
    }

    return NAN;
}


static bool parse_timestamp(
	const char* comment,
	double time_to_seconds,
	double* time_seconds)
{
    const char* position = strstr(comment, "t=");

    if (position == NULL) {
	return false;
    }

    position += 2;

    char* end = NULL;

    errno = 0;
    double value = strtod(position, &end);

    if (position == end || errno != 0 || !isfinite(value)) {
	return false;
    }

    /* 
     * after the numeric value spaces, units and commas are allowed
     */
    if (*end != '\0' && *end != ',' && !isspace((unsigned char)*end)) {
	return false;
    }

    value *= time_to_seconds;

    if (!isfinite(value)) {
	return false;
    }

    *time_seconds = value;
    return true;
}


static int reserve_samples(double** samples, size_t* capacity, size_t required){
    if (required <= *capacity) {
	return 0;
    }

    const size_t max_elements = SIZE_MAX / sizeof(**samples);

    if (required > max_elements) {
	return error_message("Trajectory data is too large.");
    }

    size_t new_capacity = (*capacity == 0 ? 1024 : *capacity);

    if (new_capacity > max_elements) {
	new_capacity = required;
    }

    while (new_capacity < required) {
	if (new_capacity > max_elements / 2) {
	    new_capacity = required;
	    break;
	}

	new_capacity *= 2;
    }

    double* new_samples = realloc(*samples, new_capacity * sizeof(**samples));

    if (new_samples == NULL) {
	return error_message("Not enough memory for trajectory data.");
    }

    *samples = new_samples;
    *capacity = new_capacity;

    return 0;
}



int xyz_read_signals(
	const char *path,
	SignalComponent component,
	double time_to_seconds,
	SignalData *output)
{
    FILE* file = fopen(path, "r");

    if (file == NULL) {
	return file_error("opening", path);
    }

    char* line = NULL;
    size_t line_capacity = 0;
    size_t sample_capacity = 0;

    double previous_time = 0.0;
    bool success = true;

    output->layout = LAYOUT_TIME_MAJOR;

    while (success) {
	ssize_t length = getline(&line, &line_capacity, file);	

	if (length < 0) {
	    if (!feof(file)) {
		error_message("Failed to read xyz file.");
		success = false;
	    }

	    break;
	}

	size_t frame_atom_count = 0;

	if (!parse_size(line, &frame_atom_count) || frame_atom_count == 0) {
	    error_message("Non-valid number of atoms in xyz file.");
	    success = false;
	    break;
	}

	if (output->timestep_count == 0) {
	    output->atom_count = frame_atom_count;
	} else if (frame_atom_count != output->atom_count) {
	   error_message("Number of atoms is not constant over time!");
	   success = false;
	   break;
	}

	if (getline(&line, &line_capacity, file) < 0) {
	    error_message("Missing header line in xyz file!");
	    success = false;
	    break;
	}

	double current_time = 0.0;

	if (!parse_timestamp(line, time_to_seconds, &current_time)) {
	    error_message("Invalid or missing timestamp t=XX in xyz file.");
	    success = false;
	    break;
	}

	if (output->timestep_count > 0) {
	    const double delta = current_time - previous_time;

	    if (!(delta > 0.0) || !isfinite(delta)) {
		error_message("Invalid timestep between two frames!");
		success = false;
		break;
	    }

	    if (output->timestep_count == 1) {
		output->dt_seconds = delta;
	    } else {
		const double tolerance =
		    1e-6 * output->dt_seconds +
		    32.0 * DBL_EPSILON * fmax(fabs(current_time), fabs(previous_time));

		if (fabs(delta - output->dt_seconds) > tolerance) {
		    error_message("Unevenly distributed time steps in input file");
		    success = false;
		    break;
		}
	    }
	}

	if (output->timestep_count == SIZE_MAX) {
	    error_message("Too many time steps.");
	    success = false;
	    break;
	}

	size_t required_samples = 0;

	if (!checked_multiply(output->timestep_count + 1, output->atom_count, &required_samples)) {
	    error_message("Size overflow at requested samples.");
	    success = false;
	    break;
	}

	if (reserve_samples(&output->samples, &sample_capacity, required_samples) != 0) {
	    success = 0;
	    break;
	}

	const size_t frame_offset = output->timestep_count * output->atom_count;

	for (size_t atom = 0; atom < output->atom_count && success; ++atom) {
	    if (getline(&line, &line_capacity, file) < 0) {
		error_message("Incomplete XYZ-frame.");
		success = false;
		break;
	    }

	    char symbol[64];
	    double x = 0.0;
	    double y = 0.0;
	    double z = 0.0;

	    if (sscanf(line, "%63s %lf %lf %lf", symbol, &x, &y, &z) != 4 ||
		    !isfinite(x) ||
		    !isfinite(y) ||
		    !isfinite(z)) {

		error_message("Invalid atom row.");
		success = false;
		break;
	    }

	    const double value = select_signal_component(x, y, z, component);

	    if (!isfinite(value)) {
		error_message("Encountered infinite trajectory value."); 
		success = false;
		break;
	    }

	    output->samples[frame_offset + atom] = value;
	}

	if (success) {
	    previous_time = current_time;
	    ++output->timestep_count;
	}
    }

    free(line);

    if (fclose(file) != 0) {
	file_error("closing", path);
	success = false;
    }

    if (success && output->timestep_count < 2) {
	error_message("At least two complete frames needed.");
	success = false;
    }

    return success ? 0 : -1;
}


int signal_data_make_atom_major(SignalData* data){
    if (data->layout == LAYOUT_ATOM_MAJOR) {
	return 0;
    }

    size_t element_count = 0;
    size_t byte_count = 0;

    if (!checked_multiply(data->atom_count, data->timestep_count, &element_count) ||
	    !checked_multiply(element_count, sizeof(*data->samples), &byte_count)) {
	return error_message("Size overflow while rearranging.");
    }

    double* reordered = malloc(byte_count);

    if (reordered == NULL) {
	return error_message("Not enough memory for rearranging data.");
    }

    for (size_t t = 0; t < data->timestep_count; ++t) {
	for (size_t atom = 0; atom < data->atom_count; ++atom) {
	    reordered[atom * data->timestep_count + t] = data->samples[t * data->atom_count + atom];
	}
    }

    free(data->samples);

    data->samples = reordered;
    data->layout = LAYOUT_ATOM_MAJOR;

    return 0;
}


const double* signal_data_atom(
	const SignalData* data,
	size_t atom_index)
{
    if (data->layout != LAYOUT_ATOM_MAJOR || atom_index >= data->atom_count) {
	return NULL;
    }

    return &data->samples[atom_index * data->timestep_count];
}

void signal_data_destroy(SignalData* data){
    free(data->samples);
    *data = (SignalData) {0};
}

















