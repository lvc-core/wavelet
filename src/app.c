#include <math.h>

#include "analysis.h"
#include "trajectory.h"
#include "output.h"
#include "config.h"
#include "common.h"
#include "frequencyGrid.h"
#include "results.h"
#include "app.h"

/*
 * static function
 */
static int analyze_all_atoms(
	const SignalData* data,
	const FrequencyGrid* frequencies,
	const MorletConfig* morlet,
	AnalysisWorkspace* workspace,
	ResultAccumulator* results,
	const char* output_directory,
	bool write_each_atom)
{
    for (size_t atom = 0; atom < data->atom_count; ++atom) {
	fprintf(stderr, "\rAnalyzing atom %zu / %zu", atom + 1, data->atom_count);

	const double* samples = signal_data_atom(data, atom);

	if (analysis_prepare_signal(workspace, samples, data->timestep_count) != 0) {
	    return -1;
	}

	FILE* atom_file = NULL;

	if (write_each_atom) {
	    if (output_open_atom(output_directory, atom, &atom_file) != 0) {
		return -1;
	    }
	}

	bool success = true;

	for (size_t k = 0; k < frequencies->count && success; ++k) {
	    const double frequency = frequencies->values_hz[k];

	    if (analysis_prepare_wavelet(workspace, morlet, frequency,data->dt_seconds) != 0) {
		success = false;
	    }

	    if (success && analysis_compute_magnitudes(workspace) != 0) {
		success = false;
	    }

	    if (success && result_accumulator_add(results, k, workspace->magnitudes) != 0) {
		success = false;
	    }

	    if (success && atom_file != NULL) {
		if (output_write_frequency_block(atom_file, frequency, data->dt_seconds, workspace->magnitudes, data->timestep_count) != 0) {
		    success = false;
		}
	    }
	}

	if (atom_file != NULL && output_close(atom_file) != 0) {
	    success = false;
	}

	if (!success) {
	    return -1;
	}
    }

    fprintf(stderr, "\n");

    return 0;
}




int app_run(
	const AppConfig* config,
	SignalData* data,
	FrequencyGrid* frequencies,
	AnalysisWorkspace* workspace,
	ResultAccumulator* results)
{
    /* 1. Read in file */
    if (xyz_read_signals(
		config->input_path,
		config->component,
		config->time_to_seconds,
		data) != 0) {
	return -1;
    }

    fprintf(
	    stderr,
	    "File contains: %zu atoms, %zu timesteps, dt=%.10g s\n",
	    data->atom_count,
	    data->timestep_count,
	    data->dt_seconds
	    );

    /* 2. Rearrange data (time-major to atom-major) */
    if (signal_data_make_atom_major(data) != 0) {
	return -1;
    }

    /* 3. Create frequency grid */
    if (frequency_grid_create(&config->frequencies, frequencies) != 0) {
	return -1;
    }

    /* 4. Check sampling and frequency range */
    if (analysis_validate_sampling(data, frequencies) != 0) {
	return -1;
    }

    /* 5. Determine padding */
    size_t padding = config->padding_samples;

    if (config->automatic_padding) {
	padding = data->timestep_count / 2 + data->timestep_count % 2;
    }

    /*
     * values chosen based on broadest gaussian.
     * using 6 standard deviations as a default
     */
    const double pi = acos(-1.0);

    const double largest_scale = config->morlet.omega0 / (2.0 * pi * (frequencies->values_hz[0] * data->dt_seconds));

    const double recommended_padding = 6.0 * largest_scale * config->morlet.width;

    if ((double)padding < recommended_padding) {
	fprintf(
		stderr,
		ORANGE "Warning: Padding=%zu is smaller than the 6-sigma width of around %.0f samples for the lowest frequency!\n" RESET,
		padding,
		ceil(recommended_padding)
	       );
    }

    /* 6. Create FFT workspace */
    if (analysis_workspace_init(workspace, data->timestep_count, padding) != 0) {
	return -1;
    }

    /* 7. Create summation matrix */
    if (result_accumulator_init(results,frequencies->count, data->timestep_count) != 0) {
	return -1;
    }

    /* 8. Prepare output directory */
    if (output_directory_prepare(config->output_directory) != 0) {
	return -1;
    }

    fprintf(stderr, "Frequencies: %zu, FFT-length: %zu, Padding per side: %zu\n",
	    frequencies->count,
	    workspace->fft_size,
	    workspace->padding_samples
	   );

    /* 9. Analyze atoms */
    if (analyze_all_atoms(
		data,
		frequencies,
		&config->morlet,
		workspace,
		results,
		config->output_directory,
		config->write_each_atom) != 0) {
	return -1;
    }

    /* 10. Calculate mean values */
    if (output_write_mean(
	    config->output_directory,
	    results,
	    frequencies,
	    data->dt_seconds,
	    data->atom_count
	    ) != 0) {
	return -1;
    }

    return 0;
}






















