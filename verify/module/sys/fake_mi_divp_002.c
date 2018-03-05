#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"


int FrameMaxCount = 5;

void *ChnOutputPort(void *arg)
{

    int FrameCount = 0;

    int fd0 = -1 , fd1 = -1 , fd2 = -1 , fd3 = -1;
    MI_U32 u32ChnId = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 u32WorkChnNum = 4;
    
    MI_SYS_ChnPort_t stDivpChnOutputPort;
    stDivpChnOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnOutputPort.u32DevId = 0;
    stDivpChnOutputPort.u32ChnId = 0;
    stDivpChnOutputPort.u32PortId = 0;

    fd0 =open("/mnt/test/divp_ch0_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd1 =open("/mnt/test/divp_ch1_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd2 =open("/mnt/test/divp_ch2_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd3 =open("/mnt/test/divp_ch3_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd0 > 0 && fd1 > 0 && fd2 > 0 && fd3 > 0)
    {
        while(1)
        {
            for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
            {
                stDivpChnOutputPort.u32ChnId = u32ChnId;
                if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stDivpChnOutputPort,&stBufInfo,&hHandle))
                {
                    int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                    if(u32ChnId == 0)
                    {
                        write(fd0,stBufInfo.stFrameData.pVirAddr[0],size);
                        FrameCount ++;
                    }
                    else if(u32ChnId == 1)
                        write(fd1,stBufInfo.stFrameData.pVirAddr[0],size);
                    else if(u32ChnId == 2)
                        write(fd2,stBufInfo.stFrameData.pVirAddr[0],size);
                    else if(u32ChnId == 3)
                        write(fd3,stBufInfo.stFrameData.pVirAddr[0],size);
                    else
                        printf("error \n");
                    
                    MI_SYS_ChnOutputPortPutBuf(hHandle);

                }
                else
                {
                    //printf("yipint test get out port buf fail\n");
                }

            }
            if(FrameCount >= FrameMaxCount)
                break;
        }
    }
    else
    {
        printf("open file fail\n");
    }
    if(fd0 > 0) close(fd0);
    if(fd1 > 0) close(fd1);
    if(fd2 > 0) close(fd2);
    if(fd3 > 0) close(fd3);

    for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
    {
        stDivpChnOutputPort.u32ChnId = u32ChnId;
        MI_SYS_SetChnOutputPortDepth(&stDivpChnOutputPort,0,5);
    }


    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32ChnId = 0;
    MI_U32 u32WorkChnNum = 4;
    int n = 0;
    int framecount = 0;
            
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_Init();

    MI_SYS_ChnPort_t stDivpChnInputPort;
    stDivpChnInputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnInputPort.u32DevId = 0;
    stDivpChnInputPort.u32ChnId = 0;
    stDivpChnInputPort.u32PortId = 0;

    MI_SYS_ChnPort_t stDivpChnOutputPort;
    stDivpChnOutputPort.eModId = E_MI_MODULE_ID_DIVP;
    stDivpChnOutputPort.u32DevId = 0;
    stDivpChnOutputPort.u32ChnId = 0;
    stDivpChnOutputPort.u32PortId = 0;

    stDivpChnOutputPort.u32ChnId = 0;
    MI_SYS_SetChnOutputPortDepth(&stDivpChnOutputPort,3,5);
    stDivpChnOutputPort.u32ChnId = 1;
    MI_SYS_SetChnOutputPortDepth(&stDivpChnOutputPort,3,5);
    stDivpChnOutputPort.u32ChnId = 2;
    MI_SYS_SetChnOutputPortDepth(&stDivpChnOutputPort,3,5);
    stDivpChnOutputPort.u32ChnId = 3;
    MI_SYS_SetChnOutputPortDepth(&stDivpChnOutputPort,3,5);
    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPort, NULL);

    int fd = -1;
    int fd0 = open("/mnt/test/1920_1080_yuv422.yuv", O_RDWR, 0666);
    int fd1 = open("/mnt/test/720_576_yuv422.yuv",O_RDWR, 0666);
    int fd2 = open("/mnt/test/1280_720_yuv422.yuv",O_RDWR, 0666);
    int fd3 = open("/mnt/test/352_288_yuv422.yuv",O_RDWR, 0666);

    if(fd0 > 0 && fd1 > 0 && fd2 > 0 && fd3 > 0)
    {
        while(1)
        {
            for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
            {
                stDivpChnInputPort.u32ChnId = u32ChnId;
                if(u32ChnId == 0)
                {
                    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
                    stBufConf.stFrameCfg.u16Width = 1920;
                    stBufConf.stFrameCfg.u16Height = 1080;
                    
                    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    fd = fd0;
                }
                if(u32ChnId == 1)
                {
                    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
                    stBufConf.stFrameCfg.u16Width = 720;
                    stBufConf.stFrameCfg.u16Height = 576;

                    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    fd = fd1;
                }
                if(u32ChnId == 2)
                {
                    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
                    stBufConf.stFrameCfg.u16Width = 1280;
                    stBufConf.stFrameCfg.u16Height = 720;
                    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    fd = fd2;
                }
                if(u32ChnId == 3)
                {
                    stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
                    MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
                    stBufConf.stFrameCfg.u16Width = 352;
                    stBufConf.stFrameCfg.u16Height = 288;
                    stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
                    stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
                    fd = fd3;
                }

                if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stDivpChnInputPort,&stBufConf,&stBufInfo,&hHandle , -1))
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
                    if(stDivpChnInputPort.u32ChnId == 0)
                        framecount ++;
                }
                else
                    printf("get buf fail\n");
            }
            if(framecount >= FrameMaxCount)
                break;
        }
    }
    if(fd0 > 0)        close(fd0);
    if(fd1 > 0)        close(fd1);
    if(fd2 > 0)        close(fd2);
    if(fd3 > 0)        close(fd3);

    printf("divp test end\n");
    
    pthread_join(tid,NULL);
    return 0;
}
