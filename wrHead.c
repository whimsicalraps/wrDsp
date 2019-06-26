#include "wrHead.h"
#include <stdlib.h>
#include <stdio.h>

rhead_t* RH_Init( void )
{
   rhead_t* self = malloc( sizeof( rhead_t ) );
   if( !self ){ printf("RH: malloc failed\n"); return NULL; }

   self->record   = lp1_init();
   self->feedback = lp1_init();
   self->monitor  = lp1_init();
   // monitor needs to default to on
   lp1_set_dest( self->feedback, 1.0 );
   lp1_set_out(  self->feedback, 1.0 );
   lp1_set_dest( self->monitor,  1.0 );
   // slew times. can tune each individually?
   lp1_set_coeff( self->record,   HEAD_SLEW_RECORD   );
   lp1_set_coeff( self->feedback, HEAD_SLEW_FEEDBACK );
   lp1_set_coeff( self->monitor,  HEAD_SLEW_MONITOR  );

   self->cv_recording = 0;
   self->tr_recording = 0;
   return self;
}

void RH_DeInit( rhead_t* self )
{
    lp1_deinit( self->monitor );
    lp1_deinit( self->feedback );
    lp1_deinit( self->record );
    free(self);
}
