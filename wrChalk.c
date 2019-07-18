#include "wrChalk.h"
#include <stdlib.h>
#include <stdio.h>

#include "../../lib/sdfs.h"     // #SDCARD_CUE_PAGES, #SDCARD_PAGE_SIZE
#include "../../lib/sdio.h"     // SD_rw_access*(), #SD_BLOCK_PAGES, #SD_BLOCK_BYTES


// private defns
static void _C_defaults( C_t* self );
static uint8_t _C_load_list( C_t* self );
static void _C_restore_data( C_t* self );
static void _C_add( C_t*    self
		          , float   subsamp
		          , int32_t ts
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
	} else if( self->n_count > CHALK_LIMIT - 128
		    && self->n_next > self->n_count ){
	}

    _C_restore_data( self );

    printf("_C_load_list()\n");
	//_C_print_list( self );

	return 0;
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

// public defns
C_t* C_init( void )
{
    C_t* self = malloc( sizeof( C_t ) );
    if( !self ){ printf("Chalk: malloc failed\n"); return NULL; }

	self->nodes = malloc( SDCARD_CUE_PAGES * SDCARD_PAGE_SIZE );
	if( !self->nodes ){ printf("C_t self->nodes!\n"); return NULL; }

	C_load( self, 1 ); // force defaults

    return self;
}

// call this after factory_reset to boot as per normal
void C_deinit( C_t* self )
{
	free( self->nodes ); self->nodes = NULL;
    free( self ); self = NULL;
}

void C_load( C_t* self, uint8_t fresh )
{
	self->dirty      = 0;
	self->meta_dirty = 0;
	if( fresh ){
        printf("C_preload() fresh\n");
		_C_defaults( self );  // set default values
	} else {
		if( _C_load_list( self ) ){ // invalid data returned
			printf("C_preload() invalid cue data\n");
		    if( _C_load_list( self ) ){ // RETRY
        //TODO
                // flip the sd_cue_select
                // RETRY with new cue_select pointer
			    _C_defaults( self ); // GIVEUP & RESET
            }
		}
	}
	if( self->loop_active ){
		self->C_loop_start = self->C_here;
		self->C_loop_end   = self->nodes[self->C_here].next;
	}
	self->is_cue        = 0;
    self->follow_action = 0;
	self->cue_in_motion = 0;
}

void C_save_list( C_t* self )
{
	if( !self->dirty ){ return; } // don't write if no change!

	// _C_print_list( self );

	cnode_t* n = self->nodes; // readability

	n[self->n_next].ts = -1; // designate end of list

	uint32_t len = (sizeof(cnode_t) * (self->n_next + 1)) / SDCARD_PAGE_SIZE + 1;
	if( len > SDCARD_CUE_PAGES ){ len = SDCARD_CUE_PAGES; } // rounding error

	if( len > SD_BLOCK_PAGES ){ // more than 1 meta-page. do 2nd half first
		sdfs_access_t p = { .cmd        = sdfs_c_write
	                      , .status     = sdfs_busy
	                      , .locn       = SD_BLOCK_PAGES
	                      , .page_count = len - SD_BLOCK_PAGES
	                      , .tr         = ((uint8_t*)n) + SD_BLOCK_BYTES
	                      };
		SD_rw_access( &p );
		len = SD_BLOCK_PAGES; // load first half in full
	}

	sdfs_access_t q = { .cmd        = sdfs_c_write
                      , .status     = sdfs_busy
                      , .locn       = 0
                      , .page_count = len
                      , .tr         = (uint8_t*)n
                      };
	SD_rw_access( &q );

	self->dirty = 0;
	// printf("write_clist\n");
}


// ADD A MARK
int8_t C_add( C_t*     self
            , uint32_t timestamp
            , float    subsamp
            )
{
	// first correct for the real 'here' cue
	// deactivate is_cue to access self->C_here (not C_SELECTED)
	uint8_t cue_state = self->is_cue;
	self->is_cue = 0;
//TODO
	C_find_here( self, timestamp, subsamp ); // CHECK THIS DOESN'T ALTER STATE
	self->is_cue = cue_state;

	int8_t error_state = C_collided( self
		                            , self->C_here // check collision with OG sample
		                            , timestamp
		                            , C_EDIT_ADD
		                            );
	if( !error_state ){
        _C_add( self
		      , subsamp
		      , timestamp
		      );
    } else { printf("C_add() collided %i\n", (int)error_state ); }
	return error_state;
}


