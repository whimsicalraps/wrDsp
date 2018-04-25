#include "wrLpGateCli.h"

#include <stdlib.h>
#include "wrCliHelpers.h"

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h

void _lpgate_init_wrapper( void* data )
{
    // THIS BLOCK_SIZE DEFINE IS INCORRECT
    // INIT FNS NEED TO BE SENT THE B_SIZE
    int b_size = 256; // total guess
    lpgate_init( data, 0, 0, b_size );
}

module_t* graph_lpgate_init( int b_size )
{
// METADATA
    module_t* box = cli_module_init( sizeof(lpgate_t)
                                   , (void*)_lpgate_init_wrapper
                                   , g_lpgate_process
                                   );
// INS
    cli_register_input( box, NULL, "IN"   );
    cli_register_input( box, NULL, "LEVEL" );
// OUTS
    cli_register_output( box, "OUT", b_size );
// PARAMS
    cli_register_param( box, g_lpg_get_level
                           , g_lpg_set_level
                           , "LEVEL"
                           );
    cli_register_param( box, g_lpg_get_filter
                           , g_lpg_set_filter
                           , "FILTER"
                           );
    cli_register_param( box, g_lpg_get_hpf
                           , g_lpg_set_hpf
                           , "HPF"
                           );
    return box;
}

void g_lpgate_process( module_t* box, int b_size )
{
    // this will become a fnptr when adding graph optimization

    // these buffers won't exist after sine lib upated for optimized (no in) fns
    float tmpx[b_size];
    for( int i=0; i<b_size; i++ ){
        tmpx[i] = 1.0;
    }
    lpgate_v( box->self
            , tmpx //box->ins[1].src
            , box->ins[0].src
            , box->outs[0].dst
            );
}
void g_lpg_set_level( void* self, float l )
{
    lpgate_set_level( (lpgate_t*)self, l );
}
float g_lpg_get_level( void* self )
{
    return ((lpgate_t*)self)->level;
}
void g_lpg_set_filter( void* self, float f )
{
    lpgate_filter_mode( (lpgate_t*)self, (uint8_t)(f > 0.5) );
}
float g_lpg_get_filter( void* self )
{
    return (float)(((lpgate_t*)self)->filter);
}
void g_lpg_set_hpf( void* self, float h )
{
    lpgate_hpf_mode( (lpgate_t*)self, (uint8_t)(h > 0.5) );
}
float g_lpg_get_hpf( void* self )
{
    return (float)(((lpgate_t*)self)->hpf);
}
