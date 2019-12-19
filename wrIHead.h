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


// setup
ihead_t* poke_init( void );
void poke_deinit( ihead_t* self );


// params: setters
// WRITE / ERASE HEAD
void poke_phase( ihead_t* self, buffer_t* buf, int phase );
void poke_recording( ihead_t* self, bool is_recording );
void poke_rec_level( ihead_t* self, float level );
void poke_pre_level( ihead_t* self, float level );
// READ HEAD
void peek_phase( ihead_t* self, buffer_t* buf, int phase );


// params: getters
// WRITE / ERASE HEAD
bool poke_is_recording( ihead_t* self );
float poke_get_rec_level( ihead_t* self );
float poke_get_pre_level( ihead_t* self );
// READ HEAD
int peek_get_phase( ihead_t* self );


// signal
// WRITE / ERASE HEAD
void poke( ihead_t*  self
         , buffer_t* buf
         , float     speed
         , float     input
         );
// READ HEAD
float peek( ihead_t* self, buffer_t* buf, float phase );
