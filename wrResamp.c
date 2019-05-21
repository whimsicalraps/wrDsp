#include "wrResamp.h"

void _interp_fast( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
                 )
{
    for( uint16_t i=0; i<b_size; i++ ) {
        // this does *not* result in equal power with varying coeff
        float isp  = 1.0 / _Abs(*speed);
        float iRcB = isp * coeff;
        float c1   = iRcB - isp + 1.0;
        float c2   = 1.0 - iRcB;

        // variable width linear interp impulse response _/\_
        tape[-1] += *codec * lim_f_0_1(c2 - isp);
        tape[ 0] += *codec * c2;
        tape[ 1] += *codec * c1;
        tape[ 2] += *codec * lim_f_0_1(c1 - isp);

        codec++;
        //tape -= 3;
        coeff += *speed++;
        while( coeff < 0.0 ){
            coeff += 1.0;
            tape--;
        }
        while( coeff >= 1.0 ){
            coeff -= 1.0;
            tape++;
        }
    }
}


void _interp_normal( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
                 )
{
    for( uint16_t i=0; i<b_size; i++ ) {
        *tape++ += *codec   * (1.0 - coeff);
        *tape   += *codec++ * coeff;

        tape--;
        coeff += *speed++;
        while( coeff < 0.0 ){
            coeff += 1.0;
            tape--;
        }
        while( coeff >= 1.0 ){
            coeff -= 1.0;
            tape++;
        }
    }
}


void _interp_slow( float*          codec
                 , float*          tape
                 , float           coeff
                 , float*          speed
                 , uint16_t        b_size
                 )
{
    for( uint16_t i=0; i<b_size; i++ ) {
        float voila = *codec++ * _Abs(*speed); // volume scale (== impulse shaping!)
        *tape++ += voila * (1.0 - coeff);
        *tape   += voila * coeff;

        tape--;
        coeff += *speed++;
        while( coeff < 0.0 ){
            coeff += 1.0;
            tape--;
        }
        while( coeff >= 1.0 ){
            coeff -= 1.0;
            tape++;
        }
    }
}

IO_block_t* resamp_codec_to_tape( float*          speed
                                , IO_block_t*     codec
                                , IO_block_t*     tapeio
                                , uint16_t        s_origin
                                , float           s_interp
                                )
{
    float aspd = _Abs(speed[0]);
    if( aspd > 1.0 ){
        _interp_fast( codec->audio
                    , &(tapeio->audio[s_origin])
                    , s_interp
                    , speed
                    , codec->size
                    );
    } else if( aspd == 1.0 ){
        _interp_normal( codec->audio
                      , &(tapeio->audio[s_origin])
                      , s_interp
                      , speed
                      , codec->size
                      );
    } else {
        _interp_slow( codec->audio
                    , &(tapeio->audio[s_origin])
                    , s_interp
                    , speed
                    , codec->size
                    );
    }
    return tapeio;
}

IO_block_t* resamp_tape_to_codec( float*          speed
                                , IO_block_t*     tapeio
                                , uint16_t        s_origin
                                , float           s_interp
                                , IO_block_t*     codec
                              )
{
    float*   buf = codec->audio;
    float*   hb  = &(tapeio->audio[s_origin -1]); // wide for lagrange
    float    co  = s_interp;
    for( uint16_t i=0; i<(codec->size); i++ ){

        float coeff[4];
        // these have been shifted from the textbook by +1 to co (shift range to 0-1)
        coeff[0] = -((co    )*(co-1.0)*(co-2.0))/6.0;
        coeff[1] =  ((co+1.0)*(co-1.0)*(co-2.0))/2.0;
        coeff[2] = -((co+1.0)*(co    )*(co-2.0))/2.0;
        coeff[3] =  ((co+1.0)*(co    )*(co-1.0))/6.0;

        *buf = 0.0;
        for( uint8_t s=0; s<4; s++ ){
            *buf += hb[s] * coeff[s];
        }
        buf++;
        co += *speed++;
        while( co < 0.0 ){
            co += 1.0;
            hb--;
        }
        while( co >= 1.0 ){
            co -= 1.0;
            hb++;
        }
    }
    return codec;
}
