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

#include "mi_print.h"
#include "mi_ceva.h"
#include "mi_cv.h"
#include "list.h"
#include "mi_sys.h"
#include "mi_gfx.h"


//MI_MODULE_DEFINE(ceva);


#define MI_CEVA_DUMP_DATA
#define MI_CEVA_DEBUG_INFO  1

#if defined(MI_CEVA_DEBUG_INFO)&&(MI_CEVA_DEBUG_INFO==1)
#define MI_CEVA_FUNC_ENTRY()        if(1){printf("%s Function In\n", __func__);}
#define MI_CEVA_FUNC_EXIT()         if(1){printf("%s Function Exit\n", __func__);}
#define MI_CEVA_FUNC_ENTRY2(ViChn)  if(1){printf("%s Function In:chn=%d\n", __func__,ViChn);}
#define MI_CEVA_FUNC_EXIT2(ViChn)   if(1){printf("%s Function Exit:chn=%d\n", __func__, ViChn);}
#else
#define MI_CEVA_FUNC_ENTRY()
#define MI_CEVA_FUNC_EXIT()
#define MI_CEVA_FUNC_ENTRY2(ViChn)
#define MI_CEVA_FUNC_EXIT2(ViChn)
#endif



#define MI_CEVA_CHECK_DEVID_VALID(id) do {  \
                        if (id >= MI_CEVA_DEVICE_MAX_NUM) \
                        { \
                           return MI_CEVA_ERR_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CEVA_CHECK_CHNID_VALID(chn) do {  \
                        if (chn >= MI_CEVA_CHANNEL_MAX_NUM) \
                        { \
                           return MI_CEVA_ERR_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CEVA_CHECK_PORTID_VALID(type, port) do {  \
                        if ( ((type == E_MI_CEVA_PORT_INPUT) && port) \
                            || ((type == E_MI_CEVA_PORT_OUTPUT) && port))   \
                        { \
                           return MI_CEVA_ERR_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CEVA_CHECK_CMD_VALID(cmd) do {  \
                        if (cmd >= MI_CEVA_CMD_MAX_NUM) \
                        { \
                           return MI_CEVA_ERR_ILLEGAL_PARAM; \
                        } \
                    } while(0);

#define MI_CEVA_CHECK_MULL_POINT(ptr) do {  \
                        if (!ptr) \
                        { \
                           return MI_CEVA_ERR_ILLEGAL_PARAM; \
                        } \
                    } while(0);

typedef struct
{
    MI_U32 u32ChnId;
    MI_BOOL bInited;
    MI_SYS_ChnPort_t stChnPort;      // 1 input 1 output

    MI_BOOL bThreadRun;
    MI_BOOL bThreadExit;
    pthread_t pthreadWork;
    pthread_mutex_t mtxChn;
    pthread_mutex_t mtxThreadRun;
    pthread_mutex_t mtxThreadExit;
    pthread_cond_t condThreadExit;
    void *pUsrData;
}mi_ceva_channel_t;

typedef struct
{
    MI_U32 u32DevId;
    mi_ceva_channel_t stChannel[MI_CEVA_CHANNEL_MAX_NUM];
    MI_CEVA_Callback pfCallback;          // isp callback
    MI_U32 u32CvCmd;
    MI_BOOL bThreadRun;
    MI_BOOL bThreadExit;
    pthread_t pthreadWork;
    pthread_mutex_t mtxDev;
    pthread_mutex_t mtxThreadRun;
    pthread_mutex_t mtxThreadExit;
    pthread_cond_t condThreadExit;
}mi_ceva_device_t;

typedef struct
{
    mi_ceva_device_t astDev[MI_CEVA_DEVICE_MAX_NUM];
}mi_ceva_module_t;

static mi_ceva_module_t _stCevaModule;

MI_S32 _MI_CEVA_PrettifyFlow(mi_ceva_channel_t *pstChnCtx, MI_CEVA_BUF_t *pCevaBufIn, MI_CEVA_BUF_t *pCevaBufOut
                             , MI_BOOL *pbGetInBuf, MI_BOOL *pbGetOutBuf)
{
    mi_ceva_device_t *pstDev = (mi_ceva_device_t*)pstChnCtx->pUsrData;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfoIn;
    MI_SYS_BUF_HANDLE hSysBufIn;
    MI_SYS_BufInfo_t stBufInfoOut;
    MI_SYS_BUF_HANDLE hSysBufOut;
    MI_S32 s32Ret = 0;
    MI_S32 i;

    pthread_mutex_lock(&pstChnCtx->mtxChn);

    if (pstChnCtx->bInited)
    {
        MI_CEVA_BUF_t *pCurCevaBufIn = pCevaBufIn + pstChnCtx->u32ChnId;
        MI_CEVA_BUF_t *pCurCevaBufOut = pCevaBufOut + pstChnCtx->u32ChnId;

        if (!*pbGetInBuf)
        {
            s32Ret = MI_SYS_ChnInputPortGetBuf(&pstChnCtx->stChnPort, &stBufConf, &stBufInfoIn, &hSysBufIn, 20);
            printf("step 1: get inputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnCtx->mtxChn);
                return MI_CEVA_ERR_FAIL;
            }
            *pbGetInBuf = TRUE;
        }

        // test
        if (!*pbGetOutBuf)
        {
            s32Ret = MI_SYS_ChnOutputPortGetBuf(&pstChnCtx->stChnPort, &stBufInfoOut, &hSysBufOut);
            printf("step 2: get outputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnCtx->mtxChn);
                return MI_CEVA_ERR_FAIL;
            }
            *pbGetOutBuf = TRUE;
        }

        pCurCevaBufIn->Width = stBufInfoIn.stFrameData.u16Width;
        pCurCevaBufIn->Height = stBufInfoIn.stFrameData.u16Height;
        pCurCevaBufIn->Format = MI_CEVA_IMAGE_FORMAT_NV16;
        MI_SYS_Mmap(stBufInfoIn.stFrameData.phyAddr[0], pCurCevaBufIn->Width*pCurCevaBufIn->Height
                    , (void**)(&pCurCevaBufIn->Buffer[0]), FALSE);
        MI_SYS_Mmap(stBufInfoIn.stFrameData.phyAddr[1], pCurCevaBufIn->Width*pCurCevaBufIn->Height
                    , (void**)(&pCurCevaBufIn->Buffer[1]), FALSE);

        pCurCevaBufOut->Width = stBufInfoOut.stFrameData.u16Width;
        pCurCevaBufOut->Height = stBufInfoOut.stFrameData.u16Height;
        pCurCevaBufOut->Format = MI_CEVA_IMAGE_FORMAT_NV16;
        MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[0], pCurCevaBufOut->Width*pCurCevaBufOut->Height
                    , (void**)(&pCurCevaBufOut->Buffer[0]), FALSE);
        MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[1], pCurCevaBufOut->Width*pCurCevaBufOut->Height
                    , (void**)(&pCurCevaBufOut->Buffer[1]), FALSE);

        s32Ret = pstDev->pfCallback(0, 1, &pCurCevaBufIn, 1, &pCurCevaBufOut);
        if (s32Ret)     // ceva process failed
        {
            stBufInfoOut.stFrameData.u16Width = stBufInfoIn.stFrameData.u16Width;
            stBufInfoOut.stFrameData.u16Height = stBufInfoIn.stFrameData.u16Height;
            stBufInfoOut.stFrameData.ePixelFormat = stBufInfoIn.stFrameData.ePixelFormat;
            memcpy(pCurCevaBufOut->Buffer[0], pCurCevaBufIn->Buffer[0]
                   , stBufInfoIn.stFrameData.u16Width * stBufInfoIn.stFrameData.u16Height);
            memcpy(pCurCevaBufOut->Buffer[1], pCurCevaBufIn->Buffer[1]
                   , stBufInfoIn.stFrameData.u16Width * stBufInfoIn.stFrameData.u16Height);
        }

        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBufIn, &stBufInfoIn, TRUE))
        {
            printf("step 4: put inputport buff failed\n");
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBufOut))
        {
            printf("step 5: put outputport buff failed\n");
        }
    }

    pthread_mutex_unlock(&pstChnCtx->mtxChn);

    return s32Ret;
}

