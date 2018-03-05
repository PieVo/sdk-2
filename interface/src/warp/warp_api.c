
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
/// @file   wrap_api.c
/// @brief wrap module api
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __KERNEL__
#include <pthread.h>
#endif

#include "mi_print.h"
#include "mi_syscall.h"
#include "mi_sys.h"
#include "mi_warp.h"
#include "warp_ioctl.h"

MI_MODULE_DEFINE(wrap);


#define MI_WARP_DEBUG_INFO  1

#if defined(MI_WARP_DEBUG_INFO)&&(MI_WARP_DEBUG_INFO==1)
#define MI_WARP_FUNC_ENTRY()        if(1){DBG_INFO("%s Function In\n", __func__);}
#define MI_WARP_FUNC_EXIT()         if(1){DBG_INFO("%s Function Exit\n", __func__);}
#define MI_WARP_FUNC_ENTRY2(ViChn)  if(1){DBG_INFO("%s Function In:chn=%d\n", __func__,ViChn);}
#define MI_WARP_FUNC_EXIT2(ViChn)   if(1){DBG_INFO("%s Function Exit:chn=%d\n", __func__, ViChn);}
#else
#define MI_WARP_FUNC_ENTRY()
#define MI_WARP_FUNC_EXIT()
#define MI_WARP_FUNC_ENTRY2(ViChn)
#define MI_WARP_FUNC_EXIT2(ViChn)
#endif



#define MI_WARP_CHECK_DEVID_VALID(id) do {  \
                        if (id >= MI_WARP_MAX_DEVICE_NUM) \
                        { \
                           return MI_ERR_WARP_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_WARP_CHECK_CHNID_VALID(chn) do {  \
                        if (chn >= MI_WARP_MAX_CHN_NUM) \
                        { \
                           return MI_ERR_WARP_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#ifndef __KERNEL__

typedef struct
{
    MI_U32 u32ChnId;
    MI_BOOL bInited;
    MI_SYS_ChnPort_t stChnPort;     // 1 input 1 output
    MI_PHY phyConfAddr;             // chn confbuf pyh addr
    MI_VIRT virConfAddr;            // chn confbuf virt addr
    pthread_mutex_t mtxConf;        // mutex of chn config
}mi_warp_channel_t;

typedef struct
{
    MI_U32 u32DevId;
    mi_warp_channel_t stChannel[MI_WARP_MAX_CHN_NUM];
}mi_warp_device_t;

typedef struct
{
    mi_warp_device_t astDev[MI_WARP_MAX_DEVICE_NUM];
}mi_warp_module_t;

static mi_warp_module_t _stWarpModule;

MI_S32 _MI_WARP_AllocInstanceBuf(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    void * pAddr = NULL;
    MI_S32 s32Ret;

    MI_WARP_FUNC_ENTRY();
    MI_WARP_CHECK_DEVID_VALID(devId);
    MI_WARP_CHECK_CHNID_VALID(chnId);

    if (_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr)
    {
        if (_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr)
        {
            DBG_INFO("Dev%d_Chn%d instanceBuf has been alloced, phyAddr[%llu], usrAddr[%u]\n", devId, chnId
                    , _stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr
                    , _stWarpModule.astDev[devId].stChannel[chnId].virConfAddr);
            return MI_WARP_OK;
        }

        goto MMAP_TO_USRADDR;
    }
    else
    {
        MI_U8 szHeapName[32];
        memset(szHeapName, 0, sizeof(szHeapName));
        sprintf(szHeapName, "Dev%d_Chn%d_buf", devId, chnId);

        //s32Ret = MI_SYS_MMA_Alloc(szHeapName, sizeof(MI_CV_Config_t), &_stWarpModule.astDev[u32DevId].stChannel[u32ChnId].phyConfAddr);
        s32Ret = MI_SYS_MMA_Alloc("mma_heap_name0", sizeof(MI_WARP_Config_t), &_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr);
        if (MI_SUCCESS != s32Ret)
        {
            DBG_INFO("Dev%d_Chn%d instanceBuf alloc failed\n", devId, chnId);
            return MI_ERR_WARP_FAIL;
        }

        goto MMAP_TO_USRADDR;
    }

MMAP_TO_USRADDR:
    s32Ret = MI_SYS_Mmap(_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr, sizeof(MI_WARP_Config_t), &pAddr, FALSE);

    if (MI_SUCCESS != s32Ret || !pAddr)
    {
        DBG_INFO("Dev%d_Chn%d instanceBuf map failed\n", devId, chnId);
        return MI_ERR_WARP_FAIL;
    }

    _stWarpModule.astDev[devId].stChannel[chnId].virConfAddr = (MI_VIRT)pAddr;

    MI_WARP_FUNC_EXIT();
    return MI_WARP_OK;
}

MI_S32 _MI_WARP_FreeInstanceBuf(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_S32 s32Ret;

    MI_WARP_FUNC_ENTRY();
    MI_WARP_CHECK_DEVID_VALID(devId);
    MI_WARP_CHECK_CHNID_VALID(chnId);

    if (_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr)
    {
        s32Ret = MI_SYS_Munmap((void*)(_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr), sizeof(MI_WARP_Config_t));
        if (MI_SUCCESS != s32Ret)
        {
            DBG_INFO("Dev%d_Chn%d instanceBuf unmap failed\n", devId, chnId);
            return MI_ERR_WARP_FAIL;
        }
    }
    _stWarpModule.astDev[devId].stChannel[chnId].virConfAddr = 0;

    if (_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr)
    {
        s32Ret = MI_SYS_MMA_Free(_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr);
        if (MI_SUCCESS != s32Ret)
        {
            DBG_INFO("Dev%d_Chn%d instanceBuf free failed\n", devId, chnId);
            return MI_ERR_WARP_FAIL;
        }
    }
    _stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr = 0;

    MI_WARP_FUNC_EXIT();
    return MI_WARP_OK;
}
#endif

MI_S32 _MI_WARP_CreateChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_PHY phyAddr)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_CreateChannel_t stCreateChn;
    stCreateChn.devId = devId;
    stCreateChn.chnId = chnId;
    stCreateChn.phyAddr = phyAddr;
    s32Ret = MI_SYSCALL(MI_WARP_CREATE_CHANNEL, &stCreateChn);

    return s32Ret;
}

MI_S32 _MI_WARP_DestroyChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_DestroyChannel_t stDestroyChn;
    stDestroyChn.devId = devId;
    stDestroyChn.chnId = chnId;
    s32Ret = MI_SYSCALL(MI_WARP_DESTROY_CHANNEL, &stDestroyChn);
    return s32Ret;
}

