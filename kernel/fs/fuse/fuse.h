#define FUSE_MIGHT_FREEZE(superblock, desc) \
do { \
	int printed = 0; \
	while (superblock->s_frozen != SB_UNFROZEN) { \
		if (!printed) { \
			printk(KERN_INFO "%d frozen in " desc ".\n", \
						current->pid); \
			printed = 1; \
		} \
		try_to_freeze(); \
		yield(); \
	} \
} while (0)
