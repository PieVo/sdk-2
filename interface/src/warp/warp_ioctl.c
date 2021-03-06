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
/// @file   vdec_ioctl.c
/// @brief vdec module ioctl
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////
#include "mi_device.h"
#include "mi_common_internal.h"
#include "mi_print.h"

#include "mi_common.h"
#include "mi_sys.h"
#include "mi_warp_impl.h"
#include "warp_ioctl.h"
#include "mi_warp_datatype.h"


//-------------------------------------------------------------------------------------------------
//  Local Defines
//-------------------------------------------------------------------------------------------------
static DEFINE_MUTEX(mutex);


//-------------------------------------------------------------------------------------------------
//  Local Macros
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Local Structures
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Local Enum
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Global Variables
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Local l Variables
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  local function  prototypes
//-------------------------------------------------------------------------------------------------

static MI_S32 MI_WARP_IOCTL_Init(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> \n", __func__);
    s32Ret = MI_WARP_IMPL_Init();

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DeInit(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_WARP_IMPL_DeInit();

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_CreateDevice(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DEV devId = *(MI_WARP_DEV*)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>DevId: %d.\n", __func__, devId);
    s32Ret = MI_WARP_IMPL_CreateDevice(devId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DestroyDevice(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DEV devId = *(MI_WARP_DEV*)ptr;
    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> devId: %d.\n", __func__, devId);
    s32Ret = MI_WARP_IMPL_DestroyDevice(devId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_StartDev(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DEV *pstStartDev = (MI_WARP_DEV *)ptr;
    MI_WARP_DEV devId = *pstStartDev;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d.\n", __func__, devId);
    s32Ret = MI_WARP_IMPL_StartDev(devId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_StopDev(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DEV *pstStartDev = (MI_WARP_DEV *)ptr;
    MI_WARP_DEV devId = *pstStartDev;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d.\n", __func__, devId);
    s32Ret = MI_WARP_IMPL_StopDev(devId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_CreateChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_CreateChannel_t stCreateChnInfo = *(MI_WARP_CreateChannel_t*)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>DevId: %d, ChnId: %d, phyAddr: %llu\n", __func__, stCreateChnInfo.devId, stCreateChnInfo.chnId,
                                                                    stCreateChnInfo.phyAddr);
    s32Ret = MI_WARP_IMPL_CreateChannel(stCreateChnInfo.devId, stCreateChnInfo.chnId, stCreateChnInfo.phyAddr);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DestroyChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DestroyChannel_t stDestroyChnInfo = *(MI_WARP_DestroyChannel_t*)ptr;
    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> devId: %d, ChnId: %d.\n", __func__, stDestroyChnInfo.devId, stDestroyChnInfo.chnId);
    s32Ret = MI_WARP_IMPL_DestroyChannel(stDestroyChnInfo.devId, stDestroyChnInfo.chnId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_TriggerChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_TriggerChannel_t stTriggerChnInfo = *(MI_WARP_TriggerChannel_t*)ptr;
    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> devId: %d, ChnId: %d.\n", __func__, stTriggerChnInfo.devId, stTriggerChnInfo.chnId);
    s32Ret = MI_WARP_IMPL_TriggerChannel(stTriggerChnInfo.devId, stTriggerChnInfo.chnId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_EnableChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_EnableChannel_t *pstEnableChannel = (MI_WARP_EnableChannel_t *)ptr;
    MI_WARP_DEV devId = pstEnableChannel->devId;
    MI_WARP_CHN chnId = pstEnableChannel->chnId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, Chn: %d.\n", __func__, devId, chnId);
    s32Ret = MI_WARP_IMPL_EnableChannel(devId, chnId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DisableChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DisableChannel_t *pstDisableChannel = (MI_WARP_DisableChannel_t *)ptr;
    MI_WARP_DEV devId = pstDisableChannel->devId;
    MI_WARP_CHN chnId = pstDisableChannel->chnId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, Chn: %d.\n", __func__, devId, chnId);
    s32Ret = MI_WARP_IMPL_DisableChannel(devId, chnId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_EnableInputPort(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_EnableInputPort_t *pstEnableInputPort = (MI_WARP_EnableInputPort_t *)ptr;
    MI_WARP_DEV devId = pstEnableInputPort->devId;
    MI_WARP_CHN chnId = pstEnableInputPort->chnId;
    MI_WARP_PORT portId = pstEnableInputPort->portId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, ChnId: %d Port: %d.\n", __func__, devId, chnId, portId);
    s32Ret = MI_WARP_IMPL_EnableInputPort(devId, chnId, portId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DisableInputPort(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DisableInputPort_t *pstDisableInputPort = (MI_WARP_DisableInputPort_t *)ptr;
    MI_WARP_DEV devId = pstDisableInputPort->devId;
    MI_WARP_CHN chnId = pstDisableInputPort->chnId;
    MI_WARP_PORT portId = pstDisableInputPort->portId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, Chn: %d Port: %d.\n", __func__, devId, chnId, portId);
    s32Ret = MI_WARP_IMPL_DisableInputPort(devId, chnId, portId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_EnableOutputPort(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_EnableOutputPort_t *pstEnableOutputPort = (MI_WARP_EnableOutputPort_t *)ptr;
    MI_WARP_DEV devId = pstEnableOutputPort->devId;
    MI_WARP_CHN chnId = pstEnableOutputPort->chnId;
    MI_WARP_PORT portId = pstEnableOutputPort->portId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, Chn: %d Port: %d.\n", __func__, devId, chnId, portId);
    s32Ret = MI_WARP_IMPL_EnableOutputPort(devId, chnId, portId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_DisableOutputPort(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_DisableOutputPort_t *pstDisableOutputPort = (MI_WARP_DisableOutputPort_t *)ptr;
    MI_WARP_DEV devId = pstDisableOutputPort->devId;
    MI_WARP_CHN chnId = pstDisableOutputPort->chnId;
    MI_WARP_PORT portId = pstDisableOutputPort->portId;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> Dev: %d, Chn: %d Port: %d.\n", __func__, devId, chnId, portId);
    s32Ret = MI_WARP_IMPL_DisableOutputPort(devId, chnId, portId);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_SetInputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_SetInputPortAttr_t *pstSetInputPortAttr = (MI_WARP_SetInputPortAttr_t *)ptr;
    MI_WARP_DEV devId = pstSetInputPortAttr->devId;
    MI_WARP_CHN chnId = pstSetInputPortAttr->chnId;
    MI_WARP_PORT portId = pstSetInputPortAttr->portId;
    MI_WARP_InputPortAttr_t *pstInputPortAttr = &pstSetInputPortAttr->stInputPortAttr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> .\n", __func__);
    s32Ret = MI_WARP_IMPL_SetInputPortAttr(devId, chnId, portId, pstInputPortAttr);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_GetInputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_GetInputPortAttr_t *pstGetInputPortAttr = (MI_WARP_GetInputPortAttr_t *)ptr;
    MI_WARP_DEV devId = pstGetInputPortAttr->devId;
    MI_WARP_CHN chnId = pstGetInputPortAttr->chnId;
    MI_WARP_PORT portId = pstGetInputPortAttr->portId;
    MI_WARP_InputPortAttr_t *pstInputPortAttr = &pstGetInputPortAttr->stInputPortAttr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> .\n", __func__);
    s32Ret = MI_WARP_IMPL_GetInputPortAttr(devId, chnId, portId, pstInputPortAttr);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_SetOutputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_SetOutputPortAttr_t *pstSetOutputPortAttr = (MI_WARP_SetOutputPortAttr_t *)ptr;
    MI_WARP_DEV devId = pstSetOutputPortAttr->devId;
    MI_WARP_CHN chnId = pstSetOutputPortAttr->chnId;
    MI_WARP_PORT portId = pstSetOutputPortAttr->portId;
    MI_WARP_OutputPortAttr_t *pstOutputPortAttr = &pstSetOutputPortAttr->stOutputPortAttr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_WARP_IMPL_SetOutputPortAttr(devId, chnId, portId, pstOutputPortAttr);

    return s32Ret;
}

static MI_S32 MI_WARP_IOCTL_GetOutputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_WARP_GetOutputPortAttr_t *pstGetOutputPortAttr = (MI_WARP_GetOutputPortAttr_t *)ptr;
    MI_WARP_DEV devId = pstGetOutputPortAttr->devId;
    MI_WARP_CHN chnId = pstGetOutputPortAttr->chnId;
    MI_WARP_PORT portId = pstGetOutputPortAttr->portId;
    MI_WARP_OutputPortAttr_t *pstOutputPortAttr = &pstGetOutputPortAttr->stOutputPortAttr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_WARP_IMPL_GetOutputPortAttr(devId, chnId, portId, pstOutputPortAttr);

    return s32Ret;
}


static MI_DEVICE_WrapperFunction_t ioctl_table[E_MI_WARP_CMD_MAX] = {
    [E_MI_WARP_INIT] = MI_WARP_IOCTL_Init,
    [E_MI_WARP_DEINIT] = MI_WARP_IOCTL_DeInit,
    [E_MI_WARP_CREATE_DEV] = MI_WARP_IOCTL_CreateDevice,
    [E_MI_WARP_DESTROY_DEV] = MI_WARP_IOCTL_DestroyDevice,
    [E_MI_WARP_STARTDEV] = MI_WARP_IOCTL_StartDev,
    [E_MI_WARP_STOPDEV] = MI_WARP_IOCTL_StopDev,
    [E_MI_WARP_CREATE_CHN] = MI_WARP_IOCTL_CreateChannel,
    [E_MI_WARP_DESTROY_CHN] = MI_WARP_IOCTL_DestroyChannel,
    [E_MI_WARP_TRIGGER_CHN] = MI_WARP_IOCTL_TriggerChannel,
    [E_MI_WARP_ENABLECHANNEL] = MI_WARP_IOCTL_EnableChannel,
    [E_MI_WARP_DISABLECHANNEL] = MI_WARP_IOCTL_DisableChannel,
    [E_MI_WARP_ENABLEINPUTPORT] = MI_WARP_IOCTL_EnableInputPort,
    [E_MI_WARP_DISABLEINPUTPORT] = MI_WARP_IOCTL_DisableInputPort,
    [E_MI_WARP_ENABLEOUTPUTPORT] = MI_WARP_IOCTL_EnableOutputPort,
    [E_MI_WARP_DISABLEOUTPUTPORT] = MI_WARP_IOCTL_DisableOutputPort,
    [E_MI_WARP_SETINPUTPORTATTR] = MI_WARP_IOCTL_SetInputPortAttr,
    [E_MI_WARP_GETINPUTPORTATTR] = MI_WARP_IOCTL_GetInputPortAttr,
    [E_MI_WARP_SETOUTPUTPORTATTR] = MI_WARP_IOCTL_SetOutputPortAttr,
    [E_MI_WARP_GETOUTPUTPORTATTR] = MI_WARP_IOCTL_GetOutputPortAttr,
};

static void mi_wrap_insmod(const char *name);
static void mi_wrap_process_init(MI_COMMON_Client_t *client);
static void mi_wrap_process_exit(MI_COMMON_Client_t *client);


MI_DEVICE_DEFINE(mi_wrap_insmod,
               ioctl_table, E_MI_WARP_CMD_MAX,
               mi_wrap_process_init, mi_wrap_process_exit);


static void mi_wrap_insmod(const char *name)
{
    MI_PRINT("module [%s] init\n", name);
}
// TODO: Tommy: IOCTRL  OPEN allow return value.
static void mi_wrap_process_init(MI_COMMON_Client_t *client)
{
    MI_PRINT("client [%d] connected\n", client->pid);
    //debug_level = MI_DBG_ALL;
    MI_WARP_IMPL_Init();
}
static void mi_wrap_process_exit(MI_COMMON_Client_t *client)
{
    MI_PRINT("client [%d] disconnected\n", client->pid);
    MI_WARP_IMPL_DeInit();
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mstar");
