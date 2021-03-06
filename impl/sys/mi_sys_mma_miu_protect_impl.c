#include "mi_sys_mma_miu_protect_impl.h"
#include <linux/version.h>
#include <linux/kernel.h>
#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
#include <../../mstar2/drv/miu/mdrv_miu.h>
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
#include <mdrv_miu.h>
#else
#error not support this kernel version
#endif
#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"

extern unsigned long lx_mem_size;// = INVALID_PHY_ADDR; //default setting
extern unsigned long lx_mem2_size;// = INVALID_PHY_ADDR; //default setting
extern unsigned long lx_mem3_size;// = INVALID_PHY_ADDR; //default setting

extern unsigned long lx_mem_addr;// = PHYS_OFFSET;
extern unsigned long lx_mem2_addr;// = INVALID_PHY_ADDR; //default setting
extern unsigned long lx_mem3_addr;// = INVALID_PHY_ADDR; //default setting

typedef enum
{
    MIU_BLOCK_IDLE = 0,
    MIU_BLOCK_BUSY
}MIU_PROTECT_BLOCK_STATUS;

/* every MIU will have one MIU_ProtectRanges for it */
struct MIU_ProtectRanges
{
    unsigned char miu;                                            // this protect_info is for which MIU
    MIU_PROTECT_BLOCK_STATUS miuBlockStatus[MIU_BLOCK_NUM];        // BLOCK_STATUS: used or available

    unsigned int krange_num;                                    // count of used block
    struct list_head list_head;                                    // list for every protect_info (MIU_ProtectRange)
    struct mutex lock;
};

static struct MIU_ProtectRanges glob_miu_kranges[KERN_CHUNK_NUM]; //record kernel protect ranges on 3 MIUs

/* each protect_info(block) of a MIU */
typedef struct
{
    unsigned char miuBlockIndex;                                // this protect_info is using which block
    unsigned long start_cpu_bus_pa;
    unsigned long length;
    struct list_head list_node;
} MIU_ProtectRange;

#define INVALID_MIU          0xFF
static inline void cpu_bus_addr_to_MiuOffset(unsigned long cpu_bus_addr, unsigned int *miu, unsigned long *offset)
{
    *miu = INVALID_MIU;

    if(cpu_bus_addr >= ARM_MIU2_BUS_BASE)
    {
        *miu = 2;
        *offset = cpu_bus_addr - ARM_MIU2_BUS_BASE;
    }
    else if(cpu_bus_addr >= ARM_MIU1_BUS_BASE)
    {
        *miu = 1;
        *offset = cpu_bus_addr - ARM_MIU1_BUS_BASE;
    }
    else if(cpu_bus_addr >= ARM_MIU0_BUS_BASE)
    {
        *miu = 0;
        *offset = cpu_bus_addr - ARM_MIU0_BUS_BASE;
    }
    else
        printk( "\033[35mFunction = %s, Line = %d, Error, Unknown MIU, for cpu_bus_addr is 0x%lX\033[m\n", __PRETTY_FUNCTION__, __LINE__, cpu_bus_addr);
}

static int g_kprotect_enabled = 1;

static bool _miu_kernel_protect(unsigned char miuBlockIndex, unsigned short *pu8ProtectId,
    unsigned long start, unsigned long end, int flag)
{
    bool ret = true;

    MI_SYS_BUG_ON(!g_kernel_protect_client_id);

    if(g_kprotect_enabled)
        ret = MDrv_MIU_Protect(miuBlockIndex, g_kernel_protect_client_id, start, end, flag);
    else
        printk( "ignore kernel protect\n");

    return ret;
}

/*  this API for the case, in same miu, two adjacent lx exist, they can share same miu protect block
  *              LX_MEM                   LX2_MEM
  *  |-----------------| ------------------|
  */
