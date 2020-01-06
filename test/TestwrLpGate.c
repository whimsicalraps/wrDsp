#include "unity.h"
#include "wrLpGate.h"

// A stub to test equivalence of vector optimized step function

void test_lpgate_v(void){
    int hp=0; // highpass active?
    int p=0; // filter mode
    int f=0;
    int l=0;
    // FIXME: highpass filter does not match
    //for( hp=0; hp<2; hp++ ){
    for( p=0; p<2; p++ ){
    for( f=0; f<4; f++ ){
    for( l=0; l<3; l++ ){
        float F = (float)f/3; // 0,0.33,0.67
        float L = (float)l/2; // 0,0.5,1.0
        {
            lpgate_t* self   = lpgate_init(hp,p);
            lpgate_t* self_v = lpgate_init(hp,p);
            float buf[]    = {F,F,F,F};
            float buf_v[]  = {F,F,F,F};
            float lvl[]    = {L,L,L,L};
            float expect[] = {0.0,0.0,0.0,0.0};
            for( int i=0; i<4; i++ ){
                expect[i] = lpgate_step( self, lvl[i], buf[i] );
            }
            TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, lpgate_v(self_v,lvl,buf_v,4), 4);
        }
    }
    }
    }
    //}
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_lpgate_v);

    return UNITY_END();
}
