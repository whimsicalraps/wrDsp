#include "wrShaper.h"

#include <stdlib.h>
#include <stdio.h>

#include "../wrLib/wrMath.h"
#include "../wrLib/wrGlobals.h"
#include "wrOscSine.h"

// verbose names for switch statement
#define SQ_LOG   0
#define LOG_TRI  1
#define TRI_EXP  2
#define EXP_SINE 3


const float log_lut[LUT_SIN_HALF + 1]={
	-0.9999902200489,-0.978281604225695,-0.957221104523062,-0.936771140858207,-0.91689730980816,-0.897568036367262,-0.878754272154726,-0.860429232830196,-0.842568168759236,-0.825148164000937,-0.808147959521389,-0.791547797211688,-0.775329281839953,-0.759475258518497,-0.743969703639602,-0.728797627541498,-0.713944987422418,-0.699398609234617,-0.685146117469572,-0.671175871896579,-0.657476910444454,-0.644038897524108,-0.630852077181678,-0.617907230550289,-0.605195637135646,-0.592709039528244,-0.580439611184561,-0.568379926962425,-0.556522936132754,-0.54486193762206,-0.533390557268032,-0.522102726894906,-0.510992665036675,-0.50005485915479,-0.48928404921347,-0.478675212490087,-0.468223549510837,-0.45792447101312,-0.447773585845982,-0.437766689728762,-0.427899754795935,-0.418168919863072,-0.408570481355037,-0.39910088484309,-0.389756717142501,-0.380534698926699,-0.371431677817971,-0.362444621918287,-0.353570613747011,-0.344806844555196,-0.336150608988712,-0.327599300074853,-0.319150404509164,-0.310801498221173,-0.302550242199463,-0.294394378558079,-0.286331726827727,-0.278360180456529,-0.270477703506271,-0.2626823275312,-0.254972148627377,-0.247345324641549,-0.239800072529273,-0.232334665852837,-0.224947432410193,-0.217636751986734,-0.210401054222392,-0.203238816586988,-0.196148562457347,-0.189128859290076,-0.182178316884354,-0.175295585729478,-0.168479355432229,-0.1617283532195,-0.155041342511883,-0.148417121564231,-0.141854522169453,-0.135352408422041,-0.128909675538064,-0.122525248728554,-0.116198082123421,-0.109927157743185,-0.103711484516019,-0.0975500973377138,-0.0914420561723365,-0.0853864451914893,-0.0793823719501999,-0.0734289665975826,-0.0675253811205289,-0.0616707886187803,-0.0558643826098356,-0.0501053763622298,-0.0443930022558067,-0.0387265111676836,-0.0331051718826815,-0.0275282705270582,-0.0219951100244499,-0.016505009572981,-0.011057304142565,-0.00565134399146683,-0.000286494201245269,0.00503786577075294,0.0103223425221382,0.0155675291202108,0.0207740055013884,0.0259423388560034,0.0310730839991048,0.0361667837278765,0.0412239691662428,0.0462451600972096,0.0512308652834631,0.0561815827767185,0.0610978002162896,0.0659799951173273,0.0708286351491527,0.0756441784040902,0.0804270736571882,0.0851777606171939,0.0898966701691349,0.0945842246088406,0.0992408378697239,0.103866915742128,0.108462856085526,0.113029049033857,0.117565877194253,0.122073715839426,0.126552933093938,0.131003890114599,0.135426941265214,0.139822434285876,0.144190710457029,0.148532104758473,0.152846946023513,0.157135557088419,0.161398254937372,0.165635350843057,0.169847150503061,0.174033954172217,0.178196056791052,0.182333748110464,0.186447312812762,0.190537030629205,0.194603176454146,0.198646020455907,0.202665828184497,0.206662860676282,0.210637374555696,0.214589622134111,0.218519851505953,0.222428306642154,0.226315227481025,0.230180850016647,0.234025406384847,0.237849124946852,0.241652230370675,0.245434943710336,0.249197482482952,0.252940060743801,0.256662889159388,0.260366175078606,0.264050122602032,0.267714932649428,0.27136080302549,0.274987928483917,0.278596500789833,0.282186708780624,0.285758738425237,0.28931277288198,0.292848992554878,0.29636757514862,0.299868695722149,0.303352526740917,0.306819238127871,0.310268997313177,0.313701969282747,0.317118316625587,0.320518199579996,0.323901776078671,0.327269201792725,0.330620630174663,0.333956212500342,0.337276097909945,0.340580433447994,0.343869364102426,0.347143032842772,0.350401580657447,0.353645146590184,0.356873867775635,0.360087879474161,0.363287315105835,0.36647230628367,0.369642982846115,0.372799472888804,0.375941902795617,0.37907039726904,0.382185079359861,0.385286070496206,0.388373490511953,0.391447457674511,0.394508088712009,0.397555498839889,0.400589801786934,0.403611109820736,0.406619533772616,0.409615183062025,0.41259816572042,0.415568588414642,0.418526556469804,0.421472173891696,0.424405543388734,0.427326766393445,0.430235943083511,0.433133172402389,0.436018552079497,0.438892178649995,0.44175414747417,0.444604552756418,0.447443487563854,0.450271043844541,0.453087312445365,0.455892383129544,0.458686344593795,0.461469284485167,0.464241289417534,0.467002444987775,0.469752835791627,0.472492545439244,0.475221656570442,0.47794025086966,0.480648409080628,0.483346211020758,0.486033735595258,0.48871106081098,0.491378263790006,0.494035420782978,0.496682607182178,0.499319897534363,0.501947365553361,0.504565084132436,0.507173125356416,0.509771560513614,0.512360460107507,0.514939893868228,0.517509930763825,0.52007063901133,0.522622086087623,0.525164338740102,0.527697462997158,0.530221524178468,0.532736586905098,0.535242715109435,0.53773997204493,0.540228420295688,0.54270812178587,0.545179137788943,0.547641528936767,0.550095355228515,0.552540676039452,0.554977550129552,0.557406035651973,0.559826190161378,0.562238070622126,0.564641733416315,0.567037234351689,0.569424628669413,0.571803971051719,0.574175315629419,0.576538715989293,0.57889422518136,0.581241895726016,0.583581779621066,0.585913928348631,0.588238392881949,0.590555223692053,0.592864470754353,0.5951661835551,0.597460411097751,0.59974720190923,0.602026604046082,0.604298665100539,0.606563432206478,0.60882095204529,0.611071270851651,0.613314434419207,0.615550488106163,0.617779476840786,0.620001445126824,0.622216437048836,0.624424496277439,0.626625666074476,0.628819989298101,0.631007508407787,0.633188265469254,0.635362302159324,0.637529659770698,0.639690379216666,0.641844501035738,0.643992065396206,0.646133112100643,0.648267680590327,0.650395809949597,0.652517538910153,0.654632905855282,0.656741948824027,0.658844705515286,0.660941213291862,0.663031509184442,0.665115629895523,0.667193611803277,0.669265490965366,0.671331303122688,0.673391083703087,0.675444867824987,0.677492690300993,0.67953458564143,0.681570588057829,0.683600731466371,0.685625049491273,0.687643575468131,0.689656342447218,0.691663383196723,0.693664730205956,0.695660415688507,0.697650471585352,0.699634929567921,0.701613821041125,0.703587177146336,0.705555028764328,0.707517406518178,0.709474340776123,0.711425861654379,0.713371999019927,0.71531278249325,0.717248241451041,0.719178405028872,0.72110330212382,0.723022961397072,0.724937411276477,0.726846679959077,0.728750795413596,0.7306497853829,0.732543677386424,0.734432498722561,0.736316276471024,0.738195037495177,0.740068808444335,0.741937615756026,0.743801485658236,0.745660444171613,0.747514517111649,0.749363730090831,0.751208108520761,0.753047677614257,0.754882462387415,0.756712487661653,0.758537778065726,0.760358358037715,0.762174251826989,0.763985483496142,0.765792076922911,0.767594055802058,0.76939144364724,0.771184263792849,0.772972539395827,0.774756293437462,0.776535548725163,0.778310327894205,0.78008065340946,0.781846547567103,0.783608032496294,0.785365130160845,0.787117862360864,0.788866250734374,0.790610316758918,0.792350081753142,0.79408556687836,0.795816793140096,0.797543781389608,0.799266552325402,0.800985126494711,0.802699524294972,0.804409765975279,0.806115871637812,0.80781786123926,0.809515754592221,0.811209571366584,0.812899331090896,0.814585053153717,0.81626675680495,0.817944461157162,0.819618185186888,0.821287947735917,0.822953767512567,0.824615663092941,0.82627365292217,0.827927755315643,0.829577988460219,0.831224370415426,0.832866919114651,0.834505652366307,0.836140587854997,0.837771743142653,0.839399135669672,0.841022782756034,0.842642701602409,0.844258909291246,0.84587142278786,0.847480258941495,0.849085434486386,0.850686966042797,0.852284870118059,0.853879163107589,0.855469861295895,0.857056980857585,0.85864053785834,0.860220548255901,0.861797027901029,0.863369992538458,0.864939457807842,0.866505439244685,0.868067952281265,0.869627012247548,0.871182634372086,0.872734833782914,0.874283625508431,0.875829024478274,0.877371045524178,0.878909703380835,0.880445012686736,0.881976987985006,0.883505643724233,0.885030994259284,0.886553053852114,0.888071836672564,0.889587356799159,0.891099628219884,0.892608664832961,0.894114480447617,0.895617088784841,0.897116503478134,0.89861273807425,0.900105806033934,0.901595720732645,0.903082495461277,0.904566143426867,0.906046677753306,0.907524111482029,0.908998457572704,0.910469728903921,0.911937938273859,0.913403098400959,0.914865221924583,0.91632432140567,0.917780409327379,0.919233498095736,0.920683600040265,0.922130727414614,0.92357489239718,0.925016107091722,0.926454383527967,0.92788973366222,0.929322169377952,0.930751702486395,0.932178344727126,0.933602107768643,0.935023003208942,0.936441042576079,0.937856237328734,0.939268598856766,0.940678138481765,0.94208486745759,0.943488796970918,0.944889938141768,0.946288302024038,0.94768389960602,0.949076741810925,0.950466839497392,0.951854203459998,0.953238844429759,0.954620773074632,0.956,0.957376535749171,0.958750390803852,0.960121575584636,0.961490100451469,0.962855975704125,0.964219211582667,0.965579818267912,0.966937805881885,0.968293184488268,0.969645964092853,0.970996154643981,0.972343766032983,0.973688808094614,0.975031290607483,0.976371223294482,0.977708615823205,0.979043477806369,0.980375818802231,0.98170564831499,0.983032975795203,0.984357810640184,0.985680162194403,0.987000039749882,0.988317452546588,0.989632409772818,0.990944920565586,0.992254994011005,0.993562639144661,0.994867864951988,0.996170680368641,0.997471094280861,0.998769115525838
	// ,1.0
};

