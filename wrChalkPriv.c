/*
  Temporary file to distribute private functions to be included in wrChalk.c and
  chalk.c during the refactoring process..

  ToDo list:
  1.) Remove sdfs.h/sdio.h from wrChalk.c (#SD_CARD stuff getting in the way)
  2.) _C_defrag_list is empty, this should be deleted?
*/

#include <stdlib.h>
#include <stdio.h>

#include "../../lib/sdfs.h"     // #SDCARD_CUE_PAGES, #SDCARD_PAGE_SIZE
#include "../../lib/sdio.h"     // SD_rw_access*(), #SD_BLOCK_PAGES, #SD_BLOCK_BYTES

// private enums
typedef enum
    { C_EDIT_ADD
    , C_EDIT_MOVE
    } C_EDIT_t;

// private defns
static void _C_defaults( C_t* self );
static uint8_t _C_load_list( C_t* self );
static void _C_defrag_list( C_t* self );
static void _C_restore_data( C_t* self );
static void _C_add( C_t*    self
		          , float   subsamp
		          , int32_t ts
		          );
static void _C_find_here( C_t* self, uint32_t ix, float ss );
static int8_t _C_collided( C_t* self
				         , int16_t candidate
				         , uint32_t timestamp
				         , C_EDIT_t move
				         );

static void _C_defaults( C_t* self )
{
	// manually setup start/end points
	self->nodes[C_start].ssample = 0.0;
	self->nodes[C_start].ts      = SD_BLOCK_PAGES;
	self->nodes[C_start].prev    = -1;
	self->nodes[C_start].next    = C_end;

	self->nodes[C_end].ssample = 0.0;
	self->nodes[C_end].ts      = LAST_CHALK_DISTANCE;
	self->nodes[C_end].prev    = C_start;
	self->nodes[C_end].next    = -1;

	self->n_count = 2; // how many nodes
	self->n_next  = 2; // ix of next free node

	// 'pointers' for playback & edit actions
	self->C_here       = C_start;
	self->C_loop_start = C_start;
	self->C_loop_end   = C_end;

	self->loop_active = 0;

	self->touched_by_angel = 0;

    // force write
    self->dirty      = 1;
    self->meta_dirty = 1;
}


static uint8_t _C_load_list( C_t* self )
{
	// here we should create temp buffer:
		// 1: query meta if we need 2 reads, or 1 is enough
		// 2: traverse LL and rebuild into sequential indices
	// for now, we leave the cuelist fragmented (1638 cue additions / tape)
	cnode_t* n = self->nodes; // readability

	sdfs_access_t p = { .cmd        = sdfs_c_read
                      , .status     = sdfs_busy
                      , .locn       = 0
                      , .page_count = SD_BLOCK_PAGES
                      , .tr         = (uint8_t*)n
                      };
	SD_rw_access_now( &p );
	SD_callback_wait( sdfs_c_read );

	sdfs_access_t q = { .cmd        = sdfs_c_read
                      , .status     = sdfs_busy
                      , .locn       = SD_BLOCK_PAGES
                      , .page_count = SD_BLOCK_PAGES
                      , .tr         = (uint8_t*)n + SD_BLOCK_BYTES
                      };
	SD_rw_access_now( &q );
	SD_callback_wait( sdfs_c_read );

	C_ix_t ix     = C_start;
// Confirm page has not been erased.
    if( n[C_start].prev < -1
     || n[C_start].prev > CHALK_LIMIT
     || n[C_start].next < C_end // will fail if .next is 0
     || n[C_start].next > CHALK_LIMIT ){ return 1; } // page is prob blank

	self->n_count = 1;
	self->n_next  = 1;
	int16_t timeout = CHALK_LIMIT;
	do {
		self->n_count++;
		if( ix >= self->n_next ){
			self->n_next = ix+1;
		}
		ix = n[ix].next;
		if( timeout == 0 ){ return 1; } // list is non-valid
		timeout--;
	} while( ix != C_end );
    self->n_count++; // leave room for C_end
    if( ix >= self->n_next ){
		self->n_next = ix+1;
	}

    if( self->n_next > self->n_count + 64 ){
		_C_defrag_list( self );
	} else if( self->n_count > CHALK_LIMIT - 128
		    && self->n_next > self->n_count ){
		_C_defrag_list( self );
	}

    _C_restore_data( self );

    printf("_C_load_list()\n");
	//_C_print_list( self );

	return 0;
}

static void _C_defrag_list( C_t* self )
{
	//
}

