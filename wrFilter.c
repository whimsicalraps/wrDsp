#include "wrFilter.h"
#include "../wrLib/wrMath.h"
#include <math.h>

///////////////
// 1Pole LPF //
///////////////

void lp1_init(filter_lp1_t* f) {
	f->x = 0;
	f->y = 0;
	f->c = 0.97;
}
void lp1_set_dest(filter_lp1_t* f, float in)
{
	f->x = in;
}
float lp1_step(filter_lp1_t* f, float in) {
	f->y = f->y + f->c * (in - f->y);
	return f->y;
}
void lp1_set_coeff(filter_lp1_t* f, float c) {
	f->c = c;
}
void lp1_set_freq(filter_lp1_t* f, float freq) {
	f->c = freq/48000; // expo!
}
float lp1_step_v(filter_lp1_t* f, float* in, float* out, uint16_t size) {
	float* in2=in;
	float* out2=out;
	float* out3=out; // point to start of arrays
	// out3 = y = previous OUT

	// first samp
	*out2++ = f->y + f->c * ((*in2++) - f->y);

	// remainder of samps -> add nFloor early exit to avoid denormals
	for(uint16_t i=0; i<(size-1); i++) {
		*out2++ = (*out3) + f->c * ((*in2++) - (*out3));
		*out3++;
	}

	f->y = *out3; // last output
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
	float* in3=in; // in3 = x = previous IN
	float* out2=out;
	float* out3=out; // out3 = y = previous OUT	

	// for first sample
	*out2++ = (*in2++) - f->x + (f->c * f->y);

	// remainder of samps
	for(uint16_t i=0; i<(size-1); i++) {
		*out2++ = (*in2++) - (*in3++) + (f->c * (*out3++));
	}

	// save vals
	f->x = *in3;
	f->y = *out3;
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
