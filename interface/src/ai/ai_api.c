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

///////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   ai_api.c
/// @brief ai module api
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __KERNEL__
#include <stdlib.h>
#endif

#include "mi_syscall.h"
#include "mi_print.h"
#include "mi_sys.h"
#include "mi_common.h"
#include "mi_ai.h"
#include "mi_aio_internal.h"
#include "ai_ioctl.h"

#if USE_CAM_OS
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#endif

//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

MI_MODULE_DEFINE(ai)

//-------------------------------------------------------------------------------------------------
//  Local Macros
//-------------------------------------------------------------------------------------------------
#define MI_AI_USR_CHECK_DEV(AiDevId)  \
    if(AiDevId < 0 || AiDevId >= MI_AI_DEV_NUM_MAX) \
    {   \
        DBG_ERR("AiDevId is invalid! AiDevId = %u.\n", AiDevId);   \
        return MI_AI_ERR_INVALID_DEVID;   \
    }

#define MI_AI_USR_CHECK_CHN(Aichn)  \
    if(Aichn < 0 || Aichn >= MI_AI_CHAN_NUM_MAX) \
    {   \
        DBG_ERR("Aichn is invalid! Aichn = %u.\n", Aichn);   \
        return MI_AI_ERR_INVALID_CHNID;   \
    }

#define MI_AI_USR_CHECK_POINTER(pPtr)  \
    if(NULL == pPtr)  \
    {   \
        DBG_ERR("Invalid parameter! NULL pointer.\n");   \
        return MI_AI_ERR_NULL_PTR;   \
    }

#define MI_AI_USR_CHECK_SAMPLERATE(eSamppleRate)    \
    if( (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_8000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_16000) &&\
        (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_32000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_48000) ) \
    { \
        DBG_ERR("Sample Rate is illegal = %u.\n", eSamppleRate);   \
        return MI_AI_ERR_ILLEGAL_PARAM;   \
    }

#if USE_CAM_OS
    #define MI_AO_Malloc CamOsMemAlloc
    #define MI_AO_Free  CamOsMemRelease
#else
    #define MI_AO_Malloc malloc
    #define MI_AO_Free free
#endif


#define MI_AI_APC_POINTER 128
#define MI_AI_APC_EQ_BAND_NUM 10

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------
typedef struct _MI_AI_ChanInfoUsr_s
{
    MI_BOOL                 bChanEnable;
    MI_BOOL                 bPortEnable;
    MI_S32                  s32ChnId;
    MI_S32                  s32OutputPortId;
    MI_BOOL                 bResampleEnable;
    MI_AUDIO_SampleRate_e   eOutResampleRate;
    MI_BOOL                 bVqeEnable;
    MI_BOOL                 bVqeAttrSet;
    MI_S32                  s32VolumeDb;
    MI_AI_VqeConfig_t       stAiVqeConfig;
    MI_AUDIO_SaveFileInfo_t stSaveFileInfo;
    MI_SYS_BUF_HANDLE       hBufHandle;
    //Resample(SRC)
    SRC_HANDLE              hSrcHandle;
    void*                   pSrcWorkingBuf;
    MI_U8*                  pu8SrcOutBuf;
    // VQE
    APC_HANDLE              hApcHandle;
    void*                   pApcWorkingBuf;
}_MI_AI_ChanInfoUsr_t;

typedef struct _MI_AI_DevInfoUsr_s
{
    MI_BOOL                 bDevEnable;
    MI_BOOL                 bDevAttrSet;
    MI_AUDIO_DEV            AiDevId;
    MI_AUDIO_Attr_t         stDevAttr;
    _MI_AI_ChanInfoUsr_t    astChanInfo[MI_AI_CHAN_NUM_MAX];
    MI_U64                  u64PhyBufAddr;          // for DMA HW address
}_MI_AI_DevInfoUsr_t;

//-------------------------------------------------------------------------------------------------
//  Local Enum
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
// initial & Todo: need to protect variable
static _MI_AI_DevInfoUsr_t _gastAiDevInfoUsr[MI_AI_DEV_NUM_MAX]={
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .astChanInfo[0] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[1] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[2] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[3] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[4] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[5] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[6] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[7] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[8] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[9] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[10] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[11] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[12] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[13] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[14] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
        .astChanInfo[15] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .hSrcHandle = NULL,
            .hApcHandle = NULL,
        },
    },
};


