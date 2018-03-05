#ifndef __KERNEL__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#endif

#include "mi_syscall.h"
#include "mi_print.h"
#include "mi_cv.h"
#include "cv_ioctl.h"
#include "list.h"
#include "mi_sys.h"
#include "mi_gfx.h"


MI_MODULE_DEFINE(cv);


#define MI_CV_DUMP_DATA
#define MI_CV_DEBUG_INFO  1

#if defined(MI_CV_DEBUG_INFO)&&(MI_CV_DEBUG_INFO==1)
#define MI_CV_FUNC_ENTRY()        if(1){DBG_INFO("%s Function In\n", __func__);}
#define MI_CV_FUNC_EXIT()         if(1){DBG_INFO("%s Function Exit\n", __func__);}
#define MI_CV_FUNC_ENTRY2(ViChn)  if(1){DBG_INFO("%s Function In:chn=%d\n", __func__,ViChn);}
#define MI_CV_FUNC_EXIT2(ViChn)   if(1){DBG_INFO("%s Function Exit:chn=%d\n", __func__, ViChn);}
#else
#define MI_CV_FUNC_ENTRY()
#define MI_CV_FUNC_EXIT()
#define MI_CV_FUNC_ENTRY2(ViChn)
#define MI_CV_FUNC_EXIT2(ViChn)
#endif



