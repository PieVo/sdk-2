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
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

int FrameMaxCount = 5;
void *Chn1OutputPort2GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort2;
    stVpeChn1OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort2.u32DevId = 0;
    stVpeChn1OutputPort2.u32ChnId = 1;
    stVpeChn1OutputPort2.u32PortId = 2;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort2,5,10);

    fd =open("/mnt/test/chn1_port2_Vpe_352_288_yuv2.yuv",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn1OutputPort2,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;
            }
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort2,0,10);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn0OutputPort0,&stVpeChn1InputPort0) , MI_SUCCESS);
    return NULL;
}

void *Chn2OutputPort2GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVpeChn2InputPort0;
    stVpeChn2InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2InputPort0.u32DevId = 0;
    stVpeChn2InputPort0.u32ChnId = 2;
    stVpeChn2InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn0OutputPort1;
    stVpeChn0OutputPort1.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort1.u32DevId = 0;
    stVpeChn0OutputPort1.u32ChnId = 0;
    stVpeChn0OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn2OutputPort2;
    stVpeChn2OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2OutputPort2.u32DevId = 0;
    stVpeChn2OutputPort2.u32ChnId = 2;
    stVpeChn2OutputPort2.u32PortId = 2;
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn2OutputPort2,5,10);


    fd =open("/mnt/test/chn2_port2_Vpe_352_288_yuv2.yuv",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn2OutputPort2,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;
            }
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn2OutputPort2,0,10);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn0OutputPort1,&stVpeChn2InputPort0) , MI_SUCCESS);
    return NULL;
}

void *Chn3OutputPort2GetBuf(void *arg)
{
    int fd = -1;
    int count  = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVpeChn3InputPort0;
    stVpeChn3InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3InputPort0.u32DevId = 0;
    stVpeChn3InputPort0.u32ChnId = 3;
    stVpeChn3InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn0OutputPort2;
    stVpeChn0OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort2.u32DevId = 0;
    stVpeChn0OutputPort2.u32ChnId = 0;
    stVpeChn0OutputPort2.u32PortId = 2;

    MI_SYS_ChnPort_t stVpeChn3OutputPort2;
    stVpeChn3OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3OutputPort2.u32DevId = 0;
    stVpeChn3OutputPort2.u32ChnId = 3;
    stVpeChn3OutputPort2.u32PortId = 2;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort2,5,10);

    fd =open("/mnt/test/chn3_port2_Vpe_352_288_yuv2.yuv",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn3OutputPort2,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;

            }
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort2,0,10);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn0OutputPort2,&stVpeChn3InputPort0) , MI_SUCCESS);
    return NULL;
}

void *Chn4OutputPort2GetBuf(void *arg)
{
    int fd = -1;
    int count = 0;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVpeChn4InputPort0;
    stVpeChn4InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4InputPort0.u32DevId = 0;
    stVpeChn4InputPort0.u32ChnId = 4;
    stVpeChn4InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn0OutputPort3;
    stVpeChn0OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort3.u32DevId = 0;
    stVpeChn0OutputPort3.u32ChnId = 0;
    stVpeChn0OutputPort3.u32PortId = 3;

    MI_SYS_ChnPort_t stVpeChn4OutputPort2;
    stVpeChn4OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4OutputPort2.u32DevId = 0;
    stVpeChn4OutputPort2.u32ChnId = 4;
    stVpeChn4OutputPort2.u32PortId = 2;


    fd =open("/mnt/test/chn4_Vpe_352_288_yuv2.yuv",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn4OutputPort2,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;

            }
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn4OutputPort2,0,10);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn0OutputPort3,&stVpeChn4OutputPort2) , MI_SUCCESS);
    return NULL;
}

int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U8 u8Pattern = 0;
    int count = 0;
    MI_U32 alloc_size;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

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


    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort2;
    stVpeChn1OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort2.u32DevId = 0;
    stVpeChn1OutputPort2.u32ChnId = 1;
    stVpeChn1OutputPort2.u32PortId = 2; 

    MI_SYS_ChnPort_t stVpeChn2InputPort0;
    stVpeChn2InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2InputPort0.u32DevId = 0;
    stVpeChn2InputPort0.u32ChnId = 2;
    stVpeChn2InputPort0.u32PortId = 0;
    
    MI_SYS_ChnPort_t stVpeChn2OutputPort2;
    stVpeChn2OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2OutputPort2.u32DevId = 0;
    stVpeChn2OutputPort2.u32ChnId = 2;
    stVpeChn2OutputPort2.u32PortId = 2; 

    MI_SYS_ChnPort_t stVpeChn3InputPort0;
    stVpeChn3InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3InputPort0.u32DevId = 0;
    stVpeChn3InputPort0.u32ChnId = 3;
    stVpeChn3InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn3OutputPort2;
    stVpeChn3OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3OutputPort2.u32DevId = 0;
    stVpeChn3OutputPort2.u32ChnId = 3;
    stVpeChn3OutputPort2.u32PortId = 2; 

    MI_SYS_ChnPort_t stVpeChn4InputPort0;
    stVpeChn4InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4InputPort0.u32DevId = 0;
    stVpeChn4InputPort0.u32ChnId = 4;
    stVpeChn4InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn4OutputPort2;
    stVpeChn4OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn4OutputPort2.u32DevId = 0;
    stVpeChn4OutputPort2.u32ChnId = 4;
    stVpeChn4OutputPort2.u32PortId = 2;
    
    MI_U32 u32SrcFrmrate = 30 , u32Chn1DstFrmrate = 15 , u32Chn2DstFrmrate = 25 , u32Chn3DstFrmrate = 10 , u32Chn4DstFrmrate = 2;
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn0OutputPort0,&stVpeChn1InputPort0,30,15) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn0OutputPort1,&stVpeChn2InputPort0,30,25) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn0OutputPort2,&stVpeChn3InputPort0,30,10) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn0OutputPort3,&stVpeChn4InputPort0,30,2) , MI_SUCCESS);

    pthread_t tid0 ,tid1 , tid2 , tid3;
    
    pthread_create(&tid0, NULL, Chn1OutputPort2GetBuf, NULL);
    pthread_create(&tid1, NULL, Chn2OutputPort2GetBuf, NULL);
    pthread_create(&tid2, NULL, Chn3OutputPort2GetBuf, NULL);
    pthread_create(&tid3, NULL, Chn4OutputPort2GetBuf, NULL);

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

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVpeChn0InputPort0,&stBufConf,&stBufInfo,&hHandle ,-1))
            {
                stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;

                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                n = read(fd , stBufInfo.stFrameData.pVirAddr[0] , size);
                if(n < size)
                {  
                    lseek(fp, 0, SEEK_SET);
                    n = read(fd , stBufInfo.stFrameData.pVirAddr[0] , size);
                }
                usleep(u32SleepTime*1000);
                MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                if(count ++ >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(u32SleepTime*1000);
                continue;
            }
        }
    }
    if(fd > 0)
        close(fd);
    
    pthread_join(tid0,NULL);
    pthread_join(tid1,NULL);
    pthread_join(tid2,NULL);
    pthread_join(tid3,NULL);

    return 0;
}
