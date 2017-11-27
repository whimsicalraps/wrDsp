#include "wrLpGateCli.h"

#include <stdlib.h>

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h

extern uint16_t block_size;
module_t* graph_lpgate_init( void )
{
    // allocate the graph_object
    module_t* new = malloc( sizeof(module_t) );

    // allocate the objects internal structure
    new->self = malloc( sizeof(lpgate_t) );
    lpgate_init( new->self, 0, 0, block_size );

    // metadata about the dsp object
    new->process_fnptr = g_lpgate_process;

    // metadata about the dsp object
    new->in_count = 2;
    new->ins = malloc( sizeof(m_in_t) * new->in_count );
    new->ins[0] = (m_in_t){ .src  = NULL
                          , .name = "IN"
                          };
    new->ins[1] = (m_in_t){ .src  = NULL
                          , .name = "LEVEL"
                          };

    new->out_count = 1;
    new->outs = malloc( sizeof(m_out_t) );
    new->outs[0] = (m_out_t){ .dst  = NULL
                            , .name = "OUT"
                            };
    new->par_count = 3;
    new->pars = malloc( sizeof(m_param_t) * new->par_count );
    new->pars[0] = (m_param_t){ .name = "LEVEL"
                              , .get_param = g_lpg_get_level
                              , .set_param = g_lpg_set_level
                              };
    new->pars[1] = (m_param_t){ .name = "FILTER"
                              , .get_param = g_lpg_get_filter
                              , .set_param = g_lpg_set_filter
                              };
    new->pars[2] = (m_param_t){ .name = "HPF"
                              , .get_param = g_lpg_get_hpf
                              , .set_param = g_lpg_set_hpf
                              };

    return new;
}

void g_lpgate_process( module_t* box )
{
    // this will become a fnptr when adding graph optimization

    // these buffers won't exist after sine lib upated for optimized (no in) fns
    float tmpx[block_size];
    for( int i=0; i<block_size; i++ ){
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
