//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#define DRV_HVSP_C


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"

#include "drv_scl_hvsp_st.h"
#include "hal_scl_hvsp.h"
#include "drv_scl_hvsp.h"
#include "drv_scl_dma_st.h"
#include "drv_scl_irq_st.h"
#include "drv_scl_irq.h"
#include "hal_scl_reg.h"
#include "drv_scl_ctx_m.h"
#include "drv_scl_ctx_st.h"

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_HVSP_DBG(x)
#define DRV_HVSP_ERR(x)      x
#define DRV_HVSP_MUTEX_LOCK()            DrvSclOsObtainMutex(_HVSP_Mutex,SCLOS_WAIT_FOREVER)
#define DRV_HVSP_MUTEX_UNLOCK()          DrvSclOsReleaseMutex(_HVSP_Mutex)
#define FHD_Width   1920
#define FHD_Height  1080
#define _3M_Width   2048
#define _3M_Height  1536
#define HD_Width    1280
#define HD_Height   720
#define D_Width    704
#define D_Height   576
#define PNL_Width   800
#define PNL_Height  480
#define SRAMFORSCALDOWN 0x10
#define SRAMFORSCALUP 0x21
#define SRAMFORSC2ALDOWN 0x20
#define SRAMFORSC2ALUP 0x21
#define SRAMFORSC3HDOWN 0x00
#define SRAMFORSC3HUP 0x21
#define SRAMFORSC3VDOWN 0x00
#define SRAMFORSC3VUP 0x21
#define SRAMFORSC4ALDOWN 0x10
#define SRAMFORSC4ALUP 0x21
#define HeightRange (gstGlobalHvspSet->gstSrcSize.u16Height ? ((gstGlobalHvspSet->gstSrcSize.u16Height+1)/20) : (FHD_Height/20))
#define Is_StartAndEndInSameByte(u16XStartoffsetbit,u16idx,u16Xoffset) \
    (u16XStartoffsetbit && (u16idx ==0) && (u16Xoffset==1))
#define Is_StartByteOffsetBit(u16XStartoffsetbit,u16idx) \
        (u16XStartoffsetbit && (u16idx ==0))
#define Is_EndByteOffsetBit(u16XEndoffsetbit,u16Xoffset,u16idx) \
    (u16XEndoffsetbit && (u16idx ==(u16Xoffset-1)))
#define Is_DNRBufferReady() (DrvSclOsGetSclFrameBufferAlloced())
#define Is_FrameBufferTooSmall(u32ReqMemSize,u32IPMMemSize) (u32ReqMemSize > u32IPMMemSize)
#define Is_PreCropWidthNeedToOpen() (gstGlobalHvspSet->gstSrcSize.u16Width > gstGlobalHvspSet->gstIPMCfg.u16Fetch)
#define Is_PreCropHeightNeedToOpen() (gstGlobalHvspSet->gstSrcSize.u16Height> gstGlobalHvspSet->gstIPMCfg.u16Vsize)
#define Is_InputSrcRotate() (gstGlobalHvspSet->gstSrcSize.u16Height > gstGlobalHvspSet->gstSrcSize.u16Width)
#define Is_IPM_NotSetReady() (gstGlobalHvspSet->gstIPMCfg.u16Fetch == 0 || gstGlobalHvspSet->gstIPMCfg.u16Vsize == 0)
#define Is_INPUTMUX_SetReady() (gstGlobalHvspSet->gstSrcSize.bSet == 1)
#define Is_PreCropNotNeedToOpen() ((gstGlobalHvspSet->gstIPMCfg.u16Fetch == gstGlobalHvspSet->gstSrcSize.u16Width) && \
    (gstGlobalHvspSet->gstIPMCfg.u16Vsize == gstGlobalHvspSet->gstSrcSize.u16Height))
#define HVSP_ID_SRAM_V_OFFSET 2
#define HVSP_ID_SRAM_H_OFFSET 16
#define HVSP_RATIO(input, output)           ((u32)((u64)(((u64)input) * 1048576) / (u32)(output)))
#define HVSP_CROP_RATIO(u16src, u16crop1, u16crop2)  ((u16)(((u32)u16crop2 * (u32)u16crop1) / (u32)u16src ))
#define HVSP_CROP_CHECK(u16croph,u16cropch,u16cropv,u16cropcv)  (((u16croph) != (u16cropch))|| ((u16cropv) != (u16cropcv)))
#define HVSP_DMA_CHECK(u16cropv,u16cropcv)  (((u16cropv) != (u16cropcv)))
#define _CHANGE_SRAM_V_QMAP(enHVSP_ID,up)         (((gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    > gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)&&(up)) || (((gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height\
    < gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height) && !(up))))
#define _CHANGE_SRAM_H_QMAP(enHVSP_ID,up)         (((gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width\
        > gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width)&&(up)) || (((gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width\
        < gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width) && !(up))))

#define _IsSingleBufferAndZoomSizeBiggerthanLimitation(u8buffernum,cropV,DspV) (((u8buffernum)==1) && \
    ((DspV) > (cropV)))
#define _IsSingleBufferAndNOLDCZoomSizeBiggerthanLimitation(bnoLDC,u8buffernum,cropV,DspV) ((bnoLDC)&&((u8buffernum)==1) && \
    ((DspV) > (cropV)))
#define _Is_Src5MSize() ((gstGlobalHvspSet->gstSrcSize.u16Height*gstGlobalHvspSet->gstSrcSize.u16Width) >= 4500000 &&(gstGlobalHvspSet->gstSrcSize.bSet))
#define _IsChangeFBBufferResolution(u16Width ,u16Height) ((u16Width !=gstGlobalHvspSet->gstIPMCfg.u16Fetch)||(u16Height !=gstGlobalHvspSet->gstIPMCfg.u16Vsize))
#define _IsChangePreCropPosition(u16X ,u16Y ,u16oriX ,u16oriY ) (((u16X !=u16oriX)||(u16Y !=u16oriY)))
#define _IsZoomOut(u16Width ,u16Height) ((u16Width < gstGlobalHvspSet->gstIPMCfg.u16Fetch)||(u16Height < gstGlobalHvspSet->gstIPMCfg.u16Vsize))

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MDrvSclCtxHvspGlobalSet_t *gstGlobalHvspSet;

//keep
s32 _HVSP_Mutex = -1;
/////////////////
/// gbHvspSuspend
/// To Save suspend status.
////////////////
bool gbHvspSuspend = 0;

//-------------------------------------------------------------------------------------------------
//  Private Functions
//-------------------------------------------------------------------------------------------------

