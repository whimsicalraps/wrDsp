#pragma once

#include <stdint.h>
#include "wrFilter.h" // filter_lp1_t

typedef enum{ transport_motor_standard
            , transport_motor_quick
            , transport_motor_instant
} transport_motor_speed_t;

// TODO: perhaps some of these shouldn't be floats?
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

uint8_t transport_init( transport_t* self, uint16_t b_size );
void transport_deinit( transport_t* self );

void transport_active( transport_t*     self
              , uint8_t          active
              , transport_motor_speed_t slew
              );
void transport_speed_stop( transport_t* self, float speed );
void transport_speed_play( transport_t* self, float speed );
void transport_nudge( transport_t* self, float delta );

uint8_t transport_is_active( transport_t* self );
float transport_get_speed( transport_t* self );

float* transport_speed_block( transport_t* self );

uint8_t transport_is_tape_moving( transport_t* self );
