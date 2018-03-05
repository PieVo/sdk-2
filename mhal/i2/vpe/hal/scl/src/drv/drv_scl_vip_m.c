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
#define _MDRV_VIP_C

#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"

#include "drv_scl_vip.h"
#include "drv_scl_pq_define.h"
#include "drv_scl_pq_declare.h"
#include "drv_scl_pq.h"

#include "drv_scl_vip_m_st.h"
#include "drv_scl_vip_m.h"
#include "drv_scl_irq_st.h"
#include "drv_scl_irq.h"
#include "hal_scl_reg.h"

#include "drv_scl_ctx_m.h"
#include "drv_scl_ctx_st.h"


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define MDrvSclCmdqMutexLock(enIP,bEn)          //(bEn ? DrvSclCmdqGetModuleMutex(enIP,1) : 0)
#define MDrvSclCmdqMutexUnlock(enIP,bEn)        //(bEn ? DrvSclCmdqGetModuleMutex(enIP,0) : 0)
#define MDrvSclVipMutexLock()                   DrvSclOsObtainMutex(_MVIP_Mutex,SCLOS_WAIT_FOREVER)
#define MDrvSclVipMutexUnlock()                 DrvSclOsReleaseMutex(_MVIP_Mutex)

#define _IsFrameBufferAllocatedReady()          (DrvSclOsGetSclFrameBufferAlloced() &(E_DRV_SCLOS_FBALLOCED_YCM))
#define AIPOffset                               PQ_IP_YEE_Main
#define _GetAipOffset(u32Type)                  (u32Type +AIPOffset)
#define MDRV_SCLVIP_WDR_MLOAD_SIZE 2048 //256pack x 8byte


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MDrvSclCtxMvipGlobalSet_t* gstGlobalMVipSet;
//keep
s32 _MVIP_Mutex = -1;
bool gbMultiSensor = 0;

//-------------------------------------------------------------------------------------------------
//  Local Function
//-------------------------------------------------------------------------------------------------
void _MDrvSclVipSetGlobal(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    gstGlobalMVipSet = &(((MDrvSclCtxGlobalSet_t*)(pvCtx->pgstGlobalSet))->stMvipCfg);
}
u16 _MDrvSclVipCioceStructSizeFromPqType(u8 u8PQIPIdx)
{
    u16 u16DataSize;
    switch(u8PQIPIdx)
    {
        case PQ_IP_NLM_Main:
            u16DataSize = PQ_IP_NLM_Size;
            break;
        case PQ_IP_422to444_Main:
            u16DataSize = PQ_IP_422to444_Size;
            break;
        case PQ_IP_VIP_Main:
            u16DataSize = PQ_IP_VIP_Size;
            break;
        case PQ_IP_VIP_LineBuffer_Main:
            u16DataSize = PQ_IP_VIP_LineBuffer_Size;
            break;
        case PQ_IP_VIP_HLPF_Main:
            u16DataSize = PQ_IP_VIP_HLPF_Size;
            break;
        case PQ_IP_VIP_HLPF_dither_Main:
            u16DataSize = PQ_IP_VIP_HLPF_dither_Size;
            break;
        case PQ_IP_VIP_VLPF_coef1_Main:
        case PQ_IP_VIP_VLPF_coef2_Main:
            u16DataSize = PQ_IP_VIP_VLPF_coef1_Size;
            break;
        case PQ_IP_VIP_VLPF_dither_Main:
            u16DataSize = PQ_IP_VIP_VLPF_dither_Size;
            break;
        case PQ_IP_VIP_Peaking_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Size;
            break;
        case PQ_IP_VIP_Peaking_band_Main:
            u16DataSize = PQ_IP_VIP_Peaking_band_Size;
            break;
        case PQ_IP_VIP_Peaking_adptive_Main:
            u16DataSize = PQ_IP_VIP_Peaking_adptive_Size;
            break;
        case PQ_IP_VIP_Peaking_Pcoring_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Pcoring_Size;
            break;
        case PQ_IP_VIP_Peaking_Pcoring_ad_Y_Main:
            u16DataSize = PQ_IP_VIP_Peaking_Pcoring_ad_Y_Size;
            break;
        case PQ_IP_VIP_Peaking_gain_Main:
            u16DataSize = PQ_IP_VIP_Peaking_gain_Size;
            break;
        case PQ_IP_VIP_Peaking_gain_ad_Y_Main:
            u16DataSize = PQ_IP_VIP_Peaking_gain_ad_Y_Size;
            break;
        case PQ_IP_VIP_YC_gain_offset_Main:
            u16DataSize = PQ_IP_VIP_YC_gain_offset_Size;
            break;
        case PQ_IP_VIP_LCE_Main:
            u16DataSize = PQ_IP_VIP_LCE_Size;
            break;
        case PQ_IP_VIP_LCE_dither_Main:
            u16DataSize = PQ_IP_VIP_LCE_dither_Size;
            break;
        case PQ_IP_VIP_LCE_setting_Main:
            u16DataSize = PQ_IP_VIP_LCE_setting_Size;
            break;
        case PQ_IP_VIP_LCE_curve_Main:
            u16DataSize = PQ_IP_VIP_LCE_curve_Size;
            break;
        case PQ_IP_VIP_DLC_Main:
            u16DataSize = PQ_IP_VIP_DLC_Size;
            break;
        case PQ_IP_VIP_DLC_dither_Main:
            u16DataSize = PQ_IP_VIP_DLC_dither_Size;
            break;
        case PQ_IP_VIP_DLC_His_range_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_range_Size;
            break;
        case PQ_IP_VIP_DLC_His_rangeH_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_rangeH_Size;
            break;
        case PQ_IP_VIP_DLC_His_rangeV_Main:
            u16DataSize = PQ_IP_VIP_DLC_His_rangeV_Size;
            break;
        case PQ_IP_VIP_DLC_PC_Main:
            u16DataSize = PQ_IP_VIP_DLC_PC_Size;
            break;
        case PQ_IP_VIP_UVC_Main:
            u16DataSize = PQ_IP_VIP_UVC_Size;
            break;
        case PQ_IP_VIP_FCC_full_range_Main:
            u16DataSize = PQ_IP_VIP_FCC_full_range_Size;
            break;
        case PQ_IP_VIP_FCC_T1_Main:
        case PQ_IP_VIP_FCC_T2_Main:
        case PQ_IP_VIP_FCC_T3_Main:
        case PQ_IP_VIP_FCC_T4_Main:
        case PQ_IP_VIP_FCC_T5_Main:
        case PQ_IP_VIP_FCC_T6_Main:
        case PQ_IP_VIP_FCC_T7_Main:
        case PQ_IP_VIP_FCC_T8_Main:
            u16DataSize = PQ_IP_VIP_FCC_T1_Size;
            break;
        case PQ_IP_VIP_FCC_T9_Main:
            u16DataSize = PQ_IP_VIP_FCC_T9_Size;
            break;
        case PQ_IP_VIP_IHC_Main:
            u16DataSize = PQ_IP_VIP_IHC_Size;
            break;
        case PQ_IP_VIP_IHC_Ymode_Main:
            u16DataSize = PQ_IP_VIP_IHC_Ymode_Size;
            break;
        case PQ_IP_VIP_IHC_dither_Main:
            u16DataSize = PQ_IP_VIP_IHC_dither_Size;
            break;
        case PQ_IP_VIP_IHC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_IHC_SETTING_Size;
            break;
        case PQ_IP_VIP_ICC_Main:
            u16DataSize = PQ_IP_VIP_ICC_Size;
            break;
        case PQ_IP_VIP_ICC_Ymode_Main:
            u16DataSize = PQ_IP_VIP_ICC_Ymode_Size;
            break;
        case PQ_IP_VIP_ICC_dither_Main:
            u16DataSize = PQ_IP_VIP_ICC_dither_Size;
            break;
        case PQ_IP_VIP_ICC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_ICC_SETTING_Size;
            break;
        case PQ_IP_VIP_Ymode_Yvalue_ALL_Main:
            u16DataSize = PQ_IP_VIP_Ymode_Yvalue_ALL_Size;
            break;
        case PQ_IP_VIP_Ymode_Yvalue_SETTING_Main:
            u16DataSize = PQ_IP_VIP_Ymode_Yvalue_SETTING_Size;
            break;
        case PQ_IP_VIP_IBC_Main:
            u16DataSize = PQ_IP_VIP_IBC_Size;
            break;
        case PQ_IP_VIP_IBC_dither_Main:
            u16DataSize = PQ_IP_VIP_IBC_dither_Size;
            break;
        case PQ_IP_VIP_IBC_SETTING_Main:
            u16DataSize = PQ_IP_VIP_IBC_SETTING_Size;
            break;
        case PQ_IP_VIP_ACK_Main:
            u16DataSize = PQ_IP_VIP_ACK_Size;
            break;
        case PQ_IP_VIP_YCbCr_Clip_Main:
            u16DataSize = PQ_IP_VIP_YCbCr_Clip_Size;
            break;
        default:
            u16DataSize = 0;

            break;
    }
    return u16DataSize;
}
void _MDrvSclVipFillPqCfgByType(u8 u8PQIPIdx, u8 *pData, u16 u16DataSize,MDrvSclVipSetPqConfig_t *stSetPQCfg)
{
    stSetPQCfg->enPQIPType    = u8PQIPIdx;
    stSetPQCfg->pPointToCfg   = pData;
    stSetPQCfg->u32StructSize = u16DataSize;
}


