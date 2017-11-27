#pragma once

#include "dsp_cli.h"
#include "wrOscSine.h" // this wraps the main DSP lib

module_t* graph_osc_sine_init( void );
void g_osc_sine_process( module_t* box );
void g_osc_sine_set_pitch( void* f, float p );
float g_osc_sine_get_pitch(void* f);
