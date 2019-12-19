#pragma once

#include <stdbool.h>

typedef struct{
    int    len; // length of current data
    float* b;   // data buffer
} buffer_t;

buffer_t* buffer_init( int count );
void buffer_deinit( buffer_t* self );

buffer_t* buffer_new( buffer_t* self, int length );
buffer_t* buffer_load_and_own( buffer_t* self, float* buffer, int length );

float buffer_peek( buffer_t* self, int* location );
void buffer_poke_mac( buffer_t* self, int* location, float mult, float accum );
