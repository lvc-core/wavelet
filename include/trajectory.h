#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <stddef.h>

typedef enum {
    SIGNAL_X,
    SIGNAL_Y,
    SIGNAL_Z,
    SIGNAL_MAGNITUDE
} SignalComponent;

typedef enum {
    LAYOUT_TIME_MAJOR,
    LAYOUT_ATOM_MAJOR
} DataLayout;


typedef struct {
    size_t atom_count;
    size_t timestep_count;

    double dt_seconds;
    
    DataLayout layout;
    double* samples;
} SignalData;


int xyz_read_signals(
	const char* path,
	SignalComponent component,
	double time_to_seconds,
	SignalData* output
);


int signal_data_make_atom_major(SignalData* data);

const double* signal_data_atom(
	const SignalData* data,
	size_t atom_index
);

void signal_data_destroy(SignalData* data);










#endif // TRAJECTORY_H