static int _insertKRange(int miu_index, unsigned long lx_addr, unsigned long lx_length)
{
    MIU_ProtectRange *krange = NULL;

    if(!list_empty(&glob_miu_kranges[miu_index].list_head))
    {
        krange = list_entry(glob_miu_kranges[miu_index].list_head.prev, MIU_ProtectRange, list_node);
        if((krange->start_cpu_bus_pa + krange->length) == lx_addr)
        {
            _miu_kernel_protect(krange->miuBlockIndex, g_kernel_protect_client_id,
                krange->start_cpu_bus_pa, krange->start_cpu_bus_pa+krange->length, MIU_PROTECT_DISABLE);

            krange->length += lx_length;

            _miu_kernel_protect(krange->miuBlockIndex, g_kernel_protect_client_id,
                krange->start_cpu_bus_pa, krange->start_cpu_bus_pa+krange->length, MIU_PROTECT_ENABLE);
            return 0;
        }
    }
    return -1;
}

static int idleBlockIndx(struct MIU_ProtectRanges * pranges)
{
    int index = 0;

    for(index = 0; index < MIU_BLOCK_NUM; ++index)
    {
        if(pranges->miuBlockStatus[index] == MIU_BLOCK_IDLE)
        {
            return index;
        }
    }

    return -1;
}