void _DrvSclHvspSetGlobal(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    gstGlobalHvspSet = &(((MDrvSclCtxGlobalSet_t*)(pvCtx->pgstGlobalSet))->stHvspCfg);
}
void _DrvSclHvspInitSWVarialbe(DrvSclHvspIdType_e HVSP_IP, void *pvCtx)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    DrvSclOsMemset(&gstGlobalHvspSet->gstScalinInfo[HVSP_IP], 0, sizeof(DrvSclHvspScalingInfo_t));
    gstGlobalHvspSet->gbVScalingup[HVSP_IP] = 0;
    gstGlobalHvspSet->gbHScalingup[HVSP_IP] = 0;
    gstGlobalHvspSet->gbSc3FirstHSet = 1;
    gstGlobalHvspSet->gbSc3FirstVSet = 1;
    if(HVSP_IP == E_DRV_SCLHVSP_ID_1)
    {
        DrvSclOsMemset(&gstGlobalHvspSet->gstIPMCfg, 0, sizeof(DrvSclHvspIpmConfig_t));
        gstGlobalHvspSet->gbVScalingup[HVSP_IP] = SRAMFORSCALUP;
        gstGlobalHvspSet->gbHScalingup[HVSP_IP] = SRAMFORSCALUP;
        gstGlobalHvspSet->gstSrcSize.u16Height    = FHD_Height;
        gstGlobalHvspSet->gstSrcSize.u16Width     = FHD_Width;
        gstGlobalHvspSet->gstSrcSize.bSet         = 0;
        HalSclHvspSetInputSrcSize(&gstGlobalHvspSet->gstSrcSize);
    }
    else if(HVSP_IP == E_DRV_SCLHVSP_ID_2)
    {
        gstGlobalHvspSet->gbVScalingup[HVSP_IP] = SRAMFORSC2ALDOWN;
        gstGlobalHvspSet->gbHScalingup[HVSP_IP] = SRAMFORSC2ALDOWN;
    }
    else if(HVSP_IP == E_DRV_SCLHVSP_ID_3)
    {

        gstGlobalHvspSet->gbVScalingup[HVSP_IP] = SRAMFORSC3VDOWN;
        gstGlobalHvspSet->gbHScalingup[HVSP_IP] = SRAMFORSC3HDOWN;
    }
    else if(HVSP_IP == E_DRV_SCLHVSP_ID_4)
    {

        gstGlobalHvspSet->gbVScalingup[HVSP_IP] = SRAMFORSC4ALDOWN;
        gstGlobalHvspSet->gbHScalingup[HVSP_IP] = SRAMFORSC4ALDOWN;
    }

}

void _Drv_HVSP_FillPreCropInfo(HalSclHvspCropInfo_t *stCropInfo)
{
    if(Is_IPM_NotSetReady() || Is_PreCropNotNeedToOpen())
    {
        if(Is_IPM_NotSetReady())
        {
            DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s %d:: IPM without setting\n", __FUNCTION__, __LINE__));
        }
        stCropInfo->bEn = 0;
        stCropInfo->u16Vst = 0;
        stCropInfo->u16Hst = 0;
        stCropInfo->u16Hsize = 0;
        stCropInfo->u16Vsize = 0;
    }
    else
    {
        stCropInfo->bEn = 1;
        if(Is_InputSrcRotate())//rotate
        {
            stCropInfo->u16Hst = 0;
            stCropInfo->u16Vst = 0;
        }
        else if(Is_PreCropWidthNeedToOpen() || Is_PreCropHeightNeedToOpen())
        {
            if(Is_PreCropWidthNeedToOpen())
            {
                stCropInfo->u16Hst      = (gstGlobalHvspSet->gstSrcSize.u16Width - gstGlobalHvspSet->gstIPMCfg.u16Fetch)/2;
            }
            if(Is_PreCropHeightNeedToOpen())
            {
                stCropInfo->u16Vst      = (gstGlobalHvspSet->gstSrcSize.u16Height - gstGlobalHvspSet->gstIPMCfg.u16Vsize)/2;
            }
        }
        else
        {
            stCropInfo->u16Hst = 0;
            stCropInfo->u16Vst = 0;
        }
        stCropInfo->u16Hsize    = gstGlobalHvspSet->gstIPMCfg.u16Fetch;
        stCropInfo->u16Vsize    = gstGlobalHvspSet->gstIPMCfg.u16Vsize;
    }
    // crop1
    stCropInfo->u16In_hsize = gstGlobalHvspSet->gstSrcSize.u16Width;
    stCropInfo->u16In_vsize = gstGlobalHvspSet->gstSrcSize.u16Height;
    if(stCropInfo->u16In_hsize == 0)
    {
        stCropInfo->u16In_hsize = FHD_Width;
    }
    if(stCropInfo->u16In_vsize == 0)
    {
        stCropInfo->u16In_vsize = FHD_Height;
    }
}
void _Drv_HVSP_SetCoringThrdOn(DrvSclHvspIdType_e enHVSP_ID)
{
    HalSclHvspSetHspCoringThrdC(enHVSP_ID,0x1);
    HalSclHvspSetHspCoringThrdY(enHVSP_ID,0x1);
    HalSclHvspSetVspCoringThrdC(enHVSP_ID,0x1);
    HalSclHvspSetVspCoringThrdY(enHVSP_ID,0x1);
}


//-------------------------------------------------------------------------------------------------
//  Public Functions
//-------------------------------------------------------------------------------------------------

void * DrvSclHvspGetWaitQueueHead(void)
{
    return DrvSclIrqGetSyncQueue();
}
void DrvSclHvspReSetHw(void *pvCtx)
{
    HalSclHvspReSetHw(pvCtx);
}
void DrvSclHvspRelease(DrvSclHvspIdType_e HVSP_IP,void *pvCtx)
{
#if I2_DVR
#else
    _DrvSclHvspInitSWVarialbe(HVSP_IP,(MDrvSclCtxCmdqConfig_t*)pvCtx);
    if(HVSP_IP == E_DRV_SCLHVSP_ID_1)
    {
        HalSclHvspSetReset((MDrvSclCtxCmdqConfig_t*)pvCtx);
    }
    if(HVSP_IP == E_DRV_SCLHVSP_ID_1)
    {
        if(!DrvSclOsReleaseMutexAll())
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[DRVHVSP]!!!!!!!!!!!!!!!!!!! HVSP Release Mutex fail\n");
        }
    }
#endif
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(HVSP_IP)), "[DRVHVSP]%s(%d) HVSP %d Release\n", __FUNCTION__, __LINE__,HVSP_IP);
}
void DrvSclHvspOpen(DrvSclHvspIdType_e HVSP_IP)
{
}
bool DrvSclHvspSuspend(DrvSclHvspSuspendResumeConfig_t *pCfg)
{
    DrvSclIrqSuspendResumeConfig_t stSclirq;
    bool bRet = TRUE;
    DrvSclOsMemset(&stSclirq,0,sizeof(DrvSclIrqSuspendResumeConfig_t));
    DrvSclOsObtainMutex(_HVSP_Mutex, SCLOS_WAIT_FOREVER);
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pCfg->pvCtx);
    HalSclHvspSetReset((MDrvSclCtxCmdqConfig_t *)pCfg->pvCtx);
    if(gbHvspSuspend == 0)
    {
        stSclirq.u32IRQNUM =  pCfg->u32IRQNUM;
        stSclirq.u32CMDQIRQNUM =  pCfg->u32CMDQIRQNUM;
        if(DrvSclIrqSuspend(&stSclirq))
        {
            bRet = TRUE;
            gbHvspSuspend = 1;
        }
        else
        {
            bRet = FALSE;
            DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d) Suspend IRQ Fail\n", __FUNCTION__, __LINE__));
        }
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady suspned\n", __FUNCTION__, __LINE__);
        bRet = TRUE;
    }

    DrvSclOsReleaseMutex(_HVSP_Mutex);

    return bRet;
}
void _Drv_HVSP_SetHWinit(void)
{
    HalSclHvspSetTestPatCfg();
    HalSclHvspVtrackSetUUID();
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_V,SRAMFORSCALUP);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_V_1,SRAMFORSC2ALDOWN);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_V_2,SRAMFORSC3VDOWN);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_V_3,SRAMFORSC4ALDOWN);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_H,SRAMFORSCALUP);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_H_1,SRAMFORSC2ALDOWN);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_H_2,SRAMFORSC3HDOWN);
    HalSclHvspSramDump(E_HAL_SCLHVSP_SRAM_DUMP_HVSP_H_3,SRAMFORSC4ALDOWN);
}

