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
/// @file   ao_api.c
/// @brief vdec module api
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __KERNEL__
#include <stdlib.h>
#endif

#include "mi_syscall.h"
#include "mi_print.h"
#include "mi_sys.h"
#include "mi_common.h"
#include "mi_ao.h"
#include "mi_aio_internal.h"
#include "ao_ioctl.h"

#if USE_CAM_OS
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#endif
//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------

MI_MODULE_DEFINE(ao)

//-------------------------------------------------------------------------------------------------
//  Local Macros
//-------------------------------------------------------------------------------------------------
#define MI_AO_USR_CHECK_DEV(AoDevId)  \
    if(AoDevId < 0 || AoDevId >= MI_AO_DEV_NUM_MAX) \
    {   \
        DBG_ERR("AoDevId is invalid! AoDevId = %u.\n", AoDevId);   \
        return MI_AO_ERR_INVALID_DEVID;   \
    }

#define MI_AO_USR_CHECK_CHN(Aochn)  \
    if(Aochn < 0 || Aochn >= MI_AO_CHAN_NUM_MAX) \
    {   \
        DBG_ERR("Aochn is invalid! Aochn = %u.\n", Aochn);   \
        return MI_AO_ERR_INVALID_CHNID;   \
    }

#define MI_AO_USR_CHECK_POINTER(pPtr)  \
    if(NULL == pPtr)  \
    {   \
        DBG_ERR("Invalid parameter! NULL pointer.\n");   \
        return MI_AO_ERR_NULL_PTR;   \
    }

#define MI_AO_USR_CHECK_SAMPLERATE(eSamppleRate)    \
    if( (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_8000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_16000) &&\
        (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_32000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_48000) ) \
    { \
        DBG_ERR("Sample Rate is illegal = %u.\n", eSamppleRate);   \
        return MI_AO_ERR_ILLEGAL_PARAM;   \
    }

#if USE_CAM_OS
    #define MI_AO_Malloc CamOsMemAlloc
    #define MI_AO_Free  CamOsMemRelease
#else
    #define MI_AO_Malloc malloc
    #define MI_AO_Free free
#endif

#define MI_AO_APC_POINTER 128
#define MI_AO_APC_EQ_BAND_NUM 10

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------
typedef struct _MI_AO_ChanInfoUsr_s
{
    MI_BOOL                 bChanEnable;
    MI_BOOL                 bPortEnable;
    MI_S32                  s32ChnId;
    MI_S32                  s32InputPortId;
    MI_S32                  s32OutputPortId;
    MI_BOOL                 bChanPause;
    MI_BOOL                 bResampleEnable;
    MI_AUDIO_SampleRate_e   eInResampleRate;
    MI_BOOL                 bVqeEnable;
    MI_BOOL                 bVqeAttrSet;
    MI_AO_VqeConfig_t       stAoVqeConfig;
    //Resample(SRC)
    SRC_HANDLE           hSrcHandle;
    MI_U8*                   pu8SrcInBuf;
    MI_U8*                   pu8SrcOutBuf;
    void*                      pSrcWorkingBuf;
    MI_S32                   s32SrcBufWrPtr;
    MI_S32                   s32SrcBufRdPtr;
    // VQE
    APC_HANDLE           hApcHandle;
    void*                      pApcWorkingBuf;

}_MI_AO_ChanInfoUsr_t;

typedef struct _MI_AO_DevInfoUsr_s
{
    MI_BOOL                 bDevEnable;
    MI_BOOL                 bDevAttrSet;
    MI_AUDIO_DEV            AoDevId;
    MI_AUDIO_Attr_t         stDevAttr;
    _MI_AO_ChanInfoUsr_t     astChanInfo[MI_AO_CHAN_NUM_MAX];
    MI_U64                  u64PhyBufAddr;          // for DMA HW address
}_MI_AO_DevInfoUsr_t;



//-------------------------------------------------------------------------------------------------
//  Local Enum
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------
// initial & Todo: need to protect variable
static _MI_AO_DevInfoUsr_t _gastAoDevInfoUsr[MI_AO_DEV_NUM_MAX]={
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .AoDevId = 0,
        .astChanInfo[0] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .pu8SrcOutBuf = NULL,
            .hSrcHandle = NULL,
            .s32SrcBufWrPtr = 0,
            .s32SrcBufRdPtr = 0,
        },
     },
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .AoDevId = 1,
        .astChanInfo[0] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .pu8SrcOutBuf = NULL,
            .hSrcHandle = NULL,
            .s32SrcBufWrPtr = 0,
            .s32SrcBufRdPtr = 0,
        },
    },
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .AoDevId = 2,
        .astChanInfo[0] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
            .bVqeEnable = FALSE,
            .bVqeAttrSet =FALSE,
            .pu8SrcOutBuf = NULL,
            .hSrcHandle = NULL,
            .s32SrcBufWrPtr = 0,
            .s32SrcBufRdPtr = 0,
        },
    },
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .AoDevId = 3,
        .astChanInfo[0] = {
        .bChanEnable = FALSE,
        .bPortEnable = FALSE,
        .bResampleEnable = FALSE,
        .bVqeEnable = FALSE,
        .bVqeAttrSet =FALSE,
        .pu8SrcOutBuf = NULL,
        .hSrcHandle = NULL,
        .s32SrcBufWrPtr = 0,
        .s32SrcBufRdPtr = 0,
        },
    },
};

