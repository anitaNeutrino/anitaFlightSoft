#ifndef _PLX_SYSDEP_H_
#define _PLX_SYSDEP_H_

/******************************************************************************
 *
 * File Name:
 *
 *      Plx_sysdep.h
 *
 * Description:
 *
 *      This file is a subset of the "sysdep.h" taken from the samples
 *      provided with the book "Linux Device Drivers", by Alessandro Rubini
 *      and Jonathan Corbet.  As-is, the original file does not work as
 *      desired with PLX drivers.  Only certain portions of the file are
 *      needed.  This file is re-distributed by permission of the authors
 *      as governed by their license, which is provided below.  The PLX
 *      modified file is, therefore, goverened by the same license.
 *
 * Revision History:
 *
 *      05-31-03 : PCI SDK v4.10
 *
 *****************************************************************************/

/*
 * sysdep.h -- centralizing compatibility issues between 2.0, 2.2, 2.4
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 * $Id: Plx_sysdep.h,v 1.1 2005/06/15 12:01:51 rjn Exp $
 */


#ifndef LINUX_VERSION_CODE
    #include <linux/version.h>
#endif

#ifndef KERNEL_VERSION /* pre-2.1.90 didn't have it */
#  define KERNEL_VERSION(vers,rel,seq) ( ((vers)<<16) | ((rel)<<8) | (seq) )
#endif

// Only allow 2.2.y and 2.4.z

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,2,0)
    #error "Linux kernel versions before v2.2 not supported"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    #error "Linux kernel versions greater than v2.4 not supported"
#endif

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)))
    #error "Linux kernel v2.3 not supported"
#endif

// remember about the current version
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
    #define LINUX_22
#else
    #define LINUX_24
#endif


/******************************************
 *          Items added by PLX
 *****************************************/

#ifdef LINUX_22
    // Include kcomp.h for 2.2, which already contains some compatibility definitions
    #include <linux/kcomp.h>

    // Include iobuf.h for 2.2 to define struct kiobuf
    #include <linux/iobuf.h>

    // Include pci.h for 2.2 to define struct pci_dev
    #include <linux/pci.h>
#endif


// struct tq_struct next field modified in 2.4
#ifdef LINUX_22
    #define Plx_tq_struct_reset_list(tq)       (tq).next = NULL
#elif defined(LINUX_24)
    #define Plx_tq_struct_reset_list(tq)
#endif


// Locking of pages is automatic when a buffer is mapped to an I/O vector in 2.2
#ifdef LINUX_22
    static inline int lock_kiovec(int nr, struct kiobuf *iovec[], int wait)
    {
        return 0;
    }

    static inline int unlock_kiovec(int nr, struct kiobuf *iovec[])
    {
        return 0;
    }
#endif


// kmap & kunmap do not exist in 2.2
#ifdef LINUX_22
    #define Plx_kmap(arg)            (void *)__Plx_kmap(arg)
    #define Plx_kunmap(arg)

    // The following is the code for page_address() taken directly from <linux/pagemap.h>
    static inline unsigned long __Plx_kmap(struct page * page)
    {
        return PAGE_OFFSET + PAGE_SIZE * (page - mem_map);
    }

#else
    #define Plx_kmap                 kmap
    #define Plx_kunmap               kunmap
#endif


// page_to_bus & pci_map_page do not exist in 2.2
#ifdef LINUX_22

    #define PCI_DMA_BIDIRECTIONAL   0
    #define PCI_DMA_TODEVICE        1
    #define PCI_DMA_FROMDEVICE      2
    #define PCI_DMA_NONE            3

    #ifdef __i386__
        #define Plx_page_to_bus(page)      ((page - mem_map) << PAGE_SHIFT)

        // The following code is taken from <asm-i386/pci.h>
        static inline u32 Plx_pci_map_page(struct pci_dev *hwdev, struct page *page,
                                           unsigned long offset, size_t size, int direction)
        {
            return ((page - mem_map) * PAGE_SIZE) + offset;
        }

        static inline void Plx_pci_unmap_page(struct pci_dev *hwdev, u32 dma_address,
                                              size_t size, int direction)
        {
            /* Nothing to do */
        }
    #endif
