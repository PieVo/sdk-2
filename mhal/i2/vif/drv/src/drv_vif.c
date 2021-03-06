#include <vif_common.h>
#include "drv_vif.h"
#include "hal_vif.h"
#include "mdrv_vif_io.h"
#include "vif_datatype.h"
#include <linux/delay.h>
#include "hal_dma.h"
#include "hal_rawdma.h"
#include <mhal_common.h>

#include "arch/infinity2_reg_padtop1.h"
#include "arch/infinity2_reg_block_ispsc.h"
#define REG_R(base,offset) (*(unsigned short*)(base+(offset*4)))
#define REG_W(base,offset,val) ((*(unsigned short*)((base)+ (offset*4)))=(val))
#define MAILBOX_HEART_REG (0x6D)
#define MAILBOX_CONCTRL_REG (0x6E)
#define MAILBOX_STATE_REG (0x6F)

#define VIF_SHIFTBITS(a)      (0x01<<(a))
#define VIF_MASK(a)      	  (~(0x01<<(a)))
#define VIF_CHECKBITS(a, b)   ((a) & ((u32)0x01 << (b)))
#define VIF_SETBIT(a, b)      (a) |= (((u32)0x01 << (b)))
#define VIF_CLEARBIT(a, b)    (a) &= (~((u32)0x01 << (b)))
//#define VIF_TASK_STACK_SIZE 	  (8192)

extern unsigned int *g_VIFReg[VIF_CHANNEL_NUM];
extern void DrvVif_msleep(u32 val);

#if 0
static MsTaskId_e      u8VifFSTaskId = 0xFF;
static MsTaskId_e      u8VifFETaskId = 0xFF;
static u32      *pVifFSStackTop = NULL;
static u32      *pVifFEStackTop = NULL;
Ms_Flag_t       _ints_event_flag;
#endif
#define ISP_MAX_BANK (13)

#define IPC_RAM_SIZE (64*1024)
#define TIMER_RATIO 12


#if IPC_DMA_ALLOC_COHERENT
extern u32 IPCRamPhys;
extern char *IPCRamVirtAddr;
#else
extern unsigned long IPCRamPhys;
extern void *IPCRamVirtAddr;
#endif


/***********************************8051 IPC ring buffer***********************************/
volatile VifRingBufShm_t *SHM_Ring = NULL;
const char * MCU_state_tlb[] = {"STOP","START","POLLING"};
VifnPTs vif_npts[VIF_PORT_NUM];
const u32 FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_MAX] = {
0xFFFFFFFF,  //FULL:          [11111111 11111111 11111111 11111111]
0x55555555,  //HALF:          [01010101 01010101 01010101 01010101]
0x11111111,  //QUARTER:       [00010001 00010001 00010001 00010001]
0x01010101,  //OCTANT:        [00000001 00000001 00000001 00000001]
0x77777777,  //THREE_QUARTERS:[01110111 01110111 01110111 01110111]
};


void (*vif_int_status_cb[VIF_CHANNEL_NUM][VIF_INTERRUPT_FINAL_STATUS_MAX]) (void);
void (*dmag_m_int_cb)(void);

//Mutex for MCU status change
CamOsMutex_t mculock;
//static struct mutex mculock;

int DrvVif_IntStatusCbReg(VIF_CHANNEL_e ch, VIF_INTERRUPT_e st, void (*cb)(void))
{
    if((ch >= VIF_CHANNEL_NUM) || (st >= VIF_INTERRUPT_FINAL_STATUS_MAX))
        return E_HAL_VIF_ERROR;

    vif_int_status_cb[ch][st] = cb;

    return E_HAL_VIF_SUCCESS;
}
EXPORT_SYMBOL(DrvVif_IntStatusCbReg);

s32 DrvVif_DoIntStatusCB(VIF_CHANNEL_e ch, VIF_INTERRUPT_e st)
{
    if((ch >= VIF_CHANNEL_NUM) || (st >= VIF_INTERRUPT_FINAL_STATUS_MAX))
        return E_HAL_VIF_ERROR;

    if(vif_int_status_cb[ch][st]){
        vif_int_status_cb[ch][st]();
    }
    return E_HAL_VIF_SUCCESS;
}

int DrvVif_RegDmagMCb(void (*cb)(void))
{
    dmag_m_int_cb = cb;

    return E_HAL_VIF_SUCCESS;
}
EXPORT_SYMBOL(DrvVif_RegDmagMCb);

int DrvVif_DoDmagMCb(unsigned int status)
{
    if(dmag_m_int_cb){
        dmag_m_int_cb();
    }
    return E_HAL_VIF_SUCCESS;
}


int initialSHMRing(void) {
    int idx=0, sub_idx=0;
    SHM_Ring = (VifRingBufShm_t *) (IPCRamVirtAddr + MCU_WINDOWS0_OFFSET);
    VIF_DEBUG("Ptr = %p, sizeof framebuf = %d  size of ring = %d\n",SHM_Ring, sizeof(VifRingBufElm_mcu), sizeof(VifRingBufShm_t));

    memset((VifRingBufShm_t *) SHM_Ring, 0x0, sizeof(VifRingBufShm_t)*VIF_PORT_NUM);
    memset(vif_npts, 0x0, sizeof(VifnPTs)*VIF_PORT_NUM);
    for (idx=0; idx<VIF_PORT_NUM; ++idx) {
        SHM_Ring[idx].nReadIdx  = 0/*idx*/;
        SHM_Ring[idx].nWriteIdx  = 0/*VIF_PORT_NUM -idx -1*/;
        SHM_Ring[idx].pre_nReadIdx  = 0;
        SHM_Ring[idx].nFrameStartCnt = 0;
        SHM_Ring[idx].nFrameDoneCnt = 0;
        SHM_Ring[idx].nFrameDoneSubCnt = 0;
        SHM_Ring[idx].nDropFrameCnt = 0;
        SHM_Ring[idx].nFPS_bitMap = FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_FULL];
        for (sub_idx=0; sub_idx<VIF_RING_QUEUE_SIZE; ++sub_idx) {
            SHM_Ring[idx].data[sub_idx].nStatus = VIF_BUF_INVALID;
        }
    }
    //Mutex initial
    CamOsMutexInit(&mculock);
    //mutex_init(&mculock);

    return E_HAL_VIF_SUCCESS;
}

int resetSHMRing(MHal_VIF_CHN u32VifChn){

    int sub_idx=0;

    SHM_Ring[u32VifChn].nReadIdx  = 0/*idx*/;
    SHM_Ring[u32VifChn].nWriteIdx  = 0/*VIF_PORT_NUM -idx -1*/;
    SHM_Ring[u32VifChn].pre_nReadIdx  = 0;
    SHM_Ring[u32VifChn].nFrameStartCnt = 0;
    SHM_Ring[u32VifChn].nFrameDoneCnt = 0;
    SHM_Ring[u32VifChn].nFrameDoneSubCnt = 0;
    SHM_Ring[u32VifChn].nDropFrameCnt = 0;
    SHM_Ring[u32VifChn].nFPS_bitMap = FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_FULL];
    for (sub_idx=0; sub_idx<VIF_RING_QUEUE_SIZE; ++sub_idx) {
        SHM_Ring[u32VifChn].data[sub_idx].nStatus = VIF_BUF_INVALID;
    }

    return E_HAL_VIF_SUCCESS;
}

int unInitialSHMRing(void) {
    memset((VifRingBufShm_t *) SHM_Ring, 0x0, sizeof(VifRingBufShm_t)*VIF_PORT_NUM);
    memset(vif_npts, 0x0, sizeof(VifnPTs)*VIF_PORT_NUM);
    SHM_Ring = NULL;
    return E_HAL_VIF_SUCCESS;
}

u64 GetTimerCnt64(u32 npts, u8 portidx)
{
    //static u64 count64 = 0;
    //static u32 prev_count = 0;
    u32 count = npts;
    if(vif_npts[portidx].prev_count>count) {
        vif_npts[portidx].count64 += (0xFFFFFFFF - vif_npts[portidx].prev_count + count);
    } else
        vif_npts[portidx].count64 += (count - vif_npts[portidx].prev_count);

    vif_npts[portidx].prev_count = count;

    return vif_npts[portidx].count64;
}

u32 GetTimerCnt32(u32 npts, u8 portidx)
{
    u64 val = GetTimerCnt64(npts, portidx);
    do_div(val,TIMER_RATIO);
    return val&0xFFFFFFFF;
}

