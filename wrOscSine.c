#include "wrOscSine.h"
#include "wrOscSineLUT.h"

#include <math.h>
#include <wrMath.h>

// initialization
void osc_sine_init( osc_sine_t* self )
{
	self->rate = 1.0;
	self->id = 0.0;
	self->zero_x = 1;
}

// input fns
// expect 0-1, but can accept through -1 for TZ effects
void osc_sine_time( osc_sine_t* self, float time_ )
{
	// 1.0 = sample_rate
	// 0.0 = stopped
	// -1. = inverse SR
	self->rate = lim_f_n1_1( time_ );
}

void osc_sine_reset( osc_sine_t* self )
{
	self->id = 0;
	self->zero_x = 1;
}

// status
int8_t osc_sine_get_zc( osc_sine_t* self )
{
	return (self->zero_x);
}

// nb: incrementers run 0-1 w/ zero cross at 0.5

// single-sample
float osc_sine_step( osc_sine_t* self, float fm )
{
	float odd = self->id;
	self->id += self->rate + fm;

	// edge & zero-cross detection
	if( self->id >= 2.0 ){
		self->id -= 2.0;
		self->zero_x = 1; // ZERO
	} else if( (self->id >= 1.0) && (odd < 1.0) ){
		self->zero_x = -1; // PEAK/TROUGH
	} else if( self->id < 0.0 ){
		self->id += 2.0;
		self->zero_x = 1; // ZERO
	} else {
		self->zero_x = 0;
	}

	// lookup table w/ linear interpolation
	float fbase = LUT_HALF * self->id;
	uint16_t base = (uint16_t)fbase;
	float mix = fbase - (float)base;
	float lut = sine_lut[base];
	return (lut + mix * (sine_lut[base + 1] - lut));
}

void osc_sine_process_v( osc_sine_t* self, uint16_t b_size, float* buf_run, float* out )
{
	float* run2 = buf_run;
	float* out2 = out;

	float odd;
	float fbase;
	uint16_t base;
	float mix;
	float lut;

	for( uint16_t i=0; i<b_size; i++ ){
		odd = self->id;
		self->id += self->rate + (*run2++);

		// edge & zero-cross detection
		if( self->id >= 2.0 ){
			self->id -= 2.0;
			self->zero_x = i+1;
		} else if( (self->id >= 1.0) && (odd < 1.0) ){
			self->zero_x = -(i+1);
		} else if( self->id < 0 ){
			self->id += 2.0;
			self->zero_x = 1;
		} else {
			self->zero_x = 0;
		}

		// lookup table w/ linear interpolation
		fbase = LUT_HALF * self->id;
		base = (uint16_t)fbase;
		mix = fbase - (float)base;
		lut = sine_lut[base];
		*out2++ = lut + mix * (sine_lut[base + 1] - lut);
	}
}