/* when alloc from cma heap, call this API to deleteKRange of this allocted buffer */
int deleteKRange(unsigned long long start_cpu_bus_pa, unsigned long length)
{
    struct MIU_ProtectRanges  *pranges ;
    MIU_ProtectRange * range,  * r_front = NULL, * r_back= NULL;
    MIU_ProtectRange old;
    unsigned long r_front_len = 0, r_back_len = 0;
    int miuBlockIndex = -1;
    bool find = false, protect_ret = false;
    int ret = MI_SUCCESS;

    unsigned long offset = 0;
    int miu_index = 0;

    if(length == 0)
        return MI_SUCCESS;

    cpu_bus_addr_to_MiuOffset(start_cpu_bus_pa, &miu_index, &offset);
    pranges = &glob_miu_kranges[miu_index];

    /*
         * kernel protect range( before allocate buffer)
         *
         * |--------------------------------|
         *
         * kernel protect range(buffer location in this range, after buffer allocated)
         *  r_front        allocated buffer    r_back
         *
         * |------|=============|-------|
         *
         * case: r_front = 0; r_back = 0; r_front=r_back=0;
         */
    mutex_lock(&pranges->lock);
    list_for_each_entry(range, &pranges->list_head, list_node)
    {
        if((start_cpu_bus_pa >= range->start_cpu_bus_pa)
            && ((start_cpu_bus_pa+length) <= (range->start_cpu_bus_pa+range->length)))
        {
            find = true;
            old.start_cpu_bus_pa = range->start_cpu_bus_pa;
            old.length = range->length;
            old.miuBlockIndex = range->miuBlockIndex;
            break;
        }
    }

    if(!find)
    {
        ret = MI_ERR_SYS_UNEXIST;
        printk("not find the buffer: start_cpu_bus_pa %llx length %lu\n", start_cpu_bus_pa, length);
        goto DELETE_KRANGE_DONE;
    }

    r_front_len = start_cpu_bus_pa - range->start_cpu_bus_pa;
    r_back_len = range->start_cpu_bus_pa + range->length - (start_cpu_bus_pa + length);

    if((r_front_len != 0) && (r_back_len != 0))
    {
        miuBlockIndex = idleBlockIndx(pranges);
        if(miuBlockIndex < 0)
        {
            ret = MI_ERR_SYS_UNEXIST;
            printk("no idle miu protect block in miu %d\n", (int)miu_index);
            goto DELETE_KRANGE_DONE;
        }

        r_back = (MIU_ProtectRange *)kzalloc(sizeof(MIU_ProtectRange), GFP_KERNEL);
        if(!r_back)
        {
            ret = MI_ERR_SYS_NOBUF;
            printk( "no memory\n");
            goto DELETE_KRANGE_DONE;
        }

        r_front = range;
        r_front->length = r_front_len;

        r_back->start_cpu_bus_pa = start_cpu_bus_pa + length;
        r_back->length = r_back_len;
        r_back->miuBlockIndex = miuBlockIndex;
        INIT_LIST_HEAD(&r_back->list_node);
        list_add(&r_back->list_node, &r_front->list_node);
        pranges->krange_num++;
    }
    else if(r_front_len != 0) //and (r_back_len == 0)
    {
        r_front = range;
        r_front->length = r_front_len;
    }
    else if(r_back_len != 0) //and (r_front_len == 0)
    {
        r_back = range;
        r_back->start_cpu_bus_pa = start_cpu_bus_pa + length;
        r_back->length = r_back_len;
    }
    else //((r_front_len == 0) && (r_back_len == 0))
    {
        list_del(&range->list_node);
        kfree(range);
        pranges->krange_num--;
    }

    protect_ret = _miu_kernel_protect(old.miuBlockIndex, g_kernel_protect_client_id, old.start_cpu_bus_pa,
        old.start_cpu_bus_pa + old.length, MIU_PROTECT_DISABLE);
    MI_SYS_BUG_ON(!protect_ret);
    pranges->miuBlockStatus[old.miuBlockIndex] = MIU_BLOCK_IDLE;

    if(r_front)
    {
        protect_ret = _miu_kernel_protect(r_front->miuBlockIndex, g_kernel_protect_client_id,
            r_front->start_cpu_bus_pa, r_front->start_cpu_bus_pa+r_front->length, MIU_PROTECT_ENABLE);
        MI_SYS_BUG_ON(!protect_ret);
        pranges->miuBlockStatus[r_front->miuBlockIndex] = MIU_BLOCK_BUSY;
    }

    if(r_back)
    {
        protect_ret = _miu_kernel_protect(r_back->miuBlockIndex, g_kernel_protect_client_id,
            r_back->start_cpu_bus_pa, r_back->start_cpu_bus_pa+r_back->length, MIU_PROTECT_ENABLE);
        MI_SYS_BUG_ON(!protect_ret);
        pranges->miuBlockStatus[r_back->miuBlockIndex] = MIU_BLOCK_BUSY;
    }

DELETE_KRANGE_DONE:
    mutex_unlock(&pranges->lock);
    return ret;
}
/* when free to cma heap, call this API to add KRange of this allocted buffer */
int addKRange(unsigned long long start_cpu_bus_pa, unsigned long length)
{
    struct MIU_ProtectRanges *pranges ;
    MIU_ProtectRange *r_prev = NULL, *r_next= NULL;
    MIU_ProtectRange *range;
    int miuBlockIndex = -1;
    bool protect_ret = false;
    int ret = MI_SUCCESS;
    unsigned long offset = 0;
    int miu_index = 0;


    if(length == 0)
        return MI_SUCCESS;

    cpu_bus_addr_to_MiuOffset(start_cpu_bus_pa, &miu_index, &offset);
    pranges = &glob_miu_kranges[miu_index];

    /*
         * kernel protect range (before freed buffer)
         *      r_prev       allocated buffer     r_next
         * |-------------|====================|------------|
         *
         * kernel protect range(freed buffer location in this range)
         *   r_prev   freed buffer    r_next
         * |--------|?-------------?|-------|
         *
    */
    mutex_lock(&pranges->lock);
    list_for_each_entry(range, &pranges->list_head, list_node)    // find this miu all kernel_protect setting(range)
    {
        if((range->start_cpu_bus_pa + range->length) <= start_cpu_bus_pa)
        {
            //printk("\033[35mFunction = %s, Line = %d, find r_prev form 0x%lX to 0x%lX\033[m\n", __PRETTY_FUNCTION__, __LINE__, range->start_cpu_bus_pa, (range->start_cpu_bus_pa + range->length));
            r_prev = range;
            continue;    // should be continue, we are going to find a nearest one k_range before this buffer
        }
    }

    if(r_prev)    // find a kernel_protect range before this buffer
    {
        if(!list_is_last(&r_prev->list_node,&pranges->list_head))
        {
            r_next = container_of(r_prev->list_node.next, MIU_ProtectRange, list_node);        // if prev_krange is not the last one, the next one krange will be r_next
            //printk("\033[35mFunction = %s, Line = %d, find r_next form 0x%lX to 0x%lX\033[m\n", __PRETTY_FUNCTION__, __LINE__, r_next->start_cpu_bus_pa, (r_next->start_cpu_bus_pa + r_next->length));
        }
    }
    else        // no kernel_protect range before this buffer ==> all k_range is behind this buffer
    {
        if(list_empty(&pranges->list_head))
            r_next = NULL;
        else
            r_next = list_first_entry(&pranges->list_head, MIU_ProtectRange, list_node);    // r_next will be first krange
    }

    //till now, find the prev range and next range of buffer freed
    if(r_prev && r_next)
    {
        if(((r_prev->start_cpu_bus_pa + r_prev->length) == start_cpu_bus_pa)
            && ((start_cpu_bus_pa + length) == r_next->start_cpu_bus_pa))    // the buffer is just the hole between r_prev and r_next
        {
            // disable r_prev
            protect_ret = _miu_kernel_protect(r_prev->miuBlockIndex, g_kernel_protect_client_id,
                r_prev->start_cpu_bus_pa, r_prev->start_cpu_bus_pa + r_prev->length, MIU_PROTECT_DISABLE);
            MI_SYS_BUG_ON(!protect_ret);

            // disable r_next
            protect_ret = _miu_kernel_protect(r_next->miuBlockIndex, g_kernel_protect_client_id,
                r_next->start_cpu_bus_pa, r_next->start_cpu_bus_pa + r_next->length, MIU_PROTECT_DISABLE);
            MI_SYS_BUG_ON(!protect_ret);
            pranges->miuBlockStatus[r_next->miuBlockIndex] = MIU_BLOCK_IDLE;    // mark a k_range is available

            r_prev->length += (r_next->length + length);                // extend the r_prev length, and protect it
            protect_ret = _miu_kernel_protect(r_prev->miuBlockIndex, g_kernel_protect_client_id,
                r_prev->start_cpu_bus_pa, r_prev->start_cpu_bus_pa + r_prev->length, MIU_PROTECT_ENABLE);
            MI_SYS_BUG_ON(!protect_ret);

            list_del(&r_next->list_node);
            kfree(r_next);
            pranges->krange_num--;

            goto ADD_KRANGE_DONE;
        }
    }

    if(r_prev)
    {
        if((r_prev->start_cpu_bus_pa + r_prev->length) == start_cpu_bus_pa)
        {
            protect_ret = _miu_kernel_protect(r_prev->miuBlockIndex, g_kernel_protect_client_id,
                r_prev->start_cpu_bus_pa, r_prev->start_cpu_bus_pa + r_prev->length, MIU_PROTECT_DISABLE);
            MI_SYS_BUG_ON(!protect_ret);

            r_prev->length += length;
            protect_ret = _miu_kernel_protect(r_prev->miuBlockIndex, g_kernel_protect_client_id,
                r_prev->start_cpu_bus_pa, r_prev->start_cpu_bus_pa + r_prev->length, MIU_PROTECT_ENABLE);
            MI_SYS_BUG_ON(!protect_ret);

            goto ADD_KRANGE_DONE;
        }
    }

    if(r_next)
    {
        if((start_cpu_bus_pa + length) == r_next->start_cpu_bus_pa)
        {
            protect_ret = _miu_kernel_protect(r_next->miuBlockIndex, g_kernel_protect_client_id,
                r_next->start_cpu_bus_pa, r_next->start_cpu_bus_pa + r_next->length, MIU_PROTECT_DISABLE);
            MI_SYS_BUG_ON(!protect_ret);

            r_next->start_cpu_bus_pa= start_cpu_bus_pa;
            r_next->length += length;
            protect_ret = _miu_kernel_protect(r_next->miuBlockIndex, g_kernel_protect_client_id,
                r_next->start_cpu_bus_pa, r_next->start_cpu_bus_pa + r_next->length, MIU_PROTECT_ENABLE);
            MI_SYS_BUG_ON(!protect_ret);

            goto ADD_KRANGE_DONE;
        }
    }

    // use a new k_range for this buffer
    miuBlockIndex = idleBlockIndx(pranges);
    if(miuBlockIndex < 0)
    {
        ret = MI_ERR_SYS_UNEXIST;
        printk("no idle miu protect block in miu %d\n", (int)miu_index);
        goto ADD_KRANGE_DONE;
    }
    //printk( "\033[35mFunction = %s, Line = %d, use a new k_range for this buffer, miu_protect %d for 0x%lX to 0x%lX\033[m\n", __PRETTY_FUNCTION__, __LINE__, miuBlockIndex, start_cpu_bus_pa, (start_cpu_bus_pa+length));

    range = (MIU_ProtectRange *)kzalloc(sizeof(MIU_ProtectRange), GFP_KERNEL);
    if(!range)
    {
        ret = MI_ERR_SYS_NOBUF;
        printk( "no memory\n");
        goto ADD_KRANGE_DONE;
    }
    range->start_cpu_bus_pa = start_cpu_bus_pa;
    range->length = length;
    range->miuBlockIndex = miuBlockIndex;
    INIT_LIST_HEAD(&range->list_node);
    if(r_prev)
        list_add(&range->list_node, &r_prev->list_node);
    else
        list_add(&range->list_node, &pranges->list_head);

    protect_ret = _miu_kernel_protect(range->miuBlockIndex, g_kernel_protect_client_id,
        range->start_cpu_bus_pa, range->start_cpu_bus_pa + range->length, MIU_PROTECT_ENABLE);
    MI_SYS_BUG_ON(!protect_ret);
    pranges->miuBlockStatus[range->miuBlockIndex] = MIU_BLOCK_BUSY;
    pranges->krange_num++;

ADD_KRANGE_DONE:
    mutex_unlock(&pranges->lock);
    return ret;
}