u32 convertMhalBuffElmtoMCU(volatile VifRingBufElm_mcu *mcu, const MHal_VIF_RingBufElm_t *ptFbInfo) {
    mcu->nPhyBufAddrY_H = (ptFbInfo->u64PhyAddr[0] >> 32) & 0xFFFFFFFF;
    mcu->nPhyBufAddrY_L = (ptFbInfo->u64PhyAddr[0]& 0xFFFFFFFF)>>5;
    mcu->nPhyBufAddrC_H = (ptFbInfo->u64PhyAddr[1] >> 32) & 0xFFFFFFFF;
    mcu->nPhyBufAddrC_L = (ptFbInfo->u64PhyAddr[1]& 0xFFFFFFFF)>>5;

    mcu->PitchY = (ptFbInfo->u32Stride[0]+31)>>5;
    mcu->PitchC = (ptFbInfo->u32Stride[1]+31)>>5;

    mcu->nCropX = ptFbInfo->nCropX;
    mcu->nCropY = ptFbInfo->nCropY;
    mcu->nCropW = ptFbInfo->nCropW-1;
    mcu->nCropH = (ptFbInfo->nCropH-1) | 0x8000;

    mcu->nHeightY = mcu->nCropH;
    mcu->nHeightC = ((ptFbInfo->nCropH >> 1) -1) | 0x8000;

    mcu->nPTS = ptFbInfo->nPTS;
    mcu->nMiPriv = ptFbInfo->nMiPriv;
    mcu->nStatus = ptFbInfo->nStatus;
    mcu->tag = 0;
    return E_HAL_VIF_SUCCESS;
}

u32 convertMCUBuffElmtoMhal(volatile VifRingBufElm_mcu *mcu, MHal_VIF_RingBufElm_t *element, u8 portidx) {
    u64 tmp;

    tmp = mcu->nPhyBufAddrY_H;
    tmp = (tmp<<32) | (mcu->nPhyBufAddrY_L << 5);
    element->u64PhyAddr[0] = tmp;

    tmp = mcu->nPhyBufAddrC_H;
    tmp = (tmp<<32) | (mcu->nPhyBufAddrC_L << 5);
    element->u64PhyAddr[1] = tmp;

    element->u32Stride[0] = (mcu->PitchY << 5);
    element->u32Stride[1] = (mcu->PitchC << 5);

    element->nCropX = mcu->nCropX;
    element->nCropY = mcu->nCropY;
    element->nCropW = mcu->nCropW+1;
    element->nCropH = (mcu->nCropH+1)&0x7FFF ;

    element->nMiPriv = mcu->nMiPriv;
    element->nStatus = mcu->nStatus;

    element->nPTS = GetTimerCnt32(mcu->nPTS, portidx);

    if(((mcu->eFieldType >> 2) & 0x1) == 1)  //high byte bit 2 of bit 10
        element->eFieldType = E_VIF_FIELDTYPE_BOTTOM;
    else
        element->eFieldType = E_VIF_FIELDTYPE_TOP;


    //printk("Dequeue buffer info: AddrY=0x%x AddrC=0x%x width=%d height=%d pitch=%d nPTS=0x%x FieldID:0x%x\n",
    //        (u32)element->u64PhyAddr[0], (u32)element->u64PhyAddr[1], element->nCropW, element->nCropH, element->u32Stride[0], element->nPTS,element->eFieldType);
    return E_HAL_VIF_SUCCESS;
}

u32 convertFPSMaskToMCU(u32 mask) {
    u32 res = 0;
    u8 *off = (u8 *)&mask;
    u8 *r_off = (u8 *)&res;
    *r_off = *(off+3);
    *(r_off+1) = *(off+2);
    *(r_off+2) = *(off+1);
    *(r_off+3) = *off;
    return res;
}