//-------------------------------------------------------------------------------------------------
// Local  Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  local function  prototypes
//-------------------------------------------------------------------------------------------------
static MI_S32 _MI_AI_ReSmpInit(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_SampleRate_e eOutSampleRate)
{
    MI_S32 s32Ret = MI_SUCCESS;

     SRCStructProcess stSrcStruct;
     MI_AUDIO_SampleRate_e eAttrSampleRate;
     MI_U16 u16ChanlNum;

    eAttrSampleRate = _gastAiDevInfoUsr[AiDevId].stDevAttr.eSamplerate;
    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, _gastAiDevInfoUsr[AiDevId].stDevAttr.eSoundmode);

    stSrcStruct.WaveIn_srate = (SrcInSrate) (eAttrSampleRate/1000);// get_sample_rate_enumeration(input_wave->wave_header.sample_per_sec);
    stSrcStruct.channel = u16ChanlNum;
    stSrcStruct.mode = _MI_AUDIO_GetSrcConvertMode(eAttrSampleRate, eOutSampleRate);
    stSrcStruct.point_number = _gastAiDevInfoUsr[AiDevId].stDevAttr.u32PtNumPerFrm;
    /* SRC init */
#ifndef __KERNEL__
    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pSrcWorkingBuf = MI_AO_Malloc(IaaSrc_GetBufferSize(stSrcStruct.mode));
    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hSrcHandle = IaaSrc_Init(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pSrcWorkingBuf, &stSrcStruct);
#endif

    // error handle
    if(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hSrcHandle == NULL)
    {
        //s32Ret = MI_ERR_AI_VQE_ERR; //add ReSmp error ?
        MI_PRINT("_MI_AI_ReSmpInit Fail !!!!!\n");
    }

    return s32Ret;
}

#ifndef __KERNEL__

static MI_S32 _MI_AI_SetEqGainDb(EqGainDb_t *pstEqGainInfo, MI_S16  *pS16Buff, MI_U32 u32Size)
{
    MI_S16 *ps16Tmp = (MI_S16 *)pstEqGainInfo;
    MI_S32 s32i = 0;

    MI_AI_USR_CHECK_POINTER(pstEqGainInfo);
    MI_AI_USR_CHECK_POINTER(pS16Buff);

    for (s32i = 0; s32i < u32Size; s32i++)
    {
        pS16Buff[s32i] = *ps16Tmp;
        ps16Tmp ++;
    }

    return MI_SUCCESS;
}

#endif


