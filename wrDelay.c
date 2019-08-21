#include "wrDelay.h"
#include <stdlib.h>
#include <stdio.h> // printf
#include "../wrLib/wrMath.h"
#include "../wrLib/wrTrig.h"

#define SAMPLE_RATE 48000 // FIXME how to define this globally
#define MS_TO_SAMPS (SAMPLE_RATE / 1000.0)
#define SAMP_AS_MS  (1.0 / MS_TO_SAMPS)

// private declarations
static inline float wrap( float input, float modulo );
float peek( delay_t* self, float tap );
void poke( delay_t* self, float input );

// public defns
delay_t* delay_init( float max_time
                   , float time
                   ){
    delay_t* self = malloc( sizeof(delay_t) );
    if( !self ){ printf("delay: couldn't malloc\n"); return NULL; }

    self->max_time  = max_time;
    self->max_samps = (int)(max_time * MS_TO_SAMPS);
    self->buffer = malloc( sizeof(float) * (int)(self->max_samps + 1) );
    if( !self->buffer ){
        printf("delay: couldn't malloc buf\n");
        free(self); self = NULL;
        return NULL;
    }
    for( int i=0; i<(int)(self->max_samps + 1); i++ ){
        self->buffer[i] = 0.0;
    }
    self->rate = 1.0;
    self->tap_write = 0.0;
    self->tap_fb    = 0.0;
    delay_set_ms( self, time );
    delay_set_feedback( self, 0.0 );
    self->svf = svf_init( 0, SAMPLE_RATE );
    self->dcfb = dc_init();
    svf_set_q( self->svf, 0.001 );
    self->fb_filter = lpgate_init( LPGATE_HPF_ON, LPGATE_FILTER );
    //lp1_set_coeff( self->fb_filter, 0.2 );

    return self;
}

void delay_deinit( delay_t* self ){
    lpgate_deinit( self->fb_filter );
    svf_deinit( self->svf );
    free(self->buffer); self->buffer = NULL;
    free(self); self = NULL;
}

void delay_set_ms( delay_t* self, float time ){
    self->tap_fb = wrap( self->tap_write - (time * MS_TO_SAMPS)
                       , self->max_samps
                       );
}

void delay_set_time_percent( delay_t* self, float percent ){
    delay_set_ms( self, self->max_time * lim_f_0_1(percent) );
}

void delay_set_rate( delay_t* self, float rate ){
    self->rate = lim_f( rate, 1.0/128.0, 2.0 );
    svf_set_coeff( self->svf, rate/4.0 );
}

float delay_get_ms( delay_t* self ){
    return SAMP_AS_MS * wrap( self->tap_write - self->tap_fb
                            , self->max_samps
                            );
}

void delay_set_feedback( delay_t* self, float feedback ){
    //self->feedback = lim_f( feedback, -0.999, 0.999 );
    self->feedback = lim_f( feedback, 0.0, 0.999 );
}

float delay_get_feedback( delay_t* self ){
    return self->feedback;
}

static float advance( delay_t* self, float* tap ){
    *tap = wrap( *tap + self->rate
               , self->max_samps
               );
    return *tap;
}

float delay_step( delay_t* self, float in, float phase ){
    float fb = lpgate_step( self->fb_filter
                          , self->feedback
                          , peek( self
                                , advance( self, &(self->tap_fb) )
                                )
                          );
    float out = peek( self
                    , wrap( advance( self, &(self->tap_write) )
                            - (phase * self->max_time * MS_TO_SAMPS)
                          , self->max_samps
                          )
                    );
    poke( self, tanh_fast( dc_step( self->dcfb
                                  , in + fb
                                  )));
    return out;
}

float* delay_step_v( delay_t* self, float* buffer
                                  , float* phase
                                  , int    b_size
                                  ){

    float* in  = buffer;
    float* out = buffer;

    for( int i=0; i<b_size; i++ ){
        *in = svf_process_frame( self->svf, *in );
        *out++ = delay_step( self
                           , *in++
                           , *phase++
                           );
    }
    return buffer;
}

// private defns
float wrap( float input, float modulo ){
    while( input >= modulo ){ input -= modulo; }
    while( input < 0.0 ){ input += modulo; }
    return input;
}

int wrapI( int input, int modulo ){
    //modulo += 1;
    while( input >= modulo ){ input -= modulo; }
    while( input < 0 ){ input += modulo; }
    return input;
}

float peek( delay_t* self, float tap ){
    int ix[4];
    ix[1] = (int)tap;
    ix[0] = wrapI( ix[1] - 1, (int)self->max_samps );
    ix[2] = wrapI( ix[1] + 1, (int)self->max_samps );
    ix[3] = wrapI( ix[1] + 2, (int)self->max_samps );

    // shifted from the textbook by +1 to co (shift range to 0-1)
    // Julius Smith III on 3rd order lagrange interpolation
    float coeff[4];
    float c = tap - (float)ix[1];
    coeff[0] = -((c    )*(c-1.0)*(c-2.0))/6.0;
    coeff[1] =  ((c+1.0)*(c-1.0)*(c-2.0))/2.0;
    coeff[2] = -((c+1.0)*(c    )*(c-2.0))/2.0;
    coeff[3] =  ((c+1.0)*(c    )*(c-1.0))/6.0;

    float out = 0.0;
    for( int s=0; s<4; s++ ){
        out += self->buffer[ix[s]] * coeff[s];
    }
    return out;
}

void poke( delay_t* self, float input ){
    int ixA = (int)self->tap_write;
    int ixZ = wrapI( ixA - 1, (int)self->max_samps );
    int ixB = wrapI( ixA + 1, (int)self->max_samps );
    int ixC = wrapI( ixA + 2, (int)self->max_samps );
    int ixD = wrapI( ixA + 3, (int)self->max_samps );
    int ixE = wrapI( ixA + 4, (int)self->max_samps );

    float c = self->tap_write - (float)ixA;
    if( self->rate <= 1.0 ){
        input *= self->rate;

      //self->buffer[ixZ] += 0.0;
        self->buffer[ixA] += (1-c)*input;
        self->buffer[ixB] += (c)*input;
      //self->buffer[ixC] += 0.0;
        self->buffer[ixD]  = 0.0;
        self->buffer[ixE]  = 0.0;
    } else {
        float isp  = 1.0 / self->rate;
        float iRcB = isp * c;
        float c1   = iRcB - isp + 1.0;
        float c2   = 1.0 - iRcB;

        self->buffer[ixZ] += input * lim_f_0_1(c2 - isp);
        self->buffer[ixA] += input * c2;
        self->buffer[ixB] += input * c1;
        self->buffer[ixC] += input * lim_f_0_1(c1 - isp);
        self->buffer[ixD]  = 0.0;
        self->buffer[ixE]  = 0.0;
    }
}