bool DrvSclHvspResume(DrvSclHvspSuspendResumeConfig_t *pCfg)
{
    DrvSclIrqSuspendResumeConfig_t stSclirq;
    bool bRet = TRUE;
    DrvSclOsMemset(&stSclirq,0,sizeof(DrvSclIrqSuspendResumeConfig_t));
    DrvSclOsObtainMutex(_HVSP_Mutex, SCLOS_WAIT_FOREVER);
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pCfg->pvCtx);
    //sclprintf("%s,(%d) %d\n", __FUNCTION__, __LINE__, gbHvspSuspend);
    if(gbHvspSuspend == 1)
    {
        stSclirq.u32IRQNUM =  pCfg->u32IRQNUM;
        stSclirq.u32CMDQIRQNUM =  pCfg->u32CMDQIRQNUM;
        if(DrvSclIrqResume(&stSclirq))
        {
            //DrvSclIrqInterruptEnable(SCLIRQ_VSYNC_FCLK_LDC);
            HalSclHvspSetTestPatCfg();
            gbHvspSuspend = 0;
            bRet = TRUE;
        }
        else
        {

            DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d) Resume IRQ Fail\n", __FUNCTION__, __LINE__));
            bRet = FALSE;
        }
        _Drv_HVSP_SetHWinit();
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady resume\n", __FUNCTION__, __LINE__);
        bRet = TRUE;
    }

    DrvSclOsReleaseMutex(_HVSP_Mutex);

    return bRet;
}
void DrvSclHvspExit(u8 bCloseISR)
{
    if(_HVSP_Mutex != -1)
    {
        DrvSclOsDeleteMutex(_HVSP_Mutex);
        _HVSP_Mutex = -1;
    }
    if(bCloseISR)
    {
        DrvSclIrqExit();
    }
    HalSclHvspMloadSramBufferFree();
    HalSclHvspExit();
    DrvSclOsSetClkForceMode(0);
}

bool DrvSclHvspInit(DrvSclHvspInitConfig_t *pInitCfg)
{
    char word[] = {"_HVSP_Mutex"};
    DrvSclIrqInitConfig_t stIRQInitCfg;
    u8 u8IDidx;
    DrvSclOsMemset(&stIRQInitCfg,0,sizeof(DrvSclIrqInitConfig_t));
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pInitCfg->pvCtx);
    if(_HVSP_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(3)), "[DRVHVSP]%s(%d) alrady done\n", __FUNCTION__, __LINE__);
        return TRUE;
    }

    if(DrvSclOsInit() == FALSE)
    {
        DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d) DrvSclOsInit Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    _HVSP_Mutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, word, SCLOS_PROCESS_SHARED);

    if (_HVSP_Mutex == -1)
    {
        DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d): create mutex fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    stIRQInitCfg.u32RiuBase = pInitCfg->u32RIUBase;
    stIRQInitCfg.u32IRQNUM  = pInitCfg->u32IRQNUM;
    stIRQInitCfg.u32CMDQIRQNUM  = pInitCfg->u32CMDQIRQNUM;
    stIRQInitCfg.pvCtx = pInitCfg->pvCtx;
    if(DrvSclIrqInit(&stIRQInitCfg) == FALSE)
    {
        DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d) Init IRQ Fail\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    HalSclHvspSetRiuBase(pInitCfg->u32RIUBase);
    DRV_HVSP_MUTEX_LOCK();
    HalSclHvspSetReset((MDrvSclCtxCmdqConfig_t *)pInitCfg->pvCtx);
    for(u8IDidx = E_DRV_SCLHVSP_ID_1; u8IDidx<E_DRV_SCLHVSP_ID_MAX; u8IDidx++)
    {
        _DrvSclHvspInitSWVarialbe(u8IDidx,(MDrvSclCtxCmdqConfig_t *)pInitCfg->pvCtx);
        _Drv_HVSP_SetCoringThrdOn(u8IDidx);
    }
    DRV_HVSP_MUTEX_UNLOCK();
    HalSclHvspSetFrameBufferManageLock(0);
    _Drv_HVSP_SetHWinit();
    HalSclHvspMloadSramBufferPrepare();
    return TRUE;
}
void _DrvSclHvspSetPreCropWhenInputMuxReady(void)
{
    HalSclHvspCropInfo_t stCropInfo_1;
    DrvSclOsMemset(&stCropInfo_1,0,sizeof(HalSclHvspCropInfo_t));
    if(Is_INPUTMUX_SetReady())
    {
        _Drv_HVSP_FillPreCropInfo(&stCropInfo_1);
        HalSclHvspSetCropConfig(E_DRV_SCLHVSP_CROP_ID_1, &stCropInfo_1);
    }
}
bool DrvSclHvspSetIPMConfig(DrvSclHvspIpmConfig_t *stCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): Base=%lx, Fetch=%d, Vsize=%d\n"
        , __FUNCTION__, __LINE__, stCfg->u32YCBaseAddr, stCfg->u16Fetch, stCfg->u16Vsize);
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stCfg->pvCtx);
    DrvSclOsMemcpy(&gstGlobalHvspSet->gstIPMCfg, stCfg, sizeof(DrvSclHvspIpmConfig_t));
    DRV_HVSP_MUTEX_LOCK();
    if(gstGlobalHvspSet->gstIPMCfg.u16Fetch%2)
    {
        gstGlobalHvspSet->gstIPMCfg.u16Fetch--;
        stCfg->u16Fetch--;
        SCL_ERR("[DRVHVSP]IPM Width not align 2\n");
    }
    //_DrvSclHvspSetPreCropWhenInputMuxReady();
    HalSclHvspSetUcmYCBase1(stCfg->u32YCBaseAddr);
    if(DrvSclOsGetSclFrameBufferNum()== 2)
    {
        HalSclHvspSetUcmYCBase2(stCfg->u32YCBaseAddr+(stCfg->u32MemSize/2));
    }
    HalSclHvspSetIpmFetchNum(stCfg->u16Fetch);
    HalSclHvspSetIpmLineOffset(stCfg->u16Fetch);
    HalSclHvspSetIpmvSize(stCfg->u16Vsize);
    HalSclHvspSetUCMConpress(stCfg->bYCMWrite ? E_HAL_SCLUCM_CE8_ON : E_HAL_SCLUCM_OFF);
    HalSclHvspSetIpmYCMWriteEn(stCfg->bYCMWrite);
    DRV_HVSP_MUTEX_UNLOCK();
    return TRUE;
}
void DrvSclHvspSetFbManageConfig(DrvSclHvspSetFbManageConfig_t *stCfg)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stCfg->pvCtx);
    HalSclHvspSetFrameBufferManageLock(0);
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_PRVCROP_ON)
    {
        HalSclHvspSetPrv2CropOnOff(1);
        sclprintf("PRVCROP ON\n");
    }
    else if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_PRVCROP_OFF)
    {
        HalSclHvspSetPrv2CropOnOff(0);
        sclprintf("PRVCROP OFF\n");
    }
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_Read_ON)
    {
        HalSclHvspSetIpmYCMReadEn(1);
        sclprintf("DNRR ON\n");
    }
    else if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_Read_OFF)
    {
        HalSclHvspSetIpmYCMReadEn(0);
        sclprintf("DNRR OFF\n");
    }
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_Write_ON)
    {
        HalSclHvspSetIpmYCMWriteEn(1);
        sclprintf("DNRW ON\n");
    }
    else if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_Write_OFF)
    {
        HalSclHvspSetIpmYCMWriteEn(0);
        sclprintf("DNRW OFF\n");
    }
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_BUFFER_1)
    {
        HalSclHvspSetIpmBufferNumber(1);
        sclprintf("DNRB 1\n");
    }
    else if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_DNR_BUFFER_2)
    {
        HalSclHvspSetIpmBufferNumber(2);
        sclprintf("DNRB 2\n");
    }
    HalSclHvspSetFrameBufferManageLock(1);
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_UNLOCK)
    {
        HalSclHvspSetFrameBufferManageLock(0);
        sclprintf("UNLOCK\n");
    }
    if(stCfg->enSet & E_DRV_SCLHVSP_FBMG_SET_LOCK)
    {
        HalSclHvspSetFrameBufferManageLock(1);
        sclprintf("LOCK\n");
    }
}

