#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <asm/io.h>
#include <linux/clk-provider.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <ms_platform.h>
#include <asm/string.h>
#include <asm/io.h>
#include "vif_datatype.h"
#include <vif_common.h>

#include <hal_dma_private.h>
#include "hal_dma.h"

#define REG_W(base,offset,val) ((*(unsigned short*)(((unsigned int)base)+ (offset*4)))=(val))
#define REG_R(base,offset) *((unsigned short*)((unsigned int)base+(offset*4)))
VdmaHandle_t gstHandle;
void HalDma_Init()
{
	//io remap
	int n;
	VdmaHandle_t *pHnd = &gstHandle;
	//TODO: ioremap
	//unsigned long uRegBase = 0xFD000000;
	unsigned long uRegBase = (unsigned long)ioremap(0x1F000000,0x400000);

	for(n=0;n<16;++n)
	{
		pHnd->Dma[n].pstYDmaMain = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1420+n,0x00));
		pHnd->Dma[n].pstCDmaMain = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1420+n,0x20));
		pHnd->Dma[n].pstYDmaSub  = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1420+n,0x40));
		pHnd->Dma[n].pstCDmaSub  = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1420+n,0x60));
	}

	//special case ch0 main
	pHnd->Dma[0].pstYDmaMain = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1517,0x20));
	pHnd->Dma[0].pstCDmaMain = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1420,0x20));
	//special case ch0 sub
	pHnd->Dma[0].pstYDmaSub = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1516,0x00));
	pHnd->Dma[0].pstCDmaSub = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1516,0x20));

	//special case ch4 Y main
	pHnd->Dma[4].pstYDmaMain = (infinity2_isp_wdma*)(uRegBase + CPU_RIU_REG(0x1517,0x60));

	//422 to 420 parameter
	pHnd->Dma[0].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1421,0x14));
	pHnd->Dma[1].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1421,0x34));
	pHnd->Dma[2].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1421,0x54));
	pHnd->Dma[3].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1421,0x74));

	pHnd->Dma[4].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1424,0x14));
	pHnd->Dma[5].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1424,0x34));
	pHnd->Dma[6].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1424,0x54));
	pHnd->Dma[7].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1424,0x74));

	pHnd->Dma[8].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1428,0x14));
	pHnd->Dma[9].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1428,0x34));
	pHnd->Dma[10].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1428,0x54));
	pHnd->Dma[11].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x1428,0x74));

	pHnd->Dma[12].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x142C,0x14));
	pHnd->Dma[13].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x142C,0x34));
	pHnd->Dma[14].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x142C,0x54));
	pHnd->Dma[15].pst422to420Ctl = (infinity2_isp_wdma_yuv_ctrl*)(uRegBase + CPU_RIU_REG(0x142C,0x74));

	//mapping other register
	pHnd->irq = (infinity2_reg_isp_dma_irq*) (uRegBase + CPU_RIU_REG(0x153B,0x00));
	pHnd->isp0 = (infinity2_reg_isp0*) (uRegBase + CPU_RIU_REG(0x1509,0x00));
	pHnd->isp11 = (infinity2_reg_isp11*) (uRegBase + CPU_RIU_REG(0x1514,0x00));
	pHnd->isp12 = (infinity2_reg_isp12*) (uRegBase + CPU_RIU_REG(0x1516,0x00));
}

void HalDma_Uninit()
{

}

int HalDma_GlobalEnable()
{
	VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
	//sw reset
	pstHnd->isp0->reg_en_isp = 0;
	pstHnd->isp0->reg_isp_sw_rstz = 0;
	pstHnd->isp0->reg_isp_sw_p1_rstz = 0;
	pstHnd->isp0->reg_isp_sw_p2_rstz = 0;
	pstHnd->isp0->reg_isp_sw_p3_rstz = 0;

	//pstHnd->isp0->reg_isp_fifo_mode = 0; //1024 * 4 , for 4 ch mode
    //pstHnd->isp11->reg_isp_fifo_mode_sel = 0; //1024 * 4 , for 4 ch mode
    //pstHnd->isp12->reg_isp_fifo_mode_sel = 0; //1024 * 4 , for 4 ch mode

	pstHnd->isp0->reg_42to40_store_en = 1;
	pstHnd->isp0->reg_data_store_src_sel = 0;

	pstHnd->isp0->reg_isp_sw_p1_rstz = 1;
	pstHnd->isp0->reg_isp_sw_p2_rstz = 1;
	pstHnd->isp0->reg_isp_sw_p3_rstz = 1;
	pstHnd->isp0->reg_isp_sw_rstz = 1;
	pstHnd->isp0->reg_en_isp = 1;
	return 0;
}

