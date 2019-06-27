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

// tapehead mode, refactored from tapedeck.h
typedef enum{ READONLY
	        , OVERDUB
	        , OVERWRITE
	        , LOOPER       // overwrite, but returns old tape content (external fb)
	        , tape_mode_count
} tape_mode_t;

rhead_t* RH_Init( void );
void RH_DeInit( rhead_t* self );

void RH_cv_recording( rhead_t* self, uint8_t state );
void RH_tr_recording( rhead_t* self, uint8_t state );

void RH_set_rw( rhead_t* self, tape_mode_t rmode );

void RH_set_record_level( rhead_t* self, float level );
void RH_set_record_params( rhead_t* self
                         , float    feedback
                         , float    monitor
                         );
