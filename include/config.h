#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "trajectory.h"
#include "frequencyGrid.h"
#include "analysis.h"

typedef struct {
    const char* input_path;
    const char* output_directory;

    SignalComponent component;
    double time_to_seconds;

    FrequencyConfig frequencies;
    MorletConfig morlet;

    bool automatic_padding;
    size_t padding_samples;

    bool write_each_atom;
} AppConfig;


void app_config_defaults(AppConfig* config);

int app_config_parse(
	AppConfig* config,
	int argc,
	char** argv
);

int app_config_validate(const AppConfig* config);

void app_config_print_usage(const char* program);






#endif // CONFIG_H