static MI_S32 _MI_AI_VqeInit(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

#ifndef __KERNEL__
    AudioProcessInit stApstruct;
    void* pApcWorkingBuf;
    APC_HANDLE hApcHandle;
    MI_U16 u16ChanlNum;

    MI_AI_VqeConfig_t stAiVqeConfig;

    AudioAnrConfig stAnrInfo;
    AudioEqConfig  stEqInfo;
    AudioHpfConfig stHpfInfo;

    AudioVadConfig stVadInfo;
    AudioDereverbConfig stDereverbInfo;
    AudioAgcConfig stAgcInfo;

    /* APC init parameter setting */
    stApstruct.point_number = MI_AI_APC_POINTER;
    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, _gastAiDevInfoUsr[AiDevId].stDevAttr.eSoundmode);
    stApstruct.channel = u16ChanlNum;
    MI_AUDIO_VQE_SAMPLERATE_TRANS_TYPE(_gastAiDevInfoUsr[AiDevId].stDevAttr.eSamplerate, stApstruct.sample_rate);

    /* APC init */
    pApcWorkingBuf = MI_AO_Malloc(IaaApc_GetBufferSize());
    if (NULL == pApcWorkingBuf)
    {
        DBG_ERR("Malloc IaaApc_GetBuffer failed\n");
        return MI_AI_ERR_NOBUF;
    }

    hApcHandle  = IaaApc_Init((char* const)pApcWorkingBuf, &stApstruct);
    if(hApcHandle == NULL)
    {
        MI_PRINT("IaaApc_Init FAIL !!!!!!!!!!!!!!!!!!!");
        return MI_AI_ERR_VQE_ERR;
    }
    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pApcWorkingBuf = pApcWorkingBuf;
    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hApcHandle = hApcHandle;

    /* VQE Setting */
    stAiVqeConfig = _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].stAiVqeConfig;

       //set Anr
    stAnrInfo.anr_enable = (MI_U32)stAiVqeConfig.bAnrOpen;
    stAnrInfo.user_mode = (MI_U32)stAiVqeConfig.stAnrCfg.bUsrMode;
     MI_AUDIO_VQE_NR_SPEED_TRANS_TYPE(stAiVqeConfig.stAnrCfg.eNrSpeed, stAnrInfo.anr_converge_speed);
    stAnrInfo.anr_intensity = stAiVqeConfig.stAnrCfg.u32NrIntensity;
    stAnrInfo.anr_smooth_level = stAiVqeConfig.stAnrCfg.u32NrSmoothLevel;

    //set Eq
    stEqInfo.eq_enable = (MI_U32)stAiVqeConfig.bEqOpen;
    stEqInfo.user_mode = (MI_U32)stAiVqeConfig.stEqCfg.bUsrMode;
    s32Ret = _MI_AI_SetEqGainDb(&stAiVqeConfig.stEqCfg.stEqGain, stEqInfo.eq_gain_db, MI_AI_APC_EQ_BAND_NUM);

    //set Hpf
    stHpfInfo.hpf_enable = (MI_U32)stAiVqeConfig.bHpfOpen;
    stHpfInfo.user_mode = (MI_U32)stAiVqeConfig.stHpfCfg.bUsrMode;
    MI_AUDIO_VQE_HPF_TRANS_TYPE(stAiVqeConfig.stHpfCfg.eHpfFreq, stHpfInfo.cutoff_frequency);

    //set Vad
    stVadInfo.vad_enable = 0;
    stVadInfo.user_mode = 1;
    stVadInfo.vad_threshold = -10;

    //set De-reverberation
    stDereverbInfo.dereverb_enable = 0;

    //set Agc
    stAgcInfo.agc_enable = (MI_U32)stAiVqeConfig.bAgcOpen;
    stAgcInfo.user_mode = (MI_U32)stAiVqeConfig.stAgcCfg.bUsrMode;
    stAgcInfo.attack_time = stAiVqeConfig.stAgcCfg.u32AttackTime;
    stAgcInfo.release_time = stAiVqeConfig.stAgcCfg.u32ReleaseTime;
    stAgcInfo.compression_ratio = stAiVqeConfig.stAgcCfg.u32CompressionRatio;
    stAgcInfo.drop_gain_max = stAiVqeConfig.stAgcCfg.u32DropGainMax;
    stAgcInfo.gain_info.gain_init = stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit;
    stAgcInfo.gain_info.gain_max = stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax;
    stAgcInfo.gain_info.gain_min = stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin;
    stAgcInfo.noise_gate_attenuation_db = stAiVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb;
    stAgcInfo.noise_gate_db = stAiVqeConfig.stAgcCfg.s32NoiseGateDb;
    stAgcInfo.target_level_db = stAiVqeConfig.stAgcCfg.s32TargetLevelDb;

                                                                       // Vad   dereverb is not set
    s32Ret = IaaApc_Config(hApcHandle, &stAnrInfo, &stEqInfo, &stHpfInfo, NULL, NULL, &stAgcInfo);
    if (0 != s32Ret)
    {
        MI_PRINT("IaaPac_config FAIL !!!!!!!!!!!!!!!!!!!\n");
        return MI_AI_ERR_VQE_ERR;
    }

#endif

    return s32Ret;
}

