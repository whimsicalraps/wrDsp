#include "wrPeek.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrInterpolate.h"

peek_t* peek_init( void )
{
    peek_t* self = malloc( sizeof( peek_t ) );
    if( !self ){ printf("peek_init couldn't malloc\n"); return NULL; }

    self->phase = 0.0;

    return self;
}

void peek_deinit( peek_t* self )
{
    free(self); self = NULL;
}

void peek_phase( peek_t* self, buffer_t* buf, int phase )
{
    self->phase = (float)phase;
}

int peek_get_phase( peek_t* self )
{
    return self->phase;
}


// TODO this should trigger an update in the buffer object
float peek( peek_t* self, buffer_t* buf, float speed )
{
    // find sample indices
    int p0  = (int)self->phase;
    int pn1 = p0-1;
    int p1  = p0+1;
    int p2  = p0+2;

    // calc coefficient
    float coeff = self->phase - (float)p0;

    // interpolate array to find value
    float samps[4] = { buffer_peek( buf, &pn1 )
                     , buffer_peek( buf, &p0 )
                     , buffer_peek( buf, &p1 )
                     , buffer_peek( buf, &p2 )
                     };
    float out = interp_hermite_4pt( coeff, &samps[1] );

    // uses wrapped integer value and reconstruct with coeff, then move forward speed
    // this means the phase could be out of bounds, but will be corrected next samp
    self->phase = (float)p0 + coeff + speed;

    return out;
}

// TODO update to use a buffer_t
// FIXME also outdated direct access to buffer data
float* peek_v( float* io
             , float* buf
             , int    buf_len
             , float  phase // initial sample
             , float* rate // step through buf with this array
             , int    size
             )
{
    printf("DON'T USE ME. SEG FAULTING\n");
    // working buffers
    float maxrate = (rate[0] > rate[size-1]) ? rate[0] : rate[size-1];
    int sampsize = maxrate*size + 4; // maybe +3?
    float s[sampsize];
    int p0 = (int)phase;

    { // build contiguous source array
        int ix[sampsize];
        int pn1 = p0 - 1;
        for( int i=0; i<sampsize; i++ ){
            // TODO can this index builder be optimized out?
            ix[i] = pn1 + i;
        }
        { // edge checking from both ends until first in range
            int i;
            i=0; while( ix[i] < 0 ){ ix[i++] += buf_len; }
            i=sampsize-1; while( ix[i] >= buf_len ){ ix[i--] -= buf_len; }
        }
        for( int i=0; i<sampsize; i++ ){
            s[i] = buf[ix[i]];
        }
    }

    // interpolate
    float* o = io;
    float coeff = phase - (float)p0;
    int ix = 1; // allow for leading samp
    for( int i=0; i<size; i++ ){
        *o++ = interp_hermite_4pt( coeff, &s[ix] );
        coeff += *rate++;
        while(coeff >= 1.0){
            coeff -= 1.0;
            ix++;
        }
    }
    return io;
}
