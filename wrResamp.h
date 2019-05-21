#include <math.h> // ceilf()
#include "wrMath.h"
#include "wrFilter.h" // filter_sr_t

typedef struct IO_block {
    int         size;
    float*      audio;
} IO_block_t; // dsp_block.h, tape.h, rhead.h

void _interp_fast( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
               );

void _interp_normal( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
               );

void _interp_slow( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
               );

IO_block_t* resamp_codec_to_tape( float*          speed
                                , IO_block_t*     codec
                                , IO_block_t*     tapeio
                                , uint16_t        s_origin
                                , float           s_interp
                                );

IO_block_t* resamp_tape_to_codec( float*          speed
                                , IO_block_t*     tapeio
                                , uint16_t        s_origin
                                , float           s_interp
                                , IO_block_t*     codec
                              );
