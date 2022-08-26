#include "wrLpGate.h"

#include <stdlib.h>

// private declarations
void _lpgate_mode_select( lpgate_t* self );
float* lpgate_v_filt(     lpgate_t* self, float* level, float* buffer, int b_size );
float* lpgate_v_gate(     lpgate_t* self, float* level, float* buffer, int b_size );
float* lpgate_v_filt_hpf( lpgate_t* self, float* level, float* buffer, int b_size );
float* lpgate_v_gate_hpf( lpgate_t* self, float* level, float* buffer, int b_size );

lpgate_t* lpgate_init( uint8_t hpf
                     , uint8_t filter
                     ){
    lpgate_t* self = malloc( sizeof(lpgate_t) );

	self->hpf    = hpf;
	self->filter = filter;
	_lpgate_mode_select(self);

	self->prev_lpf = 0;
	self->prev_out = 0;
    return self;
}

void lpgate_deinit( lpgate_t* self
                  ){
    free(self); self = NULL;
}

void lpgate_hpf_mode( lpgate_t* self
                    , uint8_t   hpf
                    ){
	self->hpf = !!hpf;
	_lpgate_mode_select(self);
    self->prev_out = self->prev_lpf; // ensure hpf in known condition
}

void lpgate_filter_mode( lpgate_t* self
                       , uint8_t   filter
                       ){
	self->filter = !!filter;
	_lpgate_mode_select(self);
}

// [filter][hpf]
static float* (*lp_fnptr[2][2])(lpgate_t*,float*,float*,int) =
	{ { lpgate_v_gate
	  , lpgate_v_gate_hpf }
	, { lpgate_v_filt
	  , lpgate_v_filt_hpf }
	};
void _lpgate_mode_select( lpgate_t* self
                        ){
	self->lpgate_fnptr = lp_fnptr[ self->filter ][ self->hpf ];
    self->prev_out = self->prev_lpf; // ensure hpf in known condition
}

float lpgate_step( lpgate_t* self
                 , float     level
                 , float     in
                 ){
	float out_lo, out_hi;

	if(self->filter){ // BOTH MODE (LPF -> VOL)
		out_lo = self->prev_lpf +
					(level *
					(in - self->prev_lpf));
		out_lo *= level/(0.1f + level) + LOG_VOL_CONST;
	} else {
		out_lo = self->prev_lpf +
					((0.5f + level*0.5f) *
					(in - self->prev_lpf));
		out_lo *= level;
	}

	if(self->hpf){ // HPF ACTIVE
		out_hi = out_lo - self->prev_lpf + (HPF_COEFF * self->prev_out);
	} else{
		out_hi = out_lo;
	}
	self->prev_lpf = out_lo;
	self->prev_out = out_hi;
	return out_hi;
}

float* lpgate_v( lpgate_t* self
               , float*    level
               , float*    buffer
               , int       b_size
               ){
    return (*self->lpgate_fnptr)( self
                                , level
                                , buffer
                                , b_size
                                );
}

// private function definitions
float* lpgate_v_filt_hpf( lpgate_t* self
                        , float*    level
                        , float*    buffer
                        , int       b_size
                        ){
    float lowpass;
    for(int i=0; i<b_size; i++){
        lowpass = (level[i] / (0.1 + level[i]) + LOG_VOL_CONST)
                        * (self->prev_lpf
                            + level[i]
                                * (buffer[i] - self->prev_lpf));
        buffer[i] = lowpass - self->prev_lpf + (HPF_COEFF * self->prev_out);
        self->prev_lpf = lowpass;
        self->prev_out = buffer[i];
    }
    return buffer;
}

float* lpgate_v_gate_hpf( lpgate_t* self
                        , float*    level
                        , float*    buffer
                        , int       b_size
                        ){
    float lowpass;
    for(int i=0; i<b_size; i++){
        lowpass = level[i] * (self->prev_lpf
                                + (0.5f + level[i] * 0.5f)
                                    * (buffer[i] - self->prev_lpf));
        buffer[i] = lowpass - self->prev_lpf + (HPF_COEFF * self->prev_out);
        self->prev_lpf = lowpass;
        self->prev_out = buffer[i];
    }
    return buffer;
}

float* lpgate_v_filt( lpgate_t* self
                    , float*    level
                    , float*    buffer
                    , int       b_size
                    ){
    for(int i=0; i<b_size; i++){
        buffer[i] = (level[i] / (0.1 + level[i]) + LOG_VOL_CONST)
                        * (self->prev_lpf
                            + level[i]
                                * (buffer[i] - self->prev_lpf));
        self->prev_lpf = buffer[i];
    }
    return buffer;
}

float* lpgate_v_gate( lpgate_t* self
                    , float*    level
                    , float*    buffer
                    , int       b_size
                    ){
    for(int i=0; i<b_size; i++){
        buffer[i] = level[i] * (self->prev_lpf
                                + (0.5f + level[i] * 0.5f)
                                    * (buffer[i] - self->prev_lpf));
        self->prev_lpf = buffer[i];
    }
    return buffer;
}