//-------------------------------------------------------------------------------------------------
// Local Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  local function  prototypes
//-------------------------------------------------------------------------------------------------
static MI_S32 _MI_AO_InterleaveData(MI_U8* pu8LRBuf, MI_U8* pu8LBuf, MI_U8* pu8RBuf, MI_U32 u32Size)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_S32 s32Iidx = 0;
    MI_S32 s32Jidx = 0;
    MI_S32 s32LeftIdx = 0;
    MI_S32 s32RightIdx = 0;
    MI_U32 u32MaxSize = u32Size / 2;

    for(s32Iidx=0; s32Iidx<u32MaxSize; s32Iidx++)
    {
        pu8LRBuf[s32Jidx++] = pu8LBuf[s32LeftIdx++];
        pu8LRBuf[s32Jidx++] = pu8LBuf[s32LeftIdx++];
        pu8LRBuf[s32Jidx++] = pu8RBuf[s32RightIdx++];
        pu8LRBuf[s32Jidx++] = pu8RBuf[s32RightIdx++];
    }

    return s32Ret;

}

static MI_S32 _MI_AO_ReSmpInit(MI_AUDIO_DEV AoDevId, MI_AI_CHN AoChn, MI_AUDIO_SampleRate_e eInSampleRate)
{
    MI_S32 s32Ret = MI_SUCCESS;

    SRCStructProcess stSrcStruct;
    MI_AUDIO_SampleRate_e eAttrSampleRate;
    MI_U16 u16ChanlNum;
    MI_U32 u32NumPerFrm;
    MI_U32 u32BitWidthByte;


     /* SRC parameter setting */
     _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr = 0;
     _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr = 0;
    eAttrSampleRate = _gastAoDevInfoUsr[AoDevId].stDevAttr.eSamplerate;
    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, _gastAoDevInfoUsr[AoDevId].stDevAttr.eSoundmode);
    MI_AUDIO_USR_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, _gastAoDevInfoUsr[AoDevId].stDevAttr.eBitwidth);
    u32NumPerFrm = _gastAoDevInfoUsr[AoDevId].stDevAttr.u32PtNumPerFrm;

    stSrcStruct.WaveIn_srate = (SrcInSrate) (eInSampleRate/1000);// get_sample_rate_enumeration(input_wave->wave_header.sample_per_sec);
    stSrcStruct.channel = u16ChanlNum;
    stSrcStruct.mode = _MI_AUDIO_GetSrcConvertMode(eInSampleRate, eAttrSampleRate);
    stSrcStruct.point_number = u32NumPerFrm;

    /* SRC init */
#ifndef __KERNEL__
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pSrcWorkingBuf = MI_AO_Malloc(IaaSrc_GetBufferSize(stSrcStruct.mode));
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hSrcHandle = IaaSrc_Init(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pSrcWorkingBuf, &stSrcStruct);

    // allocate Resample temp buffer
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf = MI_AO_Malloc(u32NumPerFrm * u32BitWidthByte * u16ChanlNum * 6); // 6 = 48k/8k
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcInBuf = MI_AO_Malloc(u32NumPerFrm * u32BitWidthByte * u16ChanlNum);

#endif

    if(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hSrcHandle == NULL)
    {
         //s32Ret = MI_ERR_AO_VQE_ERR; //add ReSmp error ?
        MI_PRINT("_MI_AO_ReSmpInit Fail !!!!! \n");
    }

    return s32Ret;
}

#ifndef __KERNEL__

static MI_S32 _MI_AO_SetEqGainDb(EqGainDb_t *pstEqGainInfo, MI_S16  *pS16Buff, MI_U32 u32Size)
{
    MI_S16 *ps16Tmp = (MI_S16 *)pstEqGainInfo;
    MI_S32 s32i = 0;

    MI_AO_USR_CHECK_POINTER(pstEqGainInfo);
    MI_AO_USR_CHECK_POINTER(pS16Buff);

    for (s32i = 0; s32i < u32Size; s32i++)
    {
        pS16Buff[s32i] = *ps16Tmp;
        ps16Tmp ++;
    }

    return MI_SUCCESS;
}

#endif

