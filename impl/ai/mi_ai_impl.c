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
#include "mi_ai_impl.h"

#include <linux/string.h>
#if !USE_CAM_OS
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>
#endif

#include "mi_common.h"
#include "mi_print.h"
#include "mi_sys_internal.h"
//#include "mi_ai_impl.h"
#include "mi_ai.h"
#include "mi_ai_datatype_internal.h"
#include "mhal_audio.h"
#include "mi_sys_proc_fs_internal.h"

#if USE_CAM_OS
#include "cam_os_wrapper.h"
#include "cam_os_util.h"
#endif

//=============================================================================
// Include files
//=============================================================================


//=============================================================================
// Extern definition
//=============================================================================

//=============================================================================
// Macro definition
//=============================================================================

#define MI_AI_CHECK_DEV(AiDevId) \
    if(AiDevId < 0 || AiDevId >= MI_AI_DEV_NUM_MAX) \
    {   \
        DBG_ERR("AiDevId is invalid! AiDevId = %u.\n", AiDevId);   \
        return MI_AI_ERR_INVALID_DEVID;   \
    }

#define MI_AI_CHECK_CHN(Aichn)  \
    if(Aichn < 0 || Aichn >= MI_AI_CHAN_NUM_MAX) \
    {   \
        DBG_ERR("Aichn is invalid! Aichn = %u.\n", Aichn);   \
        return MI_AI_ERR_INVALID_CHNID;   \
    }

#define MI_AI_CHECK_POINTER(pPtr)  \
    if(NULL == pPtr)  \
    {   \
        DBG_ERR("Invalid parameter! NULL pointer.\n");   \
        return MI_AI_ERR_NULL_PTR;   \
    }

#define MI_AI_CHECK_SAMPLERATE(eSamppleRate)    \
    if( (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_8000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_16000) &&\
        (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_32000) && (eSamppleRate != E_MI_AUDIO_SAMPLE_RATE_48000) ) \
    { \
        DBG_ERR("Sample Rate is illegal = %u.\n", eSamppleRate);   \
        return MI_AI_ERR_ILLEGAL_PARAM;   \
    }

#define MI_AI_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, eWidth)          \
    switch(eWidth) \
    {   \
        case E_MI_AUDIO_BIT_WIDTH_16:   \
            u32BitWidthByte = 2;    \
        break;  \
        case E_MI_AUDIO_BIT_WIDTH_24:   \
            u32BitWidthByte = 4;    \
            break;  \
        default:    \
            u32BitWidthByte = 0; \
            DBG_ERR("BitWidth is illegal = %u.\n", eWidth); \
            break; \
    }

#if USE_CAM_OS
    #define MI_AI_ThreadWakeUp CamOsThreadWakeUp
    #define MI_AI_ThreadStop   CamOsThreadStop
    #define MI_AI_ThreadShouldStop() (CamOsThreadShouldStop() == CAM_OS_OK)
#else
    #define MI_AI_ThreadWakeUp wake_up_process
    #define MI_AI_ThreadStop   kthread_stop
    #define MI_AI_ThreadShouldStop() (kthread_should_stop())
#endif

//==== for kthread porting ====
#if USE_CAM_OS
#define MI_AI_CreateThreadWrapper(pfnStartRoutine) \
    void * _##pfnStartRoutine (void* pArg) \
    { \
        (void)pfnStartRoutine(pArg); \
        return NULL; \
    }
#define MI_AI_CreateThread(ptThread, szName, pfnStartRoutine, pArg) \
    _MI_AI_CreateThread(ptThread, szName, _##pfnStartRoutine, pArg)

MI_S32 _MI_AI_CreateThread(MI_AI_Thread_t *ptThread, char* szName, void *(*pfnStartRoutine)(void *), void* pArg)
{
    CamOsThreadAttrb_t stAttr = {0, 0};
    CamOsRet_e eRet;
    eRet = CamOsThreadCreate(ptThread, &stAttr, pfnStartRoutine, pArg);
    if (CAM_OS_OK != eRet)
    {
        return -1;
    }
    return MI_SUCCESS;
}
#else
#define MI_AI_CreateThreadWrapper(pfnStartRoutine)
MI_S32 MI_AI_CreateThread(MI_AI_Thread_t *ptThread, char* szName, int (*pfnStartRoutine)(void *), void* pArg)
{
    *ptThread = kthread_create(pfnStartRoutine, pArg, szName);
    if (IS_ERR(*ptThread))
    {
        DBG_ERR("Fail to create AI thread.\n");
        return -1;
    }
    return MI_SUCCESS;
}
#endif


//=============================================================================
// Data type definition
//=============================================================================



//=============================================================================
// Variable definition
//=============================================================================
static _MI_AI_DevInfo_t _gastAiDevInfo[MI_AI_DEV_NUM_MAX] ={
    {
        .bDevEnable = FALSE,
        .bDevAttrSet = FALSE,
        .hDevSysHandle = MI_HANDLE_NULL,
        .astChanInfo[0] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[1] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[2] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[3] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[4] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[5] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[6] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[7] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[8] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[9] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[10] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[11] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
        },
        .astChanInfo[12] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[13] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[14] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
        .astChanInfo[15] = {
            .bChanEnable = FALSE,
            .bPortEnable = FALSE,
            .bResampleEnable = FALSE,
        },
    },
};


