#pragma once

#include "wrFilter.h" // this wraps the main DSP lib
#include "wrCliHelpers.h"

module_t* graph_lp1_init( int b_size );
void g_lp1_process( module_t* box, int b_size );
void g_lp1_set_coeff( void* f, float c );
float g_lp1_get_coeff(void* f);
