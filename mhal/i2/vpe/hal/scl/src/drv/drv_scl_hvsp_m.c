////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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
#define _MDRV_HVSP_C
#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"
#include "drv_scl_hvsp_io_st.h"
#include "drv_scl_hvsp_st.h"
#include "drv_scl_hvsp.h"
#include "drv_scl_hvsp_m.h"
#include "drv_scl_dma_m.h"
#include "drv_scl_multiinst_m.h"
#include "drv_scl_ctx_m.h"
#include "drv_scl_ctx_st.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define PARSING_PAT_TGEN_TIMING(x)  (x == E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1920_1080_  ?  "E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1920_1080_" : \
                                     x == E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1024_768_6   ?  "E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1024_768_6" : \
                                     x == E_MDRV_SCLHVSP_PAT_TGEN_TIMING_640_480_60    ?  "E_MDRV_SCLHVSP_PAT_TGEN_TIMING_640_480_60" : \
                                     x == E_MDRV_SCLHVSP_PAT_TGEN_TIMING_UNDEFINED     ?  "E_MDRV_SCLHVSP_PAT_TGEN_TIMING_UNDEFINED" : \
                                                                                        "UNKNOWN")
#define IS_WRONG_TYPE(enHVSP_ID,enSrcType)((enHVSP_ID == E_MDRV_SCLHVSP_ID_1 && (enSrcType == E_MDRV_SCLHVSP_SRC_DRAM)) ||\
                        (enHVSP_ID == E_MDRV_SCLHVSP_ID_2 && enSrcType != E_MDRV_SCLHVSP_SRC_HVSP) )
#define Is_PTGEN_FHD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 2200 && (u16Vtotal) == 1125 && (u16Vfrequency) == 30)
#define Is_PTGEN_HD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 1344 && (u16Vtotal) == 806 && (u16Vfrequency) == 60)
#define Is_PTGEN_SD(u16Htotal,u16Vtotal,u16Vfrequency) ((u16Htotal) == 800 && (u16Vtotal) == 525 && (u16Vfrequency) == 60)
#define IS_HVSPNotOpen(u16Src_Width,u16Dsp_Width) (u16Src_Width == 0 && u16Dsp_Width == 0)
#define IS_NotScalingAfterCrop(bEn,u16Dsp_Height,u16Height,u16Dsp_Width,u16Width) (bEn && \
                        (u16Dsp_Height == u16Height) && (u16Dsp_Width== u16Width))
#define RATIO_CONFIG 512
#define CAL_HVSP_RATIO(input,output) ((u32)((u64)((u32)input * RATIO_CONFIG )/(u32)output))
#define SCALE(numerator, denominator,value) ((u16)((u32)(value * RATIO_CONFIG *  numerator) / (denominator * RATIO_CONFIG)))

#define MDrvSclHvspMutexLock()            DrvSclOsObtainMutex(_MHVSP_Mutex,SCLOS_WAIT_FOREVER)
#define MDrvSclHvspMutexUnlock()          DrvSclOsReleaseMutex(_MHVSP_Mutex)
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MDrvSclCtxMhvspGlobalSet_t *gstMhvspGlobalSet;
//keep
DrvSclHvspPatTgenConfig_t gstPatTgenCfg[E_MDRV_SCLHVSP_PAT_TGEN_TIMING_MAX] =
{
    {1125,  4,  5, 36, 1080, 2200, 88,  44, 148, 1920}, // 1920_1080_30
    { 806,  3,  6, 29,  768, 1344, 24, 136, 160, 1024}, // 1024_768_60
    { 525, 33,  2, 10,  480,  800, 16,  96,  48,  640}, // 640_480_60
    {   0, 20, 10, 20,    0,    0, 30,  15,  30,    0}, // undefined
};
s32 _MHVSP_Mutex = -1;
u8 gu8LevelInst = 0xFF;
//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------

void _MDrvSclHvspSetGlobal(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    gstMhvspGlobalSet = &(((MDrvSclCtxGlobalSet_t*)(pvCtx->pgstGlobalSet))->stMhvspCfg);
}
void _MDrvSclHvspSwInit(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    _MDrvSclHvspSetGlobal(pvCtx);
    MDrvSclHvspMutexLock();
    DrvSclOsMemset(&gstMhvspGlobalSet->gstHvspScalingCfg, 0, sizeof(MDrvSclHvspScalingConfig_t));
    DrvSclOsMemset(&gstMhvspGlobalSet->gstHvspPostCropCfg, 0, sizeof(MDrvSclHvspPostCropConfig_t));
    gu8LevelInst = 0xFF;
    MDrvSclHvspMutexUnlock();
}
void _MDrvSclHvspFillIpmStruct(MDrvSclHvspIpmConfig_t *pCfg ,DrvSclHvspIpmConfig_t *stIPMCfg)
{
    stIPMCfg->u16Fetch       = pCfg->u16Width;
    stIPMCfg->u16Vsize       = pCfg->u16Height;
    stIPMCfg->bYCMRead       = (bool)((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_YCM_R);
    stIPMCfg->bYCMWrite      = (bool)(((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_YCM_W)>>1);
    stIPMCfg->bCIIRRead       = (bool)(((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_CIIR_R)>>2);
    stIPMCfg->bCIIRWrite      = (bool)(((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_CIIR_W)>>3);
    stIPMCfg->u32YCBaseAddr    = pCfg->u32YCPhyAddr;
    stIPMCfg->u32MBaseAddr    = pCfg->u32MPhyAddr;
    stIPMCfg->u32CIIRBaseAddr    = pCfg->u32CIIRPhyAddr;
    stIPMCfg->u32MemSize     = pCfg->u32MemSize;
    stIPMCfg->pvCtx          = pCfg->pvCtx;
}


MDrvSclHvspPatTgenTimingType_e _MDrvSclHvspGetPatTgenTiming(MDrvSclHvspTimingConfig_t *pTiming)
{
    MDrvSclHvspPatTgenTimingType_e enTiming;

    if(Is_PTGEN_FHD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1920_1080_;
    }
    else if(Is_PTGEN_HD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_SCLHVSP_PAT_TGEN_TIMING_1024_768_6;
    }
    else if(Is_PTGEN_SD(pTiming->u16Htotal,pTiming->u16Vtotal,pTiming->u16Vfrequency))
    {
        enTiming = E_MDRV_SCLHVSP_PAT_TGEN_TIMING_640_480_60;
    }
    else
    {
        enTiming = E_MDRV_SCLHVSP_PAT_TGEN_TIMING_UNDEFINED;
    }

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "[HVSP]%s(%d) Timing:%s(%d)", __FUNCTION__, __LINE__, PARSING_PAT_TGEN_TIMING(enTiming), enTiming);
    return enTiming;
}

bool _MDrvSclHvspIsInputSrcPatternGen(DrvSclHvspIpMuxType_e enIPMux, MDrvSclHvspInputConfig_t *pCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVHVSP()&(Get_DBGMG_HVSP(0)), "[DRVHVSP]%s(%d)\n", __FUNCTION__,  __LINE__);
    if(enIPMux == E_DRV_SCLHVSP_IP_MUX_PAT_TGEN)
    {
        MDrvSclHvspPatTgenTimingType_e enPatTgenTiming;
        DrvSclHvspPatTgenConfig_t stPatTgenCfg;

        enPatTgenTiming = _MDrvSclHvspGetPatTgenTiming(&pCfg->stTimingCfg);
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(E_MDRV_SCLHVSP_ID_1)),
            "%s:%d type:%d\n", __FUNCTION__,E_MDRV_SCLHVSP_ID_1,enPatTgenTiming);
        DrvSclOsMemcpy(&stPatTgenCfg, &gstPatTgenCfg[enPatTgenTiming],sizeof(DrvSclHvspPatTgenConfig_t));
        if(enPatTgenTiming == E_MDRV_SCLHVSP_PAT_TGEN_TIMING_UNDEFINED)
        {
            stPatTgenCfg.u16HActive = pCfg->stCaptureWin.u16Width;
            stPatTgenCfg.u16VActive = pCfg->stCaptureWin.u16Height;
            stPatTgenCfg.u16Htt = stPatTgenCfg.u16HActive +
                                  stPatTgenCfg.u16HBackPorch +
                                  stPatTgenCfg.u16HFrontPorch +
                                  stPatTgenCfg.u16HSyncWidth;

            stPatTgenCfg.u16Vtt = stPatTgenCfg.u16VActive +
                                  stPatTgenCfg.u16VBackPorch +
                                  stPatTgenCfg.u16VFrontPorch +
                                  stPatTgenCfg.u16VSyncWidth;

        }
        return (bool)DrvSclHvspSetPatTgen(TRUE, &stPatTgenCfg);
    }
    else
    {
        return 0;
    }
}
u16 _MDrvSclHvspGetScaleParameter(u16 u16Src, u16 u16Dsp, u16 u16Val)
{
    //((u16)((u32)(value * RATIO_CONFIG *  numerator) / (denominator * RATIO_CONFIG)))
    u32 u32input;
    u16 u16output;
    u32input = u16Src *u16Val;
    u16output = (u16)(u32input/u16Dsp);
    if((u32input%u16Dsp))
    {
        u16output ++;
    }
    return u16output;
}
static void  _MDrvSclHvspSetRegisterForce(u32 u32Size, u8 *pBuf,void *pvCtx)
{
    u32 i;
    u32 u32Reg;
    u16 u16Bank;
    u8  u8Addr, u8Val, u8Msk;

    // bank,  addrr,  val,  msk
    for(i=0; i<u32Size; i+=5)
    {
        u16Bank = (u16)pBuf[i+0] | ((u16)pBuf[i+1])<<8;
        u8Addr  = pBuf[i+2];
        u8Val   = pBuf[i+3];
        u8Msk   = pBuf[i+4];
        u32Reg  = (((u32)u16Bank) << 8) | (u32)u8Addr;

        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%08lx, %02x, %02x\n", u32Reg, u8Val, u8Msk);
        DrvSclHvspSetRegisterForce(u32Reg, u8Val, u8Msk,pvCtx);
    }
}
static void  _MDrvSclHvspSetRegisterForceByInst(u32 u32Size, u8 *pBuf,void *pvCtx)
{
    u32 i;
    u32 u32Reg;
    u16 u16Bank,inst;
    u8  u8Addr, u8Val, u8Msk;
    s32 s32Handler;
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    inst = *(u16 *)pvCtx;
    pCtxCfg = MDrvSclCtxGetCtx(inst);
    if(pCtxCfg->bUsed)
    {
        for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
        {
            s32Handler = pCtxCfg->s32Id[i];
            if(inst<MDRV_SCL_CTX_INSTANT_MAX && ((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX))
            {
                break;
            }
            else if(inst>=MDRV_SCL_CTX_INSTANT_MAX&& ((s32Handler&HANDLER_PRE_MASK)==SCLVIP_HANDLER_PRE_FIX))
            {
                break;
            }
        }
        MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        // bank,  addrr,  val,  msk
        for(i=0; i<u32Size; i+=5)
        {
            u16Bank = (u16)pBuf[i+0] | ((u16)pBuf[i+1])<<8;
            u8Addr  = pBuf[i+2];
            u8Val   = pBuf[i+3];
            u8Msk   = pBuf[i+4];
            u32Reg  = (((u32)u16Bank) << 8) | (u32)u8Addr;
            SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%08lx, %02x, %02x\n", u32Reg, u8Val, u8Msk);
            DrvSclHvspSetRegisterForceByInst(u32Reg, u8Val, u8Msk,&pCtxCfg->stCtx);
        }
        MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
    }
    else
    {
        SCL_ERR("[Test]%s Inst Err\n",__FUNCTION__);
    }
}
static void  _MDrvSclHvspSetRegisterForceByAllInst(u32 u32Size, u8 *pBuf,void *pvCtx)
{
    u32 i,j;
    u32 u32Reg;
    u16 u16Bank;
    u8  u8Addr, u8Val, u8Msk;
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    s32 s32Handler;
    // bank,  addrr,  val,  msk
    for(j=0; j<MDRV_SCL_CTX_INSTANT_MAX*E_MDRV_SCL_CTX_ID_NUM; j++)
    {
        pCtxCfg = MDrvSclCtxGetCtx((u16)j);
        if(pCtxCfg->bUsed)
        {
            for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
            {
                s32Handler = pCtxCfg->s32Id[i];
                if(j<MDRV_SCL_CTX_INSTANT_MAX && ((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX))
                {
                    break;
                }
                else if(j>=MDRV_SCL_CTX_INSTANT_MAX&& ((s32Handler&HANDLER_PRE_MASK)==SCLVIP_HANDLER_PRE_FIX))
                {
                    break;
                }
            }
            MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
            for(i=0; i<u32Size; i+=5)
            {
                u16Bank = (u16)pBuf[i+0] | ((u16)pBuf[i+1])<<8;
                u8Addr  = pBuf[i+2];
                u8Val   = pBuf[i+3];
                u8Msk   = pBuf[i+4];
                u32Reg  = (((u32)u16Bank) << 8) | (u32)u8Addr;

                SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%08lx, %02x, %02x\n", u32Reg, u8Val, u8Msk);
                DrvSclHvspSetRegisterForceByInst(u32Reg, u8Val, u8Msk,&pCtxCfg->stCtx);
            }
            MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        }
    }
}
bool _MDrvSclHvspSetPollWait(DrvSclOsPollWaitConfig_t *stPollWait)
{
    return (bool)DrvSclOsSetPollWait(stPollWait);
}
//---------------------------------------------------------------------------------------------------------
// IOCTL function
//---------------------------------------------------------------------------------------------------------
void MDrvSclHvspRelease(MDrvSclHvspIdType_e enHVSP_ID,void *pvCtx)
{
    DrvSclHvspIdType_e enID;
    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;
    _MDrvSclHvspSwInit((MDrvSclCtxCmdqConfig_t*)pvCtx);
    DrvSclHvspRelease(enID,pvCtx);
}
void MDrvSclHvspReSetHw(void *pvCtx)
{
    DrvSclHvspReSetHw(pvCtx);
}
void MDrvSclHvspOpen(MDrvSclHvspIdType_e enHVSP_ID)
{
    DrvSclHvspIdType_e enID;
    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;
    DrvSclHvspOpen(enID);
}
bool MDrvSclHvspExit(bool bCloseISR)
{
    if(_MHVSP_Mutex != -1)
    {
         DrvSclOsDeleteMutex(_MHVSP_Mutex);
         _MHVSP_Mutex = -1;
    }
    DrvSclHvspExit(bCloseISR);
    MDrvSclCtxDeInit();
    return 1;
}
bool MDrvSclHvspInit(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspInitConfig_t *pCfg)
{
    DrvSclHvspInitConfig_t stInitCfg;
    char word[] = {"_MHVSP_Mutex"};
    DrvSclOsMemset(&stInitCfg,0,sizeof(DrvSclHvspInitConfig_t));
    if(DrvSclOsInit() == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[MDRVHVSP]%s(%d) init SclOS fail\n",
            __FUNCTION__, __LINE__);
        return FALSE;
    }
    if (_MHVSP_Mutex != -1)
    {
        SCL_DBG(SCL_DBG_LV_DRVHVSP(), "[MDRVHVSP]%s(%d) alrady done\n", __FUNCTION__, __LINE__);
        return 1;
    }
    _MHVSP_Mutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, word, SCLOS_PROCESS_SHARED);

    if (_MHVSP_Mutex == -1)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[DRVHVSP]%s(%d): create mutex fail\n",
            __FUNCTION__, __LINE__);
        return FALSE;
    }
    stInitCfg.u32RIUBase    = pCfg->u32Riubase;
    stInitCfg.u32IRQNUM     = pCfg->u32IRQNUM;
    stInitCfg.u32CMDQIRQNUM = pCfg->u32CMDQIRQNUM;
    stInitCfg.pvCtx = pCfg->pvCtx;
    DrvSclOsSetSclFrameBufferNum(DNR_BUFFER_MODE);
    if(DrvSclHvspInit(&stInitCfg) == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Init Fail\n", __FUNCTION__);
        return FALSE;
    }
    else
    {
        MDrvSclHvspVtrackInit();
        _MDrvSclHvspSwInit((MDrvSclCtxCmdqConfig_t *)pCfg->pvCtx);
        return TRUE;
    }

}
void * MDrvSclHvspGetWaitQueueHead(void)
{
    return DrvSclHvspGetWaitQueueHead();
}
bool MDrvSclHvspSuspend(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspSuspendResumeConfig_t *pCfg)
{
    DrvSclHvspSuspendResumeConfig_t stSuspendResumeCfg;
    bool bRet = TRUE;;
    DrvSclOsMemset(&stSuspendResumeCfg,0,sizeof(DrvSclHvspSuspendResumeConfig_t));
    stSuspendResumeCfg.u32IRQNUM = pCfg->u32IRQNum;
    stSuspendResumeCfg.u32CMDQIRQNUM = pCfg->u32CMDQIRQNum;
    if(DrvSclHvspSuspend(&stSuspendResumeCfg))
    {
        bRet = TRUE;
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Suspend Fail\n", __FUNCTION__);
        bRet = FALSE;
    }

    return bRet;
}

