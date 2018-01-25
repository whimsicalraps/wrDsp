#include "wrFilter.h"

#include <math.h>
#include <stdlib.h>

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
uint8_t lp1_converged( filter_lp1_t* f )
{
	float diff = f->x - f->y;
	if((diff > -nFloor) && (diff < nFloor)) { return 1; }
	return 0;
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


////////////////
// DC-Blocker //
////////////////

// Differentiator followed by leaky integrator
void dc_init(filter_dc_t* f)
{
	f->c = 0.997; // default time constant
		// NB: lowering this val increases latency!!
	f->x = 0;
	f->y = 0;
}

void dc_time(filter_dc_t* f, float hpc)
{
	// f->c = 1.0 means DC-coupling
	// f->c = 0.0 means differentiator (only changes are passed)
	f->c = lim_f_0_1(hpc);
}

float dc_step(filter_dc_t* f, float in)
{
	f->y = in - f->x + (f->c * f->y);
	f->x = in; // save previous input
	return f->y;
}

void dc_step_v(filter_dc_t* f, float* in, float* out, uint16_t size)
{
	float* in2=in;
	float* out2=out;

	// remainder of samps
	for(uint16_t i=0; i<size; i++) {
		f->y = *in2 - f->x + (f->c * f->y);
		f->x = *in2++;
		*out2++ = f->y;
	}
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
