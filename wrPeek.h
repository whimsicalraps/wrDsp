#pragma once

#include "wrBuffer.h"

typedef struct{
    float phase;
} peek_t;

// setup
peek_t* peek_init( void );
void peek_deinit( peek_t* self );

// params
void peek_phase( peek_t* self, buffer_t* buf, int phase );
int peek_get_phase( peek_t* self );

// signal
float peek( peek_t* self, buffer_t* buf, float phase );
float* peek_v( float* io
             , float* buf
             , int    buf_len
             , float  phase // initial sample
             , float* rate // step through buf with this array
             , int    size
             );
