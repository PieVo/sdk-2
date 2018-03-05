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
    int framecount = 200;
    int count = 0;
    MI_U32 u32WorkChnNum = 4;
    MI_U32 u32ChnId = 0 ;
    int fd[4] = {-1};
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
    
    fd[0] =open("/mnt/test/VIF_CH0_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd[1] =open("/mnt/test/VIF_CH1_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd[2] =open("/mnt/test/VIF_CH2_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    fd[3] =open("/mnt/test/VIF_CH3_yuv422.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    
    if(fd[0] < 0 || fd[1] < 0 || fd[2] < 0 || fd[3] < 0)
    {
        printf("open file fail\n");
        return 0;
    }

    for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
    {
        stVifChnOutputPort0.u32ChnId = u32ChnId;
        stVifChnOutputPort1.u32ChnId = u32ChnId;
        MI_SYS_SetChnOutputPortDepth(&stVifChnOutputPort0,5,20);
        MI_SYS_SetChnOutputPortDepth(&stVifChnOutputPort1,5,20);
    }
    while(1)
    {
        for(u32ChnId = 0  ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVifChnOutputPort0,&stBufInfo,&hHandle))
            {
                if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                    printf("data read error\n");
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd[u32ChnId],stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
            }
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVifChnOutputPort1,&stBufInfo,&hHandle))
            {
                if(stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
                    printf("data read error\n");
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd[u32ChnId],stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
            }
        }
        if(count ++ > FrameMaxCount)
            break;
    }
    
    for(u32ChnId = 0 ; u32ChnId < u32WorkChnNum ; u32ChnId ++)
    {
        stVifChnOutputPort0.u32ChnId = u32ChnId;
        stVifChnOutputPort1.u32ChnId = u32ChnId;
        MI_SYS_SetChnOutputPortDepth(&stVifChnOutputPort0,0,20);
        MI_SYS_SetChnOutputPortDepth(&stVifChnOutputPort1,0,20);
    }

    printf("vif test end\n");
    if(fd[0] > 0)close(fd[0]);
    if(fd[1] > 0)close(fd[1]);
    if(fd[2] > 0)close(fd[2]);
    if(fd[3] > 0)close(fd[3]);
    return 0;
}