static void _C_restore_data( C_t* self )
{
	cnode_t* n = self->nodes;

    if( n[C_start].ts != SD_BLOCK_PAGES ){
        if( n[C_start].ts < SD_BLOCK_PAGES ){ // TS has been moved *backward*
            n[C_start].ts = SD_BLOCK_PAGES;
            while( n[n[C_start].next].ts < (SD_BLOCK_PAGES + MIN_CHALK_SPACING ) ){
                if( C_rm( self, &(n[C_start].next) ) ){
                    printf("can't rm s-b-s\n");
                } else { printf("rm start-before-start\n"); }
            }
        } else { // TS has been moved *forward*
            uint32_t sts = n[C_start].ts;
            float    sss = n[C_start].ssample;
            n[C_start].ts = SD_BLOCK_PAGES;
            n[C_start].ssample = 0.0;
            if( C_add( self, sts, sss ) ){ printf("can't fix s-a-s\n"); }
            else { printf("correct start-after-start\n"); }
        }
    }

    if( n[C_end].ts > SDCARD_PAGE_TOTAL ){ // Cues exist *after* end of tape
        n[C_end].ts = SDCARD_PAGE_TOTAL;
        while( n[n[C_end].prev].ts > SDCARD_PAGE_TOTAL ){
            if( C_rm( self, &(n[C_end].prev) ) ){
                printf("can't rm e-a-e\n");
            } else { printf("rm end-after-end\n"); }
        }
    }
}

static void _C_add( C_t*    self
		          , float   subsamp
		          , int32_t ts
		          )
{
	cnode_t* n = self->nodes;

	if( self->n_count >= CHALK_LIMIT ){
		printf("C_add() out of room\n"); return;
	} else if( self->n_next >= CHALK_LIMIT ){
		printf("C_add() need defrag\n"); return;
	}

	cnode_t* new = &(n[self->n_next]);

	new->ssample = subsamp;
	new->ts      = ts;
	new->prev    = self->C_here;
	new->next    = n[self->C_here].next;

	// splice pointers to new node
	n[new->prev].next = self->n_next;
	n[new->next].prev = self->n_next;

	self->C_here = self->n_next; // mark new node as 'here'

	if(C_islooping(self)){
		C_loop_here(self);
	}
	self->n_next++;
	self->n_count++;
	self->dirty = 1;
}

static void _C_find_here( C_t* self, uint32_t ix, float ss )
{
	cnode_t* n = self->nodes;
	if( ix > C_get_ts( self, C_Here(self) ) ){
		// must be forward
	    int16_t timeout = CHALK_LIMIT;
		while( ix > C_get_ts( self, n[C_Here(self)].next )
			&& n[C_Here(self)].next != C_end ){
            printf("C_find_here() find_nx\n");
			C_goto( self, n[C_Here(self)].next );
		    if( timeout == 0 ){
                // TODO how to recover from failed cue lookup?
                printf("_C_find_here() failed\n");
                return;
            }
		    timeout--;
		}
		if( n[C_Here(self)].next == C_end ){
			// HERE is correct, but need to push soft-end
			printf("C_find_here() nudge-inside\n");
			n[C_end].ts = LAST_CHALK_DISTANCE + (n[C_end].ts > ix)
												? n[C_end].ts
												: ix;
		} else if( ix == C_get_ts( self, n[C_Here(self)].next ) ){

			if( ss >= C_get_ss( self, n[C_Here(self)].next ) ){
			printf("C_find_here() nudge-inside2\n");
				C_goto( self, n[C_Here(self)].next );
			} else {} // CORRECT
		}
	}

	else {
		// must be backward
	    int16_t timeout = CHALK_LIMIT;
		while( ix < C_get_ts( self, C_Here(self) ) ){
			C_goto( self, n[C_Here(self)].prev );
            printf("C_find_here() find_pv\n");
		    if( timeout == 0 ){
                // TODO how to recover from failed cue lookup?
                printf("_C_find_here() failed1\n");
                return;
            }
		    timeout--;
		}
		if( ix == C_get_ts( self, C_Here(self) ) ){
			if( ss < C_get_ss( self, C_Here(self) ) ){
				C_goto( self, n[C_Here(self)].prev );
			} else {} // CORRECT
		}
	}
}

static int8_t _C_collided( C_t*     self
				         , C_ix_t   candidate
				         , uint32_t timestamp
				         , C_EDIT_t move  // flag to say "C_here is allowed"
				         )
{
	cnode_t* n = self->nodes;

	int8_t retval = 0;
	cnode_t* candy = &(n[candidate]);
	// only detects motion within it's current space
	// need to handle case where it jumps over a neighbouring marker
	uint32_t p_ts = (move == C_EDIT_MOVE)
                        ? n[candy->prev].ts
                        : candy->ts;
	if( timestamp - MIN_CHALK_SPACING <= p_ts ){ retval = -1; }
    else if( timestamp + MIN_CHALK_SPACING >= n[candy->next].ts
		  && candy->next != C_end ){ retval = 1; }
	return retval;
}
