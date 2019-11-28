#include "wrPlayer.h"

#include <stdlib.h>
#include <stdio.h>

// setup

player_t* player_init( buffer_t* buffer )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self){ printf("player malloc failed.\n"); return NULL; }

    player_load( self, buffer );
    player_playing( self, false );
    player_goto( self, 0 );

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
    self->speed = speed;
}

void player_goto( player_t* self, int sample )
{
    sample = (sample < 0) ? 0 : sample;
    self->location = (float)sample;
}

// param getters

bool player_is_playing( player_t* self )
{
    return self->playing;
}

float player_get_speed( player_t* self )
{
    return self->speed;
}


// signals

float player_step( player_t* self )
{
    if( !self->buf ){ return 0.0; } // no buffer available

    float out = self->buf->b[(int)self->location]; // FIXME nearest neighbour

    if( self->playing ){
        // transport advance. TODO speed
        self->location += self->speed;

        // naive loop. TODO reverse loop
        if( self->location >= self->buf->len ){ self->location -= self->buf->len; }
    }
    return out;
}

float* player_step_v( player_t* self, float* buf, int size )
{
    float* b = buf;
    for( int i=0; i<size; i++ ){
        *b++ = player_step( self );
    }
    return buf;
}
