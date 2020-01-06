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
}

void lpgate_filter_mode( lpgate_t* self
                       , uint8_t   filter
                       ){
    self->filter = !!filter;
    _lpgate_mode_select(self);
}
void _lpgate_mode_select( lpgate_t* self
                        ){
    // [filter][hpf]
    static float* (*fnptr[2][2])() =
        { { lpgate_v_gate
          , lpgate_v_gate_hpf }
        , { lpgate_v_filt
          , lpgate_v_filt_hpf }
        };
    self->lpgate_fnptr = fnptr[ self->filter ][ self->hpf ];
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

static float* filter_v( lpgate_t* self
                      , float*    level
                      , float*    buffer
                      , int       b_size
                      ){
    float  lp;
    float* lvl = level;
    float* in  = buffer;
    float* out = buffer;

    lp = (*lvl / (0.1 + *lvl) + LOG_VOL_CONST)
          * (self->prev_lpf
                + *lvl * (*in++ - self->prev_lpf));
    lvl++;
    *out++ = lp;

    for( int i=1; i<b_size; i++ ){
        lp = (*lvl / (0.1 + *lvl) + LOG_VOL_CONST)
                * (lp + *lvl * (*in++ - lp));
        lvl++;
        *out++ = lp;
    }

    self->prev_lpf = lp;
    self->prev_out = lp;

    return buffer;
}

static float* gate_v( lpgate_t* self
                    , float*    level
                    , float*    buffer
                    , int       b_size
                    ){
    float  lp;
    float* lvl = level;
    float* in  = buffer;
    float* out = buffer;

    lp = *lvl
          * (self->prev_lpf
                + (0.5 + *lvl * 0.5) * (*in++ - self->prev_lpf));
    lvl++;
    *out++ = lp;

    for( int i=1; i<b_size; i++ ){
        lp = *lvl * (lp + (0.5 + *lvl * 0.5) * (*in++ - lp));
        lvl++;
        *out++ = lp;
    }

    self->prev_lpf = lp;
    self->prev_out = lp;

    return buffer;
}

static float* hpf_v( lpgate_t* self
                   , float*    level
                   , float*    buffer
                   , int       b_size
                   ){
    float  lowpass[b_size];
    float* in  = lowpass;
    float* in2 = lowpass;
    float* out  = buffer;
    float* out2 = buffer;

    *out++ = *in++ - self->prev_lpf + (HPF_COEFF * self->prev_out);
    for( int i=1; i<b_size; i++ ){
        *out++ = *in++ - *in2++ + (HPF_COEFF * *out2++);
    }

    self->prev_lpf = *in2;
    self->prev_out = *out2;

    return buffer;
}


// TODO do we need to handle self->prev_* assignment outside the vectors?
//      if not, can remove the non-hpf wrappers & call directly
float* lpgate_v_filt_hpf( lpgate_t* self
                        , float*    level
                        , float*    buffer
                        , int       b_size
                        ){
    filter_v( self, level, buffer, b_size );
    return hpf_v( self, level, buffer, b_size );
}

float* lpgate_v_gate_hpf( lpgate_t* self
                        , float*    level
                        , float*    buffer
                        , int       b_size
                        ){
    gate_v( self, level, buffer, b_size );
    return hpf_v( self, level, buffer, b_size );
}

float* lpgate_v_filt( lpgate_t* self
                    , float*    level
                    , float*    buffer
                    , int       b_size
                    ){
    filter_v( self, level, buffer, b_size );
}

float* lpgate_v_gate( lpgate_t* self
                    , float*    level
                    , float*    buffer
                    , int       b_size
                    ){
    gate_v( self, level, buffer, b_size );
}
