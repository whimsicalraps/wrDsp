#include "wrHead.h"

#include "wrMath.h" // lim_f_0_1()
#include "wrConvert.h" // _s12_to_sf() _sf_to_s12()
#include "globals.h"   // #HEAD_SLEW_RECORD, #HEAD_SLEW_FEEDBACK, #HEAD_SLEW_MONITOR

#include "debug_usart.h"

#define BIT_HEADROOM    0
#define F_TO_TAPE_SCALE ((float)(MAX24b >> BIT_HEADROOM))

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

void _maybe_record_cvtr( rhead_t*        self
                       , float           this_cv
                       , float           that_cv
                       , motion_block_t* motion
                       )
{
    float rec_level = lp1_get_out( self->record );
    float fb_level  = lp1_get_out( self->feedback );
    float mon_level = lp1_get_out( self->monitor );

    // CV
    float ontape = _s12_to_sf( *(motion->cv_access) );
    // WRITE
    if( self->cv_recording ){
        // TODO make this average the outcome against existing cv_w if valid
        *(motion->cv_access + SD_BLOCK_CV) = _sf_to_s12( ontape * fb_level
                                                       + this_cv * rec_level
                                                       );
        *(motion->dirty) = d_write;
    } else { mon_level = 1.0; } // allow playback with no THIS attached
    // READ
    that_cv = ontape * mon_level;
}

void RH_set_rw( rhead_t* self, tape_mode_t rmode )
{
    lp1_set_dest( self->record,   (float)(rmode != READONLY)  );
    lp1_set_dest( self->feedback, (float)(rmode <= OVERDUB)   );
    lp1_set_dest( self->monitor,  (float)(rmode != OVERWRITE) );
}

uint8_t _RH_is_fresh_sample( int32_t* read_samp ){
   // find associated write_samp, then compare to #define val
   return ( *(SD_BLOCK_SAMPLES + read_samp) == INVALID_SAMP );
}

IO_block_t* RH_rw_process( rhead_t*        self
                         , int             headbuf_size
                         , float*          headbuf_audio
                         , float           this_cv
                         , float           that_cv
                         , motion_block_t* motion
                         )
{
   uint16_t  i;
   float*    input_v  = headbuf->audio;
   float*    return_v = headbuf->audio;
   int32_t** tape_r   = motion->s_access;

   if( motion->action == HEAD_Fadeout // if xfading, act on 1st
    || motion->action == HEAD_Active ){ // if !xfade, act on only head
       lp1_step_internal( self->record   );
       lp1_step_internal( self->feedback );
       lp1_step_internal( self->monitor  );
   }

   // playback mode if record is staying at 0.0
   if( lp1_get_dest( self->record ) == 0.0
    && lp1_get_out(  self->record ) == 0.0 ){
      if( motion->action == HEAD_Fadeout ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_out  = 1.0;
         for( i=0; i<(motion->s_count); i++ ){
            fade_out -= fade_step;
            *(SD_BLOCK_SAMPLES + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                          * fade_out;
         }
      } else if( motion->action == HEAD_Fadein ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_in   = 0.0;
         for( i=0; i<(motion->s_count); i++ ){
            fade_in += fade_step;
            *(SD_BLOCK_SAMPLES + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                          * fade_in;
         }
      } else {
         for( i=0; i<(motion->s_count); i++ ){
            *(SD_BLOCK_SAMPLES + *tape_r)
               = **tape_r; // 'write' action is playback
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor );
         }
      }
   } else {
      if( motion->action == HEAD_Fadeout ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_out = 1.0;

         for( i=0; i<(motion->s_count); i++ ){
            fade_out -= fade_step; // fade out
            // 'write' action
            if( _RH_is_fresh_sample(*tape_r) ){
               *(SD_BLOCK_SAMPLES + *tape_r)
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
               *(SD_BLOCK_SAMPLES + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * fade_out
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                // FEEDBACK (already copied)
                                   + *(SD_BLOCK_SAMPLES + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                        // fade_in starts to affect as self->record fades out
                          * (1.0 + (fade_out-1.0)*(1.0-lp1_get_out( self->record )));
         }
      }

      else if( motion->action == HEAD_Fadein ){
         float fade_step = 1.0 / (float)(motion->s_count - 1);
         float fade_in = 0.0;

         for( i=0; i<(motion->s_count); i++ ){
            fade_in += fade_step; // linear ramp, start step above zero
            // 'write' action
            if( _RH_is_fresh_sample(*tape_r) ){
               *(SD_BLOCK_SAMPLES + *tape_r)
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
               *(SD_BLOCK_SAMPLES + *tape_r)
                  = lim_i24_audio( (int32_t)
                                   ( lp1_get_out( self->record )
                                     * fade_in
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                 + *(SD_BLOCK_SAMPLES + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor )
                        // fade_in starts to affect as self->record fades out
                          * (1.0 + (fade_in-1.0)*(1.0-lp1_get_out( self->record )));
         }
      }

      else if( motion->action == HEAD_Active ){
         for( i=0; i<(motion->s_count); i++ ){
            // 'write' action
            if( _RH_is_fresh_sample(*tape_r) ){
               *(SD_BLOCK_SAMPLES + *tape_r)
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
               *(SD_BLOCK_SAMPLES + *tape_r)
                  = lim_i24_audio( (int32_t)
                                // INPUT
                                   ( lp1_get_out( self->record )
                                     * (*input_v++) * F_TO_TAPE_SCALE
                                   )
                                // FEEDBACK (already copied)
                                 + *(SD_BLOCK_SAMPLES + *tape_r)
                                 );
            }
            // 'read' action
            *return_v++ = iMAX24f * (float)(**tape_r++ << BIT_HEADROOM)
                          * lp1_get_out( self->monitor );
         }
      } // else {} inactive. won't happen

      // MARK DIRTY FLAG
      *(motion->dirty) = d_write;
   }
   _maybe_record_cvtr( self, this_cv, that_cv, motion );
   return headbuf;
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
