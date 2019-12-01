#include "wrBuffer.h"

#include <stdlib.h>
#include <stdio.h>

////////////////////////////////
// private declarations

static void buffer_free( buffer_t* self );


////////////////////////////////
// public interface defns

buffer_t* buffer_init( int count )
{
    buffer_t* self = malloc( sizeof( buffer_t ) );
    if( !self ){ printf("buffer_init malloc failed.\n"); return NULL; }

    self->len = 0;
    self->b   = NULL;
    buffer_new( self, count );
    return self;
}

void buffer_deinit( buffer_t* self )
{
    buffer_free( self );
    free(self); self = NULL;
}

buffer_t* buffer_new( buffer_t* self, int length )
{
    buffer_free( self );
    if( length ){ // zero-length just clears buffer
        self->b = malloc( sizeof( float ) * length );
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