// REMOVE A MARK
int8_t C_rm( C_t* self, C_ix_t* rm )
{
	if( *rm == C_start ){ return -1; }
	else if( *rm == C_end ){ return 1; }

	cnode_t* n = self->nodes;
	C_ix_t before = n[*rm].prev;
	C_ix_t after  = n[*rm].next;

	// skip over the deleted node
	n[before].next = after;
	n[after].prev  = before;

	// if deleting the most recently added, we can avoid frag
	if( *rm == (self->n_next-1) ){ self->n_next--; }
	// otherwise, we ignore & wait to cleanup later
	self->n_count--;

	*rm = before; // update whichever pointer was used

	self->dirty = 1;
	return 0;
}


uint8_t C_islooping( C_t* self )
{
	if(self->is_cue){ return 1; } // CUE mode forces looping
	return self->loop_active;
}


void C_loop_here( C_t* self )
{
	if( self->nodes[self->C_here].next == C_end ){
        if(self->loop_active){
            self->loop_active = 0;
            self->meta_dirty = 1;
        }
		return; // CAN'T LOOP TO END
	}
    if( self->C_loop_start != self->C_here
     || self->C_loop_end   != self->nodes[self->C_here].next ){
	    self->C_loop_start = self->C_here;
	    self->C_loop_end   = self->nodes[self->C_here].next;
        self->meta_dirty   = 1;
    }
    if( self->loop_active != 1 ){
	    self->loop_active   = 1;
	    self->meta_dirty    = 1;
    }
}


uint32_t C_get_ts( C_t* self, C_ix_t node )
{
	return self->nodes[node].ts;
}

float C_get_ss( C_t* self, C_ix_t node )
{
	return self->nodes[node].ssample;
}


C_ix_t C_Here( C_t* self )
{
	if( self->is_cue ){ return C_SELECTED; }
	else{ return self->C_here; }
}


void C_goto( C_t* self, C_ix_t dest )
{
	cnode_t* n = self->nodes;

	if( dest == C_end){
		printf("C_goto() @end\n");
		return;
	}

    // mark dirty only if jumping to new location
    if( dest != self->C_here ){ self->meta_dirty = 1; }

	if( dest != C_SELECTED ){ self->C_here = dest; }

	if( self->is_cue ){
		C_set_select( self
                    , C_get_ts( self, C_Here(self) )
                    , C_get_ss( self, C_Here(self) )
                    );
		self->C_loop_start = C_Here(self); // for looping-audition
		self->C_loop_end   = n[C_Here(self)].next;
	} else if( C_islooping(self) ){
		self->C_loop_start = C_Here(self);
		if( n[C_Here(self)].next == C_end ){
			C_loop_active( self, 0 ); // can't loop to end
		} else {
			self->C_loop_end = n[C_Here(self)].next;
		}
	}
}


void C_set_select( C_t*     self
                 , uint32_t timestamp
                 , float    subsamp
                 )
{
	// copy C_Here pointers, but take 'now' time
	// note we allow Start & End here
	cnode_t* n = self->nodes;
	n[C_SELECTED].ts      = timestamp;
	// printf("C_s_s %i\n", (int)timestamp);
	n[C_SELECTED].ssample = subsamp;
	n[C_SELECTED].prev    = n[self->C_here].prev;
	n[C_SELECTED].next    = n[self->C_here].next;
}


void C_loop_active( C_t* self, uint8_t active )
{
	if(self->loop_active != active ){
		if(active){
			self->C_loop_start = self->C_here;
			if( self->nodes[self->C_here].next == C_end ){
				return; // CAN'T LOOP TO END
			}
			self->C_loop_end = self->nodes[self->C_here].next;
		}
		self->meta_dirty  = 1;
	}
	self->loop_active = !!active;
}

void C_find_here( C_t* self, uint32_t ix, float ss )
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
               printf("C_find_here() failed\n");
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
               printf("C_find_here() failed1\n");
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

int8_t C_collided( C_t*     self
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