bool MDrvSclHvspResume(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspSuspendResumeConfig_t *pCfg)
{
    DrvSclHvspSuspendResumeConfig_t stSuspendResumeCfg;
    bool bRet = TRUE;;
    DrvSclOsMemset(&stSuspendResumeCfg,0,sizeof(DrvSclHvspSuspendResumeConfig_t));
    stSuspendResumeCfg.u32IRQNUM = pCfg->u32IRQNum;
    stSuspendResumeCfg.u32CMDQIRQNUM = pCfg->u32CMDQIRQNum;
    if(DrvSclHvspResume(&stSuspendResumeCfg))
    {
        MDrvSclHvspVtrackInit();
        bRet = TRUE;
    }
    else
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP]%s Resume Fail\n", __FUNCTION__);
        bRet = FALSE;
    }

    return bRet;
}
bool MDrvSclHvspSetInitIpmConfig(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspIpmConfig_t *pCfg)
{
    DrvSclHvspIpmConfig_t stIPMCfg;
    DrvSclOsMemset(&stIPMCfg,0,sizeof(DrvSclHvspIpmConfig_t));
    if(enHVSP_ID != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]%s ID not correct: %d\n", __FUNCTION__, enHVSP_ID);
        return FALSE;
    }
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d:PhyAddr=%lx, width=%x, height=%x \n",  __FUNCTION__,enHVSP_ID, pCfg->u32YCPhyAddr, pCfg->u16Width, pCfg->u16Height);
    _MDrvSclHvspFillIpmStruct(pCfg,&stIPMCfg);
    // ToDo
    // DrvSclHvspSetPrv2CropOnOff(stIPMCfg.bYCMRead,);
    if(DrvSclHvspSetIPMConfig(&stIPMCfg) == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP] Set IPM Config Fail\n");
        return FALSE;
    }
    else
    {
        return TRUE;;
    }
}
void _MDrvSclHvspSetPatTgenStatusByInst(bool u8inst)
{
    MDrvSclHvspInputConfig_t stCfg;
    DrvSclHvspInputInformConfig_t stInformCfg;
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    s32 s32Handler;
    u32 i;
    if(u8inst>=MDRV_SCL_CTX_INSTANT_MAX)
    {
        u8inst = 0;
    }
    pCtxCfg = MDrvSclCtxGetCtx(u8inst);
    if(pCtxCfg->bUsed)
    {
        for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
        {
            s32Handler = pCtxCfg->s32Id[i];
            if((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX)
            {
                break;
            }
        }
        DrvSclOsMemset(&stInformCfg,0,sizeof(DrvSclHvspInputInformConfig_t));
        MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        DrvSclHvspGetCrop12Inform(&stInformCfg);
        stCfg.enColor = E_MDRV_SCLHVSP_COLOR_RGB;
        stCfg.enSrcType = E_MDRV_SCLHVSP_SRC_PAT_TGEN;
        stCfg.pvCtx = &(pCtxCfg->stCtx);
        stCfg.stCaptureWin.u16Height = stInformCfg.u16inCropHeight;
        stCfg.stCaptureWin.u16Width= stInformCfg.u16inCropWidth;
        MDrvSclHvspSetInputConfig(E_MDRV_SCLHVSP_ID_1,&stCfg);
        MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
    }
}
void MDrvSclHvspSetPatTgenStatus(bool bEn)
{
    u32 i;
    if(bEn)
    {
        for(i=0;i<MDRV_SCL_CTX_INSTANT_MAX;i++)
        {
            _MDrvSclHvspSetPatTgenStatusByInst((u8)i);
        }
    }
    else
    {
        _MDrvSclHvspSetPatTgenStatusByInst(gu8LevelInst);
    }
}
bool MDrvSclHvspSetInputConfig(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspInputConfig_t *pCfg)
{
    bool Ret = TRUE;
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d\n", __FUNCTION__,enHVSP_ID);
    if(enHVSP_ID == E_MDRV_SCLHVSP_ID_1)
    {
        DrvSclHvspIpMuxType_e enIPMux;
        ST_HVSP_SIZE_CONFIG stSize;
        DrvSclOsMemset(&stSize,0,sizeof(ST_HVSP_SIZE_CONFIG));
        enIPMux = pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_ISP      ? E_DRV_SCLHVSP_IP_MUX_ISP :
                  pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_BT656    ? E_DRV_SCLHVSP_IP_MUX_BT656 :
                  pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_HVSP     ? E_DRV_SCLHVSP_IP_MUX_HVSP :
                  pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_PAT_TGEN ? E_DRV_SCLHVSP_IP_MUX_PAT_TGEN :
                                                                 E_DRV_SCLHVSP_IP_MUX_MAX;
        Ret &= (bool)DrvSclHvspSetInputMux(enIPMux,(DrvSclHvspClkConfig_t *)pCfg->stclk,pCfg->pvCtx);
        stSize.u16Height = pCfg->stCaptureWin.u16Height;
        stSize.u16Width = pCfg->stCaptureWin.u16Width;
        stSize.pvCtx = pCfg->pvCtx;
        DrvSclHvspSetInputSrcSize(&stSize);
        //DrvSclHvspSetCropWindowSize(pCfg->pvCtx);
        if(_MDrvSclHvspIsInputSrcPatternGen(enIPMux, pCfg))
        {
            Ret &= TRUE;
        }
        else
        {
            Ret &= (bool)DrvSclHvspSetPatTgen(FALSE, NULL);
        }

    }
    else if(enHVSP_ID == E_MDRV_SCLHVSP_ID_3)
    {
        DrvSclHvspIpMuxType_e enIPMux;
        enIPMux = pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_HVSP      ? E_DRV_SCLHVSP_IP_MUX_HVSP :
                  pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_DRAM      ? E_DRV_SCLHVSP_IP_MUX_LDC :
                  pCfg->enSrcType == E_MDRV_SCLHVSP_SRC_DRAM_RSC  ? E_DRV_SCLHVSP_IP_MUX_RSC :
                                                                 E_DRV_SCLHVSP_IP_MUX_MAX;
        if(enIPMux==E_DRV_SCLHVSP_IP_MUX_MAX)
        {
            return 0;
        }
        DrvSclHvspSetSc3InputMux(enIPMux,pCfg->pvCtx);
    }
    return Ret;
}

bool MDrvSclHvspSetPostCropConfig(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspPostCropConfig_t *pCfg)
{
    DrvSclHvspIdType_e enID;
    DrvSclHvspScalingConfig_t stScalingCfg;
    bool ret;
    DrvSclOsMemset(&stScalingCfg,0,sizeof(DrvSclHvspScalingConfig_t));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s\n", __FUNCTION__);

    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;

    MDrvSclHvspMutexLock();
    _MDrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t *)pCfg->pvCtx);
    DrvSclOsMemcpy(&gstMhvspGlobalSet->gstHvspPostCropCfg, pCfg, sizeof(MDrvSclHvspPostCropConfig_t));
    stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = 0;
    stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = pCfg->bCropEn;
    stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = pCfg->u16X;
    stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = pCfg->u16Y;
    stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->u16Width;
    stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->u16Height;

    stScalingCfg.u16Src_Width  = gstMhvspGlobalSet->gstHvspScalingCfg.u16Src_Width;
    stScalingCfg.u16Src_Height = gstMhvspGlobalSet->gstHvspScalingCfg.u16Src_Height;
    stScalingCfg.u16Dsp_Width  = gstMhvspGlobalSet->gstHvspScalingCfg.u16Dsp_Width;
    stScalingCfg.u16Dsp_Height = gstMhvspGlobalSet->gstHvspScalingCfg.u16Dsp_Height;
    stScalingCfg.pvCtx = pCfg->pvCtx;
    MDrvSclHvspMutexUnlock();
    ret = DrvSclHvspSetScaling(enID, &stScalingCfg,(DrvSclHvspClkConfig_t *)pCfg->stclk);
    return  ret;


}