void _MDrvSclVipSetPqParameter(MDrvSclVipSetPqConfig_t *stSetBypassCfg)
{
    MDrv_Scl_PQ_LoadSettingByData(0,stSetBypassCfg->enPQIPType,stSetBypassCfg->pPointToCfg,stSetBypassCfg->u32StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MVIP]enPQIPType:%ld,u32StructSize:%ld\n"
        ,stSetBypassCfg->enPQIPType,stSetBypassCfg->u32StructSize);
}
void _MDrvSclVipSetPqByType(u8 u8PQIPIdx, u8 *pData)
{
    MDrvSclVipSetPqConfig_t stSetPQCfg;
    u16 u16Structsize;
    DrvSclOsMemset(&stSetPQCfg,0,sizeof(MDrvSclVipSetPqConfig_t));
    if(u8PQIPIdx>=AIPOffset || u8PQIPIdx==PQ_IP_MCNR_Main)
    {
        u16Structsize = MDrv_Scl_PQ_GetIPRegCount(u8PQIPIdx);
    }
    else
    {
        u16Structsize = _MDrvSclVipCioceStructSizeFromPqType(u8PQIPIdx);
    }
    _MDrvSclVipFillPqCfgByType(u8PQIPIdx,pData,u16Structsize,&stSetPQCfg);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MVIP]Struct size:%hd,%hd\n"
        ,u16Structsize,MDrv_Scl_PQ_GetIPRegCount(u8PQIPIdx));
    _MDrvSclVipSetPqParameter(&stSetPQCfg);
}
void _MDrvSclVipSetMcnr(void *pstCfg)
{
    _MDrvSclVipSetPqByType(PQ_IP_MCNR_Main,(u8 *)pstCfg);
}


bool _MDrvSclVipForSetEachPqTypeByIp
    (u8 u8FirstType,u8 u8LastType,u8 ** pPointToData)
{
    u8 u8PQType;
    bool bRet = 0;
    for(u8PQType = u8FirstType;u8PQType<=u8LastType;u8PQType++)
    {
        _MDrvSclVipSetPqByType(u8PQType,pPointToData[u8PQType-u8FirstType]);
    }
    return bRet;
}
u32 _MDrvSclVipFillSettingBuffer(u16 u16PQIPIdx, u32 u32pstViraddr)
{
    u16 u16StructSize;
    u32 u32ViraddrOri;
    u32 u32Viraddr;
    u16StructSize = MDrv_Scl_PQ_GetIPRegCount(u16PQIPIdx);
    u32ViraddrOri = u32pstViraddr;
    u32Viraddr = (u32)DrvSclOsVirMemalloc(u16StructSize);
    if(!u32Viraddr)
    {
        SCL_ERR("[MDRVIP]%s(%d) Init *u32gpViraddr Fail\n", __FUNCTION__, __LINE__);
        return 0;
    }
    DrvSclOsMemset((void *)u32Viraddr,0,u16StructSize);
    if(u32Viraddr != u32ViraddrOri)
    {
        if(DrvSclOsCopyFromUser((void *)u32Viraddr, (void *)u32ViraddrOri, u16StructSize))
        {
            DrvSclOsMemcpy((void *)u32Viraddr, (void *)u32ViraddrOri, u16StructSize);
        }
    }
    return u32Viraddr;
}
void _MDrvSclVipAipSettingDebugMessage
    (MDrvSclVipAipConfig_t *stCfg,void * pvPQSetParameter,u16 u16StructSize)
{
    u16 u16AIPsheet;
    u8 word1,word2;
    u16 u16idx;
    u16AIPsheet = _GetAipOffset(stCfg->u16AIPType);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]Sheet:%hd,size:%hd\n",u16AIPsheet,u16StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]copy addr:%lx,vir addr:%lx\n"
        ,(u32)pvPQSetParameter,(u32)stCfg->u32Viraddr);
    if(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
    {
        for(u16idx =0 ;u16idx<u16StructSize;u16idx++)
        {
            word1 = *((u8 *)pvPQSetParameter+u16idx);
            word2 = *((u8 *)stCfg->u32Viraddr+u16idx);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]idx :%hd, copy value:%hhx,vir value:%hhx\n"
                ,u16idx,(u8)(word1),(u8)(word2));
        }
    }
}
void _MDrvSclVipAipSramSettingDebugMessage
    (MDrvSclVipAipSramConfig_t *stCfg,void * pvPQSetParameter,u16 u16StructSize)
{
    u8 word1,word2;
    u16 u16idx;
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]%s\n",__FUNCTION__);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]enType:%hd,size:%hd\n",stCfg->enAIPType,u16StructSize);
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]copy addr:%lx,vir addr:%lx\n"
        ,(u32)pvPQSetParameter,(u32)stCfg->u32Viraddr);
    if(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
    {
        for(u16idx =0 ;u16idx<u16StructSize;u16idx++)
        {
            word1 = *((u8 *)pvPQSetParameter+u16idx);
            word2 = *((u8 *)stCfg->u32Viraddr+u16idx);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[MDRVVIP]idx :%hd, copy value:%hhx,vir value:%hhx\n"
                ,u16idx,(u8)(word1),(u8)(word2));
        }
    }
}
u16 _MDrvSclVipGetSramBufferSize(MDrvSclVipAipSramType_e enAIPType)
{
    u16 u16StructSize;
    switch(enAIPType)
    {
        case E_MDRV_SCLVIP_AIP_SRAM_GAMMA_Y:
            u16StructSize = PQ_IP_YUV_Gamma_tblY_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GAMMA_U:
            u16StructSize = PQ_IP_YUV_Gamma_tblU_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GAMMA_V:
            u16StructSize = PQ_IP_YUV_Gamma_tblV_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM10to12_R:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM10to12_G:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM10to12_B:
            u16StructSize = PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM12to10_R:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM12to10_G:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_GM12to10_B:
            u16StructSize = PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_SIZE_Main;
            break;
        case E_MDRV_SCLVIP_AIP_SRAM_WDR_TBL:
            u16StructSize = (33*2*4)+33;
            break;
        default:
            u16StructSize = 0;
            break;
    }
    return u16StructSize;
}
void _MDrvSclVipMemNaming(char *pstIqName,s16 s16Idx)
{
    char KEY_DMEM_IQ_HIST[20] = "IQWDR";
    pstIqName[0] = 48+(((s16Idx&0xFFF)%1000)/100);
    pstIqName[1] = 48+(((s16Idx&0xFFF)%100)/10);
    pstIqName[2] = 48+(((s16Idx&0xFFF)%10));
    pstIqName[3] = '_';
    pstIqName[4] = '\0';
    DrvSclOsStrcat(pstIqName,KEY_DMEM_IQ_HIST);
}
void _MDrvSclVipFillIpmStruct(MDrvSclVipIpmConfig_t *pCfg ,DrvSclVipIpmConfig_t *stIPMCfg)
{
    stIPMCfg->bYCMRead       = (bool)((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_YCM_R);
    stIPMCfg->bYCMWrite      = (bool)(((pCfg->enRW)&E_MDRV_SCLHVSP_MCNR_YCM_W)>>1);
    stIPMCfg->u32YCBaseAddr    = pCfg->u32YCMPhyAddr;
    stIPMCfg->enCeType = (DrvSclVipUCMCEType_e)pCfg->enCeType;
    stIPMCfg->u32MemSize = pCfg->u32MemSize;
    stIPMCfg->pvCtx          = pCfg->pvCtx;
}

//-------------------------------------------------------------------------------------------------
//  Function
//-------------------------------------------------------------------------------------------------

void MDrvSclVipResume(MDrvSclVipInitConfig_t *pCfg)
{
    void *pvCtx;
    pvCtx = pCfg->pvCtx;
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    DrvSclVipHWInit();
}

bool MDrvSclVipInit(MDrvSclVipInitConfig_t *pCfg)
{
    DrvSclVipInitConfig_t stIniCfg;
    MS_PQ_Init_Info    stPQInitInfo;
    bool      ret = FALSE;
    char word[] = {"_MVIP_Mutex"};
    DrvSclOsMemset(&stPQInitInfo,0,sizeof(MS_PQ_Init_Info));
    DrvSclOsMemset(&stIniCfg,0,sizeof(DrvSclVipInitConfig_t));
    _MVIP_Mutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, word, SCLOS_PROCESS_SHARED);
    if (_MVIP_Mutex == -1)
    {
        SCL_ERR("[MDRVVIP]%s create mutex fail\n", __FUNCTION__);
        return FALSE;
    }
    stIniCfg.u32RiuBase         = pCfg->u32RiuBase;
    stIniCfg.pvCtx              = pCfg->pvCtx;
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)stIniCfg.pvCtx);
    if(DrvSclVipInit(&stIniCfg) == 0)
    {
        SCL_ERR( "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        return FALSE;
    }
    MDrv_Scl_PQ_init_RIU(pCfg->u32RiuBase);
    DrvSclOsMemset(&stPQInitInfo, 0, sizeof(MS_PQ_Init_Info));
    // Init PQ
    stPQInitInfo.u16PnlWidth    = 1920;
    stPQInitInfo.u8PQBinCnt     = 0;
    stPQInitInfo.u8PQTextBinCnt = 0;
    if(MDrv_Scl_PQ_Init(&stPQInitInfo))
    {
        MDrv_Scl_PQ_DesideSrcType(PQ_MAIN_WINDOW, PQ_INPUT_SOURCE_ISP);
        //ToDo: init val
        MDrv_Scl_PQ_LoadSettings(PQ_MAIN_WINDOW);
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }
    DrvSclVipHWInit();
    return ret;
}
void MDrvSclVipDelete(bool bCloseISR)
{
    if (_MVIP_Mutex != -1)
    {
        DrvSclOsDeleteMutex(_MVIP_Mutex);
        _MVIP_Mutex = -1;
    }
    if(bCloseISR)
    {
        DrvSclIrqExit();
    }
    MDrv_Scl_PQ_Exit();
    DrvSclVipExit();
}
void MDrvSclVipSupend(void)
{
}
void MDrvSclVipReSetHw(void)
{
    DrvSclOsAccessRegType_e bAccessRegMode;
    bAccessRegMode = DrvSclOsGetAccessRegMode();
    DrvSclOsSetAccessRegMode(E_DRV_SCLOS_AccessReg_CPU);
    MDrv_Scl_PQ_LoadSettings(PQ_MAIN_WINDOW);
    DrvSclOsSetAccessRegMode(bAccessRegMode);
    DrvSclVipHwReset();//hist off
}
void MDrvSclVipRelease(void *pvCfg)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
}