#include <linux/version.h>
#include <linux/kernel.h>
#if LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
#define INVALID_PHY_ADDR 0xffffffffUL
#endif

void init_glob_miu_kranges(void)
{
    unsigned long offset = 0;
    MIU_ProtectRange *krange = NULL;
    int i = 0, miu_index = 0;

    for(i = 0; i < KERN_CHUNK_NUM; ++i)
    {
        glob_miu_kranges[i].miu = i;
        memset(glob_miu_kranges[i].miuBlockStatus, 0, sizeof(unsigned char)*MIU_BLOCK_NUM);
        glob_miu_kranges[i].krange_num = 0;
        mutex_init(&glob_miu_kranges[i].lock);
        INIT_LIST_HEAD(&glob_miu_kranges[i].list_head);
    }

    if(lx_mem_size != INVALID_PHY_ADDR)
    {
        cpu_bus_addr_to_MiuOffset(lx_mem_addr, &miu_index, &offset);

        printk("\033[35mFunction = %s, Line = %d, Insert KProtect for LX @ MIU: %d\033[m\n", __PRETTY_FUNCTION__, __LINE__, miu_index);
        krange = (MIU_ProtectRange *)kzalloc(sizeof(MIU_ProtectRange), GFP_KERNEL);
        MI_SYS_BUG_ON(!krange);
        INIT_LIST_HEAD(&krange->list_node);
        krange->start_cpu_bus_pa = lx_mem_addr;
        krange->length = lx_mem_size;
        krange->miuBlockIndex = glob_miu_kranges[miu_index].krange_num;    // use miu_index's kernel protect
        // kernel protect block index start with 0

        printk("\033[35mFunction = %s, Line = %d, [INIT] for LX0 kprotect: from 0x%lX to 0x%lX, using block %u\033[m\n", __PRETTY_FUNCTION__, __LINE__, krange->start_cpu_bus_pa, krange->start_cpu_bus_pa + krange->length, krange->miuBlockIndex);
        _miu_kernel_protect(krange->miuBlockIndex, g_kernel_protect_client_id,
            krange->start_cpu_bus_pa, krange->start_cpu_bus_pa + krange->length, MIU_PROTECT_ENABLE);
        glob_miu_kranges[miu_index].miuBlockStatus[krange->miuBlockIndex] = MIU_BLOCK_BUSY;

        glob_miu_kranges[miu_index].krange_num++;                        // next miu_index's kernel protect id
        list_add_tail(&krange->list_node, &glob_miu_kranges[miu_index].list_head);
    }

    if(lx_mem2_size != INVALID_PHY_ADDR)
    {
        cpu_bus_addr_to_MiuOffset(lx_mem2_addr, &miu_index, &offset);

        printk("\033[35mFunction = %s, Line = %d, Insert KProtect for LX2 @ MIU: %d\033[m\n", __PRETTY_FUNCTION__, __LINE__, miu_index);
        if(_insertKRange(miu_index, lx_mem2_addr, lx_mem2_size))    // we first check if LX2 can be combined to an existed protect_block(krange), if not, we add a new protect_block(krange)
        {
            krange = (MIU_ProtectRange *)kzalloc(sizeof(MIU_ProtectRange), GFP_KERNEL);
            MI_SYS_BUG_ON(!krange);
            INIT_LIST_HEAD(&krange->list_node);
            krange->start_cpu_bus_pa = lx_mem2_addr;
            krange->length = lx_mem2_size;
            krange->miuBlockIndex = glob_miu_kranges[miu_index].krange_num;
            //kernel protect block index start with 0

            printk("\033[35mFunction = %s, Line = %d, [INIT] for LX2 kprotect: from 0x%lX to 0x%lX, using block %u\033[m\n", __PRETTY_FUNCTION__, __LINE__, krange->start_cpu_bus_pa, krange->start_cpu_bus_pa + krange->length, krange->miuBlockIndex);
            _miu_kernel_protect(krange->miuBlockIndex, g_kernel_protect_client_id,
                krange->start_cpu_bus_pa, krange->start_cpu_bus_pa + krange->length, MIU_PROTECT_ENABLE);
            glob_miu_kranges[miu_index].miuBlockStatus[krange->miuBlockIndex] = MIU_BLOCK_BUSY;

            glob_miu_kranges[miu_index].krange_num++;
            list_add_tail(&krange->list_node, &glob_miu_kranges[miu_index].list_head);
        }
    }

    if(lx_mem3_size != INVALID_PHY_ADDR)
    {
        cpu_bus_addr_to_MiuOffset(lx_mem3_addr, &miu_index, &offset);

        printk("\033[35mFunction = %s, Line = %d, Insert KProtect for LX3 @ MIU: %d\033[m\n", __PRETTY_FUNCTION__, __LINE__, miu_index);
        if(_insertKRange(miu_index, lx_mem3_addr, lx_mem3_size))    // we first check if LX3 can be combined to an existed protect_block(krange), if not, we add a new protect_block(krange)
        {
            krange = (MIU_ProtectRange *)kzalloc(sizeof(MIU_ProtectRange), GFP_KERNEL);
            MI_SYS_BUG_ON(!krange);
            INIT_LIST_HEAD(&krange->list_node);
            krange->start_cpu_bus_pa = lx_mem3_addr;
            krange->length = lx_mem3_size;
            krange->miuBlockIndex = glob_miu_kranges[miu_index].krange_num;
            //kernel protect block index start with 0

            printk("\033[35mFunction = %s, Line = %d, [INIT] for LX3 kprotect: from 0x%lX to 0x%lX, using block %u\033[m\n", __PRETTY_FUNCTION__, __LINE__, krange->start_cpu_bus_pa, krange->start_cpu_bus_pa + krange->length, krange->miuBlockIndex);
            _miu_kernel_protect(krange->miuBlockIndex, g_kernel_protect_client_id,
                krange->start_cpu_bus_pa, krange->start_cpu_bus_pa+krange->length, MIU_PROTECT_ENABLE);
            glob_miu_kranges[miu_index].miuBlockStatus[krange->miuBlockIndex] = MIU_BLOCK_BUSY;

            glob_miu_kranges[miu_index].krange_num++;

            list_add_tail(&krange->list_node, &glob_miu_kranges[miu_index].list_head);
        }
    }

}

