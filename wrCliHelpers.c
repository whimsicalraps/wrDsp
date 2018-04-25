#include "wrCliHelpers.h"

#include <string.h>
#include <stdio.h>

module_t* cli_module_init( size_t    struct_size
                         , init_fn_t init_fnptr
                         , g_proc_t  graph_process_fn
                         )
{
//fprintf(stderr, "init\n\r" );
    module_t* box = malloc( sizeof(module_t) );
    // allocate the objects internal structure
    if( struct_size > 0 ){ box->self = malloc( struct_size ); }
    if( init_fnptr != NULL ){ init_fnptr( box->self ); }
    if( graph_process_fn != NULL ){
        box->process_fnptr = graph_process_fn;
    }
    box->ins  = NULL;
    box->outs = NULL;
    box->pars = NULL;
    box->in_count  = 0;
    box->out_count = 0;
    box->par_count = 0;

    return box;
}

void cli_register_input( module_t* box
                       , float*    src
                       , char*     name
                       )
{
    uint16_t ix = box->in_count++;
    box->ins = realloc( box->ins, (1+ix) * sizeof(m_in_t) );
    box->ins[ix].src  = src;
    memcpy( box->ins[ix].name
          , name
          , NAME_LEN_LIM * sizeof(char)
          );
}

void cli_register_output( module_t* box
                        , char*     name
                        , int       b_size
                        )
{
    uint16_t ix = box->out_count++;
    box->outs = realloc( box->outs, (1+ix) * sizeof(m_out_t) );
    float* buf = malloc( sizeof(float)*b_size );
    //fprintf(stderr, "%p", buf);
    box->outs[ix].dst = buf;
    memcpy( box->outs[ix].name
          , name
          , NAME_LEN_LIM * sizeof(char)
          );
}

void cli_register_param( module_t*  box
                       , g_getter_t get
                       , g_setter_t set
                       , char*      name
                       )
{
    uint16_t ix = box->par_count++;
    box->pars = realloc( box->pars, (1+ix) * sizeof(m_param_t) );
    box->pars[ix] = (m_param_t){ .get_param = get
                               , .set_param = set
                               };
    memcpy( box->pars[ix].name
          , name
          , NAME_LEN_LIM * sizeof(char)
          );
}