bool MDrvSclVipSysInit(MDrvSclVipInitConfig_t *pCfg)
{
    SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s %d\n", __FUNCTION__,__LINE__);
    //DrvSclVipOpen();
    return TRUE;
}
bool MDrvSclVipSetMcnrConfig(void *pCfg)
{
    MDrvSclVipMcnrConfig_t *pstCfg = pCfg;
    void *pvCtx;
    void * pvPQSetParameter;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
#if 1/*Tmp solution*/
    if(1)
#else
    if(_IsFrameBufferAllocatedReady())
#endif
    {
        MDrvSclVipMutexLock();
        pvPQSetParameter = (void *)_MDrvSclVipFillSettingBuffer(PQ_IP_MCNR_Main,pstCfg->u32Viraddr);
        if(!pvPQSetParameter)
        {
            MDrvSclVipMutexUnlock();
            return 0;
        }
        _MDrvSclVipSetMcnr(pvPQSetParameter);
        DrvSclOsVirMemFree((void *)pvPQSetParameter);
        MDrvSclVipMutexUnlock();
        return TRUE;
    }
    else
    {
        SCL_ERR( "[MDRVVIP]%s,MCNR buffer not alloc \n", __FUNCTION__);
        return FALSE;
    }
    return 1;
}


bool MDrvSclVipSetPeakingConfig(void *pvCfg)
{
    MDrvSclVipPeakingConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_Peaking_gain_ad_Y_Main-PQ_IP_VIP_HLPF_Main+1)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0]     = (u8*)&(pCfg->stHLPF);
    p8PQType[1]     = (u8*)&(pCfg->stHLPFDith);
    p8PQType[2]     = (u8*)&(pCfg->stVLPFcoef1);
    p8PQType[3]     = (u8*)&(pCfg->stVLPFcoef2);
    p8PQType[4]     = (u8*)&(pCfg->stVLPFDith);
    p8PQType[5]     = (u8*)&(pCfg->stOnOff);
    p8PQType[6]     = (u8*)&(pCfg->stBand);
    p8PQType[7]     = (u8*)&(pCfg->stAdp);
    p8PQType[8]     = (u8*)&(pCfg->stPcor);
    p8PQType[9]     = (u8*)&(pCfg->stAdpY);
    p8PQType[10]    = (u8*)&(pCfg->stGain);
    p8PQType[11]    = (u8*)&(pCfg->stGainAdpY);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_HLPF_Main,PQ_IP_VIP_Peaking_gain_ad_Y_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}
