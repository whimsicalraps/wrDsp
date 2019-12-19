#pragma once

#include "wrBuffer.h"

#define OUT_BUF_LEN 64 // defines maximum speed

typedef struct{
    float in_buf[4];
    int   in_buf_ix; // 'phase' but always advances by 1

    float out_buf[OUT_BUF_LEN];
    float out_phase;

    int write_ix; // pointer into the destination buffer

    float phase;
    bool  fadeing;

    bool  active;
} poke_t;

// setup
poke_t* poke_init( void );
void poke_deinit( poke_t* self );

// params
void poke_phase( poke_t* self, buffer_t* buf, int phase );
void poke_active( poke_t* self, bool is_active );

bool poke_is_active( poke_t* self );

// signal
void poke( poke_t*   self
         , buffer_t* buf
         , float     speed
         , float     pre_level
         , float     input );
