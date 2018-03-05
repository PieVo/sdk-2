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

    MI_SYS_ChnPort_t stVifChn0OutputPort0;
    stVifChn0OutputPort0.eModId = E_MI_MODULE_ID_VIF;
    stVifChn0OutputPort0.u32DevId = 0;
    stVifChn0OutputPort0.u32ChnId = 0;
    stVifChn0OutputPort0.u32PortId = 1;
    fd =open("/mnt/test/960_540_VIF_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd < 0)
    {
        printf("open file fail\n");
        return 0;
    }
    MI_SYS_SetChnOutputPortDepth(&stVifChn0OutputPort0,5,20);
    
    while(1)
    {
        if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVifChn0OutputPort0,&stBufInfo,&hHandle))
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
    MI_SYS_SetChnOutputPortDepth(&stVifChn0OutputPort0,0,20);
    printf("vif test end\n");
    close(fd);
    fd = -1;
    return 0;
}
