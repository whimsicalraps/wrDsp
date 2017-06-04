#include <math.h>
#include <wrFuncGen.h>
#include <wrMath.h>

void function_init( func_gen_t* self, int8_t loop )
{
	self->go         = 0; // stopped
	self->id         = 0;
	self->rate       = 0.05;
	self->tr_state   = 0;
	self->fm_ix      = 0;
	self->loop       = loop;
	if(loop != 0){
		self->go = 1; // start if looping
	}

	self->r_up       = 1;
	self->r_down     = 1;
	// unimplemented
	// self->sustain = 0;
}

// Param Functions

// Called on trigger edges only
void function_trig( func_gen_t* self, uint8_t state )
{
	// do a switch statement based on mode bitmask
	/*switch (self->mode) {
	case XXX :
		//
		break;
	}*/
}

void function_rate( func_gen_t* self, float rate )
{
	self->rate = lim_f_0_1( rate );
}

// Audio Rate Process (helper function)
void function_ramp( func_gen_t* self, float skew )
{
	math_get_ramps( skew,
					&(self->r_up),
					&(self->r_down));
	// self->r_up = 0.5 / (0.998 * level + 0.001);
	// self->r_down = 1/ (2- (1/ self->r_up));
}

void function_ramp_v( uint16_t b_size, float ctrl_rate, float* audio_rate, float* ramp_up, float* ramp_down )
{
	float* audio_rate2 = audio_rate;
	float* ramp_up2 = ramp_up;
	float* ramp_down2 = ramp_down;

	for(uint16_t i=0;i<b_size;i++){
		*ramp_up2 = 0.5 / (0.998 * lim_f(ctrl_rate + *audio_rate2++,0,1) + 0.001);
		*ramp_down2++ = 1/ (2- (1/ *ramp_up2++));
	}
}

float function_step( func_gen_t* self, float fm_in )
{
	if( self->go != 0 ){
		float move;
		
		// determine rate based on direction
		if( self->id >= 0 ){
			move = self->rate * self->r_up + (fm_in * self->fm_ix); // (+ phase mod)
		} else {
			move = self->rate * self->r_down + (fm_in * self->fm_ix);
		}
		// increment w/ overflow protection
		while( move != 0 ){
			if( self->id >= 0 ){ // are we above zero BEFORE moving
				self->id += move;
				move = 0;
				if( self->id >= 1.0 ){
					move = (self->id - 1.0) * self->r_down / self->r_up;
					self->id = -1.0;
				} else if( self->id < 0.0 ){
					if( self->loop != 0 ){
						move = self->id * self->r_down / self->r_up;
						if( self->loop > 0 ) { self->loop += 1; }
					}
					self->id = 0;
				}
			} else {
				self->id += move;
				move = 0;
				if( self->id >= 0 ){
					if( self->loop != 0 ){
						move = self->id * self->r_up / self->r_down;
						if( self->loop > 0 ) { self->loop -= 1; }
					}
					self->id = 0;
				} else if( self->id < -1.0 ){
					move = (self->id + 1.0) * self->r_up / self->r_down;
					self->id = 1.0;
				}
			}
		}
	}
	return self->id;
	// NB: this is not a triangle!
	// assymmetric waveform for flagless rise/fall detection
	// needs the following function to convert to ramp->tri->saw wave
		// float scale = (self->id - 0.5) * 2.0;
		// if( scale < -1.0 ){
		// 	return ( -scale - 2.0 );
		// }
		// return scale;
}

float sign( float n )
{
	return ( (n > 0) - (n < 0) );
}

float function_lookup( float id )
{
	return ( sign(id)*2.0 - 1.0 );
}

void function_v( func_gen_t* self, uint16_t b_size, float* r_up, float* r_dn, float* fm_in, float* out )
{
	float* out2 = out;
	float* r_up2 = r_up;
	float* r_down2 = r_dn;
	float* fm_in2 = fm_in;
	float move;

	for(uint16_t i=0; i<b_size; i++){
		if( self->id >= 0 ){
			move = self->rate * (*r_up2) + (*fm_in2++ * self->fm_ix);
		} else {
			move = self->rate * (*r_down2) + (*fm_in2++ * self->fm_ix);
		}
		while( move != 0 ){
			if( self->id >= 0 ){
				self->id += move;
				move = 0;
				if( self->id >= 1.0 ){
					move = (self->id - 1.0) * (*r_down2) / (*r_up2);
					self->id = -1.0;
				} else if( self->id < 0 ){
					if( self->loop != 0 ){
						move = self->id * (*r_down2) / (*r_up2);
						if( self->loop > 0 ) { self->loop -= 1; }
					}
					self->id = 0;
				}
			} else {
				self->id += move;
				move = 0;
				if( self->id >= 0 ){
					if( self->loop != 0 ){
						move = self->id * (*r_up2) / (*r_down2);
						if( self->loop > 0 ) { self->loop -= 1; }
					}
					self->id = 0;
				} else if( self->id < -1.0 ){
					move = (self->id + 1.0) * (*r_up2) / (*r_down2);
					self->id = 1.0;
				}
			}
		}
		*out2++ = self->id;
		*r_up2++; *r_down2++;
	}
}