bool MDrvSclVipReqWdrMloadBuffer(s16 s16Idx)
{
    void *pvCtx;
    bool bRet = TRUE;
    u32 u32MiuAddr;
    char sg_Iq_Wdr_name[16];
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    _MDrvSclVipMemNaming(sg_Iq_Wdr_name,s16Idx);
    gstGlobalMVipSet->u32VirWdrMloadBuf =
        (u32)DrvSclOsDirectMemAlloc
        (sg_Iq_Wdr_name,(u32)MDRV_SCLVIP_WDR_MLOAD_SIZE,(DrvSclOsDmemBusType_t *)&gstGlobalMVipSet->u32WdrMloadBuf);
    if(gstGlobalMVipSet->u32VirWdrMloadBuf)
    {
        u32MiuAddr = gstGlobalMVipSet->u32WdrMloadBuf;
        DrvSclVipSetWdrMloadBuffer(u32MiuAddr);
        SCL_DBG(SCL_DBG_LV_VIP(),"[MDRVVIP]%s MLOAD Buffer:%lx\n", __FUNCTION__,gstGlobalMVipSet->u32VirWdrMloadBuf);
        bRet = TRUE;
    }
    else
    {
        SCL_ERR("[MDRVVIP]%s MLOAD Buffer:Allocate Fail\n", __FUNCTION__);
        bRet = FALSE;
    }
    MDrvSclVipMutexUnlock();
    return bRet;
}
bool MDrvSclVipFreeWdrMloadBuffer(s16 s16Idx)
{
    void *pvCtx;
    bool bRet = TRUE;
    char sg_Iq_Wdr_name[16];
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    if(gstGlobalMVipSet->u32VirWdrMloadBuf)
    {
        _MDrvSclVipMemNaming(sg_Iq_Wdr_name,s16Idx);
        DrvSclOsDirectMemFree
            (sg_Iq_Wdr_name,MDRV_SCLVIP_WDR_MLOAD_SIZE,
            (void *)gstGlobalMVipSet->u32VirWdrMloadBuf,(DrvSclOsDmemBusType_t)gstGlobalMVipSet->u32WdrMloadBuf);
        gstGlobalMVipSet->u32WdrMloadBuf = NULL;
        gstGlobalMVipSet->u32VirWdrMloadBuf = NULL;
        bRet = TRUE;
    }
    else
    {
        SCL_ERR("[MDRVVIP]%s MLOAD Buffer:Free Not Allocate\n", __FUNCTION__);
        bRet = FALSE;
    }
    MDrvSclVipMutexUnlock();
    return bRet;
}
bool MDrvSclVipSetMaskOnOff(void *pvCfg)
{
    MDrvSclVipSetMaskOnOff_t *pCfg = pvCfg;
    DrvSclVipSetMaskOnOff_t stCfg;
    void *pvCtx;
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipSetMaskOnOff_t));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    DrvSclOsMemcpy(&stCfg,pCfg,sizeof(DrvSclVipSetMaskOnOff_t));
    DrvSclVipSetMaskOnOff(&stCfg);
    MDrvSclVipMutexUnlock();
    SCL_DBG(SCL_DBG_LV_VIP(),"[MDRVVIP]%s\n", __FUNCTION__);
    return TRUE;
}
bool MDrvSclVipGetWdrHistogram(void *pvCfg)
{
    MDrvSclVipWdrRoiReport_t *pCfg = pvCfg;
    DrvSclVipWdrRoiReport_t stCfg;
    void *pvCtx;
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipWdrRoiReport_t));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    DrvSclVipGetWdrHistogram(&stCfg);
    DrvSclOsMemcpy(pCfg,&stCfg,sizeof(MDrvSclVipWdrRoiReport_t));
    MDrvSclVipMutexUnlock();
    SCL_DBG(SCL_DBG_LV_VIP(),"[MDRVVIP]%s\n", __FUNCTION__);
    return TRUE;
}
bool MDrvSclVipGetNRHistogram(void *pvCfg)
{
    DrvSclVipGetNRHistogram(pvCfg);
    return 1;
}
bool MDrvSclVipSetRoiConfig(void *pvCfg)
{
    MDrvSclVipWdrRoiHist_t *pCfg = pvCfg;
    DrvSclVipWdrRoiHist_t stCfg;
    void *pvCtx;
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipWdrRoiHist_t));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    DrvSclOsMemcpy(&stCfg,pCfg,sizeof(DrvSclVipWdrRoiHist_t));
    DrvSclVipSetRoiConfig(&stCfg);
    MDrvSclVipMutexUnlock();
    return TRUE;
}
bool MDrvSclVipSetHistogramConfig(void *pvCfg)
{
    ST_VIP_DLC_HISTOGRAM_CONFIG stDLCCfg;
    u16 i;
    MDrvSclVipDlcHistogramConfig_t *pCfg = pvCfg;
    void *pvCtx;
    DrvSclOsMemset(&stDLCCfg,0,sizeof(ST_VIP_DLC_HISTOGRAM_CONFIG));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    stDLCCfg.bVariable_Section  = pCfg->bVariable_Section;
    stDLCCfg.bstat_MIU          = pCfg->bstat_MIU;
    stDLCCfg.bcurve_fit_en      = pCfg->bcurve_fit_en;
    stDLCCfg.bcurve_fit_rgb_en  = pCfg->bcurve_fit_rgb_en;
    stDLCCfg.bDLCdither_en      = pCfg->bDLCdither_en;
    stDLCCfg.bhis_y_rgb_mode_en = pCfg->bhis_y_rgb_mode_en;
    stDLCCfg.bstatic            = pCfg->bstatic;
    stDLCCfg.bRange             = pCfg->bRange;
    stDLCCfg.u16Vst             = pCfg->u16Vst;
    stDLCCfg.u16Hst             = pCfg->u16Hst;
    stDLCCfg.u16Vnd             = pCfg->u16Vnd;
    stDLCCfg.u16Hnd             = pCfg->u16Hnd;
    stDLCCfg.u8HistSft          = pCfg->u8HistSft;
    stDLCCfg.u8trig_ref_mode    = pCfg->u8trig_ref_mode;
    stDLCCfg.u32StatBase[0]     = pCfg->u32StatBase[0];
    stDLCCfg.u32StatBase[1]     = pCfg->u32StatBase[1];

    for(i=0;i<VIP_DLC_HISTOGRAM_SECTION_NUM;i++)
    {
        stDLCCfg.u8Histogram_Range[i] = pCfg->u8Histogram_Range[i];
    }
    MDrvSclVipMutexLock();

    if(DrvSclVipSetDlcHistogramConfig(&stDLCCfg,0) == 0)
    {
        SCL_DBG(SCL_DBG_LV_VIP(), "[MDRVVIP]%s, Fail\n", __FUNCTION__);
        MDrvSclVipMutexUnlock();
        return FALSE;
    }
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipGetDlcHistogramReport(void *pvCfg)
{
    DrvSclVipDlcHistogramReport_t stDLCCfg;
    u16 i;
    MDrvSclVipDlcHistogramReport_t *pCfg = pvCfg;
    void *pvCtx;
    DrvSclOsMemset(&stDLCCfg,0,sizeof(DrvSclVipDlcHistogramReport_t));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    DrvSclVipGetDlcHistogramConfig(&stDLCCfg);
    pCfg->u32PixelWeight = stDLCCfg.u32PixelWeight;
    pCfg->u32PixelCount  = stDLCCfg.u32PixelCount;
    pCfg->u8MaxPixel     = stDLCCfg.u8MaxPixel;
    pCfg->u8MinPixel     = stDLCCfg.u8MinPixel;
    pCfg->u8Baseidx      = stDLCCfg.u8Baseidx;
    for(i=0;i<VIP_DLC_HISTOGRAM_REPORT_NUM;i++)
    {
        stDLCCfg.u32Histogram[i]    = DrvSclVipGetDlcHistogramReport(i);
        pCfg->u32Histogram[i]       = stDLCCfg.u32Histogram[i];
    }

    return TRUE;
}

bool MDrvSclVipSetDlcConfig(void *pvCfg)
{
    MDrvSclVipDlcConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_YC_gain_offset_Main-PQ_IP_VIP_DLC_His_range_Main+1)];//add PQ_IP_VIP_YC_gain_offset_Main
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->sthist);
    p8PQType[1] = (u8*)&(pCfg->stEn);
    p8PQType[2] = (u8*)&(pCfg->stDither);
    p8PQType[3] = (u8*)&(pCfg->stHistH);
    p8PQType[4] = (u8*)&(pCfg->stHistV);
    p8PQType[5] = (u8*)&(pCfg->stPC);
    p8PQType[6] = (u8*)&(pCfg->stGainOffset);
    MDrvSclVipMutexLock();

    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_DLC_His_range_Main,PQ_IP_VIP_YC_gain_offset_Main,p8PQType);

    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetLceConfig(void *pvCfg)
{
    MDrvSclVipLceConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_LCE_curve_Main-PQ_IP_VIP_LCE_Main+1)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stOnOff);
    p8PQType[1] = (u8*)&(pCfg->stDITHER);
    p8PQType[2] = (u8*)&(pCfg->stSet);
    p8PQType[3] = (u8*)&(pCfg->stCurve);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_LCE_Main,PQ_IP_VIP_LCE_curve_Main,p8PQType);

    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetUvcConfig(void *pvCfg)
{
    u8 *pUVC = NULL;
    MDrvSclVipUvcConfig_t *pCfg = pvCfg;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    pUVC = (u8*)&(pCfg->stUVC);
    MDrvSclVipMutexLock();
    _MDrvSclVipSetPqByType(PQ_IP_VIP_UVC_Main,pUVC);
    MDrvSclVipMutexUnlock();
    return TRUE;
}


bool MDrvSclVipSetIhcConfig(void *pvCfg)
{
    MDrvSclVipIhcConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_IHC_dither_Main-PQ_IP_VIP_IHC_Main+2)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stOnOff);
    p8PQType[1] = (u8*)&(pCfg->stYmd);
    p8PQType[2] = (u8*)&(pCfg->stDither);
    p8PQType[3] = (u8*)&(pCfg->stset);
    MDrvSclVipMutexLock();
    _MDrvSclVipSetPqByType(PQ_IP_VIP_IHC_SETTING_Main,p8PQType[3]);
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_IHC_Main,PQ_IP_VIP_IHC_dither_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetICEConfig(void *pvCfg)
{
    MDrvSclVipIccConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_ICC_SETTING_Main-PQ_IP_VIP_ICC_dither_Main+2)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stEn);
    p8PQType[1] = (u8*)&(pCfg->stYmd);
    p8PQType[2] = (u8*)&(pCfg->stDither);
    p8PQType[3] = (u8*)&(pCfg->stSet);
    MDrvSclVipMutexLock();
    _MDrvSclVipSetPqByType(PQ_IP_VIP_ICC_SETTING_Main,p8PQType[3]);
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_ICC_Main,PQ_IP_VIP_ICC_dither_Main,p8PQType);

    MDrvSclVipMutexUnlock();
    return TRUE;
}