s32 DrvVif_DevSetConfig(MHal_VIF_DEV u32VifDev, MHal_VIF_DevCfg_t *pstDevAttr)
{
	volatile infinity2_reg_padtop1* padtop = (infinity2_reg_padtop1*) g_TOPPAD1;
	volatile infinity2_reg_block_ispsc* vifclk = (infinity2_reg_block_ispsc*) g_ISP_ClkGen;
	VIF_DEBUG("%s , dev=%d  chnum:%d   edge:%d***\n",__FUNCTION__,u32VifDev,pstDevAttr->eWorkMode,pstDevAttr->eClkEdge);
	// TOPPAD1 setting
	//REG_W(g_TOPPAD1, 0x00, 0x0000);
	padtop->reg_all_pad_in = 0;
	switch(u32VifDev)
	{
	case 0:
		//REG_W(g_TOPPAD1, 0x02, 0xB550);
		//REG_W(g_TOPPAD1, 0x03, 0x0169);
		//sr0
		padtop->reg_ccir0_16b_mode = 0;
		padtop->reg_ccir0_8b_mode  = 1;
		padtop->reg_ccir0_clk_mode = 1;
		padtop->reg_ccir0_ctrl_mode =1;
		//REG_W(g_TOPPAD1, 0x28, 0x03FF);
		padtop->reg_snr0_d_ie = 0x3FF;

        vifclk->reg_ckg_snr0 = 0x18;
        vifclk->reg_ckg_snr1 = 0x18;
        vifclk->reg_ckg_snr2 = 0x1A;
        vifclk->reg_ckg_snr3 = 0x1A;

        switch(pstDevAttr->eWorkMode){

        case E_MHAL_VIF_WORK_MODE_1MULTIPLEX :

            vifclk->reg_ckg_snr0 = 0x18;
            vifclk->reg_ckg_snr1 = 0x18;
            vifclk->reg_ckg_snr2 = 0x18;
            vifclk->reg_ckg_snr3 = 0x18;

        break;
        case E_MHAL_VIF_WORK_MODE_2MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr0 = 0x10;
                vifclk->reg_ckg_snr1 = 0x10;
                vifclk->reg_ckg_snr2 = 0x12;
                vifclk->reg_ckg_snr3 = 0x12;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){

                vifclk->reg_ckg_snr0 = 0x18;
                vifclk->reg_ckg_snr1 = 0x18;
                vifclk->reg_ckg_snr2 = 0x1A;
                vifclk->reg_ckg_snr3 = 0x1A;

            }else{

                VIF_DEBUG("Not support\n");
            }

        break;
        case E_MHAL_VIF_WORK_MODE_4MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr0 = 0x10;
                vifclk->reg_ckg_snr1 = 0x14;
                vifclk->reg_ckg_snr2 = 0x12;
                vifclk->reg_ckg_snr3 = 0x16;
                vifclk->reg_sc_spare_lo |= 0x01<<4;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){
                VIF_DEBUG("Not support\n");
            }else{
                VIF_DEBUG("Not support\n");
            }

        break;
        default :
        break;
        }


	break;
	case 1:
		//sr1
		padtop->reg_ccir1_8b_mode  = 1;
		padtop->reg_ccir1_clk_mode = 1;
		padtop->reg_ccir1_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x2E, 0x03FF);
		padtop->reg_snr1_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x44, 0x1818);
		vifclk->reg_ckg_snr4 = 0x18;
		vifclk->reg_ckg_snr5 = 0x18;
		//REG_W(g_ISP_ClkGen, 0x45, 0x1A1A);
		vifclk->reg_ckg_snr6 = 0x1A;
		vifclk->reg_ckg_snr7 = 0x1A;


        switch(pstDevAttr->eWorkMode){

        case E_MHAL_VIF_WORK_MODE_1MULTIPLEX :

            vifclk->reg_ckg_snr4 = 0x18;
            vifclk->reg_ckg_snr5 = 0x18;
            vifclk->reg_ckg_snr6 = 0x18;
            vifclk->reg_ckg_snr7 = 0x18;

        break;
        case E_MHAL_VIF_WORK_MODE_2MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr4 = 0x10;
                vifclk->reg_ckg_snr5 = 0x10;
                vifclk->reg_ckg_snr6 = 0x12;
                vifclk->reg_ckg_snr7 = 0x12;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){

                vifclk->reg_ckg_snr4 = 0x18;
                vifclk->reg_ckg_snr5 = 0x18;
                vifclk->reg_ckg_snr6 = 0x1A;
                vifclk->reg_ckg_snr7 = 0x1A;

            }else{

                VIF_DEBUG("Not support\n");
            }

        break;
        case E_MHAL_VIF_WORK_MODE_4MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr4 = 0x10;
                vifclk->reg_ckg_snr5 = 0x14;
                vifclk->reg_ckg_snr6 = 0x12;
                vifclk->reg_ckg_snr7 = 0x16;
                vifclk->reg_sc_spare_lo |= 1<<5;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){
                VIF_DEBUG("Not support\n");
            }else{
                VIF_DEBUG("Not support\n");
            }

        break;
        default :
        break;
        }


	break;
	case 2:
		//sr2
		padtop->reg_ccir2_16b_mode = 0;
		padtop->reg_ccir2_8b_mode  = 1;
		padtop->reg_ccir2_clk_mode = 1;
		padtop->reg_ccir2_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x34, 0x03FF);
		padtop->reg_snr2_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x46, 0x1818);
		vifclk->reg_ckg_snr8 = 0x18;
		vifclk->reg_ckg_snr9 = 0x18;
		//REG_W(g_ISP_ClkGen, 0x47, 0x1A1A);
		vifclk->reg_ckg_snr10 = 0x1A;
		vifclk->reg_ckg_snr11 = 0x1A;


        switch(pstDevAttr->eWorkMode){

        case E_MHAL_VIF_WORK_MODE_1MULTIPLEX :

            vifclk->reg_ckg_snr8 = 0x18;
            vifclk->reg_ckg_snr9 = 0x18;
            vifclk->reg_ckg_snr10 = 0x18;
            vifclk->reg_ckg_snr11 = 0x18;

        break;
        case E_MHAL_VIF_WORK_MODE_2MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr8 = 0x10;
                vifclk->reg_ckg_snr9 = 0x10;
                vifclk->reg_ckg_snr10 = 0x12;
                vifclk->reg_ckg_snr11 = 0x12;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){

                vifclk->reg_ckg_snr8 = 0x18;
                vifclk->reg_ckg_snr9 = 0x18;
                vifclk->reg_ckg_snr10 = 0x1A;
                vifclk->reg_ckg_snr11 = 0x1A;

            }else{

                VIF_DEBUG("Not support\n");
            }

        break;
        case E_MHAL_VIF_WORK_MODE_4MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr8 = 0x10;
                vifclk->reg_ckg_snr9 = 0x14;
                vifclk->reg_ckg_snr10 = 0x12;
                vifclk->reg_ckg_snr11 = 0x16;
                vifclk->reg_sc_spare_lo |= 1<<6;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){
                VIF_DEBUG("Not support\n");
            }else{
                VIF_DEBUG("Not support\n");
            }

        break;
        default :
        break;
        }


	break;
	case 3:
		//sr3
		padtop->reg_ccir3_8b_mode  = 1;
		padtop->reg_ccir3_clk_mode = 1;
		padtop->reg_ccir3_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x3A, 0x03FF);
		padtop->reg_snr3_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x48, 0x1818);
		vifclk->reg_ckg_snr12 = 0x18;
		vifclk->reg_ckg_snr13 = 0x18;
		//REG_W(g_ISP_ClkGen, 0x49, 0x1A1A);
		vifclk->reg_ckg_snr14 = 0x1A;
		vifclk->reg_ckg_snr15 = 0x1A;


        switch(pstDevAttr->eWorkMode){

        case E_MHAL_VIF_WORK_MODE_1MULTIPLEX :

            vifclk->reg_ckg_snr12 = 0x18;
            vifclk->reg_ckg_snr13 = 0x18;
            vifclk->reg_ckg_snr14 = 0x18;
            vifclk->reg_ckg_snr15 = 0x18;
        break;
        case E_MHAL_VIF_WORK_MODE_2MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr12 = 0x10;
                vifclk->reg_ckg_snr13 = 0x10;
                vifclk->reg_ckg_snr14 = 0x12;
                vifclk->reg_ckg_snr15 = 0x12;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){

                vifclk->reg_ckg_snr12 = 0x18;
                vifclk->reg_ckg_snr13 = 0x18;
                vifclk->reg_ckg_snr14 = 0x1A;
                vifclk->reg_ckg_snr15 = 0x1A;

            }else{

                VIF_DEBUG("Not support\n");
            }
        break;
        case E_MHAL_VIF_WORK_MODE_4MULTIPLEX :

            if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_SINGLE_UP){

                vifclk->reg_ckg_snr12 = 0x10;
                vifclk->reg_ckg_snr13 = 0x14;
                vifclk->reg_ckg_snr14 = 0x12;
                vifclk->reg_ckg_snr15 = 0x16;
                vifclk->reg_sc_spare_lo |= 1<<7;

            }else if(pstDevAttr->eClkEdge == E_MHAL_VIF_CLK_EDGE_DOUBLE){
                VIF_DEBUG("Not support\n");
            }else{
                VIF_DEBUG("Not support\n");
            }
        break;
        default :
        break;
        }
	break;
	}

	switch(pstDevAttr->eWorkMode)
	{
    case E_MHAL_VIF_WORK_MODE_1MULTIPLEX :
        HalDma_ConfigGroup(u32VifDev,1);
    break;
    case E_MHAL_VIF_WORK_MODE_2MULTIPLEX :
        HalDma_ConfigGroup(u32VifDev,2);
    break;
    case E_MHAL_VIF_WORK_MODE_4MULTIPLEX :
        HalDma_ConfigGroup(u32VifDev,4);
	break;
	default:
        HalDma_ConfigGroup(u32VifDev,4);
	break;
	}


    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_DevEnable(MHal_VIF_DEV u32VifDev)
{
	//VIF_DEBUG("===============   %s , dev=%d  ==============",__FUNCTION__,u32VifDev);
    HalDma_GlobalEnable();
	HalDma_EnableGroup(u32VifDev);
    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_DevDisable(MHal_VIF_DEV u32VifDev)
{
	volatile infinity2_reg_padtop1* padtop = (infinity2_reg_padtop1*) g_TOPPAD1;
	volatile infinity2_reg_block_ispsc* vifclk = (infinity2_reg_block_ispsc*) g_ISP_ClkGen;
	//VIF_DEBUG("%s , dev=%d",__FUNCTION__,u32VifDev);
	// TOPPAD1 setting
	//REG_W(g_TOPPAD1, 0x00, 0x0000);
	padtop->reg_all_pad_in = 0;
	switch(u32VifDev)
	{
	case 0:
		//REG_W(g_TOPPAD1, 0x02, 0xB550);
		//REG_W(g_TOPPAD1, 0x03, 0x0169);
		//sr0
		padtop->reg_ccir0_16b_mode = 0;
		padtop->reg_ccir0_8b_mode  = 0;
		padtop->reg_ccir0_clk_mode = 0;
		padtop->reg_ccir0_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x28, 0x03FF);
		padtop->reg_snr0_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x42, 0x1818);
		vifclk->reg_ckg_snr0 = 0x00;
		vifclk->reg_ckg_snr1 = 0x00;
		//REG_W(g_ISP_ClkGen, 0x43, 0x1A1A);
		vifclk->reg_ckg_snr2 = 0x00;
		vifclk->reg_ckg_snr3 = 0x00;
	break;
	case 1:
		//sr1
		padtop->reg_ccir1_8b_mode  = 0;
		padtop->reg_ccir1_clk_mode = 0;
		padtop->reg_ccir1_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x2E, 0x03FF);
		padtop->reg_snr1_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x44, 0x1818);
		vifclk->reg_ckg_snr4 = 0x00;
		vifclk->reg_ckg_snr5 = 0x00;
		//REG_W(g_ISP_ClkGen, 0x45, 0x1A1A);
		vifclk->reg_ckg_snr6 = 0x00;
		vifclk->reg_ckg_snr7 = 0x00;
	break;
	case 2:
		//sr2
		padtop->reg_ccir2_16b_mode = 0;
		padtop->reg_ccir2_8b_mode  = 0;
		padtop->reg_ccir2_clk_mode = 0;
		padtop->reg_ccir2_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x34, 0x03FF);
		padtop->reg_snr2_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x46, 0x1818);
		vifclk->reg_ckg_snr8 = 0x00;
		vifclk->reg_ckg_snr9 = 0x00;
		//REG_W(g_ISP_ClkGen, 0x47, 0x1A1A);
		vifclk->reg_ckg_snr10 = 0x00;
		vifclk->reg_ckg_snr11 = 0x00;
	break;
	case 3:
		//sr3
		padtop->reg_ccir3_8b_mode  = 0;
		padtop->reg_ccir3_clk_mode = 0;
		padtop->reg_ccir3_ctrl_mode =0;
		//REG_W(g_TOPPAD1, 0x3A, 0x03FF);
		padtop->reg_snr3_d_ie = 0x3FF;

		/*    VIF CLK    */
		//REG_W(g_ISP_ClkGen, 0x48, 0x1818);
		vifclk->reg_ckg_snr12 = 0x00;
		vifclk->reg_ckg_snr13 = 0x00;
		//REG_W(g_ISP_ClkGen, 0x49, 0x1A1A);
		vifclk->reg_ckg_snr14 = 0x00;
		vifclk->reg_ckg_snr15 = 0x00;
	break;
	}
    return E_HAL_VIF_SUCCESS;
}


