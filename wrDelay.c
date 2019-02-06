#include "wrDelay.h"
#include <stdlib.h>
#include "wrMath.h"

#define SAMPLE_RATE 48000 // FIXME how to define this globally
#define MS_TO_SAMPS (SAMPLE_RATE / 1000.0)
#define SAMP_AS_MS  (1.0 / MS_TO_SAMPS)

int delay_init( delay_t* self, float max_time
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
    delay_set_ms( self, time );
    delay_set_feedback( self, 0.0 );
    // private
    self->read = 0;
    return 0;
}

void delay_set_ms( delay_t* self, float time )
{
    self->time  = lim_f(time, SAMP_AS_MS, self->max_time - SAMP_AS_MS);
    self->write = (self->read + (int)(time * MS_TO_SAMPS))
                    % self->max_samps;
}

float delay_get_ms( delay_t* self )
{
    return self->time;
}

void delay_set_feedback( delay_t* self, float feedback )
{
    self->feedback = lim_f( feedback, -0.999, 0.999 );
}

float delay_get_feedback( delay_t* self )
{
    return self->feedback;
}

float delay_step( delay_t* self, float in )
{
    float out = self->buffer[self->read++];
    self->read %= self->max_samps;
    self->buffer[self->write++] = in + out * self->feedback;
    self->write %= self->max_samps;
    return out;
}
float* delay_step_v( delay_t* self, float*   in
                                      , uint16_t size
                                      )
{
    float* in2 = in;
    for( int i=0; i<size; i++ ){
        *in2 = delay_step( self, *in2 );
        in2++;
    }
    return in;
}