MI_S32 _MI_WARP_EnableChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_EnableChannel_t stChnStatus;
    memset(&stChnStatus, 0x0, sizeof(MI_WARP_EnableChannel_t));
    stChnStatus.devId = devId;
    stChnStatus.chnId = chnId;
    s32Ret = MI_SYSCALL(MI_WARP_ENABLECHANNEL, &stChnStatus);
    return s32Ret;
}

MI_S32 _MI_WARP_DisableChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_EnableChannel_t stChnStatus;
    memset(&stChnStatus, 0x0, sizeof(MI_WARP_EnableChannel_t));
    stChnStatus.devId = devId;
    stChnStatus.chnId = chnId;
    s32Ret = MI_SYSCALL(MI_WARP_DISABLECHANNEL, &stChnStatus);
    return s32Ret;
}

MI_S32 _MI_WARP_EnableInputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_EnableInputPort_t stEnableInputPort;

    memset(&stEnableInputPort, 0, sizeof(stEnableInputPort));
    stEnableInputPort.devId = devId;
    stEnableInputPort.chnId = chnId;
    stEnableInputPort.portId = portId;
    s32Ret = MI_SYSCALL(MI_WARP_ENABLEINPUTPORT, &stEnableInputPort);

    return s32Ret;
}

MI_S32 _MI_WARP_DisableInputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_DisableInputPort_t stDisableInputPort;

    memset(&stDisableInputPort, 0, sizeof(stDisableInputPort));
    stDisableInputPort.devId = devId;
    stDisableInputPort.chnId = chnId;
    stDisableInputPort.portId = portId;
    s32Ret = MI_SYSCALL(MI_WARP_DISABLEINPUTPORT, &stDisableInputPort);

    return s32Ret;
}

MI_S32 _MI_WARP_EnableOutputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_EnableOutputPort_t stEnableOutputPort;

    memset(&stEnableOutputPort, 0, sizeof(stEnableOutputPort));
    stEnableOutputPort.devId = devId;
    stEnableOutputPort.chnId = chnId;
    stEnableOutputPort.portId = portId;
    s32Ret = MI_SYSCALL(MI_WARP_ENABLEOUTPUTPORT, &stEnableOutputPort);

    return s32Ret;
}

MI_S32 _MI_WARP_DisableOutputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_DisableOutputPort_t stDisableOutputPort;

    memset(&stDisableOutputPort, 0, sizeof(stDisableOutputPort));
    stDisableOutputPort.devId = devId;
    stDisableOutputPort.chnId = chnId;
    stDisableOutputPort.portId = portId;
    s32Ret = MI_SYSCALL(MI_WARP_DISABLEOUTPUTPORT, &stDisableOutputPort);

    return s32Ret;
}