static MI_S32 _MI_AO_VqeInit(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

#ifndef __KERNEL__
    AudioProcessInit stApstruct;
    APC_HANDLE hApcHandle;
    MI_U16 u16ChanlNum;
    MI_AO_VqeConfig_t stAoVqeConfig;

    AudioAnrConfig stAnrInfo;
    AudioEqConfig  stEqInfo;
    AudioHpfConfig stHpfInfo;

    AudioVadConfig stVadInfo;
    AudioDereverbConfig stDereverbInfo;
    AudioAgcConfig stAgcInfo;

    //Apc init arguments
    memset(&stApstruct, 0, sizeof(AudioProcessInit));
    stApstruct.point_number = MI_AO_APC_POINTER;
    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, _gastAoDevInfoUsr[AoDevId].stDevAttr.eSoundmode);
    stApstruct.channel = u16ChanlNum;
    MI_AUDIO_VQE_SAMPLERATE_TRANS_TYPE(_gastAoDevInfoUsr[AoDevId].stDevAttr.eSamplerate, stApstruct.sample_rate);

    /* APC init */
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf = MI_AO_Malloc(IaaApc_GetBufferSize());
    if (NULL == _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf)
    {
        DBG_ERR("Malloc IaaApc_GetBuffer failed\n");
        return MI_AO_ERR_NOBUF;
    }
    else
    {
        MI_PRINT("Malloc IaaApc_GetBuffer ok\n");
    }
    hApcHandle  = IaaApc_Init((char* const)_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf, &stApstruct);
    if(hApcHandle == NULL)
    {
        MI_PRINT("IaaApc_Init FAIL !!!!!!!!!!!!!!!!!!!\n");
        return MI_AO_ERR_VQE_ERR;
    }
    _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hApcHandle = hApcHandle;

    stAoVqeConfig =  _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].stAoVqeConfig;

    //set Anr
    stAnrInfo.anr_enable = (MI_U32)stAoVqeConfig.bAnrOpen;
    stAnrInfo.user_mode = (MI_U32)stAoVqeConfig.stAnrCfg.bUsrMode;
     MI_AUDIO_VQE_NR_SPEED_TRANS_TYPE(stAoVqeConfig.stAnrCfg.eNrSpeed, stAnrInfo.anr_converge_speed);
    stAnrInfo.anr_intensity = stAoVqeConfig.stAnrCfg.u32NrIntensity;
    stAnrInfo.anr_smooth_level = stAoVqeConfig.stAnrCfg.u32NrSmoothLevel;

    //set Eq
    stEqInfo.eq_enable = (MI_U32)stAoVqeConfig.bEqOpen;
    stEqInfo.user_mode = (MI_U32)stAoVqeConfig.stEqCfg.bUsrMode;
    s32Ret = _MI_AO_SetEqGainDb(&stAoVqeConfig.stEqCfg.stEqGain, stEqInfo.eq_gain_db, MI_AO_APC_EQ_BAND_NUM);

    //set Hpf
    stHpfInfo.hpf_enable = (MI_U32)stAoVqeConfig.bHpfOpen;
    stHpfInfo.user_mode = (MI_U32)stAoVqeConfig.stHpfCfg.bUsrMode;
    MI_AUDIO_VQE_HPF_TRANS_TYPE(stAoVqeConfig.stHpfCfg.eHpfFreq, stHpfInfo.cutoff_frequency);

    //set Vad
    stVadInfo.vad_enable = 0;
    stVadInfo.user_mode = 1;
    stVadInfo.vad_threshold = -10;

    //set De-reverberation
    stDereverbInfo.dereverb_enable = 0;

    //set Agc
    stAgcInfo.agc_enable = (MI_U32)stAoVqeConfig.bAgcOpen;
    stAgcInfo.user_mode = (MI_U32)stAoVqeConfig.stAgcCfg.bUsrMode;
    stAgcInfo.attack_time = stAoVqeConfig.stAgcCfg.u32AttackTime;
    stAgcInfo.release_time = stAoVqeConfig.stAgcCfg.u32ReleaseTime;
    stAgcInfo.compression_ratio = stAoVqeConfig.stAgcCfg.u32CompressionRatio;
    stAgcInfo.drop_gain_max = stAoVqeConfig.stAgcCfg.u32DropGainMax;
    stAgcInfo.gain_info.gain_init = stAoVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit;
    stAgcInfo.gain_info.gain_max = stAoVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax;
    stAgcInfo.gain_info.gain_min = stAoVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin;
    stAgcInfo.noise_gate_attenuation_db = stAoVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb;
    stAgcInfo.noise_gate_db = stAoVqeConfig.stAgcCfg.s32NoiseGateDb;
    stAgcInfo.target_level_db = stAoVqeConfig.stAgcCfg.s32TargetLevelDb;

                                                                       // Vad   dereverb is not set
    s32Ret = IaaApc_Config(hApcHandle, &stAnrInfo, &stEqInfo, &stHpfInfo, NULL, NULL, &stAgcInfo);
    if (0 != s32Ret)
    {
        MI_PRINT("IaaPac_config FAIL !!!!!!!!!!!!!!!!!!!\n");
        return MI_AO_ERR_VQE_ERR;
    }
#endif

    return s32Ret;

}

