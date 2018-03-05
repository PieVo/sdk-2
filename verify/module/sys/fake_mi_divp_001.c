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

void *ChnOutputPortGetBuf(void *arg)
{

    int FrameCount = 0;
    int fd = -1;
    
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;

    fd =open("/mnt/test/divp_01_yuv2.yuv",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChn0OutputPort0,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
     
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(++ FrameCount >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(10 * 1000);
            }

        }
    }
    if(fd > 0) close(fd);
    

    MI_SYS_SetChnOutputPortDepth(&stDivpChn0OutputPort0,0,5);

    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    MI_SYS_ChnPort_t stDivpChn0InputPort0;
    stDivpChn0InputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0InputPort0.u32DevId = 0;
    stDivpChn0InputPort0.u32ChnId = 0;
    stDivpChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChn0OutputPort0;
    stDivpChn0OutputPort0.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChn0OutputPort0.u32DevId = 0;
    stDivpChn0OutputPort0.u32ChnId = 0;
    stDivpChn0OutputPort0.u32PortId = 0;


    MI_SYS_SetChnOutputPortDepth(&stDivpChn0OutputPort0,3,5);
    
    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

    int framecount = 0;

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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stDivpChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;
                stBufInfo.bEndOfStream = FALSE;

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
                printf("get buf fail\n");
            usleep(100 * 1000);
        }
    }
    if(fd > 0)
        close(fd);
    printf("divp test end\n");
    
    pthread_join(tid,NULL);
    return 0;
}