int HalDma_EnableGroup(u32 uGroup)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    switch(uGroup)
    {
    case 0:
        pstHnd->isp0->reg_p0h_rawstore_mode = 0;
        pstHnd->isp0->reg_42to40_store_en = 1;//422 to 420 store enable
    break;
    case 1:
        pstHnd->isp0->reg_p1h_rawstore_mode = 0;
        pstHnd->isp11->reg_42to40_store_en = 1;//422 to 420 store enable
    break;
    case 2:
        pstHnd->isp0->reg_p2h_rawstore_mode = 0;
    break;
    case 3:
        pstHnd->isp0->reg_p3h_rawstore_mode = 0;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

int HalDma_DisableGroup(u32 uGroup)
{
/*
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    switch(uGroup)
    {
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
    break;
    }
*/
    return 0;
}

int HalDma_ConfigGroup(u32 uGroup,u32 uMaxChns)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    u8 uBufMode = 0; //default 4 x line buffer
    unsigned short reg = 0;

    switch(uMaxChns)
    {
    case 4:
        uBufMode = 0; //1280x4

        if(uGroup == 0){
            pstHnd->isp0->reg_isp_fifo_mode = 0; //1024 * 4 , for 4 ch mode
        }else if(uGroup == 1){
            pstHnd->isp11->reg_isp_fifo_mode_sel = 0; //1024 * 4 , for 4 ch mode
        }else if(uGroup == 2){
            reg = REG_R(pstHnd->isp12,0x70);
            reg &= ~(0x6000);
            reg |= 0<<13;
            REG_W(pstHnd->isp12,0x70,reg); //1024 * 4 , for 4 ch mode
        }else if(uGroup == 3){
            reg = REG_R(pstHnd->isp12,0x60);
            reg &= ~(0x6000);
            reg |= 0<<13;
            REG_W(pstHnd->isp12,0x60,reg); //1024 * 4 , for 4 ch mode
        }else{
            pr_err("Exceed max dev number.");
        }
    break;
    case 2:
        uBufMode = 1; //1920x2
        if(uGroup == 0){
            pstHnd->isp0->reg_isp_fifo_mode = 1; //2048 * 2 , for 2 ch mode
        }else if(uGroup == 1){
            pstHnd->isp11->reg_isp_fifo_mode_sel = 1; //2048*2 , for 2 ch mode
        }else if(uGroup == 2){
            reg = REG_R(pstHnd->isp12,0x70);
            reg &= ~(0x6000);
            reg |= 1<<13;
            REG_W(pstHnd->isp12,0x70,reg); //2048 * 2 , for 2 ch mode
        }else if(uGroup == 3){
            reg = REG_R(pstHnd->isp12,0x60);
            reg &= ~(0x6000);
            reg |= 1<<13;
            REG_W(pstHnd->isp12,0x60,reg); //2048 * 2 , for 2 ch mode
        }else{
            pr_err("Exceed max dev number.");
        }{
            pr_err("Exceed max dev number.");
        }
    break;
    case 1:
        uBufMode = 2; //4096x1
        if(uGroup == 0){
            pstHnd->isp0->reg_isp_fifo_mode = 2; //4096*1, for 1 ch mode
        }else if(uGroup == 1){
            pstHnd->isp11->reg_isp_fifo_mode_sel = 2; //4096*1 , for 1 ch mode
        }else if(uGroup == 2){
            reg = REG_R(pstHnd->isp12,0x70);
            reg &= ~(0x6000);
            reg |= 2<<13;
            REG_W(pstHnd->isp12,0x70,reg); //4096*1, for 1 ch mode
        }else if(uGroup == 3){
            reg = REG_R(pstHnd->isp12,0x60);
            reg &= ~(0x6000);
            reg |= 2<<13;
            REG_W(pstHnd->isp12,0x60,reg); //4096*1, for 1 ch mode
        }{
            pr_err("Exceed max dev number.");
        }
    break;
    default:
        uBufMode = 0; //1280x4
        if(uGroup == 0){
            pstHnd->isp0->reg_isp_fifo_mode = 0; //1024 * 4 , for 4 ch mode
        }else if(uGroup == 1){
            pstHnd->isp11->reg_isp_fifo_mode_sel = 0; //1024 * 4 , for 4 ch mode
        }else if(uGroup == 2){
            reg = REG_R(pstHnd->isp12,0x70);
            reg &= ~(0x6000);
            reg |= 0<<13;
            REG_W(pstHnd->isp12,0x70,reg); //1024 * 4 , for 4 ch mode
        }else if(uGroup == 3){
            reg = REG_R(pstHnd->isp12,0x60);
            reg &= ~(0x6000);
            reg |= 0<<13;
            REG_W(pstHnd->isp12,0x60,reg); //1024 * 4 , for 4 ch mode
        }{
            pr_err("Exceed max dev number.");
        }
    break;
    }

    if(uGroup<4)
    {
        pstHnd->Dma[uGroup*4].pst422to420Ctl->reg_isp_lb_mode = uBufMode;
    }
    else
    {
        pr_err("Exceed max channel number.");
        return -1;
    }
    return 0;
}