MI_S32 _MI_CEVA_SinkFlow(mi_ceva_channel_t *pstChnCtx, MI_CEVA_BUF_t *pCevaBufIn, MI_CEVA_BUF_t *pCevaBufOut
                          , MI_BOOL *pbGetInBuf, MI_BOOL *pbGetOutBuf)
{
    mi_ceva_device_t *pstDev = (mi_ceva_device_t*)pstChnCtx->pUsrData;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfoIn;
    MI_SYS_BUF_HANDLE hSysBufIn;
    MI_SYS_BufInfo_t stBufInfoOut;
    MI_SYS_BUF_HANDLE hSysBufOut;
    void *pVirAddr[2];
    MI_S32 s32Ret = 0;
    MI_S32 i;

    pthread_mutex_lock(&pstChnCtx->mtxChn);

    if (pstChnCtx->bInited)
    {
        MI_CEVA_BUF_t *pCurCevaBufIn = pCevaBufIn + pstChnCtx->u32ChnId;
        MI_CEVA_BUF_t *pCurCevaBufOut = pCevaBufOut + pstChnCtx->u32ChnId;

        if (!*pbGetInBuf)
        {
            s32Ret = MI_SYS_ChnInputPortGetBuf(&pstChnCtx->stChnPort, &stBufConf, &stBufInfoIn, &hSysBufIn, 20);
            printf("step 1: get inputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnCtx->mtxChn);
                return MI_CEVA_ERR_FAIL;
            }
            *pbGetInBuf = TRUE;
        }

        // test
        if (!*pbGetOutBuf)
        {
            s32Ret = MI_SYS_ChnOutputPortGetBuf(&pstChnCtx->stChnPort, &stBufInfoOut, &hSysBufOut);
            printf("step 2: get outputport buff, ret %d\n", s32Ret);
            if (MI_SUCCESS != s32Ret)
            {
                pthread_mutex_unlock(&pstChnCtx->mtxChn);
                return MI_CEVA_ERR_FAIL;
            }
            *pbGetOutBuf = TRUE;
        }

        pCurCevaBufIn->Width = stBufInfoIn.stFrameData.u16Width;
        pCurCevaBufIn->Height = stBufInfoIn.stFrameData.u16Height;
        pCurCevaBufIn->Format = MI_CEVA_IMAGE_FORMAT_NV16;
        MI_SYS_Mmap(stBufInfoIn.stFrameData.phyAddr[0], pCurCevaBufIn->Width*pCurCevaBufIn->Height
                   , (void**)(&pCurCevaBufIn->Buffer[0]), FALSE);
        MI_SYS_Mmap(stBufInfoIn.stFrameData.phyAddr[1], pCurCevaBufIn->Width*pCurCevaBufIn->Height, (void**)&pCurCevaBufIn->Buffer[1], FALSE);

        pCurCevaBufOut->Width = stBufInfoOut.stFrameData.u16Width;
        pCurCevaBufOut->Height = stBufInfoOut.stFrameData.u16Height;
        pCurCevaBufOut->Format = MI_CEVA_IMAGE_FORMAT_NV16;
        // usr set cevaBufOut Buffer Addr, not processed
        MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[0], pCurCevaBufOut->Width*pCurCevaBufOut->Height, &pVirAddr[0], FALSE);
        MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[1], pCurCevaBufOut->Width*pCurCevaBufOut->Height, &pVirAddr[1], FALSE);

        s32Ret = pstDev->pfCallback(0, 1, &pCurCevaBufIn, 1, &pCurCevaBufOut);

        stBufInfoOut.stFrameData.u16Width = stBufInfoIn.stFrameData.u16Width;
        stBufInfoOut.stFrameData.u16Height = stBufInfoIn.stFrameData.u16Height;
        stBufInfoOut.stFrameData.ePixelFormat = stBufInfoIn.stFrameData.ePixelFormat;
        memcpy(pVirAddr[0], pCurCevaBufIn->Buffer[0]
               , stBufInfoIn.stFrameData.u16Width * stBufInfoIn.stFrameData.u16Height);
        memcpy(pVirAddr[1], pCurCevaBufIn->Buffer[1]
               , stBufInfoIn.stFrameData.u16Width * stBufInfoIn.stFrameData.u16Height);

        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBufIn, &stBufInfoIn, TRUE))
        {
            printf("step 4: put inputport buff failed\n");
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBufOut))
        {
            printf("step 5: put outputport buff failed\n");
        }
    }

    pthread_mutex_unlock(&pstChnCtx->mtxChn);

    return s32Ret;
}