static MI_S32 _MI_AO_ProcPreSrcOut(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AUDIO_Frame_t *pstData, MI_S32 s32MilliSec)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_U16 u16ChanlNum;
    MI_U32 u32BitwidthByte;
    MI_U32  u32LenPerChanl;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hBufHandle;

    MI_S32 s32SrcBufWrPtr;
    MI_S32 s32SrcBufRdPtr;
    // Vqe
    APC_HANDLE hApcHandle;
    MI_U32 u32VqeWrPtr;
    MI_U32 u32VqeSampleUnitByte;
    MI_U32 u32VqeOutBytes;

    stChnPort.eModId = E_MI_MODULE_ID_AO;
    stChnPort.u32DevId = AoDevId;
    stChnPort.u32ChnId = AoChn;
    stChnPort.u32PortId = 0;  //input port id

    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, pstData->eSoundmode);
    MI_AUDIO_USR_TRANS_BWIDTH_TO_BYTE(u32BitwidthByte, _gastAoDevInfoUsr[AoDevId].stDevAttr.eBitwidth);
    u32LenPerChanl = _gastAoDevInfoUsr[AoDevId].stDevAttr.u32PtNumPerFrm * u32BitwidthByte;//pstData->u32Len;


    s32SrcBufWrPtr = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr;
    s32SrcBufRdPtr = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr;

    while((s32SrcBufWrPtr - s32SrcBufRdPtr ) >= u32LenPerChanl *u16ChanlNum )
    {
        stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufConf.u64TargetPts = pstData->u64TimeStamp;
        stBufConf.stRawCfg.u32Size = u32LenPerChanl * u16ChanlNum;
        stBufConf.u32Flags = 0;

        s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hBufHandle , s32MilliSec);
        if(s32Ret != MI_SUCCESS)
            return MI_AO_ERR_NOBUF;

        memcpy(stBufInfo.stRawData.pVirAddr, _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf+s32SrcBufRdPtr, stBufConf.stRawCfg.u32Size);

        s32SrcBufRdPtr += stBufConf.stRawCfg.u32Size;
        stBufInfo.stRawData.u32ContentSize  = stBufConf.stRawCfg.u32Size ;
        if(s32SrcBufRdPtr == s32SrcBufWrPtr)
        {
           s32SrcBufRdPtr = 0;
           s32SrcBufWrPtr = 0;
        }
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr = s32SrcBufWrPtr ;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr = s32SrcBufRdPtr;

        // if enable Vqe
        hApcHandle = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hApcHandle;

        if(TRUE ==  _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
        {
            u32VqeSampleUnitByte = MI_AUDIO_VQE_SAMPLES_UNIT * u32BitwidthByte * u16ChanlNum;
            u32VqeOutBytes = stBufInfo.stRawData.u32ContentSize;
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

        MI_SYS_FlushCache(stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize);

        s32Ret =  MI_SYS_ChnInputPortPutBuf(hBufHandle, &stBufInfo, FALSE);
        if(s32Ret!= MI_SUCCESS)
            return  MI_AO_ERR_BUF_FULL;


    }
    return s32Ret;
}


//-------------------------------------------------------------------------------------------------
//  global function  prototypes
//-------------------------------------------------------------------------------------------------
MI_S32 MI_AO_SetPubAttr(MI_AUDIO_DEV AoDevId, MI_AUDIO_Attr_t *pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_SetPubAttr_t stSetPubAttr;

    memset(&stSetPubAttr, 0, sizeof(stSetPubAttr));
    stSetPubAttr.AoDevId = AoDevId;
    memcpy(&stSetPubAttr.stPubAttr, pstAttr, sizeof(MI_AUDIO_Attr_t));

    s32Ret = MI_SYSCALL(MI_AO_SET_PUB_ATTR, &stSetPubAttr);

    // save attribute of AO device for user mode
   if(s32Ret == MI_SUCCESS)
   {
        _gastAoDevInfoUsr[AoDevId].bDevAttrSet = TRUE;
        memcpy(&_gastAoDevInfoUsr[AoDevId].stDevAttr, pstAttr, sizeof(MI_AUDIO_Attr_t));
    }

    return s32Ret;
}


MI_S32 MI_AO_GetPubAttr(MI_AUDIO_DEV AoDevId, MI_AUDIO_Attr_t *pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_GetPubAttr_t stGetPubAttr;

    memset(&stGetPubAttr, 0, sizeof(stGetPubAttr));
    stGetPubAttr.AoDevId = AoDevId;
    s32Ret = MI_SYSCALL(MI_AO_GET_PUB_ATTR, &stGetPubAttr);
    if (s32Ret == MI_SUCCESS)
    {
        memcpy(pstAttr, &stGetPubAttr.stPubAttr, sizeof(MI_AUDIO_Attr_t));
    }

    return s32Ret;
}

MI_S32 MI_AO_Enable(MI_AUDIO_DEV AoDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AO_ENABLE, &AoDevId);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAoDevInfoUsr[AoDevId].bDevEnable = TRUE;
    }

    return s32Ret;
}

MI_S32 MI_AO_Disable(MI_AUDIO_DEV AoDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AO_DISABLE, &AoDevId);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAoDevInfoUsr[AoDevId].bDevEnable = FALSE;
    }

    return s32Ret;
}