#else
    #define Plx_page_to_bus          page_to_bus
    #define Plx_pci_map_page         pci_map_page
    #define Plx_pci_unmap_page       pci_unmap_page
#endif


/**********************************************************
 * The following implements an interruptible wait_event
 * with a timeout.  This is used instead of the function
 * interruptible_sleep_on_timeout() since this is susceptible
 * to race conditions.
 *
 *    retval == 0; condition met; we're good.
 *    retval <  0; interrupted by signal.
 *    retval >  0; timed out.
 *********************************************************/
#define Plx_wait_event_interruptible_timeout(wq, condition, timeout) \
({                                                   \
    int __ret = 0;                                   \
                                                     \
    if (!(condition))	                             \
    {                                                \
        __Plx__wait_event_interruptible_timeout(     \
            wq,                                      \
            condition,	                             \
            timeout,                                 \
            __ret                                    \
            );	                                     \
    }                                                \
    __ret;                                           \
})


#define __Plx__wait_event_interruptible_timeout(wq, condition, timeout, ret) \
do                                                   \
{                                                    \
    if (!(condition))                                \
    {                                                \
        wait_queue_t __wait;                         \
        unsigned long expire;                        \
                                                     \
        init_waitqueue_entry(&__wait, current);      \
                                                     \
        expire = timeout + jiffies;                  \
        add_wait_queue(&wq, &__wait);                \
        for (;;)                                     \
        {                                            \
            set_current_state(TASK_INTERRUPTIBLE);   \
            if (condition)                           \
                break;                               \
            if (jiffies > expire)                    \
            {                                        \
                ret = jiffies - expire;              \
                break;                               \
            }                                        \
            if (!signal_pending(current))            \
            {                                        \
                schedule_timeout(timeout);           \
                continue;                            \
            }                                        \
            ret = -ERESTARTSYS;                      \
            break;                                   \
      }                                              \
      current->state = TASK_RUNNING;                 \
      remove_wait_queue(&wq, &__wait);               \
    }                                                \
}                                                    \
while (0)

/******************************************
 *          End items added by PLX
 *****************************************/



#ifndef LINUX_20 /* include vmalloc.h if this is 2.2/2.4 */
#  ifdef VM_READ /* a typical flag defined by mm.h */
#    include <linux/vmalloc.h>
#  endif
#endif

#include <linux/sched.h>

/* Modularization issues */
#ifdef LINUX_20
#  define __USE_OLD_SYMTAB__
#  define EXPORT_NO_SYMBOLS register_symtab(NULL);
#  define REGISTER_SYMTAB(tab) register_symtab(tab)
#else
#  define REGISTER_SYMTAB(tab) /* nothing */
#endif

#ifdef __USE_OLD_SYMTAB__
#  define __MODULE_STRING(s)         /* nothing */
#  define MODULE_PARM(v,t)           /* nothing */
#  define MODULE_PARM_DESC(v,t)      /* nothing */
#  define MODULE_AUTHOR(n)           /* nothing */
#  define MODULE_DESCRIPTION(d)      /* nothing */
#  define MODULE_SUPPORTED_DEVICE(n) /* nothing */
#endif

/*
 * In version 2.2 (up to 2.2.19, at least), the macro for request_module()
 * when no kmod is there is wrong. It's a "do {} while 0" but it shouldbe int
 */
#ifdef LINUX_22
#  ifndef CONFIG_KMOD
#    undef request_module
#    define request_module(name) -ENOSYS
#  endif
#endif


#ifndef LINUX_20
#  include <linux/init.h>     /* module_init/module_exit */
#endif

#ifndef module_init
#  define module_init(x)        int init_module(void) { return x(); }
#  define module_exit(x)        void cleanup_module(void) { x(); }
#endif

#ifndef SET_MODULE_OWNER
#  define SET_MODULE_OWNER(structure) /* nothing */
#endif

