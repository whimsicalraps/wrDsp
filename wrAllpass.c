#include "wrAllpass.h"
#include <stdlib.h>
#include "wrMath.h"

#define SAMPLE_RATE 48000 // FIXME how to define this globally
#define MS_TO_SAMPS (SAMPLE_RATE / 1000.0)
#define SAMP_AS_MS  (1.0 / MS_TO_SAMPS)

int allpass_init( allpass_t* self, float max_time
                                 , float time
                                 )
{
    self->max_time  = max_time;
    self->max_samps = (int)(max_time * MS_TO_SAMPS);
    self->buffer = malloc( sizeof(float) * (self->max_samps + 1) );
    if( self->buffer == NULL ){ return 1; }
    for( int i=0; i<(self->max_samps); i++ ){
        self->buffer[i] = 0.0;
    }
    allpass_set_ms( self, time );
    allpass_set_gain( self, 0.0 );
    // private
    self->read = 0;
    return 0;
}

void allpass_set_ms( allpass_t* self, float time )
{
    self->time  = lim_f(time, SAMP_AS_MS, self->max_time - SAMP_AS_MS);
    self->write = (self->read + (int)(time * MS_TO_SAMPS))
                    % self->max_samps;
}

float allpass_get_ms( allpass_t* self )
{
    return self->time;
}

void allpass_set_gain( allpass_t* self, float gain )
{
    self->gain = lim_f( gain, -0.999, 0.999 );
}

float allpass_get_gain( allpass_t* self )
{
    return self->gain;
}

float allpass_step( allpass_t* self, float in )
{
    float out = self->buffer[self->read++];
    self->read %= self->max_samps;
    self->buffer[self->write++] = in - out * self->gain;
    self->write %= self->max_samps;
    return out + in * self->gain;
}
float* allpass_step_v( allpass_t* self, float*   in
                                      , uint16_t size
                                      )
{
    float* in2 = in;
    for( int i=0; i<size; i++ ){
        *in2 = allpass_step( self, *in2 );
        in2++;
    }
    return in;
}
