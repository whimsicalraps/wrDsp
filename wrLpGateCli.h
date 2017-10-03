#pragma once

#include "dsp_cli.h"
#include "wrLpGate.h" // this wraps the main DSP lib

#define BLOCK_SIZE 256 // obvs need to make this programatic!

module_t* graph_lpgate_init( void );