//MAIN Channel
s32 DrvVif_ChnSetConfig(MHal_VIF_CHN u32VifChn, MHal_VIF_ChnCfg_t *pstAttr)
{
	WdmaCropParam_t stCrop;
	VIF_DEBUG("%s , ch=%d, W=%d, H=%d\n",__FUNCTION__,u32VifChn,pstAttr->stCapRect.u16Width,pstAttr->stCapRect.u16Height);
	// channel setting
	HalVif_SensorSWReset(u32VifChn, VIF_ENABLE);
	HalVif_IFStatusReset(u32VifChn, VIF_DISABLE);
	HalVif_SensorReset(u32VifChn, VIF_DISABLE);
	HalVif_SensorPowerDown(u32VifChn, VIF_DISABLE);
	HalVif_HDRen(u32VifChn, VIF_DISABLE);
	HalVif_HDRSelect(u32VifChn, VIF_HDR_VC0);
	HalVif_SelectSource(u32VifChn, VIF_CH_SRC_BT656);
	//HalVif_SensorChannelEnable(u32VifChn, VIF_ENABLE;
	HalVif_SensorFormatLeftSht(u32VifChn, VIF_DISABLE);
	HalVif_SensorBitSwap(u32VifChn, VIF_DISABLE);
	//HalVif_SensorHsyncPolarity(u32VifChn, VIF_SENSOR_POLARITY_HIGH_ACTIVE);
	HalVif_SensorVsyncPolarity(u32VifChn, VIF_SENSOR_POLARITY_HIGH_ACTIVE);
	HalVif_SensorFormat(u32VifChn, VIF_SENSOR_FORMAT_16BIT);
	HalVif_SensorRgbIn(u32VifChn, VIF_SENSOR_INPUT_FORMAT_RGB);
	HalVif_SensorFormatExtMode(u32VifChn, VIF_SENSOR_BIT_MODE_1);
	HalVif_BT656ChannelDetectEnable(u32VifChn, VIF_ENABLE);
	HalVif_BT656VSDelay(u32VifChn, VIF_BT656_VSYNC_DELAY_BT656);
	HalVif_BT656HorizontalCropSize(u32VifChn, 0xFFFF);
	//HalVif_PixCropEnd(u32VifChn,0xFFFF);

	stCrop.uW = pstAttr->stCapRect.u16Width;
	stCrop.uH = pstAttr->stCapRect.u16Height;
	stCrop.uX = pstAttr->stCapRect.u16X;
	stCrop.uY = pstAttr->stCapRect.u16Y;

    HalVif_Crop(u32VifChn,0, 0, 0x1FFF, 0x1FFF );
    HalVif_CropEnable(u32VifChn,VIF_DISABLE);
#if 1  //work around for shadow not active
    HalDma_Config(u32VifChn,&stCrop,0x08000000,0x08100000);
#else
    HalDma_Config(u32VifChn,&stCrop,0x2266f000,0x2266f000);
#endif
    HalDma_MaskOutput(u32VifChn,0x1);
	// ISP CLKGen setting

	stCrop.uW /= 2;
	stCrop.uH /= 2;
	HalDma_ConfigSub(u32VifChn,&stCrop,0x08200000,0x08300000);
	HalDma_MaskOutputSub(u32VifChn,0x1);

    //FPS mask setting
    DrvVif_setChnFPSBitMask(u32VifChn, VIF_CHN_MAIN, pstAttr->eFrameRate, NULL);

    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_ChnEnable(MHal_VIF_CHN u32VifChn)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    HalVif_SensorChannelEnable(u32VifChn, VIF_ENABLE);
    HalVif_ChannelAlignment(u32VifChn);
	HalDma_DmaMaskEnable(u32VifChn,1);
    HalDma_Trigger(u32VifChn,WDMA_TRIG_CONTINUE);
    HalDma_EnableIrq(u32VifChn);
    HalVif_EnableIrq(u32VifChn);

    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_ChnDisable(MHal_VIF_CHN u32VifChn)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    HalDma_DmaMaskEnable(u32VifChn,1);
    HalVif_SensorChannelEnable(u32VifChn, VIF_DISABLE);
    //HalDma_Trigger(u32VifChn,WDMA_TRIG_STOP);
    HalDma_DisableIrq(u32VifChn);
    resetSHMRing(u32VifChn);

    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_ChnQuery(MHal_VIF_CHN u32VifChn, MHal_VIF_ChnStat_t *pstStat)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);

	WdmaInfo_t WdmaInfo;
    VifInfo_t VifInfo;
    u32 tmp = 0;
    int i=0;

	pstStat->nReadIdx = SHM_Ring[u32VifChn].nReadIdx;
	pstStat->nWriteIdx = SHM_Ring[u32VifChn].nWriteIdx;
	pstStat->nDequeueIdx = SHM_Ring[u32VifChn].pre_nReadIdx;
    // 8051 Cail C byte order convert to ARM format
	tmp =  ((SHM_Ring[u32VifChn].nDropFrameCnt >> 8) & 0x00FF00FF) + ((SHM_Ring[u32VifChn].nDropFrameCnt << 8) & 0xFF00FF00);
	pstStat->nDropFrameCnt = ((tmp >> 16) & 0x0000FFFF) + ((tmp << 16) & 0xFFFF0000);

	tmp =  ((SHM_Ring[u32VifChn].nFrameDoneCnt >> 8) & 0x00FF00FF) + ((SHM_Ring[u32VifChn].nFrameDoneCnt << 8) & 0xFF00FF00);
    pstStat->nFrameDoneCnt = ((tmp >> 16) & 0x0000FFFF) + ((tmp << 16) & 0xFFFF0000);

    tmp =  ((SHM_Ring[u32VifChn].nFrameStartCnt >> 8) & 0x00FF00FF) + ((SHM_Ring[u32VifChn].nFrameStartCnt << 8) & 0xFF00FF00);
    pstStat->nFrameStartCnt = ((tmp >> 16) & 0x0000FFFF) + ((tmp << 16) & 0xFFFF0000);

	HalDma_GetDmaInfo(u32VifChn,&WdmaInfo);
	pstStat->nOutputWidth = WdmaInfo.uWidth;
	pstStat->nOutputHeight = WdmaInfo.uHeight;
	HalDma_GetSubDmaInfo(u32VifChn,&WdmaInfo);
	pstStat->nSubOutputWidth = WdmaInfo.uWidth;
	pstStat->nSubOutputHeight = WdmaInfo.uHeight;
	HalVif_GetVifInfo(u32VifChn,&VifInfo);
    pstStat->nReceiveWidth = VifInfo.uReceiveWidth;
    pstStat->nReceiveHeight = VifInfo.uReceiveHeight;

    for(i=0 ; i< VIF_RING_QUEUE_SIZE; i++){
        pstStat->eStatus[i] = SHM_Ring[u32VifChn].data[i].nStatus;
        //printk("<%d,%d>",pstStat->eStatus[i],SHM_Ring[u32VifChn].data[i].nStatus);
    }


    return E_HAL_VIF_SUCCESS;
}


//SUB Channel
s32 DrvVif_SubChnSetConfig(MHal_VIF_CHN u32VifChn, MHal_VIF_SubChnCfg_t *pstAttr)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    //FPS mask setting
    DrvVif_setChnFPSBitMask(u32VifChn, VIF_CHN_SUB, pstAttr->eFrameRate, NULL);

    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_SubChnEnable(MHal_VIF_CHN u32VifChn)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    HalDma_TriggerSub(u32VifChn,WDMA_TRIG_CONTINUE);
    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_SubChnDisable(MHal_VIF_CHN u32VifChn)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    HalDma_TriggerSub(u32VifChn,WDMA_TRIG_STOP);
    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_SubChnQuery(MHal_VIF_CHN u32VifChn, MHal_VIF_ChnStat_t *pstStat)
{
    //VIF_DEBUG("%s , ch=%d",__FUNCTION__,u32VifChn);
    return E_HAL_VIF_SUCCESS;
}


