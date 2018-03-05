#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"

int FrameMaxCount = 1;

void *ChnOutputPortGetBuf(void *arg)
{
    int fd0 = -1 , fd1 = -1 , fd2 = -1 , fd3 = -1;
    int count = 0;

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_U32 u32WorkPort = 4 , u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort;
    stVpeChn0OutputPort.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort.u32DevId = 0;
    stVpeChn0OutputPort.u32ChnId = 0;
    stVpeChn0OutputPort.u32PortId = 0;


    fd0 =open("/mnt/test/vpe_ch0_port0_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd1 =open("/mnt/test/vpe_ch0_port1_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd2 =open("/mnt/test/vpe_ch0_port2_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd3 =open("/mnt/test/vpe_ch0_port3_yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

    if(fd0 > 0 && fd1 > 0 && fd2 > 0 && fd3 > 0)
    {
        while(1)
        {
            for(u32PortId = 0 ; u32PortId < u32WorkPort ; u32PortId ++)
            {
                stVpeChn0OutputPort.u32PortId = u32PortId;
                if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn0OutputPort,&stBufInfo,&hHandle))
                {
                    int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                    if(u32PortId == 0)
                    {
                        write(fd0,stBufInfo.stFrameData.pVirAddr[0],size);
                        count ++;
                    }
                    else if(u32PortId == 1)
                        write(fd1,stBufInfo.stFrameData.pVirAddr[0],size);
                    else if(u32PortId == 2)
                        write(fd2,stBufInfo.stFrameData.pVirAddr[0],size);
                    else if(u32PortId == 3)
                        write(fd3,stBufInfo.stFrameData.pVirAddr[0],size);
                    else
                        printf("error \n");
                    
                    MI_SYS_ChnOutputPortPutBuf(hHandle);

                }
            }
            if(count >= FrameMaxCount)
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

    for(u32PortId = 0 ; u32PortId < u32WorkPort ; u32PortId ++)
    {
        stVpeChn0OutputPort.u32PortId = u32PortId;
        MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort,0,5);
    }


    return NULL;
}


int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    int count =0;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_Init();

    MI_SYS_ChnPort_t stVpeChn0InputPort0;
    stVpeChn0InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0InputPort0.u32DevId = 0;
    stVpeChn0InputPort0.u32ChnId = 0;
    stVpeChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort1;
    stVpeChn0OutputPort1.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort1.u32DevId = 0;
    stVpeChn0OutputPort1.u32ChnId = 0;
    stVpeChn0OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn0OutputPort2;
    stVpeChn0OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort2.u32DevId = 0;
    stVpeChn0OutputPort2.u32ChnId = 0;
    stVpeChn0OutputPort2.u32PortId = 2;

    MI_SYS_ChnPort_t stVpeChn0OutputPort3;
    stVpeChn0OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort3.u32DevId = 0;
    stVpeChn0OutputPort3.u32ChnId = 0;
    stVpeChn0OutputPort3.u32PortId = 3;

    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort0,2,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort1,2,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort2,2,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort3,2,5);

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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVpeChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
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
                if(count ++ >= FrameMaxCount)
                    break;
            }
            usleep(100 * 1000);
        }
    }
    if(fd > 0)
        close(fd);
    
    pthread_join(tid,NULL);
    return 0;
}