MI_S32 MI_WARP_CreateDevice(MI_WARP_DEV devId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    s32Ret = MI_SYSCALL(MI_WARP_CREATE_DEV, &devId);

    return s32Ret;
}

MI_S32 MI_WARP_DestroyDevice(MI_WARP_DEV devId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    s32Ret = MI_SYSCALL(MI_WARP_DESTROY_DEV, &devId);
    return s32Ret;
}

MI_S32 MI_WARP_StartDev(MI_WARP_DEV devId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    s32Ret = MI_SYSCALL(MI_WARP_STARTDEV, &devId);
    return s32Ret;
}

MI_S32 MI_WARP_StopDev(MI_WARP_DEV devId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    s32Ret = MI_SYSCALL(MI_WARP_STOPDEV, &devId);
    return s32Ret;
}


MI_S32 MI_WARP_CreateChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
#ifndef __KERNEL__
    _stWarpModule.astDev[devId].stChannel[chnId].stChnPort.u32ChnId = chnId;
    pthread_mutex_lock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);

    // create channel config buf
    DBG_INFO("%s %d: AllocInstanceBuf\n", __FUNCTION__, __LINE__);
    if (MI_WARP_OK != _MI_WARP_AllocInstanceBuf(devId, chnId))
    {
        DBG_INFO("Dev%d_Chn%d create instanceBuf failed\n");
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }

    // create channel
    DBG_INFO("%s %d: create Chn\n", __FUNCTION__, __LINE__);
    if (MI_WARP_OK != _MI_WARP_CreateChannel(devId, chnId, _stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr))
    {
        DBG_INFO("Dev%d Create chn%d failed\n");
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }
    pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);

    // enable chn & input/output port,  (1 in, 1 out)
    DBG_INFO("%s %d: enable Chn\n", __FUNCTION__, __LINE__);
    _MI_WARP_EnableChannel(devId, chnId);
    DBG_INFO("%s %d: enable inputport\n", __FUNCTION__, __LINE__);
    _MI_WARP_EnableInputPort(devId, chnId, 0);
    DBG_INFO("%s %d: enable outputport\n", __FUNCTION__, __LINE__);
    _MI_WARP_EnableOutputPort(devId, chnId, 0);

    return MI_WARP_OK;
#else
    return MI_WARP_OK;
#endif
}

MI_S32 MI_WARP_DestroyChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
#ifndef __KERNEL__
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    MI_WARP_FUNC_ENTRY();
    MI_WARP_CHECK_DEVID_VALID(devId);
    MI_WARP_CHECK_CHNID_VALID(chnId);

    // disable chn & input/output port
    _MI_WARP_DisableOutputPort(devId, chnId, 0);
    _MI_WARP_DisableInputPort(devId, chnId, 0);
    _MI_WARP_DisableChannel(devId, chnId);

    pthread_mutex_lock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
    // destroy channel
    if (MI_WARP_OK != MI_WARP_DestroyChannel(devId, chnId))
    {
        DBG_INFO("Dev%d destroy chn%d failed\n");
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }

    // destroy channel config buf
    if (MI_WARP_OK != _MI_WARP_FreeInstanceBuf(devId, chnId))
    {
        DBG_INFO("Dev%d_Chn%d free instanceBuf failed\n");
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }
    pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);

    MI_WARP_FUNC_EXIT();
    return MI_WARP_OK;
#else
    return MI_WARP_OK;
#endif
}