static void shaper_prep_v( uint8_t*  zone
                         , float*    coeff
                         , float     shape
                         , float*    shape_audio
                         , int       b_size
                         )
{
    for( int i=0; i<b_size; i++ ){
        float tmp = lim_f((shape + *shape_audio++) * 4.0f, 0.0f, 3.999999f);
        *zone = (uint8_t)tmp;
        *coeff++ = tmp - (float)*zone++;
    }
}

const float lut_sin_half_f = LUT_SIN_HALF;

// lookup functions
float _xfade( float a, float b, float c ){ return (a + c*(b-a)); }

float _squ( float in )
{
    float sq = 2.5 - (in * 1.5);
    return (sq*sq);
}
float _log( float in )
{
    float    fix = in * lut_sin_half_f;
    uint32_t ix  = (uint32_t)fix;
    float*   lut = (float*) &log_lut[ix];
    return _xfade( *lut
                 , lut[1]
                 , (fix - (float)ix)
                 );
}
float _exp( float in )
{
    // just a backwards log lookup
    float    fix = (1 - in) * lut_sin_half_f;
    uint32_t ix  = (uint32_t)fix;
    float*   lut = (float*) &log_lut[ix];
    return _xfade( -*lut
                 , -lut[1]
                 , (fix - (float)ix)
                 );
}
float _tri( float in )
{
    return (2.0 * in - 1.0);
}
float _sin( float in )
{
    float    fix = (1.0 - in) * lut_sin_half_f;
    uint32_t ix  = (uint32_t)fix;
    float*   lut = (float*) &sine_lut[ix];
    return _xfade( *lut
                 , lut[1]
                 , (fix - (float)ix)
                 );
}

