#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

int FrameMaxCount = 5;

void *Chn1OutputPort0GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stDivpChn1InputPort0;
    stDivpChn1InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn1InputPort0.u32DevId = 0;
    stDivpChn1InputPort0.u32ChnId = 1;
    stDivpChn1InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn1OutputPort0;
    stDivpChn1OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn1OutputPort0.u32DevId = 0;
    stDivpChn1OutputPort0.u32ChnId = 1;
    stDivpChn1OutputPort0.u32PortId = 0;


    fd =open("/mnt/test/chn1_divp_720_576_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn1OutputPort0,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(++ count >= FrameMaxCount)
                    break;
            }
            else
                usleep(1000 * 10);
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stDivpChn1OutputPort0,0,5);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stDivpChn0OutputPort0,&stDivpChn1InputPort0) , MI_SUCCESS);
    return NULL;
}

void *Chn2OutputPort0GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stDivpChn2InputPort0;
    stDivpChn2InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn2InputPort0.u32DevId = 0;
    stDivpChn2InputPort0.u32ChnId = 2;
    stDivpChn2InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn2OutputPort0;
    stDivpChn2OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn2OutputPort0.u32DevId = 0;
    stDivpChn2OutputPort0.u32ChnId = 2;
    stDivpChn2OutputPort0.u32PortId = 0;


    fd =open("/mnt/test/chn2_divp_720_576_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn2OutputPort0,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(++count >= FrameMaxCount)
                    break;
            }
            else
                usleep(1000 * 10);
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stDivpChn2OutputPort0,0,5);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stDivpChn0OutputPort0,&stDivpChn2InputPort0) , MI_SUCCESS);
    return NULL;
}

void *Chn3OutputPort0GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stDivpChn3InputPort0;
    stDivpChn3InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn3InputPort0.u32DevId = 0;
    stDivpChn3InputPort0.u32ChnId = 3;
    stDivpChn3InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn3OutputPort0;
    stDivpChn3OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn3OutputPort0.u32DevId = 0;
    stDivpChn3OutputPort0.u32ChnId = 3;
    stDivpChn3OutputPort0.u32PortId = 0;


    fd =open("/mnt/test/chn3_divp_720_576_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn3OutputPort0,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(++count >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(1000 * 10);
            } 
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stDivpChn3OutputPort0,0,5);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stDivpChn0OutputPort0,&stDivpChn3OutputPort0) , MI_SUCCESS);
    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U8 u8Pattern = 0;
    MI_U32 alloc_size;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));
    int framecount = 0;
    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    MI_SYS_ChnPort_t stDivpChn0InputPort0;
    stDivpChn0InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0InputPort0.u32DevId = 0;
    stDivpChn0InputPort0.u32ChnId = 0;
    stDivpChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn1InputPort0;
    stDivpChn1InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn1InputPort0.u32DevId = 0;
    stDivpChn1InputPort0.u32ChnId = 1;
    stDivpChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn2InputPort0;
    stDivpChn2InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn2InputPort0.u32DevId = 0;
    stDivpChn2InputPort0.u32ChnId = 2;
    stDivpChn2InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn3InputPort0;
    stDivpChn3InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn3InputPort0.u32DevId = 0;
    stDivpChn3InputPort0.u32ChnId = 3;
    stDivpChn3InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;


    MI_SYS_SetChnOutputPortDepth(&stDivpChn1InputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stDivpChn2InputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stDivpChn3InputPort0,3,5);
    
    MI_U32 u32SrcFrmrate = 30 , u32Chn1DstFrmrate = 15 , u32Chn2DstFrmrate = 25 , u32Chn3DstFrmrate = 2;;
    ExecFunc(MI_SYS_BindChnPort(&stDivpChn0OutputPort0,&stDivpChn1InputPort0,30,15) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stDivpChn0OutputPort0,&stDivpChn2InputPort0,30,25) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stDivpChn0OutputPort0,&stDivpChn3InputPort0,30,2) , MI_SUCCESS);

    pthread_t tid0 ,tid1 , tid2;
    
    pthread_create(&tid0, NULL, Chn1OutputPort0GetBuf, NULL);
    pthread_create(&tid1, NULL, Chn2OutputPort0GetBuf, NULL);
    pthread_create(&tid2, NULL, Chn3OutputPort0GetBuf, NULL);

    int fd = open("/mnt/test/1920_1080_yuv422.yuv", O_RDWR, 0666);
    if(fd > 0)
    {
        while(1)
        {
            
            int n = 0;
            stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
            stBufConf.stFrameCfg.u16Width = 1920;
            stBufConf.stFrameCfg.u16Height = 1080;

            stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
            MI_U32 u32SleepTime = 1000 / u32SrcFrmrate;

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stDivpChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;

                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                n = read(fd , stBufInfo.stFrameData.pVirAddr[0] , size);
                if(n < size)
                {
                    lseek(fd, 0, SEEK_SET);
                    n = read(fd , stBufInfo.stFrameData.pVirAddr[0] , size);
                }
                MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if(++framecount >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(u32SleepTime*1000);
                continue;
            }
            
            usleep(u32SleepTime*1000);

        }
    }
    if(fd > 0)
        close(fd);
    
    pthread_join(tid0,NULL);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);

    return 0;
}
