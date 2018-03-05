#ifndef _MI_SYS_BUFMGR_H_
#define _MI_SYS_BUFMGR_H_

#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include "mi_sys_buf_mgr.h"
#include "mi_common.h"
#include "mi_sys_datatype.h"
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/mmzone.h>
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_sys_internal.h"
#include <linux/scatterlist.h>

#include <linux/version.h>
#include <linux/kernel.h>
#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
#include <mstar/mstar_chip.h>
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
#include <mstar_chip.h>
#else
#error not support this kernel version
#endif


#define __MI_SYS_ALLOCATOR_MAGIC_NUM__ 0x4D414C43
#define __MI_SYS_COLLECTION_MAGIC_NUM__ 0x4D434F4C
#define __MI_SYS_BUF_REF_MAGIC_NUM__ 0x42554652

#define MI_SYS_META_DATA_SIZE_MAX 256

static inline int _mi_sys_get_fmt_alignment(MI_SYS_PixelFormat_e ePixelFormat)
{
    switch(ePixelFormat)
    {
        case E_MI_SYS_PIXEL_FRAME_YUV_MST_420:
        case E_MI_SYS_PIXEL_FRAME_YUV422_YUYV:
            return 96;
        default:
            return 32;
  }
}

#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))

struct MI_SYS_BufRef_s;

typedef void (*OnBufRefRelFunc)(struct MI_SYS_BufRef_s *pstBufRef, void *pCBData);

typedef struct MI_SYS_BufRef_s
{
    unsigned int u32MagicNumber;
    struct list_head list;
    struct MI_SYS_BufferAllocation_s *pstBufAllocation;
    OnBufRefRelFunc onRelCB;
    void *pCBData;
    MI_SYS_BufInfo_t bufinfo;
    MI_BOOL is_dumped;
    MI_BOOL bPreNotified;
}MI_SYS_BufRef_t;

typedef enum {
    MI_SYS_QUEUE_ID_USR_INJECT,
    MI_SYS_QUEUE_ID_USR_GET_FIFO,
    MI_SYS_QUEUE_ID_BIND_INPUT,
    MI_SYS_QUEUE_ID_DRIVER_BACK_REF,
    MI_SYS_QUEUE_ID_WORKING,
} MI_SYS_QueueId_e;

typedef struct MI_SYS_BufferQueue_s
{
    struct mutex mtx;
    struct list_head list;
    int  queue_buf_count;
    MI_BOOL dump_buffer_wq_cond;//condition of wait queue for dump buffer
    wait_queue_head_t dump_buffer_wq;
    MI_ModuleId_e module_id;
    MI_SYS_QueueId_e  queue_id; /* 0~5 */
    unsigned char  device_id; /* 0~3 */
    unsigned char  channel_id;  /* 0~63 */
    unsigned char  port;  /* 0~15 */
    unsigned char  io;  /* 0~1 */
}MI_SYS_BufferQueue_t;

typedef struct MI_SYS_AllocatorRef_s
{
    struct list_head list;
    struct mi_sys_Allocator_s *pstAllocator;
}MI_SYS_AllocatorRef_t;

typedef struct MI_SYS_Allocator_Collection_s
{
    unsigned int u32MagicNumber;
    struct semaphore semlock;
    struct list_head list;
    int  collection_size;
}MI_SYS_Allocator_Collection_t;

typedef  struct  MI_SYS_FrameSpecial_s
{
    MI_U32 phy_offset[3];
    MI_U32 u32Stride[3];

} MI_SYS_FrameSpecial_t;

typedef void * MI_SYS_USER_MAP_CTX;

typedef struct mi_sys_mmap_data_s
{
    unsigned long pfn;
    unsigned long pages;
    MI_BOOL bCaChe;
}mi_sys_mmap_data_t;

