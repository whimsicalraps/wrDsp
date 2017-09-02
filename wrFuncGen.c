#include <math.h>
#include <wrFuncGen.h>
#include <wrMath.h>

#include "../STM32F4-workarea/Project/JF3-test/lib/debug.h"

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
	self->s_mode        = 0;
	self->sustain_state = 0;
	self->sustaining    = 0;
	self->zc            = 0;
	self->r_up          = 1;
	self->r_down        = 1;
}

// Param Functions

// Called on trigger edges only

/* function_reset is same as cutoff = -1;
   function_trig  is same as cutoff = 0; */

void function_trig_reset( func_gen_t* self
	                    , uint8_t     state )
{
	if(state){ // release stage/stopped
		self->id = MIN_POS_FLOAT; // reset
		self->go = 1;
	}
	self->sustain_state = state;
}
void function_trig_wait( func_gen_t* self
	                   , uint8_t     state )
{
	if(state && (!self->go)){ // release stage/stopped
		self->id = MIN_POS_FLOAT; // reset
		self->go = 1;
	}
	self->sustain_state = state;
}
void function_trig_sustain( func_gen_t* self
	                      , uint8_t     state )
{
	/*if(state && !self->go){ // stopped & high trigger
		self->id = MIN_POS_FLOAT;
		self->go = 1;
	} else if( state
		    && self->id <=0.0 ){ // release phase & high trigger
		float eye = shaper_rev_lookup( &friends.shaper
	                                 , self->id
	                                 , BLOCK_SIZE-1
	                                 );
		self->id = eye 
	} else if( !state
		    && self->id > 0.0 ){ // attack phase & release
		self->id = -(self->id); // naive flip to release
	}
	self->sustain_state = state;*/
}
void function_trig_vari( func_gen_t* self
	                   , uint8_t     state
	                   , float       cutoff )
{
	// -1 is always, 0 is only in release, +1 is never
	uint8_t tr;
	(cutoff >= 0.0f)
		? ( tr = (self->id <= 0.0f)
			  && (self->id > -(cutoff)) )
		: ( tr = (self->id <= 0.0f)
			  || (self->id > (1.0f + cutoff) ) );
	if(state && tr){ // release stage/stopped
		self->id = MIN_POS_FLOAT; // reset
		self->go = 1;
	}
	self->sustain_state = state;
}
void function_trig_burst( func_gen_t* self
                        , uint8_t     state
                        , float       count )
{
	// -1 is zero, 0 is 6, +1 is 36
	if(state){ // release stage/stopped
		self->id = MIN_POS_FLOAT; // reset
		if(count <= -0.7f){ // choke channel if below -3v5
			self->go = 0;
			self->loop = 0;
		} else {
			self->go = 1;
			self->loop = (int8_t)powf( 6.0f
				                     , count + 1.0f)
			                     - 1.0f;
		}
	}
	self->sustain_state = state;
}
void function_trig_spill( func_gen_t* self
	                    , uint8_t     state
	                    , float       cutoff )
{
	// -1 is always, 0 is only in release, +1 is never
	uint8_t tr;
	(cutoff >= 0.0f)
		? ( tr = (self->id <= 0.0f)
			  && (self->id > -(cutoff)) )
		: ( tr = (self->id <= 0.0f)
			  || (self->id > (1.0f + cutoff) ) );
	if(state && tr){ // release stage/stopped
		self->id = -(self->id); // flip (soft-sync)
		if(!self->go){ // explicit start required
			self->id = MIN_POS_FLOAT;
		}
		self->go = 1;
	}
	self->sustain_state = state;
}

void function_mode( func_gen_t* self, uint8_t mode )
{
	function_loop(self, 0-(FNGEN_CYCLE == mode)); // 0 or -1
	function_sustain(self, (FNGEN_SUSTAIN == mode));
}

void function_loop( func_gen_t* self, int8_t loop )
{
	self->loop = loop;
	if(loop != 0){
		self->go = 1;
	}
}