u8 _DrvSclHvspGetScalingHRatioConfig(DrvSclHvspIdType_e enHVSP_ID,u8 bUp)
{
    u8 bret = 0;
    if(bUp)
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            bret = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            bret = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            bret = SRAMFORSC3HUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            bret = SRAMFORSC4ALUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            bret = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width >=HD_Width)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstGlobalHvspSet->gstScalinInfo[E_DRV_SCLHVSP_ID_2].stSizeAfterScaling.u16Width >=D_Width)
            {
                bret = SRAMFORSC2ALDOWN;
            }
            else
            {
                bret = SRAMFORSC3HDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            bret = SRAMFORSC4ALDOWN;
        }
    }
    return bret;
}
u8 _DrvSclHvspGetScalingVRatioConfig(DrvSclHvspIdType_e enHVSP_ID,u8 bUp)
{
    u8 bret = 0;
    if(bUp)
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            bret = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            bret = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            bret = SRAMFORSC3VUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            bret = SRAMFORSC4ALUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            bret = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height >=HD_Height)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstGlobalHvspSet->gstScalinInfo[E_DRV_SCLHVSP_ID_2].stSizeAfterScaling.u16Height >=D_Height)
            {
                bret = SRAMFORSCALDOWN;
            }
            else
            {
                bret = SRAMFORSC3VDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            bret = SRAMFORSC4ALDOWN;
        }
    }
    return bret;
}
void _DrvSclHvspSetHorizotnalScalingConfig(DrvSclHvspIdType_e enHVSP_ID,bool bEn)
{

    HalSclHvspSetScalingHoEn(enHVSP_ID, bEn);
    HalSclHvspSetScalingHoFacotr(enHVSP_ID, bEn ? gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_H: 0);
    //ToDo:Sram Mload
    HalSclHvspSetModeYHo(enHVSP_ID,bEn ? E_HAL_SCLHVSP_FILTER_MODE_SRAM_0: E_HAL_SCLHVSP_FILTER_MODE_BYPASS);
    HalSclHvspSetModeCHo(enHVSP_ID,bEn ? E_HAL_SCLHVSP_FILTER_MODE_BILINEAR: E_HAL_SCLHVSP_FILTER_MODE_BYPASS,E_HAL_SCLHVSP_SRAM_SEL_0);
    HalSclHvspSetHspDithEn(enHVSP_ID,bEn);
    HalSclHvspSetHspCoringEnC(enHVSP_ID,bEn);
    HalSclHvspSetHspCoringEnY(enHVSP_ID,bEn);
}
void _DrvSclHvspSetHTbl(DrvSclHvspIdType_e enHVSP_ID)
{
    if(gstGlobalHvspSet->gbHScalingup[enHVSP_ID] &0x1)
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC3HUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC4ALUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width >=HD_Width)
            {
                gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
                gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            // if sc 2 output > 704x576 use talbe 1.
            if(gstGlobalHvspSet->gstScalinInfo[E_DRV_SCLHVSP_ID_2].stSizeAfterScaling.u16Width >=D_Width)
            {
                gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
            else
            {
                gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC3HDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = SRAMFORSC4ALDOWN;
        }
    }
}
void _DrvSclHvspSetVTbl(DrvSclHvspIdType_e enHVSP_ID)
{
    if(gstGlobalHvspSet->gbVScalingup[enHVSP_ID] &0x1)
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSCALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC2ALUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC3VUP;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC4ALUP;
        }
    }
    else
    {
        if(enHVSP_ID ==E_DRV_SCLHVSP_ID_1)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_2)
        {
            if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height>=HD_Height)
            {
                gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
                gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
        #if I2_DVR
            if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height>=HD_Height)
        #else
            if(gstGlobalHvspSet->gstScalinInfo[E_DRV_SCLHVSP_ID_2].stSizeAfterScaling.u16Height >=D_Height)
        #endif
            {
                gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSCALDOWN;
            }
            else
            {
        #if I2_DVR
                gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC2ALDOWN;
        #else
                gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC3VDOWN;
        #endif
            }
        }
        else if(enHVSP_ID ==E_DRV_SCLHVSP_ID_4)
        {
            gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = SRAMFORSC4ALDOWN;
        }
    }
}
void _DrvSclHvspSetHorizotnalSramTblbyMload(DrvSclHvspIdType_e enHVSP_ID)
{
    if((_CHANGE_SRAM_H_QMAP(enHVSP_ID,(gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))))
    {
        gstGlobalHvspSet->gbHScalingup[enHVSP_ID] &= 0x1;
        gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = (gstGlobalHvspSet->gbHScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        _DrvSclHvspSetHTbl(enHVSP_ID);
    }
    else if(((_DrvSclHvspGetScalingHRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))!=
        ((gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0xF0)>>4)))
    {
        _DrvSclHvspSetHTbl(enHVSP_ID);
    }
    HalSclHvspSramDumpbyMload(enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID],1); //level 1 :up 0:down
}
void _DrvSclHvspSetVerticalSramTblbyMload(DrvSclHvspIdType_e enHVSP_ID)
{
    if((_CHANGE_SRAM_V_QMAP(enHVSP_ID,(gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1))))
    {
        gstGlobalHvspSet->gbVScalingup[enHVSP_ID] &= 0x1;
        gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = (gstGlobalHvspSet->gbVScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        _DrvSclHvspSetVTbl(enHVSP_ID);
    }
    else if(((_DrvSclHvspGetScalingVRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1))!=
        ((gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0xF0)>>4)))
    {
        _DrvSclHvspSetVTbl(enHVSP_ID);
    }
    HalSclHvspSramDumpbyMload(enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID],0); //level 1 :up 0:down
}
void _DrvSclHvspSetHorizotnalSramTbl(DrvSclHvspIdType_e enHVSP_ID)
{
    u32 u32flag;
    if((_CHANGE_SRAM_H_QMAP(enHVSP_ID,(gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))))
    {
        gstGlobalHvspSet->gbHScalingup[enHVSP_ID] &= 0x1;
        gstGlobalHvspSet->gbHScalingup[enHVSP_ID] = (gstGlobalHvspSet->gbHScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        //gstGlobalHvspSet->gbHScalingup[enHVSP_ID] |= ((_DrvSclHvspGetScalingHRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))<<4);
        _DrvSclHvspSetHTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_DRV_SCLHVSP_ID_1) ? E_DRV_SCLIRQ_EVENT_SC1HSRAMSET :
                  (enHVSP_ID==E_DRV_SCLHVSP_ID_2) ? E_DRV_SCLIRQ_EVENT_SC2HSRAMSET :
                                             E_DRV_SCLIRQ_EVENT_SC3HSRAMSET;
        if(DrvSclIrqGetIsBlankingRegion()|| enHVSP_ID ==E_DRV_SCLHVSP_ID_3 || VIPSETRULE())
        {
            HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]);
        }
        else
        {
            DrvSclOsSetEventIrq(DrvSclIrqGetIrqSYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change H Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]);
        }
    }
    else if(gstGlobalHvspSet->gbSc3FirstHSet && enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
    {
        gstGlobalHvspSet->gbSc3FirstHSet = 0;
        _DrvSclHvspSetHTbl(enHVSP_ID);
        u32flag = E_DRV_SCLIRQ_EVENT_SC3HSRAMSET;
         if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
         {
             HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
             SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                 "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling static UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]);
         }
    }
    else if(enHVSP_ID !=E_DRV_SCLHVSP_ID_3 && ((_DrvSclHvspGetScalingHRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))!=
        ((gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0xF0)>>4)))
    {
        //gstGlobalHvspSet->gbHScalingup[enHVSP_ID] &= 0x1;
        //gstGlobalHvspSet->gbHScalingup[enHVSP_ID] |= ((_DrvSclHvspGetScalingHRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]&0x1))<<4);
        _DrvSclHvspSetHTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_DRV_SCLHVSP_ID_1) ? E_DRV_SCLIRQ_EVENT_SC1HSRAMSET :
                  (enHVSP_ID==E_DRV_SCLHVSP_ID_2) ? E_DRV_SCLIRQ_EVENT_SC2HSRAMSET :
                                             E_DRV_SCLIRQ_EVENT_SC3HSRAMSET;
        if(DrvSclIrqGetIsBlankingRegion()|| enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_H_OFFSET,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do H Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]);
        }
        else
        {
            DrvSclOsSetEventIrq(DrvSclIrqGetIrqSYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change H Qmap SRAM id:%d scaling RUP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbHScalingup[enHVSP_ID]);
        }
    }
}
void _DrvSclHvspSetVerticalScalingConfig(DrvSclHvspIdType_e enHVSP_ID,bool bEn)
{
    HalSclHvspSetScalingVeEn(enHVSP_ID, bEn);
    HalSclHvspSetScalingVeFactor(enHVSP_ID,  bEn ? gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_V: 0);
    HalSclHvspSetModeYVe(enHVSP_ID,bEn ? E_HAL_SCLHVSP_FILTER_MODE_SRAM_0: E_HAL_SCLHVSP_FILTER_MODE_BYPASS);
    HalSclHvspSetModeCVe(enHVSP_ID,bEn ? E_HAL_SCLHVSP_FILTER_MODE_BILINEAR: E_HAL_SCLHVSP_FILTER_MODE_BYPASS,E_HAL_SCLHVSP_SRAM_SEL_0);
    HalSclHvspSetVspDithEn(enHVSP_ID,bEn);
    HalSclHvspSetVspCoringEnC(enHVSP_ID,bEn);
    HalSclHvspSetVspCoringEnY(enHVSP_ID,bEn);
}
void _DrvSclHvspSetVerticalSramTbl(DrvSclHvspIdType_e enHVSP_ID)
{
    u32 u32flag;
    if(_CHANGE_SRAM_V_QMAP(enHVSP_ID,(gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1)))
    {
        gstGlobalHvspSet->gbVScalingup[enHVSP_ID] &= 0x1;
        gstGlobalHvspSet->gbVScalingup[enHVSP_ID] = (gstGlobalHvspSet->gbVScalingup[enHVSP_ID]^1);//XOR 1 :reverse
        //gstGlobalHvspSet->gbVScalingup[enHVSP_ID] |= ((_DrvSclHvspGetScalingVRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1))<<4);
        _DrvSclHvspSetVTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_DRV_SCLHVSP_ID_1) ? E_DRV_SCLIRQ_EVENT_SC1VSRAMSET :
                  (enHVSP_ID==E_DRV_SCLHVSP_ID_2) ? E_DRV_SCLIRQ_EVENT_SC2VSRAMSET :
                                             E_DRV_SCLIRQ_EVENT_SC3VSRAMSET;

        if(DrvSclIrqGetIsBlankingRegion()|| enHVSP_ID ==E_DRV_SCLHVSP_ID_3 || VIPSETRULE())
        {
            HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]);
        }
        else
        {
            DrvSclOsSetEventIrq(DrvSclIrqGetIrqSYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]);
        }
    }
    else if(gstGlobalHvspSet->gbSc3FirstVSet && enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
    {
        gstGlobalHvspSet->gbSc3FirstVSet = 0;
        _DrvSclHvspSetVTbl(enHVSP_ID);
        u32flag = E_DRV_SCLIRQ_EVENT_SC3VSRAMSET;
         if(enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
         {
             HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
             SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                 "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling static UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]);
         }
    }
    else if(enHVSP_ID !=E_DRV_SCLHVSP_ID_3 && ((_DrvSclHvspGetScalingVRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1))!=
        ((gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0xF0)>>4)))
    {
        //gstGlobalHvspSet->gbVScalingup[enHVSP_ID] &= 0x1;
        //gstGlobalHvspSet->gbVScalingup[enHVSP_ID] |= ((_DrvSclHvspGetScalingVRatioConfig(enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]&0x1))<<4);
        _DrvSclHvspSetVTbl(enHVSP_ID);
        u32flag = (enHVSP_ID==E_DRV_SCLHVSP_ID_1) ? E_DRV_SCLIRQ_EVENT_SC1VSRAMSET :
                  (enHVSP_ID==E_DRV_SCLHVSP_ID_2) ? E_DRV_SCLIRQ_EVENT_SC2VSRAMSET :
                                             E_DRV_SCLIRQ_EVENT_SC3VSRAMSET;

        if(DrvSclIrqGetIsBlankingRegion()|| enHVSP_ID ==E_DRV_SCLHVSP_ID_3)
        {
            HalSclHvspSramDump(enHVSP_ID+HVSP_ID_SRAM_V_OFFSET,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]); //level 1 :up 0:down
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change and do V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]);
        }
        else
        {
            DrvSclOsSetEventIrq(DrvSclIrqGetIrqSYNCEventID(), u32flag);
            SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
                "[DRVHVSP]Change V Qmap SRAM id:%d scaling UP:%hhx\n",enHVSP_ID,gstGlobalHvspSet->gbVScalingup[enHVSP_ID]);
        }
    }
}
void _DrvSclHvspSetCrop(bool u8CropID,DrvSclHvspScalingConfig_t *stCfg,HalSclHvspCropInfo_t *stCropInfo)
{
    //I3
    //u16In_hsize = (u8CropID ==DRV_HVSP_CROP_1) ? gstGlobalHvspSet->gstSrcSize.u16Width : gstGlobalHvspSet->gstIPMCfg.u16Fetch;
    //u16In_vsize = (u8CropID ==DRV_HVSP_CROP_1) ? gstGlobalHvspSet->gstSrcSize.u16Height : gstGlobalHvspSet->gstIPMCfg.u16Vsize;
    stCropInfo->bEn         = stCfg->bCropEn[u8CropID];
    if(u8CropID ==DRV_HVSP_CROP_1)
    {
        stCropInfo->u16In_hsize = stCfg->u16Src_Width;
        stCropInfo->u16In_vsize = stCfg->u16Src_Height;
    }
    else if(u8CropID ==DRV_HVSP_CROP_2)
    {
        stCropInfo->u16In_hsize = stCfg->u16Crop_Width[DRV_HVSP_CROP_1];
        stCropInfo->u16In_vsize = stCfg->u16Crop_Height[DRV_HVSP_CROP_1];
    }
    stCropInfo->u16Hsize    = stCfg->u16Crop_Width[u8CropID];
    stCropInfo->u16Vsize    = stCfg->u16Crop_Height[u8CropID];
    if(stCfg->u16Crop_Width[u8CropID]%2)
    {
        stCropInfo->u16Hsize      = stCfg->u16Crop_Width[u8CropID]-1;
    }
    if(u8CropID == DRV_HVSP_CROP_1)
    {
        gstGlobalHvspSet->gstIPMCfg.u16Fetch = stCropInfo->u16Hsize;
        gstGlobalHvspSet->gstIPMCfg.u16Vsize = stCropInfo->u16Vsize;
    }
    if(stCfg->u16Crop_X[u8CropID]%2)
    {
        stCropInfo->u16Hst      = stCfg->u16Crop_X[u8CropID]+1;
    }
    else
    {
        stCropInfo->u16Hst      = stCfg->u16Crop_X[u8CropID];
    }
    stCropInfo->u16Vst      = stCfg->u16Crop_Y[u8CropID];
}
bool DrvSclHvspSetScaling(DrvSclHvspIdType_e enHVSP_ID, DrvSclHvspScalingConfig_t *stCfg, DrvSclHvspClkConfig_t* stclk)
{
    HalSclHvspCropInfo_t stCropInfo_2;
    HalSclHvspCropInfo_t stCropInfo_1;
    u64 u64Temp;
    u64 u64Temp2;
    DrvSclOsMemset(&stCropInfo_1,0,sizeof(HalSclHvspCropInfo_t));
    DrvSclOsMemset(&stCropInfo_2,0,sizeof(HalSclHvspCropInfo_t));
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stCfg->pvCtx);
    stCropInfo_1.bEn = 0;
    stCropInfo_2.bEn = 0;

    if(enHVSP_ID == E_DRV_SCLHVSP_ID_1)
    {


        SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]%s id:%d @:%lu\n",
            __FUNCTION__,enHVSP_ID,(u32)DrvSclOsGetSystemTime());
        // setup cmd trig config
        DRV_HVSP_MUTEX_LOCK();
        _DrvSclHvspSetCrop(DRV_HVSP_CROP_1,stCfg,&stCropInfo_1);
        _DrvSclHvspSetCrop(DRV_HVSP_CROP_2,stCfg,&stCropInfo_2);

        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width  = stCropInfo_2.u16Hsize;//stCfg->u16Src_Width;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height = stCropInfo_2.u16Vsize;//stCfg->u16Src_Height;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width  = stCfg->u16Dsp_Width;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height = stCfg->u16Dsp_Height;
        gstGlobalHvspSet->gstIPMCfg.u16Fetch = stCropInfo_2.u16In_hsize;
        gstGlobalHvspSet->gstIPMCfg.u16Vsize = stCropInfo_2.u16In_vsize;
        // Crop1
        HalSclHvspSetCropConfig(E_DRV_SCLHVSP_CROP_ID_1, &stCropInfo_1);
        // Crop2
        HalSclHvspSetCropConfig(E_DRV_SCLHVSP_CROP_ID_2, &stCropInfo_2);
        HalSclHvspSetIpmvSize(stCropInfo_2.u16In_vsize);
        HalSclHvspSetIpmLineOffset(stCropInfo_2.u16In_hsize);
        HalSclHvspSetIpmFetchNum(stCropInfo_2.u16In_hsize);
        HalSclHvspSetNeSampleStep(stCropInfo_2.u16In_hsize,stCropInfo_2.u16In_vsize);
        HalSclHvspSetLdcWidth(stCropInfo_2.u16In_hsize);
        HalSclHvspSetLdcHeight(stCropInfo_2.u16In_vsize);
        // NLM size
        HalSclHvspSetVipSize(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        HalSclHvspSetNlmLineBufferSize(stCropInfo_2.u16In_hsize,stCropInfo_2.u16In_vsize);
        HalSclHvspSetNlmEn(1);
        //AIP size
        HalSclHvspSetXnrSize(stCropInfo_2.u16In_hsize,stCropInfo_2.u16In_vsize);
        HalSclHvspSetWdrLocalSize(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        HalSclHvspSetMXnrSize(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
        HalSclHvspSetUVadjSize(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
    }
    else
    {
        DRV_HVSP_MUTEX_LOCK();
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width      = stCfg->u16Src_Width;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height     = stCfg->u16Src_Height;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width   = stCfg->u16Dsp_Width;
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height  = stCfg->u16Dsp_Height;
    }
    if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width>1 && (gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width>1))
    {
        u64Temp = (((u64)gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width) * 1048576);
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_H =
            (u32)CamOsMathDivU64(u64Temp, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width,&u64Temp2);
    }
    else
    {
        SCL_ERR("[DRVHVSP]%d Ratio Error\n",enHVSP_ID);
    }
    if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height>1 && (gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height>1))
    {
        u64Temp = (((u64)gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height) * 1048576);
        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_V =
            (u32)CamOsMathDivU64(u64Temp, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height,&u64Temp2);
    }
    else
    {
        SCL_ERR("[DRVHVSP]%d Ratio Error\n",enHVSP_ID);
    }
    //gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_H = (u32)HVSP_RATIO(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
    //                                                        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width);

    //gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_V = (u32)HVSP_RATIO(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height,
    //                                                        gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);

    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, AfterCrop(%d, %d)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, AfterScaling(%d, %d)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[DRVHVSP]%s(%d):: HVSP_%d, Ratio(%lx, %lx)\n", __FUNCTION__, __LINE__,
        enHVSP_ID, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_H, gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].u32ScalingRatio_V);

    // horizotnal HVSP Scaling
    if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width == gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width)
    {
        _DrvSclHvspSetHorizotnalScalingConfig(enHVSP_ID, FALSE);
    }
    else
    {
        _DrvSclHvspSetHorizotnalScalingConfig(enHVSP_ID, TRUE);
        //ToDo :MLOAD
        _DrvSclHvspSetHorizotnalSramTblbyMload(enHVSP_ID);
    }

    // vertical HVSP Scaling
    if(gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height == gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height)
    {
        _DrvSclHvspSetVerticalScalingConfig(enHVSP_ID, FALSE);
    }
    else
    {
        _DrvSclHvspSetVerticalScalingConfig(enHVSP_ID, TRUE);
        //ToDo :MLOAD
        _DrvSclHvspSetVerticalSramTblbyMload(enHVSP_ID);
    }

    // HVSP In size
    HalSclHvspSetHVSPInputSize(enHVSP_ID,
                                 gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Width,
                                 gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterCrop.u16Height);

    // HVSP Out size
    HalSclHvspSetHVSPOutputSize(enHVSP_ID,
                                  gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Width,
                                  gstGlobalHvspSet->gstScalinInfo[enHVSP_ID].stSizeAfterScaling.u16Height);
    DRV_HVSP_MUTEX_UNLOCK();
    stCfg->bRet = 1;
    return stCfg->bRet;
}
void DrvSclHvspSetInputSrcSize(ST_HVSP_SIZE_CONFIG *stSize)
{
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d)\n", __FUNCTION__,  __LINE__);
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stSize->pvCtx);
    if(stSize->u16Height > 0)
    {
        gstGlobalHvspSet->gstSrcSize.u16Height    = stSize->u16Height;
    }
    if(stSize->u16Width > 0)
    {
        gstGlobalHvspSet->gstSrcSize.u16Width     = stSize->u16Width;
    }
    gstGlobalHvspSet->gstSrcSize.bSet = 1;
    HalSclHvspSetInputSrcSize(&gstGlobalHvspSet->gstSrcSize);
}
void DrvSclHvspSetCropWindowSize(void *pvCtx)
{
    HalSclHvspCropInfo_t stCropInfo_1;
    DrvSclOsMemset(&stCropInfo_1,0,sizeof(HalSclHvspCropInfo_t));
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    _Drv_HVSP_FillPreCropInfo(&stCropInfo_1);
    DRV_HVSP_MUTEX_LOCK();
    HalSclHvspSetCropConfig(E_DRV_SCLHVSP_CROP_ID_1, &stCropInfo_1);
    DRV_HVSP_MUTEX_UNLOCK();
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s Input:(%hd,%hd) Crop:(%hd,%hd)\n", __FUNCTION__,
        stCropInfo_1.u16In_hsize,stCropInfo_1.u16In_vsize,stCropInfo_1.u16Hsize,stCropInfo_1.u16Vsize);
}

