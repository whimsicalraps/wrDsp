#pragma once

#include "wrLpGate.h" // this wraps the main DSP lib
#include "wrCliHelpers.h"

module_t* graph_lpgate_init( void );
void g_lpgate_process( module_t* box );
void g_lpg_set_level( void* self, float l );
float g_lpg_get_level( void* self );
void g_lpg_set_filter( void* self, float f );
float g_lpg_get_filter( void* self );
void g_lpg_set_hpf( void* self, float h );
float g_lpg_get_hpf( void* self );