//=============================================================================
// Local function definition
//=============================================================================
#if defined(MI_SYS_PROC_FS_DEBUG) && (MI_AI_PROCFS_DEBUG == 1)
static MI_S32 _MI_AI_IMPL_OnDumpDevAttr(MI_SYS_DEBUG_HANDLE_t  handle, MI_U32  u32DevId, void *pUsrData)
{
    MI_S32 s32Ret = MI_SUCCESS;

    _MI_AI_DevInfo_t* pstAiDevInfo  = (_MI_AI_DevInfo_t*)pUsrData;
    MI_U8 szWorkMode[15] = {'\0'};
    MI_U8 szBitWidth[15] = {'\0'};
    MI_U8 szSoundMode[15] = {'\0'};
    MI_U32 u32AttrSampleRate = (MI_U32)pstAiDevInfo->stDevAttr.eSamplerate;
    MI_U32 u32PtNumPerFrm = pstAiDevInfo->stDevAttr.u32PtNumPerFrm;

    switch (pstAiDevInfo->stDevAttr.eWorkmode)
    {
        case E_MI_AUDIO_MODE_I2S_MASTER:
            strcpy(szWorkMode,"i2s-mas");
            break;

        case E_MI_AUDIO_MODE_I2S_SLAVE:
            strcpy(szWorkMode, "i2s-sla");
            break;

        case E_MI_AUDIO_MODE_TDM_MASTER:
            strcpy(szWorkMode, "tdm-mas");
            break;

        default:
            strcpy(szWorkMode, "not-set");
            break;
    }

    if(pstAiDevInfo->stDevAttr.eBitwidth == E_MI_AUDIO_BIT_WIDTH_16)
        strcpy(szBitWidth, "16bit");
    else if(pstAiDevInfo->stDevAttr.eBitwidth == E_MI_AUDIO_BIT_WIDTH_24)
        strcpy(szBitWidth, "24bit");
    else
        strcpy(szBitWidth, "not-set");

    if(pstAiDevInfo->stDevAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO)
        strcpy(szSoundMode, "mono");
    else if(pstAiDevInfo->stDevAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)
        strcpy(szSoundMode, "stereo");
    else
        strcpy(szSoundMode, "not-set");

    handle.OnPrintOut(handle,"\n======================================Private AI%d Info ======================================\n", u32DevId);
    handle.OnPrintOut(handle,"-----Start AI Dev%d Attr---------------------------------------------------------------\n", u32DevId);
    handle.OnPrintOut(handle,"AoDev  WorkMode  SampR  BitWidth  SondMod  PtNumPerFrm\n");
    handle.OnPrintOut(handle,"%5d  %8s  %5d  %8s  %7s  %11d \n",pstAiDevInfo->AiDevId, szWorkMode, u32AttrSampleRate, szBitWidth, szSoundMode, u32PtNumPerFrm);
    handle.OnPrintOut(handle,"bMute  VolumedB  TotalWrFrmCnt  \n");
    handle.OnPrintOut(handle,"-----End AI Dev%d Attr-----------------------------------------------------------------\n", u32DevId);
    handle.OnPrintOut(handle,"\n");

    return s32Ret;

}
static MI_S32 _MI_AI_IMPL_OnDumpChannelAttr(MI_SYS_DEBUG_HANDLE_t  handle, MI_U32  u32DevId,void *pUsrData)
{
    MI_S32 s32Ret = MI_SUCCESS;

    _MI_AI_DevInfo_t* pstAiDevInfo  = (_MI_AI_DevInfo_t*)pUsrData;
    _MI_AI_ChanInfo_t  stChanInfo;
    MI_AUDIO_DEV    AiDevId = pstAiDevInfo->AiDevId;
    MI_AI_CHN   AiChn;
    MI_AUDIO_SampleRate_e  eInResampleRate = pstAiDevInfo->stDevAttr.eSamplerate;
    MI_AUDIO_SampleRate_e eOutResampleRate;
    MI_BOOL  bResampleEnable;
    MI_BOOL bVqeEnable;
    MI_AI_VqeConfig_t   stAiVqeConfig;
    MI_U32 u32Iidx;
    MI_U8 szAnrSpeed[15] = {'\0'};

    for(u32Iidx = 0; u32Iidx < MI_AUDIO_MAX_CHN_NUM; u32Iidx++)
    {
        stChanInfo = pstAiDevInfo->astChanInfo[u32Iidx];

        if(TRUE == stChanInfo.bChanEnable)
        {
            AiChn = stChanInfo.s32ChnId;
            bResampleEnable = stChanInfo.bResampleEnable;
            eOutResampleRate = stChanInfo.eOutResampleRate;
            bVqeEnable = stChanInfo.bVqeEnable;
            stAiVqeConfig = stChanInfo.stAiVqeConfig;

            handle.OnPrintOut(handle,"-----Start AI CHN%d STATUS------------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bReSmp  InSampR  OutSampR \n");
            handle.OnPrintOut(handle,"%5d  %5d  %6d  %7d  %8d \n",AiDevId, AiChn, bResampleEnable, eInResampleRate, eOutResampleRate);

            // Vqe status
            handle.OnPrintOut(handle,"-----AI CHN%d VQE STATUS--------------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bVqe  WorkRate  PoiNum \n");
            handle.OnPrintOut(handle,"%5d  %5d  %4d  %8d  %6d \n",AiDevId, AiChn, bVqeEnable, stAiVqeConfig.s32WorkSampleRate, stAiVqeConfig.s32FrameSample);

            if (E_MI_AUDIO_NR_SPEED_LOW == stAiVqeConfig.stAnrCfg.eNrSpeed)
            {
                strcpy(szAnrSpeed, "speed-low");
            }
            else if (E_MI_AUDIO_NR_SPEED_MID == stAiVqeConfig.stAnrCfg.eNrSpeed)
            {
                strcpy(szAnrSpeed, "speed-mid");
            }
            else if (E_MI_AUDIO_NR_SPEED_HIGH == stAiVqeConfig.stAnrCfg.eNrSpeed)
            {
                strcpy(szAnrSpeed, "speed-high");
            }
            else
            {
                strcpy(szAnrSpeed, "speed-notset");
            }

            handle.OnPrintOut(handle,"-----AI CHN%d ANR STATUS------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bAnr            Speed  Intensity  SmoothLevel\n");
            handle.OnPrintOut(handle,"%5d  %5d  %4d  %15s  %9d  %11d\n",AiDevId, AiChn, stAiVqeConfig.bAnrOpen, szAnrSpeed,
                                                                       stAiVqeConfig.stAnrCfg.u32NrIntensity,
                                                                       stAiVqeConfig.stAnrCfg.u32NrSmoothLevel);

            // Eq Status
            handle.OnPrintOut(handle,"-----AI CHN%d Eq TATUS------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bEq  100Hz  200Hz  250Hz  350Hz  500Hz  800Hz  1.2KHz  2.5KHz  4KHz  8KHz  \n");
            handle.OnPrintOut(handle,"%5d  %5d  %3d  %5d  %5d  %5d  %5d  %5d  %5d  %6d  %6d  %4d  %4d  \n",
                                AiDevId, AiChn, stAiVqeConfig.bEqOpen,
                                stAiVqeConfig.stEqCfg.stEqGain.s16EqGain100Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain200Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain250Hz,
                                stAiVqeConfig.stEqCfg.stEqGain.s16EqGain350Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain500Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain800Hz,
                                stAiVqeConfig.stEqCfg.stEqGain.s16EqGain1200Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain2500Hz, stAiVqeConfig.stEqCfg.stEqGain.s16EqGain4000Hz,
                                stAiVqeConfig.stEqCfg.stEqGain.s16EqGain8000Hz);

            // Hpf Status
            handle.OnPrintOut(handle,"-----AI CHN%d Hpf STATUS-------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bHpf  HpfFreq  \n");
            handle.OnPrintOut(handle,"%5d  %5d  %4d  %7d  \n",AiDevId, AiChn, stAiVqeConfig.bHpfOpen, stAiVqeConfig.stHpfCfg.eHpfFreq);

            // Agc Status
            handle.OnPrintOut(handle,"-----AI CHN%d Agc STATUS------------------------------------------------------\n", AiChn);
            handle.OnPrintOut(handle,"AiDev  AiChn  bAgc  AttackTime  ReleaseTime  CompressionRatio  DropGainMax  \n");
            handle.OnPrintOut(handle,"%5d  %5d  %4d  %10d  %11d  %16d  %11d   \n", AiDevId, AiChn, stAiVqeConfig.bAgcOpen,
                                                                                (MI_S32)stAiVqeConfig.stAgcCfg.u32AttackTime,
                                                                                stAiVqeConfig.stAgcCfg.u32ReleaseTime,
                                                                                stAiVqeConfig.stAgcCfg.u32CompressionRatio,
                                                                                stAiVqeConfig.stAgcCfg.u32DropGainMax);

            handle.OnPrintOut(handle,"GainInit  GainMin  GainMax  NoiseGateAttenuationDb  NoiseGateDb  TargetLevelDb  \n");
            handle.OnPrintOut(handle,"%8d  %7d  %7d  %22d  %11d  %13d  \n", stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainInit,
                                                                            stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMin,
                                                                            stAiVqeConfig.stAgcCfg.stAgcGainInfo.s32GainMax,
                                                                            stAiVqeConfig.stAgcCfg.u32NoiseGateAttenuationDb,
                                                                            stAiVqeConfig.stAgcCfg.s32NoiseGateDb,
                                                                            stAiVqeConfig.stAgcCfg.s32TargetLevelDb);

            handle.OnPrintOut(handle,"-----End AI Chn%d STATUS-------------------------------------------------------------\n\n", AiChn);
        }
    }
    return s32Ret;
}

