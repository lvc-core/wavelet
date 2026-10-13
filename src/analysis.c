#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <float.h>

#include "common.h"
#include "analysis.h"

/*
 * static functions
 */

static int fft_transform_init(
	FftTransform* transform,
	size_t fft_size,
	int direction)
{
    size_t bytes = 0;

    if (!checked_multiply(fft_size, sizeof(*transform->input), &bytes)) {
	return error_message("FFT buffer is too large.");
    }

    transform->input = fftw_malloc(bytes);
    transform->output = fftw_malloc(bytes);

    if (transform->input == NULL || transform->output == NULL) {
	return error_message("FFT buffer could not be allocated.");
    }

    transform->plan = fftw_plan_dft_1d((int)fft_size, transform->input, transform->output, direction, FFTW_MEASURE);

    if (transform->plan == NULL) {
	return error_message("FFT plan could not be created.");
    }

    return 0;
}

static void fft_transform_destroy(FftTransform* transform){
    if (transform->plan != NULL) {
	fftw_destroy_plan(transform->plan);
    }

    if (transform->input != NULL) {
	fftw_free(transform->input);
    }

    if (transform->output != NULL) {
	fftw_free(transform->output);
    }

    *transform = (FftTransform) {0};
}

static void morlet_fill_fft_input(
	fftw_complex* output,
	size_t fft_size,
	double scale,
	double width,
	double omega0,
	double normalization)
{
    for (size_t i = 0; i < fft_size; ++i) {
	const double lag = (i <= fft_size / 2 ? (double)i : (double)i - (double)fft_size);

	const double tau = lag / scale;
	const double normalized_lag = tau / width;

	/*
	 * ignore values greater than 12 standard deviations
	 */
	if (fabs(normalized_lag) > 12.0) {
	    output[i][0] = 0.0;
	    output[i][1] = 0.0;
	} else {
	    const double envelope = normalization * exp(-0.5 * normalized_lag * normalized_lag);

	    const double phase = omega0 * tau;

	    output[i][0] = envelope * cos(phase);
	    output[i][1] = envelope * sin(phase);
	}
    }
}




int analysis_validate_sampling(
	const SignalData* data,
	const FrequencyGrid* frequencies)
{
    if (data->timestep_count < 2 ||
	    !(data->dt_seconds > 0.0) ||
	    !isfinite(data->dt_seconds)) {
	return error_message("Invalid sampling.");
    }

    const double nyquist_hz = 0.5 / data->dt_seconds;

    if (frequencies->values_hz[frequencies->count - 1] >= nyquist_hz) {
	fprintf(stderr, "Error: FMAX has to be lower than the Nyguistfrequency %.10g Hz.\n", nyquist_hz);

	return -1;
    }

    return 0;
}


int analysis_workspace_init(
	AnalysisWorkspace* workspace,
	size_t sample_count,
	size_t padding_samples)
{
    if (sample_count == 0 || padding_samples > (SIZE_MAX - sample_count) / 2) {
	return error_message("Invalid FFT size.");
    }

    const size_t fft_size = sample_count + 2 * padding_samples;

    if (fft_size > (size_t)INT_MAX) {
	return error_message("FFT length greater than limit of fftw_plan_dft_1d.");
    }

    workspace->sample_count = sample_count;
    workspace->padding_samples = padding_samples;
    workspace->fft_size = fft_size;

    if (fft_transform_init(&workspace->signal_forward, fft_size, FFTW_FORWARD) != 0) {
	return -1;
    }

    if (fft_transform_init(&workspace->wavelet_forward, fft_size, FFTW_FORWARD) != 0) {
	return -1;
    }

    if (fft_transform_init(&workspace->inverse, fft_size, FFTW_BACKWARD) != 0) {
	return -1;
    }

    size_t bytes = 0;

    if (!checked_multiply(sample_count, sizeof(*workspace->magnitudes), &bytes)) {
	return error_message("result block is too large.");
    }

    workspace->magnitudes = malloc(bytes);

    if (workspace->magnitudes == NULL) {
	return error_message("Could not allocate memory for result block.");
    }

    return 0;
}


int analysis_prepare_signal(
	AnalysisWorkspace* workspace,
	const double* samples,
	size_t sample_count)
{
    if (samples == NULL || sample_count != workspace->sample_count) {
	return error_message("Signal not compatible with FFT workspace.");
    }

    for (size_t i = 0; i < workspace->fft_size; ++i) {
	workspace->signal_forward.input[i][0] = 0.0;
	workspace->signal_forward.input[i][1] = 0.0;
    }

    for (size_t t = 0; t < sample_count; ++t) {
	workspace->signal_forward.input[workspace->padding_samples + t][0] = samples[t];
    }

    fftw_execute(workspace->signal_forward.plan);

    return 0;
}


int analysis_prepare_wavelet(
	AnalysisWorkspace* workspace,
	const MorletConfig* config,
	double frequency_hz,
	double dt_seconds)
{
    const double pi = acos(-1.0);

    const double angular_frequency = 2.0 * pi * (frequency_hz * dt_seconds);

    const double scale = config->omega0 / angular_frequency;
    const double sigma_samples = scale * config->width;
    
    const double normalization = 1.0 / (pow(pi, 0.25) * sqrt(sigma_samples));

    /* 
     * omega0 * width limits the phase within the gaussian
     */
    const double phase_extent = config->omega0 * config->width;

    if (!(scale > 0.0) ||
	    !isfinite(scale) ||
	    !(sigma_samples > 0.0) ||
	    !isfinite(sigma_samples) ||
	    !(normalization > 0.0) ||
	    !isfinite(normalization) ||
	    !isfinite(phase_extent) ||
	    phase_extent > DBL_MAX / 16.0) {
	return error_message("Invalid numerical wavelet parameters.");
    }

    morlet_fill_fft_input(workspace->wavelet_forward.input, workspace->fft_size, scale, config->width, config->omega0, normalization);

    fftw_execute(workspace->wavelet_forward.plan);

    return 0;
}


/*
 * Calculates:
 *
 * IFFT(FFT(signal) * conjugate(FFT(wavelet)))
 *
 * then:
 * - FFT normalization
 * - remove padding
 * - abs of complex coefficients
 */
int analysis_compute_magnitudes(AnalysisWorkspace* workspace){
    for (size_t i = 0; i < workspace->fft_size; ++i) {
	const double a = workspace->signal_forward.output[i][0];
	const double b = workspace->signal_forward.output[i][1];

	const double c = workspace->wavelet_forward.output[i][0];
	const double d = workspace->wavelet_forward.output[i][1];

	workspace->inverse.input[i][0] = a * c + b * d;
	workspace->inverse.input[i][1] = b * c - a * d;
    }

    fftw_execute(workspace->inverse.plan);

    const double normalization = 1.0 / (double)workspace->fft_size;

    for (size_t t = 0; t < workspace->sample_count; ++t) {
	const size_t index = workspace->padding_samples + t;

	const double real_part = workspace->inverse.output[index][0] * normalization;

	const double imaginary_part = workspace->inverse.output[index][1] * normalization;

	const double value = hypot(real_part, imaginary_part);

	if (!isfinite(value)) {
	    return error_message("Non-finite result in waveletanalysis.");
	}

	workspace->magnitudes[t] = value;
    }

    return 0;
}


void analysis_workspace_destroy(AnalysisWorkspace* workspace){
    fft_transform_destroy(&workspace->signal_forward);
    fft_transform_destroy(&workspace->wavelet_forward);
    fft_transform_destroy(&workspace->inverse);

    free(workspace->magnitudes);

    *workspace = (AnalysisWorkspace) {0};
}

















