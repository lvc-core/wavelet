#include <stdlib.h>
#include <string.h>



#include "analysis.h"
#include "app.h"
#include "config.h"
#include "frequencyGrid.h"
#include "results.h"
#include "trajectory.h"



int main(int argc, char** argv){
    if (argc == 2 &&
	    (strcmp(argv[1], "--help") == 0 ||
	     strcmp(argv[1], "-h") == 0)) {
	app_config_print_usage(argv[0]);
	return EXIT_SUCCESS;
    }

    AppConfig config;
    app_config_defaults(&config);

    if (app_config_parse(&config, argc, argv) != 0) {
	app_config_print_usage(argv[0]);
	return EXIT_FAILURE;
    }

    if (app_config_validate(&config) != 0) {
	return EXIT_FAILURE;
    }
    
    /*
     * resources are initialized so that destroy() can be safely
     * used after partial initialization
     */
    SignalData data = {0};
    FrequencyGrid frequencies = {0};
    AnalysisWorkspace workspace = {0};
    ResultAccumulator results = {0};

    const int status = app_run(
	    &config,
	    &data,
	    &frequencies,
	    &workspace,
	    &results
	    );

    result_accumulator_destroy(&results);
    analysis_workspace_destroy(&workspace);
    frequency_grid_destroy(&frequencies);
    signal_data_destroy(&data);

    if (status != 0) {
	fprintf(stderr, "\n Analysis did not run successfully.\n");
	return EXIT_FAILURE;
    }

    fprintf(stderr, "Done. Mean values: %s/results.dat\n", config.output_directory);

    return EXIT_SUCCESS;
}