bool MDrvSclVipSetIhcICCADPYConfig(void *pvCfg)
{
    MDrvSclVipIhcIccConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_Ymode_Yvalue_SETTING_Main-PQ_IP_VIP_Ymode_Yvalue_ALL_Main+1)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stYmdall);
    p8PQType[1] = (u8*)&(pCfg->stYmdset);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_Ymode_Yvalue_ALL_Main,PQ_IP_VIP_Ymode_Yvalue_SETTING_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetIbcConfig(void *pvCfg)
{
    MDrvSclVipIbcConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_IBC_SETTING_Main-PQ_IP_VIP_IBC_Main+1)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stEn);
    p8PQType[1] = (u8*)&(pCfg->stDither);
    p8PQType[2] = (u8*)&(pCfg->stSet);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_IBC_Main,PQ_IP_VIP_IBC_SETTING_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetFccConfig(void *pvCfg)
{
    MDrvSclVipFccConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_FCC_T9_Main-PQ_IP_VIP_FCC_T1_Main+2)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[9] = (u8*)&(pCfg->stfr);
    p8PQType[0] = (u8*)&(pCfg->stT[0]);
    p8PQType[1] = (u8*)&(pCfg->stT[1]);
    p8PQType[2] = (u8*)&(pCfg->stT[2]);
    p8PQType[3] = (u8*)&(pCfg->stT[3]);
    p8PQType[4] = (u8*)&(pCfg->stT[4]);
    p8PQType[5] = (u8*)&(pCfg->stT[5]);
    p8PQType[6] = (u8*)&(pCfg->stT[6]);
    p8PQType[7] = (u8*)&(pCfg->stT[7]);
    p8PQType[8] = (u8*)&(pCfg->stT9);
    MDrvSclVipMutexLock();
    _MDrvSclVipSetPqByType(PQ_IP_VIP_FCC_full_range_Main,p8PQType[9]);
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_FCC_T1_Main,PQ_IP_VIP_FCC_T9_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}


bool MDrvSclVipSetAckConfig(void *pvCfg)
{
    MDrvSclVipAckConfig_t *pCfg = pvCfg;
    u8 *p8PQType[(PQ_IP_VIP_YCbCr_Clip_Main-PQ_IP_VIP_ACK_Main+1)];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->stACK);
    p8PQType[1] = (u8*)&(pCfg->stclip);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_VIP_ACK_Main,PQ_IP_VIP_YCbCr_Clip_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetNlmConfig(void *pvCfg)
{
    u8 *stNLM = NULL;
    MDrvSclVipNlmConfig_t *pCfg = pvCfg;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    stNLM  = (u8*)&(pCfg->stNLM);
    MDrvSclVipMutexLock();
    _MDrvSclVipSetPqByType(PQ_IP_NLM_Main,stNLM);
    MDrvSclVipMutexUnlock();
    return TRUE;
}

bool MDrvSclVipSetNlmSramConfig(MDrvSclVipNlmSramConfig_t stSRAM)
{
    DrvSclVipNlmSramConfig_t stCfg;
    void *pvCtx;
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipNlmSramConfig_t));
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    stCfg.u32baseadr    = stSRAM.u32Baseadr;
    stCfg.bCLientEn     = stSRAM.bEn;
    stCfg.u32viradr     = stSRAM.u32viradr;
    stCfg.btrigContinue = 0;                    //single
    stCfg.u16depth      = VIP_NLM_ENTRY_NUM;    // entry
    stCfg.u16reqlen     = VIP_NLM_ENTRY_NUM;
    stCfg.u16iniaddr    = 0;
    SCL_DBG(SCL_DBG_LV_VIP(),
        "[MDRVVIP]%s, flag:%hhx ,addr:%lx,addr:%lx \n", __FUNCTION__,stSRAM.bEn,stSRAM.u32viradr,stSRAM.u32Baseadr);
    DrvSclVipSetNlmSramConfig(&stCfg);
    return TRUE;
}

bool MDrvSclVipSetVipOtherConfig(void *pvCfg)
{
    MDrvSclVipConfig_t *pCfg = pvCfg;
    u8 *p8PQType[2];
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    p8PQType[0] = (u8*)&(pCfg->st422_444);
    p8PQType[1] = (u8*)&(pCfg->stBypass);
    MDrvSclVipMutexLock();
    _MDrvSclVipForSetEachPqTypeByIp(PQ_IP_422to444_Main,PQ_IP_VIP_Main,p8PQType);
    MDrvSclVipMutexUnlock();
    return TRUE;
}


void MDrvSclVipSetMultiSensorConfig(bool bEn)
{
    MDrvSclVipMutexLock();
    if(bEn)
    {
        gbMultiSensor++;
    }
    else
    {
        if(gbMultiSensor)
        {
            gbMultiSensor--;
        }
    }
    MDrvSclVipMutexUnlock();
}

bool MDrvSclVipSetWdrMloadConfig(void)
{
    void *pvCtx;
    bool bEn = 0;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    if(DrvSclVipGetWdrOnOff())
    {
        bEn = (gbMultiSensor>1)? 1 : 0;// for >2 is real multisensor
    }
    else
    {
        bEn = 0;
    }
    DrvSclVipSetWdrMloadConfig(bEn,gstGlobalMVipSet->u32WdrMloadBuf);
    return bEn;
}
bool MDrvSclVipSetAipConfig(MDrvSclVipAipConfig_t *stCfg)
{
    void * pvPQSetParameter;
    u16 u16StructSize;
    u16 u16AIPsheet;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    u16AIPsheet = _GetAipOffset(stCfg->u16AIPType);
    u16StructSize = MDrv_Scl_PQ_GetIPRegCount(u16AIPsheet);
    MDrvSclVipMutexLock();
    pvPQSetParameter = (void *)_MDrvSclVipFillSettingBuffer(u16AIPsheet,stCfg->u32Viraddr);
    if(!pvPQSetParameter)
    {
        MDrvSclVipMutexUnlock();
        return 0;
    }
    _MDrvSclVipAipSettingDebugMessage(stCfg,pvPQSetParameter,u16StructSize);
    _MDrvSclVipSetPqByType(u16AIPsheet,(u8 *)pvPQSetParameter);
    DrvSclOsVirMemFree((void *)pvPQSetParameter);
    MDrvSclVipMutexUnlock();
    return 1;
}


bool MDrvSclVipSetAipSramConfig(MDrvSclVipAipSramConfig_t *stCfg)
{
    void * pvPQSetParameter;
    void * pvPQSetPara;
    u16 u16StructSize;
    DrvSclVipSramType_e enAIPType;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    MDrvSclVipMutexLock();
    u16StructSize = _MDrvSclVipGetSramBufferSize(stCfg->enAIPType);
    pvPQSetParameter = DrvSclOsVirMemalloc(u16StructSize);
    if(pvPQSetParameter==NULL)
    {
        MDrvSclVipMutexUnlock();
        return 0;
    }
    DrvSclOsMemset(pvPQSetParameter,0,u16StructSize);

    if(DrvSclOsCopyFromUser(pvPQSetParameter, (void  *)stCfg->u32Viraddr, u16StructSize))
    {
        DrvSclOsMemcpy(pvPQSetParameter, (void *)stCfg->u32Viraddr, u16StructSize);
    }
    enAIPType = stCfg->enAIPType;
    _MDrvSclVipAipSramSettingDebugMessage(stCfg,pvPQSetParameter,u16StructSize);
    // NOt need to lock CMDQ ,because hal will do it.
    pvPQSetPara = DrvSclVipSetAipSramConfig(pvPQSetParameter,enAIPType);
    if(pvPQSetPara!=NULL)
    {
        DrvSclOsVirMemFree(pvPQSetPara);
    }
    MDrvSclVipMutexUnlock();
    return DrvSclVipGetSramCheckPass();
}
bool MDrvSclVipSetInitIpmConfig(MDrvSclVipIpmConfig_t *pCfg)
{
    DrvSclVipIpmConfig_t stIPMCfg;
    DrvSclOsMemset(&stIPMCfg,0,sizeof(DrvSclVipIpmConfig_t));
    SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "%s:PhyAddr=%lx, width=%x, height=%x \n",  __FUNCTION__, pCfg->u32YCMPhyAddr, pCfg->u16Width, pCfg->u16Height);
    _MDrvSclVipFillIpmStruct(pCfg,&stIPMCfg);
    // ToDo
    // DrvSclVipSetPrv2CropOnOff(stIPMCfg.bYCMRead,);
    if(DrvSclVipSetIPMConfig(&stIPMCfg) == FALSE)
    {
        SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_NORMAL, "[VIP] Set IPM Config Fail\n");
        return FALSE;
    }
    else
    {
        return TRUE;;
    }
}