#define MI_CV_CHECK_DEVID_VALID(id) do {  \
                        if (id >= MI_CV_DEVICE_MAX_NUM) \
                        { \
                           return MI_ERR_CV_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CV_CHECK_CHNID_VALID(chn) do {  \
                        if (chn >= MI_CV_CHANNEL_MAX_NUM) \
                        { \
                           return MI_ERR_CV_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CV_CHECK_PORTID_VALID(type, port) do {  \
                        if ( ((type == E_MI_CV_PORT_INPUT) && port) \
                            || ((type == E_MI_CV_PORT_OUTPUT) && port))   \
                        { \
                           return MI_ERR_CV_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CV_CHECK_CMD_VALID(cmd) do {  \
                        if (cmd >= MI_CV_CMD_MAX_NUM) \
                        { \
                           return MI_ERR_CV_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CV_CHECK_MULL_POINT(ptr) do {  \
                        if (!ptr) \
                        { \
                           return MI_ERR_CV_ILLEGAL_PARAM; \
                        } \
                    } while(0);


//static mi_cv_module_t _stCvModule;

/*
void* _MI_CV_AppTask(void *pData)
{
    return NULL;


    mi_cv_device_t *pstDev = (mi_cv_device_t*)pData;
    MI_BOOL bThreadRun = FALSE;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
//    MI_CV_ImageData_t *pstImageData = NULL;
    MI_S32 s32Ret =  MI_ERR_CV_FAIL;
    MI_S32 i;

    // test
    MI_SYS_BufInfo_t stBufInfoOut;
    MI_SYS_BUF_HANDLE hSysBufOut;
    MI_BOOL bGetInBuf = FALSE;
    MI_BOOL bGetOutBuf = FALSE;
    MI_U16 u16Fence = 0;
    MI_GFX_Surface_t stSrc;
    MI_GFX_Rect_t stSrcRect;
    MI_GFX_Surface_t stDst;
    MI_GFX_Rect_t stDstRect;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u32Flags = 0;
    stBufConf.u64TargetPts = MI_SYS_INVALID_PTS;
    stBufConf.stFrameCfg.eFormat=E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = 1280;
    stBufConf.stFrameCfg.u16Height = 720;

    for (i = 0; i < MI_CV_CHANNEL_MAX_NUM; i++)
    {
        if (pstDev->stChannel[i].stChnPort)

    }
    MI_SYS_SetChnOutputPortDepth(&pstDev->stChannel[i].stChnPort, 3, 5);
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufInfoOut, 0, sizeof(MI_SYS_BufInfo_t));

    while (1)
    {
        pthread_mutex_lock(&pstDev->mtxThreadRun);
        bThreadRun = pstDev->bThreadRun;
        pthread_mutex_unlock(&pstDev->mtxThreadRun);

        if (!bThreadRun)
        {
            sleep(1);
            continue;
        }

        pthread_mutex_lock(&pstDev->mtxDev);

        switch (pstDev->u32CvCmd)
        {
            case 0: // prettify
                _MI_CV_PrettifyFlow();
                break;
            case 1: // sink
                _MI_CV_SinkFlow();
                break;

            case 2: // stitching
                _MI_CV_StitchingFlow();
                break;
            default:
                printf("err cmd\n");
                break;
        }

        if (!bGetInBuf)
        {
            s32Ret = MI_SYS_ChnInputPortGetBuf(&pstChnContext->stChnPort, &stBufConf, &stBufInfo, &hSysBuf, 20);
            printf("step 1: get inputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnContext->mtxChnInBuf);
                continue;
            }
            bGetInBuf = TRUE;
        }

        // test
        if (!bGetOutBuf)
        {
            s32Ret = MI_SYS_ChnOutputPortGetBuf(&pstChnContext->stChnPort, &stBufInfoOut, &hSysBufOut);
            printf("step 2: get outputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnContext->mtxChnInBuf);
                continue;
            }
            bGetOutBuf = TRUE;
        }

        // copy inbuf to outbuf
        printf("step 3: gfx copy data\n");
        stSrcRect.s32Xpos = 0;
        stSrcRect.u32Height = (MI_U32)(stBufInfo.stFrameData.u16Height);
        stSrcRect.s32Ypos = 0;
        stSrcRect.u32Width = (MI_U32)(stBufInfo.stFrameData.u16Width);

        stSrc.eColorFmt = E_MI_GFX_FMT_YUV422;
        stSrc.phyAddr = stBufInfo.stFrameData.phyAddr[0];
        stSrc.u32Height = stSrcRect.u32Height;
        stSrc.u32Width = stSrcRect.u32Width;
        stSrc.u32Stride = stBufInfo.stFrameData.u32Stride[0];

        memset(&stDst, 0, sizeof(stDst));
        stDst.eColorFmt = E_MI_GFX_FMT_YUV422;
        stDst.phyAddr   = stBufInfoOut.stFrameData.phyAddr[0];
        stDst.u32Stride   = stBufInfoOut.stFrameData.u32Stride[0];
        stDst.u32Height = stBufInfoOut.stFrameData.u16Height;
        stDst.u32Width  = stBufInfoOut.stFrameData.u16Width;

        memset(&stDstRect, 0, sizeof(stDstRect));
        stDstRect.s32Xpos = 0;
        stDstRect.u32Width = stDst.u32Width;
        stDstRect.s32Ypos = 0;
        stDstRect.u32Height = stDst.u32Height;
        stBufInfoOut.bEndOfStream = stBufInfo.bEndOfStream;

        if(MI_SUCCESS != MI_GFX_BitBlit(&stSrc, &stSrcRect, &stDst, &stDstRect, NULL, &u16Fence))
        {
              DBG_ERR("[%s %d] MI_GFX_BitBlit fail!!!\n", __FUNCTION__, __LINE__);
        }

        MI_GFX_WaitAllDone(FALSE, u16Fence);
        stBufInfoOut.u64Pts = stBufInfo.u64Pts;


        s32Ret = MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo, TRUE);
        printf("step 4: put inputport buff, ret %d\n", (s32Ret == MI_SUCCESS));

        s32Ret = MI_SYS_ChnOutputPortPutBuf(hSysBufOut);
        printf("step 5: put outputport buff, ret %d\n", (s32Ret == MI_SUCCESS));
        pthread_mutex_unlock(&pstChnContext->mtxChnInBuf);
    }
    return NULL;

}
*/


MI_S32 MI_CV_CreateDev(MI_U32 u32DevId)
{
//    MI_U32 u32CevaChn = 0;
//    MI_U8 szThreadName[32];
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CV_FUNC_ENTRY();
    MI_CV_CHECK_DEVID_VALID(u32DevId);

    s32Ret = MI_SYSCALL(MI_CV_CREATE_DEV, &u32DevId);
/*
    if (MI_CV_OK != s32Ret)
    {
        printf("Create dev %d failed", u32DevId);
        return s32Ret;
    }

    _stCvModule.astDev[u32DevId].u32DevId = u32DevId;
    _stCvModule.astDev[u32DevId].pfCallback = NULL;
    _stCvModule.astDev[u32DevId].u32CvCmd = 0;

    for(u32CevaChn = 0; u32CevaChn < MI_CV_CHANNEL_MAX_NUM; u32CevaChn++)
    {
        _stCvModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.eModId = E_MI_MODULE_ID_CV;
        _stCvModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32DevId = u32DevId;
        _stCvModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32ChnId = u32CevaChn;
        _stCvModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32PortId = 0;
    }

    pthread_mutex_init(&_stCvModule.astDev[u32DevId].mtxDev, NULL);
    pthread_mutex_init(&_stCvModule.astDev[u32DevId].mtxThreadExit, NULL);
    pthread_cond_init(&_stCvModule.astDev[u32DevId].condThreadExit, NULL);
    pthread_mutex_init(&_stCvModule.astDev[u32DevId].mtxThreadRun, NULL);


    memset(szThreadName, 0, sizeof(szThreadName));
    sprintf(szThreadName, "CV_AppThread_dev%02d", u32DevId);
    pthread_create(&_stCvModule.astDev[u32DevId].pthreadWork, NULL, _MI_CV_AppTask, &_stCvModule.astDev[u32DevId]);
    pthread_setname_np(_stCvModule.astDev[u32DevId].pthreadWork, szThreadName);
    _stCvModule.astDev[u32DevId].bThreadRun = TRUE;
    _stCvModule.astDev[u32DevId].bThreadExit = FALSE;
*/

    MI_CV_FUNC_EXIT();

    return s32Ret;
}

MI_S32 MI_CV_DestroyDev(MI_U32 u32DevId)
{
//    MI_U32 u32CevaChn = 0;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CV_FUNC_ENTRY();
    MI_CV_CHECK_DEVID_VALID(u32DevId);

    s32Ret = MI_SYSCALL(MI_CV_DESTROY_DEV, &u32DevId);
/*
    _stCvModule.astDev[u32DevId].bThreadExit = TRUE;
    pthread_join(_stCvModule.astDev[u32DevId].pthreadWork, NULL);
    _stCvModule.astDev[u32DevId].bThreadRun = FALSE;
    _stCvModule.astDev[u32DevId].pfCallback = NULL;
    _stCvModule.astDev[u32DevId].u32CvCmd = 0;

    s32Ret = MI_SYSCALL(MI_CV_DESTROY_DEV, &u32DevId);
    if (MI_CV_OK != s32Ret)
    {
        return MI_ERR_CV_FAIL;
    }

    pthread_mutex_destroy(&_stCvModule.astDev[u32DevId].mtxDev);
    pthread_mutex_destroy(&_stCvModule.astDev[u32DevId].mtxThreadRun);
    pthread_mutex_destroy(&_stCvModule.astDev[u32DevId].mtxThreadExit);
    pthread_cond_destroy(&_stCvModule.astDev[u32DevId].condThreadExit);
*/
    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_CreateChn(MI_U32 u32DevId, MI_U32 u32ChnId)
{
    MI_CV_CreateChannel_t stCreateChn;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CV_FUNC_ENTRY();
    MI_CV_CHECK_DEVID_VALID(u32DevId);
    MI_CV_CHECK_CHNID_VALID(u32ChnId);

    stCreateChn.devId = u32DevId;
    stCreateChn.chnId = u32ChnId;
    s32Ret = MI_SYSCALL(MI_CV_CREATE_CHANNEL, &stCreateChn);

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_DestroyChn(MI_U32 u32DevId, MI_U32 u32ChnId)
{
    MI_CV_DestroyChannel_t stDestroyChn;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CV_FUNC_ENTRY();
    MI_CV_CHECK_DEVID_VALID(u32DevId);
    MI_CV_CHECK_CHNID_VALID(u32ChnId);

    stDestroyChn.devId = u32DevId;
    stDestroyChn.chnId = u32ChnId;
    s32Ret = MI_SYSCALL(MI_CV_DESTROY_CHANNEL, &stDestroyChn);

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_SetInputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CV_InputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_SetInputPortAttr_t stSetInputPortAttr;
    MI_CV_FUNC_ENTRY();

    memset(&stSetInputPortAttr, 0, sizeof(stSetInputPortAttr));
    stSetInputPortAttr.devId = u32DevId;
    stSetInputPortAttr.chnId = u32ChnId;
    stSetInputPortAttr.portId = u32PortId;
    stSetInputPortAttr.stInputPortAttr = *pstAttr;
    s32Ret = MI_SYSCALL(MI_CV_SETINPUTPORTATTR, &stSetInputPortAttr);

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_GetInputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CV_InputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_GetInputPortAttr_t stGetInputPortAttr;
    MI_CV_FUNC_ENTRY();

    memset(&stGetInputPortAttr, 0, sizeof(stGetInputPortAttr));
    stGetInputPortAttr.devId = u32DevId;
    stGetInputPortAttr.chnId = u32ChnId;
    stGetInputPortAttr.portId = u32PortId;
    s32Ret = MI_SYSCALL(MI_CV_GETINPUTPORTATTR, &stGetInputPortAttr);

    if (MI_CV_OK == s32Ret)
    {
        memcpy(pstAttr, &stGetInputPortAttr.stInputPortAttr, sizeof(MI_CV_InputPortAttr_t));
    }

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_SetOutputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CV_OutputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_SetOutputPortAttr_t stSetOutputPortAttr;
    MI_CV_FUNC_ENTRY();

    memset(&stSetOutputPortAttr, 0, sizeof(stSetOutputPortAttr));
    stSetOutputPortAttr.devId = u32ChnId;
    stSetOutputPortAttr.chnId = u32ChnId;
    stSetOutputPortAttr.portId = u32PortId;
    stSetOutputPortAttr.stOutputPortAttr = *pstAttr;
    s32Ret = MI_SYSCALL(MI_CV_SETOUTPUTPORTATTR, &stSetOutputPortAttr);

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_GetOutputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CV_OutputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_GetOutputPortAttr_t stGetOutputPortAttr;
    MI_CV_FUNC_ENTRY();

    memset(&stGetOutputPortAttr, 0, sizeof(stGetOutputPortAttr));
    stGetOutputPortAttr.devId = u32DevId;
    stGetOutputPortAttr.chnId = u32ChnId;
    stGetOutputPortAttr.portId = u32PortId;
    s32Ret = MI_SYSCALL(MI_CV_GETOUTPUTPORTATTR, &stGetOutputPortAttr);

    if (MI_CV_OK == s32Ret)
    {
        memcpy(pstAttr, &stGetOutputPortAttr.stOutputPortAttr, sizeof(MI_CV_OutputPortAttr_t));
    }

    MI_CV_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CV_GetCdnnCfgBuf(void *pAddr, MI_U32 u32Size)           // 5M
{
    MI_S32 s32Ret = MI_CV_OK;
    MI_CV_GetCdnnCfgBuf_t stGetCdnnCfgBuf;
    MI_CV_FUNC_ENTRY();

    memset(&stGetCdnnCfgBuf, 0, sizeof(stGetCdnnCfgBuf));
    stGetCdnnCfgBuf.u32Size = u32Size;
    s32Ret = MI_SYSCALL(MI_CV_GET_CDNN_CFG_BUF, &stGetCdnnCfgBuf);

    if (MI_CV_OK == s32Ret)
    {
        if (MI_SUCCESS != MI_SYS_Mmap(stGetCdnnCfgBuf.phyAddr, u32Size, &pAddr, FALSE) || !pAddr)
        {
            s32Ret = MI_ERR_CV_FAIL;
        }
    }
    else
    {
        pAddr = NULL;
        s32Ret = MI_ERR_CV_FAIL;
    }

    MI_CV_FUNC_EXIT();
    return s32Ret;
}