bool MDrvSclHvspSetScalingConfig(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspScalingConfig_t *pCfg )
{
    DrvSclHvspIdType_e enID;
    DrvSclHvspScalingConfig_t stScalingCfg;
    bool ret;
    DrvSclOsMemset(&stScalingCfg,0,sizeof(DrvSclHvspScalingConfig_t));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "%s:%d\n", __FUNCTION__,enHVSP_ID);
    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;

    if(enID == E_DRV_SCLHVSP_ID_1)
    {
       MDrvSclHvspMutexLock();
       _MDrvSclHvspSetGlobal((MDrvSclCtxCmdqConfig_t *)pCfg->pvCtx);
       DrvSclOsMemcpy(&gstMhvspGlobalSet->gstHvspScalingCfg, pCfg, sizeof(MDrvSclHvspScalingConfig_t));
       MDrvSclHvspMutexUnlock();
    }

    if( SetPostCrop || (DrvSclHvspGetInputSrcMux(enID)==E_DRV_SCLHVSP_IP_MUX_PAT_TGEN))
    {
        stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = 0;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_1]      = 0;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_1]      = 0;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_1]  = pCfg->u16Src_Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_1] = pCfg->u16Src_Height;
        stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = pCfg->stCropWin.bEn;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16X;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = pCfg->stCropWin.u16Y;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->stCropWin.u16Height;
    }
    else
    {
        stScalingCfg.bCropEn[DRV_HVSP_CROP_1]        = pCfg->stCropWin.bEn;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_1]      = pCfg->stCropWin.u16X;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_1]      = pCfg->stCropWin.u16Y;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_1]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_1] = pCfg->stCropWin.u16Height;
        stScalingCfg.bCropEn[DRV_HVSP_CROP_2]        = 0;
        stScalingCfg.u16Crop_X[DRV_HVSP_CROP_2]      = 0;
        stScalingCfg.u16Crop_Y[DRV_HVSP_CROP_2]      = 0;
        stScalingCfg.u16Crop_Width[DRV_HVSP_CROP_2]  = pCfg->stCropWin.u16Width;
        stScalingCfg.u16Crop_Height[DRV_HVSP_CROP_2] = pCfg->stCropWin.u16Height;
    }
    stScalingCfg.u16Src_Width  = pCfg->u16Src_Width;
    stScalingCfg.u16Src_Height = pCfg->u16Src_Height;
    stScalingCfg.u16Dsp_Width  = pCfg->u16Dsp_Width;
    stScalingCfg.u16Dsp_Height = pCfg->u16Dsp_Height;
    stScalingCfg.pvCtx = pCfg->pvCtx;
    ret = DrvSclHvspSetScaling(enID, &stScalingCfg,(DrvSclHvspClkConfig_t *)pCfg->stclk);

    return  ret;
}




bool MDrvSclHvspSetMiscConfig(MDrvSclHvspMiscConfig_t *pCfg)
{
    u8 *pBuf = NULL;

    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);

    pBuf = DrvSclOsVirMemalloc(pCfg->u32Size);

    if(pBuf == NULL)
    {
        SCL_ERR( "[HVSP1] allocate buffer fail\n");
        return 0;
    }


    if(DrvSclOsCopyFromUser(pBuf, (void *)pCfg->u32Addr, pCfg->u32Size))
    {
        SCL_ERR( "[HVSP1] copy msic buffer error\n");
        DrvSclOsVirMemFree(pBuf);
        return 0;
    }

    switch(pCfg->u8Cmd)
    {
        case E_MDRV_SCLHVSP_MISC_CMD_SET_REG:
            _MDrvSclHvspSetRegisterForce(pCfg->u32Size, pBuf,pCfg->pvCtx);
            break;

        default:
            break;
    }

    DrvSclOsVirMemFree(pBuf);

    return 1;
}

bool MDrvSclHvspSetMiscConfigForKernel(MDrvSclHvspMiscConfig_t *pCfg)
{
    u16 u16inst;
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
     switch(pCfg->u8Cmd)
     {
         case E_MDRV_SCLHVSP_MISC_CMD_SET_REG:
             _MDrvSclHvspSetRegisterForce(pCfg->u32Size, (u8 *)pCfg->u32Addr,pCfg->pvCtx);
             break;
         case E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINSTALL:
             _MDrvSclHvspSetRegisterForceByAllInst(pCfg->u32Size, (u8 *)pCfg->u32Addr,pCfg->pvCtx);
             break;
         case E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINST:
             u16inst = gu8LevelInst;
             if(gu8LevelInst==0xFF)
             {
                u16inst = 0;
             }
             _MDrvSclHvspSetRegisterForceByInst(pCfg->u32Size, (u8 *)pCfg->u32Addr,(void*)&u16inst);
             break;
         default:
             break;
     }
     return 1;
}

bool MDrvSclHvspGetSCLInform(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspScInformConfig_t *pstCfg)
{
     DrvSclHvspScInformConfig_t stInformCfg;
     DrvSclHvspIdType_e enID;
     bool bRet = 1;
     DrvSclOsMemset(&stInformCfg,0,sizeof(DrvSclHvspScInformConfig_t));
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)), "[HVSP1]%s\n", __FUNCTION__);
     enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
            enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
            enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                            E_DRV_SCLHVSP_ID_1;

     stInformCfg.pvCtx = pstCfg->pvCtx;
     DrvSclHvspGetSCLInform(enID, &stInformCfg);
     DrvSclOsMemcpy(pstCfg, &stInformCfg, sizeof(MDrvSclHvspScInformConfig_t));
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enHVSP_ID)),
        "[HVSP1]u16Height:%hd u16Width:%hd\n",stInformCfg.u16Height, stInformCfg.u16Width);
     return bRet;
}

bool MDrvSclHvspSetOsdConfig(MDrvSclHvspIdType_e enHVSP_ID, MDrvSclHvspOsdConfig_t* pstCfg)
{
     bool Ret = TRUE;
     DrvSclHvspOsdConfig_t stOSdCfg;
     DrvSclHvspIdType_e enID;
     DrvSclOsMemset(&stOSdCfg,0,sizeof(DrvSclHvspOsdConfig_t));
     enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
            enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
            enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                            E_DRV_SCLHVSP_ID_1;
     DrvSclOsMemcpy(pstCfg, &stOSdCfg, sizeof(MDrvSclHvspOsdConfig_t));
     DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
     SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(enID)),
        "%s::ID:%d, OnOff:%hhd ,Bypass:%hhd\n",
        __FUNCTION__,enID,stOSdCfg.stOsdOnOff.bOSDEn,stOSdCfg.stOsdOnOff.bOSDBypass);
     return Ret;
}

