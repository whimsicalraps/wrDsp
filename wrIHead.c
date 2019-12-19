#include "wrIHead.h"

#include <stdlib.h>
#include <stdio.h>

#include "wrInterpolate.h"


//////////////////////////////
// private declarations
static float* push_input( ihead_t* self, float* buf, float in );
static int write( ihead_t* self, float rate, float input );


/////////////////////////////////
// public interface

ihead_t* poke_init( void )
{
    ihead_t* self = malloc( sizeof( ihead_t ) );
    if( !self ){ printf("poke_init failed malloc\n"); return NULL; }

// WRITE / ERASE HEAD
    for( int i=0; i<4; i++ ){ self->in_buf[i] = 0.0; }
    self->in_buf_ix = 0;

    for( int i=0; i<OUT_BUF_LEN; i++ ){ self->out_buf[i] = 0.0; }
    self->out_phase = 0.0; // [0,1)

    self->write_ix = 0; // phase into the buffer_t

    poke_recording( self, false );
    poke_rec_level( self, 1.0 );
    poke_pre_level( self, 0.0 );

// READ HEAD
    self->rphase = 0.0;

    return self;
}

void poke_deinit( ihead_t* self )
{
    free(self); self = NULL;
}


/////////////////////////////////////////
// params: setters
void poke_recording( ihead_t* self, bool is_recording ){
    self->recording = is_recording;
}
void poke_rec_level( ihead_t* self, float level ){
    self->rec_level = level;
}
void poke_pre_level( ihead_t* self, float level ){
    self->pre_level = level;
}
void poke_phase( ihead_t* self, buffer_t* buf, int phase ){
    self->write_ix = phase;
}
void peek_phase( ihead_t* self, buffer_t* buf, int phase ){
    self->rphase = (float)phase;
}


//////////////////////////////////
// params: getters
bool poke_is_recording( ihead_t* self ){ return self->recording; }
float poke_get_rec_level( ihead_t* self ){ return self->rec_level; }
float poke_get_pre_level( ihead_t* self ){ return self->pre_level; }
int peek_get_phase( ihead_t* self ){ return self->rphase; }


///////////////////////////////////////
// signals
void poke( ihead_t*   self
         , buffer_t* buf
         , float     speed
         , float     input
         )
{
    if( speed == 0.0 ){ return; }

    int nframes = write( self, speed, input * self->rec_level );

    int dir = (speed >= 0.0) ? 1 : -1;
    float* y = self->out_buf; // y is the data to be written to the buffer_t
    if( self->recording ){
        for( int i=0; i<nframes; i++ ){
            // TODO apply clip, brickwall, compand to *y
            buffer_poke_mac( buf, &self->write_ix, self->pre_level, *y++ );
            self->write_ix = self->write_ix + dir;
        }
    } else { // record inactive, just update phase (as a block)
        self->write_ix = self->write_ix + (nframes * dir);
    }
}

float peek( ihead_t* self, buffer_t* buf, float speed )
{
    // find sample indices
    int p0  = (int)self->rphase;
    int pn1 = p0-1;
    int p1  = p0+1;
    int p2  = p0+2;

    // interpolation coefficient [0,1)
    float coeff = self->rphase - (float)p0;

    // interpolate array to find value
    float samps[4] = { buffer_peek( buf, &pn1 )
                     , buffer_peek( buf, &p0 )
                     , buffer_peek( buf, &p1 )
                     , buffer_peek( buf, &p2 )
                     };
    float out = interp_hermite_4pt( coeff, &samps[1] );

    // uses wrapped integer value and reconstruct with coeff, then move forward speed
    // this means the phase could be out of bounds, but will be corrected next samp
    self->rphase = (float)p0 + coeff + speed;

    return out;
}


//////////////////////////////
// private funcs

static float* push_input( ihead_t* self, float* buf, float in )
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

static int write( ihead_t* self, float rate, float input )
{
    float pushed_buf[4];
    push_input( self, pushed_buf, input );

    rate = (rate < 0.0) ? -rate : rate; // handle negative speeds

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