MI_S32 MI_WARP_TriggerChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_TriggerChannel_t stTriggerChn;

    MI_WARP_FUNC_ENTRY();
    stTriggerChn.devId = devId;
    stTriggerChn.chnId = chnId;
    s32Ret = MI_SYSCALL(MI_WARP_TRIGGER_CHANNEL, &stTriggerChn);

    MI_WARP_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_WARP_SetInputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_InputPortAttr_t *pstInputPortAttr)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_SetInputPortAttr_t stSetInputPortAttr;

    MI_WARP_FUNC_ENTRY();
    memset(&stSetInputPortAttr, 0, sizeof(stSetInputPortAttr));
    stSetInputPortAttr.devId = devId;
    stSetInputPortAttr.chnId = chnId;
    stSetInputPortAttr.portId = portId;
    stSetInputPortAttr.stInputPortAttr = *pstInputPortAttr;
    s32Ret = MI_SYSCALL(MI_WARP_SETINPUTPORTATTR, &stSetInputPortAttr);

    MI_WARP_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_WARP_GetInputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_InputPortAttr_t *pstInputPortAttr)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_GetInputPortAttr_t stGetInputPortAttr;

    MI_WARP_FUNC_ENTRY();
    memset(&stGetInputPortAttr, 0, sizeof(stGetInputPortAttr));
    stGetInputPortAttr.devId = devId;
    stGetInputPortAttr.chnId = chnId;
    stGetInputPortAttr.portId = portId;
    s32Ret = MI_SYSCALL(MI_WARP_GETINPUTPORTATTR, &stGetInputPortAttr);
    if (MI_WARP_OK == s32Ret)
    {
        memcpy(pstInputPortAttr, &stGetInputPortAttr.stInputPortAttr, sizeof(MI_WARP_InputPortAttr_t));
    }

    MI_WARP_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_WARP_SetOutputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_SetOutputPortAttr_t stSetOutputPortAttr;

    MI_WARP_FUNC_ENTRY();
    memset(&stSetOutputPortAttr, 0, sizeof(stSetOutputPortAttr));
    stSetOutputPortAttr.devId = devId;
    stSetOutputPortAttr.chnId = chnId;
    stSetOutputPortAttr.portId = portId;
    stSetOutputPortAttr.stOutputPortAttr = *pstOutputPortAttr;
    s32Ret = MI_SYSCALL(MI_WARP_SETOUTPUTPORTATTR, &stSetOutputPortAttr);

    MI_WARP_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_WARP_GetOutputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_GetOutputPortAttr_t stGetOutputPortAttr;

    MI_WARP_FUNC_ENTRY();
    memset(&stGetOutputPortAttr, 0, sizeof(stGetOutputPortAttr));
    stGetOutputPortAttr.devId = devId;
    stGetOutputPortAttr.chnId = chnId;
    stGetOutputPortAttr.portId = portId;
    //stGetOutputPortAttr.stOutputPortAttr = *pstOutputPortAttr;
    s32Ret = MI_SYSCALL(MI_WARP_GETOUTPUTPORTATTR, &stGetOutputPortAttr);

    if (MI_WARP_OK == s32Ret)
    {
        memcpy(pstOutputPortAttr, &stGetOutputPortAttr.stOutputPortAttr, sizeof(MI_WARP_OutputPortAttr_t));
    }

    MI_WARP_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_WARP_SetChnConfig(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_Config_t *pstConfig)
{
#ifndef __KERNEL__
    MI_WARP_FUNC_ENTRY();
    pthread_mutex_lock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
    if ( (!_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr)
        || (!_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr))
    {
        printf("Dev%d_Chn%d instanceBuf has not been created\n", devId, chnId);
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }

    memcpy((MI_WARP_Config_t*)_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr, pstConfig, sizeof(MI_WARP_Config_t));
    pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);

    MI_WARP_FUNC_EXIT();
#else
    return MI_WARP_OK;
#endif
}

MI_S32 MI_WARP_GetChnConfig(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_Config_t *pstConfig)
{
#ifndef __KERNEL__
    MI_WARP_FUNC_ENTRY();
    pthread_mutex_lock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
    if (!_stWarpModule.astDev[devId].stChannel[chnId].phyConfAddr || !_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr)
    {
        printf("Dev%d_Chn%d instanceBuf has not been created\n");
        pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);
        return MI_ERR_WARP_FAIL;
    }

    memcpy(pstConfig, (MI_WARP_Config_t*)_stWarpModule.astDev[devId].stChannel[chnId].virConfAddr, sizeof(MI_WARP_Config_t));
    pthread_mutex_unlock(&_stWarpModule.astDev[devId].stChannel[chnId].mtxConf);

    MI_WARP_FUNC_EXIT();
#else
    return MI_WARP_OK;
#endif
}


EXPORT_SYMBOL(MI_WARP_CreateDevice);
EXPORT_SYMBOL(MI_WARP_DestroyDevice);
EXPORT_SYMBOL(MI_WARP_StartDev);
EXPORT_SYMBOL(MI_WARP_StopDev);

EXPORT_SYMBOL(MI_WARP_CreateChannel);
EXPORT_SYMBOL(MI_WARP_DestroyChannel);
EXPORT_SYMBOL(MI_WARP_TriggerChannel);

EXPORT_SYMBOL(MI_WARP_SetInputPortAttr);
EXPORT_SYMBOL(MI_WARP_GetInputPortAttr);
EXPORT_SYMBOL(MI_WARP_SetOutputPortAttr);
EXPORT_SYMBOL(MI_WARP_GetOutputPortAttr);
EXPORT_SYMBOL(MI_WARP_SetChnConfig);
EXPORT_SYMBOL(MI_WARP_GetChnConfig);







