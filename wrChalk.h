#include <stdint.h>

#define LAST_CHALK_DISTANCE 0x100 // distance to push last-chalk-marker
#define CHALK_LIMIT         1365 // max count of chalks per tape (500?)
#define MIN_CHALK_SPACING   64
#define C_SELECTED          ((int16_t)(CHALK_LIMIT - 1))

/*
	ToDo List:
	1.) Remove C_goto()?
	2.) Remove sdfs.h/sdio.h references
*/


typedef int16_t C_ix_t; // this is just an alias. no type safety. just visual

typedef struct cnode{
	float         ssample; // subsample position!
	int32_t       ts;      // timestamp (in increments of uSD block size)
	C_ix_t        prev;
	C_ix_t        next;

	// 'actions' would be stored in here
	// loop, skip or RAM destination
	// speed
	// direction
	// record state
} cnode_t;

typedef enum
	{ ls_none
	, ls_neighbourcheck
	, ls_stop
	, ls_crossed
	, ls_loopend
	, ls_loopstart
	} C_ls_t;


typedef enum
    { go_prev = -1
    , go_here =  0
    , go_next =  1
    , go_nnext = 2
    , go_selected = 0x7F
    } cue_dir_t;

typedef enum
	{ C_start = 0
	, C_end   = 1
	} C_names_t;

typedef enum
    { C_EDIT_ADD
    , C_EDIT_MOVE
    } C_EDIT_t;

typedef struct{
	cnode_t* nodes;

	int16_t  n_next;       // highest indexed node
	int16_t  n_count;      // number of current nodes in CLL

	C_ix_t   C_here;       // chalk pointer: most recent chalk to pass tapehead
	C_ix_t   C_loop_start;
	C_ix_t   C_loop_end;
	int8_t   loop_active;

	uint8_t  is_cue;
    uint8_t  follow_action;
	uint8_t  cue_in_motion;
    C_ls_t   lock;         // cue_in_motion is true & we're at the limit
	int8_t   dirty;        // changes to list
	int8_t   meta_dirty;   // pointers & loop state

	int32_t  touched_by_angel; // the sample with highest timestamp that has been written
} C_t;

C_t* C_init( void );
void C_deinit( C_t* self );
void C_load( C_t* self, uint8_t fresh );
void C_save_list( C_t* self );

int8_t C_add( C_t* self
            , uint32_t timestamp
            , float    subsamp
            );
int8_t C_rm( C_t* self, C_ix_t* node ); // for C_here or C_selected
C_ix_t C_Here( C_t* self );
void C_goto( C_t* self, C_ix_t dest );

uint8_t C_islooping( C_t* self );
void C_loop_here( C_t* self );

uint32_t C_get_ts( C_t* self, C_ix_t node );
float C_get_ss( C_t* self, C_ix_t node );

void C_loop_active( C_t* self, uint8_t active );
void C_set_select( C_t*     self
                 , uint32_t timestamp
                 , float    subsamp
                 );


void C_find_here( C_t* self, uint32_t ix, float ss );
int8_t C_collided( C_t* self
				         , int16_t candidate
				         , uint32_t timestamp
				         , C_EDIT_t move
				         );
