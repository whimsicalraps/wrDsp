#pragma once

#include <stdint.h>

#define LOG_VOL_CONST 	(0.1/1.1)
#define HPF_COEFF		(0.997)

typedef struct lpgate{
	uint8_t hpf;
	uint8_t filter;
	uint16_t b_size;

	float prev_lo;
	float prev_hi;
} lpgate_t;

void lpgate_init( lpgate_t* self, uint8_t hpf, uint8_t filter, uint16_t b_size );
	// level input expects (0-1)
float lpgate_step( lpgate_t* self, float level, float in );
void lpgate_v( lpgate_t* self, float* level, float* audio, float* out );
