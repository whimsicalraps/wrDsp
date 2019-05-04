#include "wrDelay.h"
#include <stdlib.h>
#include <stdio.h> // printf
#include "wrMath.h"

#define SAMPLE_RATE 48000 // FIXME how to define this globally
#define MS_TO_SAMPS (SAMPLE_RATE / 1000.0)
#define SAMP_AS_MS  (1.0 / MS_TO_SAMPS)

// private declarations
float wrap( float input, float modulo );
float peek( delay_t* self );
void poke( delay_t* self, float input );

// public defns
delay_t* delay_init( float max_time
                   , float time
                   ){
    delay_t* self = malloc( sizeof(delay_t) );
    if(self == NULL){ printf("delay: couldn't malloc\n"); }

    self->max_time  = max_time;
    self->max_samps = max_time * MS_TO_SAMPS;
    self->buffer = malloc( sizeof(float) * (int)(self->max_samps + 1) );
    if( self->buffer == NULL ){ printf("delay: couldn't malloc buf\n"); }

    for( int i=0; i<(int)(self->max_samps + 1); i++ ){
        self->buffer[i] = 0.0;
    }
    self->rate = 1.0;
    self->tap_write = 0.0; // before _set_ms()
    delay_set_ms( self, time ); // ->time, ->tap_fb
    delay_set_feedback( self, 0.0 ); // ->feedback
    // private

    return self;
}

void delay_set_ms( delay_t* self, float time ){
    self->time  = lim_f(time, SAMP_AS_MS, self->max_time - SAMP_AS_MS);
    self->tap_fb = wrap( self->tap_write - (time * MS_TO_SAMPS)
                     , self->max_samps
                     );
}

void delay_set_time_percent( delay_t* self, float percent ){
    delay_set_ms( self, self->max_time * lim_f_0_1(percent) );
}

void delay_set_rate( delay_t* self, float rate ){
    self->rate = lim_f( rate, 1.0/16.0, 16.0 );
}

float delay_get_ms( delay_t* self ){
    return self->time;
}

void delay_set_feedback( delay_t* self, float feedback ){
    self->feedback = lim_f( feedback, -0.999, 0.999 );
}

float delay_get_feedback( delay_t* self ){
    return self->feedback;
}

float delay_step( delay_t* self, float in ){
    float out = peek( self );
    self->tap_fb = wrap( self->tap_fb + self->rate
                     , self->max_samps
                     );
    float s = (self->rate >= 1.0) ? 1.0 : self->rate;
    poke( self, in * s + out * self->feedback );
    self->tap_write = wrap( self->tap_write + self->rate
                      , self->max_samps
                      );
    return out;
}

float* delay_step_v( delay_t* self, float* buffer
                                  , int    b_size
                                  ){
    float* in  = buffer;
    float* out = buffer;
    for( int i=0; i<b_size; i++ ){
        *out++ = delay_step( self, *in++ );
    }
    return buffer;
}

// private defns
float wrap( float input, float modulo ){
    while( input >= modulo ){ input -= modulo; }
    while( input < 0.0 ){ input += modulo; }
    return input;
}

float peek( delay_t* self ){
    int ixA = (int)self->tap_fb;
    int ixB = (int)wrap( self->tap_fb + 1, self->max_samps );
    float c = self->tap_fb - (float)ixA;
    return self->buffer[ixA] + c*(self->buffer[ixB] - self->buffer[ixA]);
}

void poke( delay_t* self, float input ){
    int ixA = (int)self->tap_write;
    int ixB = (int)wrap( self->tap_write + 1, self->max_samps );
    float c = self->tap_write - (float)ixA;
    // these coeffs seem backward to me, but reversed is terrible aliasing?!
    self->buffer[ixA] = input * c;
    self->buffer[ixB] = input * (1.0 - c);
}
