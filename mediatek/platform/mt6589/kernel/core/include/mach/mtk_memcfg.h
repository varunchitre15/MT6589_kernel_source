
#ifndef __MTK_MEMCFG_H__
#define __MTK_MEMCFG_H__

#ifdef CONFIG_MTK_MEMCFG

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...)  \
    do {    \
        printk(fmt, ##arg); \
        mtk_memcfg_write_memory_layout_buf(fmt, ##arg); \
    } while (0)

extern void mtk_memcfg_write_memory_layout_buf(char *, ...); 
extern unsigned long mtk_memcfg_get_force_inode_gfp_lowmem(void);
extern unsigned long mtk_memcfg_set_force_inode_gfp_lowmem(unsigned long);
#ifdef CONFIG_SLUB_DEBUG
extern unsigned long mtk_memcfg_get_bypass_slub_debug_flag(void);
extern unsigned long mtk_memcfg_set_bypass_slub_debug_flag(unsigned long);
#else
#define mtk_memcfg_get_bypass_slub_debug_flag() (0)
#define mtk_memcfg_set_bypass_slub_debug_flag(flag) (0)
#endif /* end CONFIG_SLUB_DEBUG */

#else

#define MTK_MEMCFG_LOG_AND_PRINTK(fmt, arg...)  \
    do {    \
        printk(fmt, ##arg); \
    } while (0)

#define mtk_memcfg_get_force_inode_gfp_lowmem() (0)
#define mtk_memcfg_set_force_inode_gfp_lowmem(flag) (0)
#define mtk_memcfg_get_bypass_slub_debug_flag() (0)
#define mtk_memcfg_set_bypass_slub_debug_flag(flag) (0)
#define mtk_memcfg_write_memory_layout_buf(fmt, arg...) do { } while (0)

#endif /* end CONFIG_MTK_MEMCFG */

#endif /* end __MTK_MEMCFG_H__ */
