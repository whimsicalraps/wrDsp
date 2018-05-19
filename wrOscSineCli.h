#pragma once

#include "wrOscSine.h" // this wraps the main DSP lib
#include "wrCliHelpers.h"

module_t* graph_osc_sine_init( int b_size );
void g_osc_sine_process( module_t* box, int b_size );
void g_osc_( float* out );
void g_osc_sine_set_pitch( void* f, float p );
float g_osc_sine_get_pitch(void* f);
