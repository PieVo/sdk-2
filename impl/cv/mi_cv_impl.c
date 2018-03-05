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
#include <linux/version.h>
#include <linux/kernel.h>

#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/math64.h>
#include <linux/idr.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>


#include "mi_print.h"
#include "mi_syscfg.h"
#include "mi_sys.h"
#include "mi_cv_impl.h"
#include "mi_cv.h"
#include "mi_sys_proc_fs_internal.h"
#include "mi_gfx.h"


#define MI_CV_PROCFS_DEBUG 1
#define CV_DEBUG_INFO 0
#define COPY_DATA_BY_GFX

#define MI_CV_MAX_QUEUE_NUM   3

#define MI_CV_CHECK_POINTER(pPtr)  \
    if(NULL == pPtr)  \
    {   \
        DBG_ERR("Invalid parameter! NULL pointer.\n");   \
        return MI_ERR_CV_NULL_PTR;   \
    }

#if LINUX_VERSION_CODE == KERNEL_VERSION(3,10,40)
#define SEMA_INIT(pSem) init_MUTEX(pSem)
#elif LINUX_VERSION_CODE == KERNEL_VERSION(3,18,30)
#define SEMA_INIT(pSem) sema_init(pSem, 1)
#endif

#define MI_INITIAL_BUF_QUEUE(queue) do{ \
            INIT_LIST_HEAD(&queue.bufList);  \
            queue.u32BufCnt = 0;   \
            SEMA_INIT(&queue.semQueue);    \
} while(0);


#define MI_DEINITIAL_BUF_QUEUE(queue) do{ \
            INIT_LIST_HEAD(queue.bufList);  \
            queue.u32BufCnt = 0;   \
} while(0);


static MI_CV_Module_t _stCevaModule;
static MI_SYS_BufConf_t _astBufConf[MI_CV_DEVICE_MAX_NUM];    // get port buf confg
static DEFINE_SEMAPHORE(module_sem);

extern MI_S32 mi_gfx_PendingDone(MI_U16 u16TargetFence);

MI_S32 _MI_CV_OnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort, void *pUsrData)
{
    MI_CV_Device_t *pstDev;
    MI_S32 s32Ret;

    pstDev = (MI_CV_Device_t *)pUsrData;
    s32Ret = MI_ERR_CV_FAIL;

    if(!MI_CV_VALID_MODID(pstChnCurPort->eModId) || !MI_CV_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_CV_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_CV_VALID_OUTPUTPORTID(pstChnCurPort->u32PortId))
        return MI_ERR_CV_FAIL;

    down(&module_sem);
    if(!_stCevaModule.bInited)
        goto exit;

    mutex_lock(&pstDev->mtxDev);
    if(pstDev->eStatus == E_MI_CV_DEVICE_UNINIT || pstDev->eStatus == E_MI_CV_DEVICE_START)
        goto exit_device;

    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.stBindPort = *pstChnPeerPort;
    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.bBind = 1;
    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_CV_OnUnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_CV_Device_t *pstDev = (MI_CV_Device_t *)pUsrData;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    if(!MI_CV_VALID_MODID(pstChnCurPort->eModId) || !MI_CV_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_CV_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_CV_VALID_OUTPUTPORTID(pstChnCurPort->u32PortId))
        return MI_ERR_CV_FAIL;

    down(&module_sem);
    if(!_stCevaModule.bInited)
        goto exit;

    mutex_lock(&pstDev->mtxDev);
    if(pstDev->eStatus == E_MI_CV_DEVICE_UNINIT || pstDev->eStatus == E_MI_CV_DEVICE_START)
        goto exit_device;

    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.bBind = 0;
    s32Ret = MI_CV_OK ;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_CV_OnBindInputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_CV_Device_t *pstDev = (MI_CV_Device_t *)pUsrData;
    MI_CV_InputPort_t *pstInputPort;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    if(!MI_CV_VALID_MODID(pstChnCurPort->eModId) || !MI_CV_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_CV_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_CV_VALID_INPUTPORTID(pstChnCurPort->u32PortId))
    {
        DBG_ERR("Invalid cur port\n");
        return MI_ERR_CV_FAIL;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        goto exit;
    }
    mutex_lock(&pstDev->mtxDev);

    pstInputPort = &pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort;
    if(pstDev->eStatus == E_MI_CV_DEVICE_UNINIT)
    {
        DBG_ERR("Device not open\n");
        goto exit_device;
    }

    MI_SYS_BUG_ON(pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.bBind);
    pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.stBindPort = *pstChnPeerPort;
    pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.bBind = 1;

    s32Ret = MI_CV_OK;
    goto exit_device;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_CV_OnUnBindInputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_CV_Device_t *pstDev = (MI_CV_Device_t *)pUsrData;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_InputPort_t *pstInputPort;

    if(!MI_CV_VALID_MODID(pstChnCurPort->eModId) || !MI_CV_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_CV_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_CV_VALID_INPUTPORTID(pstChnCurPort->u32PortId))
    {
        DBG_ERR("Invalid cur port\n");
        return MI_ERR_CV_FAIL;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        goto exit;
    }

    mutex_lock(&pstDev->mtxDev);
    pstInputPort = &pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort;
    if(pstDev->eStatus == E_MI_CV_DEVICE_UNINIT)
    {
        DBG_ERR("Device not open\n");
        goto exit_device;
    }

    if(!pstInputPort->bBind)
    {
        DBG_ERR("Input port not bound\n");
        goto exit_device;
    }

    memset(&pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.stBindPort, 0, sizeof(MI_SYS_ChnPort_t));
    pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.bBind = 0;
    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);

    return s32Ret;
}