//-------------------------------------------------------------------------------------------------
//  global function  prototypes
//-------------------------------------------------------------------------------------------------
MI_S32 MI_AI_SetPubAttr(MI_AUDIO_DEV AiDevId, MI_AUDIO_Attr_t *pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_SetPubAttr_t stSetPubAttr;

    memset(&stSetPubAttr, 0, sizeof(stSetPubAttr));
    stSetPubAttr.AiDevId = AiDevId;
    memcpy(&stSetPubAttr.stAttr, pstAttr, sizeof(MI_AUDIO_Attr_t));
    s32Ret = MI_SYSCALL(MI_AI_SET_PUB_ATTR, &stSetPubAttr);

    if (s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].bDevAttrSet = TRUE;
        memcpy(&_gastAiDevInfoUsr[AiDevId].stDevAttr, pstAttr, sizeof(MI_AUDIO_Attr_t));
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_SetPubAttr);

MI_S32 MI_AI_GetPubAttr(MI_AUDIO_DEV AiDevId, MI_AUDIO_Attr_t*pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_GetPubAttr_t stGetPubAttr;

    memset(&stGetPubAttr, 0, sizeof(stGetPubAttr));
    stGetPubAttr.AiDevId = AiDevId;
    s32Ret = MI_SYSCALL(MI_AI_GET_PUB_ATTR, &stGetPubAttr);

    if (s32Ret == MI_SUCCESS)
    {
        memcpy(pstAttr, &stGetPubAttr.stAttr, sizeof(MI_AUDIO_Attr_t));
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_GetPubAttr);

MI_S32 MI_AI_Enable(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AI_ENABLE, &AiDevId);

    if (s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].bDevEnable = TRUE;
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_Enable);

MI_S32 MI_AI_Disable(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AI_DISABLE, &AiDevId);

    if (s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].bDevEnable = FALSE;
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_Disable);

MI_S32 MI_AI_EnableChn(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_EnableChn_t stEnableChn;

    memset(&stEnableChn, 0, sizeof(stEnableChn));
    stEnableChn.AiDevId = AiDevId;
    stEnableChn.AiChn = AiChn;
    s32Ret = MI_SYSCALL(MI_AI_ENABLE_CHN, &stEnableChn);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable = TRUE;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].s32ChnId = AiChn;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bPortEnable = TRUE;
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_EnableChn);

