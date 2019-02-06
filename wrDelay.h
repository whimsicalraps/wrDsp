#pragma once

#include <stdint.h>

typedef struct{
    float* buffer;
    float  max_time;
    float  time;
    float  feedback;
    // private
    int    max_samps;
    int    read;
    int    write;
} delay_t;

int    delay_init(     delay_t* self, float max_time
                                    , float time
                                    );
void   delay_set_ms(   delay_t* self, float time );
float  delay_get_ms(   delay_t* self );
void   delay_set_feedback( delay_t* self, float feedback );
float  delay_get_feedback( delay_t* self );
float  delay_step(     delay_t* self, float in );
float* delay_step_v(   delay_t* self, float*   in
                                    , uint16_t size
                                    );
