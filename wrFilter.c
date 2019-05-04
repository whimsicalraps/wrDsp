#include "wrFilter.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "wrMath.h"

///////////////
// 1Pole LPF //
///////////////

void lp1_init(filter_lp1_t* f)
{
	f->x = 0;
	f->y = 0;
	f->c = 0.97;
}
void lp1_set_dest(filter_lp1_t* f, float in)
{
	f->x = in;
}
float lp1_get_dest( filter_lp1_t* f )
{
	return f->x;
}
void lp1_set_out(filter_lp1_t* f, float level)
{
	f->y = level;
}
float lp1_get_out( filter_lp1_t* f )
{
	return f->y;
}
float lp1_step(filter_lp1_t* f, float in)
{
	f->y = f->y + f->c * (in - f->y);
	return f->y;
}
uint8_t lp1_converged( filter_lp1_t* f )
{
	float diff = f->x - f->y;
	if((diff > -nFloor) && (diff < nFloor)) { return 1; }
	return 0;
}
float lp1_step_internal(filter_lp1_t* f)
{
    if(lp1_converged( f )){
        f->y = f->x;
    } else {
	    f->y = f->y + f->c * (f->x - f->y);
    }
	return f->y;
}
void lp1_set_coeff(filter_lp1_t* f, float c)
{
	f->c = c;
}
float lp1_get_coeff(filter_lp1_t* f)
{
    return f->c;
}
void lp1_set_freq(filter_lp1_t* f, float freq)
{
	f->c = freq/48000; // expo!
}
void lp1_step_v(filter_lp1_t* f, float* in, float* out, uint16_t size)
{
	float* in2=in;
	float* out2=out;
	float* out3=out; // point to start of arrays
	// out3 = y = previous OUT

	// first samp
	*out2++ = f->y + f->c * ((*in2++) - f->y);

	// remainder of samps -> add nFloor early exit to avoid denormals
	for(uint16_t i=0; i<(size-1); i++) {
		*out2++ = (*out3) + f->c * ((*in2++) - (*out3));
		out3++;
	}

	f->y = *out3; // last output
}
void lp1_step_c_v(filter_lp1_t* f, float* out, uint16_t size)
{
	float* out2=out;
	float* out3=out; // point to start of arrays

	// first samp
		// check if we've already converged
	if( lp1_converged(f) ){
		for(uint16_t i=0; i<size; i++){
			*out2++ = f->x;
		}
		f->y = f->x;
		return;
	}
	*out2++ = f->y + f->c * (f->x - f->y);

	// remainder of samps -> add nFloor early exit to avoid denormals
	for(uint16_t i=0; i<(size-1); i++) {
		*out2++ = (*out3) + f->c * (f->x - (*out3));
		out3++;
	}

	f->y = *out3; // last output
}



//////////////////////////////
// 1Pole LPF (Assymetrical) //
//////////////////////////////

void lp1_a_init( filter_lp1_a_t* f )
{
	f->y      = 0;
	f->c_rise = 0.98;
	f->c_fall = 0.8;
}
float lp1_a_step( filter_lp1_a_t* f, float in )
{
    float c = (in > f->y) ? f->c_rise : f->c_fall;
	f->y = f->y + c * (in - f->y);
	return f->y;
}
void lp1_a_set_coeff( filter_lp1_a_t* f, float c_rise
                                       , float c_fall
                                       )
{
	f->c_rise = c_rise;
	f->c_fall = c_fall;
}
void lp1_a_step_v( filter_lp1_a_t* f, float*   in
                                    , float*   out
                                    , uint16_t size
                                    )
{
	float* in2=in;
	float* out2=out;
	float* out3=out;
	// out3 = y = previous OUT

    // if we match this case, we can't assume same direction this block
    if( (in[0]      >= 0.0)
      ^ (in[size-1] >= 0.0) ){ // frame changes direction: case on each sample
        float c = (*in2 > f->y) ? f->c_rise : f->c_fall;
    	*out2++ = f->y + c * ((*in2++) - f->y);
    	for(uint16_t i=0; i<(size-1); i++) {
            c = (*in2 > *out3) ? f->c_rise : f->c_fall;
    		*out2++ = (*out3) + c * ((*in2++) - (*out3));
    		out3++;
    	}
    } else { // assume the whole in frame is same direction relative to output
        float c = (in[0] > f->y) ? f->c_rise : f->c_fall;

    	*out2++ = f->y + c * ((*in2++) - f->y);
    	for(uint16_t i=0; i<(size-1); i++) {
    		*out2++ = (*out3) + c * ((*in2++) - (*out3));
    		out3++;
    	}
    }
	f->y = *out3; // last output
}

///////////////////
// SWITCH & RAMP //
///////////////////

void switch_ramp_init( filter_sr_t* f )
{
    f->ramp = 0.0;
    f->rate = 0.001; // is this per-sample step-size, or 1pole coefficient?
}
void switch_ramp_set_rate( filter_sr_t* f, float rate )
{
    f->rate = rate;
}
void switch_ramp_jump( filter_sr_t* f, float step_size )
{
    f->ramp += step_size; // accumulate in case of overlapping ramps
}
float* switch_ramp_step_v( filter_sr_t* f, float*   io
                                         , uint16_t size )
{
    float* samp = io;
    if( f->ramp != 0.0 ){ // passthrough if no ramp
        if( f->ramp >= (size * f->rate) ){ // positive ramp
            for( uint16_t i=0; i<size; i++ ){
                *samp++ += f->ramp;
                f->ramp -= f->rate;
            }
        } else if( f->ramp <= -(size * f->rate) ){ // negative ramp
            for( uint16_t i=0; i<size; i++ ){
                *samp++ += f->ramp;
                f->ramp += f->rate;
            }
        } else { // almost zero. check each step
            if( f->ramp > 0 ){ // pos
                for( uint16_t i=0; i<size; i++ ){
                    *samp++ += f->ramp;
                    f->ramp -= f->rate;
                    if( f->ramp <= 0.0 ){
                        f->ramp = 0.0;
                        break;
                    }
                }
            } else { // neg
                for( uint16_t i=0; i<size; i++ ){
                    *samp++ += f->ramp;
                    f->ramp += f->rate;
                    if( f->ramp >= 0.0 ){
                        f->ramp = 0.0;
                        break;
                    }
                }
            }
        }
    }
    return io;
}