MI_S32 MI_AO_EnableChn(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_EnableChn_t stEnableChn;

    memset(&stEnableChn, 0, sizeof(stEnableChn));
    stEnableChn.AoDevId = AoDevId;
    stEnableChn.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_ENABLE_CHN, &stEnableChn);

    if(s32Ret == MI_SUCCESS)
   {
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable = TRUE;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32ChnId = AoChn;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bPortEnable = TRUE;
    }

    return s32Ret;
}

MI_S32 MI_AO_DisableChn(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_DisableChn_t stDisableChn;

    memset(&stDisableChn, 0, sizeof(stDisableChn));
    stDisableChn.AoDevId = AoDevId;
    stDisableChn.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_DISABLE_CHN, &stDisableChn);

    if(s32Ret == MI_SUCCESS)
   {
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable = FALSE;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bPortEnable = FALSE;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable = FALSE;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bResampleEnable = FALSE;
    }

    return s32Ret;
}

MI_S32 MI_AO_SendFrame(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AUDIO_Frame_t *pstData, MI_S32 s32MilliSec)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_S32 s32BitwidthByte;
    MI_U16 u16ChanlNum;
    MI_U32  u32LenPerChanl;
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hBufHandle;
    //MI_U32 u32LenBytePerChanl;

    // Resample,
    MI_S32 s32SrcOutSample;
    SRC_HANDLE hSrcHandle;
    MI_S32 s32InputPoint;
    MI_S32 s32SrcBufWrPtr;
    MI_S32 s32SrcBufRdPtr;
    MI_U8* pu8SrcInBuf;
    MI_U8* pu8SrcOutBuf;

    // Vqe
    APC_HANDLE hApcHandle;
    MI_U32 u32VqeWrPtr;
    MI_U32 u32VqeSampleUnitByte;
    MI_U32 u32VqeOutBytes;

    ///check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);
    MI_AO_USR_CHECK_POINTER(pstData);

    if(TRUE != _gastAoDevInfoUsr[AoDevId].bDevEnable)
        return MI_AO_ERR_NOT_ENABLED;

    if(TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable)
        return MI_AO_ERR_NOT_ENABLED;

    u32LenPerChanl = pstData->u32Len;

    MI_AUDIO_USR_TRANS_BWIDTH_TO_BYTE(s32BitwidthByte, pstData->eBitwidth);
    MI_AUDIO_USR_TRANS_EMODE_TO_CHAN(u16ChanlNum, pstData->eSoundmode);

    //check pstData data length is  Multiples of MI_AUDIO_VQE_SAMPLES_UNIT ?
    if( u32LenPerChanl & (MI_AUDIO_VQE_SAMPLES_UNIT * s32BitwidthByte - 1) )
        return MI_AO_ERR_ILLEGAL_PARAM;

    stChnPort.eModId = E_MI_MODULE_ID_AO;
    stChnPort.u32DevId = AoDevId;
    stChnPort.u32ChnId = AoChn;
    stChnPort.u32PortId = 0;  //input port id

    if( TRUE !=  _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bResampleEnable)
    {
        stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufConf.u64TargetPts = pstData->u64TimeStamp;
        stBufConf.stRawCfg.u32Size = (u32LenPerChanl ) * u16ChanlNum;
        stBufConf.u32Flags = 0;

        s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hBufHandle , s32MilliSec);
        if(s32Ret != MI_SUCCESS)
            return  MI_AO_ERR_NOBUF;

        //Let 2 channel data to interleave
        if(pstData->eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)
           _MI_AO_InterleaveData(stBufInfo.stRawData.pVirAddr, (MI_U8*)pstData->apVirAddr[0], (MI_U8*)pstData->apVirAddr[1], u32LenPerChanl);
        else
            memcpy(stBufInfo.stRawData.pVirAddr, pstData->apVirAddr[0], u32LenPerChanl);

        stBufInfo.stRawData.u32ContentSize  = stBufConf.stRawCfg.u32Size ;

        // if enable Vqe
        hApcHandle = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hApcHandle;

        if(TRUE ==  _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
        {
            u32VqeSampleUnitByte = MI_AUDIO_VQE_SAMPLES_UNIT * s32BitwidthByte * u16ChanlNum;
            u32VqeOutBytes = stBufInfo.stRawData.u32ContentSize;
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


        MI_SYS_FlushCache(stBufInfo.stRawData.pVirAddr,stBufInfo.stRawData.u32BufSize);

        s32Ret =  MI_SYS_ChnInputPortPutBuf(hBufHandle, &stBufInfo, FALSE);
        if(s32Ret!= MI_SUCCESS)
            return  MI_AO_ERR_BUF_FULL;

     }
    else    //enable Resample
    {

        //need to process remain output buffer of resample because no input port buffer in last sending
        s32Ret =  _MI_AO_ProcPreSrcOut(AoDevId, AoChn, pstData, s32MilliSec);
        if(s32Ret != MI_SUCCESS)
            return s32Ret;

        pu8SrcInBuf = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcInBuf;
        pu8SrcOutBuf = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf;

        //Let 2 channel data to interleave
        if(pstData->eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)
           _MI_AO_InterleaveData(pu8SrcInBuf, (MI_U8*)pstData->apVirAddr[0], (MI_U8*)pstData->apVirAddr[1], u32LenPerChanl);
        else
            memcpy(pu8SrcInBuf, pstData->apVirAddr[0], u32LenPerChanl);

        hSrcHandle = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hSrcHandle;
        /* Run SRC process */
        s32InputPoint = u32LenPerChanl /s32BitwidthByte;
        s32SrcOutSample = 0;
        s32SrcBufWrPtr = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr;
        s32SrcBufRdPtr = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr;

#ifndef __KERNEL__
        //s32SrcOutSample = IaaSrc_Run(hSrcHandle, (MI_S16*)pstData->apVirAddr[0], (MI_S16*)( pu8SrcOutBuf+s32SrcBufWrPtr), s32InputPoint);
        s32SrcOutSample = IaaSrc_Run(hSrcHandle, (MI_S16*)pu8SrcInBuf, (MI_S16*)( pu8SrcOutBuf+s32SrcBufWrPtr), s32InputPoint);
#endif


        s32SrcBufWrPtr += s32SrcOutSample*s32BitwidthByte*u16ChanlNum; // update  s32SrcBufWrPtr
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr = s32SrcBufWrPtr ;

        while( (s32SrcBufWrPtr - s32SrcBufRdPtr ) >= u32LenPerChanl*u16ChanlNum )
        {
            stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
            stBufConf.u64TargetPts = pstData->u64TimeStamp;
            stBufConf.stRawCfg.u32Size = u32LenPerChanl * u16ChanlNum;
            stBufConf.u32Flags = 0;


            s32Ret = MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hBufHandle , s32MilliSec);
            if(s32Ret != MI_SUCCESS)
                return MI_SUCCESS; // the frame is processed, the output of SRC will process by _MI_AO_ProcPreSrcOut in next time;

            memcpy(stBufInfo.stRawData.pVirAddr, pu8SrcOutBuf+s32SrcBufRdPtr, stBufConf.stRawCfg.u32Size);

            s32SrcBufRdPtr += stBufConf.stRawCfg.u32Size;
            stBufInfo.stRawData.u32ContentSize  = stBufConf.stRawCfg.u32Size ;
            if(s32SrcBufRdPtr == s32SrcBufWrPtr)
            {
               s32SrcBufRdPtr = 0;
               s32SrcBufWrPtr = 0;
            }
            _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr = s32SrcBufWrPtr ;
            _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr = s32SrcBufRdPtr;


             // if enable Vqe
            hApcHandle = _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hApcHandle;

            if(TRUE ==  _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
            {
                u32VqeSampleUnitByte = MI_AUDIO_VQE_SAMPLES_UNIT * s32BitwidthByte * u16ChanlNum;
                u32VqeOutBytes = stBufInfo.stRawData.u32ContentSize;
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


            MI_SYS_FlushCache(stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize);

            s32Ret =  MI_SYS_ChnInputPortPutBuf(hBufHandle, &stBufInfo, FALSE);
            if(s32Ret!= MI_SUCCESS)
                return  MI_AO_ERR_BUF_FULL;

        }


    }

    return s32Ret;
}

MI_S32 MI_AO_EnableReSmp(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AUDIO_SampleRate_e eInSampleRate)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_EnableReSmp_t stEnableResmp;

    ///check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);
    MI_AO_USR_CHECK_SAMPLERATE(eInSampleRate);

    if(TRUE != _gastAoDevInfoUsr[AoDevId].bDevEnable)
        return MI_AO_ERR_NOT_ENABLED;

    if(TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable)
        return MI_AO_ERR_NOT_ENABLED;

    // put resample status to kernel mode for DebugFs
    memset(&stEnableResmp, 0, sizeof(stEnableResmp));
    stEnableResmp.AoDevId = AoDevId;
    stEnableResmp.AoChn= AoChn;
    stEnableResmp.eInSampleRate = eInSampleRate;
    s32Ret = MI_SYSCALL(MI_AO_ENABLE_RESMP, &stEnableResmp);

    if(s32Ret == MI_SUCCESS)
    {
        // Enable channel of AO device resample
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bResampleEnable = TRUE;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].eInResampleRate = eInSampleRate;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufWrPtr = 0;
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].s32SrcBufRdPtr = 0;

        /* SRC parameter setting */
        s32Ret = _MI_AO_ReSmpInit(AoDevId, AoChn, eInSampleRate);
    }

    return s32Ret;
}