u32 DrvSclHvspGetInputSrcMux(DrvSclHvspIdType_e enID)
{
    return HalSclHvspGetInputSrcMux(enID);
}

bool DrvSclHvspSetInputMux(DrvSclHvspIpMuxType_e enIP,DrvSclHvspClkConfig_t* stclk,void *pvCtx)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    DRV_HVSP_DBG(sclprintf("[DRVHVSP]%s(%d): IP=%x\n", __FUNCTION__,  __LINE__,enIP));
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): IP=%x\n", __FUNCTION__,  __LINE__,enIP);
    HalSclHvspSetInputMuxType(E_DRV_SCLHVSP_ID_1,enIP);
    if(!DrvSclOsGetClkForceMode() && stclk != NULL)
    {
        HalSclHvspSetIdClkOnOff(1,stclk);
    }
    if(enIP >= E_DRV_SCLHVSP_IP_MUX_MAX)
    {
        DRV_HVSP_ERR(sclprintf("[DRVHVSP]%s(%d):: Wrong IP Type\n", __FUNCTION__, __LINE__));
        return FALSE;
    }

    HalSclHvspSetHwInputMux(enIP);
    return TRUE;
}
bool DrvSclHvspSetSc3InputMux(DrvSclHvspIpMuxType_e enIP, void *pvCtx)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d): MuxType=%x\n", __FUNCTION__,  __LINE__,enIP);
    HalSclHvspSetInputMuxType(E_DRV_SCLHVSP_ID_3,enIP);
    HalSclHvspSetHwSc3InputMux(enIP,pvCtx);
    return TRUE;
}
bool DrvSclHvspSetRegisterForceByInst(u32 u32Reg, u8 u8Val, u8 u8Msk, void *pvCtx)
{
    HalSclHvspSetRegisterForceByInst(u32Reg, u8Val, u8Msk);
    return TRUE;
}