MI_S32 mi_sys_attach_allocator_to_collection(mi_sys_Allocator_t *pstAllocator, MI_SYS_Allocator_Collection_t *pstAllocatorCollection);
MI_S32 mi_sys_detach_allocator_from_collection(mi_sys_Allocator_t *pstAllocator, MI_SYS_Allocator_Collection_t *pstAllocatorCollection);
void mi_sys_init_allocator_collection(MI_SYS_Allocator_Collection_t *pstAllocatorCollection);
void mi_sys_deinit_allocator_collection(MI_SYS_Allocator_Collection_t *pstAllocatorCollection);
MI_SYS_BufferAllocation_t * mi_sys_alloc_from_allocator_collection(MI_SYS_Allocator_Collection_t *pstAllocatorCollection,  MI_SYS_BufConf_t *pstBufConfig);
unsigned long mi_sys_buf_mgr_get_size(MI_SYS_BufConf_t *pstBufConfig, MI_SYS_FrameSpecial_t  *pstFrameDataInfo);

MI_SYS_USER_MAP_CTX mi_sys_buf_get_cur_user_map_ctx(void);
int mi_sys_buf_mgr_user_map(struct sg_table *sg_table,void **pp_user_map_ptr, void *key, int bForceSkipPermitCheck);
int mi_sys_buf_mgr_user_unmap(void *key);
void  mi_sys_buf_mgr_user_unmap_by_process(void);

MI_S32 mi_sys_buf_mgr_flushcache(void *va,unsigned long len);

struct MI_SYS_BufRef_s *mi_sys_create_bufref( MI_SYS_BufferAllocation_t *pstBufAllocation,
                                                    MI_SYS_BufConf_t *pstBufConfig, OnBufRefRelFunc onRelCB, void *pCBData);
struct MI_SYS_BufRef_s *mi_sys_dup_bufref(MI_SYS_BufRef_t *pstBufRef, OnBufRefRelFunc onRelCB, void *pCBData);

void mi_sys_release_bufref(struct MI_SYS_BufRef_s *pstBufRef);

void mi_sys_init_buf_queue(struct MI_SYS_BufferQueue_s *pstQueue, MI_ModuleId_e eModuleId, MI_SYS_QueueId_e eQueueId, unsigned char device_id, unsigned char channel_id, unsigned char port, unsigned char io);
void mi_sys_add_to_queue_head(struct  MI_SYS_BufRef_s *pstBufRef, struct MI_SYS_BufferQueue_s*pstQueue);
void mi_sys_add_to_queue_tail( struct MI_SYS_BufRef_s *pstBufRef, struct MI_SYS_BufferQueue_s*pstQueue);
struct MI_SYS_BufRef_s* mi_sys_remove_from_queue_head(struct MI_SYS_BufferQueue_s*pstQueue);
struct MI_SYS_BufRef_s* mi_sys_remove_from_queue_tail(struct MI_SYS_BufferQueue_s*pstQueue);
void mi_sys_remove_from_queue(MI_SYS_BufRef_t *pstBufRef, MI_SYS_BufferQueue_t*pstQueue);
#ifdef MI_SYS_PROC_FS_DEBUG
MI_U64 mi_sys_calc_buf_size_in_queue(MI_SYS_BufferQueue_t*pstQueue);
#endif
MI_S32 mi_sys_buf_map_kern_va(MI_SYS_BufferAllocation_t*pstBufAllocation, MI_SYS_BufInfo_t *pstBufInfo);

int mi_sys_buf_mgr_vmap_kern(struct sg_table *sg_table,void **pp_kern_map_ptr);
int mi_sys_buf_mgr_vunmap_kern(void *va_in_kern);
int mi_sys_check_page_range_mapable(unsigned long pfn, long count);
mi_sys_Allocator_t *mi_sys_get_global_meta_allocator(void);
void mi_sys_buf_mgr_fill_bufinfo(MI_SYS_BufInfo_t *pstBufInfo,
                              MI_SYS_BufConf_t *pstBufConfig, MI_SYS_FrameSpecial_t *pstFrameDataInfo, MI_PHY  phyaddr);

MI_U32 _mi_sys_calc_buf_size_from_frame_bufinfo(unsigned long alignment,MI_SYS_FrameData_t stFrameData);
#endif