int HalDma_Config(u32 uCh,WdmaCropParam_t *ptCrop,u32 uMiuBaseY,u32 uMiuBaseC)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    volatile infinity2_isp_wdma_yuv_ctrl *yuv_ctrl = pstHnd->Dma[uCh].pst422to420Ctl;

    //Y dma setting
    volatile infinity2_isp_wdma *dma = pstHnd->Dma[uCh].pstYDmaMain;
    dma->reg_isp_wdma_mode = 0; //8bit mode
    dma->reg_isp_wdma_pitch = (ptCrop->uW+31)/32;
    dma->reg_isp_wdma_width_m1 = ptCrop->uW - 1;
    dma->reg_isp_wdma_height_m1 = ptCrop->uH - 1;
    dma->reg_isp_wdma_base = (uMiuBaseY>>5)&0xFFFF;
    dma->reg_isp_wdma_base_1 = (uMiuBaseY>>21)&0xFFFF;
    dma->reg_isp_wdma_align_en = 1; //auto align mode

    dma = pstHnd->Dma[uCh].pstCDmaMain;
    dma->reg_isp_wdma_mode = 0; //8bit mode
    dma->reg_isp_wdma_pitch = (ptCrop->uW+31)/32;
    dma->reg_isp_wdma_width_m1 = ptCrop->uW - 1;
    dma->reg_isp_wdma_height_m1 = ptCrop->uH/2 - 1;
    dma->reg_isp_wdma_base = (uMiuBaseC>>5)&0xFFFF;
    dma->reg_isp_wdma_base_1 = (uMiuBaseC>>21)&0xFFFF;
    dma->reg_isp_wdma_align_en = 1; //auto align mode

    //YUV422 to YUV420 converter
    yuv_ctrl->reg_isp_422to420_c_ratio = 0;
    //yuv_ctrl->reg_isp_align_num = 2;
    return 0;
}

