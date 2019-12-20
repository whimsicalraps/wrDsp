#include "wrBuffer.h"

#include <stdio.h>

////////////////////////////////
// private declarations

static void buffer_free( buffer_t* self );


////////////////////////////////
// public interface defns

buffer_t* buffer_init( size_t bytes_per_value
                     , int count
                     , buffer_interface_t* interface
                     )
{
    buffer_t* self = malloc( sizeof( buffer_t ) );
    if( !self ){ printf("buffer_init malloc failed.\n"); return NULL; }

    self->len = 0;
    self->b   = NULL;
    self->interface = interface;
    interface->buf = self; // back-link for interface to access buffer
    buffer_new( self, bytes_per_value, count );
    return self;
}

void buffer_deinit( buffer_t* self )
{
    buffer_free( self );
    buffer_interface_t* i = ((buffer_interface_t*)(self->interface));
    i->free(i);
    free(self); self = NULL;
}

buffer_t* buffer_new( buffer_t* self, size_t bytes_per_value, int length )
{
    buffer_free( self );
    if( length ){ // zero-length just clears buffer
        self->b = malloc( bytes_per_value * length );
        if( self->b ){
            printf("buffer_new malloc failed\n");
        } else {
            self->len = length;
        }
    }
    return self;
}

// WARNING! this function takes ownership of a buffer
// use it by passing a malloc'd pointer in directly
buffer_t* buffer_load_and_own( buffer_t* self, float* buffer, int length )
{
    buffer_free( self );
    if( buffer && length ){ // zero-length just clears buffer
        self->b   = buffer;
        self->len = length;
    }
    return self;
}

float buffer_peek( buffer_t* self, int* location )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->peek)( i, location );
}

void buffer_poke_mac( buffer_t* self, int* location, float mult, float accum )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    (*i->poke)( i, location, mult, accum );
}

bool buffer_request( buffer_t* self, int location )
{
    buffer_interface_t* i = (buffer_interface_t*)(self->interface);
    return (*i->request)( i, location );
}

////////////////////////////////
// private defns

static void buffer_free( buffer_t* self )
{
    self->len = 0;
    if( self->b ){
        free(self->b);
        self->b = NULL;
    }
}