// dev thread process
MI_S32 _MI_CEVA_StitchingFlow(mi_ceva_device_t *pstDev, MI_CEVA_BUF_t *pCevaBufIn, MI_CEVA_BUF_t *pCevaBufOut)
{
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfoIn[MI_CEVA_CHANNEL_MAX_NUM];
    MI_SYS_BUF_HANDLE hSysBufIn[MI_CEVA_CHANNEL_MAX_NUM];
    MI_SYS_BufInfo_t stBufInfoOut;
    MI_SYS_BUF_HANDLE hSysBufOut;
    MI_S32 s32Ret = 0;
    MI_U32 u32ChnCnt = 0;
    MI_S32 i;

    for (i = 0; i < MI_CEVA_CHANNEL_MAX_NUM; i++)
    {
        if (pstDev->stChannel[i].bInited)
        {

            while (MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&pstDev->stChannel[i].stChnPort
                                                           , &stBufConf, &stBufInfoIn[i], &hSysBufIn[i], 20));

            (pCevaBufIn + u32ChnCnt)->Width = stBufInfoIn[i].stFrameData.u16Width;
            (pCevaBufIn + u32ChnCnt)->Height = stBufInfoIn[i].stFrameData.u16Height;
            (pCevaBufIn + u32ChnCnt)->Format = MI_CEVA_IMAGE_FORMAT_NV16;
            MI_SYS_Mmap(stBufInfoIn[i].stFrameData.phyAddr[0], stBufInfoIn[i].stFrameData.u16Width * stBufInfoIn[i].stFrameData.u16Height
                        , (void**)&(pCevaBufIn+u32ChnCnt)->Buffer[0], FALSE);
            MI_SYS_Mmap(stBufInfoIn[i].stFrameData.phyAddr[1], stBufInfoIn[i].stFrameData.u16Width * stBufInfoIn[i].stFrameData.u16Height
                        , (void**)&(pCevaBufIn+u32ChnCnt)->Buffer[1], FALSE);
            u32ChnCnt++;
        }
    }

    // use 0 outputport
    while (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&pstDev->stChannel[i].stChnPort, &stBufInfoOut, &hSysBufOut))
    {
        usleep(1000);
    }

    pCevaBufOut->Width = stBufInfoIn[0].stFrameData.u16Width;
    pCevaBufOut->Height = stBufInfoIn[0].stFrameData.u16Height;
    pCevaBufOut->Format = MI_CEVA_IMAGE_FORMAT_NV16;
    pCevaBufOut->Width = stBufInfoIn[0].stFrameData.u16Width;
    pCevaBufOut->Width = stBufInfoIn[0].stFrameData.u16Width;
    MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[0], stBufInfoOut.stFrameData.u16Width * stBufInfoOut.stFrameData.u16Height
                , (void**)&pCevaBufOut->Buffer[0], FALSE);
    MI_SYS_Mmap(stBufInfoOut.stFrameData.phyAddr[1], stBufInfoOut.stFrameData.u16Width * stBufInfoOut.stFrameData.u16Height
                , (void**)&pCevaBufOut->Buffer[1], FALSE);

    s32Ret = pstDev->pfCallback(pstDev->u32CvCmd, u32ChnCnt, &pCevaBufIn, 1, &pCevaBufOut);
    if (s32Ret)
    {
        stBufInfoOut.stFrameData.u16Width = stBufInfoIn[0].stFrameData.u16Width;
        stBufInfoOut.stFrameData.u16Height = stBufInfoIn[0].stFrameData.u16Height;
        stBufInfoOut.stFrameData.ePixelFormat = stBufInfoIn[0].stFrameData.ePixelFormat;
        memcpy(pCevaBufOut->Buffer[0], pCevaBufIn->Buffer[0]
               , stBufInfoIn[0].stFrameData.u16Width * stBufInfoIn[0].stFrameData.u16Height);
        memcpy(pCevaBufOut->Buffer[1], pCevaBufIn->Buffer[1]
               , stBufInfoIn[0].stFrameData.u16Width * stBufInfoIn[0].stFrameData.u16Height);
    }

    for (i = 0; i < MI_CEVA_CHANNEL_MAX_NUM; i++)
    {
        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBufIn[0], &stBufInfoIn[0], TRUE))
        {
            printf("step 4: put inputport buff failed\n");
        }
    }

    if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBufOut))
    {
        printf("step 5: put outputport buff failed\n");
    }

    return s32Ret;
}