// function pointers
float _up_sq_lg( float in, float coeff ){
    return lim_f_n1_1( ( _log(in) + 1.0 )
                       * _squ(coeff)
                       - 1.0
                       );
}
float _dn_sq_lg( float in, float coeff ){
    return lim_f_n1_1( ( _exp(-in) - 1.0 )
                       * _squ(coeff)
                       + 1.0
                       );
}
float _up_lg_tr(float in, float coeff){ return _xfade( _log( in), _tri( in), coeff); }
float _dn_lg_tr(float in, float coeff){ return _xfade( _exp(-in), _tri(-in), coeff); }
float _up_tr_ex(float in, float coeff){ return _xfade( _tri( in), _exp( in), coeff); }
float _dn_tr_ex(float in, float coeff){ return _xfade( _tri(-in), _log(-in), coeff); }
float _up_ex_sn(float in, float coeff){ return _xfade( _exp( in), _sin( in), coeff); }
float _dn_ex_sn(float in, float coeff){ return _xfade( _log(-in), _sin( in), coeff); }

float shaper_apply( float     input
                  , uint8_t   zone
                  , float     coeff
                  )
{
    typedef float (func_t)( float in
                          , float coeff
                          );
    static func_t (*sh_fnptr[4][2]) =
    { { _up_sq_lg
      , _dn_sq_lg
      }
    , { _up_lg_tr
      , _dn_lg_tr
      }
    , { _up_tr_ex
      , _dn_tr_ex
      }
    , { _up_ex_sn
      , _dn_ex_sn
      } };
    return sh_fnptr[zone][input <= 0.0]( input
                                       , coeff
                                       );
}


