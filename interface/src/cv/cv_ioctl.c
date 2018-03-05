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
#include "mi_cv_impl.h"
#include "cv_ioctl.h"
#include "mi_cv_datatype.h"


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

static MI_S32 MI_CV_IOCTL_Init(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> \n", __func__);
    s32Ret = MI_CV_IMPL_Init();

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_DeInit(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_CV_IMPL_DeInit();

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_CreateDevice(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_DEV devId = *(MI_CV_DEV*)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> DevId: %d.\n", __func__, devId);
    s32Ret = MI_CV_IMPL_CreateDevice(devId);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_DestroyDevice(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_DEV devId = *(MI_CV_DEV*)ptr;
    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> devId: %d.\n", __func__, devId);
    s32Ret = MI_CV_IMPL_DestroyDevice(devId);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_CreateChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_CreateChannel_t stCreateChnInfo = *(MI_CV_CreateChannel_t*)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>DevId: %d, ChnId: %d\n", __func__, stCreateChnInfo.devId, stCreateChnInfo.chnId);
    s32Ret = MI_CV_IMPL_CreateChannel(stCreateChnInfo.devId, stCreateChnInfo.chnId);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_DestroyChannel(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_DestroyChannel_t stDestroyChnInfo = *(MI_CV_DestroyChannel_t*)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> devId: %d, ChnId: %d.\n", __func__, stDestroyChnInfo.devId, stDestroyChnInfo.chnId);
    s32Ret = MI_CV_IMPL_DestroyChannel(stDestroyChnInfo.devId, stDestroyChnInfo.chnId);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_SetInputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_SetInputPortAttr_t *pstSetInputPortAttr = (MI_CV_SetInputPortAttr_t *)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> .\n", __func__);
    s32Ret = MI_CV_IMPL_SetInputPortAttr(pstSetInputPortAttr->devId, pstSetInputPortAttr->chnId
                           , pstSetInputPortAttr->portId, &pstSetInputPortAttr->stInputPortAttr);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_GetInputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_GetInputPortAttr_t *pstGetInputPortAttr = (MI_CV_GetInputPortAttr_t *)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s> .\n", __func__);
    s32Ret = MI_CV_IMPL_GetInputPortAttr(pstGetInputPortAttr->devId, pstGetInputPortAttr->chnId
                           , pstGetInputPortAttr->portId, &pstGetInputPortAttr->stInputPortAttr);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_SetOutputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_SetOutputPortAttr_t *pstSetOutputPortAttr = (MI_CV_SetOutputPortAttr_t *)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_CV_IMPL_SetOutputPortAttr(pstSetOutputPortAttr->devId, pstSetOutputPortAttr->chnId
                           , pstSetOutputPortAttr->portId, &pstSetOutputPortAttr->stOutputPortAttr);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_GetOutputPortAttr(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_GetOutputPortAttr_t *pstGetOutputPortAttr = (MI_CV_GetOutputPortAttr_t *)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_CV_IMPL_GetOutputPortAttr(pstGetOutputPortAttr->devId, pstGetOutputPortAttr->chnId
                           , pstGetOutputPortAttr->portId, &pstGetOutputPortAttr->stOutputPortAttr);

    return s32Ret;
}

static MI_S32 MI_CV_IOCTL_GetCdnnCfgBuf(MI_DEVICE_Context_t *env, void *ptr)
{
    MI_S32 s32Ret;
    MI_CV_GetCdnnCfgBuf_t *pstGetCdnnCfgBuf = (MI_CV_GetCdnnCfgBuf_t *)ptr;

    AUTO_LOCK(env, mutex);
    DBG_INFO("<%s>\n", __func__);
    s32Ret = MI_CV_IMPL_GetCdnnCfgBuf(pstGetCdnnCfgBuf->phyAddr, pstGetCdnnCfgBuf->u32Size);

    return s32Ret;
}



static MI_DEVICE_WrapperFunction_t ioctl_table[E_MI_CV_CMD_MAX] = {
    [E_MI_CV_INIT] = MI_CV_IOCTL_Init,
    [E_MI_CV_DEINIT] = MI_CV_IOCTL_DeInit,
    [E_MI_CV_CREATE_DEV] = MI_CV_IOCTL_CreateDevice,
    [E_MI_CV_DESTROY_DEV] = MI_CV_IOCTL_DestroyDevice,
    [E_MI_CV_CREATE_CHN] = MI_CV_IOCTL_CreateChannel,
    [E_MI_CV_DESTROY_CHN] = MI_CV_IOCTL_DestroyChannel,
    [E_MI_CV_SETINPUTPORTATTR] = MI_CV_IOCTL_SetInputPortAttr,
    [E_MI_CV_GETINPUTPORTATTR] = MI_CV_IOCTL_GetInputPortAttr,
    [E_MI_CV_SETOUTPUTPORTATTR] = MI_CV_IOCTL_SetOutputPortAttr,
    [E_MI_CV_GETOUTPUTPORTATTR] = MI_CV_IOCTL_GetOutputPortAttr,
    [E_MI_CV_GET_CDNN_CFG_BUF] = MI_CV_IOCTL_GetCdnnCfgBuf,
};

static void mi_cv_insmod(const char *name);
static void mi_cv_process_init(MI_COMMON_Client_t *client);
static void mi_cv_process_exit(MI_COMMON_Client_t *client);


MI_DEVICE_DEFINE(mi_cv_insmod,
               ioctl_table, E_MI_CV_CMD_MAX,
               mi_cv_process_init, mi_cv_process_exit);


static void mi_cv_insmod(const char *name)
{
    MI_PRINT("module [%s] init\n", name);
}
// TODO: Tommy: IOCTRL  OPEN allow return value.
static void mi_cv_process_init(MI_COMMON_Client_t *client)
{
    MI_PRINT("client [%d] connected\n", client->pid);
    //debug_level = MI_DBG_ALL;
    MI_CV_IMPL_Init();
}
static void mi_cv_process_exit(MI_COMMON_Client_t *client)
{
    MI_PRINT("client [%d] disconnected\n", client->pid);
    MI_CV_IMPL_DeInit();
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Mstar");
