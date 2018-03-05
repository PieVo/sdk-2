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
    int fd = -1 , count = 0;

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVpeChn0InputPort0;
    stVpeChn0InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0InputPort0.u32DevId = 0;
    stVpeChn0InputPort0.u32ChnId = 0;
    stVpeChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn2InputPort0;
    stVpeChn2InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2InputPort0.u32DevId = 0;
    stVpeChn2InputPort0.u32ChnId = 2;
    stVpeChn2InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn3InputPort0;
    stVpeChn3InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3InputPort0.u32DevId = 0;
    stVpeChn3InputPort0.u32ChnId = 3;
    stVpeChn3InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort1;
    stVpeChn1OutputPort1.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort1.u32DevId = 0;
    stVpeChn1OutputPort1.u32ChnId = 1;
    stVpeChn1OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn2OutputPort2;
    stVpeChn2OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2OutputPort2.u32DevId = 0;
    stVpeChn2OutputPort2.u32ChnId = 2;
    stVpeChn2OutputPort2.u32PortId = 2;

    MI_SYS_ChnPort_t stVpeChn3OutputPort3;
    stVpeChn3OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3OutputPort3.u32DevId = 0;
    stVpeChn3OutputPort3.u32ChnId = 3;
    stVpeChn3OutputPort3.u32PortId = 3;

    fd =open("/mnt/test/vpe_960_640_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn3OutputPort3,&stBufInfo,&hHandle))
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(count ++ >= FrameMaxCount)
                    break;
            }
            else
            {
                usleep(1000 * 10);
            }
        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort3,0,10);
    
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn0OutputPort0,&stVpeChn1InputPort0) , MI_SUCCESS);
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn1OutputPort1,&stVpeChn2InputPort0) , MI_SUCCESS);
    ExecFunc(MI_SYS_UnBindChnPort(&stVpeChn2OutputPort2,&stVpeChn3InputPort0) , MI_SUCCESS);

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
    int count = 0;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);

    MI_SYS_ChnPort_t stVpeChn0InputPort0;
    stVpeChn0InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0InputPort0.u32DevId = 0;
    stVpeChn0InputPort0.u32ChnId = 0;
    stVpeChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn2InputPort0;
    stVpeChn2InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2InputPort0.u32DevId = 0;
    stVpeChn2InputPort0.u32ChnId = 2;
    stVpeChn2InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn3InputPort0;
    stVpeChn3InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3InputPort0.u32DevId = 0;
    stVpeChn3InputPort0.u32ChnId = 3;
    stVpeChn3InputPort0.u32PortId = 0;


    MI_SYS_ChnPort_t stVpeChn0OutputPort0;
    stVpeChn0OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn0OutputPort0.u32DevId = 0;
    stVpeChn0OutputPort0.u32ChnId = 0;
    stVpeChn0OutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort1;
    stVpeChn1OutputPort1.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort1.u32DevId = 0;
    stVpeChn1OutputPort1.u32ChnId = 1;
    stVpeChn1OutputPort1.u32PortId = 1;

    MI_SYS_ChnPort_t stVpeChn2OutputPort2;
    stVpeChn2OutputPort2.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn2OutputPort2.u32DevId = 0;
    stVpeChn2OutputPort2.u32ChnId = 2;
    stVpeChn2OutputPort2.u32PortId = 2;

    MI_SYS_ChnPort_t stVpeChn3OutputPort3;
    stVpeChn3OutputPort3.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn3OutputPort3.u32DevId = 0;
    stVpeChn3OutputPort3.u32ChnId = 3;
    stVpeChn3OutputPort3.u32PortId = 3;

    MI_SYS_SetChnOutputPortDepth(&stVpeChn3OutputPort3,10,20);

    ExecFunc(MI_SYS_BindChnPort(&stVpeChn0OutputPort0,&stVpeChn1InputPort0,30,30) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn1OutputPort1,&stVpeChn2InputPort0,30,30) , MI_SUCCESS);
    ExecFunc(MI_SYS_BindChnPort(&stVpeChn2OutputPort2,&stVpeChn3InputPort0,30,30) , MI_SUCCESS);

    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

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

            MI_SYS_ChnInputPortGetBuf(&stVpeChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1);
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
            usleep(32*1000);
            MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],size);
            MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo ,FALSE);
            if(count ++ >= FrameMaxCount)
                break;
        }
    }
    if(fd > 0)
        close(fd);
  
    
    pthread_join(tid,NULL);
    return 0;
}
