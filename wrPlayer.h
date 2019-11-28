#pragma once

#include "wrBuffer.h"

typedef struct{
    buffer_t*  buf;

    bool playing; // transport state
    float speed; // transport speed
    float location; // 'playhead' pointer to buffer
} player_t;

// setup
player_t* player_init( buffer_t* buffer );
void player_deinit( player_t* self );

// param setters
player_t* player_load( player_t* self, buffer_t* buffer );

void player_playing( player_t* self, bool is_play );
void player_speed( player_t* self, float speed );
void player_goto( player_t* self, int sample );

// param getters
bool player_is_playing( player_t* self );
float player_get_speed( player_t* self );

// signals
float player_step( player_t* self );
float* player_step_v( player_t* self, float* buf, int size );
