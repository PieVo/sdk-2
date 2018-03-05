#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"

int FrameMaxCount = 5;
int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));

    int count = 0;
    int fd = -1;
    MI_SYS_Init();

    MI_SYS_ChnPort_t stVifChnOutputPort0;
    stVifChnOutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChnOutputPort0.u32DevId = 0;
    stVifChnOutputPort0.u32ChnId = 0;
    stVifChnOutputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVifChnOutputPort1;
    stVifChnOutputPort1.eModId = E_MI_MODULE_ID_VIF;
    stVifChnOutputPort1.u32DevId = 0;
    stVifChnOutputPort1.u32ChnId = 0;
    stVifChnOutputPort1.u32PortId = 1;

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

    MI_SYS_ChnPort_t stVpeChn1InputPort0;
    stVpeChn1InputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1InputPort0.u32DevId = 0;
    stVpeChn1InputPort0.u32ChnId = 1;
    stVpeChn1InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVpeChn1OutputPort0;
    stVpeChn1OutputPort0.eModId = E_MI_MODULE_ID_VPE;
    stVpeChn1OutputPort0.u32DevId = 0;
    stVpeChn1OutputPort0.u32ChnId = 1;
    stVpeChn1OutputPort0.u32PortId = 0;


    fd =open("/mnt/test/Vpe_1920_1080_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort0,3,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort0,3,5);

    MI_SYS_BindChnPort(&stVifChnOutputPort0,&stVpeChn0InputPort0,30,30);
    MI_SYS_BindChnPort(&stVifChnOutputPort1,&stVpeChn1InputPort0,30,30);

    while(1)
    {

        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn0OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChn1OutputPort0,&stBufInfo,&hHandle))
        {
            if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                printf("data read error\n");
            MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
            write(fd,stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            if(count ++ > FrameMaxCount)
                break;
        }

    }

    MI_SYS_SetChnOutputPortDepth(&stVpeChn0OutputPort0,0,5);
    MI_SYS_SetChnOutputPortDepth(&stVpeChn1OutputPort0,0,5);

    MI_SYS_UnBindChnPort(&stVifChnOutputPort0,&stVpeChn0InputPort0);
    MI_SYS_UnBindChnPort(&stVifChnOutputPort1,&stVpeChn1InputPort0);

    printf("vif & vpe test end\n");
    if(fd > 0)close(fd);
    return 0;
}
