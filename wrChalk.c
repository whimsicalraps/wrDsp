#include "wrChalk.h"
#include "wrChalkPriv.c"

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
	_C_find_here( self, timestamp, subsamp ); // CHECK THIS DOESN'T ALTER STATE
	self->is_cue = cue_state;

	int8_t error_state = _C_collided( self
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

cue_dir_t C_ts_is_here( C_t* self, uint32_t timestamp
	                             , float    subsamp
	                             )
{
	const float CUE_WINDOW = 300.0;
	cnode_t* n = self->nodes; // readability
	if( timestamp == n[self->C_here].ts ){
		if( subsamp < n[self->C_here].ssample + CUE_WINDOW
		 && subsamp > n[self->C_here].ssample - CUE_WINDOW ){
			return go_selected;
		}
	} else if( timestamp == (n[self->C_here].ts + SD_BLOCK_PAGES) ){
		// timestamp is 1 block after C_here
		if( (n[self->C_here].ssample - (float)SD_BLOCK_SAMPLES + CUE_WINDOW )
			> subsamp ){
			return go_selected;
		}
	} else if( timestamp == (n[self->C_here].ts - SD_BLOCK_PAGES) ){
		// timestamp is 1 block before C_here
		if( (n[self->C_here].ssample + (float)SD_BLOCK_SAMPLES - CUE_WINDOW )
			< subsamp ){
			return go_selected;
		}
	}
	return go_here;
}

uint8_t C_ts_is_addable( C_t* self, uint32_t timestamp )
{
// TODO
	// Can divide .ss by page-blocks and += here_ts & timestamp
	// thus more accurate time-distancing
	uint32_t here_ts = self->nodes[self->C_here].ts;

	if( timestamp < (here_ts - (MIN_CHALK_SPACING))
	 || timestamp > (here_ts + (MIN_CHALK_SPACING)) ){
		return 1;
	}
	return 0;
}
