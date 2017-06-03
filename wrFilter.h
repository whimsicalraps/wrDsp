#pragma once

#include <stdint.h>

typedef struct filter_lp1 {
	float x;   // destination
	float y;
	float c;
} filter_lp1_t;

typedef struct _filter_dc {
  float c; 	   // time constant
  float x;	   // previous input
  float y;	   // previous output
} filter_dc_t;

typedef struct filter_svf {
	float x[3];
	float q;
	float c;
	uint8_t mode;
} filter_svf_t;

// Lowpass: 1-pole
void l1p_init(filter_lp1_t* f);
float l1p_step(filter_lp1_t* f, float in);
void l1p_set_coeff(filter_lp1_t* f, float c);
void l1p_set_freq(filter_lp1_t* f, float freq);
float l1p_step_v(filter_lp1_t* f, float* out, uint16_t size);

// DC-Blocker: Leaky Integrator -> Differentiator
void dc_init(filter_dc_t* f);
void dc_time(filter_dc_t* f, float hpc);
float dc_step(filter_dc_t* f, float in);
void dc_step_v(filter_dc_t* f, float* in, float* out, uint16_t size);

// State-variable: 2-pole
void svf_init(filter_svf_t* f, uint8_t mode);
float svf_process_frame(filter_svf_t* f, float input);
float svf_step(filter_svf_t* f, float input);
void svf_set_mode(filter_svf_t* f, uint8_t mode);
void svf_set_coeff(filter_svf_t* f, float coeff);
void svf_set_freq(filter_svf_t* f, float freq);
void svf_set_q(filter_svf_t* f, float quality);
