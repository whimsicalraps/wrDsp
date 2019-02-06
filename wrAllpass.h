#pragma once

#include <stdint.h>

typedef struct{
    float* buffer;
    float  max_time;
    float  time;
    float  gain;
    // private
    int    max_samps;
    int    read;
    int    write;
} allpass_t;

int    allpass_init(     allpass_t* self, float max_time
                                        , float time
                                        );
void   allpass_set_ms(   allpass_t* self, float time );
float  allpass_get_ms(   allpass_t* self );
void   allpass_set_gain( allpass_t* self, float gain );
float  allpass_get_gain( allpass_t* self );
float  allpass_step(     allpass_t* self, float in );
float* allpass_step_v(   allpass_t* self, float*   in
                                        , uint16_t size
                                        );