bool DrvSclHvspSetRegisterForce(u32 u32Reg, u8 u8Val, u8 u8Msk, void *pvCtx)
{
    HalSclHvspSetReg(u32Reg, u8Val, u8Msk);
    return TRUE;
}
void DrvSclHvspSetOsdConfig(DrvSclHvspIdType_e enID, DrvSclHvspOsdConfig_t *stOSdCfg)
{
    HalSclHvspSetOsdLocate(enID,stOSdCfg->enOSD_loc);
    HalSclHvspSetOsdOnOff(enID,stOSdCfg->stOsdOnOff.bOSDEn);
    HalSclHvspSetOsdBypass(enID,stOSdCfg->stOsdOnOff.bOSDBypass);
    HalSclHvspSetOsdBypassWTM(enID,stOSdCfg->stOsdOnOff.bWTMBypass);
}
void DrvSclHvspSetPrv2CropOnOff(u8 bEn, void *pvCtx)
{
    HalSclHvspSetPrv2CropOnOff(bEn);
}
void DrvSclHvspGetSclInts(void *pvCtx,DrvSclHvspScIntsType_t *sthvspints)
{
    DrvSclIrqScIntsType_t *stints;
    stints = DrvSclIrqGetSclInts();
    DrvSclOsMemcpy(sthvspints,stints,sizeof(DrvSclHvspScIntsType_t));
}
void DrvSclHvspSclIq(DrvSclHvspIdType_e enID,DrvSclHvspIqType_e enIQ,void *pvCtx)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    switch(enIQ)
    {
        case E_DRV_SCLHVSP_IQ_H_Tbl0:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_H_OFFSET,gstGlobalHvspSet->gbHScalingup[enID]&0x1); //level 1 :up 0:down
            gstGlobalHvspSet->gbHScalingup[enID] = gstGlobalHvspSet->gbHScalingup[enID]&0x1;
        break;
        case E_DRV_SCLHVSP_IQ_H_Tbl1:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_H_OFFSET,(0x10 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbHScalingup[enID] = (0x10 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_H_Tbl2:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_H_OFFSET,(0x20 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbHScalingup[enID] = (0x20 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_H_Tbl3:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_H_OFFSET,(0x30 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbHScalingup[enID] = (0x30 |(gstGlobalHvspSet->gbHScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_H_BYPASS:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_BYPASS);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case E_DRV_SCLHVSP_IQ_H_BILINEAR:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYHo(enID,E_HAL_SCLHVSP_FILTER_MODE_BILINEAR);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case E_DRV_SCLHVSP_IQ_V_Tbl0:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_V_OFFSET,gstGlobalHvspSet->gbVScalingup[enID]&0x1); //level 1 :up 0:down
            gstGlobalHvspSet->gbVScalingup[enID] = gstGlobalHvspSet->gbVScalingup[enID]&0x1;
        break;
        case E_DRV_SCLHVSP_IQ_V_Tbl1:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_V_OFFSET,(0x10 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbVScalingup[enID] = (0x10 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_V_Tbl2:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_V_OFFSET,(0x20 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbVScalingup[enID] = (0x20 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_V_Tbl3:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_SRAM_0);
            DRV_HVSP_MUTEX_UNLOCK();
            HalSclHvspSramDump(enID+HVSP_ID_SRAM_V_OFFSET,(0x30 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1))); //level 1 :up 0:down
            gstGlobalHvspSet->gbVScalingup[enID] = (0x30 |(gstGlobalHvspSet->gbVScalingup[enID]&0x1));
        break;
        case E_DRV_SCLHVSP_IQ_V_BYPASS:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_BYPASS);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        case E_DRV_SCLHVSP_IQ_V_BILINEAR:
            DRV_HVSP_MUTEX_LOCK();
            HalSclHvspSetModeYVe(enID,E_HAL_SCLHVSP_FILTER_MODE_BILINEAR);
            DRV_HVSP_MUTEX_UNLOCK();
        break;
        default:
        break;
    }
}
bool DrvSclHvspGetSCLInform(DrvSclHvspIdType_e enID,DrvSclHvspScInformConfig_t *stInformCfg)
{
    stInformCfg->u16X               = HalSclHvspGetCrop2Xinfo();
    stInformCfg->u16Y               = HalSclHvspGetCrop2Yinfo();
    stInformCfg->u16Width           = HalSclHvspGetHvspOutputWidth(enID);
    stInformCfg->u16Height          = HalSclHvspGetHvspOutputHeight(enID);
    stInformCfg->u16crop2inWidth    = HalSclHvspGetCrop2InputWidth();
    stInformCfg->u16crop2inHeight   = HalSclHvspGetCrop2InputHeight();
    stInformCfg->u16crop2OutWidth   = HalSclHvspGetCrop2OutputWidth();
    stInformCfg->u16crop2OutHeight  = HalSclHvspGetCrop2OutputHeight();
    stInformCfg->bEn                = HalSclHvspGetCrop2En();
    return TRUE;
}
bool DrvSclHvspGetHvspAttribute(DrvSclHvspIdType_e enID,DrvSclHvspInformConfig_t *stInformCfg)
{
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stInformCfg->pvCtx);
    stInformCfg->u16Width           = HalSclHvspGetHvspOutputWidth(enID);
    stInformCfg->u16Height          = HalSclHvspGetHvspOutputHeight(enID);
    stInformCfg->u16inWidth         = HalSclHvspGetHvspInputWidth(enID);
    stInformCfg->u16inHeight        = HalSclHvspGetHvspInputHeight(enID);
    stInformCfg->bEn                = HalSclHvspGetScalingFunctionStatus(enID);
    stInformCfg->bEn |= (gstGlobalHvspSet->gbVScalingup[enID]&0xF0);
    stInformCfg->bEn |= ((gstGlobalHvspSet->gbHScalingup[enID]&0xF0)<<2);
    return TRUE;
}
void DrvSclHvspGetOsdAttribute(DrvSclHvspIdType_e enID,DrvSclHvspOsdConfig_t *stOsdCfg)
{
    stOsdCfg->enOSD_loc = HalSclHvspGetOsdLocate(enID);
    stOsdCfg->stOsdOnOff.bOSDEn = HalSclHvspGetOsdOnOff(enID);
    stOsdCfg->stOsdOnOff.bOSDBypass = HalSclHvspGetOsdBypass(enID);
    stOsdCfg->stOsdOnOff.bWTMBypass = HalSclHvspGetOsdBypassWTM(enID);
}
bool DrvSclHvspGetFrameBufferAttribute(DrvSclHvspIdType_e enID,DrvSclHvspIpmConfig_t *stInformCfg)
{
    bool bLDCorPrvCrop;
    _DrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t*)stInformCfg->pvCtx);
    stInformCfg->bYCMWrite = gstGlobalHvspSet->gstIPMCfg.bYCMWrite;
    stInformCfg->u16Fetch = gstGlobalHvspSet->gstIPMCfg.u16Fetch;
    stInformCfg->u16Vsize = gstGlobalHvspSet->gstIPMCfg.u16Vsize;
    stInformCfg->u32YCBaseAddr = gstGlobalHvspSet->gstIPMCfg.u32YCBaseAddr+0x20000000;
    stInformCfg->u32MemSize = gstGlobalHvspSet->gstIPMCfg.u32MemSize;
    stInformCfg->bYCMRead = gstGlobalHvspSet->gstIPMCfg.bYCMRead;
    bLDCorPrvCrop = (HalSclHvspGetLdcPathSel()) ? 1 :
                    (HalSclHvspGetPrv2CropOnOff())? 2 : 0;
    return bLDCorPrvCrop;
}
bool DrvSclHvspSetPatTgen(bool bEn, DrvSclHvspPatTgenConfig_t *pCfg)
{
    u16 u16VSync_St, u16HSync_St;
    bool bRet = TRUE;
    DrvSclIrqSetPTGenStatus(bEn);
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d)\n", __FUNCTION__,  __LINE__);
    if(bEn)
    {
        u16VSync_St = 1;
        u16HSync_St = 0;
        if(pCfg)
        {
            if((u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1 )<1125)
            {
                HalSclHvspSetPatTgVtt(0xFFFF); //scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
            }
            else
            {
                HalSclHvspSetPatTgVtt(0xFFFF); //rotate
            }
            HalSclHvspSetPatTgVsyncSt(u16VSync_St);
            HalSclHvspSetPatTgVsyncEnd(u16VSync_St + pCfg->u16VSyncWidth - 1);
            HalSclHvspSetPatTgVdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            HalSclHvspSetPatTgVdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);
            HalSclHvspSetPatTgVfdeSt(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch);
            HalSclHvspSetPatTgVfdeEnd(u16VSync_St + pCfg->u16VSyncWidth + pCfg->u16VBackPorch + pCfg->u16VActive - 1);

            HalSclHvspSetPatTgHtt(0xFFFF); // scaling up need bigger Vtt, , using vtt of 1920x1080 for all timing
            HalSclHvspSetPatTgHsyncSt(u16HSync_St);
            HalSclHvspSetPatTgHsyncEnd(u16HSync_St + pCfg->u16HSyncWidth - 1);
            HalSclHvspSetPatTgHdeSt(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch);
            HalSclHvspSetPatTgHdeEnd(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch + pCfg->u16HActive - 1);
            HalSclHvspSetPatTgHfdeSt(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch);
            HalSclHvspSetPatTgHfdeEnd(u16HSync_St + pCfg->u16HSyncWidth + pCfg->u16HBackPorch + pCfg->u16HActive - 1);

            HalSclHvspSetPatTgEn(TRUE);
            bRet = TRUE;
        }
        else
        {
            bRet = FALSE;
        }
    }
    else
    {
        HalSclHvspSetPatTgEn(FALSE);
        bRet = TRUE;
    }
    return bRet;
}
void DrvSclHvspIdclkRelease(DrvSclHvspClkConfig_t* stclk)
{
    if(!DrvSclOsGetClkForceMode())
    HalSclHvspSetIdClkOnOff(0,stclk);
}
void DrvSclHvspGetCrop12Inform(DrvSclHvspInputInformConfig_t *stInformCfg)
{
    stInformCfg->bEn = HalSclHvspGetCrop1En();
    stInformCfg->u16inWidth         = HalSclHvspGetCrop1Width();
    stInformCfg->u16inHeight        = HalSclHvspGetCrop1Height();
    stInformCfg->u16inCropWidth         = HalSclHvspGetCrop2InputWidth();
    stInformCfg->u16inCropHeight        = HalSclHvspGetCrop2InputHeight();
    stInformCfg->u16inCropX         = HalSclHvspGetCropX();
    stInformCfg->u16inCropY        = HalSclHvspGetCropY();
    stInformCfg->u16inWidthcount    = HalSclHvspGetCrop1WidthCount();
    if(stInformCfg->u16inWidthcount)
    {
        stInformCfg->u16inWidthcount++;
    }
    stInformCfg->u16inHeightcount   = HalSclHvspGetCrop1HeightCount();
    stInformCfg->enMux              = HalSclHvspGetInputSrcMux(E_DRV_SCLHVSP_ID_1);
    stInformCfg->enSc3Mux           = HalSclHvspGetInputSrcMux(E_DRV_SCLHVSP_ID_3);
}
void DrvSclHvspSetClkForcemode(u8 bEn)
{
    DrvSclOsSetClkForceMode(bEn);
}
bool DrvSclHvspGetClkForcemode(void)
{
    return DrvSclOsGetClkForceMode();
}
void DrvSclHvspSetClkRate(u8 u8Idx)
{
    if(DrvSclOsGetClkForceMode())
    {
        u8Idx |= E_HALSCLHVSP_CLKATTR_FORCEMODE;
    }
    HalSclHvspSetClkRate(u8Idx);
}
bool DrvSclHvspVtrackSetPayloadData(u16 u16Timecode, u8 u8OperatorID)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    HalSclHvspVtrackSetPayloadData(u16Timecode, u8OperatorID);
    return 1;
}
bool DrvSclHvspVtrackSetKey(bool bUserDefinded, u8 *pu8Setting)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    HalSclHvspVtrackSetKey(bUserDefinded, pu8Setting);
    return 1;
}
bool DrvSclHvspVtrackSetUserDefindedSetting(bool bUserDefinded, u8 *pu8Setting)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    HalSclHvspVtrackSetUserDefindedSetting(bUserDefinded, pu8Setting);
    return 1;
}
bool DrvSclHvspVtrackEnable( u8 u8FrameRate, DrvSclHvspVtrackEnableType_e bEnable)
{
    SCL_DBG(SCL_DBG_LV_DRVVIP(), "[DRVVIP]%s \n", __FUNCTION__);
    HalSclHvspVtrackEnable(u8FrameRate, bEnable);
    return 1;
}

#undef DRV_HVSP_C
