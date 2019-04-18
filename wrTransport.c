#include "wrTransport.h"

#include <stdlib.h>
#include "wrMath.h"

// Private declarations
void _TR_tape_unlock( transport_t* self, float dir );

// Public definitions
uint8_t TR_init( transport_t* self, uint16_t b_size )
{
    uint8_t error = 0;

    self->active        = 0;
    self->tape_end_lock = 0;

    lp1_init(      &(self->speed_slew) );
    lp1_set_coeff( &(self->speed_slew), ACCEL_STANDARD );

    self->b_size = b_size;
    uint16_t speed_len = (b_size + 1);
    self->speed_v = malloc(sizeof(float) * speed_len );

    if( self->speed_v == NULL ){ error = 1; }
    for( uint16_t i=0; i<speed_len; i++ ){ self->speed_v[i] = 0.0; }
    self->speed_play   = 1.0;
    self->speed_stop   = 0.0;

    lp1_init(      &(self->speed_manual) );
    lp1_set_coeff( &(self->speed_manual), ACCEL_SEEK );
    self->nudge        = 0.0;
    self->nudge_accum  = 0.0;

    return error;
}

void TR_active( transport_t*     self
              , uint8_t          active
              , TR_MOTOR_Speed_t slew
              )
{
    self->active = !!active;
    lp1_set_coeff( &(self->speed_manual)
                 , (self->active)
                        ? ACCEL_NUDGE
                        : ACCEL_SEEK
                 );
    lp1_set_coeff( &(self->speed_slew)
                 , (slew == TR_MOTOR_Standard)
                        ? ACCEL_STANDARD
                        : ACCEL_QUICK
                 );
    if( slew == TR_MOTOR_Instant ){
        lp1_set_out( &(self->speed_slew)
                   , TR_get_speed( self )
                   );
    }
    if( self->active ){
        _TR_tape_unlock( self, TR_get_speed( self ) );
    }
}

void TR_speed_stop( transport_t* self, float speed )
{
    float tmin = -TR_MAX_SPEED;
    float tmax =  TR_MAX_SPEED;
    switch( TR_is_locked( self ) ){
        case -1:
            if( speed > 0.0 ){ TR_lock_free(self); }
            else{ tmin = 0.0; }
            break;
        case  1:
            if( speed < 0.0 ){ TR_lock_free(self); }
            else{ tmax = 0.0; }
            break;
        default: break;
    }
    self->speed_stop = lim_f( speed
                            , tmin
                            , tmax
                            );
}

void TR_speed_play( transport_t* self, float speed )
{
    self->speed_play = lim_f( speed
                            , -TR_MAX_SPEED
                            ,  TR_MAX_SPEED
                            );
}

void TR_nudge( transport_t* self, float delta )
{
    self->nudge = delta;
    _TR_tape_unlock( self, delta );
}

uint8_t TR_is_active( transport_t* self )
{
    return (self->active);
}

float TR_get_speed( transport_t* self )
{
    return ((self->active)
                ? self->speed_play
                : self->speed_stop);
}
float* TR_speed_block( transport_t* self )
{
    lp1_set_dest( &(self->speed_slew)
                , TR_get_speed( self )
                );

    // apply nudge / seek
    if( self->nudge ){
        if( self->active ){ // only nudge!
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -0.01
                                     ,  0.01
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum + self->nudge
                                     , -TR_MAX_SPEED
                                     ,  TR_MAX_SPEED
                                     );
        }
    } else {
        if( self->nudge_accum >= 0.0 ){
            self->nudge_accum = lim_f( self->nudge_accum - NUDGE_RELEASE
                                     , 0.0
                                     , TR_MAX_SPEED
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum + NUDGE_RELEASE
                                     , -TR_MAX_SPEED
                                     , 0.0
                                     );
        }
    }

    // self->speed_slew.y += lp1_step( &(self->speed_manual), self->nudge );
    // self->speed_slew.y += self->nudge_accum;

    // limit nudged speed to +/-2.0
    float tmax = TR_MAX_SPEED;
    float tmin = -TR_MAX_SPEED;
    switch( TR_is_locked( self ) ){
        case -1: tmin = 0.0; break;
        case  1: tmax = 0.0; break;
        default: break;
    }
    lp1_set_out( &(self->speed_slew)
               , lim_f( lp1_get_out( &(self->speed_slew) ) + self->nudge_accum
                      , tmin
                      , tmax
                      ) );

    // slew toward new speed
    lp1_step_c_v( &(self->speed_slew)
                , self->speed_v
                , self->b_size );

    return self->speed_v;
}

uint8_t TR_is_tape_moving( transport_t* self )
{
    float speed = lp1_get_out( &(self->speed_slew) );
    if( speed < -nFloor
     || speed > nFloor ){ return 1; }
    return 0;
}

uint8_t TR_lock_set( transport_t* self, int8_t direction )
{
    if( direction ){
        self->tape_end_lock = direction;
        return 1;
    }
    return 0;
}
void TR_lock_free( transport_t* self )
{
    self->tape_end_lock = 0;
}
int8_t TR_is_locked( transport_t* self )
{
    return self->tape_end_lock;
}

// Private definitions
void _TR_tape_unlock( transport_t* self, float dir )
{
    switch( TR_is_locked( self ) ){
        case -1: if( dir > 0.0 ){ TR_lock_free(self); } break;
        case  1: if( dir < 0.0 ){ TR_lock_free(self); } break;
        default: break;
    }
}