void MDrvSclVipFillBasicStructSetPqCfg(MDrvSclVipConfigType_e enVIPtype,void *pPointToCfg,MDrvSclVipSetPqConfig_t* stSetPQCfg)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    stSetPQCfg->pPointToCfg = pPointToCfg;
    switch(enVIPtype)
    {
        case E_MDRV_SCLVIP_SETROI_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipWdrRoiHist_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetRoiConfig;
            break;
        case E_MDRV_SCLVIP_MCNR_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipMcnrConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetMcnrConfig;
            break;

        case E_MDRV_SCLVIP_ACK_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipAckConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetAckConfig;
            break;

        case E_MDRV_SCLVIP_IBC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipIbcConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetIbcConfig;
            break;

        case E_MDRV_SCLVIP_IHCICC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipIhcIccConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetIhcICCADPYConfig;
            break;

        case E_MDRV_SCLVIP_ICC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipIccConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetICEConfig;
            break;

        case E_MDRV_SCLVIP_IHC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipIhcConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetIhcConfig;
            break;

        case E_MDRV_SCLVIP_FCC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipFccConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetFccConfig;
            break;
        case E_MDRV_SCLVIP_UVC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipUvcConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetUvcConfig;
            break;

        case E_MDRV_SCLVIP_DLC_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipDlcConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetDlcConfig;
            break;

        case E_MDRV_SCLVIP_DLC_HISTOGRAM_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipDlcHistogramConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetHistogramConfig;
            break;

        case E_MDRV_SCLVIP_LCE_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipLceConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetLceConfig;
            break;

        case E_MDRV_SCLVIP_PEAKING_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipPeakingConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetPeakingConfig;
            break;

        case E_MDRV_SCLVIP_NLM_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipNlmConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetNlmConfig;
            break;
        case E_MDRV_SCLVIP_CONFIG:
                stSetPQCfg->u32StructSize = sizeof(MDrvSclVipConfig_t);
                stSetPQCfg->pfForSet = MDrvSclVipSetVipOtherConfig;
            break;
        default:
                stSetPQCfg->u32StructSize = 0;
                stSetPQCfg->bSetConfigFlag = 0;
                stSetPQCfg->pfForSet = NULL;
            break;

    }
}

#if SCLOS_NONUSEFUNC
bool gbVIPCheckCMDQorPQ = 0;
#define _IsCmdqNeedToReturnOrigin()             (gbVIPCheckCMDQorPQ == E_MDRV_SCLVIP_CMDQ_CHECK_RETURN_ORI)
#define _IsCmdqAlreadySetting()                 (gbVIPCheckCMDQorPQ == E_MDRV_SCLVIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsSetCmdq()                            (gbVIPCheckCMDQorPQ >= E_MDRV_SCLVIP_CMDQ_CHECK_ALREADY_SETINNG)
#define _IsAutoSetting()                        (gbVIPCheckCMDQorPQ == E_MDRV_SCLVIP_CMDQ_CHECK_AUTOSETTING\
                                                 || gbVIPCheckCMDQorPQ == E_MDRV_SCLVIP_CMDQ_CHECK_PQ)
