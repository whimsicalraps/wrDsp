#include "wrFilter.h" // filter_lp

#define HEAD_SLEW_RECORD   ((float)(0.05))
#define HEAD_SLEW_FEEDBACK ((float)(0.05))
#define HEAD_SLEW_MONITOR  ((float)(0.05))


typedef struct{
    filter_lp1_t* record;
    filter_lp1_t* feedback;
    filter_lp1_t* monitor;

    uint8_t       cv_recording;
    uint8_t       tr_recording;
} rhead_t;

// tapehead mode
typedef enum{ READONLY
	        , OVERDUB
	        , OVERWRITE
	        , LOOPER       // overwrite, but returns old tape content (external fb)
	        , tape_mode_count
} tape_mode_t;

rhead_t* RH_Init( void );
void RH_DeInit( rhead_t* self );
