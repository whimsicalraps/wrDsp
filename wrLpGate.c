#include "wrLpGate.h"

#include "../STM32F4-workarea/Project/JF3-test/lib/debug.h"

// private declarations
void _lpgate_mode_select( lpgate_t* self );
// vector modes
void lpgate_v_filt(     lpgate_t* self, float* level, float* audio, float* out );
void lpgate_v_gate(     lpgate_t* self, float* level, float* audio, float* out );
void lpgate_v_filt_hpf( lpgate_t* self, float* level, float* audio, float* out );
void lpgate_v_gate_hpf( lpgate_t* self, float* level, float* audio, float* out );

void lpgate_init( lpgate_t* self, uint8_t hpf, uint8_t filter, uint16_t b_size )
{
	self->hpf = hpf;
	self->filter = filter;
	_lpgate_mode_select(self);

	self->b_size = b_size;

	self->prev_lo = 0;
	self->prev_hi = 0;
}

void lpgate_hpf_mode( lpgate_t* self, uint8_t hpf )
{
	self->hpf = !!hpf; // force 0/1
	_lpgate_mode_select(self);
}
void lpgate_filter_mode( lpgate_t* self, uint8_t filter )
{
	self->filter = !!filter; // force 0/1
	_lpgate_mode_select(self);
}
void _lpgate_mode_select( lpgate_t* self )
{
	// filter, hpf
	static void (*fnptr[2][2])() =
		{ { lpgate_v_gate
		  , lpgate_v_gate_hpf }
		, { lpgate_v_filt
		  , lpgate_v_filt_hpf }
		};
	self->lpgate_fnptr = fnptr[ self->filter ][ self->hpf ];
}

float lpgate_step( lpgate_t* self, float level, float in )
{
	float out_lo, out_hi;

	if(self->filter){ // BOTH MODE (LPF -> VOL)
		out_lo = self->prev_lo +
					(level * 
					(in - self->prev_lo));
		out_lo *= level/(0.1f + level) + LOG_VOL_CONST;
	} else {
		out_lo = self->prev_lo +
					((0.5f + level*0.5f) * 
					(in - self->prev_lo));
		out_lo *= level;
	}

	if(self->hpf){ // HPF ACTIVE
		out_hi = out_lo - self->prev_lo + (HPF_COEFF * self->prev_hi);
	} else{
		out_hi = out_lo;
	}
	self->prev_lo = out_lo;
	self->prev_hi = out_hi;
	return out_hi;
}


void lpgate_v( lpgate_t* self, float* level, float* audio, float* out )
{
	// call the fnptr inside
	(*self->lpgate_fnptr)( self
		                 , level
		                 , audio
		                 , out );
}

void lpgate_v_filt_hpf( lpgate_t* self, float* level, float* audio, float* out )
{
	float lpf[self->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	*lpf2 = self->prev_lo +
				(*level2 * 
				(*in2++ - self->prev_lo));
	*lpf2++ *= *level2 / (0.1f + *level2) + LOG_VOL_CONST;
// divide is too expensive
// below is an alternative which goes half-way to matching curves
// but it's still hugely expensive
// let's not worry about changing the curve for now...
	// *lpf2++ *= 1.8 * (*level2 - *level2 * *level2 * 0.5) + LOG_VOL_CONST;
	level2++;

	for(i=1; i<(self->b_size); i++){
		*lpf2 = *lpf3 +
					(*level2 * 
					(*in2++ - *lpf3));
		*lpf2++ *= *level2 / (0.1f + *level2) + LOG_VOL_CONST;
		// *lpf2++ *= 1.8 * (*level2 - *level2 * *level2 * 0.5) + LOG_VOL_CONST;
		lpf3++;
		level2++;
	}

	// hpf
	lpf2 = lpf;
	lpf3 = lpf;
	float* out3 = out;
	
	*out2++ = *lpf2++ - self->prev_lo + (HPF_COEFF * self->prev_hi);
	
	for(i=1; i<(self->b_size); i++){
		*out2++ = *lpf2++ - *lpf3++ + (HPF_COEFF * *out3++);
	}
	
	self->prev_lo = *lpf3;
	self->prev_hi = *out3;
}
void lpgate_v_gate_hpf( lpgate_t* self, float* level, float* audio, float* out )
{
	float lpf[self->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	*lpf2 = self->prev_lo +
				((0.5f + *level2*0.5f) * 
				(*in2++ - self->prev_lo));
	*lpf2++ *= *level2++;

	for(i=1; i<(self->b_size); i++){
		*lpf2 = *lpf3 +
					((0.5f + *level2*0.5f) * 
					(*in2++ - *lpf3));
		*lpf2++ *= *level2++;
		lpf3++;
	}
	
	// hpf
	lpf2 = lpf;
	lpf3 = lpf;
	float* out3 = out;

	*out2++ = *lpf2++ - self->prev_lo + (HPF_COEFF * self->prev_hi);
	
	for(i=1; i<(self->b_size); i++){
		*out2++ = *lpf2++ - *lpf3++ + (HPF_COEFF * *out3++);
	}
	
	self->prev_lo = *lpf3;
	self->prev_hi = *out3;
}
void lpgate_v_filt(     lpgate_t* self, float* level, float* audio, float* out )
{
	float lpf[self->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	*lpf2 = self->prev_lo +
				(*level2 * 
				(*in2++ - self->prev_lo));
	*lpf2 *= *level2 / (0.1f + *level2) + LOG_VOL_CONST;
	*out2++ = *lpf2++;
	level2++;

	for(i=1; i<(self->b_size); i++){
		*lpf2 = *lpf3 +
					(*level2 * 
					(*in2++ - *lpf3));
		*lpf2 *= *level2 / (0.1f + *level2) + LOG_VOL_CONST;
		*out2++ = *lpf2++;
		lpf3++;
		level2++;
	}

	self->prev_lo = *(--lpf2);
	self->prev_hi = *(--out2);
}
void lpgate_v_gate( lpgate_t* self, float* level, float* audio, float* out )
{
	float lpf[self->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	*lpf2 = self->prev_lo +
				((0.5f + *level2 * 0.5f) * 
				// (*level2 * 
				(*in2++ - self->prev_lo));
	*lpf2 *= *level2++;
	*out2++ = *lpf2++;

	for(i=1; i<(self->b_size); i++){
		*lpf2 = *lpf3 +
					((0.5f + *level2 * 0.5f) * 
					// (*level2 * 
					(*in2++ - *lpf3));
		*lpf2 *= *level2++;
		*out2++ = *lpf2++;
		lpf3++;
	}

	self->prev_lo = *(--lpf2);
	self->prev_hi = *(--out2);
}