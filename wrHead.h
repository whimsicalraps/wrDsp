#pragma once

#include "wrResamp.h" // IO_block_t
#include "wrFilter.h" // filter_lp1_t
#include "wrEvent.h"

typedef enum{ READONLY
	        , OVERDUB
	        , OVERWRITE
	        , LOOPER
	        , tape_mode_count
} tape_mode_t;

typedef enum{ head_inactive = -1
            , head_active
            , head_fadein
            , head_fadeout
} head_action_t;

typedef struct{
    int32_t**      s_access;   // reel->samp_access
    int            s_count;    // reel->hb_count
    int            s_origin;   // reel->hb_origin
    float          s_interp;   // reel->pos_subsamp
    int16_t*       cv_access;
    etrig_t*       tr_access;
} motion_block_t; // tape.h, reel.h, rhead.h

typedef struct{
    head_action_t  action;     // flag to set activeness, and xfade motion

    filter_lp1_t* record;
    filter_lp1_t* feedback;
    filter_lp1_t* monitor;

    uint8_t       cv_recording;
    uint8_t       tr_recording;

    uint32_t      SD_block;
    uint32_t      SD_cv;
} head_t;

head_t* head_init( void );
void head_deinit( head_t* self );

void head_cv_recording( head_t* self, uint8_t state );
void head_tr_recording( head_t* self, uint8_t state );

/*void head_record_cvtr( head_t*        self
                    , cv_block_t*     block
                    , motion_block_t* motion
                  );*/

void head_set_rw( head_t* self, tape_mode_t rmode );
IO_block_t* head_rw_process( head_t*        self
                         , IO_block_t*     headbuf
                         , motion_block_t* motion
                         );
void head_set_record_level( head_t* self, float level );
void head_set_record_params( head_t* self
                         , float    feedback
                         , float    monitor
                         );
