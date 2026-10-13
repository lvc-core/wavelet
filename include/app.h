#ifndef APP_H
#define APP_H

#include "analysis.h"
#include "trajectory.h"
#include "config.h"
#include "frequencyGrid.h"
#include "results.h"


int app_run(
	const AppConfig* config,
	SignalData* data,
	FrequencyGrid* frequencies,
	AnalysisWorkspace* workspace,
	ResultAccumulator* results
);



#endif // APP_H