// N sensors 1 result
void* _MI_CEVA_DevThread(void *pData)
{
    mi_ceva_device_t *pstDev = (mi_ceva_device_t*)pData;
    MI_BOOL bThreadRun = FALSE;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret =  MI_CEVA_ERR_FAIL;
    MI_CEVA_BUF_t aCevaBufIn[MI_CEVA_CHANNEL_MAX_NUM];
    MI_CEVA_BUF_t CevaBufOut;
    MI_S32 i;

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

    memset(aCevaBufIn, 0, sizeof(aCevaBufIn));
    memset(&CevaBufOut, 0, sizeof(CevaBufOut));
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufInfoOut, 0, sizeof(MI_SYS_BufInfo_t));

    for (i = 0; i < MI_CEVA_CHANNEL_MAX_NUM; i++)
    {
        if (pstDev->stChannel[i].bInited)
        {
            MI_SYS_SetChnOutputPortDepth(&pstDev->stChannel[i].stChnPort, 3, 5);
        }
    }

    while (!pstDev->bThreadExit)
    {
        pthread_mutex_lock(&pstDev->mtxThreadRun);
        bThreadRun = pstDev->bThreadRun;
        pthread_mutex_unlock(&pstDev->mtxThreadRun);

        if (!bThreadRun)
        {
            sleep(10);
            continue;
        }

        pthread_mutex_lock(&pstDev->mtxDev);
        switch (pstDev->u32CvCmd)
        {
            case 2: // stitching
                s32Ret = _MI_CEVA_StitchingFlow(pstDev, aCevaBufIn, &CevaBufOut);
                break;
            default:
                printf("err cmd\n");
                break;
        }
        pthread_mutex_lock(&pstDev->mtxDev);
    }
    return NULL;


}