#define _IsCheckPQorCMDQmode()                  (gbVIPCheckCMDQorPQ)
#define _IsNotToCheckPQorCMDQmode()             (!gbVIPCheckCMDQorPQ)
#define _SetSuspendResetFlag(u32flag)           (gstGlobalMVipSet->gstSupCfg.bresetflag |= u32flag)
#define _IsSuspendResetFlag(enType)             ((gstGlobalMVipSet->gstSupCfg.bresetflag &enType)>0)
#define _SetSuspendAipResetFlag(u32flag)        (gstGlobalMVipSet->gstSupCfg.bAIPreflag |= (0x1 << u32flag))
#define _IsSuspendAipResetFlag(enType)          (gstGlobalMVipSet->gstSupCfg.bAIPreflag &(0x1 << enType))
void MDrvSclVipFreeMemory(void)
{
    MDrvSclVipAipType_e enVIPtype;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    if(gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr)
    {
        DrvSclOsVirMemFree((void *)gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr);
        gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr = 0;
    }
    for(enVIPtype = 0;enVIPtype<E_MDRV_SCLVIP_AIP_NUM;enVIPtype++)
    {
        if(gstGlobalMVipSet->gstSupCfg.staip[enVIPtype].u32Viraddr)
        {
            DrvSclOsVirMemFree((void *)gstGlobalMVipSet->gstSupCfg.staip[enVIPtype].u32Viraddr);
            gstGlobalMVipSet->gstSupCfg.staip[enVIPtype].u32Viraddr = 0;
        }
    }
    DrvSclVipFreeMem();
}
void _MDrvSclVipResetAlreadySetting(MDrvSclVipSetPqConfig_t *stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = DrvSclOsMemalloc(stSetPQCfg->u32StructSize, GFP_KERNEL);
    DrvSclOsMemset(pvPQSetParameter, 0, stSetPQCfg->u32StructSize);
    stSetPQCfg->pfForSet(pvPQSetParameter);
    DrvSclOsMemFree(pvPQSetParameter);
}
void _MDrvSclVipModifyAipAlreadySetting(MDrvSclVipAipType_e enAIPtype, MDrvSclVipResetType_e enRe)
{
    MDrvSclVipAipConfig_t staip;
    bool bSet;
    u32 u32viraddr;
    DrvSclOsMemset(&staip,0,sizeof(MDrvSclVipAipConfig_t));
    bSet = (enRe == E_MDRV_SCLVIP_RESET_ZERO) ? 0 : 0xFF;
    staip = gstGlobalMVipSet->gstSupCfg.staip[enAIPtype];
    staip.u32Viraddr =(u32)DrvSclOsVirMemalloc(MDrv_Scl_PQ_GetIPRegCount(_GetAipOffset(enAIPtype)));
    if(!staip.u32Viraddr)
    {
        return;
    }
    DrvSclOsMemset((void*)staip.u32Viraddr, bSet, MDrv_Scl_PQ_GetIPRegCount(_GetAipOffset(enAIPtype)));
    if(enRe == E_MDRV_SCLVIP_RESET_ZERO)
    {
        MDrvSclVipSetAipConfig(&staip);
    }
    else if(enRe == E_MDRV_SCLVIP_RESET_ALREADY)
    {
        if(gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u32Viraddr)
        {
            MDrvSclVipSetAipConfig(&gstGlobalMVipSet->gstSupCfg.staip[enAIPtype]);
        }
    }
    else if (enRe == E_MDRV_SCLVIP_FILL_GOLBAL_FULL)
    {
        u32viraddr = gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u32Viraddr;
        gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u32Viraddr = staip.u32Viraddr;
        gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u16AIPType = enAIPtype;
        staip.u32Viraddr = u32viraddr;
    }
    DrvSclOsVirMemFree((void *)staip.u32Viraddr);
}
void _MDrvSclVipFillAutoSetStruct(MDrvSclVipSetPqConfig_t *stSetPQCfg)
{
    void * pvPQSetParameter;
    pvPQSetParameter = DrvSclOsMemalloc(stSetPQCfg->u32StructSize, GFP_KERNEL);
    DrvSclOsMemset(pvPQSetParameter, 0xFF, stSetPQCfg->u32StructSize);
    DrvSclOsMemcpy(stSetPQCfg->pGolbalStructAddr, pvPQSetParameter, stSetPQCfg->u32StructSize);
    DrvSclOsMemFree(pvPQSetParameter);
}
void _MDrvSclVipModifyMcnrAlreadySetting( MDrvSclVipResetType_e enRe)
{
    MDrvSclVipMcnrConfig_t stmcnr;
    bool bSet;
    u32 u32viraddr;
    DrvSclOsMemset(&stmcnr,0,sizeof(MDrvSclVipMcnrConfig_t));
    SCL_ERR("[MVIP]%s",__FUNCTION__);
    bSet = (enRe == E_MDRV_SCLVIP_RESET_ZERO) ? 0 : 0xFF;
    stmcnr = gstGlobalMVipSet->gstSupCfg.stmcnr;
    stmcnr.u32Viraddr = (u32)DrvSclOsVirMemalloc(MDrv_Scl_PQ_GetIPRegCount(PQ_IP_MCNR_Main));
    if(!stmcnr.u32Viraddr)
    {
        return;
    }
    stmcnr.bEnMCNR = (enRe == E_MDRV_SCLVIP_RESET_ZERO) ? 0 :
                     (enRe == E_MDRV_SCLVIP_FILL_GOLBAL_FULL) ? 1 :
                        gstGlobalMVipSet->gstSupCfg.stmcnr.bEnMCNR;

    stmcnr.bEnCIIR = (enRe == E_MDRV_SCLVIP_RESET_ZERO) ? 0 :
                     (enRe == E_MDRV_SCLVIP_FILL_GOLBAL_FULL) ? 1 :
                        gstGlobalMVipSet->gstSupCfg.stmcnr.bEnCIIR;
    DrvSclOsMemset((void*)stmcnr.u32Viraddr,  bSet, MDrv_Scl_PQ_GetIPRegCount(PQ_IP_MCNR_Main));
    if(enRe == E_MDRV_SCLVIP_RESET_ZERO)
    {
        MDrvSclVipSetMcnrConfig(&stmcnr);
    }
    else if(enRe == E_MDRV_SCLVIP_RESET_ALREADY)
    {
        MDrvSclVipSetMcnrConfig(&gstGlobalMVipSet->gstSupCfg.stmcnr);
    }
    else if (enRe == E_MDRV_SCLVIP_FILL_GOLBAL_FULL)
    {
        u32viraddr = gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr;
        gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr = stmcnr.u32Viraddr;
        stmcnr.u32Viraddr = u32viraddr;
        gstGlobalMVipSet->gstSupCfg.stmcnr.bEnMCNR = stmcnr.bEnMCNR;
        gstGlobalMVipSet->gstSupCfg.stmcnr.bEnCIIR = stmcnr.bEnCIIR;
    }
    DrvSclOsVirMemFree((void *)stmcnr.u32Viraddr);
}
void _MDrvSclVipPrepareCheckSetting(MDrvSclVipConfigType_e enVIPtype)
{
    MDrvSclVipSetPqConfig_t stSetPQCfg;
    bool bEn;
    u8 u8framecount;
    DrvSclOsMemset(&stSetPQCfg,0,sizeof(MDrvSclVipSetPqConfig_t));
    bEn = (_IsSetCmdq()) ? 1 : 0 ;
    u8framecount = 0;
    MDrvSclVipFillBasicStructSetPqCfg(enVIPtype,NULL,&stSetPQCfg);
    if(_IsCmdqAlreadySetting())
    {
        if(enVIPtype ==E_MDRV_SCLVIP_MCNR_CONFIG)
        {
            _MDrvSclVipModifyMcnrAlreadySetting(E_MDRV_SCLVIP_RESET_ZERO);
        }
        else
        {
            _MDrvSclVipResetAlreadySetting(&stSetPQCfg);
        }
    }
    else if(_IsAutoSetting())
    {
        if(enVIPtype ==E_MDRV_SCLVIP_MCNR_CONFIG)
        {
            _MDrvSclVipModifyMcnrAlreadySetting(E_MDRV_SCLVIP_FILL_GOLBAL_FULL);
        }
        else
        {
            _MDrvSclVipFillAutoSetStruct(&stSetPQCfg);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP,
                "[VIP]%x:addr:%lx\n",enVIPtype,(u32)stSetPQCfg.pGolbalStructAddr);
        }
    }
}
void _MDrvSclVipPrepareAipCheckSetting(MDrvSclVipAipType_e enAIPtype)
{
    bool bEn;
    u8 u8framecount;
    bEn = (_IsSetCmdq()) ? 1 : 0 ;
    u8framecount = 0;
    if(_IsCmdqAlreadySetting())
    {
        _MDrvSclVipModifyAipAlreadySetting(enAIPtype,E_MDRV_SCLVIP_RESET_ZERO);
    }
    else if(_IsAutoSetting())
    {
        _MDrvSclVipModifyAipAlreadySetting(enAIPtype,E_MDRV_SCLVIP_FILL_GOLBAL_FULL);
    }
}
void _MDrvSclVipForPrepareCheckSetting(void)
{
    MDrvSclVipConfigType_e enVIPtype;
    MDrvSclVipAipType_e enAIPType;
    for(enVIPtype =E_MDRV_SCLVIP_ACK_CONFIG;enVIPtype<=E_MDRV_SCLVIP_MCNR_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            _MDrvSclVipPrepareCheckSetting(enVIPtype);
        }
    }
    for(enAIPType =E_MDRV_SCLVIP_AIP_YEE;enAIPType<E_MDRV_SCLVIP_AIP_NUM;(enAIPType++))
    {
        if(_IsSuspendAipResetFlag(enAIPType))
        {
            _MDrvSclVipPrepareAipCheckSetting(enAIPType);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]AIP: %d \n",enAIPType);
        }
    }
}
void _MDrvSclVipForSetEachIp(void)
{
    MDrvSclVipConfigType_e enVIPtype;
    MDrvSclVipAipType_e enAIPType;
    MDrvSclVipSetPqConfig_t stSetPQCfg;
    DrvSclOsMemset(&stSetPQCfg,0,sizeof(MDrvSclVipSetPqConfig_t));
    for(enVIPtype =E_MDRV_SCLVIP_ACK_CONFIG;enVIPtype<=E_MDRV_SCLVIP_MCNR_CONFIG;(enVIPtype*=2))
    {
        if(_IsSuspendResetFlag(enVIPtype))
        {
            MDrvSclVipFillBasicStructSetPqCfg(enVIPtype,NULL,&stSetPQCfg);
            stSetPQCfg.pfForSet((void *)stSetPQCfg.pGolbalStructAddr);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]%s %d \n", __FUNCTION__,enVIPtype);
        }
    }
    for(enAIPType =E_MDRV_SCLVIP_AIP_YEE;enAIPType<E_MDRV_SCLVIP_AIP_NUM;(enAIPType++))
    {
        if(_IsSuspendAipResetFlag(enAIPType))
        {
            MDrvSclVipSetAipConfig(&gstGlobalMVipSet->gstSupCfg.staip[enAIPType]);
            SCL_DBG(SCL_DBG_LV_VIP()&EN_DBGMG_VIPLEVEL_VIPSUP, "[MDRVVIP]AIP: %d \n",enAIPType);
        }
    }
}
void _MDrvSclVipCopyAipConfigToGlobal(MDrvSclVipAipType_e enVIPtype, void *pCfg, u32 u32StructSize)
{
    if(_IsNotToCheckPQorCMDQmode())
    {
        DrvSclOsMemcpy(&gstGlobalMVipSet->gstSupCfg.staip[enVIPtype], pCfg,u32StructSize);
        gstGlobalMVipSet->gstSupCfg.staip[enVIPtype].u16AIPType= enVIPtype;
        _SetSuspendAipResetFlag(enVIPtype);
    }
}
void _MDrvSclVipCopyIpConfigToGlobal(MDrvSclVipConfigType_e enVIPtype,void *pCfg)
{
    MDrvSclVipSetPqConfig_t stSetPQCfg;
    DrvSclOsMemset(&stSetPQCfg,0,sizeof(MDrvSclVipSetPqConfig_t));
    if(_IsNotToCheckPQorCMDQmode())
    {
        MDrvSclVipFillBasicStructSetPqCfg(enVIPtype,pCfg,&stSetPQCfg);
        DrvSclOsMemcpy(stSetPQCfg.pGolbalStructAddr, stSetPQCfg.pPointToCfg,stSetPQCfg.u32StructSize);
        _SetSuspendResetFlag(enVIPtype);
    }
}
void MDrvSclVipAllocMemory(void)
{
    MDrvSclVipAipType_e enAIPtype;
    u16 u16StructSize;
    u16StructSize = MDrv_Scl_PQ_GetIPRegCount(PQ_IP_MCNR_Main);
    if(!gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr)
    {
        gstGlobalMVipSet->gstSupCfg.stmcnr.u32Viraddr = (u32)DrvSclOsVirMemalloc(u16StructSize);
    }
    for(enAIPtype = E_MDRV_SCLVIP_AIP_YEE;enAIPtype<E_MDRV_SCLVIP_AIP_NUM;enAIPtype++)
    {
        u16StructSize = MDrv_Scl_PQ_GetIPRegCount(_GetAipOffset(enAIPtype));
        if(!gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u32Viraddr)
        {
            gstGlobalMVipSet->gstSupCfg.staip[enAIPtype].u32Viraddr = (u32)DrvSclOsVirMemalloc(u16StructSize);
        }
    }
    DrvSclVipAllocMem();
}
void MDrvSclVipSuspendResetFlagInit(void)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    gstGlobalMVipSet->gstSupCfg.bresetflag = 0;
}
void _MDrvSclVipResumeDumpSRAM(void)
{
    if(_IsSuspendResetFlag(E_MDRV_SCLVIP_NLM_CONFIG))
    {
        if(gstGlobalMVipSet->gstSupCfg.stnlm.stSRAM.bEn)
        {
            MDrvSclVipSetNlmSramConfig(gstGlobalMVipSet->gstSupCfg.stnlm.stSRAM);
        }
        else if(!gstGlobalMVipSet->gstSupCfg.stnlm.stSRAM.bEn && gstGlobalMVipSet->gstSupCfg.stnlm.stSRAM.u32viradr)
        {
            MDrvSclVipSetNlmSramConfig(gstGlobalMVipSet->gstSupCfg.stnlm.stSRAM);
        }

    }
    DrvSclVipSramDump();
}
void MDrvSclVipResetEachIP(void)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    _MDrvSclVipForSetEachIp();
    _MDrvSclVipResumeDumpSRAM();
}
void MDrvSclVipCheckRegister(void)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_Scl_PQ_Check_Type(0,PQ_CHECK_REG);
    MDrvSclVipResetEachIP();
    MDRv_Scl_PQ_Check_Type(0,PQ_CHECK_OFF);
}

