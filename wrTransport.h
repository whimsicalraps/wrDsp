#pragma once

#include <stdint.h>
#include "wrFilter.h" // filter_lp1_t

typedef enum{ TR_MOTOR_Standard
            , TR_MOTOR_Quick
            , TR_MOTOR_Instant
} TR_MOTOR_Speed_t;

typedef struct std_speeds{
  float max_speed;

  float accel_standard;
  float accel_quick;
  float accel_seek;
  float accel_nudge;

  float nudge_release;
} std_speeds_t;

typedef struct{
	  uint8_t      active;
    int8_t       tape_islocked;

	  filter_lp1_t speed_slew; // smoothing for speed changes
    uint16_t     b_size;     // blocks per processing frame
    float*       speed_v;    // array of speeds per sample
    float        speed_play;
    float        speed_stop;

    filter_lp1_t speed_manual; // smoothing for manual changes
    float        nudge;      // how much are we currently nudging?
	  float        nudge_accum;

    std_speeds_t speeds;
} transport_t;

uint8_t TR_init( transport_t* self, uint16_t b_size );
void TR_deinit( transport_t* self );

void TR_active( transport_t*     self
              , uint8_t          active
              , TR_MOTOR_Speed_t slew
              );
void TR_speed_stop( transport_t* self, float speed );
void TR_speed_play( transport_t* self, float speed );
void TR_nudge( transport_t* self, float delta );

uint8_t TR_is_active( transport_t* self );
float TR_get_speed( transport_t* self );

float* TR_speed_block( transport_t* self );

uint8_t TR_is_tape_moving( transport_t* self );
