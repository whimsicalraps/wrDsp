#pragma once

#include "wrLpGate.h"
#include "wrFilter.h"

typedef struct{
    float* buffer;
    float  max_time;
    float  max_samps;
    float  rate;
    float  feedback;
    lpgate_t* fb_filter;
    filter_svf_t* svf;
    filter_dc_t* dcfb;
    float  tap_fb;
    float  tap_write;
} delay_t;

delay_t* delay_init( float max_time
                   , float time
                   );
void   delay_deinit( delay_t* self );
void   delay_set_ms( delay_t* self, float time );
void   delay_set_time_percent( delay_t* self, float percent );
void   delay_set_rate( delay_t* self, float rate );
float  delay_get_ms( delay_t* self );
void   delay_set_feedback( delay_t* self, float feedback );
float  delay_get_feedback( delay_t* self );
float  delay_step( delay_t* self, float in, float phase );
float* delay_step_v( delay_t* self, float* buffer
                                  , float* phase
                                  , int    b_size
                                  );
