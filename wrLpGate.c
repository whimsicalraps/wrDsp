#include "wrLpGate.h"
 
void lpgate_init( lpgate_t* self, uint8_t hpf, uint8_t filter, uint16_t b_size )
{
	self->hpf = hpf;
	self->filter = filter;
	self->b_size = b_size;

	self->prev_lo = 0;
	self->prev_hi = 0;
}

float lpgate_step( lpgate_t* self, float level, float in )
{
	float out_lo, out_hi;

	if(self->filter){ // BOTH MODE (LPF -> VOL)
		out_lo = self->prev_lo +
					(level * 
					(in - self->prev_lo));
		out_lo *= level/(0.1 + level) + LOG_VOL_CONST;
	} else {
		out_lo = self->prev_lo +
					((0.5 + level*0.5) * 
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
	float lpf[self->b_size]; // allows sequential processing
	float* lpf2 = lpf;
	float* lpf3 = lpf;

	float* level2 = level;
	float* in2 = audio;
	float* out2 = out;
	uint16_t i;

	// first sample
	if(self->filter){ // BOTH MODE (LPF -> VOL)
		*lpf2 = self->prev_lo +
					(*level2 * 
					(*in2++ - self->prev_lo));
		*lpf2++ *= *level2/(0.1 + *level2) + LOG_VOL_CONST;
		level2++;
		for(i=1; i<(self->b_size); i++){
			*lpf2 = *lpf3 +
						(*level2 * 
						(*in2++ - *lpf3));
			*lpf2++ *= *level2/(0.1 + *level2) + LOG_VOL_CONST;
			lpf3++;
			level2++;
		}
	} else { // VCA MODE (subtle LPF)
		*lpf2 = self->prev_lo +
					((0.5 + *level2*0.5) * 
					(*in2++ - self->prev_lo));
		*lpf2++ *= *level2++;
		for(i=1; i<(self->b_size); i++){
			*lpf2 = *lpf3 +
						((0.5 + *level2*0.5) * 
						(*in2++ - *lpf3));
			*lpf2++ *= *level2++;
			lpf3++;
		}
	}

	lpf2 = lpf;
	if(self->hpf){ // HPF ACTIVE
		lpf3 = lpf;
		float* out3 = out;
		*out2++ = *lpf2++ - self->prev_lo + (HPF_COEFF * self->prev_hi);
		for(i=1; i<(self->b_size); i++){
			*out2++ = *lpf2++ - *lpf3++ + (HPF_COEFF * *out3++);
		}
		self->prev_lo = *lpf3;
		self->prev_hi = *out3;
	} else{ // PASS THROUGH
		for(i=0; i<(self->b_size); i++){
			*out2++ = *lpf2++;
		}
		self->prev_lo = *(--lpf2);
		self->prev_hi = *(--out2);
	}
}