static MI_S32 _MI_AI_IMPL_OnDumpInputPortAttr(MI_SYS_DEBUG_HANDLE_t  handle, MI_U32 u32DevId, void *pUsrData)
{
    MI_S32 s32Ret = MI_SUCCESS;

    return s32Ret;
}

static MI_S32 _MI_AI_IMPL_OnDumpOutputPortAttr(MI_SYS_DEBUG_HANDLE_t  handle, MI_U32  u32DevId, void *pUsrData)
{
    MI_S32 s32Ret = MI_SUCCESS;

    return s32Ret;
}

static MI_S32 _MI_AI_IMPL_OnHelp(MI_SYS_DEBUG_HANDLE_t  handle,MI_U32  u32DevId,void *pUsrData)
{
    MI_S32 s32Ret = MI_SUCCESS;
    // ToDo

    return s32Ret;
}
#endif

static MI_S32 _MI_AI_DeinterleaveData(MI_SYS_BufInfo_t *pstOutputBufInfo, _MI_AI_DevInfo_t* pstAiDevInfo, MI_S32 s32ChnIdx)
{
    MI_S32 s32Ret;
    MI_S16* ps16DestAddr;
    MI_S16* ps16SrcAddr;
    MI_U32 u32BufSize;
    MI_BOOL bInterleaved ;
    MI_U32 u32BufLenSmp;
    MI_U32 u32idx;
    MI_U32 u32ChnCnt;
    MI_U32 u32BitWidthByte;

    s32Ret = MI_SUCCESS;
    bInterleaved = pstAiDevInfo->bInterleaved;
    u32ChnCnt = pstAiDevInfo->stDevAttr.u32ChnCnt;
    MI_AI_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, pstAiDevInfo->stDevAttr.eBitwidth);
    u32BufSize = pstOutputBufInfo->stRawData.u32BufSize;
    u32BufLenSmp = u32BufSize / u32BitWidthByte ;

    if(bInterleaved == FALSE)
    {
        ps16DestAddr = pstOutputBufInfo->stRawData.pVirAddr;
        ps16SrcAddr =(MI_S16*)( pstAiDevInfo->pu8TempBufAddr + (s32ChnIdx * u32BufSize));

        memcpy(ps16DestAddr, ps16SrcAddr, u32BufSize);
        pstOutputBufInfo->stRawData.u32ContentSize = u32BufSize;
    }
    else // bInterleaved == TRUE
    {
        ps16DestAddr =(MI_S16*) pstOutputBufInfo->stRawData.pVirAddr;
        ps16SrcAddr = (MI_S16*)pstAiDevInfo->pu8TempBufAddr;
        for(u32idx = 0; u32idx < u32BufLenSmp; u32idx ++)
        {
            //(*((MI_S16*)pu8DestAddr++)) = (*( (MI_S16*)pu8SrcAddr + s32ChnIdx + u32idx*u32ChnCnt) );
            ps16DestAddr[u32idx] =  ps16SrcAddr[ s32ChnIdx + u32idx*u32ChnCnt] ;
        }
        pstOutputBufInfo->stRawData.u32ContentSize = u32BufSize;
     }

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief create AI device
/// @param[in]  AiDevId: AI device ID.
/// @return MI_SUCCESS: succeed in creating AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
//------------------------------------------------------------------------------
static MI_S32 _MI_AI_CreateDevice(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;

    mi_sys_ModuleDevInfo_t stModInfo;
    mi_sys_ModuleDevBindOps_t stAiOps;
    MI_SYS_DRV_HANDLE hDevSysHandle;
    MS_U32 u32BlkSize;

#ifdef MI_SYS_PROC_FS_DEBUG
    mi_sys_ModuleDevProcfsOps_t pstModuleProcfsOps;
#endif

    //check input parameter
    MI_AI_CHECK_DEV(AiDevId);

    memset(&stModInfo, 0x0, sizeof(mi_sys_ModuleDevInfo_t));
    stModInfo.eModuleId = E_MI_MODULE_ID_AI;
    stModInfo.u32DevId = AiDevId;
    stModInfo.u32DevChnNum = 16;
    stModInfo.u32InputPortNum = 0;
    stModInfo.u32OutputPortNum = 1;

    memset(&stAiOps, 0x0, sizeof(stAiOps));
    stAiOps.OnBindInputPort   = NULL; //_MI_AI_OnBindChnnInputCallback;
    stAiOps.OnUnBindInputPort = NULL; // _MI_AI_OnUnBindChnnInputCallback;
    stAiOps.OnBindOutputPort = NULL; //_MI_AI_OnBindChnOutputCallback;
    stAiOps.OnUnBindOutputPort = NULL; //_MI_AI_OnUnBindChnOutputCallback;
    stAiOps.OnOutputPortBufRelease = NULL;
#ifdef MI_SYS_PROC_FS_DEBUG
    memset(&pstModuleProcfsOps, 0 , sizeof(pstModuleProcfsOps));
#if (MI_AI_PROCFS_DEBUG == 1)
    pstModuleProcfsOps.OnDumpDevAttr = _MI_AI_IMPL_OnDumpDevAttr;
    pstModuleProcfsOps.OnDumpChannelAttr = _MI_AI_IMPL_OnDumpChannelAttr;
    pstModuleProcfsOps.OnDumpInputPortAttr = _MI_AI_IMPL_OnDumpInputPortAttr;
    pstModuleProcfsOps.OnDumpOutPortAttr = _MI_AI_IMPL_OnDumpOutputPortAttr;
    pstModuleProcfsOps.OnHelp = _MI_AI_IMPL_OnHelp;
#else
    pstModuleProcfsOps.OnDumpDevAttr = NULL;
    pstModuleProcfsOps.OnDumpChannelAttr = NULL;
    pstModuleProcfsOps.OnDumpInputPortAttr = NULL;
    pstModuleProcfsOps.OnDumpOutPortAttr = NULL;
    pstModuleProcfsOps.OnHelp = NULL;
#endif

#endif


    hDevSysHandle = mi_sys_RegisterDev(&stModInfo, &stAiOps, &_gastAiDevInfo[AiDevId]
                                                #ifdef MI_SYS_PROC_FS_DEBUG
                                                  , &pstModuleProcfsOps
                                                  ,MI_COMMON_GetSelfDir
                                                #endif
                                      );
    _gastAiDevInfo[AiDevId].hDevSysHandle = hDevSysHandle;
     if (hDevSysHandle == NULL)
    {
        DBG_ERR("MI AI device %d fail to register dev.\n", AiDevId);
    }

    // malloc audio HW memory
    u32BlkSize =  MI_AI_PCM_BUF_SIZE_BYTE;
    mi_sys_MMA_Alloc(NULL, u32BlkSize, (MI_PHY*)&_gastAiDevInfo[AiDevId].u64PhyBufAddr);//  TODO: need to using MMAP ???
     _gastAiDevInfo[AiDevId].pVirBufAddr = mi_sys_Vmap(_gastAiDevInfo[AiDevId].u64PhyBufAddr, u32BlkSize, FALSE);

    return s32Ret;

}

