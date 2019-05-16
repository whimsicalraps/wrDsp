#pragma once

#include <stm32f7xx.h>

#include "wrFilter.h" // filter_lp1_t

typedef struct{
    filter_lp1_t* record;
    filter_lp1_t* feedback;
    filter_lp1_t* monitor;

    uint8_t       cv_recording;
    uint8_t       tr_recording;
} rhead_t;

rhead_t* RH_Init( void );
void RH_DeInit( rhead_t* self );

void RH_cv_recording( rhead_t* self, uint8_t state );
void RH_tr_recording( rhead_t* self, uint8_t state );

void RH_set_rw( rhead_t* self, tape_mode_t rmode );
IO_block_t* RH_rw_process( rhead_t*        self
                         , IO_block_t*     headbuf
                         , motion_block_t* motion
                         );
void RH_set_record_level( rhead_t* self, float level );
void RH_set_record_params( rhead_t* self
                         , float    feedback
                         , float    monitor
                         );