#if defined(MI_SYS_PROC_FS_DEBUG) &&(MI_DIVP_PROCFS_DEBUG == 1)
MI_S32 _MI_CV_OnDumpstDevAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_CV_Device_t *pstDev = NULL;
    pstDev = &(_stCevaModule.astDev[u32DevId]);
    handle.OnPrintOut(handle, "\n-------------------------------- Dump Warp Dev%d Info --------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n------------------------------ End Dump Warp Dev%d Info ------------------------------\n", u32DevId);
    return MI_SUCCESS;
}

/*
N.B. use handle.OnWriteOut to print
*/
MI_S32 _MI_CV_OnDumpChannelAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_CV_Device_t *pstDev = NULL;
    pstDev = &(_stCevaModule.astDev[u32DevId]);
    handle.OnPrintOut(handle, "\n-------------------------------- Dump Warp Chn Info --------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n------------------------------ End Dump Warp Chn Info ------------------------------\n", u32DevId);
    return MI_SUCCESS;
}

/*
N.B. use handle.OnWriteOut to print
*/
MI_S32 _MI_CV_OnDumpInputPortAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_CV_Device_t *pstDev = NULL;
    pstDev = &(_stCevaModule.astDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------- Dump Wrap InputPort Info -----------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------- End Dump Wrap InputPort Info ---------------------------\n", u32DevId);
    return MI_SUCCESS;
}

