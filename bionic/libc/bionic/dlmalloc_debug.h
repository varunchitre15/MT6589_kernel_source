


#ifndef RUNTIME_CHECK
#define RUNTIME_CHECK
enum chunk_position {
	chunk_prev,
	chunk_current,
	chunk_next
};

enum structure_type {
	structure_none,
	chunk_free = (0x1 <<1),
	chunk_Inuse = (0x1 <<2),
	chunk_mapped= (0x1 <<3),
	structure_segement= (0x1 <<4),
	structure_mstate= (0x1 <<5),
	chunk_None= (0x1 <<6),
	max_type = (0x1 << 7)
};

enum error_member {
	member_none,
	chunk_member_prev_foot = (0x1 << 1),
	chunk_member_prev_foot_mapped_bit = (0x1 << 2),
	chunk_member_head_size = (0x1 << 3),
	chunk_member_head_tree_size = (0x1 << 4),
	chunk_member_head_current_free_bit = (0x1 << 5),	
	chunk_member_head_previous_free_bit = (0x1 << 6),
	chunk_member_head_fencepost_bit = (0x1 << 7),
	chunk_member_head_none_bit = (0x1 << 8),
	chunk_member_bk = (0x1 << 9),
	chunk_member_fd = (0x1 << 10),
	chunk_member_index= (0x1 << 11),
	chunk_member_left_child = (0x1 << 12),
	chunk_member_right_child= (0x1 << 13),
	chunk_member_parent= (0x1 << 14),
	chunk_address = (0x1 << 15),
	msegment_size = (0x1 << 16),
	msegment_address = (0x1 << 17),
	mstate_flag_mmap = (0x1 << 18),
	mstate_top_size = (0x1 << 19),
	mstate_top = (0x1 << 20),
	mstate_tree_map = (0x1 <<21),
	mstate_small_map = (0x1 <<22),
	mstate_dv_size = (0x1 <<23),
	mstate_dv = (0x1 <<24),
	mstate_footprint = (0x1 <<25),
	mstate_magic = (0x1 << 26)
};

enum dlmalloc_function {
	action_dlmalloc,
	action_dlfree,
	action_dlrealloc,
	action_destroy_mspace,
	action_mspaceMalloc,
	action_mspaceFree,
	action_mspaceCalloc,
	action_mspaceRealloc,
	action_mmap_alloc,
	action_do_check_malloc_state,
	action_do_find_valid_chunk,
	action_mergeObject,
	action_prepend_alloc,
	action_add_segment,
	action_sys_alloc,
	action_release_unused_segment,
	action_sys_trim,
	action_trim,
	action_tmalloc_large,
	action_tmalloc_small,
	action_internal_realloc,
	action_mspace_memalign,
	action_mspace_independent_comalloc,
	action_internal_memalign,
	action_mspace_independent_calloc,
	action_mspace_trim,
	action_mspace_malloc_stats,
	action_mspace_footprint,
	action_mspace_max_allowed_footprint,
	action_mspace_set_max_allowed_footprint,
	action_mspace_max_footprint,
	action_mspace_mallinfo,
	action_mspace_walk_free_pages,
	action_mspace_walk_heap,
	action_mmap_resize,
	action_dlindependent_calloc,
	action_dlindependent_comalloc,
	action_init_user_mstate,
	action_function_max
};

enum dlmalloc_action {
	action_none,
	from_smallbin_fit,
	from_smallbin,
	from_treebin_small,
	from_treebin_large,
	from_dv,
	from_top,
	from_mmap,
	to_smallbin,
	to_treebin,
	to_dv,
	to_top,
	check_tree_bin,
	check_small_bin,
	check_traversal,
	check_dv,
	check_top,
	action_max	
};

enum error_type{
	error_double_free,
	error_use_after_free,
	error_chunk_overflow,
	error_structure_corruption	
};

struct ChunkDebug_Info 
{
	void *	record_mstate;
	unsigned int	record_chunk;
	unsigned int	record_chunk_size;
	unsigned int	record_type;
	unsigned int	record_address;
	unsigned int	record_error_member;
	unsigned int	record_function;
	unsigned int	record_action;
	unsigned int	record_error_type;
	unsigned int	record_error_flag;

};
struct ErrorReport
{
	unsigned int	ErrType;
	unsigned int    ErrStructureType;
	unsigned int    ErrStructureMember;
	void * 		ErrMstate;
	void *		ErrSeg;
	unsigned int    ErrAddr;
	unsigned int    ErrStartAddr;
	unsigned int    ErrEndAddr;
	unsigned int    PreChunkAddr;
	unsigned int    NextChunkAddr;
	void  *MallocBt;
	void *FreeBt;
};

typedef struct ChunkDebug_Info chunkDebug;
typedef struct ChunkDebug_Info *chunkDebugPtr;
#endif
