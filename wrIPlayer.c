#include "wrIPlayer.h"

#include <stdlib.h>
#include <stdio.h>


///////////////////////////////
// private declarations

static float tape_clamp( player_t* self, float location );


///////////////////////////////
// setup

player_t* player_init( buffer_t* buffer )
{
    player_t* self = malloc( sizeof( player_t ) );
    if( !self){ printf("player malloc failed.\n"); return NULL; }

    self->head = ihead_fade_init();
    if( !self){ printf("player head failed.\n"); return NULL; }

    self->speed = 0.0;
    player_load( self, buffer );
    player_playing( self, false );
    player_rec_level( self, 0.0 );
    player_pre_level( self, 1.0 );
    player_loop(self, true);
    self->going = false;
    return self;
}

void player_deinit( player_t* self )
{
    ihead_fade_deinit( self->head );
    free(self); self = NULL;
}


/////////////////////////////
// params

player_t* player_load( player_t* self, buffer_t* buffer )
{
    self->buf = buffer;
    if(self->buf){
        self->tape_end = buffer->len;
        player_goto( self, 0 );
        player_loop_start( self, 0.0 );
        player_loop_end( self, self->tape_end );
    }
    return self;
}

void player_playing( player_t* self, bool is_play )
{
    self->playing = is_play;
}

bool player_goto( player_t* self, int sample )
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
            return true; // queued
        }
    }
    return false;
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

void player_loop( player_t* self, bool is_looping )
{
    self->loop = is_looping;
}

void player_loop_start( player_t* self, float location )
{
    self->loop_start = tape_clamp( self, location );
}

void player_loop_end( player_t* self, float location )
{
    self->loop_end = tape_clamp( self, location );
}


///////////////////////////////////
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

bool player_is_looping( player_t* self )
{
    return self->loop;
}

float player_get_loop_start( player_t* self )
{
    return self->loop_start;
}

float player_get_loop_end( player_t* self )
{
    return self->loop_end;
}


/////////////////////////////////////
// signals

// this is an abstraction of a 'tape head'
// TODO rename!
#define LEAD_IN ((float)64.0)
float player_step( player_t* self, float in )
{
    if( !self->buf ){ return 0.0; } // no buffer available

    float motion = (self->playing) ? self->speed : 0.0;
    float out = ihead_fade_peek( self->head, self->buf );
    ihead_fade_poke( self->head
                   , self->buf
                   , motion
                   , in
                   );
    float new_phase = ihead_fade_update_phase( self->head, motion );

    if( !self->going ){ // only edge check if there isn't a queued jump
        float jumpto = -1.0;
        if( self->loop ){ // apply loop brace
            if( new_phase >= self->loop_end ){ jumpto = self->loop_start; }
            else if( new_phase <  self->loop_start ){ jumpto = self->loop_end; }
        } else { // no loop brace, so just loop the whole buffer
            // TODO this check could be *always* run, but result in a STOP (tape edge safety)
            if( new_phase >= (self->tape_end - LEAD_IN) ){ jumpto = LEAD_IN; }
            else if( new_phase < LEAD_IN ){ jumpto = self->tape_end - LEAD_IN; }
        }
        if( jumpto >= 0.0 ){ // if there's a new jump, request it
            self->going = player_goto( self, jumpto ); // true = busy, will callback on completion
        }
    }
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


//////////////////////////////////
// private funcs

static float tape_clamp( player_t* self, float location )
{
    if( location < LEAD_IN ){ location = LEAD_IN; }
    if( location > (self->tape_end - LEAD_IN) ){ location = self->tape_end - LEAD_IN; }
    return location;
}
