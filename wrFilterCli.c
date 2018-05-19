#include "wrFilterCli.h"

#include <stdlib.h>

module_t* graph_lp1_init( int b_size )
{
// METADATA
    module_t* box = cli_module_init( sizeof(filter_lp1_t)
                                   , (void*)lp1_init
                                   , g_lp1_process
                                   );
// INS
    cli_register_input( box, NULL, "IN" );
// OUTS
    cli_register_output( box, "OUT", b_size );
// PARAMS
    cli_register_param( box, g_lp1_get_coeff
                           , g_lp1_set_coeff
                           , "COEFF"
                           );
    return box;
}
//extern uint16_t block_size;
void g_lp1_process( module_t* box, int b_size )
{
    // this will become a fnptr when adding graph optimization
    lp1_step_v( box->self
              , box->ins[0].src
              , box->outs[0].dst
              , b_size
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