/*
 * "select" changed in 2.1.23. The implementation is twin, but this
 * header is new
 *
 */
#ifdef LINUX_20
#  define __USE_OLD_SELECT__
#else
#  include <linux/poll.h>
#endif

#ifdef LINUX_20
#  define INODE_FROM_F(filp) ((filp)->f_inode)
#else
#  define INODE_FROM_F(filp) ((filp)->f_dentry->d_inode)
#endif

/* Other changes in the fops are solved using wrappers */

/*
 * Wait queues changed with 2.3
 */
#ifndef DECLARE_WAIT_QUEUE_HEAD
#  define DECLARE_WAIT_QUEUE_HEAD(head) struct wait_queue *head = NULL
   typedef  struct wait_queue *wait_queue_head_t;
#  define init_waitqueue_head(head) (*(head)) = NULL

/* offer wake_up_sync as an alias for wake_up */
#  define wake_up_sync(head) wake_up(head)
#  define wake_up_interruptible_sync(head) wake_up_interruptible(head)

/* Pretend we have add_wait_queue_exclusive */
#  define add_wait_queue_exclusive(q,entry) add_wait_queue ((q), (entry))

#endif /* no DECLARE_WAIT_QUEUE_HEAD */

/*
 * Define wait_event for 2.0 kernels.  (This ripped off directly from
 * the 2.2.18 sched.h)
 */
#ifdef LINUX_20

#define __wait_event(wq, condition) 					\
do {									\
	struct wait_queue __wait;					\
									\
	__wait.task = current;						\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		current->state = TASK_UNINTERRUPTIBLE;			\
		mb();							\
		if (condition)						\
			break;						\
		schedule();						\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)

#define wait_event(wq, condition) 					\
do {									\
	if (condition)	 						\
		break;							\
	__wait_event(wq, condition);					\
} while (0)