void function_sustain( func_gen_t* self, uint8_t sust )
{
	self->s_mode = sust;
}

void function_rate( func_gen_t* self, float rate )
{
	self->rate = lim_f_n1_1( rate );
}

void function_fm_ix( func_gen_t* self, float ix )
{
	self->fm_ix = lim_f_0_1(ix) * 0.1f;
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

void function_ramp_v( uint16_t b_size
	                , float ctrl_rate
	                , float* audio_rate
	                , float* ramp_up
	                , float* ramp_down )
{
	float* audio_rate2 = audio_rate;
	float* ramp_up2 = ramp_up;
	float* ramp_down2 = ramp_down;

	for(uint16_t i=0;i<b_size;i++){
		*ramp_up2 = 0.5f / (0.998f * lim_f(ctrl_rate + *audio_rate2++,0.0f,1.0f) + 0.001f);
		*ramp_down2++ = 1.0f/ (2.0f- (1.0f/ *ramp_up2++));
	}
}

float function_step( func_gen_t* self, float fm_in )
{
	if( self->go ){
		float move;
		
		// determine rate based on direction
		if( self->id >= 0 ){
			move = self->rate * self->r_up + (fm_in * self->fm_ix); // (+ phase mod)
		} else {
			move = self->rate * self->r_down + (fm_in * self->fm_ix);
		}
		// increment w/ overflow protection
		while( move != 0.0f ){
			if( self->id >= 0.0f ){ // are we above zero BEFORE moving
				self->id += move;
				move = 0.0f;
				if( self->id >= 1.0f ){
					move = (self->id - 1.0f) * self->r_down / self->r_up;
					self->id = -1.0f;
				} else if( self->id < 0.0f ){
					if( self->loop != 0 ){
						move = self->id * self->r_down / self->r_up;
						if( self->loop > 0 ) { self->loop += 1; }
					}
					self->id = 0.0f;
				}
			} else {
				self->id += move;
				move = 0.0f;
				if( self->id >= 0.0f ){
					if( self->loop != 0 ){
						move = self->id * self->r_up / self->r_down;
						if( self->loop > 0 ) { self->loop -= 1; }
						self->id = MIN_POS_FLOAT;
					} else {
						self->go = 0;
						self->id = 0.0f;
					}
				} else if( self->id < -1.0f ){
					move = (self->id + 1.0f) * self->r_up / self->r_down;
					self->id = 1.0f;
				}
			}
		}
	}
	return self->id;
	// NB: this is +/-1 sawtooth, with 0 as 'stopped'
	// use function_lookup(self->id) to convert
}

// compiler inlines this anyway
float sign( float n )
{
	return ( (n > 0.0f) - (n < 0.0f) );
}

float function_lookup( float id )
{
	return ( sign(id)*2.0f - 1.0f );
}

void function_v( func_gen_t* self
	           , uint16_t    b_size
	           , float*      r_up
	           , float*      r_dn
	           , float*      fm_in
	           , float*      out )
{
	float* out2 = out;
	float* r_up2 = r_up;
	float* r_down2 = r_dn;
	float* fm_in2 = fm_in;

	self->sustaining = 0;
	self->zc = 0;

	if( self->go ){
		for(uint16_t i=0; i<b_size; i++){
			float move = (*fm_in2++ * self->fm_ix) // linear FM
			             + self->rate              // base freq
			               * ( (self->id >= 0.0f)  // is rising?
			                   ? (*r_up2)
			                   : (*r_down2)
			                 );
			while( move != 0.0f ){
				if( self->id > 0.0f ){ // attack
					self->id += move;
					move = 0.0f;
					if( self->id >= 1.0f ){
						if( self->s_mode && self->sustain_state ){
							// fill rest of block with 1s
							self->sustaining = 1;
							self->id = 1.0f;
							for( i; i<b_size; i++ ){
								*out2++ = self->id;
							}
							return;
						}
						move = (self->id - 1.0f) * (*r_down2) / (*r_up2);
						self->id = -1.0f;
					} else if( self->id < 0.0f ){ // rev TZ
						self->zc = 1;
						if( self->loop ){
							move = self->id * (*r_down2) / (*r_up2);
							if( self->loop > 0.0f ) { self->loop++; } // TZ adds to burst!
						}
						self->id = 0.0f;
					}
				} else { // release
					self->id += move;
					move = 0.0f;
					if( self->id >= 0.0f ){ // rel -> ?atk
						self->zc = 1;
						if( self->loop ){
							move = self->id * (*r_up2) / (*r_down2);
							if( self->loop > 0 ) { self->loop--; }
							self->id = MIN_POS_FLOAT; // get into attack case
						} else {
							// fill rest of block with 0s
							self->id = 0.0f; // only for STOP
							self->go = 0;
							for( i; i<b_size; i++ ){
								*out2++ = self->id;
							}
							return;
						}
					} else if( self->id < -1.0f ){ // TZ back to attack
						move = (self->id + 1.0f) * (*r_up2) / (*r_down2);
						self->id = 1.0f;
					}
				}
			}
			*out2++ = self->id;
			r_up2++; r_down2++;
		}
	} else {
		self->id = 0.0f;
		for( uint16_t i=0; i<b_size; i++ ){
			*out2++ = self->id;
		}
	}
	return;
}
void function_fmix_v( func_gen_t* self
	                , uint16_t    b_size
	                , float*      r_up
	                , float*      r_dn
	                , float*      fm_in
	                , float*      fm_ix
	                , float*      out
	                )
{
	float* out2 = out;
	float* r_up2 = r_up;
	float* r_down2 = r_dn;
	float* fm_in2 = fm_in;

	self->sustaining = 0;
	self->zc = 0;

	if( self->go ){
		for(uint16_t i=0; i<b_size; i++){
			float move = ( *fm_in2++               // FM audio source
			               * ( self->fm_ix         // FM pot
			                   + 0.1 * *fm_ix++ )  // FM index adds to pot
			               ) // linear FM
			             + self->rate              // base freq
			               * ( (self->id >= 0.0f)  // is rising?
			                   ? (*r_up2)
			                   : (*r_down2)
			                 );
			while( move != 0.0f ){
				if( self->id > 0.0f ){ // attack
					self->id += move;
					move = 0.0f;
					if( self->id >= 1.0f ){
						if( self->s_mode && self->sustain_state ){
							// fill rest of block with 1s
							self->sustaining = 1;
							self->id = 1.0f;
							for( i; i<b_size; i++ ){
								*out2++ = self->id;
							}
							return;
						}
						move = (self->id - 1.0f) * (*r_down2) / (*r_up2);
						self->id = -1.0f;
					} else if( self->id < 0.0f ){ // rev TZ
						self->zc = 1;
						if( self->loop ){
							move = self->id * (*r_down2) / (*r_up2);
							if( self->loop > 0.0f ) { self->loop++; } // TZ adds to burst!
						}
						self->id = 0.0f;
					}
				} else { // release
					self->id += move;
					move = 0.0f;
					if( self->id >= 0.0f ){ // rel -> ?atk
						self->zc = 1;
						if( self->loop ){
							move = self->id * (*r_up2) / (*r_down2);
							if( self->loop > 0 ) { self->loop--; }
							self->id = MIN_POS_FLOAT; // get into attack case
						} else {
							// fill rest of block with 0s
							self->id = 0.0f; // only for STOP
							self->go = 0;
							for( i; i<b_size; i++ ){
								*out2++ = self->id;
							}
							return;
						}
					} else if( self->id < -1.0f ){ // TZ back to attack
						move = (self->id + 1.0f) * (*r_up2) / (*r_down2);
						self->id = 1.0f;
					}
				}
			}
			*out2++ = self->id;
			r_up2++; r_down2++;
		}
	} else {
		self->id = 0.0f;
		for( uint16_t i=0; i<b_size; i++ ){
			*out2++ = self->id;
		}
	}
	return;
}