#pragma once

#include <stdint.h>

// verbose names for switch statement
#define SQ_LOG   0
#define LOG_TRI  1
#define TRI_EXP  2
#define EXP_SINE 3

extern const float log_lut[];

typedef struct shaper{
	uint16_t b_size;	// size of audio vector
	uint16_t chans;		// channel count for parallelization
	uint8_t* zone;		// vector of interp regions
	float* coeff;		// vector of interp coefficients
} shaper_t;

int8_t shaper_init( shaper_t* self, uint16_t b_size, uint16_t channels );
void shaper_prep( shaper_t* self, float control );
void shaper_prep_v( shaper_t* self, float* audio, float control );
float shaper_apply_old( shaper_t* self
	              , float     input
	              , uint16_t  samp
	              );
float shaper_apply_fold( shaper_t* self
	              , float     input
	              , uint16_t  samp
	              );
float shaper_apply( shaper_t* self
	              , float     input
	              , uint16_t  samp
	              );
void shaper_apply_v( shaper_t* self, float* input, float* output );
void shaper_v_p( shaper_t* self, float* inputs, float* outputs );

float shaper_rev_lookup( shaper_t* self, float state );