#ifdef MI_SYS_PROC_FS_DEBUG

extern unsigned short clientId_KernelProtect[];
extern unsigned int u8_MiuWhiteListNum;
extern int clientId_KernelProtectToName(unsigned short clientId,char *clientName);

void dump_miu_and_lx_info(struct  seq_file  *m)
{
    int i=0;
    char *clientName;

    seq_printf(m,"ARM_MIU0_BUS_BASE 0x%llx       ARM_MIU0_BASE_ADDR 0x%llx\n",(MI_PHY)ARM_MIU0_BUS_BASE, (MI_PHY)ARM_MIU0_BASE_ADDR);
    seq_printf(m,"ARM_MIU1_BUS_BASE 0x%llx       ARM_MIU1_BASE_ADDR 0x%llx\n",(MI_PHY)ARM_MIU1_BUS_BASE, (MI_PHY)ARM_MIU1_BASE_ADDR);
    seq_printf(m,"ARM_MIU2_BUS_BASE 0x%llx       ARM_MIU2_BASE_ADDR 0x%llx\n",(MI_PHY)ARM_MIU2_BUS_BASE, (MI_PHY)ARM_MIU2_BASE_ADDR);
    seq_printf(m,"lx_mem_addr  0x%lx     lx_mem_size  0x%lx\n",lx_mem_addr,lx_mem_size);
    seq_printf(m,"lx_mem2_addr 0x%lx     lx_mem2_size 0x%lx\n",lx_mem2_addr,lx_mem2_size);
    seq_printf(m,"lx_mem3_addr 0x%lx     lx_mem3_size 0x%lx\n",lx_mem3_addr,lx_mem3_size);

    seq_printf(m,"\n\nKernelProtect IP white list:\n");
    seq_printf(m,"%15s%40s\n","clientId","name");
    //seq_printf(m,"(these numbers are in kernel code mdrv_miu.h file eMIUClientID enumeration variable,not in utopia code)\n");
    //N.B. strlen(clientId_KernelProtect) value is IDNUM_KERNELPROTECT in kernel code,
    //but here not easy to get IDNUM_KERNELPROTECT,so use strlen(clientId_KernelProtect).
    clientName = (char *)kmalloc(40,GFP_KERNEL);
    if(clientName != NULL)
    {
        memset(clientName,0,40);
        for(i=0; i < u8_MiuWhiteListNum ;i++)
        {
            seq_printf(m,"%15d",clientId_KernelProtect[i]);
            if(-1 != clientId_KernelProtectToName(clientId_KernelProtect[i],clientName))
                seq_printf(m,"%40s",clientName);
            else
                seq_printf(m,"%40s","error");

             seq_printf(m,"\n");
        }
        seq_printf(m,"\n");
        kfree(clientName);
    }

}