bool MDrvSclHvspSetFbManageConfig(MDrvSclHvspFbmgSetType_e enSet)
{
    bool Ret = TRUE;
    DrvSclHvspSetFbManageConfig_t stCfg;
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclHvspSetFbManageConfig_t));
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    stCfg.enSet = enSet;
    if(stCfg.enSet &E_DRV_SCLHVSP_FBMG_SET_DNR_BUFFER_1)
    {
        DrvSclOsSetSclFrameBufferNum(1);
    }
    else if (stCfg.enSet &E_DRV_SCLHVSP_FBMG_SET_DNR_BUFFER_2)
    {
        DrvSclOsSetSclFrameBufferNum(2);
    }
    DrvSclHvspSetFbManageConfig(&stCfg);
    return Ret;
}
void MDrvSclHvspIdclkRelease(MDrvSclHvspClkConfig_t* stclk)
{
    SCL_DBG(SCL_DBG_LV_HVSP()&(Get_DBGMG_HVSP(3)), "%s\n", __FUNCTION__);
    DrvSclHvspIdclkRelease((DrvSclHvspClkConfig_t *)stclk);
}

ssize_t MDrvSclHvspProcShowInst(char *buf,u8 u8inst)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    DrvSclHvspInformConfig_t sthvspformCfg;
    DrvSclHvspInformConfig_t sthvsp2formCfg;
    DrvSclHvspInformConfig_t sthvsp3formCfg;
    DrvSclHvspInformConfig_t sthvsp4formCfg;
    DrvSclHvspScInformConfig_t stzoomformCfg;
    DrvSclHvspIpmConfig_t stIpmformCfg;
    DrvSclHvspInputInformConfig_t stInformCfg;
    s32 s32Handler;
    u32 i;
    int u32idx;
    MDrvSclDmaGetConfig_t stGetCfg;
    MDrvSclDmaAttrType_t stScldmaAttr;
    MDrvSclDmaAttrType_t stScldmaAttr1;
    MDrvSclDmaAttrType_t stScldmaAttr2;
    MDrvSclDmaAttrType_t stScldmaAttr3;
    MDrvSclDmaAttrType_t stScldmaAttr4;
    MDrvSclDmaAttrType_t stScldmaAttr5;
    MDrvSclDmaAttrType_t stScldmaAttr6;
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    pCtxCfg = MDrvSclCtxGetCtx(u8inst);
    if(pCtxCfg->bUsed)
    {
        for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
        {
            s32Handler = pCtxCfg->s32Id[i];
            if((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX)
            {
                break;
            }
        }
        DrvSclOsMemset(&sthvspformCfg,0,sizeof(DrvSclHvspInformConfig_t));
        DrvSclOsMemset(&sthvsp2formCfg,0,sizeof(DrvSclHvspInformConfig_t));
        DrvSclOsMemset(&sthvsp3formCfg,0,sizeof(DrvSclHvspInformConfig_t));
        DrvSclOsMemset(&sthvsp4formCfg,0,sizeof(DrvSclHvspInformConfig_t));
        DrvSclOsMemset(&stzoomformCfg,0,sizeof(DrvSclHvspScInformConfig_t));
        DrvSclOsMemset(&stIpmformCfg,0,sizeof(DrvSclHvspIpmConfig_t));
        DrvSclOsMemset(&stInformCfg,0,sizeof(DrvSclHvspInputInformConfig_t));
        //out =0,in=1
        DrvSclOsMemset(&stGetCfg,0,sizeof(MDrvSclDmaGetConfig_t));
        MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        sthvspformCfg.pvCtx = &(pCtxCfg->stCtx);
        sthvsp2formCfg.pvCtx = &(pCtxCfg->stCtx);
        sthvsp3formCfg.pvCtx = &(pCtxCfg->stCtx);
        sthvsp4formCfg.pvCtx = &(pCtxCfg->stCtx);
        stIpmformCfg.pvCtx = &(pCtxCfg->stCtx);
        DrvSclHvspGetCrop12Inform(&stInformCfg);
        DrvSclHvspGetSCLInform(E_DRV_SCLHVSP_ID_1,&stzoomformCfg);
        DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_1,&sthvspformCfg);
        DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_2,&sthvsp2formCfg);
        DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_3,&sthvsp3formCfg);
        DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_4,&sthvsp4formCfg);
        DrvSclHvspGetFrameBufferAttribute(E_DRV_SCLHVSP_ID_1,&stIpmformCfg);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_1;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRM;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_1;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_SNP;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr1);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_2;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRM;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr2);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_2;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRM2;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr3);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_3;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRM;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr4);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_3;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRMR;
        stGetCfg.bReadDMAMode = 1;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr5);
        stGetCfg.enSCLDMA_ID = E_MDRV_SCLDMA_ID_MDWIN;
        stGetCfg.enMemType = E_MDRV_SCLDMA_MEM_FRM;
        stGetCfg.bReadDMAMode = 0;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stScldmaAttr6);
        MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        SCL_ERR("========================SCL PROC FRAMEWORK======================\n");
        SCL_ERR("Inst:%hhu\n",u8inst);
        SCL_ERR("Read Inst:%hhu\n",gu8LevelInst);
        SCL_ERR("------------------------SCL INPUT MUX----------------------\n");
        if(stInformCfg.enMux==0)
        {
            SCL_ERR("Input SRC :BT656\n");
        }
        else if(stInformCfg.enMux==1)
        {
            SCL_ERR("Input SRC :ISP\n");
        }
        else if(stInformCfg.enMux==3)
        {
            SCL_ERR("Input SRC :PTGEN\n");
        }
        else
        {
            SCL_ERR("Input SRC :OTHER\n");
        }
        if(stInformCfg.enSc3Mux==2)
        {
            SCL_ERR("SC3 SRC :HVSP\n");
        }
        else if(stInformCfg.enSc3Mux==4)
        {
            SCL_ERR("SC3 SRC :LDC\n");
        }
        else if(stInformCfg.enSc3Mux==5)
        {
            SCL_ERR("SC3 SRC :RSC\n");
        }
        else
        {
            SCL_ERR("SC3 SRC :OTHER\n");
        }
        SCL_ERR("Input H   :%hd\n",stInformCfg.u16inWidth);
        SCL_ERR("Input V   :%hd\n",stInformCfg.u16inHeight);
        SCL_ERR("(only for single ch)Receive H :%hd\n",stInformCfg.u16inWidthcount);
        SCL_ERR("(only for single ch)Receive V :%hd\n",stInformCfg.u16inHeightcount);
        SCL_ERR("------------------------SCL FB-----------------------------\n");
        SCL_ERR("FB H          :%hd\n",stIpmformCfg.u16Fetch);
        SCL_ERR("FB V          :%hd\n",stIpmformCfg.u16Vsize);
        SCL_ERR("FB Addr       :%lx\n",stIpmformCfg.u32YCBaseAddr);
        SCL_ERR("FB memsize    :%ld\n",stIpmformCfg.u32MemSize);
        SCL_ERR("FB Buffer     :%hhd\n",DrvSclOsGetSclFrameBufferNum());
        SCL_ERR("FRAME DELAY     :%hhd\n",DrvSclOsGetSclFrameDelay());
        SCL_ERR("------------------------SCL Crop----------------------------\n");
        SCL_ERR("Crop      :%hhd\n",stInformCfg.bEn);
        SCL_ERR("CropX     :%hd\n",stInformCfg.u16inCropX);
        SCL_ERR("CropY     :%hd\n",stInformCfg.u16inCropY);
        SCL_ERR("CropOutW  :%hd\n",stInformCfg.u16inCropWidth);
        SCL_ERR("CropOutH  :%hd\n",stInformCfg.u16inCropHeight);
        SCL_ERR("SrcW      :%hd\n",stInformCfg.u16inWidth);
        SCL_ERR("SrcH      :%hd\n",stInformCfg.u16inHeight);
        SCL_ERR("------------------------SCL Zoom----------------------------\n");
        SCL_ERR("Zoom      :%hhd\n",stzoomformCfg.bEn);
        SCL_ERR("ZoomX     :%hd\n",stzoomformCfg.u16X);
        SCL_ERR("ZoomY     :%hd\n",stzoomformCfg.u16Y);
        SCL_ERR("ZoomOutW  :%hd\n",stzoomformCfg.u16crop2OutWidth);
        SCL_ERR("ZoomOutH  :%hd\n",stzoomformCfg.u16crop2OutHeight);
        SCL_ERR("SrcW      :%hd\n",stzoomformCfg.u16crop2inWidth);
        SCL_ERR("SrcH      :%hd\n",stzoomformCfg.u16crop2inHeight);
        SCL_ERR("------------------------SCL HVSP1----------------------------\n");
        SCL_ERR("InputH    :%hd\n",sthvspformCfg.u16inWidth);
        SCL_ERR("InputV    :%hd\n",sthvspformCfg.u16inHeight);
        SCL_ERR("OutputH   :%hd\n",sthvspformCfg.u16Width);
        SCL_ERR("OutputV   :%hd\n",sthvspformCfg.u16Height);
        SCL_ERR("H en  :%hhx\n",sthvspformCfg.bEn&0x1);
        SCL_ERR("H function  :%hhx\n",(sthvspformCfg.bEn&0xC0)>>6);
        SCL_ERR("V en  :%hhx\n",(sthvspformCfg.bEn&0x2)>>1);
        SCL_ERR("V function  :%hhx\n",(sthvspformCfg.bEn&0x30)>>4);
        SCL_ERR("------------------------SCL HVSP2----------------------------\n");
        SCL_ERR("InputH    :%hd\n",sthvsp2formCfg.u16inWidth);
        SCL_ERR("InputV    :%hd\n",sthvsp2formCfg.u16inHeight);
        SCL_ERR("OutputH   :%hd\n",sthvsp2formCfg.u16Width);
        SCL_ERR("OutputV   :%hd\n",sthvsp2formCfg.u16Height);
        SCL_ERR("H en  :%hhx\n",sthvsp2formCfg.bEn&0x1);
        SCL_ERR("H function  :%hhx\n",(sthvsp2formCfg.bEn&0xC0)>>6);
        SCL_ERR("V en  :%hhx\n",(sthvsp2formCfg.bEn&0x2)>>1);
        SCL_ERR("V function  :%hhx\n",(sthvsp2formCfg.bEn&0x30)>>4);
        SCL_ERR("------------------------SCL HVSP3----------------------------\n");
        SCL_ERR("InputH    :%hd\n",sthvsp3formCfg.u16inWidth);
        SCL_ERR("InputV    :%hd\n",sthvsp3formCfg.u16inHeight);
        SCL_ERR("OutputH   :%hd\n",sthvsp3formCfg.u16Width);
        SCL_ERR("OutputV   :%hd\n",sthvsp3formCfg.u16Height);
        SCL_ERR("H en  :%hhx\n",sthvsp3formCfg.bEn&0x1);
        SCL_ERR("H function  :%hhx\n",(sthvsp3formCfg.bEn&0xC0)>>6);
        SCL_ERR("V en  :%hhx\n",(sthvsp3formCfg.bEn&0x2)>>1);
        SCL_ERR("V function  :%hhx\n",(sthvsp3formCfg.bEn&0x30)>>4);
        SCL_ERR("------------------------SCL HVSP4----------------------------\n");
        SCL_ERR("InputH    :%hd\n",sthvsp4formCfg.u16inWidth);
        SCL_ERR("InputV    :%hd\n",sthvsp4formCfg.u16inHeight);
        SCL_ERR("OutputH   :%hd\n",sthvsp4formCfg.u16Width);
        SCL_ERR("OutputV   :%hd\n",sthvsp4formCfg.u16Height);
        SCL_ERR("H en  :%hhx\n",sthvsp4formCfg.bEn&0x1);
        SCL_ERR("H function  :%hhx\n",(sthvsp4formCfg.bEn&0xC0)>>6);
        SCL_ERR("V en  :%hhx\n",(sthvsp4formCfg.bEn&0x2)>>1);
        SCL_ERR("V function  :%hhx\n",(sthvsp4formCfg.bEn&0x30)>>4);
        SCL_ERR("------------------------SCL DMA1FRM----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr.bVFilp);
        SCL_ERR("------------------------SCL DMA1SNP----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr1.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr1.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr1.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr1.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr1.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr1.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr1.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr1.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr1.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr1.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr1.bVFilp);
        SCL_ERR("------------------------SCL DMA2FRM----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr2.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr2.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr2.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr2.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr2.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr2.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr2.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr2.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr2.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr2.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr2.bVFilp);
        SCL_ERR("------------------------SCL DMA2FRM2----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr3.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr3.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr3.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr3.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr3.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr3.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr3.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr3.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr3.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr3.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr3.bVFilp);
        SCL_ERR("------------------------SCL DMA3FRMW----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr4.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr4.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr4.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr4.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr4.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr4.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr4.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr4.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr4.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr4.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr4.bVFilp);
        SCL_ERR("------------------------SCL DMA3FRMR----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr5.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr5.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr5.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr5.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr5.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr5.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr5.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr5.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr5.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr5.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr5.bVFilp);
        SCL_ERR("------------------------SCL MDWIN----------------------------\n");
        SCL_ERR("DMA Enable:%hhd\n",stScldmaAttr6.bDMAEn);
        SCL_ERR("DMA color format: %s\n",PARSING_SCLDMA_IOCOLOR(stScldmaAttr6.enColorType));
        SCL_ERR("DMA trigger mode: %s\n",PARSING_SCLDMA_IOBUFMD(stScldmaAttr6.enBufMDType));
        SCL_ERR("DMA Buffer Num: %hd\n",stScldmaAttr6.u16BufNum);
        for(u32idx=0 ;u32idx<stScldmaAttr6.u16BufNum;u32idx++)
        {
            SCL_ERR("DMA Buffer Y Address[%d]: %lx\n",u32idx,stScldmaAttr6.u32Base_Y[u32idx]);
            SCL_ERR("DMA Buffer C Address[%d]: %lx\n",u32idx,stScldmaAttr6.u32Base_C[u32idx]);
            SCL_ERR("DMA Buffer V Address[%d]: %lx\n",u32idx,stScldmaAttr6.u32Base_V[u32idx]);
        }
        SCL_ERR("DMA Stride: %lu\n",stScldmaAttr6.u32LineOffset_Y);
        SCL_ERR("DMA Mirror: %hhu\n",stScldmaAttr6.bHFilp);
        SCL_ERR("DMA Flip: %hhu\n",stScldmaAttr6.bVFilp);
        SCL_ERR("========================SCL PROC FRAMEWORK======================\n");
        end = end;
    }
    return (str - buf);
}
#if 0
ssize_t MDrvSclHvspProcShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    DrvSclHvspInformConfig_t sthvspformCfg;
    DrvSclHvspInformConfig_t sthvsp2formCfg;
    DrvSclHvspInformConfig_t sthvsp3formCfg;
    DrvSclHvspScInformConfig_t stzoomformCfg;
    DrvSclHvspIpmConfig_t stIpmformCfg;
    DrvSclHvspInputInformConfig_t stInformCfg;
    bool bLDCorPrvCrop;
    DrvSclHvspOsdConfig_t stOsdCfg;
    DrvSclOsMemset(&sthvspformCfg,0,sizeof(DrvSclHvspInformConfig_t));
    DrvSclOsMemset(&sthvsp2formCfg,0,sizeof(DrvSclHvspInformConfig_t));
    DrvSclOsMemset(&sthvsp3formCfg,0,sizeof(DrvSclHvspInformConfig_t));
    DrvSclOsMemset(&stzoomformCfg,0,sizeof(DrvSclHvspScInformConfig_t));
    DrvSclOsMemset(&stIpmformCfg,0,sizeof(DrvSclHvspIpmConfig_t));
    DrvSclOsMemset(&stInformCfg,0,sizeof(DrvSclHvspInputInformConfig_t));
    DrvSclOsMemset(&stOsdCfg,0,sizeof(DrvSclHvspOsdConfig_t));
    DrvSclHvspGetCrop12Inform(&stInformCfg);
    DrvSclHvspGetSCLInform(E_DRV_SCLHVSP_ID_1,&stzoomformCfg);
    DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_1,&sthvspformCfg);
    DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_2,&sthvsp2formCfg);
    DrvSclHvspGetHvspAttribute(E_DRV_SCLHVSP_ID_3,&sthvsp3formCfg);
    DrvSclHvspGetOsdAttribute(E_DRV_SCLHVSP_ID_1,&stOsdCfg);
    bLDCorPrvCrop = DrvSclHvspGetFrameBufferAttribute(E_DRV_SCLHVSP_ID_1,&stIpmformCfg);
    str += DrvSclOsScnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
    str += DrvSclOsScnprintf(str, end - str, "Inst:%hhu\n",gu8LevelInst); // global set
    str += DrvSclOsScnprintf(str, end - str, "Read Inst:%hhu\n",gu8LevelInst); //now
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL INPUT MUX----------------------\n");
    if(stInformCfg.enMux==0)
    {
        str += DrvSclOsScnprintf(str, end - str, "Input SRC :BT656\n");
    }
    else if(stInformCfg.enMux==1)
    {
        str += DrvSclOsScnprintf(str, end - str, "Input SRC :ISP\n");
    }
    else if(stInformCfg.enMux==3)
    {
        str += DrvSclOsScnprintf(str, end - str, "Input SRC :PTGEN\n");
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "Input SRC :OTHER\n");
    }
    str += DrvSclOsScnprintf(str, end - str, "Input H   :%hd\n",stInformCfg.u16inWidth);
    str += DrvSclOsScnprintf(str, end - str, "Input V   :%hd\n",stInformCfg.u16inHeight);
    str += DrvSclOsScnprintf(str, end - str, "Receive H :%hd\n",stInformCfg.u16inWidthcount);
    str += DrvSclOsScnprintf(str, end - str, "Receive V :%hd\n",stInformCfg.u16inHeightcount);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL FB-----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "FB H          :%hd\n",stIpmformCfg.u16Fetch);
    str += DrvSclOsScnprintf(str, end - str, "FB V          :%hd\n",stIpmformCfg.u16Vsize);
    str += DrvSclOsScnprintf(str, end - str, "FB Addr       :%lx\n",stIpmformCfg.u32YCBaseAddr);
    str += DrvSclOsScnprintf(str, end - str, "FB memsize    :%ld\n",stIpmformCfg.u32MemSize);
    str += DrvSclOsScnprintf(str, end - str, "FB Buffer     :%hhd\n",DrvSclOsGetSclFrameBufferNum());
    str += DrvSclOsScnprintf(str, end - str, "FB Write      :%hhd\n",stIpmformCfg.bYCMWrite);
    if(bLDCorPrvCrop &0x1)
    {
        str += DrvSclOsScnprintf(str, end - str, "READ PATH     :LDC\n");
    }
    else if(bLDCorPrvCrop &0x2)
    {
        str += DrvSclOsScnprintf(str, end - str, "READ PATH     :PrvCrop\n");
    }
    else if(stIpmformCfg.bYCMRead)
    {
        str += DrvSclOsScnprintf(str, end - str, "READ PATH     :MCNR\n");
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "READ PATH     :NONE\n");
    }
    str += DrvSclOsScnprintf(str, end - str, "FRAME DELAY     :%hhd\n",DrvSclOsGetSclFrameDelay());
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL Crop----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "Crop      :%hhd\n",stInformCfg.bEn);
    str += DrvSclOsScnprintf(str, end - str, "CropX     :%hd\n",stInformCfg.u16inCropX);
    str += DrvSclOsScnprintf(str, end - str, "CropY     :%hd\n",stInformCfg.u16inCropY);
    str += DrvSclOsScnprintf(str, end - str, "CropOutW  :%hd\n",stInformCfg.u16inCropWidth);
    str += DrvSclOsScnprintf(str, end - str, "CropOutH  :%hd\n",stInformCfg.u16inCropHeight);
    str += DrvSclOsScnprintf(str, end - str, "SrcW      :%hd\n",stInformCfg.u16inWidth);
    str += DrvSclOsScnprintf(str, end - str, "SrcH      :%hd\n",stInformCfg.u16inHeight);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL Zoom----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "Zoom      :%hhd\n",stzoomformCfg.bEn);
    str += DrvSclOsScnprintf(str, end - str, "ZoomX     :%hd\n",stzoomformCfg.u16X);
    str += DrvSclOsScnprintf(str, end - str, "ZoomY     :%hd\n",stzoomformCfg.u16Y);
    str += DrvSclOsScnprintf(str, end - str, "ZoomOutW  :%hd\n",stzoomformCfg.u16crop2OutWidth);
    str += DrvSclOsScnprintf(str, end - str, "ZoomOutH  :%hd\n",stzoomformCfg.u16crop2OutHeight);
    str += DrvSclOsScnprintf(str, end - str, "SrcW      :%hd\n",stzoomformCfg.u16crop2inWidth);
    str += DrvSclOsScnprintf(str, end - str, "SrcH      :%hd\n",stzoomformCfg.u16crop2inHeight);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += DrvSclOsScnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "Locate: After\n");
    }
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL HVSP1----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "InputH    :%hd\n",sthvspformCfg.u16inWidth);
    str += DrvSclOsScnprintf(str, end - str, "InputV    :%hd\n",sthvspformCfg.u16inHeight);
    str += DrvSclOsScnprintf(str, end - str, "OutputH   :%hd\n",sthvspformCfg.u16Width);
    str += DrvSclOsScnprintf(str, end - str, "OutputV   :%hd\n",sthvspformCfg.u16Height);
    str += DrvSclOsScnprintf(str, end - str, "H en  :%hhx\n",sthvspformCfg.bEn&0x1);
    str += DrvSclOsScnprintf(str, end - str, "H function  :%hhx\n",(sthvspformCfg.bEn&0xC0)>>6);
    str += DrvSclOsScnprintf(str, end - str, "V en  :%hhx\n",(sthvspformCfg.bEn&0x2)>>1);
    str += DrvSclOsScnprintf(str, end - str, "V function  :%hhx\n",(sthvspformCfg.bEn&0x30)>>4);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL HVSP2----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "InputH    :%hd\n",sthvsp2formCfg.u16inWidth);
    str += DrvSclOsScnprintf(str, end - str, "InputV    :%hd\n",sthvsp2formCfg.u16inHeight);
    str += DrvSclOsScnprintf(str, end - str, "OutputH   :%hd\n",sthvsp2formCfg.u16Width);
    str += DrvSclOsScnprintf(str, end - str, "OutputV   :%hd\n",sthvsp2formCfg.u16Height);
    str += DrvSclOsScnprintf(str, end - str, "H en  :%hhx\n",sthvsp2formCfg.bEn&0x1);
    str += DrvSclOsScnprintf(str, end - str, "H function  :%hhx\n",(sthvsp2formCfg.bEn&0xC0)>>6);
    str += DrvSclOsScnprintf(str, end - str, "V en  :%hhx\n",(sthvsp2formCfg.bEn&0x2)>>1);
    str += DrvSclOsScnprintf(str, end - str, "V function  :%hhx\n",(sthvsp2formCfg.bEn&0x30)>>4);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL HVSP3----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "InputH    :%hd\n",sthvsp3formCfg.u16inWidth);
    str += DrvSclOsScnprintf(str, end - str, "InputV    :%hd\n",sthvsp3formCfg.u16inHeight);
    str += DrvSclOsScnprintf(str, end - str, "OutputH   :%hd\n",sthvsp3formCfg.u16Width);
    str += DrvSclOsScnprintf(str, end - str, "OutputV   :%hd\n",sthvsp3formCfg.u16Height);
    str += DrvSclOsScnprintf(str, end - str, "H en  :%hhx\n",sthvsp3formCfg.bEn&0x1);
    str += DrvSclOsScnprintf(str, end - str, "H function  :%hhx\n",(sthvsp3formCfg.bEn&0xC0)>>6);
    str += DrvSclOsScnprintf(str, end - str, "V en  :%hhx\n",(sthvsp3formCfg.bEn&0x2)>>1);
    str += DrvSclOsScnprintf(str, end - str, "V function  :%hhx\n",(sthvsp3formCfg.bEn&0x30)>>4);
    str += DrvSclOsScnprintf(str, end - str, "\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL PROC FRAMEWORK======================\n");
    end = end;
    return (str - buf);
}
#endif
//#if defined (SCLOS_TYPE_LINUX_KERNEL)
void MDrvSclHvspSetProcInst(u8 u8level)
{
    gu8LevelInst = u8level;
}
ssize_t MDrvSclHvspProcShow(char *buf)
{
    u32 i;
    if(gu8LevelInst==0xFF)
    {
        // check all
        for(i=0;i<MDRV_SCL_CTX_INSTANT_MAX;i++)
        {
            MDrvSclHvspProcShowInst(buf,(u8)i);
        }
    }
    else if(gu8LevelInst<MDRV_SCL_CTX_INSTANT_MAX)
    {
        MDrvSclHvspProcShowInst(buf,gu8LevelInst);
    }
    return 0;
}
void MDrvSclHvspRegShowInst(u8 u8level,u32 u32RegAddr,bool bAllbank)
{
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    s32 s32Handler;
    u32 i;
    pCtxCfg = MDrvSclCtxGetCtx(u8level);
    if(pCtxCfg->bUsed)
    {
        for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
        {
            s32Handler = pCtxCfg->s32Id[i];
            if(u8level<MDRV_SCL_CTX_INSTANT_MAX && ((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX))
            {
                break;
            }
            else if(u8level>=MDRV_SCL_CTX_INSTANT_MAX&& ((s32Handler&HANDLER_PRE_MASK)==SCLVIP_HANDLER_PRE_FIX))
            {
                break;
            }
        }
        MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        SCL_ERR("[Dump]BANK:%lx Inst:%hhx\n",((u32RegAddr&0xFFFF00)>>8),u8level);
        MDrvSclCtxDumpRegSetting(u32RegAddr,bAllbank);
        MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
    }
}
ssize_t MDrvSclHvspRegShow(char *buf,u32 u32RegAddr,u16 *u16RegVal,bool bAllbank)
{
    u32 i;
    if(gu8LevelInst==0xFF)
    {
        // check all
        for(i=0;i<E_MDRV_SCL_CTX_ID_NUM *MDRV_SCL_CTX_INSTANT_MAX;i++)
        {
            MDrvSclHvspRegShowInst(i,u32RegAddr,bAllbank);
        }
    }
    else if(gu8LevelInst<E_MDRV_SCL_CTX_ID_NUM *MDRV_SCL_CTX_INSTANT_MAX)
    {
        MDrvSclHvspRegShowInst(gu8LevelInst,u32RegAddr,bAllbank);
    }
    return 0;
}
void MDrvSclHvspSetClkForcemode(bool bEn)
{
    DrvSclHvspSetClkForcemode(bEn);
}
void MDrvSclHvspSetClkRate(void* adjclk,u8 u8Idx)
{
    DrvSclOsClkStruct_t* pstclock = NULL;
    DrvSclHvspSetClkRate(u8Idx);
    if (DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)adjclk)==0)
    {
    }
    else
    {
        if (NULL != (pstclock = DrvSclOsClkGetParentByIndex((DrvSclOsClkStruct_t*)adjclk, (u8Idx &0x0F))))
        {
            DrvSclOsClkSetParent((DrvSclOsClkStruct_t*)adjclk, pstclock);
        }
    }
}
void MDrvSclHvspSetClkOnOff(void* adjclk,bool bEn)
{
    DrvSclOsClkStruct_t* pstclock = NULL;
    if (DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)adjclk)==0 &&bEn)
    {
        if (NULL != (pstclock = DrvSclOsClkGetParentByIndex((DrvSclOsClkStruct_t*)adjclk, 0)))
        {
            DrvSclOsClkSetParent((DrvSclOsClkStruct_t*)adjclk, pstclock);
            DrvSclOsClkPrepareEnable((DrvSclOsClkStruct_t*)adjclk);
        }
    }
    else if(DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)adjclk)!=0 && !bEn)
    {
        if (NULL != (pstclock = DrvSclOsClkGetParentByIndex((DrvSclOsClkStruct_t*)adjclk, 0)))
        {
            DrvSclOsClkSetParent((DrvSclOsClkStruct_t*)adjclk, pstclock);
            DrvSclOsClkDisableUnprepare((DrvSclOsClkStruct_t*)adjclk);
        }
    }
}
ssize_t MDrvSclHvspmonitorHWShow(char *buf,int VsyncCount ,int MonitorErrCount)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL monitor HW BUTTON======================\n");
    str += DrvSclOsScnprintf(str, end - str, "CROP Monitor    :1\n");
    str += DrvSclOsScnprintf(str, end - str, "DMA1FRM Monitor :2\n");
    str += DrvSclOsScnprintf(str, end - str, "DMA1SNP Monitor :3\n");
    str += DrvSclOsScnprintf(str, end - str, "DMA2FRM Monitor :4\n");
    str += DrvSclOsScnprintf(str, end - str, "DMA3FRM Monitor :5\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL monitor HW ======================\n");
    str += DrvSclOsScnprintf(str, end - str, "vysnc count:%d",VsyncCount);
    str += DrvSclOsScnprintf(str, end - str, "Monitor Err count:%d\n",MonitorErrCount);
    end = end;
    return (str - buf);
}
void MDrvSclHvspDbgmgDumpShow(u8 u8level)
{
    u32 i,j;
    MDrvSclCtxConfig_t *pCtxCfg = NULL;
    s32 s32Handler;
    if(u8level==0xFF)
    {
        // check all
        for(j=0;j<E_MDRV_SCL_CTX_ID_NUM *MDRV_SCL_CTX_INSTANT_MAX;j++)
        {
            pCtxCfg = MDrvSclCtxGetCtx(j);
            if(pCtxCfg->bUsed)
            {
                for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
                {
                    s32Handler = pCtxCfg->s32Id[i];
                    if(j<MDRV_SCL_CTX_INSTANT_MAX && ((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX))
                    {
                        break;
                    }
                    else if(j>=MDRV_SCL_CTX_INSTANT_MAX&& ((s32Handler&HANDLER_PRE_MASK)==SCLVIP_HANDLER_PRE_FIX))
                    {
                        break;
                    }
                }
                MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
                MDrvSclCtxDumpSetting();
                MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
            }
        }
    }
    else if(u8level<E_MDRV_SCL_CTX_ID_NUM*MDRV_SCL_CTX_INSTANT_MAX)
    {
        pCtxCfg = MDrvSclCtxGetCtx(u8level);
        if(pCtxCfg->bUsed)
        {
            for(i=0; i<MDRV_SCL_CTX_CLIENT_ID_MAX; i++)
            {
                s32Handler = pCtxCfg->s32Id[i];
                if(u8level<MDRV_SCL_CTX_INSTANT_MAX && ((s32Handler&HANDLER_PRE_MASK)==SCLHVSP_HANDLER_PRE_FIX))
                {
                    break;
                }
                else if(u8level>=MDRV_SCL_CTX_INSTANT_MAX&& ((s32Handler&HANDLER_PRE_MASK)==SCLVIP_HANDLER_PRE_FIX))
                {
                    break;
                }
            }
            MDrvSclCtxSetLockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
            MDrvSclCtxDumpSetting();
            MDrvSclCtxSetUnlockConfig(s32Handler,pCtxCfg->stCtx.enCtxId);
        }
    }
}
ssize_t MDrvSclHvspDbgmgFlagShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += DrvSclOsScnprintf(str, end - str, "CONFIG            ECHO        STATUS\n");
    str += DrvSclOsScnprintf(str, end - str, "MDRV_CONFIG       (1)         0x%x\n",gbdbgmessage[EN_DBGMG_MDRV_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "IOCTL_CONFIG      (2)         0x%x\n",gbdbgmessage[EN_DBGMG_IOCTL_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "HVSP_CONFIG       (3)         0x%x\n",gbdbgmessage[EN_DBGMG_HVSP_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "SCLDMA_CONFIG     (4)         0x%x\n",gbdbgmessage[EN_DBGMG_SCLDMA_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "PNL_CONFIG        (5)         0x%x\n",gbdbgmessage[EN_DBGMG_PNL_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "VIP_CONFIG        (6)         0x%x\n",gbdbgmessage[EN_DBGMG_VIP_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "DRVPQ_CONFIG      (7)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVPQ_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "CTX_CONFIG        (8)         0x%x\n",gbdbgmessage[EN_DBGMG_CTX_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "VPESCL_CONFIG     (9)         0x%x\n",gbdbgmessage[EN_DBGMG_VPESCL_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "VPEIQ_CONFIG      (A)         0x%x\n",gbdbgmessage[EN_DBGMG_VPEIQ_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "DRVHVSP_CONFIG    (B)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVHVSP_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "DRVSCLDMA_CONFIG  (C)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLDMA_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "DRVSCLIRQ_CONFIG  (D)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVSCLIRQ_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "VPEISP_CONFIG     (E)         0x%x\n",gbdbgmessage[EN_DBGMG_CMDQ_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "DRVVIP_CONFIG     (F)         0x%x\n",gbdbgmessage[EN_DBGMG_DRVVIP_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "PRIORITY_CONFIG   (G)         0x%x\n",gbdbgmessage[EN_DBGMG_PRIORITY_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "UTILITY_CONFIG    (H)         0x%x\n",gbdbgmessage[EN_DBGMG_UTILITY_CONFIG]);
    str += DrvSclOsScnprintf(str, end - str, "UTILITY_DUMP      (I)         byLevel\n");
    str += DrvSclOsScnprintf(str, end - str, "ALL Reset         (0)\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL Debug Message BUTTON======================\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
    str += DrvSclOsScnprintf(str, end - str, "default is level 1\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------IOCTL LEVEL---------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : SC1\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : SC2\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : SC3\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : VIP\n");
    str += DrvSclOsScnprintf(str, end - str, "0x10 : SC1HLEVEL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x20 : SC2HLEVEL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x40 : LCD\n");
    str += DrvSclOsScnprintf(str, end - str, "0x80 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------HVSP LEVEL---------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : HVSP1\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : HVSP2\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : HVSP3\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------SCLDMA LEVEL-------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : SC1 FRM\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : SC1 SNP \n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : SC2 FRM\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : SC3 FRM\n");
    str += DrvSclOsScnprintf(str, end - str, "0x10 : SC1 FRM HL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x20 : SC1 SNP HL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x40 : SC2 FRM HL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x80 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------VIP LEVEL(IOlevel)-------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : VIP LOG \n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : VIP SUSPEND\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "------------------------------CTX LEVEL---------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : Low freq\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : \n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : High freq\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : \n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------PQ LEVEL---------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : befor crop\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : color eng\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : VIP Y\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : VIP C\n");
    str += DrvSclOsScnprintf(str, end - str, "0x10 : AIP\n");
    str += DrvSclOsScnprintf(str, end - str, "0x20 : AIP post\n");
    str += DrvSclOsScnprintf(str, end - str, "0x40 : HVSP\n");
    str += DrvSclOsScnprintf(str, end - str, "0x80 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------SCLIRQ LEVEL(drvlevel)---------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : NORMAL\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : SC1RINGA \n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : SC1RINGN\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : SC1SINGLE\n");
    str += DrvSclOsScnprintf(str, end - str, "0x10 : SC2RINGA\n");
    str += DrvSclOsScnprintf(str, end - str, "0x20 : SC2RINGN \n");
    str += DrvSclOsScnprintf(str, end - str, "0x40 : SC3SINGLE\n");
    str += DrvSclOsScnprintf(str, end - str, "0x80 : ELSE\n");
    str += DrvSclOsScnprintf(str, end - str, "-------------------------------VPE LEVEL-------------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 1 : Level 1 low sc:freq size Iq:cal\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 2 : Level 2 low freq config\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 4 : Level 3 high sc:freq size Iq:cal\n");
    str += DrvSclOsScnprintf(str, end - str, "0x 8 : Level 4 high freq config\n");
    str += DrvSclOsScnprintf(str, end - str, "0x10 : Level 5 port 0 \n");
    str += DrvSclOsScnprintf(str, end - str, "0x20 : Level 6 port 1 \n");
    str += DrvSclOsScnprintf(str, end - str, "0x40 : Level 7 port 2 \n");
    str += DrvSclOsScnprintf(str, end - str, "0x80 : Level 8 port 3 \n");
    str += DrvSclOsScnprintf(str, end - str, "\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL Debug Message LEVEL======================\n");
    end = end;
    return (str - buf);
}
ssize_t MDrvSclHvspIntsShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    DrvSclHvspScIntsType_t stInts;
    u32 u32val;
    MDrvSclCtxCmdqConfig_t* pvCtx;
    DrvSclOsMemset(&stInts,0,sizeof(DrvSclHvspScIntsType_t));
    pvCtx = MDrvSclCtxGetConfigCtx(E_MDRV_SCL_CTX_ID_SC_ALL);
    DrvSclHvspGetSclInts(pvCtx,&stInts);
    str += DrvSclOsScnprintf(str, end - str, "========================SCL INTS======================\n");
    str += DrvSclOsScnprintf(str, end - str, "AFF          Count: %lu\n",stInts.u32AffCount);
    str += DrvSclOsScnprintf(str, end - str, "DMAERROR     Count: %lu\n",stInts.u32ErrorCount);
    str += DrvSclOsScnprintf(str, end - str, "ISPIn        Count: %lu\n",stInts.u32ISPInCount);
    str += DrvSclOsScnprintf(str, end - str, "ISPDone      Count: %lu\n",stInts.u32ISPDoneCount);
    str += DrvSclOsScnprintf(str, end - str, "DIFF         Count: %lu\n",(stInts.u32ISPInCount-stInts.u32ISPDoneCount));
    str += DrvSclOsScnprintf(str, end - str, "ResetCount   Count: %hhu\n",stInts.u8CountReset);
    str += DrvSclOsScnprintf(str, end - str, "SC1FrmDone   Count: %lu\n",stInts.u32SC1FrmDoneCount);
    str += DrvSclOsScnprintf(str, end - str, "SC1SnpDone   Count: %lu\n",stInts.u32SC1SnpDoneCount);
    str += DrvSclOsScnprintf(str, end - str, "SC2FrmDone   Count: %lu\n",stInts.u32SC2FrmDoneCount);
    str += DrvSclOsScnprintf(str, end - str, "SC2Frm2Done  Count: %lu\n",stInts.u32SC2Frm2DoneCount);
    str += DrvSclOsScnprintf(str, end - str, "SC3Done      Count: %lu\n",stInts.u32SC3DoneCount);
    str += DrvSclOsScnprintf(str, end - str, "SCLMainDone  Count: %lu\n",stInts.u32SCLMainDoneCount);
    if(stInts.u32SC1FrmDoneCount)
    {
        u32val = (u32)(stInts.u32SC1FrmActiveTime/stInts.u32SC1FrmDoneCount);
    }
    else
    {
        u32val = stInts.u32SC1FrmActiveTime;
    }
    str += DrvSclOsScnprintf(str, end - str, "SC1Frm       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC1SnpDoneCount)
    {
        u32val = (u32)(stInts.u32SC1SnpActiveTime/stInts.u32SC1SnpDoneCount);
    }
    else
    {
        u32val = stInts.u32SC1SnpActiveTime;
    }
    str += DrvSclOsScnprintf(str, end - str, "SC1Snp       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC2FrmDoneCount)
    {
        u32val = (u32)(stInts.u32SC2FrmActiveTime/stInts.u32SC2FrmDoneCount);
    }
    else
    {
        u32val = stInts.u32SC2FrmDoneCount;
    }
    str += DrvSclOsScnprintf(str, end - str, "SC2Frm       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC2Frm2DoneCount)
    {
        u32val = (u32)(stInts.u32SC2Frm2ActiveTime/stInts.u32SC2Frm2DoneCount);
    }
    else
    {
        u32val = stInts.u32SC2Frm2DoneCount;
    }
    str += DrvSclOsScnprintf(str, end - str, "SC2Frm2      ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SC3DoneCount)
    {
        u32val = (u32)(stInts.u32SC3ActiveTime/stInts.u32SC3DoneCount);
    }
    else
    {
        u32val = stInts.u32SC3DoneCount;
    }
    str += DrvSclOsScnprintf(str, end - str, "SC3          ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32SCLMainDoneCount)
    {
        u32val = (u32)(stInts.u32SCLMainActiveTime/stInts.u32SCLMainDoneCount);
    }
    else
    {
        u32val = stInts.u32SCLMainActiveTime;
    }
    str += DrvSclOsScnprintf(str, end - str, "SCLMain       ActiveTime: %lu (us)\n",u32val);
    if(stInts.u32ISPDoneCount)
    {
        u32val = (u32)(stInts.u32ISPTime/stInts.u32ISPDoneCount);
    }
    else
    {
        u32val = stInts.u32ISPTime;
    }
    str += DrvSclOsScnprintf(str, end - str, "ISP       ActiveTime: %lu (us)\n",u32val);
    str += DrvSclOsScnprintf(str, end - str, "ISP       BlankingTime: %lu (us)\n",stInts.u32ISPBlankingTime);
    str += DrvSclOsScnprintf(str, end - str, "========================SCL INTS======================\n");
    end = end;
    return (str - buf);
}
void MDrvSclHvspScIqStore(const char *buf,MDrvSclHvspIdType_e enHVSP_ID)
{
    const char *str = buf;
    DrvSclHvspIdType_e enID;
    MDrvSclCtxCmdqConfig_t* pvCtx;
    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;
    pvCtx = MDrvSclCtxGetConfigCtx(E_MDRV_SCL_CTX_ID_SC_ALL);
    if(NULL!=buf)
    {
        //if(!)
        if((int)*str == 49)    //input 1
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_Tbl0,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 50)  //input 2
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_Tbl1,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 51)  //input 3
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_Tbl2,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 52)  //input 4
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_Tbl3,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 53)  //input 5
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_BYPASS,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 54)  //input 6
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_H_BILINEAR,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 55)  //input 7
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_Tbl0,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 56)  //input 8
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_Tbl1,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 57)  //input 9
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_Tbl2,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 65)  //input A
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_Tbl3,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 66)  //input B
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_BYPASS,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
        else if((int)*str == 67)  //input C
        {
            DrvSclHvspSclIq(enID,E_DRV_SCLHVSP_IQ_V_BILINEAR,pvCtx);
            SCL_ERR( "[SCLIQ]Set %d\n",(int)*str);
        }
    }
}
ssize_t MDrvSclHvspScIqShow(char *buf, MDrvSclHvspIdType_e enHVSP_ID)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    DrvSclHvspInformConfig_t sthvspformCfg;
    DrvSclOsMemset(&sthvspformCfg,0,sizeof(DrvSclHvspInformConfig_t));
    DrvSclHvspGetHvspAttribute(enHVSP_ID,&sthvspformCfg);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL IQ----------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "H en  :%hhx\n",sthvspformCfg.bEn&0x1);
    str += DrvSclOsScnprintf(str, end - str, "H function  :%hhx\n",(sthvspformCfg.bEn&0xC0)>>6);
    str += DrvSclOsScnprintf(str, end - str, "V en  :%hhx\n",(sthvspformCfg.bEn&0x2)>>1);
    str += DrvSclOsScnprintf(str, end - str, "V function  :%hhx\n",(sthvspformCfg.bEn&0x30)>>4);
    str += DrvSclOsScnprintf(str, end - str, "SC H   :1~6\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 0   :1\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 1   :2\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 2   :3\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 3   :4\n");
    str += DrvSclOsScnprintf(str, end - str, "SC bypass    :5\n");
    str += DrvSclOsScnprintf(str, end - str, "SC bilinear  :6\n");
    str += DrvSclOsScnprintf(str, end - str, "SC V :7~C\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 0   :7\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 1   :8\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 2   :9\n");
    str += DrvSclOsScnprintf(str, end - str, "SC table 3   :A\n");
    str += DrvSclOsScnprintf(str, end - str, "SC bypass    :B\n");
    str += DrvSclOsScnprintf(str, end - str, "SC bilinear  :C\n");
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL IQ----------------------\n");
    end = end;
    return (str - buf);
}
ssize_t MDrvSclHvspClkFrameworkShow(char *buf,MDrvSclHvspClkConfig_t* stclk)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL CLK FRAMEWORK======================\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 1 > clk :open force mode\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 0 > clk :close force mode\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 2 > clk :fclk1 172\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 3 > clk :fclk1 86\n");
    str += DrvSclOsScnprintf(str, end - str, "echo E > clk :fclk1 216\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 4 > clk :fclk1 open\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 5 > clk :fclk1 close\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 6 > clk :fclk2 172\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 7 > clk :fclk2 86\n");
    str += DrvSclOsScnprintf(str, end - str, "echo F > clk :fclk2 216\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 8 > clk :fclk2 open\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 9 > clk :fclk2 close\n");
    str += DrvSclOsScnprintf(str, end - str, "echo : > clk :idclk ISP\n");
    str += DrvSclOsScnprintf(str, end - str, "echo D > clk :idclk BT656\n");
    str += DrvSclOsScnprintf(str, end - str, "echo B > clk :idclk open\n");
    str += DrvSclOsScnprintf(str, end - str, "echo = > clk :idclk close\n");
    str += DrvSclOsScnprintf(str, end - str, "echo C > clk :odclk MAX\n");
    str += DrvSclOsScnprintf(str, end - str, "echo ? > clk :odclk LPLL\n");
    str += DrvSclOsScnprintf(str, end - str, "echo @ > clk :odclk open\n");
    str += DrvSclOsScnprintf(str, end - str, "echo A > clk :odclk close\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL CLK STATUS======================\n");
    str += DrvSclOsScnprintf(str, end - str, "force mode :%hhd\n",DrvSclHvspGetClkForcemode());
    if(DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)stclk->fclk1))
    {
        str += DrvSclOsScnprintf(str, end - str, "fclk1 open :%ld ,%ld\n",
            DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)stclk->fclk1),DrvSclOsClkGetRate((DrvSclOsClkStruct_t*)stclk->fclk1));
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "fclk1 close\n");
    }
    if(DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)stclk->fclk2))
    {
        str += DrvSclOsScnprintf(str, end - str, "fclk2 open :%ld,%ld\n",
            DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)stclk->fclk2),DrvSclOsClkGetRate((DrvSclOsClkStruct_t*)stclk->fclk2));
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "fclk2 close\n");
    }
    if(DrvSclOsClkGetEnableCount((DrvSclOsClkStruct_t*)stclk->idclk))
    {
        if(DrvSclOsClkGetRate((DrvSclOsClkStruct_t*)stclk->idclk) > 10)
        {
            str += DrvSclOsScnprintf(str, end - str, "idclk open :ISP\n");
        }
        else if(DrvSclOsClkGetRate((DrvSclOsClkStruct_t*)stclk->idclk) == 1)
        {
            str += DrvSclOsScnprintf(str, end - str, "idclk open :BT656\n");
        }
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "idclk close\n");
    }
    end = end;
    return (str - buf);
}
ssize_t MDrvSclHvspFBMGShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "LDCPATH_ON    :1\n");
    str += DrvSclOsScnprintf(str, end - str, "LDCPATH_OFF   :2\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRRead_ON    :3\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRRead_OFF   :4\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRWrite_ON   :5\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRWrite_OFF  :6\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRBuf1       :7\n");
    str += DrvSclOsScnprintf(str, end - str, "DNRBuf2       :8\n");
    str += DrvSclOsScnprintf(str, end - str, "UNLOCK        :9\n");
    str += DrvSclOsScnprintf(str, end - str, "PrvCrop_ON    :A\n");
    str += DrvSclOsScnprintf(str, end - str, "PrvCrop_OFF   :B\n");
    str += DrvSclOsScnprintf(str, end - str, "CIIR_ON       :C\n");
    str += DrvSclOsScnprintf(str, end - str, "CIIR_OFF      :D\n");
    str += DrvSclOsScnprintf(str, end - str, "LOCK          :E\n");
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL FBMG----------------------------\n");
    end = end;
    return (str - buf);
}
ssize_t MDrvSclHvspOsdShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    DrvSclHvspOsdConfig_t stOsdCfg;
    DrvSclOsMemset(&stOsdCfg,0,sizeof(DrvSclHvspOsdConfig_t));
    DrvSclHvspGetOsdAttribute(E_DRV_SCLHVSP_ID_1,&stOsdCfg);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "ONOFF     :%hhd\n",stOsdCfg.stOsdOnOff.bOSDEn);
    if(stOsdCfg.enOSD_loc)
    {
        str += DrvSclOsScnprintf(str, end - str, "Locate: Before\n");
    }
    else
    {
        str += DrvSclOsScnprintf(str, end - str, "Locate: After\n");
    }
    str += DrvSclOsScnprintf(str, end - str, "Bypass    :%hhd\n",stOsdCfg.stOsdOnOff.bOSDBypass);
    str += DrvSclOsScnprintf(str, end - str, "WTM Bypass:%hhd\n",stOsdCfg.stOsdOnOff.bWTMBypass);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 1 > OSD :open OSD\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 0 > OSD :close OSD\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 2 > OSD :Set OSD before\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 3 > OSD :Set OSD After\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 4 > OSD :Set OSD Bypass\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 5 > OSD :Set OSD Bypass Off\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 6 > OSD :Set OSD WTM Bypass\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 7 > OSD :Set OSD WTM Bypass Off\n");
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL OSD----------------------------\n");
    end = end;
    return (str - buf);
}
void MDrvSclHvspOsdStore(const char *buf,MDrvSclHvspIdType_e enHVSP_ID)
{
    const char *str = buf;
    DrvSclHvspOsdConfig_t stOSdCfg;
    DrvSclHvspIdType_e enID;
    DrvSclOsMemset(&stOSdCfg,0,sizeof(DrvSclHvspOsdConfig_t));
    enID = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_ID_2 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_ID_3 :
           enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_ID_4 :
                                           E_DRV_SCLHVSP_ID_1;

    DrvSclHvspGetOsdAttribute(enID,&stOSdCfg);
    if((int)*str == 49)    //input 1
    {
        SCL_ERR( "[OSD]open OSD %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDEn = 1;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 48)  //input 0
    {
        SCL_ERR( "[OSD]close OSD %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDEn = 0;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 50)  //input 2
    {
        SCL_ERR( "[OSD]Set OSD before %d\n",(int)*str);
        stOSdCfg.enOSD_loc = E_DRV_SCLHVSP_OSD_LOC_BEFORE;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 51)  //input 3
    {
        SCL_ERR( "[OSD]Set OSD After %d\n",(int)*str);
        stOSdCfg.enOSD_loc = E_DRV_SCLHVSP_OSD_LOC_AFTER;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 52)  //input 4
    {
        SCL_ERR( "[OSD]Set OSD Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 1;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 53)  //input 5
    {
        SCL_ERR( "[OSD]Set OSD Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bOSDBypass = 0;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 54)  //input 6
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 1;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
    else if((int)*str == 55)  //input 7
    {
        SCL_ERR( "[OSD]Set OSD WTM Bypass Off %d\n",(int)*str);
        stOSdCfg.stOsdOnOff.bWTMBypass = 0;
        DrvSclHvspSetOsdConfig(enID, &stOSdCfg);
    }
}
ssize_t MDrvSclHvspLockShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL Lock----------------------------\n");
    str = DrvSclOsCheckMutex(str,end);
    str += DrvSclOsScnprintf(str, end - str, "------------------------SCL Lock----------------------------\n");
    end = end;
    return (str - buf);
}
bool MDrvSclHvspVtrackEnable( u8 u8FrameRate, MDrvSclHvspVtrackEnableType_e bEnable)
{
    DrvSclHvspVtrackEnable(u8FrameRate, bEnable);
    return 1;
}
bool MDrvSclHvspVtrackSetPayloadData(u16 u16Timecode, u8 u8OperatorID)
{
    DrvSclHvspVtrackSetPayloadData(u16Timecode,u8OperatorID);
    return 1;
}
bool MDrvSclHvspVtrackSetKey(bool bUserDefinded, u8 *pu8Setting)
{
    DrvSclHvspVtrackSetKey(bUserDefinded,pu8Setting);
    return 1;
}

bool MDrvSclHvspVtrackSetUserDefindedSetting(bool bUserDefinded, u8 *pu8Setting)
{
    DrvSclHvspVtrackSetUserDefindedSetting(bUserDefinded,pu8Setting);
    return 1;
}
bool MDrvSclHvspVtrackInit(void)
{
    DrvSclHvspVtrackSetUserDefindedSetting(0,NULL);
    DrvSclHvspVtrackSetPayloadData(0,0);
    DrvSclHvspVtrackSetKey(0,NULL);
    return 1;
}
#if (I2_DVR==0)
bool MDrvSclHvspGetCmdqDoneStatus(MDrvSclHvspIdType_e enHVSP_ID)
{
    DrvSclHvspPollIdType_e enPollId;
    enPollId = enHVSP_ID == E_MDRV_SCLHVSP_ID_2 ? E_DRV_SCLHVSP_POLL_ID_2 :
               enHVSP_ID == E_MDRV_SCLHVSP_ID_3 ? E_DRV_SCLHVSP_POLL_ID_3 :
               enHVSP_ID == E_MDRV_SCLHVSP_ID_4 ? E_DRV_SCLHVSP_POLL_ID_4 :
                                           E_DRV_SCLHVSP_POLL_ID_1;
    return DrvSclHvspGetCmdqDoneStatus(enPollId);
}
#endif
//#endif
