#pragma once

/*
  TODO:
  Initialize page_buffer_ts as buffer_ts
*/

typedef struct{
    int* byte_array;
    //buffer_state_t
    //int* metadata
} buffer_t;

typedef enum{
    full,
    empty,
    write
} buffer_state_t;

// RAM buffer layout -> should be made more programatic
// SD_Init() should return SD_QUEUE_LENGTH and pass to R_Init() which builds the list
// This provides a good opportunity to switch to a more functional malloc() style
typedef enum{ a
            , b
            , c
            , d
            , e
            , f
            , g
            , h
            , i
            , j
            , last_scratch_buf

            , BUF_A_prev
            , BUF_A_next
            , BUF_B_prev
            , BUF_B_next

            , BUF_S_0
            , BUF_S_1

            , PAGE_COUNT
} page_in_buf_t;

// pointers to RAM buffers. they start matching above, then get shuffled.
typedef enum{ p_skrt
            , p_prev
            , p_here
            , p_next
            , p_e
            , p_f
            , p_g
            , p_h
            , p_i
            , p_j
            , p_unused

            , p_before0
            , p_before1
            , p_after0
            , p_after1

            , p_selected0
            , p_selected1

            , p_access_count
} p_access_list_t;

typedef enum{ buf_left  = -1
            , buf_stay  =  0
            , buf_right =  1
} buf_dir_t;