// 1 result each sensor
void* _MI_CEVA_ChnThread(void* pData)
{
    mi_ceva_channel_t *pstChnCtx = (mi_ceva_channel_t*)pData;
    mi_ceva_device_t *pstDev = (mi_ceva_device_t*)pstChnCtx->pUsrData;
    MI_BOOL bThreadRun = FALSE;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_S32 s32Ret =  MI_CEVA_ERR_FAIL;
    MI_CEVA_BUF_t aCevaBufIn[MI_CEVA_CHANNEL_MAX_NUM];
    MI_CEVA_BUF_t aCevaBufOut[MI_CEVA_CHANNEL_MAX_NUM];
    MI_S32 i;

    MI_SYS_BufInfo_t stBufInfoOut;
    MI_SYS_BUF_HANDLE hSysBufOut;
    MI_BOOL bGetInBuf = FALSE;
    MI_BOOL bGetOutBuf = FALSE;

    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
    stBufConf.u32Flags = 0;
    stBufConf.u64TargetPts = MI_SYS_INVALID_PTS;
    stBufConf.stFrameCfg.eFormat=E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
    stBufConf.stFrameCfg.eFrameScanMode=E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
    stBufConf.stFrameCfg.u16Width = 1280;
    stBufConf.stFrameCfg.u16Height = 720;

    memset(aCevaBufIn, 0, sizeof(aCevaBufIn));
    memset(aCevaBufOut, 0, sizeof(aCevaBufOut));
    memset(&stBufInfo, 0, sizeof(MI_SYS_BufInfo_t));
    memset(&stBufInfoOut, 0, sizeof(MI_SYS_BufInfo_t));


    if (pstChnCtx->bInited)
    {
        MI_SYS_SetChnOutputPortDepth(&pstChnCtx->stChnPort, 3, 5);
    }

    while (!pstChnCtx->bThreadExit)
    {
        pthread_mutex_lock(&pstChnCtx->mtxThreadRun);
        bThreadRun = pstChnCtx->bThreadRun;
        pthread_mutex_unlock(&pstChnCtx->mtxThreadRun);

        if (!bThreadRun)
        {
            sleep(10);
            continue;
        }

        pthread_mutex_lock(&pstChnCtx->mtxChn);
        switch (pstDev->u32CvCmd)
        {
            case 0: // prettify
                s32Ret = _MI_CEVA_PrettifyFlow(pstChnCtx, aCevaBufIn, aCevaBufOut, &bGetInBuf, &bGetOutBuf);
                break;
            case 1: // sink
                s32Ret = _MI_CEVA_SinkFlow(pstChnCtx, aCevaBufIn, aCevaBufOut, &bGetInBuf, &bGetOutBuf);
                break;
            default:
                printf("err cmd\n");
                break;
        }
        pthread_mutex_unlock(&pstChnCtx->mtxChn);
    }
    return NULL;
}


