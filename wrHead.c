#include "wrHead.h"

#include "wrMath.h" // lim_f_0_1()
#include "wrConvert.h" // _s12_to_sf() _sf_to_s12()
#include <stdio.h>

#define BIT_HEADROOM    0

#define HEAD_SLEW_RECORD   ((float)(0.05))
#define HEAD_SLEW_FEEDBACK ((float)(0.05))
#define HEAD_SLEW_MONITOR  ((float)(0.05))

#define INVALID_SAMP 0x7FFFFFFF

#define F_TO_TAPE_SCALE ((float)(MAX24b >> BIT_HEADROOM))

head_t* head_init( void )
{
   head_t* self = malloc( sizeof( head_t ) );
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

   self->SD_block = 2688;
   self->SD_cv = 64;

   return self;
}

void head_deinit( head_t* self )
{
    lp1_deinit( self->monitor );
    lp1_deinit( self->feedback );
    lp1_deinit( self->record );
    free(self);
}

void head_cv_recording( head_t* self, uint8_t state )
{
   self->cv_recording = state;
}

void head_tr_recording( head_t* self, uint8_t state )
{
   self->tr_recording = state;
}


/*
    Not exactly sure how to break this CV stuff up because all
    the original code in the function seems highly specific to
    the W/ module. I moved it all to lines 409-425 of tape.c but
    we could maybe use some portion of that in this functino
*/

/*
void head_record_cvtr( head_t*        self
                    , cv_block_t*     block
                    , motion_block_t* motion
                    )
{

}
*/

void head_set_rw( head_t* self, tape_mode_t rmode )
{
    lp1_set_dest( self->record,   (float)(rmode != READONLY)  );
    lp1_set_dest( self->feedback, (float)(rmode <= OVERDUB)   );
    lp1_set_dest( self->monitor,  (float)(rmode != OVERWRITE) );
}

void head_set_SD_block(head_t* self, int SD_block)
{
  self->SD_block = SD_block;
}

void head_set_SD_CV(head_t* self, int SD_cv)
{
  self->SD_cv = SD_cv;
}

uint8_t _head_is_fresh_sample( head_t* self, int32_t* read_samp ){
   // find associated write_samp, then compare to #define val
   return ( *(self->SD_block + read_samp) == INVALID_SAMP );
}

IO_block_t* head_rw_process( head_t*      self
                         , IO_block_t*   headbuf
                         , motion_block_t* motion
                         )
{
   uint16_t  i;
   float*    input_v  = headbuf->audio;
   float*    return_v = headbuf->audio;
   int32_t** tape_r   = motion->s_access;

   if( self->action == head_fadeout // if xfading, act on 1st
    || self->action == head_active ){ // if !xfade, act on only head
       lp1_step_internal( self->record   );
       lp1_step_internal( self->feedback );
       lp1_step_internal( self->monitor  );
   }

   // playback mode if record is staying at 0.0
   if( lp1_get_dest( self->record ) == 0.0
    && lp1_get_out(  self->record ) == 0.0 ){
      if( self->action == head_fadeout ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_out  = 1.0;
         for( i=0; i<(motion->s_count); i++ ){
            fade_out -= fade_step;
            *(self->SD_block + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                          * fade_out;
         }
      } else if( self->action == head_fadein ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_in   = 0.0;
         for( i=0; i<(motion->s_count); i++ ){
            fade_in += fade_step;
            *(self->SD_block + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                          * fade_in;
         }
      } else {
         for( i=0; i<(motion->s_count); i++ ){
            *(self->SD_block + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor );
         }
      }
   } else {
      if( self->action == head_fadeout ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_out = 1.0;

         for( i=0; i<(motion->s_count); i++ ){
            fade_out -= fade_step; // fade out
            // 'write' action
            if( _head_is_fresh_sample(self, *tape_r) ){
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * fade_out
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                // SCALE FEEDBACK TOWARD PLAYBACK
                                   + ( ( lp1_get_out( self->feedback )-1.0)
                                       * lp1_get_out( self->record )
                                       + 1.0
                                // FEEDBACK
                                     ) * (float)(**tape_r)
                                   )
                                 );
            } else {
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * fade_out
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                // FEEDBACK (already copied)
                                   + *(self->SD_block + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                        // fade_in starts to affect as self->record fades out
                          * (1.0 + (fade_out-1.0)*(1.0-lp1_get_out( self->record )));
         }
      }

      else if( self->action == head_fadein ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_in = 0.0;

         for( i=0; i<(motion->s_count); i++ ){
            fade_in += fade_step; // linear ramp, start step above zero
            // 'write' action
            if( _head_is_fresh_sample(self, *tape_r) ){
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT * FADEIN
                                   ( lp1_get_out( self->record )
                                     * fade_in
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                // SCALED FEEDBACK TOWARD PLAYBACK
                                   + ( ( lp1_get_out( self->feedback )-1.0)
                                       * lp1_get_out( self->record )
                                       + 1.0
                                     )
                                // FEEDBACK (already copied)
                                     * (float)(**tape_r)
                                   )
                                 );
            } else {
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                   ( lp1_get_out( self->record )
                                     * fade_in
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                 + *(self->SD_block + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                        // fade_in starts to affect as self->record fades out
                          * (1.0 + (fade_in-1.0)*(1.0-lp1_get_out( self->record )));
         }
      }

      else if( self->action == head_active ){
         for( i=0; i<(motion->s_count); i++ ){
            // 'write' action
            if( _head_is_fresh_sample(self, *tape_r) ){
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                // SCALE FEEDBACK TOWARD PLAYBACK
                                   + ( ( lp1_get_out( self->feedback )-1.0)
                                       * lp1_get_out( self->record )
                                       + 1.0
                                     )
                                // FEEDBACK
                                     * (float)(**tape_r)
                                   )
                                 );
            } else {
               *(self->SD_block + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                // FEEDBACK (already copied)
                                 + *(self->SD_block + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor );
         }
      } // else {} inactive. won't happen
   }
   return headbuf;
}

void head_set_record_level( head_t* self, float level )
{
   lp1_set_dest( self->record, level );
}

void head_set_record_params( head_t* self
                         , float    feedback
                         , float    monitor
                         )
{
   lp1_set_dest( self->feedback, feedback );
   lp1_set_dest( self->monitor,  monitor  );
}
