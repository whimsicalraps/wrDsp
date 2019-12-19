#include "wrPoke.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrInterpolate.h"


//////////////////////////////
// private declarations
static float* push_input( poke_t* self, float* buf, float in );
static int write( poke_t* self, float rate, float input );


/////////////////////////////////
// public interface

poke_t* poke_init( void )
{
    poke_t* self = malloc( sizeof( poke_t ) );
    if( !self ){ printf("poke_init failed malloc\n"); return NULL; }

    for( int i=0; i<4; i++ ){ self->in_buf[i] = 0.0; }
    self->in_buf_ix = 0;

    for( int i=0; i<OUT_BUF_LEN; i++ ){ self->out_buf[i] = 0.0; }
    self->out_phase = 0.0; // [0,1)

    self->write_ix = 0; // phase into the buffer_t

    self->fadeing = false;
    self->active = false;

    return self;
}

void poke_deinit( poke_t* self )
{
    free(self); self = NULL;
}

void poke_active( poke_t* self, bool is_active )
{
    self->active = is_active;
}

void poke_phase( poke_t* self, buffer_t* buf, int phase )
{
    self->write_ix = phase;
    if( self->write_ix >= buf->len ){ self->write_ix -= buf->len; }
    if( self->write_ix < 0 ){ self->write_ix += buf->len; }
}

bool poke_is_active( poke_t* self )
{
    return self->active;
}

void poke( poke_t*   self
         , buffer_t* buf
         , float     speed
         , float     pre_level
         , float     input
         )
{
    if( speed == 0.0 ){ return; }

    int nframes = write( self, speed, input );

    int dir = (speed >= 0.0) ? 1 : -1;
    float* y = self->out_buf; // y is the data to be written to the buffer_t
    if( self->active ){
        for( int i=0; i<nframes; i++ ){
            // TODO apply clip, brickwall, compand to *y
            buffer_poke_mac( buf, &self->write_ix, pre_level, *y++ );
            self->write_ix = self->write_ix + dir;
        }
    } else { // record inactive, just update phase (as a block)
        self->write_ix = self->write_ix + (nframes * dir);
    }
}


//////////////////////////////
// private funcs

static float* push_input( poke_t* self, float* buf, float in )
{
    const int IN_BUF_MASK = 3; // ie IN_BUF_LEN - 1
    self->in_buf_ix = (self->in_buf_ix + 1) & IN_BUF_MASK;
    self->in_buf[self->in_buf_ix] = in;

    int i0 = (self->in_buf_ix + 1) & IN_BUF_MASK;
    int i1 = (self->in_buf_ix + 2) & IN_BUF_MASK;
    int i2 = (self->in_buf_ix + 3) & IN_BUF_MASK;
    int i3 =  self->in_buf_ix;

    buf[0] = self->in_buf[i0];
    buf[1] = self->in_buf[i1];
    buf[2] = self->in_buf[i2];
    buf[3] = self->in_buf[i3];

    return buf;
}

static int write( poke_t* self, float rate, float input )
{
    float pushed_buf[4];
    push_input( self, pushed_buf, input );

    rate = (rate < 0.0) ? -rate : rate;
    float phi = 1.0/rate;
    float new_phase = self->out_phase + rate;
    int new_frames = (int)new_phase;
    // we want to track fractional output phase for interpolation
    // this is normalized to the distance between input frames
    // so: the distance to the first output frame boundary:
    if( new_frames ){
        float f = 1.0 - self->out_phase;
        f *= phi; // normalized (divided by rate);

        int i=0;
        // FIXME: i changed the sequencing here because it didn't make sense?
        while(i < new_frames) {
            // distance between output frames in this normalized space is 1/rate
            self->out_buf[i] = interp_hermite_4pt( f, &(pushed_buf[1]) );
            f += phi;
            i++;
        }
    }
    // store the remainder of the updated, un-normalized output phase
    self->out_phase = new_phase - (float)new_frames;
    return new_frames; // count of complete frames written
}