//Ring buffer queue/dequeue
s32 DrvVif_QueueFrameBuffer(MHal_VIF_CHN u32VifChn, MHal_VIF_PORT u32ChnPort, const MHal_VIF_RingBufElm_t *ptFbInfo)
{
    int mcu_chn = u32VifChn + ((u32ChnPort>0)? VIF_PORT_NUM/2:0);
    u16 w_idx = SHM_Ring[mcu_chn].nWriteIdx;

    /* DEBUG */
    //#define DRAM_BASE 0x20000000
    pr_debug("%s Port=%d, BufY=0x%X, BufC=0x%X",__FUNCTION__,u32ChnPort,(u32)ptFbInfo->u64PhyAddr[0],(u32)ptFbInfo->u64PhyAddr[1]);
    pr_debug("Stride0=0x%X, Stride1=0x%X",(u32)ptFbInfo->u32Stride[0],(u32)ptFbInfo->u32Stride[1]);
    /*
    if(u32ChnPort==0)
        HalDma_SetOutputAddr(u32VifChn,
            Chip_Phys_to_MIU(ptFbInfo->u64PhyAddr[0]),
            Chip_Phys_to_MIU(ptFbInfo->u64PhyAddr[1]),
            ptFbInfo->u32Stride[0]);
    else
        HalDma_SetOutputAddrSub(u32VifChn,
            Chip_Phys_to_MIU(ptFbInfo->u64PhyAddr[0]),
            Chip_Phys_to_MIU(ptFbInfo->u64PhyAddr[1]),
            ptFbInfo->u32Stride[0]);

    return E_HAL_VIF_SUCCESS;
    */
    pr_debug("[VIF] Channel[%d] Port[%d] Ququeue frame!\n",u32VifChn,u32ChnPort);

    if (!SHM_Ring[mcu_chn].nEnable) {
        //SHM_Ring[u32VifChn].nEnable =1;
        pr_debug("Port[%d] do not ready to enqueue!!\n",u32VifChn);
        return MHAL_FAILURE;
    }

    if (SHM_Ring[mcu_chn].data[w_idx].nStatus != VIF_BUF_INVALID) {
        //SHM_Ring[u32VifChn].data[w_idx] = *ptFbInfo;
        pr_debug("Ring buffer full!! Waiting for dequeue!!\n");
        return MHAL_FAILURE;
    }

    //convert Mhal elm to MCU elm
    convertMhalBuffElmtoMCU(&(SHM_Ring[mcu_chn].data[w_idx]), ptFbInfo);
    SHM_Ring[mcu_chn].data[w_idx].nStatus = VIF_BUF_EMPTY;
    wmb();

    if ((w_idx +1) == VIF_RING_QUEUE_SIZE)
        SHM_Ring[mcu_chn].nWriteIdx = 0;
    else
        SHM_Ring[mcu_chn].nWriteIdx++;

    wmb();

    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_QueryFrames(MHal_VIF_CHN u32VifChn, MHal_VIF_PORT u32ChnPort, u32 *pNumBuf)
{
    int mcu_chn = u32VifChn + ((u32ChnPort>0)? VIF_PORT_NUM/2:0);
    u8 p_ridx = SHM_Ring[mcu_chn].pre_nReadIdx;

    u32 readyNum = 0;
    while(readyNum < VIF_RING_QUEUE_SIZE && SHM_Ring[mcu_chn].data[p_ridx].nStatus == VIF_BUF_READY) {
        readyNum++;
        p_ridx = (p_ridx+1)%VIF_RING_QUEUE_SIZE;
    }
    *pNumBuf = readyNum;
    rmb();
    //VIF_DEBUG("Port[%d] Number of ready frame = %d \n",u32VifChn, *pNumBuf);
    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_DequeueFrameBuffer(MHal_VIF_CHN u32VifChn, MHal_VIF_PORT u32ChnPort, MHal_VIF_RingBufElm_t *ptFbInfo)
{
    int mcu_chn = u32VifChn + ((u32ChnPort>0)? VIF_PORT_NUM/2:0);
    u8 p_ridx = SHM_Ring[mcu_chn].pre_nReadIdx;

    //VIF_DEBUG("[VIF] Channel[%d] Port[%d] Dequeue frame!\n",u32VifChn,u32ChnPort);

    //Chip_Flush_Cache_Range((unsigned long)phys_to_virt(ptFbInfo->u64PhyAddr[0]), ptFbInfo->nCropW*ptFbInfo->nCropH);
    //Chip_Flush_Cache_Range((unsigned long)phys_to_virt(ptFbInfo->u64PhyAddr[1]), ((ptFbInfo->nCropW + 31)/32)*32 * ptFbInfo->nCropH);


    if (1/*SHM_Ring[mcu_chn].data[p_ridx].nStatus == VIF_BUF_READY*/) {
        convertMCUBuffElmtoMhal(&(SHM_Ring[mcu_chn].data[p_ridx]), ptFbInfo, mcu_chn);
        rmb();

        SHM_Ring[mcu_chn].data[p_ridx].nStatus = VIF_BUF_INVALID;
        wmb();

        if (SHM_Ring[mcu_chn].pre_nReadIdx +1 == VIF_RING_QUEUE_SIZE)
            SHM_Ring[mcu_chn].pre_nReadIdx = 0;
        else
            SHM_Ring[mcu_chn].pre_nReadIdx++;
        wmb();
    } else {
        return E_HAL_VIF_SUCCESS;
    }
    return E_HAL_VIF_SUCCESS;
}


s32 DrvVif_setChnFPSBitMask(MHal_VIF_CHN u32VifChn, MHal_VIF_PORT u32ChnPort, MHal_VIF_FrameRate_e u32Chnfps, u32 *manualMask) {
    int mcu_chn = u32VifChn + ((u32ChnPort>0)? VIF_PORT_NUM/2:0);

    switch(u32Chnfps) {
        case E_MHAL_VIF_FRAMERATE_FULL:
        case E_MHAL_VIF_FRAMERATE_HALF:
        case E_MHAL_VIF_FRAMERATE_QUARTER:
        case E_MHAL_VIF_FRAMERATE_OCTANT:
        case E_MHAL_VIF_FRAMERATE_THREE_QUARTERS:
            SHM_Ring[mcu_chn].nFPS_bitMap = FpsBitMaskArray[u32Chnfps];
        break;
        case E_MHAL_VIF_FRAMERATE_MANUAL:
            if (manualMask) {
                pr_err("FPS manual mask is 0x%x!!\n",*manualMask);
                SHM_Ring[mcu_chn].nFPS_bitMap = convertFPSMaskToMCU(*manualMask);
            } else {
                pr_err("FPS manual mask is NULL!!\n");
                SHM_Ring[mcu_chn].nFPS_bitMap = FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_FULL];
            }
        break;
        default:
            SHM_Ring[mcu_chn].nFPS_bitMap = FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_FULL];
            pr_err("MHal_VIF_FrameRate out of range!!\n");
    }

/*    if (u32Chnfps >= E_MHAL_VIF_FRAMERATE_MAX) {
        SHM_Ring[mcu_chn].nFPS_bitMap = FpsBitMaskArray[E_MHAL_VIF_FRAMERATE_FULL];
        pr_err("MHal_VIF_FrameRate out of range!!\n");
    } else {
        SHM_Ring[mcu_chn].nFPS_bitMap = FpsBitMaskArray[u32Chnfps];
    }
*/
    return E_HAL_VIF_SUCCESS;
}

// MCU control function

int DrvVif_setARMControlStatus(u16 status) {
    REG_W(g_MAILBOX, MAILBOX_CONCTRL_REG, status);
    DrvVif_msleep(20);
    return E_HAL_VIF_SUCCESS;
}

int DrvVif_getMCUStatus(u16 *status) {
    *status = REG_R(g_MAILBOX,MAILBOX_STATE_REG);
    DrvVif_msleep(1000);
    return E_HAL_VIF_SUCCESS;
}

u32 DrvVif_changeMCUStatus(u16 status) {
    u16 m_st = 0xFFFF;
    u16 wait_count = 0;

    if (status >= MCU_STATE_NUM)
        VIF_DEBUG("Setting status out of bound!!\n");

    DrvVif_setARMControlStatus(status);
    DrvVif_getMCUStatus(&m_st);
    while(m_st != status) {
        VIF_DEBUG("Waititg for MCU %s done~~~....(%x)\n",MCU_state_tlb[status],m_st);
        DrvVif_getMCUStatus(&m_st);
        ++wait_count;

        if (wait_count >MCU_REG_TIMEOUT)
            return E_HAL_VIF_ERROR;
    }

    VIF_DEBUG("MCU %s id done(%x)\n",MCU_state_tlb[status],m_st);

    return E_HAL_VIF_SUCCESS;
}

