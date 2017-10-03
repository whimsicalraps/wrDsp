#include "wrFilterCli.h"

#include <stdlib.h>

// Wrapper Init Function
// -- this section is the Ex wrapper that should exist in parallel to normal lib
//#include "dsp_block.h"

// this seems so generic it shouldn't have to be done per module
// really just need sizeof(filter_lp1_t) and fnptr to lp1_init
// >> this would mean we don't need a *Ex.c at all, only *Ex.h

module_t* graph_lp1_init( void )
{
    // allocate the graph_object
    module_t* new = malloc( sizeof(module_t) );
//    new->id       = id;

    // allocate the objects internal structure
    new->self = malloc( sizeof(filter_lp1_t) );
    lp1_init( new->self );

    // metadata about the dsp object
    // ??

    return new;
}
