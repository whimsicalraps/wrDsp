#pragma once

#include <stdint.h>

typedef struct func_gen{
	int8_t    go;
	float     id;
	float     rate;
	uint8_t   tr_state;
	float     fm_ix;
	int8_t    loop;	// nb: -1 is infinite loop. +ve is number of retrigs left.

	float     r_up;
	float     r_down;
} func_gen_t;

// Initialization
void function_init( func_gen_t* self, int8_t loop );

// Param functions
void function_trig( func_gen_t* self, uint8_t state );
void function_rate( func_gen_t* self, float rate );

// Audio rate process
void function_ramp( func_gen_t* self, float skew );
void function_ramp_v( uint16_t b_size, float ctrl_rate, float* audio_rate, float* ramp_up, float* ramp_down );

float function_step( func_gen_t* self, float fm_in );
// void function_v( trig_state[j], process[0], process[j] );
void function_v( func_gen_t* self, uint16_t b_size, float* r_up, float* r_dn, float* fm_in, float* out );


float function_lookup( float id );