MI_S32 _MI_CEVA_SetInputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CEVA_InputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_CEVA_ERR_FAIL;
    MI_CV_InputPortAttr_t stInputPortAttr;
    MI_CEVA_FUNC_ENTRY();

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    stInputPortAttr.u16Width = pstAttr->u16Width;
    stInputPortAttr.u16Height = pstAttr->u16Height;
    stInputPortAttr.ePixelFormat = pstAttr->ePixelFormat;
    stInputPortAttr.eCompressMode = pstAttr->eCompressMode;
    s32Ret = MI_CV_SetInputPortAttr(u32DevId, u32ChnId, u32PortId, &stInputPortAttr);

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 _MI_CEVA_GetInputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CEVA_InputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_CEVA_ERR_FAIL;
    MI_CV_InputPortAttr_t stInputPortAttr;
    MI_CEVA_FUNC_ENTRY();

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));
    s32Ret = MI_CV_GetInputPortAttr(u32DevId, u32ChnId, u32PortId, &stInputPortAttr);

    if (MI_CV_OK == s32Ret)
    {
        pstAttr->u16Width = stInputPortAttr.u16Width;
        pstAttr->u16Height = stInputPortAttr.u16Height;
        pstAttr->ePixelFormat = stInputPortAttr.ePixelFormat;
        pstAttr->eCompressMode = stInputPortAttr.eCompressMode;
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 _MI_CEVA_SetOutputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CEVA_OutputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_OutputPortAttr_t stOutputPortAttr;
    MI_CEVA_FUNC_ENTRY();

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.u16Width = pstAttr->u16Width;
    stOutputPortAttr.u16Height = pstAttr->u16Height;
    stOutputPortAttr.ePixelFormat = pstAttr->ePixelFormat;
    stOutputPortAttr.eCompressMode = pstAttr->eCompressMode;
    s32Ret = MI_CV_SetOutputPortAttr(u32DevId, u32ChnId, u32PortId, &stOutputPortAttr);

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 _MI_CEVA_GetOutputPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CEVA_OutputPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_CV_OutputPortAttr_t stOutputPortAttr;
    MI_CEVA_FUNC_ENTRY();

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    s32Ret = MI_CV_GetOutputPortAttr(u32DevId, u32ChnId, u32PortId, &stOutputPortAttr);

    if (MI_CEVA_OK == s32Ret)
    {
        pstAttr->u16Width = stOutputPortAttr.u16Width;
        pstAttr->u16Height = stOutputPortAttr.u16Height;
        pstAttr->ePixelFormat = stOutputPortAttr.ePixelFormat;
        pstAttr->eCompressMode = stOutputPortAttr.eCompressMode;
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CEVA_CreateDev(MI_U32 u32DevId)
{
    MI_U32 u32CevaChn = 0;
    MI_U8 szThreadName[32];
    MI_S32 s32Ret = MI_CEVA_ERR_FAIL;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_DEVID_VALID(u32DevId);

    s32Ret = MI_CV_CreateDev(u32DevId);

    if (MI_CV_OK != s32Ret)
    {
        printf("Create dev %d failed", u32DevId);
        return MI_CEVA_ERR_FAIL;
    }

    _stCevaModule.astDev[u32DevId].u32DevId = u32DevId;
    _stCevaModule.astDev[u32DevId].pfCallback = NULL;
    _stCevaModule.astDev[u32DevId].u32CvCmd = 0;
    _stCevaModule.astDev[u32DevId].bThreadRun = FALSE;
    _stCevaModule.astDev[u32DevId].bThreadExit = FALSE;

    for(u32CevaChn = 0; u32CevaChn < MI_CEVA_CHANNEL_MAX_NUM; u32CevaChn++)
    {
        _stCevaModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.eModId = E_MI_MODULE_ID_CV;
        _stCevaModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32DevId = u32DevId;
        _stCevaModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32ChnId = u32CevaChn;
        _stCevaModule.astDev[u32DevId].stChannel[u32CevaChn].stChnPort.u32PortId = 0;
        _stCevaModule.astDev[u32DevId].stChannel[u32CevaChn].bInited = FALSE;
    }

    pthread_mutex_init(&_stCevaModule.astDev[u32DevId].mtxDev, NULL);
    pthread_mutex_init(&_stCevaModule.astDev[u32DevId].mtxThreadExit, NULL);
    pthread_cond_init(&_stCevaModule.astDev[u32DevId].condThreadExit, NULL);
    pthread_mutex_init(&_stCevaModule.astDev[u32DevId].mtxThreadRun, NULL);

    memset(szThreadName, 0, sizeof(szThreadName));
    sprintf(szThreadName, "CEVA_AppThread_dev%02d", u32DevId);
    pthread_create(&_stCevaModule.astDev[u32DevId].pthreadWork, NULL, _MI_CEVA_DevThread, &_stCevaModule.astDev[u32DevId]);
    pthread_setname_np(_stCevaModule.astDev[u32DevId].pthreadWork, szThreadName);

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_DestroyDev(MI_U32 u32DevId)
{
    MI_U32 u32CevaChn = 0;
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_DEVID_VALID(u32DevId);

    _stCevaModule.astDev[u32DevId].bThreadExit = TRUE;
    pthread_join(_stCevaModule.astDev[u32DevId].pthreadWork, NULL);
    _stCevaModule.astDev[u32DevId].bThreadRun = FALSE;
    _stCevaModule.astDev[u32DevId].pfCallback = NULL;
    _stCevaModule.astDev[u32DevId].u32CvCmd = 0;

    s32Ret = MI_CEVA_DestroyDev(u32DevId);
    if (MI_CV_OK != s32Ret)
    {
        printf("Destroy dev %d failed", u32DevId);
        return MI_CEVA_ERR_FAIL;
    }

    pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].mtxDev);
    pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].mtxThreadRun);
    pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].mtxThreadExit);
    pthread_cond_destroy(&_stCevaModule.astDev[u32DevId].condThreadExit);

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_CreateChn(MI_U32 u32DevId, MI_U32 u32ChnId)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;
    MI_U8 szThreadName[32];

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_DEVID_VALID(u32DevId);
    MI_CEVA_CHECK_CHNID_VALID(u32ChnId);
    s32Ret = MI_CV_CreateChn(u32DevId, u32ChnId);

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bInited = TRUE;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bThreadExit = FALSE;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bThreadRun = FALSE;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].pUsrData = &_stCevaModule.astDev[u32DevId];

        pthread_mutex_init(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxChn, NULL);
        pthread_mutex_init(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxThreadRun, NULL);
        pthread_mutex_init(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxThreadExit, NULL);
        pthread_cond_init(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].condThreadExit, NULL);

        memset(szThreadName, 0, sizeof(szThreadName));
        sprintf(szThreadName, "CEVA_AppThread_dev%02d", u32DevId);
        pthread_create(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].pthreadWork, NULL
                       , _MI_CEVA_ChnThread, &_stCevaModule.astDev[u32DevId].stChannel[u32ChnId]);
        pthread_setname_np(_stCevaModule.astDev[u32DevId].pthreadWork, szThreadName);
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CEVA_DestroyChn(MI_U32 u32DevId, MI_U32 u32ChnId)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_DEVID_VALID(u32DevId);
    MI_CEVA_CHECK_CHNID_VALID(u32ChnId);
    s32Ret = MI_CV_DestroyChn(u32DevId, u32ChnId);;

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bInited = FALSE;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bThreadExit = TRUE;
        _stCevaModule.astDev[u32DevId].stChannel[u32ChnId].bThreadRun = FALSE;

        pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxChn);
        pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxThreadRun);
        pthread_mutex_destroy(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].mtxThreadExit);
        pthread_cond_destroy(&_stCevaModule.astDev[u32DevId].stChannel[u32ChnId].condThreadExit);
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CEVA_SetChnPortAttr(MI_CEVA_ChnPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_MULL_POINT(pstAttr);
    MI_CEVA_CHECK_PORTID_VALID(pstAttr->ePortType, pstAttr->u32PortId);

    if (E_MI_CEVA_PORT_INPUT == pstAttr->ePortType)
    {
        s32Ret = _MI_CEVA_SetInputPortAttr(pstAttr->u32DevId, pstAttr->u32ChnId, pstAttr->u32PortId, &pstAttr->stInputPortAttr);
    }
    else
    {
        s32Ret = _MI_CEVA_SetOutputPortAttr(pstAttr->u32DevId, pstAttr->u32ChnId, pstAttr->u32PortId, &pstAttr->stOutputPortAttr);
    }

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}