#define __wait_event_interruptible(wq, condition, ret)			\
do {									\
	struct wait_queue __wait;					\
									\
	__wait.task = current;						\
	add_wait_queue(&wq, &__wait);					\
	for (;;) {							\
		current->state = TASK_INTERRUPTIBLE;			\
		mb();							\
		if (condition)						\
			break;						\
		if (!signal_pending(current)) {				\
			schedule();					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	current->state = TASK_RUNNING;					\
	remove_wait_queue(&wq, &__wait);				\
} while (0)
	
#define wait_event_interruptible(wq, condition)				\
({									\
	int __ret = 0;							\
	if (!(condition))						\
		__wait_event_interruptible(wq, condition, __ret);	\
	__ret;								\
})
#endif


/*
 * 2.3 added tasklets
 */
#ifdef LINUX_24
#  define HAVE_TASKLETS
#endif




/* FIXME: implement the other versions of wake_up etc */


/*
 * access to user space: use the 2.2 functions,
 * and implement them as macros for 2.0
 */

#ifdef LINUX_20
#  include <asm/segment.h>
#  define access_ok(t,a,sz)           (verify_area((t),(void *) (a),(sz)) ? 0 : 1)
#  define verify_area_20              verify_area
#  define   copy_to_user(t,f,n)         (memcpy_tofs((t), (f), (n)), 0)
#  define copy_from_user(t,f,n)       (memcpy_fromfs((t), (f), (n)), 0)
#  define   __copy_to_user(t,f,n)       copy_to_user((t), (f), (n))
#  define __copy_from_user(t,f,n)     copy_from_user((t), (f), (n))

#  define PUT_USER(val,add)           (put_user((val),(add)), 0)
#  define __PUT_USER(val,add)         PUT_USER((val),(add))

#  define GET_USER(dest,add)          ((dest)=get_user((add)), 0)
#  define __GET_USER(dest,add)        GET_USER((dest),(add))
#else
#  include <asm/uaccess.h>
#  include <asm/io.h>
#  define verify_area_20(t,a,sz) (0) /* == success */
#  define   PUT_USER   put_user
#  define __PUT_USER __put_user
#  define   GET_USER   get_user
#  define __GET_USER __get_user
#endif

/*
 * Allocation issues
 */
#ifdef GFP_USER /* only if mm.h has been included */
#  ifdef LINUX_20
#    define __GFP_DMA GFP_DMA /* 2.0 didn't have the leading __ */
#  endif
#  ifndef LINUX_24
#    define __GFP_HIGHMEM  0  /* was not there */
#    define GFP_HIGHUSER   0   /* idem */
#  endif

#  ifdef LINUX_20
#    define __get_free_pages(a,b) __get_free_pages((a),(b),0)
#  endif
#  ifndef LINUX_24
#    define get_zeroed_page get_free_page
#  endif
#endif

/* Also, define check_mem_region etc */
#ifndef LINUX_24
#  define check_mem_region(a,b)     0 /* success */
#  define request_mem_region(a,b,c) /* nothing */
#  define release_mem_region(a,b)   /* nothing */
#endif

/* The use_count of exec_domain and binfmt changed in 2.1.23 */

#ifdef LINUX_20
#  define INCRCOUNT(p)  ((p)->module ? __MOD_INC_USE_COUNT((p)->module) : 0)
#  define DECRCOUNT(p)  ((p)->module ? __MOD_DEC_USE_COUNT((p)->module) : 0)
#  define CURRCOUNT(p)  ((p)->module && (p)->module->usecount)
#else
#  define INCRCOUNT(p)  ((p)->use_count++)
#  define DECRCOUNT(p)  ((p)->use_count--)
#  define CURRCOUNT(p)  ((p)->use_count)
#endif



/*
 * 2.2 didn't have create_proc_{read|info}_entry yet.
 * And it looks like there are no other "interesting" entry point, as
 * the rest is somehow esotique (mknod, symlink, ...)
 */
#ifdef LINUX_22
#  ifdef PROC_SUPER_MAGIC  /* Only if procfs is being used */
extern inline struct proc_dir_entry *create_proc_read_entry(const char *name,
                          mode_t mode, struct proc_dir_entry *base, 
                          read_proc_t *read_proc, void * data)
{
    struct proc_dir_entry *res=create_proc_entry(name,mode,base);
    if (res) {
        res->read_proc=read_proc;
        res->data=data;
    }
    return res;
}

#    ifndef create_proc_info_entry /* added in 2.2.18 */
typedef int (get_info_t)(char *, char **, off_t, int, int);
extern inline struct proc_dir_entry *create_proc_info_entry(const char *name,
        mode_t mode, struct proc_dir_entry *base, get_info_t *get_info)
{
        struct proc_dir_entry *res=create_proc_entry(name,mode,base);
        if (res) res->get_info=get_info;
        return res;
}
#    endif  /* no create_proc_info_entry */
#  endif
#endif


/* 2.0 had no read and write memory barriers, and 2.2 lacks the
   set_ functions */
#ifndef LINUX_24
#  ifdef LINUX_20
#    define wmb() mb() /* this is a big penalty on non-reordering platfs */
#    define rmb() mb() /* this is a big penalty on non-reordering platfs */
#  endif /* LINUX_20 */

#define set_mb() do { var = value; mb(); } while (0)
#define set_wmb() do { var = value; wmb(); } while (0)
#endif /* ! LINUX_24 */





/*
 * 2.1 stuffed the "flush" method into the middle of the file_operations
 * structure.  The FOP_NO_FLUSH symbol is for drivers that do not implement
 * flush (most of them), it can be inserted in initializers for all 2.x
 * kernel versions.
 */
#ifdef LINUX_20
#  define FOP_NO_FLUSH   /* nothing */
#  define TAG_LLSEEK    lseek
#  define TAG_POLL      select
#else
#  define FOP_NO_FLUSH  NULL,
#  define TAG_LLSEEK    llseek
#  define TAG_POLL      poll
#endif



/*
 * fasync changed in 2.2.
 */
#ifdef LINUX_20
/*  typedef struct inode *fasync_file; */
#  define fasync_file struct inode *
#else
  typedef int fasync_file;
#endif

/* kill_fasync had less arguments, and a different indirection in the first */
#ifndef LINUX_24
#  define kill_fasync(ptrptr,sig,band)  kill_fasync(*(ptrptr),(sig))
#endif

#ifdef LINUX_PCI_H /* only if PCI stuff is being used */
#  ifdef LINUX_20
#    include "pci-compat.h" /* a whole set of replacement functions */
#  else
#    define  pci_release_device(d) /* placeholder, used in 2.0 to free stuff */
#  endif
#endif



/*
 * Some task state stuff
 */

#ifndef set_current_state
#  define set_current_state(s) current->state = (s);
#endif


/*
 * Schedule_task was a late 2.4 addition.
 */
#ifndef LINUX_24
extern inline int schedule_task(struct tq_struct *task)
{
        queue_task(task, &tq_scheduler);
        return 1;
}
#endif


/*
 * Timing issues
 */

#ifdef _LINUX_DELAY_H /* only if linux/delay.h is included */
#  ifndef mdelay /* linux-2.0 */
#    ifndef MAX_UDELAY_MS
#      define MAX_UDELAY_MS   5
#    endif
#    define mdelay(n) (\
        (__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
        ({unsigned long msec=(n); while (msec--) udelay(1000);}))
#  endif /* mdelay */
#endif /* _LINUX_DELAY_H */


/*
 * No del_timer_sync before 2.4
 */
#ifndef LINUX_24
#  define del_timer_sync(timer) del_timer(timer)  /* and hope */
#endif


/*
 * Various changes in mmap and friends.
 */

#ifndef NOPAGE_SIGBUS
#  define NOPAGE_SIGBUS  NULL  /* return value of the nopage memory method */
#  define NOPAGE_OOM     NULL  /* No real equivalent in older kernels */
#endif

#ifndef VM_RESERVED            /* Added 2.4.0-test10 */
#  define VM_RESERVED 0
#endif

#ifdef LINUX_24 /* use "vm_pgoff" to get an offset */
#define VMA_OFFSET(vma)  ((vma)->vm_pgoff << PAGE_SHIFT)
#else /* use "vm_offset" */
#define VMA_OFFSET(vma)  ((vma)->vm_offset)
#endif

#ifdef MAP_NR
#define virt_to_page(page) (mem_map + MAP_NR(page))
#endif

#ifndef get_page
#  define get_page(p) atomic_inc(&(p)->count)
#endif



/*
 * I/O memory was not managed by ealier kernels, define them as success
 */

#if 0 /* FIXME: what is the right way to do request_mem_region? */
#ifndef LINUX_24
#  define check_mem_region(start, len)          0
#  define request_mem_region(start, len, name)  0
#  define release_mem_region(start, len)        0

   /*
    * Also, request_ and release_ region used to return void. Return 0 instead
    */
#  define request_region(s, l, n)  ({request_region((s),(l),(n));0;})
#  define release_region(s, l)     ({release_region((s),(l));0;})

#endif /* not LINUX_24 */
#endif



/*
 * Memory barrier stuff, define what's missing from older kernel versions
 */
#ifdef switch_to /* this is always a macro, defined in <asm/sysstem.h> */

#  ifndef set_mb
#    define set_mb(var, value) do {(var) = (value); mb();}  while 0
#  endif
#  ifndef set_rmb
#    define set_rmb(var, value) do {(var) = (value); rmb();}  while 0
#  endif
#  ifndef set_wmb
#    define set_wmb(var, value) do {(var) = (value); wmb();}  while 0
#  endif

/* The hw barriers are defined as sw barriers. A correct thing if this
   specific kernel/platform is supported but has no specific instruction */
#  ifndef mb
#    define mb barrier
#  endif
#  ifndef rmb
#    define rmb barrier
#  endif
#  ifndef wmb
#    define wmb barrier
#  endif

#endif /* switch to (i.e. <asm/system.h>) */


#endif /* _SYSDEP_H_ */
