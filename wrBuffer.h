#pragma once

#include <stdbool.h>
#include <stdlib.h>


typedef struct buffer{
    int   len; // length of current data
    void* b;   // data buffer
    void* interface; // defined below
} buffer_t;

typedef struct buffer_interface{
    // link to parent
    buffer_t* buf;

    // user-provided fnptrs for access
    float (*peek)( struct buffer_interface*, int* );
    void (*poke)( struct buffer_interface*, int*, float, float );
    bool (*request)( struct buffer_interface*, int );
    void (*free)( struct buffer_interface* );

    // store a user struct here for implementing their interface
    void* userdata;
} buffer_interface_t;

typedef float (*buffer_peek_t)( buffer_interface_t* self, int* location );
typedef void (*buffer_poke_mac_t)( buffer_interface_t* self, int* location, float mult, float accum );
typedef bool (*buffer_request_t)( buffer_interface_t* self, int location );

buffer_t* buffer_init( size_t bytes_per_value, int count, buffer_interface_t* interface );
void buffer_deinit( buffer_t* self );

buffer_t* buffer_new( buffer_t* self, size_t bytes_per_value, int length );
buffer_t* buffer_load_and_own( buffer_t* self, float* buffer, int length );

// these accessors wrap a user-provided implementation through buffer_interface_t
float buffer_peek( buffer_t* self, int* location );
void buffer_poke_mac( buffer_t* self, int* location, float mult, float accum );
bool buffer_request( buffer_t* self, int location );