u32 DrvVif_stopMCU(void) {
    CamOsMutexLock(&mculock);
    //Get reg initial value, must be STOP
    if (DrvVif_changeMCUStatus(MCU_STATE_STOP)) {
        CamOsMutexUnlock(&mculock);
        return E_HAL_VIF_ERROR;
    }
    CamOsMutexUnlock(&mculock);
    return E_HAL_VIF_SUCCESS;
}


u32 DrvVif_startMCU(void) {
    CamOsMutexLock(&mculock);
    //Initial success, set to READY mode
    if (DrvVif_changeMCUStatus(MCU_STATE_READY)) {
        CamOsMutexUnlock(&mculock);
        return E_HAL_VIF_ERROR;
    }
    CamOsMutexUnlock(&mculock);
    return E_HAL_VIF_SUCCESS;
}


u32 DrvVif_pollingMCU(void) {
    u16 m_st = 0x00FF;
    DrvVif_getMCUStatus(&m_st);
    if (m_st == MCU_STATE_STOP) {
        pr_err("Invalid operation from stop state to polling!!\n");
        return E_HAL_VIF_ERROR;
    }
    //Enter POLLING mode
    CamOsMutexLock(&mculock);
    if (DrvVif_changeMCUStatus(MCU_STATE_POLLING)) {
        CamOsMutexUnlock(&mculock);
        return E_HAL_VIF_ERROR;
    }
    CamOsMutexUnlock(&mculock);
    return E_HAL_VIF_SUCCESS;
}


u32 DrvVif_pauseMCU(void) {
    CamOsMutexLock(&mculock);
    if (DrvVif_changeMCUStatus(MCU_STATE_READY)) {
        CamOsMutexUnlock(&mculock);
        return E_HAL_VIF_ERROR;
    }
    CamOsMutexUnlock(&mculock);
    return E_HAL_VIF_SUCCESS;
}


/*******************************************************************************************8*/

static volatile u32 ulgvif_def_mask[VIF_CHANNEL_NUM] =
{
    //VIF_CHANNEL_0
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_FALLING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_HW_FLASH_STROBE_DONE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE),
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0)|
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1),

    //VIF_CHANNEL_1,
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_FALLING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_HW_FLASH_STROBE_DONE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE),
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0)|
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1),

    //VIF_CHANNEL_2,
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_FALLING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_HW_FLASH_STROBE_DONE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE),
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0)|
    //VIF_SHIFTBITS(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1),
};

// For MV5 use VIF 3 channel
void DrvVif_ISR(void)
{
#if 0
    //volatile u32 u4Clear0 = 0;
    //volatile u32 u4Clear1 = 0;
    //volatile u32 u4Clear2 = 0;

    volatile u32 u4Status0 = 0;
    volatile u32 u4Status1 = 0;
    volatile u32 u4Status2 = 0;

    u4Status0 = HalVif_IrqFinalStatus(VIF_CHANNEL_0);
    //HalVif_IrqMask(VIF_CHANNEL_0, u4Status0);

    u4Status1 = HalVif_IrqFinalStatus(VIF_CHANNEL_1);
    //HalVif_IrqMask(VIF_CHANNEL_1, u4Status1);

    u4Status2 = HalVif_IrqFinalStatus(VIF_CHANNEL_2);
    //HalVif_IrqMask(VIF_CHANNEL_2, u4Status2);


    //UartSendTrace("## [%s] mask1 0x%04x, mask2 0x%04x, mask3 0x%04x\n", __func__, u4Status, u4Status2, u4Status3);

	/***********************************
	* LINECNT 0
	***********************************/
	// Channel_0
    if(VIF_CHECKBITS(u4Status0, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH0_FRAME_START_INTS);
        HalVif_IrqClr(VIF_CHANNEL_0, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0));
    }

	//Channel_01
    if(VIF_CHECKBITS(u4Status1, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH1_FRAME_START_INTS);
        HalVif_IrqClr(VIF_CHANNEL_1, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0));
    }

	// Channel_2
    if(VIF_CHECKBITS(u4Status2, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH2_FRAME_START_INTS);
        HalVif_IrqClr(VIF_CHANNEL_2, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0));
    }

	/***********************************
	* LINECNT 1
	***********************************/
	// Channel_0
    if(VIF_CHECKBITS(u4Status0, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH0_FRAME_END_INTS);
        HalVif_IrqClr(VIF_CHANNEL_0, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1));
    }

	// Channel_0
    if(VIF_CHECKBITS(u4Status1, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH1_FRAME_END_INTS);
        HalVif_IrqClr(VIF_CHANNEL_1, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1));
    }

	// Channel_0
    if(VIF_CHECKBITS(u4Status2, VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1))
    {
        //wake_up_interruptible_all(&isp_wq_VSTART);
        MsFlagSetbits(&_ints_event_flag, VIF_CH2_FRAME_END_INTS);
        HalVif_IrqClr(VIF_CHANNEL_2, VIF_MASK(VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1));
    }
#endif
}

#if 0
int DrvVif_IntsSimulator(VIF_INTS_EVENT_TYPE_e ints)
{
    MsFlagSetbits(&_ints_event_flag, ints);
	return VIF_SUCCESS;
}

int DrvVif_ISRCreate()
{
    MsIntInitParam_u param;

    memset(&param, 0, sizeof(param));
    param.intc.eMap = INTC_MAP_IRQ;
    param.intc.ePriority = INTC_PRIORITY_10;
    param.intc.pfnIsr = DrvVif_ISR;
    MsInitInterrupt(&param, MS_INT_NUM_IRQ_VIF);
    MsUnmaskInterrupt(MS_INT_NUM_IRQ_VIF);

	return VIF_SUCCESS;
}
#endif
int DrvVif_CLK(void)
{

	return VIF_SUCCESS;
}

int DrvVif_PatGenCfg(VIF_CHANNEL_e ch, VifPatGenCfg_t *PatGenCfg)
{

	return VIF_SUCCESS;
}

int DrvVif_SensorFrontEndCfg(VIF_CHANNEL_e ch, VifSensorCfg_t *sensorcfg)
{

	// set interrupt line count for FS & FE
	HalVif_Vif2IspLineCnt0(ch, sensorcfg->FrameStartLineCount);
	HalVif_Vif2IspLineCnt1(ch, sensorcfg->FrameEndLineCount);
	return VIF_SUCCESS;
}

int DrvVif_LineCountCfg(VIF_CHANNEL_e ch, unsigned int linecnt0, unsigned int linecnt1)
{

	// set interrupt line count for FS & FE
	HalVif_Vif2IspLineCnt0(ch, linecnt0);
	HalVif_Vif2IspLineCnt1(ch, linecnt1);
	return VIF_SUCCESS;
}
EXPORT_SYMBOL(DrvVif_LineCountCfg);

int DrvVif_SensorReset(VIF_CHANNEL_e ch, int reset)
{
    HalVif_SensorReset(ch, reset);//h0000, bit: 2
	return VIF_SUCCESS;
}

int DrvVif_SensorPdwn(VIF_CHANNEL_e ch, int Pdwn)
{
    if(Pdwn){
        HalVif_SensorSWReset(ch, VIF_DISABLE);//h0000, bit: 0
        HalVif_SensorPowerDown(ch, VIF_ENABLE);//h0000, bit: 3
    }
    else{ //Enable sensor
        HalVif_SensorSWReset(ch, VIF_ENABLE);//h0000, bit: 0
        HalVif_SensorPowerDown(ch, VIF_DISABLE);//h0000, bit: 3
    }
	return VIF_SUCCESS;
}

int DrvVif_ChannelEnable(VIF_CHANNEL_e ch, bool ben)
{
    if(ben){
        HalVif_SensorChannelEnable(ch, VIF_ENABLE);//h0000, bit: 15
    }
    else{ //Enable sensor
        HalVif_SensorChannelEnable(ch, VIF_DISABLE);//h0000, bit: 15
    }
    return VIF_SUCCESS;
}