float* shaper_apply_v( float  shape
                     , float* shape_audio
                     , float* io
                     , int    b_size
                     )
{
    float* in2 = io;
    float* out2 = io;

    uint8_t zone[b_size];
    float   coeff[b_size];

    shaper_prep_v( zone
                 , coeff
                 , shape
                 , shape_audio
                 , b_size
                 );

    for( int i=0; i<b_size; i++ ){
        *out2++ = shaper_apply( *in2++
                              , zone[i]
                              , coeff[i]
                              );
    }
    return io;
}

// can streamline in 2 ways:
	// 1. rollout funcgen changes for continuous oscillator (not up/down)
		// this avoids the RISE/FALL dichotomy
	// 2. process all chans in parallel
		// *zone stays the same for all chans for a given sample




/*
inline float curve_lookup(float samp, uint8_t dir, int16_t block){
	uint16_t trunk, trunk2; // truncated sample value to 10b
	float fmp, funk; // 0-1 interp for sample value
	float tmp, tmp2, out;

	if(dir == RISE) {
		if(curve_mode[block] == 3) { // SQ slide/fade
			fmp = sq_[(int32_t)(1023.0f * curve_mix[block])] * samp * i8192f;
			if(fmp > 1016) { fmp = 1016; } // clamp after increasing level
			trunk = (int32_t)fmp; // truncate down to int
			if(trunk < 0) { trunk = 0; }
			trunk2 = trunk+1;
			if(trunk2>1023) { trunk2 = 1023; } // limited above to 1016 anyway
			funk = fmp - (float)trunk; // find fract diff
		}
		switch(curve_mode[block]){
		case 3: { // EXP-SQ
			return (exp_[trunk] + funk*(exp_[trunk2] - exp_[trunk]));
			// break;
		}
		}
	} else if(dir != STOP) {
		if(curve_mode[block] == 3) { // SQ slide/fade
			fmp = sq_[(int32_t)(1023.0f * curve_mix[block])] * (1024-(samp * i8192f));
			if(fmp >= 1016) fmp = 1016; // clamp after increasing level
			trunk = (int32_t)fmp; // truncate down to int
			if(trunk < 0) trunk = 0;
			trunk2 = trunk+1; // higher interp value
			if(trunk2>1023) { trunk2 = 1023; }
			funk = fmp - (float)trunk; // find fract diff
		}
		switch(curve_mode[block]){
		case 3: { // EXP-SQ
			return (MAX24f-exp_[trunk] + funk*((MAX24f-exp_[trunk2]) - (MAX24f-exp_[trunk])));
			// break;
		}

}*/
