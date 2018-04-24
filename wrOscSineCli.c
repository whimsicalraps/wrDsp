#include "wrOscSineCli.h"

#include "wrCliHelpers.h"
#include <stdlib.h>

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h

module_t* graph_osc_sine_init( void )
{
// METADATA
    module_t* box = cli_module_init( sizeof(osc_sine_t)
                                   , (void*)osc_sine_init
                                   , g_osc_sine_process
                                   );
// INS
    cli_register_input( box, NULL, "EXPO FM"   );
    cli_register_input( box, NULL, "LINEAR FM" );
// OUTS
    cli_register_output( box, "OUT" );
// PARAMS
    cli_register_param( box, g_osc_sine_get_pitch
                           , g_osc_sine_set_pitch
                           , "PITCH"
                           );
    return box;
}
//extern uint16_t block_size;
void g_osc_sine_process( module_t* box )
{
    // this will become a fnptr when adding graph optimization

    // these buffers won't exist after sine lib upated for optimized (no in) fns
    float tmpx[block_size];
    float tmpy[block_size];
    for( int i=0; i<block_size; i++ ){
        tmpx[i] = 1.0;
        tmpy[i] = 0.0;
    }
    osc_sine_process_v( box->self
                      , block_size
                      , tmpx //box->ins[0].src
                      , tmpy //box->ins[1].src
                      , box->outs[0].dst
                      );
}
void g_osc_sine_set_pitch( void* f, float p )
{
    osc_sine_time( (osc_sine_t*)f, p );
}
float g_osc_sine_get_pitch(void* f)
{
    return ((osc_sine_t*)f)->rate;
}

