#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stddef.h>
#include <fftw3.h>

#include "frequencyGrid.h"
#include "trajectory.h"

typedef struct {
    double omega0;
    double width;
} MorletConfig;

typedef struct {
    fftw_complex* input;
    fftw_complex* output;
    fftw_plan plan;
} FftTransform;

typedef struct {
    size_t sample_count;
    size_t padding_samples;
    size_t fft_size;

    FftTransform signal_forward;
    FftTransform wavelet_forward;
    FftTransform inverse;

    double* magnitudes;
} AnalysisWorkspace;


int analysis_validate_sampling(
	const SignalData* data,
	const FrequencyGrid* frequencies
);


int analysis_workspace_init(
	AnalysisWorkspace* workspace,
	size_t sample_count,
	size_t padding_samples
);

int analysis_prepare_signal(
	AnalysisWorkspace* workspace,
	const double* samples,
	size_t sample_count
);

int analysis_prepare_wavelet(
	AnalysisWorkspace* workspace,
	const MorletConfig* config,
	double frequency_hz,
	double dt_seconds
);

int analysis_compute_magnitudes(
	AnalysisWorkspace* workspace
);

void analysis_workspace_destroy(
	AnalysisWorkspace* workspace
);







#endif // ANALYSIS_H