int HalDma_ConfigSub(u32 uCh,WdmaCropParam_t *ptCrop,u32 uMiuBaseY,u32 uMiuBaseC)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

    //Y dma setting
    volatile infinity2_isp_wdma *dma = pstHnd->Dma[uCh].pstYDmaSub;
    dma->reg_isp_wdma_mode = 0; //8bit mode
    dma->reg_isp_wdma_pitch = (ptCrop->uW+31)/32;
    dma->reg_isp_wdma_width_m1 = ptCrop->uW - 1;
    dma->reg_isp_wdma_height_m1 = ptCrop->uH - 1;
    dma->reg_isp_wdma_base = (uMiuBaseY>>5)&0xFFFF;
    dma->reg_isp_wdma_base_1 = (uMiuBaseY>>21)&0xFFFF;
    dma->reg_isp_wdma_align_en = 1; //auto align mode

    dma->reg_pix4_mode = 0; 	//1 pixel mode
    dma->reg_bayer_fmt = 0; 	//non bayer mode
    dma->reg_420dn_en = 1; 		//enable 420 down-sample mode
    dma->reg_420dn_yc_sel = 0; 	//420 down-sample for y
    dma->reg_bits_per_rdy = 0;	//bits
    dma->reg_dn_mode = 1; 		//1/2 W and H
    dma->reg_src_width = ptCrop->uW*2;	//origin size
    dma->reg_src_height = ptCrop->uH*2;	//origin size
    dma->reg_420dn_y_w0 = 0;
    dma->reg_420dn_y_w1 = 16;
    dma->reg_420dn_y_w2 = 16;
    dma->reg_420dn_y_w3 = 0;
    dma->reg_420dn_y_w3_s = 0;
    dma->reg_420dn_y_sft = 5; //total weight 2^y_sft


    //C dma setting
    dma = pstHnd->Dma[uCh].pstCDmaSub;
    dma->reg_isp_wdma_mode = 0; //8bit mode
    dma->reg_isp_wdma_pitch = (ptCrop->uW+31)/32;
    dma->reg_isp_wdma_width_m1 = ptCrop->uW - 1;
    dma->reg_isp_wdma_height_m1 = ptCrop->uH/2 - 1;
    dma->reg_isp_wdma_base = (uMiuBaseC>>5)&0xFFFF;
    dma->reg_isp_wdma_base_1 = (uMiuBaseC>>21)&0xFFFF;
    dma->reg_isp_wdma_align_en = 1; //auto align mode

    dma->reg_pix4_mode = 0; 	//1 pixel mode
    dma->reg_bayer_fmt = 0; 	//non bayer mode
    dma->reg_420dn_en = 1; 		//enable 420 down-sample mode
    dma->reg_420dn_yc_sel = 1; 	//420 down-sample mode
    dma->reg_bits_per_rdy = 0;	//bits
    dma->reg_dn_mode = 1; 		//1/2 W and H
    dma->reg_420dn_uv_mode = 2;	//2 pixel average
    dma->reg_src_width = ptCrop->uW*2;	//origin size
    dma->reg_src_height = ptCrop->uH;	//origin size
    //dma->reg_420dn_y_w0 = 0;
    //dma->reg_420dn_y_w1 = 16;
    //dma->reg_420dn_y_w2 = 16;
    //dma->reg_420dn_y_w3 = 0;
    //dma->reg_420dn_y_w3_s = 0;
    //dma->reg_420dn_y_sft = 5; //total weight 2^y_sft

    return 0;
}

int HalDma_Trigger(u32 uCh,WdmaTrigMode_e eMode)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    //infinity2_isp_wdma *dma = pstHnd->Dma[uCh].pstYDmaSub;
    if(uCh>=16)
    {
        BUG();
    }
    switch(eMode)
    {
    case WDMA_TRIG_SINGLE:
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger = 1;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_en = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_en = 1;
    break;
    case WDMA_TRIG_CONTINUE:
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_auto = 1;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_en = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_auto = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_en = 1;
    break;
    case WDMA_TRIG_STOP:
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_en = 0;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_trigger_mode = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_en = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_trigger_mode = 0;
    break;
    default:
    break;
    }
    return 0;
}

int HalDma_TriggerSub(u32 uCh,WdmaTrigMode_e eMode)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

    if(uCh>=16)
    {
        BUG();
    }

    switch(eMode)
    {
    case WDMA_TRIG_SINGLE:
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger = 1;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_en = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_en = 1;
    break;
    case WDMA_TRIG_CONTINUE:
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_auto = 1;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_en = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_auto = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger_mode = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_en = 1;
    break;
    case WDMA_TRIG_STOP:
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_en = 0;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_trigger_mode = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_en = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_auto = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_trigger_mode = 0;
    break;
    default:
    break;
    }
    return 0;
}

