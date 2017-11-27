#pragma once

#include "dsp_cli.h"
#include "wrFilter.h" // this wraps the main DSP lib

module_t* graph_lp1_init( void );
void g_lp1_process( module_t* box );
void g_lp1_set_coeff( void* f, float c );
float g_lp1_get_coeff(void* f);
