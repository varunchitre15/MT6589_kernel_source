#if !defined (AEE_COMMON_H)
#define AEE_COMMON_H

int get_memory_size(void);

int in_fiq_handler(void);

int aee_dump_stack_top_binary(char *buf, int buf_len, unsigned long bottom, unsigned long top);

#endif /* AEE_COMMON_H */