MI_S32 MI_AI_DisableChn(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_DisableChn_t stDisableChn;

    memset(&stDisableChn, 0, sizeof(stDisableChn));
    stDisableChn.AiDevId = AiDevId;
    stDisableChn.AiChn = AiChn;
    s32Ret = MI_SYSCALL(MI_AI_DISABLE_CHN, &stDisableChn);

    if (s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable = FALSE;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bPortEnable = FALSE;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable = FALSE;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bResampleEnable = FALSE;
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_DisableChn);

MI_S32 MI_AI_GetFrame(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_Frame_t *pstFrm, MI_AUDIO_AecFrame_t *pstAecFrm , MI_S32 s32MilliSec)
{
    MI_S32 s32Ret = MI_SUCCESS;

    //get output port buffer
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 u32ReadBufSize;
    MI_U32 u32BitwidthByte;
    MI_SYS_ChnPort_t stAiChnOutputPort0;

    // Resample & VQE
    MI_U8* pu8SrcOutBuf;
    SRC_HANDLE hSrcHandle;
    APC_HANDLE hApcHandle;
    MI_S32 s32SrcOutSample;
    MI_S32 s32InputPoint;

    MI_U32 u32VqeWrPtr;
    MI_U32 u32VqeSampleUnitByte;
    MI_U32 u32VqeOutBytes;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);
    MI_AI_USR_CHECK_POINTER(pstFrm);
    //MI_AI_USR_CHECK_POINTER(pstAecFrm);

    if(TRUE != _gastAiDevInfoUsr[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    stAiChnOutputPort0.eModId = E_MI_MODULE_ID_AI;
    stAiChnOutputPort0.u32DevId = AiDevId;
    stAiChnOutputPort0.u32ChnId = AiChn;
    stAiChnOutputPort0.u32PortId = 0;

   //MI_SYS_SetChnOutputPortDepth(&stAiChnOutputPort0,5,6); //remove to AP ???

    // get data of sending by AI output port
    if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stAiChnOutputPort0, &stBufInfo, &hHandle))
    {
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hBufHandle = hHandle;
        u32ReadBufSize = stBufInfo.stRawData.u32BufSize;

        pstFrm->eBitwidth = _gastAiDevInfoUsr[AiDevId].stDevAttr.eBitwidth;
        pstFrm->eSoundmode = _gastAiDevInfoUsr[AiDevId].stDevAttr.eSoundmode;
        pstFrm->u64TimeStamp = stBufInfo.u64Pts;
        MI_AUDIO_USR_TRANS_BWIDTH_TO_BYTE(u32BitwidthByte, _gastAiDevInfoUsr[AiDevId].stDevAttr.eBitwidth);

        if(_gastAiDevInfoUsr[AiDevId].stDevAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
        {
            //memcpy(pstFrm->apVirAddr[0], stBufInfo.stRawData.pVirAddr, u32ReadBufSize);
            // if enable Vqe
            hApcHandle = _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hApcHandle;

            if(TRUE ==  _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable)
            {
                u32VqeSampleUnitByte = MI_AUDIO_VQE_SAMPLES_UNIT * u32BitwidthByte;
                u32VqeOutBytes = stBufInfo.stRawData.u32BufSize;
                u32VqeWrPtr = 0;
#ifndef __KERNEL__
                while(u32VqeOutBytes)
                {
                    IaaApc_Run(hApcHandle, (MI_S16*)(stBufInfo.stRawData.pVirAddr + u32VqeWrPtr));
                    u32VqeOutBytes -= u32VqeSampleUnitByte;
                    u32VqeWrPtr += u32VqeSampleUnitByte;
                }
#endif
            }

            if(TRUE == _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bResampleEnable)
            {

                // allocate Resample output buffer
#ifndef __KERNEL__
                if(NULL == _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf)
                    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf = MI_AO_Malloc(u32ReadBufSize * 6); // 6 = 48k/8k
#endif
                pu8SrcOutBuf = _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf;

                hSrcHandle = _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hSrcHandle;
                /* Run SRC process */
                s32InputPoint =  u32ReadBufSize /u32BitwidthByte; // u32ReadBufSize / 2 ;
                s32SrcOutSample = 0;

#ifndef __KERNEL__
                s32SrcOutSample = IaaSrc_Run(hSrcHandle, (MI_S16*)stBufInfo.stRawData.pVirAddr, (MI_S16*)pu8SrcOutBuf, s32InputPoint);

#endif
                pstFrm->u32Len = s32SrcOutSample * u32BitwidthByte; // s32SrcOutSample * 2;
                pstFrm->apVirAddr[0] = pu8SrcOutBuf;
                pstFrm->apVirAddr[1] = NULL;
            }
            else // FALSE == _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bResampleEnable
            {
                pstFrm->u32Len = u32ReadBufSize;
                pstFrm->apVirAddr[0] = stBufInfo.stRawData.pVirAddr;
                pstFrm->apVirAddr[1] = NULL;
            }

        }
        else // need to process stereo channel ???
        {
            pstFrm->u32Len = u32ReadBufSize >> 1;
            //_MI_AI_UsrDeinterleave(pstFrm, stBufInfo, u32ReadBufSize);
        }

        //MI_SYS_ChnOutputPortPutBuf(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hBufHandle);
    }
    else
    {
        DBG_INFO("[Usr]Get output port buffer of Channel %d fail \n", AiChn);
        return MI_AI_ERR_BUF_EMPTY;
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_GetFrame);

MI_S32 MI_AI_ReleaseFrame(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_Frame_t *pstFrm, MI_AUDIO_AecFrame_t *pstAecFrm)
{
    MI_S32 s32Ret = MI_SUCCESS;

    if(TRUE != _gastAiDevInfoUsr[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    MI_SYS_ChnOutputPortPutBuf(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hBufHandle);
    pstFrm->apVirAddr[0] = NULL;
    pstFrm->apVirAddr[1] = NULL;
    pstFrm->u32Len = 0;

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_ReleaseFrame);

MI_S32 MI_AI_SetChnParam(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AI_ChnParam_t *pstChnParam)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_SetChnParam_t stSetChnParam;

    memset(&stSetChnParam, 0, sizeof(stSetChnParam));
    stSetChnParam.AiDevId = AiDevId;
    stSetChnParam.AiChn = AiChn;
    memcpy(&stSetChnParam.stChnParam, pstChnParam, sizeof(MI_AI_ChnParam_t));

    s32Ret = MI_SYSCALL(MI_AI_SET_CHN_PARAM, &stSetChnParam);
    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_SetChnParam);

MI_S32 MI_AI_GetChnParam(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AI_ChnParam_t *pstChnParam)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_GetChnParam_t stGetChnParam;

    memset(&stGetChnParam, 0, sizeof(stGetChnParam));
    stGetChnParam.AiDevId = AiDevId;
    stGetChnParam.AiChn = AiChn;

    s32Ret = MI_SYSCALL(MI_AI_GET_CHN_PARAM, &stGetChnParam);
    if (s32Ret == MI_SUCCESS)
    {
        memcpy(pstChnParam, &stGetChnParam.stChnParam, sizeof(MI_AI_ChnParam_t));
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_GetChnParam);

MI_S32 MI_AI_EnableReSmp(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_SampleRate_e eOutSampleRate)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_EnableReSmp_t stEnableResmp;

    ///check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);
    MI_AI_USR_CHECK_SAMPLERATE(eOutSampleRate);

    if(TRUE != _gastAiDevInfoUsr[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;


    // put resample status to kernel mode for DebugFs
    memset(&stEnableResmp, 0, sizeof(stEnableResmp));
    stEnableResmp.AiDevId = AiDevId;
    stEnableResmp.AiChn= AiChn;
    stEnableResmp.eOutSampleRate= eOutSampleRate;
    s32Ret = MI_SYSCALL(MI_AI_ENABLE_RESMP, &stEnableResmp);

    if(s32Ret == MI_SUCCESS)
    {
        // Enable channel of AI device resample
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bResampleEnable = TRUE;
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].eOutResampleRate = eOutSampleRate;

        /* SRC parameter setting */
        s32Ret = _MI_AI_ReSmpInit(AiDevId, AiChn, eOutSampleRate);
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_EnableReSmp);

MI_S32 MI_AI_DisableReSmp(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_DisableReSmp_t stDisableResmp;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);

    //put resample status to kernel mode for DebugFs
    memset(&stDisableResmp, 0, sizeof(stDisableResmp));
    stDisableResmp.AiDevId = AiDevId;
    stDisableResmp.AiChn = AiChn;
    s32Ret = MI_SYSCALL(MI_AI_DISABLE_RESMP, &stDisableResmp);

    if(s32Ret == MI_SUCCESS)
    {
        // Disable channel of AI device resample
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bResampleEnable = FALSE;

#ifndef __KERNEL__
        if(NULL != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pSrcWorkingBuf)
        {
            MI_AO_Free(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pSrcWorkingBuf);
            _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pSrcWorkingBuf = NULL;
        }

        if(NULL != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf)
        {
            MI_AO_Free(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf);
            _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pu8SrcOutBuf = NULL;
        }
        IaaSrc_Release( _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hSrcHandle);
#endif
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_DisableReSmp);

MI_S32 MI_AI_SetVqeAttr(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AI_VqeConfig_t *pstVqeConfig)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_SetVqeAttr_t stSetVqeAttr;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);
    MI_AI_USR_CHECK_POINTER(pstVqeConfig);
    //MI_AO_USR_CHECK_DEV(AoDevId); //ToDo
    //MI_AO_USR_CHECK_CHN(AoChn); //ToDo

    // check if Vqe of AI device channel is disable ?
    if( FALSE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return MI_AI_ERR_NOT_PERM;

    // check if AI device channel is enable ?
    if( TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    // check if Vqe Configure legal ?

    // put VQE status to kernel mode for DebugFs
    memset(&stSetVqeAttr, 0, sizeof(stSetVqeAttr));
    stSetVqeAttr.AiDevId = AiDevId;
    stSetVqeAttr.AiChn= AiChn;
    memcpy(&stSetVqeAttr.stVqeConfig, pstVqeConfig, sizeof(MI_AI_VqeConfig_t));
    s32Ret = MI_SYSCALL(MI_AI_SET_VQE_ATTR, &stSetVqeAttr);

    if(s32Ret == MI_SUCCESS)
    {
        // save Vqe configure of AI device channel
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeAttrSet = TRUE;
        memcpy(&_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].stAiVqeConfig, pstVqeConfig, sizeof(MI_AI_VqeConfig_t));
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_SetVqeAttr);

MI_S32 MI_AI_GetVqeAttr(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn,  MI_AI_VqeConfig_t *pstVqeConfig)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);
    MI_AI_USR_CHECK_POINTER(pstVqeConfig);

    // check if Vqe attribute of AO device channel is set ?
    if(TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeAttrSet)
        return MI_AI_ERR_NOT_PERM;

    // load Vqe config
    memcpy(pstVqeConfig, &_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].stAiVqeConfig, sizeof(MI_AI_VqeConfig_t));

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_GetVqeAttr);

MI_S32 MI_AI_EnableVqe(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_EnableVqe_t stEnableVqe;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);

    //
    if( TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeAttrSet)
        return MI_AI_ERR_NOT_PERM;

    if( TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE == _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return s32Ret;

     // put VQE status to kernel mode for DebugFs
    memset(&stEnableVqe, 0, sizeof(stEnableVqe));
    stEnableVqe.AiDevId = AiDevId;
    stEnableVqe.AiChn= AiChn;
    s32Ret = MI_SYSCALL(MI_AI_ENABLE_VQE, &stEnableVqe);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable = TRUE;

        // need to call VQE init here
        s32Ret = _MI_AI_VqeInit(AiDevId, AiChn);
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_EnableVqe);

MI_S32 MI_AI_DisableVqe(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AI_DisableVqe_t stDisableVqe;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);

    if(FALSE == _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return s32Ret;

    // put vqe status to kernel mode for DebugFs
    memset(&stDisableVqe, 0, sizeof(stDisableVqe));
    stDisableVqe.AiDevId = AiDevId;
    stDisableVqe.AiChn = AiChn;
    s32Ret = MI_SYSCALL(MI_AI_DISABLE_VQE, &stDisableVqe);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable = FALSE;
        //_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeAttrSet = FALSE; // ???

#ifndef __KERNEL__
        if(NULL != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pApcWorkingBuf)
        {
            MI_AO_Free( _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pApcWorkingBuf);
            _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].pApcWorkingBuf = NULL;
        }

        IaaApc_Free(_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].hApcHandle);
