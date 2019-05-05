#include "wrTransport.h"

#include <stdlib.h>
#include <stdio.h> // printf
#include "wrMath.h"


transport_t* transport_init( uint16_t b_size )
{
    transport_t* self = malloc( sizeof( transport_t ) );
    if( !self ){ printf("wrTransport malloc failed\n"); return NULL; }

    self->active        = 0;
    self->tape_islocked = 0;

    // default speed values
    self->speeds = (std_speeds_t) {
        .max_speed = 2.0
        , .accel_standard = 0.001
        , .accel_quick = 0.05
        , .accel_seek = 0.001
        , .accel_nudge = 0.005
        , .nudge_release = 0.002
    };

    self->speed_slew = lp1_init();
    lp1_set_coeff( self->speed_slew, (self->speeds).accel_standard );

    self->b_size = b_size;
    uint16_t speed_len = (b_size + 1);
    self->speed_v = malloc(sizeof(float) * speed_len );
    if( !self->speed_v ){ printf("tr_speed_v malloc!\n"); return NULL; }

    for( uint16_t i=0; i<speed_len; i++ ){ self->speed_v[i] = 0.0; }
    self->speed_active   = 1.0;
    self->speed_inactive   = 0.0;

    self->speed_manual = lp1_init();
    lp1_set_coeff( self->speed_manual, (self->speeds).accel_seek );
    self->nudge        = 0.0;
    self->nudge_accum  = 0.0;

    return self;
}


void transport_deinit( transport_t* self )
{
    free(self->speed_v);
    free(self);
}


void transport_active( transport_t*     self
              , uint8_t          active
              , transport_motor_speed_t slew
              )
{
    self->active = !!active;
    lp1_set_coeff( self->speed_manual
                 , (self->active)
                        ? (self->speeds).accel_nudge
                        : (self->speeds).accel_seek
                 );
    lp1_set_coeff( self->speed_slew
                 , (slew == transport_motor_standard)
                        ? (self->speeds).accel_standard
                        : (self->speeds).accel_quick
                 );
    if( slew == transport_motor_instant ){
        lp1_set_out( self->speed_slew
                   , transport_get_speed( self )
                   );
    }
    if( self->active ){
        //switch statement unlocks tape
        switch( self->tape_islocked ){
            case -1: if( transport_get_speed( self ) > 0.0 ){ self->tape_islocked = 0; } break;
            case  1: if( transport_get_speed( self ) < 0.0 ){ self->tape_islocked = 0; } break;
            default: break;
        }
    }
}


void transport_speed_inactive( transport_t* self, float speed )
{
    float tmin = -(self->speeds).max_speed;
    float tmax =  (self->speeds).max_speed;
    switch( self->tape_islocked ){
        case -1:
            if( speed > 0.0 ){ self->tape_islocked = 0; }
            else{ tmin = 0.0; }
            break;
        case  1:
            if( speed < 0.0 ){ self->tape_islocked = 0; }
            else{ tmax = 0.0; }
            break;
        default: break;
    }
    self->speed_inactive = lim_f( speed
                            , tmin
                            , tmax
                            );
}


void transport_speed_active( transport_t* self, float speed )
{
    self->speed_active = lim_f( speed
                            , -(self->speeds).max_speed
                            ,  (self->speeds).max_speed
                            );
}


void transport_nudge( transport_t* self, float delta )
{
    //switch statement unlocks tape
    self->nudge = delta;
    switch( self->tape_islocked ){
        case -1: if( delta > 0.0 ){ self->tape_islocked = 0; } break;
        case  1: if( delta < 0.0 ){ self->tape_islocked = 0; } break;
        default: break;
    }
}


uint8_t transport_is_active( transport_t* self )
{
    return (self->active);
}


float transport_get_speed( transport_t* self )
{
    return ((self->active)
                ? self->speed_active
                : self->speed_inactive);
}


float* transport_speed_block( transport_t* self )
{
    lp1_set_dest( self->speed_slew
                , transport_get_speed( self )
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
                                     , -(self->speeds).max_speed
                                     ,  (self->speeds).max_speed
                                     );
        }
    } else {
        if( self->nudge_accum >= 0.0 ){
            self->nudge_accum = lim_f( self->nudge_accum - (self->speeds).nudge_release
                                     , 0.0
                                     , (self->speeds).max_speed
                                     );
        } else {
            self->nudge_accum = lim_f( self->nudge_accum + (self->speeds).nudge_release
                                     , -(self->speeds).max_speed
                                     , 0.0
                                     );
        }
    }

    // limit nudged speed to +/-2.0
    float tmax = (self->speeds).max_speed;
    float tmin = -(self->speeds).max_speed;
    switch( self->tape_islocked ){
        case -1: tmin = 0.0; break;
        case  1: tmax = 0.0; break;
        default: break;
    }
    lp1_set_out( self->speed_slew
          , lim_f( lp1_get_out( self->speed_slew ) + self->nudge_accum
                 , tmin
                 , tmax
                 ) );

    // slew toward new speed
    lp1_step_c_v( self->speed_slew
                , self->speed_v
                , self->b_size );

    return self->speed_v;
}


uint8_t transport_is_tape_moving( transport_t* self )
{
    float speed = lp1_get_out( self->speed_slew );
    if( speed < -nFloor
        || speed > nFloor ){ return 1; }
    return 0;
}