int DrvVif_SetDefaultIntsMask(void)
{
    u32 ulloop=0;

    for(ulloop = 0; ulloop < VIF_CHANNEL_NUM; ++ulloop){
        HalVif_IrqMask(ulloop, ulgvif_def_mask[ulloop]);
    }

#if 0
    unsigned int irqDefaultMask = VIF_SHIFTBITS(VIF_INTERRUPT_VREG_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_FALLING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_HW_FLASH_STROBE_DONE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE);

    HalVif_IrqMask(VIF_CHANNEL_0, irqDefaultMask);
    HalVif_IrqMask(VIF_CHANNEL_1, irqDefaultMask);
    HalVif_IrqMask(VIF_CHANNEL_2, irqDefaultMask);
#endif

    return VIF_SUCCESS;
}

int DrvVif_SetVifChanelBaseAddr(void)
{
    u32 ulloop=0;

    for(ulloop = 0; ulloop < VIF_CHANNEL_NUM; ++ulloop)
    {
		HalVif_SetVifChanelBaseAddr(ulloop);
    }

#if 0
    unsigned int irqDefaultMask = VIF_SHIFTBITS(VIF_INTERRUPT_VREG_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_VREG_FALLING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_HW_FLASH_STROBE_DONE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE)|
    VIF_SHIFTBITS(VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE);

    HalVif_IrqMask(VIF_CHANNEL_0, irqDefaultMask);
    HalVif_IrqMask(VIF_CHANNEL_1, irqDefaultMask);
    HalVif_IrqMask(VIF_CHANNEL_2, irqDefaultMask);
#endif

    return VIF_SUCCESS;
}

int DrvVif_ConfigParallelIF(VIF_CHANNEL_e ch,
                            VIF_SENSOR_INPUT_FORMAT_e svif_sensor_in_format,
                            VIF_SENSOR_FORMAT_e PixDepth,
                            VIF_CLK_POL PclkPol,
                            VIF_CLK_POL VsyncPol,
                            VIF_CLK_POL HsyncPol,
                            PIN_POL RstPol,
                            u32 CropX,
                            u32 CropY,
                            u32 CropW,
                            u32 CropH,
                            u32 CropEnable,
                            VifPclk_e Mclk
                            )
{
    volatile infinity2_reg_padtop1* padtop = (infinity2_reg_padtop1*) g_TOPPAD1;
    VIF_CHANNEL_SOURCE_e source = VIF_CH_SRC_PARALLEL_SENSOR_0;
    VIF_ONOFF_e mask;

    switch(ch)
    {
    case VIF_CHANNEL_0:
    case VIF_CHANNEL_4:
    case VIF_CHANNEL_8:
    case VIF_CHANNEL_12:
    break;
    default:
        pr_err("[VIF] Invaild channel for parallel interface.\n");
    break;
    }

    mask = HalVif_SensorMask(ch,VIF_DISABLE);
    //Disable HDR as default.
    HalVif_HDRen(ch, VIF_DISABLE);// h0000, bit: 4
    HalVif_HDRSelect(ch, VIF_HDR_SRC_MIPI0);// h0000, bit: 5~7

    HalVif_SensorFormatLeftSht(ch, VIF_DISABLE);// h0006, bit: 3
    HalVif_SensorBitSwap(ch, VIF_DISABLE);// h0006, bit: 7

    //HalVif_SelectSource(ch,src);  //select vif source
    HalVif_SensorFormat(ch,PixDepth); //select pixel depth

    //VIF_SENSOR_POLARITY_HIGH_ACTIVE,
    //VIF_SENSOR_POLARITY_LOW_ACTIVE,
    HalVif_SensorPclkPolarity(ch,PclkPol==VIF_CLK_POL_POS ? VIF_SENSOR_POLARITY_HIGH_ACTIVE : VIF_SENSOR_POLARITY_LOW_ACTIVE);
    HalVif_SensorHsyncPolarity(ch,HsyncPol==VIF_CLK_POL_POS ? VIF_SENSOR_POLARITY_HIGH_ACTIVE : VIF_SENSOR_POLARITY_LOW_ACTIVE);
    //HalVif_SensorVsyncPolarity(ch,VsyncPol==VIF_CLK_POL_POS?VIF_SENSOR_POLARITY_HIGH_ACTIVE:VIF_SENSOR_POLARITY_LOW_ACTIVE);
    HalVif_SensorVsyncPolarity(ch,VsyncPol==VIF_CLK_POL_POS ? VIF_SENSOR_POLARITY_HIGH_ACTIVE: VIF_SENSOR_POLARITY_LOW_ACTIVE); //need to check with designer

    if(VIF_SENSOR_INPUT_FORMAT_RGB == svif_sensor_in_format){ //TBD. Ask designer.
        HalVif_SensorFormatExtMode(ch,VIF_SENSOR_BIT_MODE_1);
    }
    else{
        HalVif_SensorFormatExtMode(ch,VIF_SENSOR_BIT_MODE_0);
    }
    HalVif_SensorRgbIn(ch, svif_sensor_in_format);

    HalVif_PixCropStart(ch,CropX);
    HalVif_PixCropEnd(ch,CropX+CropW-1);
    HalVif_LineCropStart(ch,CropY);
    HalVif_LineCropEnd(ch,CropY+CropH);

    HalVif_CropEnable(ch,CropEnable);

    HalVif_SetPclkSource(ch,PLCK_SENSOR, 0, 0);
    HalVif_SetPclkSource(ch+1,PLCK_SENSOR, 0, 0);
    HalVif_SetPclkSource(ch+2,PLCK_SENSOR, 0, 0);
    HalVif_SetPclkSource(ch+3,PLCK_SENSOR, 0, 0);
    HalVif_SetMCLK(ch,Mclk);

    HalVif_SensorReset(ch,RstPol);
    msleep(1);
    HalVif_SensorReset(ch,~RstPol);

    switch(ch)
    {
    case 0:
        //padtop->reg_ccir0_clk_mode = 1;//PCLK SNR GPIO2
        padtop->reg_snr0_in_mode = 1;
        //padtop->reg_ccir0_ctrl_mode = 1; //PDWN/RST/MCLK
        padtop->reg_snr0_d_ie = 0x03FF;
        padtop->reg_snr0_gpio_drv = 0xFF;
        padtop->reg_snr0_gpio_ie = 0x07;
        source = VIF_CH_SRC_PARALLEL_SENSOR_0;
        break;
    case 4:
        //padtop->reg_ccir1_clk_mode = 1;//PCLK SNR GPIO2
        padtop->reg_snr1_in_mode = 1;
        //padtop->reg_ccir1_ctrl_mode = 1; //PDWN/RST/MCLK
        padtop->reg_snr1_d_ie = 0x03FF;
        padtop->reg_snr1_gpio_drv = 0xFF;
        padtop->reg_snr1_gpio_ie = 0x01;
        source = VIF_CH_SRC_PARALLEL_SENSOR_1;;
        break;
    case 8:
        //padtop->reg_ccir2_clk_mode = 1;//PCLK SNR GPIO2
        padtop->reg_snr2_in_mode = 1;
        //padtop->reg_ccir2_ctrl_mode = 1; //PDWN/RST/MCLK
        padtop->reg_snr2_d_ie = 0x03FF;
        padtop->reg_snr2_gpio_drv = 0x00FF;
        padtop->reg_snr2_gpio_ie = 0x07;
        source = VIF_CH_SRC_PARALLEL_SENSOR_2;
        break;
    case 12:
        //padtop->reg_ccir3_clk_mode = 1;//PCLK SNR GPIO2
        padtop->reg_snr3_in_mode = 1;
        //padtop->reg_ccir3_ctrl_mode = 1; //PDWN/RST/MCLK
        padtop->reg_snr3_d_ie = 0x03FF;
        padtop->reg_snr3_gpio_drv = 0xFF;
        padtop->reg_snr3_gpio_ie = 0x07;
        source = VIF_CH_SRC_PARALLEL_SENSOR_3;
        break;
    default:
        pr_info("[VIF] Invalid channel for parallel sensor.");
        break;
    }
    HalVif_SelectSource(ch,source);
    HalVif_EnableParallelIF(ch,1); //channel enable
    mask = HalVif_SensorMask(ch,mask);
    return 0;
}
EXPORT_SYMBOL(DrvVif_ConfigParallelIF);