MI_S32 MI_AO_DisableReSmp(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_DisableReSmp_t stDisableResmp;

    // check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);

    //put resample status to kernel mode for DebugFs
    memset(&stDisableResmp, 0, sizeof(stDisableResmp));
    stDisableResmp.AoDevId = AoDevId;
    stDisableResmp.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_DISABLE_RESMP, &stDisableResmp);

    if(s32Ret == MI_SUCCESS)
    {
        // Disable channel of AO device resample
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bResampleEnable = FALSE;

#ifndef __KERNEL__
    if(NULL != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pSrcWorkingBuf)
    {
        MI_AO_Free(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pSrcWorkingBuf);
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pSrcWorkingBuf = NULL;
    }

    if(NULL != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcInBuf)
    {
        MI_AO_Free(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcInBuf);
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcInBuf = NULL;
     }

    if(NULL != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf)
    {
        MI_AO_Free(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf);
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pu8SrcOutBuf = NULL;
     }

    IaaSrc_Release( _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hSrcHandle);

#endif
    }

    return s32Ret;
}

MI_S32 MI_AO_PauseChn(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_PauseChn_t stPauseChn;

    memset(&stPauseChn, 0, sizeof(stPauseChn));
    stPauseChn.AoDevId = AoDevId;
    stPauseChn.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_PAUSE_CHN, &stPauseChn);

    return s32Ret;
}