void MDrvSclVipCheckConsist(void)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    SCL_ERR( "[MDRVVIP]%s  \n", __FUNCTION__);
    MDRv_Scl_PQ_Check_Type(0,PQ_CHECK_SIZE);
    MDrvSclVipResetEachIP();
    MDRv_Scl_PQ_Check_Type(0,PQ_CHECK_OFF);
}
void MDrvSclVipPrepareStructToCheckRegister(MDrvSclVipCmdqCheckType_e enCheckType)
{
    MDrvSclVipAipType_e enIdx = 0;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    SCL_ERR( "[MDRVVIP]%s CHECK_TYPE:%hhd \n", __FUNCTION__,enCheckType);
    MDrvSclVipSetCheckMode(enCheckType);
    if(_IsAutoSetting())
    {
        _SetSuspendResetFlag(0x2FFFF);
        for(enIdx = E_MDRV_SCLVIP_AIP_YEE;enIdx<E_MDRV_SCLVIP_AIP_NUM;enIdx++)
        {
            _SetSuspendAipResetFlag(enIdx);
        }
    }
    _MDrvSclVipForPrepareCheckSetting();

}
void MDrvSclVipSetCheckMode(MDrvSclVipCmdqCheckType_e enCheckType)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    gbVIPCheckCMDQorPQ = enCheckType;
}

#if defined (SCLOS_TYPE_LINUX_KERNEL)
ssize_t MDrvSclVipVipSetRuleShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL VIPSetRule======================\n");
    str += DrvSclOsScnprintf(str, end - str, "NOW Rule:%d\n",DrvSclOsGetVipSetRule());
    str += DrvSclOsScnprintf(str, end - str, "echo 0 > VIPSetRule :Default (RIU+assCMDQ)\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 1 > VIPSetRule :CMDQ in active (RIU blanking+CMDQ active)\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 2 > VIPSetRule :CMDQ & checking(like 1,but add IST to check)\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 3 > VIPSetRule :ALLCMDQ (Rule 4)\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 4 > VIPSetRule :ALLCMDQ & checking(like 4,but add IST to checkSRAM)\n");
    str += DrvSclOsScnprintf(str, end - str, "echo 5 > VIPSetRule :ALLCMDQ & checking(like 4,but add IST to check)\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL VIPSetRule======================\n");
    return (str - buf);
}

ssize_t MDrvSclVipVipShow(char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(MDRV_SCL_CTX_ID_DEFAULT);
    _MDrvSclVipSetGlobal((MDrvSclCtxCmdqConfig_t *)pvCtx);
    str += DrvSclOsScnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    str += DrvSclOsScnprintf(str, end - str, "     IP      Status\n");
    str += DrvSclOsScnprintf(str, end - str, "---------------VIP---------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "  VIP Bypass     %s\n", DrvSclVipGetBypassStatus(E_MDRV_SCLVIP_CONFIG) ? "OFF" :"ON");
    str += DrvSclOsScnprintf(str, end - str, "     MCNR         %s\n", DrvSclVipGetBypassStatus(E_MDRV_SCLVIP_MCNR_CONFIG) ? "Bypass" :"ON");
    str += DrvSclOsScnprintf(str, end - str, "     NLM         %s\n", DrvSclVipGetBypassStatus(E_MDRV_SCLVIP_NLM_CONFIG) ? "Bypass" :"ON");
    str += DrvSclOsScnprintf(str, end - str, "     ACK         %s\n", gstGlobalMVipSet->gstSupCfg.stack.stACK.backen ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     IBC         %s\n", gstGlobalMVipSet->gstSupCfg.stibc.stEn.bIBC_en ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     ICC         %s\n", gstGlobalMVipSet->gstSupCfg.sticc.stEn.bICC_en ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     IHC         %s\n", gstGlobalMVipSet->gstSupCfg.stihc.stOnOff.bIHC_en? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC1        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[0].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC2        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[1].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC3        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[2].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC4        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[3].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC5        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[4].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC6        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[5].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC7        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[6].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC8        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT[7].bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     FCC9        %s\n", gstGlobalMVipSet->gstSupCfg.stfcc.stT9.bEn ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     UVC         %s\n", gstGlobalMVipSet->gstSupCfg.stuvc.stUVC.buvc_en? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, " DLC CURVEFITPW  %s\n", gstGlobalMVipSet->gstSupCfg.stdlc.stEn.bcurve_fit_var_pw_en? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, " DLC  CURVEFIT   %s\n", gstGlobalMVipSet->gstSupCfg.stdlc.stEn.bcurve_fit_en? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, " DLC  STATISTIC  %s\n", gstGlobalMVipSet->gstSupCfg.stdlc.stEn.bstatistic_en? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     LCE         %s\n", gstGlobalMVipSet->gstSupCfg.stlce.stOnOff.bLCE_En ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "     PK          %s\n", gstGlobalMVipSet->gstSupCfg.stpk.stOnOff.bpost_peaking_en ? "ON" :"OFF");
    str += DrvSclOsScnprintf(str, end - str, "---------------AIP--------------------\n");

    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     EE          %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE].u32Viraddr)+71)) ? "ON" :"OFF");
        str += DrvSclOsScnprintf(str, end - str, "     EYEE        %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE].u32Viraddr)+70)) ? "ON" :"OFF");
        str += DrvSclOsScnprintf(str, end - str, "     YEE Merge   %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE].u32Viraddr)+204)) ? "ON" :"OFF");
    }

    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE_AC_LUT].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     YC SEC From %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE_AC_LUT].u32Viraddr)+64)==0) ? "YEE" :
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE_AC_LUT].u32Viraddr)+64)==0x1) ? "2DPK" :
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YEE_AC_LUT].u32Viraddr)+64)==0x2) ? "MIX" :"Debug mode" );
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_PFC].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     WDR GLOB    %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_PFC].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_WDR].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     WDR LOCAL   %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_WDR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_MXNR].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     MXNR        %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_MXNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_UVADJ].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     YUVADJ      %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_UVADJ].u32Viraddr)+1)) ? "ON" :"OFF");
        str += DrvSclOsScnprintf(str, end - str, "     UVADJbyY    %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_UVADJ].u32Viraddr)+0)) ? "ON" :"OFF");
        str += DrvSclOsScnprintf(str, end - str, "     UVADJbyS    %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_UVADJ].u32Viraddr)+2)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_XNR].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     XNR         %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_XNR].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YCUVM].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     YCUVM       %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YCUVM].u32Viraddr)+0)) ? "Bypass" :"ON");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_COLORTRAN].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     COLOR TRAN  %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_COLORTRAN].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GAMMA].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     YUV GAMMA   %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GAMMA].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += DrvSclOsScnprintf(str, end - str, "---------------COLOR ENGINE--------------------\n");
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YUVTORGB].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     Y2R         %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_YUVTORGB].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GM10TO12].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     GM10to12    %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GM10TO12].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_CCM].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     CCM         %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_CCM].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_HSV].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     HSV         %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_HSV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GM12TO10].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     GM12to10    %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_GM12TO10].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    if(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_RGBTOYUV].u32Viraddr)
    {
        str += DrvSclOsScnprintf(str, end - str, "     R2Y         %s\n",
            (*((u8 *)(gstGlobalMVipSet->gstSupCfg.staip[E_MDRV_SCLVIP_AIP_RGBTOYUV].u32Viraddr)+0)) ? "ON" :"OFF");
    }
    str += DrvSclOsScnprintf(str, end - str, "---------------SRAM--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-1-----------Y GAMMA--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-2-----------U GAMMA--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-3-----------V GAMMA--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-4-----------R GAMMAA2C--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-5-----------G GAMMAA2C--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-6-----------B GAMMAA2C--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-7-----------R GAMMAC2A--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-8-----------G GAMMAC2A--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-9-----------B GAMMAC2A--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "-A-----------WDR--------------------\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL VIP STATUS======================\n");
    return (str - buf);
}
#endif
#endif