#endif
    }

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_DisableVqe);

MI_S32 MI_AI_ClrPubAttr(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AI_CLR_PUB_ATTR, &AiDevId);

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_ClrPubAttr);

MI_S32 MI_AI_SaveFile(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_SaveFileInfo_t *pstSaveFileInfo)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);
    MI_AI_USR_CHECK_POINTER(pstSaveFileInfo);

    // check if AI Vqe enable ?
    if(TRUE != _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return MI_AI_ERR_NOT_PERM;

    memcpy(&_gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].stSaveFileInfo, pstSaveFileInfo, sizeof(MI_AUDIO_SaveFileInfo_t));

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_SaveFile);

MI_S32 MI_AI_SetVqeVolume(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_S32 s32VolumeDb)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_USR_CHECK_DEV(AiDevId);
    MI_AI_USR_CHECK_CHN(AiChn);

    // check s32VolumeDb range ???

    //
    _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].s32VolumeDb = s32VolumeDb;

    return s32Ret;
}
EXPORT_SYMBOL(MI_AI_SetVqeVolume);

MI_S32 MI_AI_GetVqeVolume(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_S32 *ps32VolumeDb)
{
    MI_S32 s32Ret = MI_SUCCESS;

     // check input parameter
     MI_AI_USR_CHECK_DEV(AiDevId);
     MI_AI_USR_CHECK_CHN(AiChn);
     MI_AI_USR_CHECK_POINTER(ps32VolumeDb);

     //
     *ps32VolumeDb = _gastAiDevInfoUsr[AiDevId].astChanInfo[AiChn].s32VolumeDb;

     return s32Ret;
}
EXPORT_SYMBOL(MI_AI_GetVqeVolume);