//------------------------------------------------------------------------------
/// @brief destroy AI device
/// @param[in]  AiDevId: AI device ID.
/// @return MI_SUCCESS: succeed in destroying AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
//------------------------------------------------------------------------------
static MI_S32 _MI_AI_DestroyDevice(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;

    //check input parameter
    MI_AI_CHECK_DEV(AiDevId);

    s32Ret = mi_sys_UnRegisterDev(_gastAiDevInfo[AiDevId].hDevSysHandle);
    mi_sys_UnVmap(_gastAiDevInfo[AiDevId].pVirBufAddr);
    s32Ret = mi_sys_MMA_Free(_gastAiDevInfo[AiDevId].u64PhyBufAddr);

    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("MI AI device %d fail to unregister dev.\n", AiDevId);
    }

    return s32Ret;

}


//------------------------------------------------------------------------------
/// @brief MI AI device read data thread
/// @param[in]  data: AI device Info
/// @return MI_SUCCESS: succeed in disabling AI device .
///             MI_AI_ERR_NOT_PERM: not permit
//------------------------------------------------------------------------------
int _MI_AI_ReadDataThread(void* data)
{

    _MI_AI_DevInfo_t* pstAiDevInfo = (_MI_AI_DevInfo_t*)data;
    MHAL_AUDIO_DEV AinDevId = pstAiDevInfo->AiDevId;
    MI_SYS_DRV_HANDLE hDevSysHandle = pstAiDevInfo->hDevSysHandle;

    //
    MI_U32 u32BitWidthByte;
    MI_S32 s32ReadActualSize;
    MI_U32 u32TempBufSize;
    MI_S32 s32ChnIdx;
    MI_SYS_BufInfo_t *pstOutputBufInfo[MI_AUDIO_MAX_CHN_NUM];
    MI_SYS_BufConf_t stBuf_config;

    MI_AUDIO_BitWidth_e eBitWidth = pstAiDevInfo->stDevAttr.eBitwidth;
    MI_U32 u32SamplesPerFrm = pstAiDevInfo->stDevAttr.u32PtNumPerFrm;
    MI_U32 u32ChnCnt = pstAiDevInfo->stDevAttr.u32ChnCnt;
    MI_AI_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, eBitWidth);
    u32TempBufSize = u32ChnCnt * u32BitWidthByte * u32SamplesPerFrm;
    pstAiDevInfo->pu8TempBufAddr = kmalloc(u32TempBufSize, GFP_KERNEL);

    // 1.0 Start AI Device HW
    MHAL_AUDIO_StartPcmIn(AinDevId);

    while(!MI_AI_ThreadShouldStop())
    {
        // 1.1 read data by audio drvier
        void* pRdBuffer = (void*) pstAiDevInfo->pu8TempBufAddr;
        //s32ReadActualSize = MHAL_AUDIO_ReadDataIn(AinDevId, pRdBuffer, u32TempBufSize, FALSE);
        s32ReadActualSize = MHAL_AUDIO_ReadDataIn(AinDevId, pRdBuffer, u32TempBufSize, TRUE);
        if(s32ReadActualSize < 0)
            DBG_ERR("s32ReadActualSize is %d \n", s32ReadActualSize);

        if(s32ReadActualSize <= 0)
            continue;

        // 1.2 check if output port buffer of AI device from MI_SYS ?
        stBuf_config.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBuf_config.u64TargetPts = 100; // TODO, ??? hardware, sample rate information
        stBuf_config.stRawCfg.u32Size = s32ReadActualSize / u32ChnCnt;

        stBuf_config.u32Flags = MI_SYS_MAP_VA;
        DBG_INFO("Output port buffer size:%d \n", stBuf_config.stRawCfg.u32Size);

        for(s32ChnIdx=0; s32ChnIdx < MI_AUDIO_MAX_CHN_NUM; s32ChnIdx++)
        {
            if( TRUE == pstAiDevInfo->astChanInfo[s32ChnIdx].bChanEnable)
            {
                //s32ChnId = pstAiDevInfo->stChanInfo[s32ChnIdx].s32ChnId;
                pstOutputBufInfo[s32ChnIdx] = mi_sys_GetOutputPortBuf(hDevSysHandle, s32ChnIdx, 0, &stBuf_config, FALSE);
                // error handle
                if(NULL == pstOutputBufInfo[s32ChnIdx])
                {
                    DBG_WRN("Get Output port buffer of Channel %d fail \n", s32ChnIdx);
                }
                else
                {
                    DBG_INFO("Get Output port buffer of Channel %d Success \n", s32ChnIdx);
                    _MI_AI_DeinterleaveData(pstOutputBufInfo[s32ChnIdx], pstAiDevInfo, s32ChnIdx);
                    // 1.3 return output buffer to MI_SYS
                    mi_sys_FinishBuf(pstOutputBufInfo[s32ChnIdx]);
                }

            }

        }

    }

   kfree(pstAiDevInfo->pu8TempBufAddr);

    return 0;
}

