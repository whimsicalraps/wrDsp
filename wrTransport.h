#pragma once

#include <stdint.h> /// CHANGE TO <stdint.h>
#include "globals.h"  // #UI_MAX_SPEED
#include "wrFilter.h" // filter_lp1_t
/// MOVE LOCK FUNCTIONS TO SOURCE (THEY ONLY APPEAR ONCE USUALLY)
/// MOVE TO A STRUCT AS INIT VARIABLES (SEE reel.c 131-130 SD_rw_access_now)
#define TR_MAX_SPEED   (UI_MAX_SPEED)
#define ACCEL_STANDARD (UI_ACCEL_NAVLIVE)
#define ACCEL_QUICK    (UI_ACCEL_CUE)
#define ACCEL_SEEK     (UI_NUDGE_NAV)
#define ACCEL_NUDGE    (UI_NUDGE_CUELIVE)
#define NUDGE_RELEASE  (UI_SEEK_RELEASE)

typedef enum{ TR_MOTOR_Standard
            , TR_MOTOR_Quick
            , TR_MOTOR_Instant
} TR_MOTOR_Speed_t;

typedef struct{
	uint8_t      active;
    int8_t       tape_end_lock;

	filter_lp1_t speed_slew; // smoothing for speed changes
    uint16_t     b_size;     // blocks per processing frame
    float*       speed_v;    // array of speeds per sample
    float        speed_play;
    float        speed_stop;

    filter_lp1_t speed_manual; // smoothing for manual changes
    float        nudge;      // how much are we currently nudging?
	float        nudge_accum;
} transport_t;

uint8_t TR_init( transport_t* self, uint16_t b_size );
/// ADD A DEINIT
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

uint8_t TR_lock_set( transport_t* self, int8_t direction );
void TR_lock_free( transport_t* self );
int8_t TR_is_locked( transport_t* self );