MI_S32 MI_CEVA_GetChnPortAttr(MI_U32 u32DevId, MI_U32 u32ChnId, MI_U32 u32PortId, MI_CEVA_ChnPortType_e ePortType, MI_CEVA_ChnPortAttr_t* pstAttr)
{
    MI_S32 s32Ret = MI_ERR_CV_FAIL;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_MULL_POINT(pstAttr);

    if (E_MI_CEVA_PORT_INPUT == ePortType)
    {
        s32Ret = _MI_CEVA_GetInputPortAttr(u32DevId, u32ChnId, u32PortId, &pstAttr->stInputPortAttr);
    }
    else
    {
        s32Ret = _MI_CEVA_GetOutputPortAttr(u32DevId, u32ChnId, u32PortId, &pstAttr->stOutputPortAttr);
    }

    if (MI_CV_OK == s32Ret)
    {
        s32Ret = MI_CEVA_OK;
    }
    else
    {
        s32Ret = MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return s32Ret;
}

MI_S32 MI_CEVA_RegisterCallback(MI_U32 u32DevId, MI_U32 u32CvCmd, MI_CEVA_Callback pfCallback)
{
    MI_S32 i;

    MI_CEVA_FUNC_ENTRY();
    MI_CEVA_CHECK_CMD_VALID(u32CvCmd);

    pthread_mutex_lock(&_stCevaModule.astDev[u32DevId].mtxDev);
    _stCevaModule.astDev[u32DevId].pfCallback = pfCallback;
    _stCevaModule.astDev[u32DevId].u32CvCmd = u32CvCmd;

    switch (u32CvCmd)
    {
        case 0:
        case 1:
            _stCevaModule.astDev[u32DevId].bThreadRun = FALSE;

            for (i = 0; i < MI_CEVA_CHANNEL_MAX_NUM; i++)
            {
                if (_stCevaModule.astDev[u32DevId].stChannel[i].bInited)
                {
                    _stCevaModule.astDev[u32DevId].stChannel[i].bThreadRun = TRUE;
                }
            }
            break;
        case 2:
            _stCevaModule.astDev[u32DevId].bThreadRun = TRUE;

            for (i = 0; i < MI_CEVA_CHANNEL_MAX_NUM; i++)
            {
                _stCevaModule.astDev[u32DevId].stChannel[i].bThreadRun = FALSE;
            }
            break;
        default:
            DBG_ERR("Cmd not surpport\n");

    }
    pthread_mutex_unlock(&_stCevaModule.astDev[u32DevId].mtxDev);

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_Run(MI_U32 u32DevId, MI_U32 u32ChnId)
{
    MI_CEVA_FUNC_ENTRY();

    pthread_mutex_lock(&_stCevaModule.astDev[u32DevId].mtxThreadRun);
    _stCevaModule.astDev[u32DevId].bThreadRun = TRUE;
    pthread_mutex_unlock(&_stCevaModule.astDev[u32DevId].mtxThreadRun);

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_Stop(MI_U32 u32DevId,MI_U32 u32ChnId)
{
    MI_CEVA_FUNC_ENTRY();

    pthread_mutex_lock(&_stCevaModule.astDev[u32DevId].mtxThreadRun);
    _stCevaModule.astDev[u32DevId].bThreadRun = FALSE;
    pthread_mutex_unlock(&_stCevaModule.astDev[u32DevId].mtxThreadRun);

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_GetCdnnCfgBuf(void *pAddr)
{
    MI_CEVA_FUNC_ENTRY();

    if (MI_CV_OK != MI_CV_GetCdnnCfgBuf(pAddr, MI_CEVA_CDNN_CFG_BUFSIZE))
    {
        pAddr = NULL;

        MI_CEVA_FUNC_EXIT();
        return MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_GetCdnnComputeBuf(MI_PHY phyAddr, void *pAddr)
{
    MI_S32 s32Ret;

    MI_CEVA_FUNC_ENTRY();
    s32Ret = MI_SYS_MMA_Alloc("mma_heap_name0", MI_CEVA_CDNN_COMPUTE_BUFSIZE, &phyAddr);
    if (MI_SUCCESS != s32Ret)
    {
        DBG_INFO("Buf Alloc Failed\n");
        pAddr = NULL;

        MI_CEVA_FUNC_EXIT();
        return MI_CEVA_ERR_FAIL;
    }

    s32Ret = MI_SYS_Mmap(phyAddr, MI_CEVA_CDNN_COMPUTE_BUFSIZE, &pAddr, FALSE);
    if (MI_SUCCESS != s32Ret || !pAddr)
    {
        DBG_INFO("Buf mmap failed\n");
        pAddr = NULL;

        MI_CEVA_FUNC_EXIT();
        return MI_CEVA_ERR_FAIL;
    }

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

MI_S32 MI_CEVA_ReleaseCdnnComputeBuf(MI_PHY phyAddr, void *pAddr)
{
    MI_S32 s32Ret;

    MI_CEVA_FUNC_ENTRY();

    if (pAddr)
    {
        s32Ret = MI_SYS_Munmap(pAddr, MI_CEVA_CDNN_COMPUTE_BUFSIZE);
        if (MI_SUCCESS != s32Ret)
        {
            DBG_INFO("Buff unmap failed\n");
            return MI_CEVA_ERR_FAIL;
        }
    }
    pAddr = NULL;

    if (phyAddr)
    {
        s32Ret = MI_SYS_MMA_Free(phyAddr);
        if (MI_SUCCESS != s32Ret)
        {
            DBG_INFO("Buf free failed\n");
            return MI_CEVA_ERR_FAIL;
        }
    }
    phyAddr = 0;

    MI_CEVA_FUNC_EXIT();
    return MI_CEVA_OK;
}