MI_S32 MI_AO_ResumeChn(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_ResumeChn_t stResumeChn;

    memset(&stResumeChn, 0, sizeof(stResumeChn));
    stResumeChn.AoDevId = AoDevId;
    stResumeChn.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_RESUME_CHN, &stResumeChn);

    return s32Ret;
}

MI_S32 MI_AO_ClearChnBuf(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_ClearChnBuf_t stClearChnBuf;

    memset(&stClearChnBuf, 0, sizeof(stClearChnBuf));
    stClearChnBuf.AoDevId = AoDevId;
    stClearChnBuf.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_CLEAR_CHN_BUF, &stClearChnBuf);

    return s32Ret;
}

MI_S32 MI_AO_QueryChnStat(MI_AUDIO_DEV AoDevId , MI_AO_CHN AoChn, MI_AO_ChnState_t *pstStatus)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_QueryChnStat_t stQueryChnStat;

    memset(&stQueryChnStat, 0, sizeof(stQueryChnStat));
    stQueryChnStat.AoDevId = AoDevId;
    stQueryChnStat.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_QUERY_CHN_STAT, &stQueryChnStat);

    if(s32Ret == MI_SUCCESS)
    {
        memcpy(pstStatus, &stQueryChnStat.stStatus, sizeof(MI_AO_ChnState_t));
    }

    return s32Ret;
}

MI_S32 MI_AO_SetVolume(MI_AUDIO_DEV AoDevId, MI_S32 s32VolumeDb)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_SetVolume_t stSetVolume;

    memset(&stSetVolume, 0, sizeof(stSetVolume));
    stSetVolume.AoDevId = AoDevId;
    stSetVolume.s32VolumeDb = s32VolumeDb;
    s32Ret = MI_SYSCALL(MI_AO_SET_VOLUME, &stSetVolume);

    return s32Ret;
}

MI_S32 MI_AO_GetVolume(MI_AUDIO_DEV AoDevId, MI_S32 *ps32VolumeDb)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_GetVolume_t stGetVolume;

    memset(&stGetVolume, 0, sizeof(stGetVolume));
    stGetVolume.AoDevId = AoDevId;
    s32Ret = MI_SYSCALL(MI_AO_GET_VOLUME, &stGetVolume);

    if(s32Ret == MI_SUCCESS)
    {
        memcpy(ps32VolumeDb, &stGetVolume.s32VolumeDb, sizeof(MI_S32));
    }

    return s32Ret;
}

MI_S32 MI_AO_SetMute(MI_AUDIO_DEV AoDevId, MI_BOOL bEnable)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_SetMute_t stSetMute;

    memset(&stSetMute, 0, sizeof(stSetMute));
    stSetMute.AoDevId = AoDevId;
    stSetMute.bEnable = bEnable;
    s32Ret = MI_SYSCALL(MI_AO_SET_MUTE, &stSetMute);

    return s32Ret;
}

MI_S32 MI_AO_GetMute(MI_AUDIO_DEV AoDevId, MI_BOOL *pbEnable)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_GetMute_t stGetMute;

    memset(&stGetMute, 0, sizeof(stGetMute));
    stGetMute.AoDevId = AoDevId;
    s32Ret = MI_SYSCALL(MI_AO_GET_MUTE, &stGetMute);

    if(s32Ret == MI_SUCCESS)
    {
        memcpy(pbEnable, &stGetMute.bEnable, sizeof(MI_BOOL));
    }

    return s32Ret;
}

MI_S32 MI_AO_ClrPubAttr(MI_AUDIO_DEV AoDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    s32Ret = MI_SYSCALL(MI_AO_CLR_PUB_ATTR, &AoDevId);

    return s32Ret;
}

