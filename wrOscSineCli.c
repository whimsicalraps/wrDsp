#include "wrOscSineCli.h"

#include <stdlib.h>

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h

module_t* graph_osc_sine_init( void )
{
    // allocate the graph_object
    module_t* new = malloc( sizeof(module_t) );

    // allocate the objects internal structure
    new->self = malloc( sizeof(osc_sine_t) );
    osc_sine_init( new->self );

    new->process_fnptr = g_osc_sine_process;

    // metadata about the dsp object
    new->in_count = 2;
    new->ins = malloc( sizeof(m_in_t) * 2 );
    new->ins[0] = (m_in_t){ .src  = NULL
                          , .name = "EXPO FM"
                          };
    new->ins[1] = (m_in_t){ .src  = NULL
                          , .name = "LINEAR FM"
                          };

    new->out_count = 1;
    new->outs = malloc( sizeof(m_out_t) );
    new->outs[0] = (m_out_t){ .dst  = NULL
                            , .name = "OUT"
                            };
    new->par_count = 1;
    new->pars = malloc( sizeof(m_param_t) );
    new->pars[0] = (m_param_t){ .name = "PITCH"
                              , .get_param = g_osc_sine_get_pitch
                              , .set_param = g_osc_sine_set_pitch
                              };

    return new;
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

