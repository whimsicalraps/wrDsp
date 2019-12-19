#include "wrIPlayer.h"

#include <stdlib.h>
#include <stdio.h>

#define REC_OFFSET (-8) // write head trails read head

// setup

player_t* player_init( buffer_t* buffer )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self){ printf("player malloc failed.\n"); return NULL; }

    self->rhead = peek_init();
    if( !self){ printf("player rhead failed.\n"); return NULL; }

    self->whead = poke_init();
    if( !self){ printf("player whead failed.\n"); return NULL; }

    player_load( self, buffer );
    player_playing( self, false );
    player_goto( self, 0 );
    player_rec_level( self, 0.0 );
    player_pre_level( self, 1.0 );

    return self;
}

void player_deinit( player_t* self )
{
    free(self); self = NULL;
}

// params

player_t* player_load( player_t* self, buffer_t* buffer )
{
    self->buf = buffer;
    return self;
}

void player_playing( player_t* self, bool is_play )
{
    self->playing = is_play;
}

void player_speed( player_t* self, float speed )
{
    float old_speed = self->speed;
    self->speed = speed;
    if( (old_speed >= 0.0 && speed < 0.0)
     || (old_speed <= 0.0 && speed > 0.0) ){
        player_goto( self, player_get_goto(self) ); // reset head offset
    }
}

void player_recording( player_t* self, bool is_record )
{
    poke_active( self->whead, is_record );
}

void player_rec_level( player_t* self, float rec_level )
{
    self->rec_level = rec_level;
}

void player_pre_level( player_t* self, float pre_level )
{
    self->pre_level = pre_level;
}

void player_goto( player_t* self, int sample )
{
    if( self->buf ){
        sample = (sample < 0) ? 0 : sample;
        peek_phase( self->rhead, self->buf, sample );
        poke_phase( self->whead
                  , self->buf
                  , sample
                    + ((self->speed >= 0.0) ? REC_OFFSET : -REC_OFFSET ));
    }
}


// param getters

bool player_is_playing( player_t* self )
{
    return self->playing;
}

float player_get_goto( player_t* self )
{
    return (float)peek_get_phase( self->rhead );
}

float player_get_speed( player_t* self )
{
    return self->speed;
}

bool player_is_recording( player_t* self )
{
    return poke_is_active( self->whead );
}

float player_get_rec_level( player_t* self )
{
    return self->rec_level;
}

float player_get_pre_level( player_t* self )
{
    return self->pre_level;
}


// signals

float player_step( player_t* self, float in )
{
    if( !self->buf ){ return 0.0; } // no buffer available

    float speed = (self->playing) ? self->speed : 0.0;
    float out = peek( self->rhead
                    , self->buf
                    , speed
                    );
    poke( self->whead
        , self->buf
        , speed
        , self->pre_level
        , in * self->rec_level
        );
    return out;
}

float* player_step_v( player_t* self, float* io, int size )
{
    //for( int i=0; i<size; i++ ){
    //    buf[i] = buffer_peek( self->buf->b, self->buf->len, self->location );
    //}
    //float rates[size];
    //for( int i=0; i<size; i++ ){ rates[i] = self->speed; }
    //float phase = self->location - (int)self->location;

    //return buffer_peek_v( buf
    //                    , self->buf->b
    //                    , self->buf->len
    //                    , phase
    //                    , rates
    //                    , size
    //                    );

    float* b = io;
    for( int i=0; i<size; i++ ){
        *b = player_step( self, *b );
        b++;
    }
    return io;
}