int DrvVif_ConfigMipiIF(VIF_CHANNEL_e ch,
                            VIF_SENSOR_INPUT_FORMAT_e svif_sensor_in_format,
                            VIF_SENSOR_FORMAT_e PixDepth,
                            VIF_CLK_POL PclkPol,
                            VIF_CLK_POL VsyncPol,
                            VIF_CLK_POL HsyncPol,
                            PIN_POL RstPol,
                            u32 CropX,
                            u32 CropY,
                            u32 CropW,
                            u32 CropH,
                            u32 CropEnable,
                            VifPclk_e Mclk
                            )
{
    volatile infinity2_reg_padtop1* padtop = (infinity2_reg_padtop1*) g_TOPPAD1;
    u8 group = (ch >> 2);

    HalVif_SensorFormat(ch,PixDepth); //select pixel depth

    HalVif_SensorRgbIn(ch, svif_sensor_in_format);

    HalVif_PixCropStart(ch,CropX);
    HalVif_PixCropEnd(ch,CropX+CropW-1);
    HalVif_LineCropStart(ch,CropY);
    HalVif_LineCropEnd(ch,CropY+CropH-1);

    HalVif_CropEnable(ch,CropEnable);

    HalVif_SetMCLK(ch,Mclk);

    HalVif_SelectSource(ch, group);

    HalVif_SetPclkSource(ch, group, 1, 0);
    HalVif_SetPclkSource(ch+1, group, 1, 0);
    HalVif_SetPclkSource(ch+2, group, 1, 0);
    HalVif_SetPclkSource(ch+3, group, 1, 0);

    //padtop->reg_ccir0_ctrl_mode = 1; //PDWN/RST/MCLK

    switch(group)
    {
    case 0:
        padtop->reg_snr0_d_ie = 0x0;
        break;
    case 1:
        padtop->reg_snr1_d_ie = 0x0;
        break;
    case 2:
        padtop->reg_snr2_d_ie = 0x0;
        break;
    case 3:
        padtop->reg_snr3_d_ie = 0x0;
        break;
    default:
        pr_info("[VIF] Invalid channel for mipi sensor.");
        break;
    }

    HalVif_SensorReset(ch,RstPol);
    msleep(1); // ToDo?
    HalVif_SensorReset(ch,~RstPol);

    return 0;
}


#if 0
int DrvVif_ConfigMipiRX(u32 MipiPort,
                        u32 Lans,
                        u32 Speed
                       )
{

}
#endif
int DrvVif_ConfigBT656IF(SENSOR_PAD_GROUP_e pad_group, VIF_CHANNEL_e ch, VifBT656Cfg_t *pvif_bt656_cfg)
{
    int ret = VIF_SUCCESS;

    HalVif_BT656InputSelect(ch, pad_group);
    HalVif_BT656ChannelDetectEnable(ch, pvif_bt656_cfg->bt656_ch_det_en);
    HalVif_BT656ChannelDetectSelect(ch, pvif_bt656_cfg->bt656_ch_det_sel);
    HalVif_BT656BitSwap(ch, pvif_bt656_cfg->bt656_bit_swap);
#if 0
    HalVif_BT6568BitMode(ch, pvif_bt656_cfg->bt656_8bit_mode);
    switch(pad_group){ /*Note: It is sensor pad not VIF channel.*/
        case SENSOR_PAD_GROUP_A:
            HalVif_BT6568BitExt(ch, 1);
            break;
        case SENSOR_PAD_GROUP_B:
            HalVif_BT6568BitExt(ch, 0);
            break;
        default:
            //Error
            break;
    }
#endif
    HalVif_BT656VSDelay(ch, pvif_bt656_cfg->bt656_vsync_delay);
    HalVif_BT656HsyncInvert(ch,  pvif_bt656_cfg->bt656_hsync_inv);
    HalVif_BT656VsyncInvert(ch, pvif_bt656_cfg->bt656_vsync_inv);
    HalVif_BT656ClampEnable(ch, pvif_bt656_cfg->bt656_clamp_en);
    HalVif_BT656VerticalCropSize(ch,  0); //Do not modify it.
    HalVif_BT656HorizontalCropSize(ch, 0x0fff);  //Do not modify it.

    return ret;
}

int DrvVif_SetChannelSource(VIF_CHANNEL_e ch, VIF_CHANNEL_SOURCE_e svif_ch_src)
{
    int ret = VIF_SUCCESS;

    HalVif_SelectSource(ch, svif_ch_src);  //select vif source
    return ret;
}

int DrvVif_MCULoadBin(void)
{
    /* Move fw to MCU SRAM */
    REG_W(g_BDMA,0x02,0x0630); //02 SRC:SPI  DST:51 psram
    REG_W(g_BDMA,0x04,(IPCRamPhys & 0xFFFF)); //04 SRC start l addr
    REG_W(g_BDMA,0x05,((IPCRamPhys & 0x1FFFFFFF)>>16)&0xFFFF); //05 SRC start h addr
    REG_W(g_BDMA,0x06,0x0000); //06 DST start l addr
    REG_W(g_BDMA,0x07,0x0000); //07 DST start h addr
    REG_W(g_BDMA,0x08,0x8000); //08 DMA size 16kB   , can change size here
    REG_W(g_BDMA,0x09,0x0000); //09 DMA size
    REG_W(g_BDMA,0x03,0x0000); //09 DMA size

    DrvVif_msleep(20); //wait DMA done

    REG_W(g_BDMA,0x00,0x0001); //00 trigger DMA

    DrvVif_msleep(500); //wait DMA done
    REG_W(g_BDMA,0x01,0x0008); //01 write [3] to clear DMA done

    /* MCU51 boot from SRAM */
    REG_W(g_MCU8051,0x0C,0x0001); //0C
    REG_W(g_MCU8051,0x00,0x0000); //00
    REG_W(g_MCU8051,0x01,0x0000); //01
    REG_W(g_MCU8051,0x02,0x0000); //02
    REG_W(g_MCU8051,0x03,0xFBFF); //03

    DrvVif_msleep(500); //wait DMA done
    REG_W(g_PMSLEEP,0x29,0x9003); //29
    return VIF_SUCCESS;
}

int DrvVif_DmaEnable(VIF_CHANNEL_e ch,unsigned char en)
{
    HalRawDma_GlobalEnable();
    HalRawDma_GroupReset(ch/4);
    HalRawDma_GroupEnable(ch/4);
    //DrvVif_EnableInterrupt(ch,1);
    return VIF_SUCCESS;
}
EXPORT_SYMBOL(DrvVif_DmaEnable);

int DrvVif_RawStore(VIF_CHANNEL_e ch,u32 uMiuBase,u32 ImgW,u32 ImgH)
{
    WdmaCropParam_t tCrop = {0,0,ImgW,ImgH};
    pr_info("CH=%d, DMA Base = 0x%X\n",ch,uMiuBase);
    //DrvVif_SetDmagMLineCntInt(ch,1);
    //DrvVif_SetDmagMLineCntInt(ch,ImgH-1);
    HalRawDma_Config(ch,&tCrop,uMiuBase);
    HalRawDma_Trigger(ch,WDMA_TRIG_SINGLE);
    return VIF_SUCCESS;
}
EXPORT_SYMBOL(DrvVif_RawStore);

int DrvVif_InputMask(int ch,int OnOff)
{
    return HalVif_SensorMask(ch,OnOff);
}
EXPORT_SYMBOL(DrvVif_InputMask);

s32 DrvVif_Reset(void)
{
    return true;
}

int DrvVif_EnableInterrupt(VIF_CHANNEL_e ch, unsigned char en)
{
    if(en){
        HalVif_IrqMask(ch, 0xFF);
        HalVif_IrqClr1(ch, 0xFF);
        HalVif_IrqUnMask(ch, 0xFF);
    }else{
        HalVif_IrqMask(ch, 0xFF);
    }
    return true;
}
EXPORT_SYMBOL(DrvVif_EnableInterrupt);

int DrvVif_SetDmagMLineCntInt(VIF_CHANNEL_e ch, unsigned char en)
{
    if(en){
        HalDma_DoneIrqMask(ch, 0x1);
        HalDma_DoneIrqClr(ch, 0x1);
        HalDma_DoneIrqUnMask(ch, 0x1);
    }else{
        HalDma_DoneIrqMask(ch, 0x1);
    }
    return true;
}
EXPORT_SYMBOL(DrvVif_SetDmagMLineCntInt);

int DrvVif_SetDmagMLineCnt(VIF_CHANNEL_e ch, unsigned int cnt)
{

    HalDma_SetDmagMLineCnt(ch, cnt);

    return true;
}
EXPORT_SYMBOL(DrvVif_SetDmagMLineCnt);
