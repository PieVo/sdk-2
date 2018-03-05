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
#include <linux/irq.h>


#include "mi_print.h"
#include "mi_sys.h"
#include "mi_warp_impl.h"
#include "mi_warp.h"
#include "irqs.h"
#include "mhal_warp.h"
#include "mi_sys_proc_fs_internal.h"
#include "mi_gfx.h"


#define MI_WARP_PROCFS_DEBUG 1
#define WARP_DEBUG_INFO 0
#define COPY_DATA_BY_GFX

#define MI_WARP_MAX_QUEUE_NUM   3

#define MI_WARP_CHECK_POINTER(pPtr)  \
    if(NULL == pPtr)  \
    {   \
        DBG_ERR("Invalid parameter! NULL pointer.\n");   \
        return MI_ERR_WARP_NULL_PTR;   \
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


static MI_WARP_Module_t _stWarpModule;
static MI_SYS_BufConf_t _astBufConf[MI_WARP_MAX_DEVICE_NUM];    // get port buf confg
static DEFINE_SEMAPHORE(module_sem);

extern MI_S32 mi_gfx_PendingDone(MI_U16 u16TargetFence);


irqreturn_t  _MI_WARP_ISR_Proc(s32 irq, void* data)
{
    MHAL_WARP_DEV_HANDLE hHandle = (MHAL_WARP_DEV_HANDLE*)data;
    MHAL_WARP_ISR_STATE_E eState;

    eState = MHAL_WARP_IsrProc(hHandle);
    switch(eState)
    {
        case MHAL_WARP_ISR_STATE_DONE:
            return IRQ_HANDLED;

        default:
            return IRQ_NONE;
    }

    return IRQ_NONE;
}

MI_U32 _MI_WARP_Trigger_Callback(MHAL_WARP_INST_HANDLE instance, void *usrData)
{
    struct semaphore *sema = (struct semaphore *)usrData;

    DBG_INFO("callback is called\n");

    up(sema);

    return 0;
}


#if TEST_CHN_SINGLE_WORKTHREAD
static MI_S32 _MI_WARP_TestThread(void *pData)
{
#if 1
    // add input data into listIn
    MI_WARP_Device_t *pstDev = NULL;
    MI_SYS_BufInfo_t *pstInBufInfo = NULL;
    MI_SYS_BufInfo_t *pstOutBufInfo = NULL;
    MI_BOOL bBlockedByRateCtrl = FALSE;

#ifdef COPY_DATA_BY_GFX
#if 1
#else
    MI_U16 u16Fence = 0;
#endif
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDst;
    MI_GFX_Rect_t stDstRect;
#endif

    struct semaphore sema;
    MI_WARP_Channel_t *pstChnCtx = (MI_WARP_Channel_t*)pData;
    MHAL_WARP_CONFIG *pConfig = (MHAL_WARP_CONFIG*)pstChnCtx->virConfAddr;
    pstDev = &(_stWarpModule.stDev[pstChnCtx->u32DevId]);

    SEMA_INIT(&sema);

    MI_GFX_Open();

    while(1)
    {
        if(kthread_should_stop())
        {
            DBG_INFO("getInbufThread stop!\n");
            MI_GFX_Close();
            return MI_WARP_OK;
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 1..waiton input, pstDev %p, devhandle %p\n", pstDev, pstDev->hDevHandle);
#endif
        if(mi_sys_WaitOnInputTaskAvailable(pstDev->hDevHandle, 16) != MI_SUCCESS)
        {
            continue;
        }

        down(&pstChnCtx->semChnTest);

#if WARP_DEBUG_INFO
        DBG_ERR("step 2..get input\n");
#endif

        pstInBufInfo = mi_sys_GetInputPortBuf(pstDev->hDevHandle, pstChnCtx->u32ChnId, 0, 0);
        if (!pstInBufInfo)
        {
            msleep(5);
            up(&pstChnCtx->semChnTest);
            continue;
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 3..get output\n");
#endif

        pstOutBufInfo = mi_sys_GetOutputPortBuf(pstDev->hDevHandle, pstChnCtx->u32ChnId, 0, &_astBufConf[pstDev->u32DeviceId], &bBlockedByRateCtrl);
        if(!pstOutBufInfo)
        {
            if(bBlockedByRateCtrl)
            {
                mi_sys_FinishBuf(pstInBufInfo);
            }
            else
            {
                DBG_ERR("Rewind outportBuf\n");
                mi_sys_RewindBuf(pstInBufInfo);
            }
            up(&pstChnCtx->semChnTest);
            msleep(5);
            continue;
        }

        stSrcRect.s32Xpos = 0;
        stSrcRect.u32Height = (MI_U32)(pstInBufInfo->stFrameData.u16Height);
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = (MI_U32)(pstInBufInfo->stFrameData.u16Width);

        stSrc.eColorFmt = E_MI_GFX_FMT_YUV422;
        stSrc.phyAddr = pstInBufInfo->stFrameData.phyAddr[0];
        stSrc.u32Height = stSrcRect.u32Height;
        stSrc.u32Width = stSrcRect.u32Width;
        stSrc.u32Stride = pstInBufInfo->stFrameData.u32Stride[0];

        memset(&stDst, 0, sizeof(stDst));
        stDst.eColorFmt = E_MI_GFX_FMT_YUV422;
        stDst.phyAddr   = pstOutBufInfo->stFrameData.phyAddr[0];
        stDst.u32Stride   = pstOutBufInfo->stFrameData.u32Stride[0];
        stDst.u32Height = pstOutBufInfo->stFrameData.u16Height;
        stDst.u32Width  = pstOutBufInfo->stFrameData.u16Width;

        memset(&stDstRect, 0, sizeof(stDstRect));
        stDstRect.s32Xpos = 0;
        stDstRect.u32Width = stDst.u32Width;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Height = stDst.u32Height;
        pstOutBufInfo->bEndOfStream = pstInBufInfo->bEndOfStream;

#if WARP_DEBUG_INFO
        DBG_ERR("step 4..do gfx blit\n");
#endif

#if 1
        // warp process
        pConfig->input_image.width = pstInBufInfo->stFrameData.u16Width;
        pConfig->input_image.height = pstInBufInfo->stFrameData.u16Height;
        pConfig->input_image.format = MHAL_WARP_IMAGE_FORMAT_YUV422;
        pConfig->input_data.num_planes = 2;
        pConfig->input_data.data[0] = (MS_U8*)pstInBufInfo->stFrameData.pVirAddr[0];
        pConfig->input_data.data[1] = (MS_U8*)pstInBufInfo->stFrameData.pVirAddr[1];
/*
        pConfig->input_data.data[0] = (MS_U8*)mi_sys_Vmap(pstInBufInfo->stFrameData.phyAddr[0]
                                       , pstInBufInfo->stFrameData.u16Width * pstInBufInfo->stFrameData.u16Height, FALSE);
        pConfig->input_data.data[1] = (MS_U8*)mi_sys_Vmap(pstInBufInfo->stFrameData.phyAddr[1]
                                       , pstInBufInfo->stFrameData.u16Width * pstInBufInfo->stFrameData.u16Height, FALSE);
*/
        pConfig->output_image.width = pstOutBufInfo->stFrameData.u16Width;
        pConfig->output_image.height = pstOutBufInfo->stFrameData.u16Height;
        pConfig->output_image.format = MHAL_WARP_IMAGE_FORMAT_YUV422;
        pConfig->output_data.num_planes = 2;
        pConfig->output_data.data[0] = (MS_U8*)pstOutBufInfo->stFrameData.pVirAddr[0];
        pConfig->output_data.data[1] = (MS_U8*)pstOutBufInfo->stFrameData.pVirAddr[1];
/*
        pConfig->output_data.data[0] = (MS_U8*)mi_sys_Vmap(pstOutBufInfo->stFrameData.phyAddr[0]
                                       , pstOutBufInfo->stFrameData.u16Width * pstOutBufInfo->stFrameData.u16Height, FALSE);
        pConfig->output_data.data[1] = (MS_U8*)mi_sys_Vmap(pstOutBufInfo->stFrameData.phyAddr[1]
                                       , pstOutBufInfo->stFrameData.u16Width * pstOutBufInfo->stFrameData.u16Height, FALSE);
*/
        // proce a image
        if (MHAL_WARP_INSTANCE_STATE_READY != MHAL_WARP_CheckState(pstChnCtx->hInstHandle))
        {
            up(&pstChnCtx->semChnTest);
            msleep(5);
            continue;
        }

        down(&sema);
        if (MHAL_SUCCESS != MHAL_WARP_Trigger(pstChnCtx->hInstHandle, pConfig, (MHAL_WARP_CALLBACK)_MI_WARP_Trigger_Callback, &sema))
        {
            printk("can't process image!!\n");
        }
        up(&sema);

#else
        if(MI_SUCCESS != MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, NULL, &u16Fence))
        {
              DBG_ERR("[%s %d] MI_GFX_BitBlit fail!!!\n", __FUNCTION__, __LINE__);
        }

        MI_GFX_WaitAllDone(FALSE, u16Fence);
#endif
        pstOutBufInfo->u64Pts = pstInBufInfo->u64Pts;

#if WARP_DEBUG_INFO
        DBG_ERR("step 5..finish input\n");
#endif

        if(mi_sys_FinishBuf(pstInBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish input buf fail \n");
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 6..finish output\n");
#endif

        if(mi_sys_FinishBuf(pstOutBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish output buf fail \n");
        }

        up(&pstChnCtx->semChnTest);
    }

    MI_GFX_Close();
#endif
    return MI_WARP_OK;
}

#else
static MI_S32 _MI_WARP_GetInputBufThread(void *pData)
{
    // add input data into listIn
    MI_WARP_Device_t *pstDev = NULL;
    MI_SYS_BufInfo_t *pstInBufInfo = NULL;
    MI_SYS_BufInfo_t *pstOutBufInfo = NULL;
    MI_BOOL bBlockedByRateCtrl = FALSE;

#ifdef COPY_DATA_BY_GFX
    MI_U16 u16Fence = 0;
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDst;
    MI_GFX_Rect_t stDstRect;
#endif

    MI_WARP_Channel_t *pstChnCtx = (MI_WARP_Channel_t*)pData;
    pstDev = &(_stWarpModule.stDev[pstChnCtx->u32DevId]);

    MI_GFX_Open();

    while(1)
    {
        if(kthread_should_stop())
        {
            DBG_INFO("getInbufThread stop!\n");
            MI_GFX_Close();
            return MI_WARP_OK;
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 1..waiton input, pstDev %p, devhandle %p\n", pstDev, pstDev->hDevHandle);
#endif
        if(mi_sys_WaitOnInputTaskAvailable(pstDev->hDevHandle, 16) != MI_SUCCESS)
        {
            continue;
        }

        down(&pstChnCtx->semChnInputBuf);

#if WARP_DEBUG_INFO
        DBG_ERR("step 2..get input\n");
#endif

        pstInBufInfo = mi_sys_GetInputPortBuf(pstDev->hDevHandle, pstChnCtx->u32ChnId, 0, 0);
        if (!pstInBufInfo)
        {
            msleep(5);
            up(&pstChnCtx->semChnInputBuf);
            continue;
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 3..get output\n");
#endif

        pstOutBufInfo = mi_sys_GetOutputPortBuf(pstDev->hDevHandle, pstChnCtx->u32ChnId, 0, &_astBufConf[pstDev->u32DeviceId], &bBlockedByRateCtrl);
        if(!pstOutBufInfo)
        {
            if(bBlockedByRateCtrl)
            {
                mi_sys_FinishBuf(pstInBufInfo);
            }
            else
            {
                DBG_ERR("Rewind outportBuf\n");
                mi_sys_RewindBuf(pstInBufInfo);
            }
            up(&pstChnCtx->semChnInputBuf);
            msleep(5);
            continue;
        }

        stSrcRect.s32Xpos = 0;
        stSrcRect.u32Height = (MI_U32)(pstInBufInfo->stFrameData.u16Height);
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = (MI_U32)(pstInBufInfo->stFrameData.u16Width);

        stSrc.eColorFmt = E_MI_GFX_FMT_YUV422;
        stSrc.phyAddr = pstInBufInfo->stFrameData.phyAddr[0];
        stSrc.u32Height = stSrcRect.u32Height;
        stSrc.u32Width = stSrcRect.u32Width;
        stSrc.u32Stride = pstInBufInfo->stFrameData.u32Stride[0];

        memset(&stDst, 0, sizeof(stDst));
        stDst.eColorFmt = E_MI_GFX_FMT_YUV422;
        stDst.phyAddr   = pstOutBufInfo->stFrameData.phyAddr[0];
        stDst.u32Stride   = pstOutBufInfo->stFrameData.u32Stride[0];
        stDst.u32Height = pstOutBufInfo->stFrameData.u16Height;
        stDst.u32Width  = pstOutBufInfo->stFrameData.u16Width;

        memset(&stDstRect, 0, sizeof(stDstRect));
        stDstRect.s32Xpos = 0;
        stDstRect.u32Width = stDst.u32Width;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Height = stDst.u32Height;
        pstOutBufInfo->bEndOfStream = pstInBufInfo->bEndOfStream;

#if WARP_DEBUG_INFO
        DBG_ERR("step 4..do gfx blit\n");
#endif

        if(MI_SUCCESS != MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, NULL, &u16Fence))
        {
              DBG_ERR("[%s %d] MI_GFX_BitBlit fail!!!\n", __FUNCTION__, __LINE__);
        }

        MI_GFX_WaitAllDone(FALSE, u16Fence);
        pstOutBufInfo->u64Pts = pstInBufInfo->u64Pts;

#if WARP_DEBUG_INFO
        DBG_ERR("step 5..finish input\n");
#endif

        if(mi_sys_FinishBuf(pstInBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish input buf fail (Module: divp Chn:%d Port:%d)\n", 0, 0);
        }

#if WARP_DEBUG_INFO
        DBG_ERR("step 6..finish output\n");
#endif

        if(mi_sys_FinishBuf(pstOutBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish output buf fail (Module: divp Chn:%d Port:%d)\n", 0, 0);
        }

        up(&pstChnCtx->semChnInputBuf);
    }

    MI_GFX_Close();

    return MI_WARP_OK;
}

static MI_S32 _MI_WARP_PutOutputBufThread(void *pData)
{
    // get output data from listOut

    return 0;
}
#endif

#if TEST_DEV_WORKTHREAD
static MI_S32 _MI_WARP_Device_Work(void *pData)
{
    MI_WARP_Device_t *pstDev;
    MI_SYS_BufInfo_t *pstInBufInfo = NULL;
    MI_SYS_BufInfo_t *pstOutBufInfo = NULL;
    MI_BOOL bBlockedByRateCtrl = FALSE;

#ifdef COPY_DATA_BY_GFX
    MI_U16 u16Fence = 0;
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDst;
    MI_GFX_Rect_t stDstRect;
#endif

    DBG_ERR("enter dev thread\n");
    pstDev = (MI_WARP_Device_t*)pData;
    MI_GFX_Open();

    while(1)
    {
        if(kthread_should_stop())
        {
            DBG_INFO("Warp work thread stop!\n");
            MI_GFX_Close();
            return MI_WARP_OK;
        }

        if(mi_sys_WaitOnInputTaskAvailable(pstDev->hDevHandle, 16) != MI_SUCCESS)
        {
            continue;
        }

        down(&pstDev->chnCtxSem[0]);
        pstInBufInfo = mi_sys_GetInputPortBuf(pstDev->hDevHandle, 0, 0, 0);
        if (!pstInBufInfo)
        {
            msleep(5);
            up(&pstDev->chnCtxSem[0]);
            continue;
        }

        pstOutBufInfo = mi_sys_GetOutputPortBuf(pstDev->hDevHandle, 0, 0, &_astBufConf[0], &bBlockedByRateCtrl);

        if(!pstOutBufInfo)
        {
            //DBG_ERR("GetOutPutPortBuf is NULL, rateCtrl=%d\n", bBlockedByRateCtrl);
            if(bBlockedByRateCtrl)
            {
            //    DBG_ERR("finish outportBuf\n");
                mi_sys_FinishBuf(pstInBufInfo);
            }
            else
            {
                DBG_ERR("Rewind outportBuf\n");
                mi_sys_RewindBuf(pstInBufInfo);
            }

            up(&pstDev->chnCtxSem[0]);
            msleep(5);
            continue;
        }

        stSrcRect.s32Xpos = 0;
        stSrcRect.u32Height = (MI_U32)(pstInBufInfo->stFrameData.u16Height);
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = (MI_U32)(pstInBufInfo->stFrameData.u16Width);

        stSrc.eColorFmt = E_MI_GFX_FMT_YUV422;
        stSrc.phyAddr = pstInBufInfo->stFrameData.phyAddr[0];
        stSrc.u32Height = stSrcRect.u32Height;
        stSrc.u32Width = stSrcRect.u32Width;
        stSrc.u32Stride = pstInBufInfo->stFrameData.u32Stride[0];

        memset(&stDst, 0, sizeof(stDst));
        stDst.eColorFmt = E_MI_GFX_FMT_YUV422;
        stDst.phyAddr   = pstOutBufInfo->stFrameData.phyAddr[0];
        stDst.u32Stride   = pstOutBufInfo->stFrameData.u32Stride[0];
        stDst.u32Height = pstOutBufInfo->stFrameData.u16Height;
        stDst.u32Width  = pstOutBufInfo->stFrameData.u16Width;

        memset(&stDstRect, 0, sizeof(stDstRect));
        stDstRect.s32Xpos = 0;
        stDstRect.u32Width = stDst.u32Width;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Height = stDst.u32Height;
        pstOutBufInfo->bEndOfStream = pstInBufInfo->bEndOfStream;

        if(MI_SUCCESS != MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, NULL, &u16Fence))
        {
              DBG_ERR("[%s %d] MI_GFX_BitBlit fail!!!\n", __FUNCTION__, __LINE__);
        }

        MI_GFX_WaitAllDone(FALSE, u16Fence);
        pstOutBufInfo->u64Pts = pstInBufInfo->u64Pts;

        if(mi_sys_FinishBuf(pstInBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish input buf fail (Module: divp Chn:%d Port:%d)\n", 0, 0);
        }

        if(mi_sys_FinishBuf(pstOutBufInfo) != MI_SUCCESS)
        {
            DBG_INFO("finish output buf fail (Module: divp Chn:%d Port:%d)\n", 0, 0);
        }

        up(&pstDev->chnCtxSem[0]);
    }

    MI_GFX_Close();

    return MI_WARP_OK;
}
#endif

MI_S32 _MI_WARP_OnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort, void *pUsrData)
{
    MI_WARP_Device_t *pstDev;
    MI_S32 s32Ret;

    pstDev = (MI_WARP_Device_t *)pUsrData;
    s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_MODID(pstChnCurPort->eModId) || !MI_WARP_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_WARP_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_WARP_VALID_OUTPUTPORTID(pstChnCurPort->u32PortId))
        return MI_ERR_WARP_FAIL;

    down(&module_sem);
    if(!_stWarpModule.bInited)
        goto exit;

    mutex_lock(&pstDev->mtx);
    if(pstDev->eStatus == E_MI_WARP_DEVICE_UNINIT || pstDev->eStatus == E_MI_WARP_DEVICE_START)
        goto exit_device;

    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.stBindPort = *pstChnPeerPort;
    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.bBind = 1;
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_WARP_OnUnBindOutputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = (MI_WARP_Device_t *)pUsrData;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    if(!MI_WARP_VALID_MODID(pstChnCurPort->eModId) || !MI_WARP_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_WARP_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_WARP_VALID_OUTPUTPORTID(pstChnCurPort->u32PortId))
        return MI_ERR_WARP_FAIL;

    down(&module_sem);
    if(!_stWarpModule.bInited)
        goto exit;

    mutex_lock(&pstDev->mtx);
    if(pstDev->eStatus == E_MI_WARP_DEVICE_UNINIT || pstDev->eStatus == E_MI_WARP_DEVICE_START)
        goto exit_device;

    pstDev->stChannel[pstChnCurPort->u32ChnId].stOutputPort.bBind = 0;
    s32Ret = MI_WARP_OK ;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_WARP_OnBindInputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = (MI_WARP_Device_t *)pUsrData;
    MI_WARP_Inputport_t *pstInputPort;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_MODID(pstChnCurPort->eModId) || !MI_WARP_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_WARP_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_WARP_VALID_INPUTPORTID(pstChnCurPort->u32PortId))
    {
        DBG_ERR("Invalid cur port\n");
        DBG_ERR("mod: %d dev: %d chn: %d port: %d\n", pstChnCurPort->eModId, pstChnCurPort->u32DevId,
                 pstChnCurPort->u32ChnId, pstChnCurPort->u32PortId);
        return MI_ERR_WARP_FAIL;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        goto exit;
    }
    mutex_lock(&pstDev->mtx);

    pstInputPort = &pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort;
    if(pstDev->eStatus == E_MI_WARP_DEVICE_UNINIT)
    {
        DBG_ERR("Device not open\n");
        goto exit_device;
    }

    MI_SYS_BUG_ON(pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.bBind);
    pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.stBindPort = *pstChnPeerPort;
    pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort.bBind = 1;

    s32Ret = MI_WARP_OK;
    goto exit_device;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 _MI_WARP_OnUnBindInputPort(MI_SYS_ChnPort_t *pstChnCurPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = (MI_WARP_Device_t *)pUsrData;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_Inputport_t *pstInputPort;

    if(!MI_WARP_VALID_MODID(pstChnCurPort->eModId) || !MI_WARP_VALID_DEVID(pstChnCurPort->u32DevId)
       || !MI_WARP_VALID_CHNID(pstChnCurPort->u32ChnId) || !MI_WARP_VALID_INPUTPORTID(pstChnCurPort->u32PortId))
    {
        DBG_ERR("Invalid cur port\n");
        return MI_ERR_WARP_FAIL;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        goto exit;
    }

    mutex_lock(&pstDev->mtx);
    pstInputPort = &pstDev->stChannel[pstChnCurPort->u32ChnId].stInputPort;
    if(pstDev->eStatus == E_MI_WARP_DEVICE_UNINIT)
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
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);

    return s32Ret;
}




#if defined(MI_SYS_PROC_FS_DEBUG) &&(MI_DIVP_PROCFS_DEBUG == 1)
MI_S32 _MI_WARP_OnDumpstDevAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = NULL;
    pstDev = &(_stWarpModule.stDev[u32DevId]);
    handle.OnPrintOut(handle, "\n-------------------------------- Dump Warp Dev%d Info --------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n------------------------------ End Dump Warp Dev%d Info ------------------------------\n", u32DevId);
    return MI_SUCCESS;
}

/*
N.B. use handle.OnWriteOut to print
*/
MI_S32 _MI_WARP_OnDumpChannelAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = NULL;
    pstDev = &(_stWarpModule.stDev[u32DevId]);
    handle.OnPrintOut(handle, "\n-------------------------------- Dump Warp Chn Info --------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n------------------------------ End Dump Warp Chn Info ------------------------------\n", u32DevId);
    return MI_SUCCESS;
}

/*
N.B. use handle.OnWriteOut to print
*/
MI_S32 _MI_WARP_OnDumpInputPortAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = NULL;
    pstDev = &(_stWarpModule.stDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------- Dump Wrap InputPort Info -----------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------- End Dump Wrap InputPort Info ---------------------------\n", u32DevId);
    return MI_SUCCESS;
}

MI_S32 _MI_WARP_OnDumpOutPortAttr(MI_SYS_DEBUG_HANDLE_t handle,MI_U32 u32DevId, void *pUsrData)
{
    MI_WARP_Device_t *pstDev = NULL;
    pstDev = &(_stWarpModule.stDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------- Dump Wrap OutputPort Info ----------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------- End Dump Wrap OutputPort Info --------------------------\n", u32DevId);
    return MI_SUCCESS;
}

MI_S32 _MI_WARP_OnHelp(MI_SYS_DEBUG_HANDLE_t  handle,MI_U32  u32DevId,void *pUsrData)
{
    MI_WARP_Device_t *pstDev = NULL;
    pstDev = &(_stWarpModule.stDev[u32DevId]);
    handle.OnPrintOut(handle, "\n----------------------------------  Wrap Help Info ---------------------------------\n", u32DevId);

    handle.OnPrintOut(handle, "\n--------------------------------- End Wrap Help Info -------------------------------\n", u32DevId);
    return MI_SUCCESS;
}
#endif



MI_S32 MI_WARP_IMPL_Init(void)
{
    MI_S32 i;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    down(&module_sem);
    if(_stWarpModule.bInited)
    {
        DBG_ERR("WRAP IMPL: inited already\n");
        s32Ret = MI_ERR_WARP_MOD_INITED;
        goto exit;
    }

    memset(&_stWarpModule,0,sizeof(_stWarpModule));

    for(i=0; i < MI_WARP_MAX_DEVICE_NUM; i++)
    {
        mutex_init(&_stWarpModule.stDev[i].mtx);
    }

    _stWarpModule.bInited = 1;

    for(i =0; i < MI_WARP_MAX_CHN_NUM; i++)
    {
        // init chnInlistbuf

        // init chnOutlistbuf
    }
    s32Ret = MI_WARP_OK;
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_DeInit(void)
{
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_S32 i;
    MI_WARP_Device_t *pstDev = NULL;

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    for(i=0; i < MI_WARP_MAX_DEVICE_NUM; i++)
    {
        pstDev = &(_stWarpModule.stDev[i]);
        if(pstDev->eStatus != E_MI_WARP_DEVICE_UNINIT)
        {
            DBG_ERR("Device %d not closed\n", i);
            s32Ret = MI_ERR_WARP_DEV_NOT_CLOSE;
            goto exit;
        }
    }

    for(i=0; i < MI_WARP_MAX_DEVICE_NUM; i++)
    {
        mutex_destroy(&_stWarpModule.stDev[i].mtx);
    }

    _stWarpModule.bInited = 0;
    s32Ret = MI_WARP_OK;

exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_WARP_IMPL_CreateDevice(MI_WARP_DEV devId)
{
    MI_WARP_Device_t *pstDev = NULL;
    mi_sys_ModuleDevInfo_t stModInfo;
    mi_sys_ModuleDevBindOps_t stBindOps;
    MI_U32 u32IrqId = 0;
    MI_S32 s32IrqRet;
    MI_S32 i;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

#ifdef MI_SYS_PROC_FS_DEBUG
    mi_sys_ModuleDevProcfsOps_t pstModuleProcfsOps;
#endif

    // check valid
    if(!MI_WARP_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid Device Id\n");
        return MI_ERR_WARP_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    pstDev->u32DeviceId = devId;

    mutex_lock(&pstDev->mtx);
    if(pstDev->eStatus != E_MI_WARP_DEVICE_UNINIT)
    {
        DBG_ERR("Device Opened already\n");
        s32Ret = MI_ERR_WARP_DEV_OPENED;
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
    pstDev->u32WakeEvent = 0;

    for (i = 0; i < MI_WARP_MAX_CHN_NUM; i++)
    {
        pstDev->stChannel[i].stInputPort.stInputPort.eModId = E_MI_MODULE_ID_WARP;
        pstDev->stChannel[i].stInputPort.stInputPort.u32DevId = devId;
        pstDev->stChannel[i].stInputPort.stInputPort.u32ChnId = i;
        pstDev->stChannel[i].stInputPort.stInputPort.u32PortId = 0;
        pstDev->stChannel[i].stInputPort.u64Try = 0;
        pstDev->stChannel[i].stInputPort.u64RecvOk = 0;
        pstDev->stChannel[i].stInputPort.eStatus = E_MI_WARP_INPUTPORT_UNINIT;

        pstDev->stChannel[i].stOutputPort.stOutputPort.eModId = E_MI_MODULE_ID_WARP;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32DevId = devId;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32ChnId = i;
        pstDev->stChannel[i].stOutputPort.stOutputPort.u32PortId = 0;
        pstDev->stChannel[i].stOutputPort.u64SendOk = 0;
        pstDev->stChannel[i].stOutputPort.bInited = FALSE;

        pstDev->stChannel[i].eChnStatus = E_MI_WARP_CHN_UNINIT;
        pstDev->stChannel[i].u32ChnId = i;
        pstDev->stChannel[i].hInstHandle = NULL;
        pstDev->stChannel[i].phyConfAddr = 0;
        pstDev->stChannel[i].virConfAddr = 0;
        mutex_init(&pstDev->stChannel[i].mtxWarpConf);

#if TEST_DEV_WORKTHREAD
    SEMA_INIT(&pstDev->chnCtxSem[i]);
#endif
    }

    stModInfo.eModuleId = E_MI_MODULE_ID_WARP;
    stModInfo.u32DevId = devId;
    stModInfo.u32DevChnNum = MI_WARP_MAX_CHN_NUM;
    stModInfo.u32InputPortNum = 1;
    stModInfo.u32OutputPortNum = 1;

    stBindOps.OnBindInputPort = _MI_WARP_OnBindInputPort;
    stBindOps.OnBindOutputPort = _MI_WARP_OnBindOutputPort;
    stBindOps.OnUnBindInputPort = _MI_WARP_OnUnBindInputPort;
    stBindOps.OnUnBindOutputPort = _MI_WARP_OnUnBindOutputPort;
    stBindOps.OnOutputPortBufRelease = NULL;

#ifdef MI_SYS_PROC_FS_DEBUG
    memset(&pstModuleProcfsOps, 0 , sizeof(pstModuleProcfsOps));
#if(MI_WARP_PROCFS_DEBUG ==1)
    pstModuleProcfsOps.OnDumpDevAttr = _MI_WARP_OnDumpstDevAttr;
    pstModuleProcfsOps.OnDumpChannelAttr = _MI_WARP_OnDumpChannelAttr;
    pstModuleProcfsOps.OnDumpInputPortAttr = _MI_WARP_OnDumpInputPortAttr;
    pstModuleProcfsOps.OnDumpOutPortAttr = _MI_WARP_OnDumpOutPortAttr;
    pstModuleProcfsOps.OnHelp = _MI_WARP_OnHelp;
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
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

#if WARP_DEBUG_INFO
    DBG_ERR("pstDev: %p, register devHandle %p\n", pstDev, pstDev->hDevHandle);
#endif

    //create thread
#if TEST_DEV_WORKTHREAD
    pstDev->work_thread = kthread_run(_MI_WARP_Device_Work, pstDev, "Warp-Dev%d", devId);
    if(IS_ERR(pstDev->work_thread))
    {
        DBG_ERR("Create work thread fail\n");
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }
#endif

    // create mhal_warp
    if (MHAL_SUCCESS != MHAL_WARP_CreateDevice(devId, &pstDev->hMhalHandle) )
    {
        DBG_ERR("Create mhal_warp dev %d failed\n", devId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto fail_stopthread;
    }
    mutex_unlock(&pstDev->mtx);

    // enable irq
//    MHAL_WARP_GetIrqNum(&u32IrqId);
    DBG_INFO("u32IrqId = %d.\n", u32IrqId);
    s32IrqRet = request_irq(INT_IRQ_78_WARP2RIU_INT+32, _MI_WARP_ISR_Proc, IRQ_TYPE_LEVEL_HIGH, "mi_warp_isr", &pstDev->hMhalHandle);
    if(0 != s32IrqRet)
    {
        DBG_ERR("request_irq failed. u32IrqId = %u, s32IrqRet = %d.\n\n ", u32IrqId, s32IrqRet);
    }
//    MHAL_WARP_EnableIsr(TRUE);

    pstDev->eStatus = E_MI_WARP_DEVICE_INIT;    // createDev uinit->init
    s32Ret = MI_WARP_OK;
    goto exit;

fail_stopthread:
#if TEST_DEV_WORKTHREAD
    kthread_stop(pstDev->work_thread);
#endif

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;


}

MI_S32 MI_WARP_IMPL_DestroyDevice(MI_WARP_DEV devId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_WARP_Inputport_t *pstInputPort;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_U32 u32IrqId = 0;
    MI_S32 i;

    // check valid
    if(!MI_WARP_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid devId=%d\n", devId);
        return MI_ERR_WARP_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not init\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);

    if(pstDev->eStatus == E_MI_WARP_DEVICE_UNINIT)
    {
        DBG_ERR("Device not opened\n");
        s32Ret = MI_ERR_WARP_DEV_NOT_OPEN;
        goto exit_device;
    }

    if(E_MI_WARP_DEVICE_START == pstDev->eStatus)
    {
        DBG_ERR("Device is working: %d\n", pstDev->eStatus);
        s32Ret = MI_ERR_WARP_DEV_NOT_STOP;
        goto exit_device;
    }

    // check if able to destroy dev
    for(i = 0; i < MI_WARP_MAX_CHN_NUM; i++)
    {
        if (pstDev->stChannel[i].eChnStatus != E_MI_WARP_CHN_UNINIT)
        {
            DBG_ERR("channel %d is still working\n", i);
            s32Ret = MI_ERR_WARP_CHN_NOT_CLOSE;
            goto exit_device;
        }

        pstInputPort = &pstDev->stChannel[i].stInputPort;
        if(pstInputPort->eStatus == E_MI_WARP_INPUTPORT_ENABLED)
        {
            DBG_ERR("Inputport is still enabled\n");
            s32Ret = MI_ERR_WARP_PORT_NOT_DISABLE;
            goto exit_device;
        }

        if(pstInputPort->bBind)
        {
            DBG_ERR("Inputport is still bound\n");
            s32Ret = MI_ERR_WARP_PORT_NOT_UNBIND;
            goto exit_device;
        }

        pstInputPort = &pstDev->stChannel[i].stInputPort;
        if(pstInputPort->eStatus != E_MI_WARP_INPUTPORT_UNINIT)
        {
            pstInputPort->eStatus = E_MI_WARP_INPUTPORT_UNINIT;
        }

        pstDev->stChannel[i].stOutputPort.bInited = FALSE;

        mutex_destroy(&pstDev->stChannel[i].mtxWarpConf);
    }

#if TEST_DEV_WORKTHREAD
    if(pstDev->work_thread)
    {
        kthread_stop(pstDev->work_thread);
    }
#endif

    // free irq
//    MHAL_WARP_GetIrqNum(&u32IrqId);
    free_irq(u32IrqId, pstDev->hMhalHandle);

    if (MHAL_SUCCESS != MHAL_WARP_DestroyDevice(pstDev->hMhalHandle))
    {
        DBG_ERR("Mhal destroy dev %d failed\n", devId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

    MI_SYS_BUG_ON(pstDev->hDevHandle == NULL);
    mi_sys_UnRegisterDev(pstDev->hDevHandle);

    pstDev->eStatus = E_MI_WARP_DEVICE_UNINIT;      // devStatus  start
    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;

}

MI_S32 MI_WARP_IMPL_StartDev(MI_WARP_DEV devId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid devId=%d\n", devId);
        return MI_ERR_WARP_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_DEVICE_UNINIT == pstDev->eStatus)
    {
        DBG_ERR("Device not open\n");
        s32Ret = MI_ERR_WARP_DEV_NOT_OPEN;
        goto exit_device;
    }

    if(E_MI_WARP_DEVICE_START != pstDev->eStatus)
    {
        clear_bit(E_MI_WARP_WORKER_PAUSED, (unsigned long *)&pstDev->u32WakeEvent);
#if TEST_DEV_WORKTHREAD
        wake_up_process(pstDev->work_thread);
#endif
        pstDev->eStatus = E_MI_WARP_DEVICE_START;
    }

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_StopDev(MI_WARP_DEV devId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId))
    {
        DBG_ERR("Invalid devId=%d\n", devId);
        return MI_ERR_WARP_INVALID_DEVID;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_DEVICE_UNINIT == pstDev->eStatus)
    {
        DBG_ERR("Device not open\n");
        s32Ret = MI_ERR_WARP_DEV_NOT_OPEN;
        goto exit_device;
    }

    if(E_MI_WARP_DEVICE_START == pstDev->eStatus)
    {
        set_bit(E_MI_WARP_WORKER_PAUSE, (unsigned long *)&pstDev->u32WakeEvent);
        smp_mb();

        while(!test_bit(E_MI_WARP_WORKER_PAUSED, (unsigned long *)&pstDev->u32WakeEvent))
        {
#if TEST_DEV_WORKTHREAD
            wake_up_process(pstDev->work_thread);
#endif
            msleep(1);
        }
        pstDev->eStatus = E_MI_WARP_DEVICE_STOP;
    }
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

// channel and input/output port disabled defaultly
MI_S32 MI_WARP_IMPL_CreateChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_PHY phyAddr)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    //MI_S32 i;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    //init instance isp proc sema
    SEMA_INIT(&pstDev->stChannel[chnId].semTriggerConf);
#if TEST_CHN_SINGLE_WORKTHREAD
    SEMA_INIT(&pstDev->stChannel[chnId].semChnTest);
#else
    SEMA_INIT(&pstDev->stChannel[chnId].semChnInputBuf);
    SEMA_INIT(&pstDev->stChannel[chnId].semChnOutputBuf);
#endif

    // initial get/put bufQueue
#if TEST_CHN_SINGLE_WORKTHREAD
#else
    MI_INITIAL_BUF_QUEUE(pstDev->stChannel[chnId].stGetQueue);
    MI_INITIAL_BUF_QUEUE(pstDev->stChannel[chnId].stPutQueue);
#endif


#if WARP_DEBUG_INFO
    DBG_ERR("step 1..............\n ");
#endif

    mutex_lock(&pstDev->mtx);
    if (pstDev->stChannel[chnId].eChnStatus != E_MI_WARP_CHN_UNINIT)
    {
        DBG_ERR("device %d chn %d has been Created\n", devId);
        s32Ret = MI_ERR_WARP_CHN_OPENED;
        goto exit_device;
    }

    if (MHAL_SUCCESS != MHAL_WARP_CreateInstance(pstDev->hMhalHandle, &pstDev->stChannel[chnId].hInstHandle))
    {
        DBG_ERR("device %d chn %d fail to create instance in HAL layer.\n", devId, chnId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

#if WARP_DEBUG_INFO
        DBG_ERR("step 2..............\n ");
#endif

    pstDev->stChannel[chnId].u32DevId = devId;
    pstDev->stChannel[chnId].u32ChnId = chnId;
    pstDev->stChannel[chnId].phyConfAddr = phyAddr;
    pstDev->stChannel[chnId].virConfAddr = (MI_VIRT)mi_sys_Vmap(phyAddr, sizeof(MHAL_WARP_CONFIG), FALSE);
    if (!pstDev->stChannel[chnId].virConfAddr)
    {
        DBG_ERR("device %d chn %d sys map error!\n", devId, chnId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

    pstDev->stChannel[chnId].eChnStatus = E_MI_WARP_CHN_INIT;
    pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_WARP_INPUTPORT_INIT;
    pstDev->stChannel[chnId].stOutputPort.bInited = FALSE;

#if WARP_DEBUG_INFO
        DBG_ERR("step 3..............\n ");
#endif

    // create chn workthread
#if TEST_CHN_SINGLE_WORKTHREAD
    pstDev->stChannel[chnId].TestThread = kthread_run(_MI_WARP_TestThread, &pstDev->stChannel[chnId], "WarpTest-Chn%d", chnId);
    if(IS_ERR(pstDev->stChannel[chnId].TestThread))
    {
        DBG_ERR("Create test thread fail\n");
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }
#else
    pstDev->stChannel[chnId].getInBufThread = kthread_run(_MI_WARP_GetInputBufThread, &pstDev->stChannel[chnId], "WarpGet-Chn%d", chnId);
    if(IS_ERR(pstDev->stChannel[chnId].getInBufThread))
    {
        DBG_ERR("Create get thread fail\n");
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

    pstDev->stChannel[chnId].putOutBufThread = kthread_run(_MI_WARP_PutOutputBufThread, &pstDev->stChannel[chnId], "WarpPut-Chn%d", chnId);
    if(IS_ERR(pstDev->stChannel[chnId].putOutBufThread))
    {
        DBG_ERR("Create put thread fail\n");
        s32Ret = MI_ERR_WARP_FAIL;
        goto fail_stopthread;
    }
#endif

    s32Ret = MI_WARP_OK;
    goto exit_device;

#if TEST_CHN_SINGLE_WORKTHREAD
#else
fail_stopthread:
    kthread_stop(pstDev->stChannel[chnId].getInBufThread);
#endif

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_DestroyChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_CHN_UNINIT == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d  chn not inited\n", devId, chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    if (E_MI_WARP_CHN_ENABLED == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d  is working\n", devId, chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_STOP;
        goto exit_device;
    }

    if (pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        DBG_ERR("device %d chn %d outputport is working\n", devId, chnId);
        s32Ret = MI_ERR_WARP_PORT_NOT_DISABLE;
        goto exit_device;
    }

    if (E_MI_WARP_INPUTPORT_ENABLED == pstDev->stChannel[chnId].stInputPort.eStatus)
    {
        DBG_ERR("device %d chn %d inputport is working\n", devId, chnId);
        s32Ret = MI_ERR_WARP_PORT_NOT_DISABLE;
        goto exit_device;
    }

    // stop chn work thread
#if TEST_CHN_SINGLE_WORKTHREAD
    if(pstDev->stChannel[chnId].TestThread)
    {
        kthread_stop(pstDev->stChannel[chnId].TestThread);
    }
#else
    if(pstDev->stChannel[chnId].getInBufThread)
    {
        kthread_stop(pstDev->stChannel[chnId].getInBufThread);
    }

    if(pstDev->stChannel[chnId].putOutBufThread)
    {
        kthread_stop(pstDev->stChannel[chnId].putOutBufThread);
    }
#endif

    if (MHAL_SUCCESS != MHAL_WARP_DestroyInstance(pstDev->stChannel[chnId].hInstHandle))
    {
        DBG_ERR("device %d chn %d fail to destroy instance in HAL layer.\n", devId, chnId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }

    if (pstDev->stChannel[chnId].virConfAddr)
    {
        mi_sys_UnVmap((void*)pstDev->stChannel[chnId].virConfAddr);
    }

    pstDev->stChannel[chnId].hInstHandle = NULL;
    pstDev->stChannel[chnId].virConfAddr = 0;
    pstDev->stChannel[chnId].phyConfAddr = 0;
    pstDev->stChannel[chnId].eChnStatus = E_MI_WARP_CHN_UNINIT;

    pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_WARP_INPUTPORT_UNINIT;
    pstDev->stChannel[chnId].stOutputPort.bInited = FALSE;

    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);

exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_WARP_IMPL_TriggerChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MHAL_WARP_CONFIG *pstConfig = NULL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_CHN_UNINIT == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d  chn not inited\n", devId, chnId);
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit_device;
    }

    // read config buf
    mutex_lock(&pstDev->stChannel[chnId].mtxWarpConf);
    pstConfig = (MHAL_WARP_CONFIG*)pstDev->stChannel[chnId].virConfAddr;
    mutex_unlock(&pstDev->stChannel[chnId].mtxWarpConf);

    // triggle to process image
    if (MHAL_SUCCESS != MHAL_WARP_Trigger(pstDev->stChannel[chnId].hInstHandle, pstConfig
                                          , (MHAL_WARP_CALLBACK)_MI_WARP_Trigger_Callback
                                          , &pstDev->stChannel[chnId].semTriggerConf))
    {
        DBG_ERR("device %d chn %d fail to destroy instance in HAL layer.\n", devId, chnId);
        s32Ret = MI_ERR_WARP_FAIL;
        goto exit_device;
    }
    down(&pstDev->stChannel[chnId].semTriggerConf);

    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);

exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_WARP_IMPL_EnableChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if (!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if (!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    DBG_ERR("pstDev %p\n", pstDev);

    mutex_lock(&pstDev->mtx);
    DBG_ERR("enter mutex\n");
    if (E_MI_WARP_CHN_UNINIT == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d not inited\n", devId, chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    DBG_ERR("chn states %d\n", pstDev->stChannel[chnId].eChnStatus);
    if (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus)
    {
#if WARP_DEBUG_INFO
        DBG_ERR("step 1: devhandle %p, chn %d\n ", pstDev->hDevHandle, chnId);
#endif

        mi_sys_EnableChannel(pstDev->hDevHandle, chnId);
#if TEST_CHN_SINGLE_WORKTHREAD
    wake_up_process(pstDev->stChannel[chnId].TestThread);
#else
    wake_up_process(pstDev->stChannel[chnId].getInBufThread);
    wake_up_process(pstDev->stChannel[chnId].putOutBufThread);
#endif
    pstDev->stChannel[chnId].eChnStatus = E_MI_WARP_CHN_ENABLED;


#if WARP_DEBUG_INFO
        DBG_ERR("step 2: devhandle %p, chn %d\n ", pstDev->hDevHandle, chnId);
#endif
    }
    s32Ret = MI_WARP_OK;

exit_device:
    DBG_ERR("leave mutex\n");
    mutex_unlock(&pstDev->mtx);

exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_DisableChannel(MI_WARP_DEV devId, MI_WARP_CHN chnId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
//    MI_S32 i;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d\n", devId, chnId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    mutex_lock(&pstDev->mtx);

    if (E_MI_WARP_CHN_UNINIT == pstDev->stChannel[chnId].eChnStatus)
    {
        DBG_ERR("device %d chn %d is not inited\n", devId, chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    if (pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        DBG_ERR("device %d chn %d outputport is working\n", devId, chnId);
        s32Ret = MI_ERR_WARP_PORT_NOT_DISABLE;
        goto exit_device;
    }

    if (E_MI_WARP_INPUTPORT_ENABLED == pstDev->stChannel[chnId].stInputPort.eStatus)
    {
        DBG_ERR("device %d chn %d inputport is working\n", devId, chnId);
        s32Ret = MI_ERR_WARP_PORT_NOT_DISABLE;
        goto exit_device;
    }

    if (E_MI_WARP_CHN_ENABLED == pstDev->stChannel[chnId].eChnStatus)
    {
        mi_sys_DisableChannel(pstDev->hDevHandle, chnId);
        pstDev->stChannel[chnId].eChnStatus = E_MI_WARP_CHN_DISABLED;
    }

    if (E_MI_WARP_INPUTPORT_ENABLED == pstDev->stChannel[chnId].stInputPort.eStatus)
    {
        pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_WARP_INPUTPORT_DISABLED;
    }

    pstDev->stChannel[chnId].stOutputPort.bInited = FALSE;

    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);

exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_EnableInputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    mutex_lock(&pstDev->mtx);
    if (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus
        || (E_MI_WARP_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus))
    {
        DBG_ERR("Chn %d or inputport not inited\n", chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    mi_sys_EnableInputPort(pstDev->hDevHandle, chnId, portId);
    if(pstDev->stChannel[chnId].stInputPort.eStatus != E_MI_WARP_INPUTPORT_ENABLED)
    {
        pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_WARP_INPUTPORT_ENABLED;
    }
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_DisableInputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    mutex_lock(&pstDev->mtx);
    if (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus || (E_MI_WARP_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus))
    {
        DBG_ERR("Input port not inited\n");
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    mi_sys_DisableInputPort(pstDev->hDevHandle, chnId, portId);
    if(pstDev->stChannel[chnId].stInputPort.eStatus == E_MI_WARP_INPUTPORT_ENABLED)
    {
        pstDev->stChannel[chnId].stInputPort.eStatus = E_MI_WARP_INPUTPORT_DISABLED;
    }
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_EnableOutputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    mutex_lock(&pstDev->mtx);
    if (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus
        || (E_MI_WARP_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus))
    {
        DBG_ERR("Chn %d or inputport not inited\n", chnId);
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    mi_sys_EnableOutputPort(pstDev->hDevHandle, chnId, portId);
    if(!pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        pstDev->stChannel[chnId].stOutputPort.bInited = TRUE;
    }
    s32Ret = MI_WARP_OK;

exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_DisableOutputPort(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);

    mutex_lock(&pstDev->mtx);
    if (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus || (E_MI_WARP_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus))
    {
        DBG_ERR("Input port not inited\n");
        s32Ret = MI_ERR_WARP_CHN_NOT_OPEN;
        goto exit_device;
    }

    mi_sys_DisableOutputPort(pstDev->hDevHandle, chnId, portId);
    if(pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        pstDev->stChannel[chnId].stOutputPort.bInited = FALSE;
    }

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}



MI_S32 MI_WARP_IMPL_SetInputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_InputPortAttr_t *pstInputPortAttr)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;
    MI_WARP_Inputport_t *pstInputPort;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    pstInputPort = &pstDev->stChannel[chnId].stInputPort;

    if(E_MI_WARP_DEVICE_UNINIT == pstDev->eStatus)
    {
        DBG_ERR("device %d not open\n", devId);
        s32Ret = MI_ERR_WARP_DEV_NOT_OPEN;
        goto exit_device;
    }

    pstInputPort->stAttr = *pstInputPortAttr;
    //overlay inputport use normal memory allocator

    if(pstInputPort->eStatus == E_MI_WARP_INPUTPORT_UNINIT)
    {
        pstInputPort->eStatus = E_MI_WARP_INPUTPORT_INIT;
    }

#if WARP_DEBUG_INFO
    DBG_ERR("Set inputport attr\n");
#endif

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_GetInputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_InputPortAttr_t *pstInputPortAttr)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret=-1;

#if WARP_DEBUG_INFO
            DBG_ERR("Enter get inputport attr\n");
#endif

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_INPUTPORT_UNINIT == pstDev->stChannel[chnId].stInputPort.eStatus)
    {
        DBG_ERR("Input port not inited\n");
        s32Ret = MI_ERR_WARP_NOT_CONFIG;
        goto exit_device;
    }
    *pstInputPortAttr = pstDev->stChannel[chnId].stInputPort.stAttr;


    DBG_ERR("In wid:   %d\n", pstInputPortAttr->u16Width);
    DBG_ERR("In hgt:   %d\n", pstInputPortAttr->u16Height);
    DBG_ERR("In fmt:   %d\n", (MI_S32)pstInputPortAttr->ePixelFormat);
    DBG_ERR("In compress:   %d\n", (MI_S32)pstInputPortAttr->eCompressMode);

#if WARP_DEBUG_INFO
        DBG_ERR("Get inputport attr\n");
#endif

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}

MI_S32 MI_WARP_IMPL_SetOutputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(E_MI_WARP_DEVICE_UNINIT == pstDev->eStatus || (E_MI_WARP_CHN_ENABLED != pstDev->stChannel[chnId].eChnStatus))
    // debug here
    {
        DBG_ERR("device %d is not open or chn is not enable\n", devId);
        s32Ret = MI_ERR_WARP_DEV_NOT_OPEN;
        goto exit_device;
    }

    pstDev->stChannel[chnId].stOutputPort.stAttr = *pstOutputPortAttr;
//    pstDev->stChannel[chnId].stOutputPort[portId].u64Interval = pstOutputPortAttr->u32FrmRate?(1000000/pstOutputPortAttr->u32FrmRate):(1000000/30);
    pstDev->stChannel[chnId].stOutputPort.bInited = 1;

#if WARP_DEBUG_INFO
    DBG_ERR("Set outputport attr\n");
#endif

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);

    return s32Ret;
}

MI_S32 MI_WARP_IMPL_GetOutputPortAttr(MI_WARP_DEV devId, MI_WARP_CHN chnId, MI_WARP_PORT portId, MI_WARP_OutputPortAttr_t *pstOutputPortAttr)
{
    MI_WARP_Device_t *pstDev = NULL;
    MI_S32 s32Ret = MI_ERR_WARP_FAIL;

#if WARP_DEBUG_INFO
                DBG_ERR("Enter get inputport attr\n");
#endif

    if(!MI_WARP_VALID_DEVID(devId) || !MI_WARP_VALID_CHNID(chnId) || !MI_WARP_VALID_OUTPUTPORTID(portId))
    {
        DBG_ERR("Invalid Param: devId=%d, Chn=%d, OutputPortId=%d\n", devId, chnId, portId);
        return MI_ERR_WARP_ILLEGAL_PARAM;
    }

    down(&module_sem);
    if(!_stWarpModule.bInited)
    {
        DBG_ERR("Module not inited\n");
        s32Ret = MI_ERR_WARP_MOD_NOT_INIT;
        goto exit;
    }

    pstDev = &(_stWarpModule.stDev[devId]);
    mutex_lock(&pstDev->mtx);
    if(!pstDev->stChannel[chnId].stOutputPort.bInited)
    {
        DBG_ERR("Output port not inited\n");
        s32Ret = MI_ERR_WARP_NOT_CONFIG;
        goto exit_device;
    }
    *pstOutputPortAttr = pstDev->stChannel[chnId].stOutputPort.stAttr;

    DBG_ERR("Out wid:   %d\n", pstOutputPortAttr->u16Width);
    DBG_ERR("Out hgt:   %d\n", pstOutputPortAttr->u16Height);
    DBG_ERR("Out fmt:   %d\n", (MI_S32)pstOutputPortAttr->ePixelFormat);
    DBG_ERR("Out compress:   %d\n", (MI_S32)pstOutputPortAttr->eCompressMode);

#if WARP_DEBUG_INFO
        DBG_ERR("Get outputport attr\n");
#endif

    s32Ret = MI_WARP_OK;
exit_device:
    mutex_unlock(&pstDev->mtx);
exit:
    up(&module_sem);
    return s32Ret;
}




