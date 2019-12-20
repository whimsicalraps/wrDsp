#include "wrIPlayer.h"

#include <stdlib.h>
#include <stdio.h>


// setup

player_t* player_init( buffer_t* buffer )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self){ printf("player malloc failed.\n"); return NULL; }

    self->head = ihead_fade_init();
    if( !self){ printf("player head failed.\n"); return NULL; }

    player_load( self, buffer );
    player_playing( self, false );
    player_goto( self, 0 );
    player_rec_level( self, 0.0 );
    player_pre_level( self, 1.0 );

    return self;
}

void player_deinit( player_t* self )
{
    ihead_fade_deinit( self->head );
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
    ihead_fade_recording( self->head, is_record );
}

void player_rec_level( player_t* self, float rec_level )
{
    ihead_fade_rec_level( self->head, rec_level );
}

void player_pre_level( player_t* self, float pre_level )
{
    ihead_fade_pre_level( self->head, pre_level );
}

void player_goto( player_t* self, int sample )
{
    if( self->buf ){
        if( buffer_request( self->buf, sample ) ){
            ihead_fade_jumpto( self->head
                             , self->buf
                             , sample
                             , (self->speed >= 0.0)
                             );
        } else {
            printf("TODO queue a request until it becomes available\n");
        }
    }
}


// param getters

bool player_is_playing( player_t* self )
{
    return self->playing;
}

float player_get_goto( player_t* self )
{
    return (float)ihead_fade_get_location( self->head );
}

float player_get_speed( player_t* self )
{
    return self->speed;
}

bool player_is_recording( player_t* self )
{
    return ihead_fade_is_recording( self->head );
}

float player_get_rec_level( player_t* self )
{
    return ihead_fade_get_rec_level( self->head );
}

float player_get_pre_level( player_t* self )
{
    return ihead_fade_get_pre_level( self->head );
}


// signals

// this is an abstraction of a 'tape head'
// TODO rename!
float player_step( player_t* self, float in )
{
    if( !self->buf ){ return 0.0; } // no buffer available

    float motion = (self->playing) ? self->speed : 0.0;
    float out = ihead_fade_peek( self->head
                               , self->buf
                               , motion
                               );
    ihead_fade_poke( self->head
                   , self->buf
                   , motion
                   , in
                   );
    return out;
}

float* player_step_v( player_t* self, float* io, int size )
{
    float* b = io;
    for( int i=0; i<size; i++ ){
        *b = player_step( self, *b );
        b++;
    }
    return io;
}