extern void dump_mi_sys_bufinfo(void);

//N.B. in ".write" file_operations for proc fs,should not use seq_printf
ssize_t miu_protect_write(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
{
    char dir_name_buf[150];
    size_t dir_name_buf_size = -1;//default value not be 0,because return 0 will cause loop.

    dir_name_buf_size = min(count, (sizeof(dir_name_buf)-1));
    if(copy_from_user(dir_name_buf, user_buf, dir_name_buf_size))
    {
        DBG_ERR("%s :%d \n",__FUNCTION__,__LINE__);
        return -EFAULT;
    }

    printk("in %s:%d\n",__FUNCTION__,__LINE__);

    if(dir_name_buf[0]=='0')
    {
        printk("first character of input is 0,parse input as 0\n");
        g_kprotect_enabled = 0;
        printk("kernel protect disabled\n");
    }
    else if(dir_name_buf[0]=='1')
    {
        printk("first character of input is 1,parse input as 1\n");
        g_kprotect_enabled = 1;
        printk("kernel protect enabled\n");
    }
    else if(dir_name_buf[0]=='2')
    {
        printk("first character of input is 2,parse input as 2\n");
        dump_mi_sys_bufinfo();
        return dir_name_buf_size;
    }
    else
    {
        //input user_buf is a dir
        printk(KERN_ERR "first character of input is not 0,not 1 ,not 2,so not support!!! If want get miu_protect info,please use cat cmd ,not use echo cmd !!\n");
    }

    return dir_name_buf_size;
}


int kprotect_status(struct seq_file *m, void *v)
{
    int kprotect_value;
    char buf[2];
    int i,j;
    struct MIU_ProtectRanges  *pranges ;
    MIU_ProtectRange *range = NULL;
    unsigned long offset = 0;
    int miu_index = 0;
    int lx_used_miu[KERN_CHUNK_NUM]={-1,-1,-1} ;
    char *clientName;

    kprotect_value = g_kprotect_enabled;
    if(kprotect_value == 0)
        buf[0] = '0';
    else
        buf[0] = '1';

    seq_printf(m,"===================  start miu_protect_info  ================================\n");

    if(kprotect_value > 0)
        seq_printf(m, "kernel protect enabled\n");
    else
        seq_printf(m, "kernel protect disabled\n");

    if(lx_mem_size != INVALID_PHY_ADDR)
    {
        cpu_bus_addr_to_MiuOffset(lx_mem_addr, &miu_index, &offset);
        lx_used_miu[0] = miu_index ;
        seq_printf(m,"LX :\n");
        seq_printf(m,"cpu_start_addr:0x%lx    size:0x%lx\n",lx_mem_addr,lx_mem_size);
    }
    if(lx_mem2_size != INVALID_PHY_ADDR)
    {
            cpu_bus_addr_to_MiuOffset(lx_mem2_addr, &miu_index, &offset);
        lx_used_miu[1] = miu_index ;
        seq_printf(m,"LX2 :\n");
        seq_printf(m,"cpu_start_addr:0x%lx    size:0x%lx\n",lx_mem2_addr,lx_mem2_size);
    }
    if(lx_mem3_size != INVALID_PHY_ADDR)
    {
            cpu_bus_addr_to_MiuOffset(lx_mem3_addr, &miu_index, &offset);
        lx_used_miu[2] = miu_index ;
        seq_printf(m,"LX3 :\n");
        seq_printf(m,"cpu_start_addr:0x%lx    size:0x%lx\n",lx_mem3_addr,lx_mem3_size);
    }

    for(i = 0; i < KERN_CHUNK_NUM-1; i++)
    {
        if(lx_used_miu[i] == -1)
            continue;
        for(j=i+1;j<KERN_CHUNK_NUM;j++)
            {
                  if(lx_used_miu[j] ==lx_used_miu[i])
                    lx_used_miu[j] =-1;
            }
    }

    seq_printf(m,"\n\nmiu_index   miuBlockIndex   start_cpu_bus_pa   length\n");
    for(i = 0; i < KERN_CHUNK_NUM; i++)
    {
        if(lx_used_miu[i] == -1)
            continue;

        miu_index = lx_used_miu[i];

        for(j=0;j<MIU_BLOCK_NUM;j++)
        {
            if(glob_miu_kranges[miu_index].miuBlockStatus[j] == MIU_BLOCK_BUSY)
                break;
        }
        if(j == MIU_BLOCK_NUM)//all miu block are MIU_BLOCK_IDLE
        {
            seq_printf(m,"%d  NA NA NA\n",miu_index);//only printk miu_index
            continue;//this miu_index do not have protect range
        }
        pranges = &glob_miu_kranges[miu_index];
        mutex_lock(&pranges->lock);
        list_for_each_entry(range, &pranges->list_head, list_node)
        {
            seq_printf(m,"0x%x                0x%02x       0x%lx        0x%lx\n",miu_index,range->miuBlockIndex,range->start_cpu_bus_pa,range->length);
        }
        mutex_unlock(&pranges->lock);
    }
    seq_printf(m,"\n\n");//only printk miu_index

    seq_printf(m,"\n\nKernelProtect IP white list:\n");
    seq_printf(m,"%15s%40s\n","clientId","name");
    //seq_printf(m,"(these numbers are in kernel code mdrv_miu.h file eMIUClientID enumeration variable,not in utopia code)\n");
    //N.B. strlen(clientId_KernelProtect) value is IDNUM_KERNELPROTECT in kernel code,
    //but here not easy to get IDNUM_KERNELPROTECT,so use strlen(clientId_KernelProtect).
    clientName = (char *)kmalloc(40,GFP_KERNEL);
    if(clientName != NULL)
    {
        memset(clientName,0,40);

        for(i=0; i < u8_MiuWhiteListNum  ;i++)
        {
            seq_printf(m,"%15d",clientId_KernelProtect[i]);
            if(-1 != clientId_KernelProtectToName(clientId_KernelProtect[i],clientName))
                seq_printf(m,"%40s",clientName);
            else
                seq_printf(m,"%40s","error");

             seq_printf(m,"\n");
        }
        seq_printf(m,"\n");
        kfree(clientName);
    }


    return 0;
}

int miu_protect_open(struct inode *inode, struct file *file)
{
    return single_open(file, kprotect_status, PDE_DATA(inode));
}
#endif