MI_S32 MI_AO_SetVqeAttr(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AO_VqeConfig_t *pstVqeConfig)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_SetVqeAttr_t stSetVqeAttr;


    // check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);
    MI_AO_USR_CHECK_POINTER(pstVqeConfig);

    // check if Vqe of AO device channel is disable ?
    if( FALSE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
        return MI_AO_ERR_NOT_PERM;

    // check if AO device channel is enable ?
    if( TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable)
        return MI_AO_ERR_NOT_ENABLED;

    // check if Vqe Configure legal ?

    // put VQE status to kernel mode for DebugFs
    memset(&stSetVqeAttr, 0, sizeof(stSetVqeAttr));
    stSetVqeAttr.AoDevId = AoDevId;
    stSetVqeAttr.AoChn= AoChn;
    memcpy(&stSetVqeAttr.stVqeConfig, pstVqeConfig, sizeof(MI_AO_VqeConfig_t));
    s32Ret = MI_SYSCALL(MI_AO_SET_VQE_ATTR, &stSetVqeAttr);

    if(s32Ret == MI_SUCCESS)
    {
        // save Vqe configure of AO device channel
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeAttrSet = TRUE;
        memcpy(&_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].stAoVqeConfig, pstVqeConfig, sizeof(MI_AO_VqeConfig_t));
    }

    return s32Ret;
}

MI_S32 MI_AO_GetVqeAttr(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AO_VqeConfig_t *pstVqeConfig)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);
    MI_AO_USR_CHECK_POINTER(pstVqeConfig);

    // check if Vqe attribute of AO device channel is set ?
    if(TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeAttrSet)
        return MI_AO_ERR_NOT_PERM;

    // load Vqe config
    memcpy(pstVqeConfig, &_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].stAoVqeConfig, sizeof(MI_AO_VqeConfig_t));

    return s32Ret;
}

MI_S32 MI_AO_EnableVqe(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
     MI_S32 s32Ret = MI_SUCCESS;
     MI_AO_EnableVqe_t stEnableVqe;

    // check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);

    //
    if( TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeAttrSet)
        return MI_AO_ERR_NOT_PERM;

    if( TRUE != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bChanEnable)
        return MI_AO_ERR_NOT_ENABLED;

    if(TRUE == _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
        return s32Ret;

    // put VQE status to kernel mode for DebugFs
    memset(&stEnableVqe, 0, sizeof(stEnableVqe));
    stEnableVqe.AoDevId = AoDevId;
    stEnableVqe.AoChn= AoChn;
    s32Ret = MI_SYSCALL(MI_AO_ENABLE_VQE, &stEnableVqe);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable = TRUE;

        //VQE init
        s32Ret = _MI_AO_VqeInit(AoDevId, AoChn);
    }

    return s32Ret;
}

MI_S32 MI_AO_DisableVqe(MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AO_DisableVqe_t stDisableVqe;

    // check input parameter
    MI_AO_USR_CHECK_DEV(AoDevId);
    MI_AO_USR_CHECK_CHN(AoChn);

    if(FALSE == _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable)
        return s32Ret;

    // put vqe status to kernel mode for DebugFs
    memset(&stDisableVqe, 0, sizeof(stDisableVqe));
    stDisableVqe.AoDevId = AoDevId;
    stDisableVqe.AoChn = AoChn;
    s32Ret = MI_SYSCALL(MI_AO_DISABLE_VQE, &stDisableVqe);

    if(s32Ret == MI_SUCCESS)
    {
        _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeEnable = FALSE;
        //_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].bVqeAttrSet = FALSE; // ???

#ifndef __KERNEL__
    if(NULL != _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf)
    {
        MI_AO_Free( _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf);
         _gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].pApcWorkingBuf = NULL;
    }

    IaaApc_Free(_gastAoDevInfoUsr[AoDevId].astChanInfo[AoChn].hApcHandle);
#endif
    }

    return s32Ret;
}

EXPORT_SYMBOL(MI_AO_SetPubAttr);
EXPORT_SYMBOL(MI_AO_GetPubAttr);
EXPORT_SYMBOL(MI_AO_Enable);
EXPORT_SYMBOL(MI_AO_Disable);
EXPORT_SYMBOL(MI_AO_EnableChn);
EXPORT_SYMBOL(MI_AO_DisableChn);
EXPORT_SYMBOL(MI_AO_SendFrame);
EXPORT_SYMBOL(MI_AO_EnableReSmp);
EXPORT_SYMBOL(MI_AO_DisableReSmp);
EXPORT_SYMBOL(MI_AO_PauseChn);
EXPORT_SYMBOL(MI_AO_ResumeChn);
EXPORT_SYMBOL(MI_AO_ClearChnBuf);
EXPORT_SYMBOL(MI_AO_QueryChnStat);
EXPORT_SYMBOL(MI_AO_SetVolume);
EXPORT_SYMBOL(MI_AO_GetVolume);
EXPORT_SYMBOL(MI_AO_SetMute);
EXPORT_SYMBOL(MI_AO_GetMute);
EXPORT_SYMBOL(MI_AO_ClrPubAttr);
EXPORT_SYMBOL(MI_AO_SetVqeAttr);
EXPORT_SYMBOL(MI_AO_GetVqeAttr);
EXPORT_SYMBOL(MI_AO_EnableVqe);
EXPORT_SYMBOL(MI_AO_DisableVqe);
