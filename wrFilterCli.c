#include "wrFilterCli.h"

#include <stdlib.h>

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h
//
// load up an enum or struct in header & use that in a generic init fn
// this will include i/o names, pointers to process fns etc

module_t* graph_lp1_init( void )
{
    // allocate the graph_object
    module_t* new = malloc( sizeof(module_t) );

    new->process_fnptr = g_lp1_process;

    // allocate the objects internal structure
    new->self = malloc( sizeof(filter_lp1_t) );
    lp1_init( new->self );

    // metadata about the dsp object
    // autofill when using 'replace' or auto-patching moments
    new->in_count = 1;
    new->ins = malloc( sizeof(m_in_t) );
    new->ins[0] = (m_in_t){ .src  = NULL
                          , .name = "IN"
                          };
    new->out_count = 1;
    new->outs = malloc( sizeof(m_out_t) );
    new->outs[0] = (m_out_t){ .dst  = NULL
                            , .name = "OUT"
                            };
    new->par_count = 1;
    new->pars = malloc( sizeof(m_param_t) );
    new->pars[0] = (m_param_t){ .name = "COEFF"
                              , .get_param = g_lp1_get_coeff
                              , .set_param = g_lp1_set_coeff
                              };

    return new;
}
extern uint16_t block_size;
void g_lp1_process( module_t* box )
{
    // this will become a fnptr when adding graph optimization
    lp1_step_v( box->self
              , box->ins[0].src
              , box->outs[0].dst
              , block_size
              );
}
void g_lp1_set_coeff( void* f, float c )
{
    lp1_set_coeff( (filter_lp1_t*)f, c );
}
float g_lp1_get_coeff(void* f)
{
    return ((filter_lp1_t*)f)->c;
}

