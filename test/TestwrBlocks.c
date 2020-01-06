#include "unity.h"
#include "wrBlocks.h"

void test_b_cp(void){
    {
        float src[] = {-1.0};
        float expect[] = {0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_cp(src, 0.0, 1), 1);
    }
    {
        float src[] = {-1.0, 0.0, 1.0};
        float expect[] = {0.0, 0.0, 0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_cp(src, 0.0, 3), 3);
    }
    {
        float src[] = {-1.0, 0.0, 1.0};
        float expect[] = {-0.0, -0.0, -0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_cp(src, -0.0, 3), 3);
    }
}

void test_b_cp_v(void){
    {
        float src[] = {0.0};
        float set[] = {0.0};
        float expect[] = {0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_cp_v(src, set, 1), 1);
    }
    {
        float src[]    = {-4.0,2.0,8.0};
        float set[]    = {-1.0,0.0,1.0};
        float expect[] = {-1.0,0.0,1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_cp_v(src, set, 3), 3);
    }
}

void test_b_add(void){
    {
        float src[] = {-1.0};
        float expect[] = {-1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_add(src, 0.0, 1), 1);
    }
    {
        float src[] = {-1.0,0.0,1.0};
        float expect[] = {0.0,1.0,2.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_add(src, 1.0, 3), 3);
    }
}

void test_b_sub(void){
    {
        float src[] = {-1.0};
        float expect[] = {1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_sub(src, 0.0, 1), 1);
    }
    {
        float src[]    = { 2.0,0.0,1.0};
        float expect[] = {-1.0,1.0,0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_sub(src, 1.0, 3), 3);
    }
    {
        float src[]    = {2.0,0.0,1.0};
        float expect[] = {0.0,0.0,0.0};
        for( int i=0; i<3; i++ ){
            expect[i] = 1.0 - src[i];
        }
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_sub(src, 1.0, 3), 3);
    }
}

void test_b_mul(void){
    {
        float src[] = {-1.0};
        float expect[] = {0.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_mul(src, 0.0, 1), 1);
    }
    {
        float src[]    = { 2.0,0.0,1.0};
        float expect[] = { 2.0,0.0,1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_mul(src, 1.0, 3), 3);
    }
    {
        float src[]    = {2.0,0.0,-1.0};
        float expect[] = {4.0,0.0,-2.0};
        for( int i=0; i<3; i++ ){
            expect[i] = 2.0 * src[i];
        }
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_mul(src, 2.0, 3), 3);
    }
}

void test_b_accum_v(void){
    {
        float src[] = {0.0};
        float add[] = {1.0};
        float expect[] = {1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_accum_v(src, add, 1), 1);
    }
    {
        float src[]    = {-4.0, 2.0, 8.0};
        float add[]    = { 3.0,-2.0,-7.0};
        float expect[] = {-1.0, 0.0, 1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_accum_v(src, add, 3), 3);
    }
}

static float squared( float in ){ return in*in; }
void test_b_map(void){
    {
        float src[] = {-1.0};
        float expect[] = {1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_map(squared,src,1), 1);
    }
    {
        float src[]    = {-2.0,0.0,1.0};
        float expect[] = { 4.0,0.0,1.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_map(squared,src,3), 3);
    }
    {
        float src[]    = {2.0,0.0,-1.0};
        float expect[] = {0.0,0.0,0.0};
        for( int i=0; i<3; i++ ){
            expect[i] = squared( src[i] );
        }
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_map(squared,src,3), 3);
    }
}

void test_b_accum_vv(void){
    {
        float src1[] = {1.0};
        float src2[] = {2.0};
        float* srcs[] = {src1,src2};
        float expect[] = {3.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_accum_vv(srcs, 2, 1), 1);
    }
    {
        float src1[] = {1.0,0.0,-1.0};
        float src2[] = {2.0,3.0,4.0};
        float* srcs[] = {src1,src2};
        float expect[] = {3.0,3.0,3.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_accum_vv(srcs, 2, 3), 3);
    }
    {
        float src1[] = {1.0,0.0,-1.0};
        float src2[] = {2.0,3.0,4.0};
        float* srcs[] = {src1,src2};
        float expect[] = {0.0,0.0,0.0};
        for( int i=0; i<3; i++ ){
            expect[i] = src1[i] + src2[i];
        }
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_accum_vv(srcs, 2, 3), 3);
    }
}

void test_b_reduce_vv(void){
    {
        float src1[] = {1.0};
        float src2[] = {2.0};
        float* srcs[] = {src1,src2};
        float expect[] = {3.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_reduce_vv(b_accum_v,srcs, 2, 1), 1);
    }
    {
        float src1[] = {1.0,0.0,-1.0};
        float src2[] = {2.0,3.0,4.0};
        float* srcs[] = {src1,src2};
        float expect[] = {3.0,3.0,3.0};
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_reduce_vv(b_accum_v,srcs, 2, 3), 3);
    }
    {
        float src1[] = {1.0,0.0,-1.0};
        float src2[] = {2.0,3.0,4.0};
        float* srcs[] = {src1,src2};
        float expect[] = {0.0,0.0,0.0};
        for( int i=0; i<3; i++ ){
            expect[i] = src1[i] + src2[i];
        }
        TEST_ASSERT_EQUAL_FLOAT_ARRAY(expect, b_reduce_vv(b_accum_v,srcs, 2, 3), 3);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_b_cp);
    RUN_TEST(test_b_cp_v);
    RUN_TEST(test_b_add);
    RUN_TEST(test_b_sub);
    RUN_TEST(test_b_mul);
    RUN_TEST(test_b_accum_v);
    RUN_TEST(test_b_map);
    RUN_TEST(test_b_accum_vv);
    RUN_TEST(test_b_reduce_vv);

    return UNITY_END();
}