int HalDma_EnableIrq(u32 nCh)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    pstHnd->irq->reg_c_irq_mask3 &= ~(0x1<<nCh); //sub ch
    pstHnd->irq->reg_c_irq_mask4 &= ~(0x1<<nCh); //sub ch
/*
    pstHnd->irq->reg_c_irq_mask2 &= ~(0x1<<nCh);



    if(nCh % 3  == 0)
        pstHnd->irq->reg_c_irq_mask5 = 0; //group 0 line cnt
    else if(nCh % 3 == 1 )
        pstHnd->irq->reg_c_irq_mask6 = 0; //group 1 line cnt

    else if(nCh % 3 == 2)
        pstHnd->irq->reg_c_irq_mask7 = 0; //group 2 line cnt

    else if(nCh % 3 == 3)
        pstHnd->irq->reg_c_irq_mask8 = 0; //group 3 line cnt
    else
        VIF_DEBUG("%s , channel not found\n",__FUNCTION__);
*/
    pstHnd->Dma[nCh].pstYDmaMain->reg_isp_wdma_irqlinex = 16;
    pstHnd->Dma[nCh].pstCDmaMain->reg_isp_wdma_irqlinex = 16;
    pstHnd->Dma[nCh].pstYDmaSub->reg_isp_wdma_irqlinex = 16;
    pstHnd->Dma[nCh].pstCDmaSub->reg_isp_wdma_irqlinex = 16;


    pstHnd->Dma[nCh].pstYDmaMain->reg_isp_wdma_mask_out = 1;
    pstHnd->Dma[nCh].pstCDmaMain->reg_isp_wdma_mask_out = 1;
    pstHnd->Dma[nCh].pstYDmaSub->reg_isp_wdma_mask_out = 1;
    pstHnd->Dma[nCh].pstCDmaSub->reg_isp_wdma_mask_out = 1;

    return 0;
}

int HalDma_DmaMaskEnable(u32 nCh,u8 enable){

    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

    pstHnd->Dma[nCh].pstYDmaMain->reg_isp_wdma_mask_out = enable;
    pstHnd->Dma[nCh].pstCDmaMain->reg_isp_wdma_mask_out = enable;
    pstHnd->Dma[nCh].pstYDmaSub->reg_isp_wdma_mask_out = enable;
    pstHnd->Dma[nCh].pstCDmaSub->reg_isp_wdma_mask_out = enable;

    return 0;
}

int HalDma_DisableIrq(u32 nCh)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    pstHnd->irq->reg_c_irq_mask3 |= (0x1<<nCh); //sub ch
    pstHnd->irq->reg_c_irq_mask4 |= (0x1<<nCh); //sub ch
    return 0;
}

//uMask=1:Disable DMA output to MIU
//uMask=1:Enable DMA output to MIU
int HalDma_MaskOutput(u32 uCh,u8 uMask)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    if(uMask)
    {
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_mask_out = 1;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_mask_out = 1;
    }
    else
    {
        pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_mask_out = 0;
        pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_mask_out = 0;
    }
    return 0;
}

int HalDma_MaskOutputSub(u32 uCh,u8 uMask)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
    if(uMask)
    {
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_mask_out = 1;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_mask_out = 1;
    }
    else
    {
        pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_mask_out = 0;
        pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_mask_out = 0;
    }
    return 0;
}

int HalDma_SetOutputAddr(u32 uCh,u32 uOutAddrY,u32 uOutAddrC,u32 uPitch)
{
	VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
	pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_base = (uOutAddrY>>5)&0xFFFF;
	pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_base = (uOutAddrY>>21)&0xFFFF;
	pstHnd->Dma[uCh].pstYDmaMain->reg_isp_wdma_pitch = (uPitch+31)/32;
	pstHnd->Dma[uCh].pstCDmaMain->reg_isp_wdma_pitch = (uPitch+31)/32;
	return 0;
}

