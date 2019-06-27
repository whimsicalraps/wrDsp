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



void RH_cv_recording( rhead_t* self, uint8_t state )
{
   self->cv_recording = state;
}



void RH_tr_recording( rhead_t* self, uint8_t state )
{
   self->tr_recording = state;
}




void RH_set_rw( rhead_t* self, tape_mode_t rmode )
{
    lp1_set_dest( self->record,   (float)(rmode != READONLY)  );
    lp1_set_dest( self->feedback, (float)(rmode <= OVERDUB)   );
    lp1_set_dest( self->monitor,  (float)(rmode != OVERWRITE) );
}




void RH_set_record_level( rhead_t* self, float level )
{
   lp1_set_dest( self->record, level );
}




void RH_set_record_params( rhead_t* self
                         , float    feedback
                         , float    monitor
                         )
{
   lp1_set_dest( self->feedback, feedback );
   lp1_set_dest( self->monitor,  monitor  );
}
