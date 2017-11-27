#pragma once

#include <stdint.h>

#define LOG_VOL_CONST 	((float)(0.1/1.1))
#define HPF_COEFF		((float)(0.997))

#define LPGATE_HPF_OFF  0
#define LPGATE_HPF_ON	1
#define LPGATE_VCA  	0
#define LPGATE_FILTER	1

typedef struct lpgate{
	uint8_t hpf;
	uint8_t filter;
    float   level;
	void (*lpgate_fnptr)( struct lpgate* self
		                , float* level
		                , float* audio
		                , float* out );
	uint16_t b_size;

	float prev_lo;
	float prev_hi;
} lpgate_t;

void lpgate_init( lpgate_t* self, uint8_t hpf, uint8_t filter, uint16_t b_size );
void lpgate_set_level( lpgate_t* self, float level );
void lpgate_hpf_mode( lpgate_t* self, uint8_t hpf );
void lpgate_filter_mode( lpgate_t* self, uint8_t filter );
	// level input expects (0-1)
float lpgate_step( lpgate_t* self, float level, float in );
void lpgate_v( lpgate_t* self, float* level, float* audio, float* out );