int HalDma_SetOutputAddrSub(u32 uCh,u32 uOutAddrY,u32 uOutAddrC,u32 uPitch)
{
	VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;
	pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_base = (uOutAddrY>>5)&0xFFFF;
	pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_base = (uOutAddrY>>21)&0xFFFF;
	pstHnd->Dma[uCh].pstYDmaSub->reg_isp_wdma_pitch = (uPitch+31)/32;
	pstHnd->Dma[uCh].pstCDmaSub->reg_isp_wdma_pitch = (uPitch+31)/32;
	return 0;
}


unsigned int HalDma_DoneIrqFinalStatus(VIF_CHANNEL_e ch)
{
	unsigned int u32IntFinalStatus = 0;
    infinity2_reg_isp_dma_irq *pvif_handle = (infinity2_reg_isp_dma_irq *)g_DMA_IRQ;
	ch %= 2;

	switch(ch)
	{
		case VIF_CHANNEL_0:
			u32IntFinalStatus = pvif_handle->reg_irq_final_status5;
			break;
		case VIF_CHANNEL_1:
			u32IntFinalStatus = pvif_handle->reg_irq_final_status5; //ToDo
			break;
		default:
			pr_info("[%s] err, over VIF chanel number \n", __func__);
	}

	return u32IntFinalStatus;
}


void HalDma_DoneIrqMask(VIF_CHANNEL_e ch, unsigned int mask)
{
    infinity2_reg_isp_dma_irq *pvif_handle = (infinity2_reg_isp_dma_irq *)g_DMA_IRQ;
	ch %= 2;

	switch(ch)
	{
		case VIF_CHANNEL_0:
			pvif_handle->reg_c_irq_mask5 |= mask;
			break;
		case VIF_CHANNEL_1:
			pvif_handle->reg_c_irq_mask5 |= mask; //ToDo
			break;
		default:
			pr_info("[%s] err, over VIF chanel number \n", __func__);
	}
}

void HalDma_DoneIrqUnMask(VIF_CHANNEL_e ch, unsigned int mask)
{
    infinity2_reg_isp_dma_irq *pvif_handle = (infinity2_reg_isp_dma_irq *)g_DMA_IRQ;
	ch %= 2;

	switch(ch)
	{
		case VIF_CHANNEL_0:
			pvif_handle->reg_c_irq_mask5 &= (~mask);
			break;
		case VIF_CHANNEL_1:
			pvif_handle->reg_c_irq_mask5 &= (~mask); //ToDo
			break;
		default:
			pr_info("[%s] err, over VIF chanel number \n", __func__);
	}
}

void HalDma_DoneIrqClr(VIF_CHANNEL_e ch, unsigned int mask)
{
    infinity2_reg_isp_dma_irq *pvif_handle = (infinity2_reg_isp_dma_irq *)g_DMA_IRQ;
	ch %= 2;

	switch(ch)
	{
		case VIF_CHANNEL_0:
            pvif_handle->reg_c_irq_clr5 |= mask;
			pvif_handle->reg_c_irq_clr5 &= (~mask);
			break;
		case VIF_CHANNEL_1:
            pvif_handle->reg_c_irq_clr5 |= mask; //ToDo
			pvif_handle->reg_c_irq_clr5 &= (~mask);

			break;
		default:
			pr_info("[%s] err, over VIF chanel number \n", __func__);
	}
}

void HalDma_SetDmagMLineCnt(VIF_CHANNEL_e ch, unsigned int cnt)
{
    VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

    pstHnd->Dma[ch].pstYDmaMain->reg_isp_wdma_irqlinex = cnt;

}

void HalDma_GetDmaInfo(VIF_CHANNEL_e ch,WdmaInfo_t *info)
{
	VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

	info->uWidth = pstHnd->Dma[ch].pstYDmaMain->reg_isp_wdma_width_m1;
	info->uHeight = pstHnd->Dma[ch].pstYDmaMain->reg_isp_wdma_height_m1;

}

void HalDma_GetSubDmaInfo(VIF_CHANNEL_e ch,WdmaInfo_t *info)
{
	VdmaHandle_t *pstHnd = (VdmaHandle_t*) &gstHandle;

	info->uWidth = pstHnd->Dma[ch].pstYDmaSub->reg_isp_wdma_width_m1;
	info->uHeight = pstHnd->Dma[ch].pstYDmaSub->reg_isp_wdma_height_m1;

}