MI_S32 _MI_AI_Init()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId;

    for(AiDevId = 0; AiDevId<MI_AI_DEV_NUM_MAX ;AiDevId++)
        s32Ret = _MI_AI_CreateDevice(AiDevId);

    // init Audio HW
    s32Ret = MHAL_AUDIO_Init();

    if( MI_SUCCESS == s32Ret)
        DBG_INFO("MI_AI init success. \n");
    else
        DBG_INFO("MI_AI init fail. \n");

    return s32Ret;
}

MI_S32 _MI_AI_DeInit()
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_DEV AiDevId;

    for(AiDevId = 0; AiDevId<MI_AI_DEV_NUM_MAX; AiDevId++)
        s32Ret = _MI_AI_DestroyDevice(AiDevId);

    if( MI_SUCCESS == s32Ret)
        DBG_INFO("MI_AI Deinit success. \n");
    else
        DBG_INFO("MI_AI Deinit fail. \n");

    return s32Ret;
}

//=============================================================================
// Global function definition
//=============================================================================


//------------------------------------------------------------------------------
/// @brief set attribute of AI device
/// @param[in] AiDevId: AI device ID.
/// @param[in] pstAttr: Attribute of AI device.
/// @return MI_SUCCESS: succeed in setting attribute of AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_ILLEGAL_PARAM:    invalid input patamter.
///             MI_AI_ERR_NULL_PTR:         NULL point error
///             MI_AI_ERR_NOT_PERM:         not permit
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_SetPubAttr(MI_AUDIO_DEV AiDevId, MI_AUDIO_Attr_t *pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;

    //check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_POINTER(pstAttr);

    //check if AI device is disable ?
    if(FALSE != _gastAiDevInfo[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_PERM;

    //check if input parameter is legal ?
    MI_AI_CHECK_SAMPLERATE(pstAttr->eSamplerate);

    if( (pstAttr->u32ChnCnt > 16) || (pstAttr->eSoundmode >= E_MI_AUDIO_SOUND_MODE_MAX)
        || (pstAttr->eBitwidth >= E_MI_AUDIO_BIT_WIDTH_MAX)
        || (pstAttr->eWorkmode >= E_MI_AUDIO_MODE_MAX) )
    {
        DBG_ERR("Attribute of AI is illegal. \n");
        return MI_AI_ERR_ILLEGAL_PARAM;
    }

    if( ((pstAttr->u32ChnCnt == 1) && (pstAttr->eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO)) )
    {
        DBG_ERR("Error channel configure of AI. \n");
        return MI_AI_ERR_ILLEGAL_PARAM;
    }

    _gastAiDevInfo[AiDevId].AiDevId = AiDevId;
    _gastAiDevInfo[AiDevId].bDevAttrSet = TRUE;

    // save attribute of AI device
    memcpy(&_gastAiDevInfo[AiDevId].stDevAttr, pstAttr, sizeof(MI_AUDIO_Attr_t));

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief get attribute of AI device
/// @param[in]  AiDevId: AI device ID.
/// @param[out] pstAttr: Attribute of AI device.
/// @return MI_SUCCESS: succeed in setting attribute of AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_NULL_PTR:         NULL point error
///             MI_AI_ERR_NOT_CONFIG:       AI device not configure
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_GetPubAttr(MI_AUDIO_DEV AiDevId, MI_AUDIO_Attr_t *pstAttr)
{
    MI_S32 s32Ret = MI_SUCCESS;

    ///check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_POINTER(pstAttr);

    // check if AI device attr is set ?
    if(TRUE != _gastAiDevInfo[AiDevId].bDevAttrSet)
        return MI_AI_ERR_NOT_CONFIG;

    memcpy(pstAttr, &_gastAiDevInfo[AiDevId].stDevAttr, sizeof(MI_AUDIO_Attr_t));

    return s32Ret;
}


//------------------------------------------------------------------------------
/// @brief enable AI device
/// @param[in]  AiDevId: AI device ID.
/// @return MI_SUCCESS: succeed in enabling AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_NOT_CONFIG:       AI device not configure
///             MI_AI_ERR_NOT_ENABLED:      AI device not enable
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_Enable(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;

    MI_U32 u32BitWidthByte;
    MHAL_AUDIO_PcmCfg_t stDmaConfig;
    MHAL_AUDIO_I2sCfg_t stI2sConfig;
    MHAL_AUDIO_DEV AinDevId;

    ///check input parameter
    MI_AI_CHECK_DEV(AiDevId);

    // check if attribute of AI device is set ?
    if(TRUE != _gastAiDevInfo[AiDevId].bDevAttrSet)
        return MI_AI_ERR_NOT_CONFIG;

    // check if AI device is enable ?
    if(TRUE == _gastAiDevInfo[AiDevId].bDevEnable)
        return MI_SUCCESS;

    _gastAiDevInfo[AiDevId].bDevEnable = TRUE;

    // 1.0 write AI device HW configure
    memset(&stDmaConfig, 0, sizeof(MHAL_AUDIO_PcmCfg_t));

    AinDevId = (MHAL_AUDIO_DEV)AiDevId;
    stDmaConfig.eWidth = (MHAL_AUDIO_BitWidth_e)_gastAiDevInfo[AiDevId].stDevAttr.eBitwidth;
    stDmaConfig.eRate = (MHAL_AUDIO_Rate_e)_gastAiDevInfo[AiDevId].stDevAttr.eSamplerate;
    stDmaConfig.u16Channels = _gastAiDevInfo[AiDevId].stDevAttr.u32ChnCnt;
    //  limit: 16 ch must non Interleaved
    //  <=8 ch can Interleaved
    // for k6l and k6, it should be TRUE
    _gastAiDevInfo[AiDevId].bInterleaved = TRUE;
    //_gastAiDevInfo[AiDevId].bInterleaved = FALSE;
    stDmaConfig.bInterleaved = _gastAiDevInfo[AiDevId].bInterleaved;

    MI_AI_TRANS_BWIDTH_TO_BYTE(u32BitWidthByte, stDmaConfig.eWidth);

    // ToDO: need to check buffer size
    stDmaConfig.u32PeriodSize = _gastAiDevInfo[AiDevId].stDevAttr.u32PtNumPerFrm * u32BitWidthByte * stDmaConfig.u16Channels ;
    stDmaConfig.phyDmaAddr = _gastAiDevInfo[AiDevId].u64PhyBufAddr;
    stDmaConfig.u32BufferSize = MI_AI_PCM_BUF_SIZE_BYTE ;
    //stDmaConfig.u32StartThres = MI_AI_START_WRTIE_THRESHOLD;
    // virual address:
    stDmaConfig.pu8DmaArea = (MS_U8 *)_gastAiDevInfo[AiDevId].pVirBufAddr;
    s32Ret = MHAL_AUDIO_ConfigPcmIn(AinDevId, &stDmaConfig);

    // 1.1 write AI device I2S HW configure
    memset(&stI2sConfig, 0, sizeof(MHAL_AUDIO_I2sCfg_t));

    stI2sConfig.eMode =  (MHAL_AUDIO_I2sMode_e) _gastAiDevInfo[AiDevId].stDevAttr.eWorkmode;
    stI2sConfig.eWidth = (MI_AUDIO_BitWidth_e)_gastAiDevInfo[AiDevId].stDevAttr.eBitwidth;
    stI2sConfig.eFmt = E_MHAL_AUDIO_I2S_FMT_LEFT_JUSTIFY; // ??? how to decide
    stI2sConfig.u16Channels = _gastAiDevInfo[AiDevId].stDevAttr.u32ChnCnt;

    s32Ret = MHAL_AUDIO_ConfigI2sIn(AinDevId, &stI2sConfig);
    s32Ret = MHAL_AUDIO_OpenPcmIn(AinDevId);

    // 2.0 Create Read data thread
    //_gastAiDevInfo[AiDevId].pstAiReadDataThread = kthread_create(_MI_AI_ReadDataThread, &_gastAiDevInfo[AiDevId], "MIAiReadDataThread");
    //wake_up_process( _gastAiDevInfo[AiDevId].pstAiReadDataThread);
#if !USE_CAM_OS
    _gastAiDevInfo[AiDevId].pstAiReadDataThread = kthread_run(_MI_AI_ReadDataThread, &_gastAiDevInfo[AiDevId], "MIAiReadDataThread");

    if (IS_ERR(_gastAiDevInfo[AiDevId].pstAiReadDataThread))
    {
        kthread_stop(_gastAiDevInfo[AiDevId].pstAiReadDataThread);
        s32Ret = MI_AI_ERR_BUSY;
    }

    DBG_INFO("MI_AI Cerate Thread success. \n");
#else
         s32Ret = MI_AI_CreateThread(_gastAiDevInfo[AiDevId].pstAiReadDataThread, "MIAiReadDataThread", _MI_AI_ReadDataThread, &_gastAiDevInfo[AiDevId]);
         if(s32Ret != MI_SUCCESS)
            return MI_AI_ERR_BUSY;

    DBG_INFO("MI_AI Cerate Thread success. \n");
#endif

    DBG_INFO("MI_AI IMPL Enable success. \n");

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief disable AI device
/// @param[in]  AiDevId: AI device ID.
/// @return MI_SUCCESS: succeed in disabling AI device .
///             MI_AI_ERR_INVALID_DEVID: invalid AI device ID
///             MI_AI_ERR_NOT_PERM: not permit
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_Disable(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MHAL_AUDIO_DEV AinDevId = (MHAL_AUDIO_DEV) AiDevId;
    MI_S32 s32ChanlIdx;

    ///check input parameter
    MI_AI_CHECK_DEV(AiDevId);

    // check if channels of AI device are all disable ?
    for(s32ChanlIdx = 0; s32ChanlIdx<MI_AI_CHAN_NUM_MAX; s32ChanlIdx++)
    {
        if( TRUE == _gastAiDevInfo[AiDevId].astChanInfo[s32ChanlIdx].bChanEnable)
            return MI_AI_ERR_BUSY ;
    }

    _gastAiDevInfo[AiDevId].bDevEnable = FALSE;

    // 1.0 according AI device ID， stop _MI_AI_ReadDataThread
    MI_AI_ThreadStop(_gastAiDevInfo[AiDevId].pstAiReadDataThread);

    // 1.1 Stop PCM Out;
    s32Ret = MHAL_AUDIO_StopPcmIn(AinDevId);
    s32Ret = MHAL_AUDIO_ClosePcmIn(AinDevId);

    DBG_INFO("MI_AI IMPL Disbale success. \n");

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief enable AI device channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:   AI device Channel.
/// @return MI_SUCCESS: succeed in enabling AI device channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_NOT_ENABLED:      AI device not enable
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_EnableChn(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    ///check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);

    // 1. chehck if AI deivec ID is enable ?
    if(TRUE != _gastAiDevInfo[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_ENABLED;

    // 2. enable channel of AI device
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bChanEnable = TRUE;
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bPortEnable = TRUE;
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].s32ChnId = AiChn;
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].s32OutputPortId = 0;

    mi_sys_EnableChannel(_gastAiDevInfo[AiDevId].hDevSysHandle, AiChn);
    mi_sys_EnableOutputPort(_gastAiDevInfo[AiDevId].hDevSysHandle, AiChn, 0);

    DBG_INFO("MI_AI Enable chn success. \n");

    return s32Ret;
}


//------------------------------------------------------------------------------
/// @brief disabel AI device channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:   AI device Channel.
/// @return MI_SUCCESS: succeed in enabling AI device channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_DisableChn(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    ///check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);

    // 1. disble channel of AI device
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bChanEnable = FALSE;
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bPortEnable = FALSE;

    // disable channel & output port
    mi_sys_DisableChannel(_gastAiDevInfo[AiDevId].hDevSysHandle, AiChn);
    mi_sys_DisableOutputPort(_gastAiDevInfo[AiDevId].hDevSysHandle, AiChn , 0);

    DBG_INFO("MI_AI Disable chn success. \n");

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Set channel parameter of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @param[in]  pstChnParam: channel parameter.
/// @return MI_SUCCESS: succeed in clearing attribute of AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_NULL_PTR:         NULL point error
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_SetChnParam(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AI_ChnParam_t *pstChnParam)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);
    MI_AI_CHECK_POINTER(pstChnParam);

    // set channel parameter of AI channel
    memcpy(&_gastAiDevInfo[AiDevId].astChanInfo[AiChn].stChnParam, pstChnParam, sizeof(MI_AI_ChnParam_t));

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Set channel parameter of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @param[out] pstChnParam: channel parameter.
/// @return MI_SUCCESS: succeed in clearing attribute of AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_NULL_PTR:         NULL point error
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_GetChnParam(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AI_ChnParam_t *pstChnParam)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);
    MI_AI_CHECK_POINTER(pstChnParam);

    // get channel parameter of AI channel
    memcpy(pstChnParam, &_gastAiDevInfo[AiDevId].astChanInfo[AiChn].stChnParam, sizeof(MI_AI_ChnParam_t));

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Enable resample of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @param[in]  eOutSampleRate: out sample rate of resample
/// @return MI_SUCCESS: succeed in enabling resample of AI channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_ILLEGAL_PARAM:    illegal eOutSampleRate
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_EnableReSmp(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_SampleRate_e eOutSampleRate)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);
    MI_AI_CHECK_SAMPLERATE(eOutSampleRate);

    if(TRUE != _gastAiDevInfo[AiDevId].bDevEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE != _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    // set resample parameter
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bResampleEnable = TRUE;
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].eOutResampleRate = eOutSampleRate;

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Disable resample of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @return MI_SUCCESS: succeed in disableing resample of AI channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_ILLEGAL_PARAM:    illegal eOutSampleRate
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_DisableReSmp(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);

    // set resample parameter
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bResampleEnable = FALSE;

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Set Vqe Attribute of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @param[in]  AoDevId: AO device ID.
/// @param[in]  AoChn:  AO device Channel.
/// @param[in] pstVqeConfig: Attribute of AI device.
/// @return MI_SUCCESS: succeed in setting Vqe attribute of AI channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
///             MI_AI_ERR_NULL_PTR:         NULL point error
///             MI_AI_ERR_NOT_PERM:         not permit
///             MI_AI_ERR_NOT_ENABLED:      AI device not enable
//------------------------------------------------------------------------------

MI_S32 MI_AI_IMPL_SetVqeAttr(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn, MI_AUDIO_DEV AoDevId, MI_AO_CHN AoChn, MI_AI_VqeConfig_t *pstVqeConfig)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);
    MI_AI_CHECK_POINTER(pstVqeConfig);
    // ToDo: check AO device ID/Channel ID

    // check if Vqe of AO device channel is disable ?
    if( FALSE != _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return MI_AI_ERR_NOT_PERM;

    // check if AO device channel is enable ?
    if( TRUE != _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    // save Vqe configure of AI device channel
    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeAttrSet = TRUE;
    memcpy(&_gastAiDevInfo[AiDevId].astChanInfo[AiChn].stAiVqeConfig, pstVqeConfig, sizeof(MI_AI_VqeConfig_t));

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Enable Vqe of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @return MI_SUCCESS: succeed in enabling Vqe of AI channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_EnableVqe(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);

    //
    if( TRUE != _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeAttrSet)
        return MI_AI_ERR_NOT_PERM;

    if( TRUE != _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bChanEnable)
        return MI_AI_ERR_NOT_ENABLED;

    if(TRUE == _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return s32Ret;


    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeEnable = TRUE;

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Disable Vqe of AI channel
/// @param[in]  AiDevId: AI device ID.
/// @param[in]  AiChn:  AI device Channel.
/// @return MI_SUCCESS: succeed in disabling Vqe of AI channel .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_INVALID_CHNID:    invalid AI device channel
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_DisableVqe(MI_AUDIO_DEV AiDevId, MI_AI_CHN AiChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

     // check input parameter
    MI_AI_CHECK_DEV(AiDevId);
    MI_AI_CHECK_CHN(AiChn);

    if(FALSE == _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeEnable)
        return s32Ret;

    _gastAiDevInfo[AiDevId].astChanInfo[AiChn].bVqeEnable = FALSE;

    return s32Ret;
}

//------------------------------------------------------------------------------
/// @brief Clear attribute of AI device
/// @param[in] AiDevId: AI device ID.
/// @return MI_SUCCESS: succeed in clearing attribute of AI device .
///             MI_AI_ERR_INVALID_DEVID:    invalid AI device ID
///             MI_AI_ERR_NOT_PERM:         not permit
//------------------------------------------------------------------------------
MI_S32 MI_AI_IMPL_ClrPubAttr(MI_AUDIO_DEV AiDevId)
{
    MI_S32 s32Ret = MI_SUCCESS;

    // check input parameter
    MI_AI_CHECK_DEV(AiDevId);

    // chehck if AI deivec ID is disable ?
    if(FALSE != _gastAiDevInfo[AiDevId].bDevEnable)
        return MI_AI_ERR_BUSY;

    _gastAiDevInfo[AiDevId].bDevAttrSet = FALSE;
    memset(&_gastAiDevInfo[AiDevId].stDevAttr, 0, sizeof(MI_AUDIO_Attr_t));

    return s32Ret;
}