////////////////////////////
// AVERAGED WINDOW SMOOTH //
////////////////////////////

void awin_init( filter_awin_t* f, uint16_t win_size )
{
    f->out         = 0.5;
    f->win_size    = win_size;
	f->win_ix      = 0;
	f->win_scale   = 1.0 / (float)(win_size + 1);
	f->history     = NULL;
    f->history     = malloc(sizeof(float)*win_size);
	for( uint16_t i=0; i<win_size; i++ ){ f->history[i] = 0.5; }
	f->slope_sense = 6000.0 * f->win_scale;
}
void awin_slope( filter_awin_t* f, float slope_sensitivity)
{
    f->slope_sense = slope_sensitivity;
}
float awin_step( filter_awin_t* f, float input )
{
    float windowed_avg = input;
	for( uint8_t i=0; i < f->win_size; i++ ){
		windowed_avg += f->history[i];
	}
	windowed_avg *= f->win_scale;
	f->history[f->win_ix++] = input;
	if( f->win_ix >= f->win_size ){ f->win_ix=0; }
    
	// rate of change
	float roc = f->slope_sense
	            * ( windowed_avg - f->out );

    roc = lim_f_0_1( max_f( roc * roc
	                      , 0.008
						  )
				   );

	// slope-sensitive-smoother
    f->out = lim_f_0_1( input
		              + ( 1.0 - roc )
		                * ( f->out - input )
                      );
	return f->out;
}
float awin_get_out( filter_awin_t* f )
{
    return f->out;
}
float awin_get_in( filter_awin_t* f )
{
	int16_t ix = f->win_ix-1;
	if( ix<0 ){ ix += f->win_size; }
    return f->history[ix];
}


////////////////
// DC-Blocker //
////////////////

// Differentiator followed by leaky integrator
filter_dc_t* dc_init( void ){
    filter_dc_t* self = malloc( sizeof( filter_dc_t ) );
    if( self == NULL ){ printf("DC: malloc failed\n"); }

    self->coeff    = 0.997;
    self->prev_in  = 0.0;
    self->prev_out = 0.0;

    return self;
}

void dc_time( filter_dc_t* self, float hpc ){
	// f->c = 1.0 means DC-coupling
	// f->c = 0.0 means differentiator (only changes are passed)
	self->coeff = lim_f_0_1( hpc );
}

float dc_step(filter_dc_t* self, float in){
	self->prev_out = in - self->prev_in + (self->coeff * self->prev_out);
	self->prev_in = in; // save previous input
	return self->prev_out;
}

float* dc_step_v( filter_dc_t* self, float* buffer
                                   , int    b_size
                                   ){
    float* in  = buffer;
    float* out = buffer;
    for( int i=0; i<b_size; i++ ){
        self->prev_out = *in - self->prev_in + (self->coeff * self->prev_out);
        self->prev_in = *in++;
        *out++ = self->prev_out;
    }
    return buffer;
}


////////////////////////////
// State-variable: 2-pole //
////////////////////////////

void svf_init(filter_svf_t* f, uint8_t mode, uint32_t sample_rate) {
	f->x[0] = 0;
	f->x[1] = 0;
	f->x[2] = 0;
	f->q = 0.5;
	f->c = 0.02;
	f->mode = mode;
    f->sample_rate = sample_rate;
}
void svf_set_mode(filter_svf_t* f, uint8_t mode) {
	f->mode = mode;
}
float svf_process_frame(filter_svf_t* f, float input) {
	f->x[0] = f->x[0] + (f->c * f->x[1]);
	f->x[2] = (input - (f->q * f->x[1])) - f->x[0];
	f->x[1] = f->x[1] + (f->c * f->x[2]);
	return f->x[f->mode];
}
float svf_step(filter_svf_t* f, float input) {
	// 2x oversample & average
	float out = svf_process_frame(f, input);
	out += svf_process_frame(f, input);
	out *= 0.5;
	return out;
}
void svf_set_coeff(filter_svf_t* f, float coeff) {
	f->c = lim_f(coeff * 0.5, 0.001, 0.499);
}
void svf_set_freq(filter_svf_t* f, float freq) {
//	f->c = lim_f(freq/(f->sample_rate), 0.001, 0.499); // need expo lookup here
    // when freq = sample_rate/4, f->c = 0.5

    f->c = exp(-2.0 * WR_PI * freq * 1/f->sample_rate );

	// Set mixfreq to whatever rate your system is using (eg 48Khz)
	// Calculate filter cutoff frequencies
	// es->lf = 2 * sin(M_PI * ((double)lowfreq / (double)mixfreq)); 
}
void svf_set_q(filter_svf_t* f, float quality) {
	f->q = lim_f(0.5 - (quality * 0.5), 0.001, 0.499); // q needs stronger limits to avoid overload
}
