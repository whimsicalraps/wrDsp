#pragma once

#include "wrBuffer.h"

#define OUT_BUF_LEN 64 // defines maximum speed

typedef struct{
    // WRITE / ERASE HEAD
    float in_buf[4];
    int   in_buf_ix; // 'phase' but always advances by 1

    float out_buf[OUT_BUF_LEN];
    float out_phase;

    int write_ix; // pointer into the destination buffer

    float phase;

    bool  recording;

    float rec_level;
    float pre_level;

    // READ HEAD
    float rphase;
} ihead_t;

typedef struct{
    // public
    float    fade_length;
    float    fade_rec_level;
    float    fade_pre_level;

    // private
    ihead_t* head[2];
    int      fade_active_head;
    float    fade_phase;
    float    fade_increment; // 0-1 normalized per-sample
    int      fade_countdown;
} ihead_fade_t;


// setup
ihead_t* ihead_init( void );
void ihead_deinit( ihead_t* self );

ihead_fade_t* ihead_fade_init( void );
void ihead_fade_deinit( ihead_fade_t* self );


// params: setters
void ihead_jumpto( ihead_t* self, buffer_t* buf, int phase, bool is_forward );
void ihead_recording( ihead_t* self, bool is_recording );
void ihead_rec_level( ihead_t* self, float level );
void ihead_pre_level( ihead_t* self, float level );

void ihead_fade_jumpto( ihead_fade_t* self, buffer_t* buf, int phase, bool is_forward );
void ihead_fade_recording( ihead_fade_t* self, bool is_recording );
void ihead_fade_rec_level( ihead_fade_t* self, float level );
void ihead_fade_pre_level( ihead_fade_t* self, float level );



// params: getters
int ihead_get_location( ihead_t* self );
bool ihead_is_recording( ihead_t* self );
float ihead_get_rec_level( ihead_t* self );
float ihead_get_pre_level( ihead_t* self );

int ihead_fade_get_location( ihead_fade_t* self );
bool ihead_fade_is_recording( ihead_fade_t* self );
float ihead_fade_get_rec_level( ihead_fade_t* self );
float ihead_fade_get_pre_level( ihead_fade_t* self );


// signal
// WRITE / ERASE HEAD
void ihead_poke( ihead_t*  self
               , buffer_t* buf
               , float     speed
               , float     input
               );
void ihead_fade_poke( ihead_fade_t*  self
                    , buffer_t*      buf
                    , float          speed
                    , float          input
                    );
// READ HEAD
float ihead_peek( ihead_t* self, buffer_t* buf );
float ihead_fade_peek( ihead_fade_t* self, buffer_t* buf );

float ihead_fade_update_phase( ihead_fade_t* self, float speed );