MI_S32 _MI_CV_OnDumpOutPortAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId, void *pUsrData)
{
    MI_CV_Device_t *pstDev = NULL;
    pstDev = &(_stCevaModule.astDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------- Dump Wrap OutputPort Info ----------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------- End Dump Wrap OutputPort Info --------------------------\n", u32DevId);
    return MI_SUCCESS;
}

MI_S32 _MI_CV_OnHelp(MI_SYS_DEBUG_HANDLE_t  handle,MI_U32  u32DevId,void *pUsrData)
{
    MI_CV_Device_t *pstDev = NULL;
    pstDev = &(_stCevaModule.astDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------------  Wrap Help Info ---------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------------- End Wrap Help Info -------------------------------\n", u32DevId);
    return MI_SUCCESS;
}
#endif



MI_S32 MI_CV_IMPL_Init(void)
{
    MI_S32 i;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    down(&module_sem);
    if(_stCevaModule.bInited)
    {
        DBG_ERR("inited already\n");
        s32Ret = MI_ERR_CV_MOD_INITED;
        goto exit;
    }

    memset(&_stCevaModule,0,sizeof(_stCevaModule));
    _stCevaModule.bInited = 1;

    for(i=0; i < MI_CV_DEVICE_MAX_NUM; i++)
    {
        mutex_init(&_stCevaModule.astDev[i].mtxDev);
        _stCevaModule.astDev[i].eStatus = E_MI_CV_DEVICE_UNINIT;
    }

    s32Ret = MI_CV_OK;
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_DeInit(void)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_S32 i;
    MI_CV_Device_t *pstDev = NULL;

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    for(i=0; i < MI_CV_DEVICE_MAX_NUM; i++)
    {
        pstDev = &(_stCevaModule.astDev[i]);
        if(pstDev->eStatus != E_MI_CV_DEVICE_UNINIT)
        {
            DBG_ERR("Device %d not closed\n", i);
            s32Ret = MI_ERR_CV_DEV_NOT_CLOSE;
            goto exit;
        }
    }

    for(i=0; i < MI_CV_DEVICE_MAX_NUM; i++)
    {
        mutex_destroy(&_stCevaModule.astDev[i].mtxDev);
    }

    _stCevaModule.bInited = 0;
    s32Ret = MI_CV_OK;

exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_CV_IMPL_CreateDevice(MI_CV_DEV devId)
{
    MI_CV_Device_t *pstDev = NULL;
    mi_sys_ModuleDevInfo_t stModInfo;
    mi_sys_ModuleDevBindOps_t stBindOps;
    MI_S32 i;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

#ifdef MI_SYS_PROC_FS_DEBUG
    mi_sys_ModuleDevProcfsOps_t pstModuleProcfsOps;
#endif

    // check valid
    if(!MI_CV_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid Device Id\n");
        return MI_ERR_CV_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    pstDev->u32DevId = devId;

    mutex_lock(&pstDev->mtxDev);
    if(pstDev->eStatus != E_MI_CV_DEVICE_UNINIT)
    {
        DBG_ERR("Device has been Created already\n");
        s32Ret = MI_ERR_CV_DEV_OPENED;
        goto exit_device;
    }

    memset(&_astBufConf[devId], 0, sizeof(MI_SYS_BufConf_t));
    _astBufConf[devId].eBufType = E_MI_SYS_BUFDATA_FRAME;
    _astBufConf[devId].u64TargetPts = MI_SYS_INVALID_PTS;
    _astBufConf[devId].stFrameCfg.u16Width = 1920;
    _astBufConf[devId].stFrameCfg.u16Height = 1080;
    _astBufConf[devId].stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    _astBufConf[devId].stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;

    // initial dev, chn, inputport, outputport to uinit
//    pstDev->u32WakeEvent = 0;

    for (i = 0; i < MI_CV_CHANNEL_MAX_NUM; i++)
    {
        pstDev->stChannel[i].stInputPort.stInputPort.eModId = E_MI_MODULE_ID_CV;
        pstDev->stChannel[i].stInputPort.stInputPort.u32DevId = devId;
        pstDev->stChannel[i].stInputPort.stInputPort.u32ChnId = i;
        pstDev->stChannel[i].stInputPort.stInputPort.u32PortId = 0;
        pstDev->stChannel[i].stInputPort.u64Try = 0;
        pstDev->stChannel[i].stInputPort.u64RecvOk = 0;
        pstDev->stChannel[i].stInputPort.eStatus = E_MI_CV_INPUTPORT_UNINIT;

        pstDev->stChannel[i].stOutputPort.stOutputPort.eModId = E_MI_MODULE_ID_CV;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32DevId = devId;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32ChnId = i;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32PortId = 0;
        pstDev->stChannel[i].stOutputPort.u64SendOk = 0;
        pstDev->stChannel[i].stOutputPort.bInited = FALSE;

        pstDev->stChannel[i].eChnStatus = E_MI_CV_CHN_UNINIT;
        pstDev->stChannel[i].u32ChnId = i;
    }

    stModInfo.eModuleId = E_MI_MODULE_ID_CV;
    stModInfo.u32DevId = devId;
    stModInfo.u32DevChnNum = MI_CV_CHANNEL_MAX_NUM;
    stModInfo.u32InputPortNum = 1;
    stModInfo.u32OutputPortNum = 1;

    stBindOps.OnBindInputPort = _MI_CV_OnBindInputPort;
    stBindOps.OnBindOutputPort = _MI_CV_OnBindOutputPort;
    stBindOps.OnUnBindInputPort = _MI_CV_OnUnBindInputPort;
    stBindOps.OnUnBindOutputPort = _MI_CV_OnUnBindOutputPort;
    stBindOps.OnOutputPortBufRelease = NULL;

#ifdef MI_SYS_PROC_FS_DEBUG
    memset(&pstModuleProcfsOps, 0 , sizeof(pstModuleProcfsOps));
#if(MI_CV_PROCFS_DEBUG ==1)
    pstModuleProcfsOps.OnDumpDevAttr = _MI_CV_OnDumpstDevAttr;
    pstModuleProcfsOps.OnDumpChannelAttr = _MI_CV_OnDumpChannelAttr;
    pstModuleProcfsOps.OnDumpInputPortAttr = _MI_CV_OnDumpInputPortAttr;
    pstModuleProcfsOps.OnDumpOutPortAttr = _MI_CV_OnDumpOutPortAttr;
    pstModuleProcfsOps.OnHelp = _MI_CV_OnHelp;
#else
    pstModuleProcfsOps.OnDumpDevAttr = NULL;
    pstModuleProcfsOps.OnDumpChannelAttr = NULL;
    pstModuleProcfsOps.OnDumpInputPortAttr = NULL;
    pstModuleProcfsOps.OnDumpOutPortAttr = NULL;
    pstModuleProcfsOps.OnHelp = NULL;
#endif

#endif

    pstDev->hDevHandle = mi_sys_RegisterDev(&stModInfo, &stBindOps, pstDev
                                        #ifdef MI_SYS_PROC_FS_DEBUG
                                        , &pstModuleProcfsOps
                                        ,MI_COMMON_GetSelfDir
                                        #endif
                                      );
    if(NULL == pstDev->hDevHandle)
    {
        DBG_ERR("Register Module Device fail\n");
        s32Ret = MI_ERR_CV_FAIL;
        goto exit_device;
    }

#if CV_DEBUG_INFO
    DBG_ERR("pstDev: %p, register devHandle %p\n", pstDev, pstDev->hDevHandle);
#endif

/*
    //create thread for test
    pstDev->work_thread = kthread_run(_MI_CV_Device_Work, pstDev, "Cv-Dev%d", devId);
    if(IS_ERR(pstDev->work_thread))
    {
        DBG_ERR("Create work thread fail\n");
        s32Ret = MI_ERR_CV_FAIL;
        goto exit_device;
    }
*/

    pstDev->eStatus = E_MI_CV_DEVICE_START;
    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_DestroyDevice(MI_CV_DEV devId)
{
    MI_CV_Device_t *pstDev = NULL;
//    MI_CV_InputPort_t *pstInputPort;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_S32 i;

    // check valid
    if(!MI_CV_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid devId=%d\n", devId);
        return MI_ERR_CV_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not init\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);

    if(pstDev->eStatus == E_MI_CV_DEVICE_UNINIT)
    {
        DBG_ERR("Device not opened\n");
        s32Ret = MI_ERR_CV_DEV_NOT_OPEN;
        goto exit_device;
    }

    // check if able to destroy dev
    for(i = 0; i < MI_CV_CHANNEL_MAX_NUM; i++)
    {
        if (pstDev->stChannel[i].eChnStatus != E_MI_CV_CHN_UNINIT)
        {
            DBG_ERR("channel %d is still working\n", i);
            s32Ret = MI_ERR_CV_CHN_NOT_CLOSE;
            goto exit_device;
        }
    }

/*
    if(pstDev->work_thread)
    {
        kthread_stop(pstDev->work_thread);
    }
*/

    MI_SYS_BUG_ON(pstDev->hDevHandle == NULL);
    mi_sys_UnRegisterDev(pstDev->hDevHandle);

    pstDev->eStatus = E_MI_CV_DEVICE_UNINIT;      // devStatus  start
    s32Ret = MI_CV_OK;
exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;

}

// channel and input/output port disabled defaultly
MI_S32 MI_CV_IMPL_CreateChannel(MI_CV_DEV devId, MI_CV_CHN chnId)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    if (pstDev->stChannel[chnId].eChnStatus != E_MI_CV_CHN_UNINIT)
    {
        DBG_ERR("device %d chn %d has been Created\n", devId);
        s32Ret = MI_ERR_CV_CHN_OPENED;
        goto exit_device;
    }

    mi_sys_EnableChannel(pstDev->hDevHandle, chnId);
    mi_sys_EnableInputPort(pstDev->hDevHandle, chnId, 0);
    mi_sys_EnableOutputPort(pstDev->hDevHandle, chnId, 0);

    pstDev->stChannel[chnId].u32ChnId = chnId;
    pstDev->stChannel[chnId].eChnStatus = E_MI_CV_CHN_ENABLED;
    pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_CV_INPUTPORT_ENABLED;
    pstDev->stChannel[chnId].stOutputPort.bInited = TRUE;

    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_DestroyChannel(MI_CV_DEV devId, MI_CV_CHN chnId)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    if(E_MI_CV_CHN_UNINIT == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d  chn not inited\n", devId, chnId);
        s32Ret = MI_ERR_CV_CHN_NOT_OPEN;
        goto exit_device;
    }

    mi_sys_DisableOutputPort(pstDev->hDevHandle, chnId, 0);
    mi_sys_DisableInputPort(pstDev->hDevHandle, chnId, 0);
    mi_sys_DisableChannel(pstDev->hDevHandle, chnId);
    pstDev->stChannel[chnId].eChnStatus = E_MI_CV_CHN_UNINIT;
    pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_CV_INPUTPORT_UNINIT;
    pstDev->stChannel[chnId].stOutputPort.bInited = FALSE;

    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);

exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_CV_IMPL_SetInputPortAttr(MI_CV_DEV devId, MI_CV_CHN chnId, MI_CV_PORT portId, MI_CV_InputPortAttr_t *pstInputPortAttr)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_InputPort_t *pstInputPort;

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId) || !MI_CV_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    pstInputPort = &pstDev->stChannel[chnId].stInputPort;

    if(E_MI_CV_DEVICE_UNINIT == pstDev->eStatus)
    {
        DBG_ERR("device %d not open\n", devId);
        s32Ret = MI_ERR_CV_DEV_NOT_OPEN;
        goto exit_device;
    }

    pstInputPort->stAttr = *pstInputPortAttr;
    //overlay inputport use normal memory allocator

    if(pstInputPort->eStatus == E_MI_CV_INPUTPORT_UNINIT)
    {
        pstInputPort->eStatus = E_MI_CV_INPUTPORT_INIT;
    }

#if CV_DEBUG_INFO
    DBG_ERR("Set inputport attr\n");
#endif

    s32Ret = MI_CV_OK;
exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_GetInputPortAttr(MI_CV_DEV devId, MI_CV_CHN chnId, MI_CV_PORT portId, MI_CV_InputPortAttr_t *pstInputPortAttr)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret=-1;

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId) || !MI_CV_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    if(E_MI_CV_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus)
    {
        DBG_ERR("Input port not inited\n");
        s32Ret = MI_ERR_CV_NOT_CONFIG;
        goto exit_device;
    }
    *pstInputPortAttr = pstDev->stChannel[chnId].stInputPort.stAttr;

#if CV_DEBUG_INFO
    DBG_ERR("In wid:   %d\n", pstInputPortAttr->u16Width);
    DBG_ERR("In hgt:   %d\n", pstInputPortAttr->u16Height);
    DBG_ERR("In fmt:   %d\n", (MI_S32)pstInputPortAttr->ePixelFormat);
    DBG_ERR("In compress:   %d\n", (MI_S32)pstInputPortAttr->eCompressMode);
    DBG_ERR("Get inputport attr\n");
#endif

    s32Ret = MI_CV_OK;
exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_SetOutputPortAttr(MI_CV_DEV devId, MI_CV_CHN chnId, MI_CV_PORT portId, MI_CV_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId) || !MI_CV_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    if(E_MI_CV_DEVICE_UNINIT == pstDev->eStatus || (E_MI_CV_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus))
    {
        DBG_ERR("device %d is not open or chn is not enable\n", devId);
        s32Ret = MI_ERR_CV_DEV_NOT_OPEN;
        goto exit_device;
    }

    pstDev->stChannel[chnId].stOutputPort.stAttr = *pstOutputPortAttr;
//    pstDev->stChannel[chnId].stOutputPort[portId].u64Interval = pstOutputPortAttr->u32FrmRate?(1000000/pstOutputPortAttr->u32FrmRate):(1000000/30);
    pstDev->stChannel[chnId].stOutputPort.bInited = 1;

    s32Ret = MI_CV_OK;

exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_CV_IMPL_GetOutputPortAttr(MI_CV_DEV devId, MI_CV_CHN chnId, MI_CV_PORT portId, MI_CV_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_CV_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

#if CV_DEBUG_INFO
                DBG_ERR("Enter get inputport attr\n");
#endif

    if(!MI_CV_VALID_DEVID(devId) || !MI_CV_VALID_CHNID(chnId) || !MI_CV_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_CV_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stCevaModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_CV_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stCevaModule.astDev[devId]);
    mutex_lock(&pstDev->mtxDev);
    if(!pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        DBG_ERR("Output port not inited\n");
        s32Ret = MI_ERR_CV_NOT_CONFIG;
        goto exit_device;
    }
    *pstOutputPortAttr = pstDev->stChannel[chnId].stOutputPort.stAttr;

#if CV_DEBUG_INFO
    DBG_ERR("Out wid:   %d\n", pstOutputPortAttr->u16Width);
    DBG_ERR("Out hgt:   %d\n", pstOutputPortAttr->u16Height);
    DBG_ERR("Out fmt:   %d\n", (MI_S32)pstOutputPortAttr->ePixelFormat);
    DBG_ERR("Out compress:   %d\n", (MI_S32)pstOutputPortAttr->eCompressMode);
    DBG_ERR("Get outputport attr\n");
#endif

    s32Ret = MI_CV_OK;
exit_device:
    mutex_unlock(&pstDev->mtxDev);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_CV_IMPL_GetCdnnCfgBuf(MI_PHY phyAddr, MI_U32 u32Size)
{
    MI_S32 s32Ret;
    const MI_SYSCFG_MmapInfo_t* pstMmapInfo = NULL;

    MI_SYSCFG_GetMmapInfo("E_EMAC_ID_CEVA", &pstMmapInfo);
    if(pstMmapInfo != NULL)
    {
        printk("Main buffer Addr =  0x%x \n", pstMmapInfo->u32Addr);
        printk("Main buffer size =  0x%x \n", pstMmapInfo->u32Size);
        printk("Main buffer u32Align =  0x%x \n", pstMmapInfo->u32Align);
        printk("Main buffer u8MiuNo =  0x%x \n", pstMmapInfo->u8MiuNo);

        phyAddr = pstMmapInfo->u32Addr;
        u32Size = pstMmapInfo->u32Size;
        s32Ret = MI_CV_OK;
    }
    else
    {
        printk("Get Ceva buffer Fail \n");
        s32Ret = MI_ERR_CV_FAIL;
    }

    return s32Ret;
}


