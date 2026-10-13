#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#include "common.h"
#include "frequencyGrid.h"
#include "trajectory.h"
#include "config.h"

/*
 * static functions
 */
static bool parse_double(
	const char* text,
	double* output)
{
    char* end = NULL;

    errno = 0;
    double value = strtod(text, &end);

    if (text == end || errno != 0 || !isfinite(value)) {
	return false;
    }

    while (isspace((unsigned char)*end)) {
	++end;
    }

    if (*end != '\0') {
	return false;
    }

    *output = value;
    return true;
}



void app_config_defaults(AppConfig* config){
    *config = (AppConfig) {
	.component = SIGNAL_MAGNITUDE,
	    .time_to_seconds = 1.0,

	    .frequencies = {
		.spacing = FREQUENCY_LINEAR
	    },

	    .morlet = {
		.omega0 = 6.0,
		.width = 1.0
	    },

	    .automatic_padding = true,
	    .padding_samples = 0,
	    .write_each_atom = false
    };
}


int app_config_parse(
	AppConfig* config,
	int argc,
	char** argv)
{
    if (argc < 8) {
	return error_message("Not enough input arguments.");
    }

    config->input_path = argv[1];
    config->output_directory = argv[7];

    if (!parse_double(argv[2], &config->morlet.omega0) ||
	    !parse_double(argv[3], &config->frequencies.min_hz) ||
	    !parse_double(argv[4], &config->frequencies.max_hz) ||
	    !parse_size(argv[5], &config->frequencies.count) ||
	    !parse_double(argv[6], &config->time_to_seconds)) {
	return error_message("Invalid input arguments.");
    }

    for (int i = 8; i < argc; i++) {
	const char* option = argv[i];

	if (strcmp(option, "--log") == 0) {
	    config->frequencies.spacing = FREQUENCY_LOGARITHMIC;
	} else if (strcmp(option, "--write-atoms") == 0) {
	    config->write_each_atom = true;
	} else if (strcmp(option, "--padding") == 0) {
	    if (i + 1 >= argc) {
		return error_message("--padding requires a value.");
	    }

	    if (!parse_size(argv[++i], &config->padding_samples)) {
		return error_message("Invalid padding.");
	    }

	    config->automatic_padding = false;
	} else if (strcmp(option, "--width") == 0) {
	    if (i + 1 >= argc) {
		return error_message("--width requires a value.");
	    }

	    if (!parse_double(argv[++i], &config->morlet.width)) {
		return error_message("Invalid wavelet width.");
	    }
	} else if (strcmp(option, "--component") == 0) {
	    if (i + 1 >= argc) {
		return error_message("--component requires a value.");
	    }

	    const char* component = argv[++i];

	    if (strcmp(component, "x") == 0) {
		config->component = SIGNAL_X;
	    } else if (strcmp(component, "y") == 0) {
		config->component = SIGNAL_Y;
	    } else if (strcmp(component, "z") == 0) {
		config->component = SIGNAL_Z;
	    } else if (strcmp(component, "norm") == 0) {
		config->component = SIGNAL_MAGNITUDE;
	    } else {
		return error_message("component has to be x, y, z or norm");
	    }
	} else {
	    fprintf(stderr, "Unknown option: %s\n", option);
	    return -1;
	}
    }

    return 0;
}


int app_config_validate(const AppConfig* config){
    if (config->input_path == NULL ||
	    config->input_path[0] == '\0' ||
	    config->output_directory == NULL ||
	    config->output_directory[0] == '\0') {
	return error_message("Input- and/or output path missing.");
    }

    if (config->frequencies.count < 2) {
	return error_message("At least two frequencies required.");
    }

    if (!(config->frequencies.min_hz > 0.0) || !(config->frequencies.max_hz > config->frequencies.min_hz)) {
	return error_message("Requirement: 0 < FMIN < FMAX.");
    }

    if (!(config->morlet.omega0 > 0.0) ||
	    !(config->morlet.width > 0.0) ||
	    !(config->time_to_seconds > 0.0)) {
	return error_message("omega0, wavelet-width and timescaling has to be positive.");
    }

    return 0;
}


void app_config_print_usage(const char* program){
    fprintf(
	    stderr,
	    "Usage:\n"
	    "	%s INPUT OMEGA0 FMIN_HZ FMAX_HZ NFREQ "
	    "TIME_TO_SECONDS OUTPUT_DIR [options]\n"
	    "\n"
	    "options:\n"
	    "	--log			logarithmic frequency grid\n"
	    "	--component x		x-component\n"
	    "	--component y		y-component\n"
	    "	--component z		z-component\n"
	    "	--component norm	absolute value of vector (default)\n"
	    "	-- width W		Morlet width, default: 1\n"
	    "	--padding N		Zerosamples per side\n"
	    "	--write-atoms		Write results for each atom\n"
	    "\n"
	    "Without padding ceil(N_timesteps / 2) samples"
	    "per side will be used\n"
	    "\n"
	    "Example for timestams in seconds:\n"
	    "	%s input.xyz 6 0.1 20 200 1 results --log\n"
	    "\n"
	    "Example for timestams in femtoseconds:\n"
	    "	%s input.xyz 6 1e12 2e13 200 1e-15 results --log\n",
	program,
	program,
	program);
}

















