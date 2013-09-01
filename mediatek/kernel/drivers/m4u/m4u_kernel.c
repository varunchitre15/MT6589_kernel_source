#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/timer.h>
#include <linux/cache.h>
#include <linux/xlog.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/m4u_profile.h>

#ifdef MTK_M4U_DBG
#define M4UDBG(string, args...)	xlog_printk(ANDROID_LOG_DEBUG, "M4U_K", string, ##args);
#else
#define M4UDBG(string, args...)
#endif

// for error msg, high priority
#define MTK_M4U_MSG
#ifdef MTK_M4U_MSG
#define M4UMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, "M4U_K", "[pid=%d]"string,current->tgid,##args);
#define M4UWARN(string, args...)	xlog_printk(ANDROID_LOG_WARN, "M4U_K", "[pid=%d]"string,current->tgid,##args);
#else
#define M4UMSG(string, args...)
#define M4UWARN(string, args...)
#endif

#define M4UERR(string, args...) do { \
	xlog_printk(ANDROID_LOG_ERROR, "M4U_K", "[pid=%d]error_assert_fail: "string,current->tgid,##args);  \
	aee_kernel_exception("M4U", "[M4U_K] error:"string,##args);  \
}while(0)


extern unsigned long max_pfn;

unsigned char *pMlock_cnt;
EXPORT_SYMBOL(pMlock_cnt);

unsigned int mlock_cnt_size = 256*1024;
EXPORT_SYMBOL(mlock_cnt_size);

MMP_Event M4U_MMP_Events[PROFILE_MAX];
EXPORT_SYMBOL(M4U_MMP_Events);

void mlock_vma_page(struct page *page);
void munlock_vma_page(struct page *page);


unsigned long m4u_virt_to_phys(const void *v)
{
    return virt_to_phys(v);
}
EXPORT_SYMBOL(m4u_virt_to_phys);


struct page* m4u_pfn_to_page(unsigned int pfn)
{
    return pfn_to_page(pfn);
}
EXPORT_SYMBOL(m4u_pfn_to_page);

unsigned int m4u_page_to_phys(struct page* pPage)
{
    return page_to_phys(pPage);
}
EXPORT_SYMBOL(m4u_page_to_phys);

struct page* m4u_phys_to_page(unsigned int phys)
{
    return phys_to_page(phys);
}
EXPORT_SYMBOL(m4u_phys_to_page);

unsigned int m4u_page_to_pfn(struct page* pPage)
{
    return page_to_pfn(pPage);
}
EXPORT_SYMBOL(m4u_page_to_pfn);

void init_mlock_cnt(void)
{
    //allocate pMlock_cnt
    mlock_cnt_size = max_pfn + PHYS_PFN_OFFSET;
    pMlock_cnt= (unsigned char*) vmalloc(mlock_cnt_size);
    if(!pMlock_cnt)
    {
        M4UMSG("allocate for pMlock_cnt error! size=%d.\n", mlock_cnt_size);
        return;
    }
    //printk("max_pfn=%d, offset=%d\n", max_pfn, PHYS_PFN_OFFSET );
    
    memset(pMlock_cnt, 0, mlock_cnt_size);
    M4UMSG("allocate for pMlock_cnt done: size=%d\n", mlock_cnt_size);
}
EXPORT_SYMBOL(init_mlock_cnt);


#if 0  // use new user_v2p()
unsigned int m4u_user_v2p(unsigned int va)
{
    unsigned int pmdOffset = (va & (PMD_SIZE - 1));
    unsigned int pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *pte;
    unsigned int pa;
    
    if(NULL==current)
    {
    	  M4UMSG("error: m4u_user_v2p, current is NULL! \n");
    	  return 0;
    }
    if(NULL==current->mm)
    {
    	  M4UMSG("error: m4u_user_v2p, current->mm is NULL! tgid=0x%x, name=%s \n", current->tgid, current->comm);
    	  return 0;
    }
        
    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
    M4UDBG("m4u_user_v2p(), pgd 0x%x\n", pgd);    
    M4UDBG("pgd_none=%d, pgd_bad=%d\n", pgd_none(*pgd), pgd_bad(*pgd));
    
    if(pgd_none(*pgd)||pgd_bad(*pgd))
    {
        M4UMSG("warning: m4u_user_v2p(), va=0x%x, pgd invalid! \n", va);
        return 0;
    }
    
    pmd = pmd_offset(pgd, va);
    M4UDBG("m4u_user_v2p(), pmd 0x%x\n", pmd);
    M4UDBG("pmd_none=%d, pmd_bad=%d, pmd_val=0x%x\n", pmd_none(*pmd), pmd_bad(*pmd), pmd_val(*pmd));
   
    /* If this is a page table entry, keep on walking to the next level */ 
    if (( (unsigned int)pmd_val(*pmd) & PMD_TYPE_MASK) == PMD_TYPE_TABLE)
    {
        if(pmd_none(*pmd)||pmd_bad(*pmd))
        {
            M4UDBG("warning: m4u_user_v2p(), va=0x%x, pmd invalid! \n", va);
            return 0;
        }
        
        // we encounter some pte not preset issue, do not know why    
        pte = pte_offset_map(pmd, va);        
        if(pte_present(*pte)) 
        { 
            pa=(pte_val(*pte) & (PAGE_MASK)) | pageOffset; 
            M4UDBG("PA = 0x%8x\n", pa);
            return pa; 
        }
    }
    else /* Only 1 level page table */
    {
       if(pmd_none(*pmd))
       {
          M4UDBG("Error: m4u_user_v2p(), virtual addr 0x%x, pmd invalid! \n", va);
          return 0;
       }
       pa=(pte_val(*pmd) & (PMD_MASK)) | pmdOffset; 
       M4UDBG("PA = 0x%8x\n", pa);
       return pa;    
    }
    
    M4UDBG("warning: m4u_user_v2p(), pte invalid! \n");
    // m4u_dump_maps(va);
    
    return 0;
}

#else
unsigned int m4u_user_v2p(unsigned int va)
{
    unsigned int pageOffset = (va & (PAGE_SIZE - 1));
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    unsigned int pa;
    //M4UMSG("Enter m4u_user_v2p()! \n", va);

    if(NULL==current)
    {
    	  M4UMSG("warning: m4u_user_v2p, current is NULL! \n");
    	  return 0;
    }
    if(NULL==current->mm)
    {
    	  M4UMSG("warning: m4u_user_v2p, current->mm is NULL! tgid=0x%x, name=%s \n", current->tgid, current->comm);
    	  return 0;
    }
        
    pgd = pgd_offset(current->mm, va); /* what is tsk->mm */
    if(pgd_none(*pgd)||pgd_bad(*pgd))
    {
        M4UMSG("m4u_user_v2p(), va=0x%x, pgd invalid! \n", va);
        return 0;
    }

    pud = pud_offset(pgd, va);
    if(pud_none(*pud)||pud_bad(*pud))
    {
        M4UDBG("m4u_user_v2p(), va=0x%x, pud invalid! \n", va);
        return 0;
    }
    
    pmd = pmd_offset(pud, va);
    if(pmd_none(*pmd)||pmd_bad(*pmd))
    {
        M4UDBG("m4u_user_v2p(), va=0x%x, pmd invalid! \n", va);
        return 0;
    }
        
    pte = pte_offset_map(pmd, va);
    if(pte_present(*pte)) 
    { 
        pa=(pte_val(*pte) & (PAGE_MASK)) | pageOffset; 
        return pa; 
    }     

    M4UDBG("m4u_user_v2p(), va=0x%x, pte invalid! \n", va);
    // m4u_dump_maps(va);
    
    return 0;
}
#endif
EXPORT_SYMBOL(m4u_user_v2p);

/*
 * This function we refer to mm/memory.c:__get_user_pages
 * Two things are skipped:
 *	cond_resched(): if the function is efficient enought then we don't need it
 *      flush_{dcache/anon}_page(): the cache will be properly handled by m4u_cache_maint()
 */
#define PFNMAP_FLAG_SET 0x00555555  
int __m4u_get_user_pages(int eModuleID, struct task_struct *tsk, struct mm_struct *mm, 
                     unsigned long start, int nr_pages, unsigned int gup_flags,
                     struct page **pages, struct vm_area_struct **vmas)
{
        int i;
        unsigned long vm_flags;

        if (nr_pages <= 0)
                return 0;

        //VM_BUG_ON(!!pages != !!(gup_flags & FOLL_GET));
        if(!!pages != !!(gup_flags & FOLL_GET)) {
            M4UMSG(" error: __m4u_get_user_pages !!pages != !!(gup_flags & FOLL_GET), pages=0x%x, gup_flags & FOLL_GET=0x%x \n",
                    (unsigned int)pages, gup_flags & FOLL_GET);
        }

        /*   
         * Require read or write permissions.
         * If FOLL_FORCE is set, we only require the "MAY" flags.
         */
        vm_flags  = (gup_flags & FOLL_WRITE) ?
                        (VM_WRITE | VM_MAYWRITE) : (VM_READ | VM_MAYREAD);
        vm_flags &= (gup_flags & FOLL_FORCE) ?
                        (VM_MAYREAD | VM_MAYWRITE) : (VM_READ | VM_WRITE);
        i = 0; 

        M4UDBG("Trying to get_user_pages from start vaddr 0x%08x with %d pages\n", start, nr_pages);

        do { 
                struct vm_area_struct *vma;
                M4UDBG("For a new vma area from 0x%08x\n", start);
                vma = find_extend_vma(mm, start);

                if (!vma)
                {
                    M4UMSG("error: the vma is not found, start=0x%x, module=%d \n", 
                           (unsigned int)start, eModuleID);
                    return i ? : -EFAULT;
                } 
                if( ((~vma->vm_flags) & (VM_IO|VM_RESERVED|VM_PFNMAP|VM_SHARED|VM_WRITE)) == 0 )
                {
                    M4UMSG("error: m4u_get_pages(): bypass pmem garbage pages! vma->vm_flags=0x%x, start=0x%x, module=%d \n", 
                            (unsigned int)(vma->vm_flags), (unsigned int)start, eModuleID);
                	return i ? i : -EFAULT;;
                }                     
                if(vma->vm_flags & VM_IO)
                {
                	  M4UDBG("warning: vma is marked as VM_IO \n");
                }
                if(vma->vm_flags & VM_PFNMAP)
                {
                    M4UMSG("error: vma permission is not correct, vma->vm_flags=0x%x, start=0x%x, module=%d \n", 
                            (unsigned int)(vma->vm_flags), (unsigned int)start, eModuleID);
                    M4UMSG("hint: maybe the memory is remapped with un-permitted vma->vm_flags! \n");          
                    //m4u_dump_maps(start);
                    return i ? i : -EFAULT;;
                }
                if(!(vm_flags & vma->vm_flags)) 
                {
                    M4UMSG("error: vm_flags invalid, vm_flags=0x%x, vma->vm_flags=0x%x, start=0x%x, module=%d \n", 
                           (unsigned int)vm_flags,
                           (unsigned int)(vma->vm_flags), 
                           (unsigned int)start,
                            eModuleID);
                    //m4u_dump_maps(start);                  
                    return i ? : -EFAULT;
                }

                do {
                        struct page *page;
                        unsigned int foll_flags = gup_flags;
                        /*
                         * If we have a pending SIGKILL, don't keep faulting
                         * pages and potentially allocating memory.
                         */
                        if (unlikely(fatal_signal_pending(current)))
                                return i ? i : -ERESTARTSYS;
                        MMProfileLogEx(M4U_MMP_Events[PROFILE_FOLLOW_PAGE], MMProfileFlagStart, eModuleID, start&(~0xFFF));
                        page = follow_page(vma, start, foll_flags);
                        MMProfileLogEx(M4U_MMP_Events[PROFILE_FOLLOW_PAGE], MMProfileFlagEnd, eModuleID, 0x1000);
                        while (!page) {
                                int ret;

                                M4UDBG("Trying to allocate for %dth page(vaddr: 0x%08x)\n", i, start);
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_FORCE_PAGING], MMProfileFlagStart, eModuleID, start&(~0xFFF));
                                ret = handle_mm_fault(mm, vma, start,
                                        (foll_flags & FOLL_WRITE) ?
                                        FAULT_FLAG_WRITE : 0);
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_FORCE_PAGING], MMProfileFlagEnd, eModuleID, 0x1000);
                                if (ret & VM_FAULT_ERROR) {
                                        if (ret & VM_FAULT_OOM) {
                                                M4UMSG("handle_mm_fault() error: no memory, aaddr:0x%08lx (%d pages are allocated), module=%d\n", 
                                                start, i, eModuleID);
                                                //m4u_dump_maps(start);
                                                return i ? i : -ENOMEM;
					                    }
                                        if (ret &
                                            (VM_FAULT_HWPOISON|VM_FAULT_SIGBUS)) {
                                                M4UMSG("handle_mm_fault() error: invalide memory address, vaddr:0x%lx (%d pages are allocated), module=%d\n", 
                                                start, i, eModuleID);
                                                //m4u_dump_maps(start);
                                                return i ? i : -EFAULT;
					                    }
                                        BUG();
                                }
                                if (ret & VM_FAULT_MAJOR)
                                        tsk->maj_flt++;
                                else
                                        tsk->min_flt++;

                                /*
                                 * The VM_FAULT_WRITE bit tells us that
                                 * do_wp_page has broken COW when necessary,
                                 * even if maybe_mkwrite decided not to set
                                 * pte_write. We can thus safely do subsequent
                                 * page lookups as if they were reads. But only
                                 * do so when looping for pte_write is futile:
                                 * in some cases userspace may also be wanting
                                 * to write to the gotten user page, which a
                                 * read fault here might prevent (a readonly
                                 * page might get reCOWed by userspace write).
                                 */
                                if ((ret & VM_FAULT_WRITE) &&
                                    !(vma->vm_flags & VM_WRITE))
                                        foll_flags &= ~FOLL_WRITE;
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_FOLLOW_PAGE], MMProfileFlagStart, eModuleID, start&(~0xFFF));
                                page = follow_page(vma, start, foll_flags);
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_FOLLOW_PAGE], MMProfileFlagEnd, eModuleID, 0x1000);
                        }
                        if (IS_ERR(page)) {
                                M4UMSG("handle_mm_fault() error: faulty page is returned, vaddr:0x%lx (%d pages are allocated), module=%d \n", 
                                        start, i, eModuleID);
                                //m4u_dump_maps(start);
                                return i ? i : PTR_ERR(page);
			            }
                        if (pages) {
                                pages[i] = page;
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_MLOCK], MMProfileFlagStart, eModuleID, start&(~0xFFF));
                                if (trylock_page(page)) {
                                    mlock_vma_page(page);
                                    unlock_page(page);
                                }
                                if(PageMlocked(page)==0)
                                {
                                    M4UMSG("Can't mlock page\n");
                                    dump_page(page);
                                }
                                else
                                {
                                    unsigned int pfn = page_to_pfn(page);
                                    if(pfn < mlock_cnt_size)
                                    {
                                        pMlock_cnt[page_to_pfn(page)]++;
                                    }
                                    else
                                    {
                                        M4UERR("mlock_cnt_size is too small: pfn=%d, size=%d\n", pfn, mlock_cnt_size);
                                    }
                                    
                                    //M4UMSG("lock page:\n");
                                    //dump_page(page);
                                }
                                MMProfileLogEx(M4U_MMP_Events[PROFILE_MLOCK], MMProfileFlagEnd, eModuleID, 0x1000);

                        }
                        if (vmas)
                                vmas[i] = vma;
                        i++;
                        start += PAGE_SIZE;
                        nr_pages--;
                } while (nr_pages && start < vma->vm_end);
        } while (nr_pages);

        return i;
}

// refer to mm/memory.c:get_user_pages()
int m4u_get_user_pages(int eModuleID, struct task_struct *tsk, struct mm_struct *mm,
                unsigned long start, int nr_pages, int write, int force,
                struct page **pages, struct vm_area_struct **vmas)
{
        int flags = FOLL_TOUCH;

        if (pages)
                flags |= FOLL_GET;
        if (write)
                flags |= FOLL_WRITE;
        if (force)
                flags |= FOLL_FORCE;

        return __m4u_get_user_pages(eModuleID, tsk, mm, start, nr_pages, flags, pages, vmas);
}
EXPORT_SYMBOL(m4u_get_user_pages);
