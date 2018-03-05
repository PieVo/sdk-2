#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <poll.h>
#include "mi_sys.h"


#define DISP_CHN_NUM (16)

int FrameMaxCount = 5;
int main(int argc, const char *argv[])
{
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufConf_t stBufConf;
    MI_U32 u32ChnId = 0;
    int count = 0;
    int fd[DISP_CHN_NUM] = {-1};
    memset(&stBufInfo , 0 , sizeof(MI_SYS_BufInfo_t));
    memset(&stBufConf , 0 , sizeof(MI_SYS_BufConf_t));

    MI_SYS_ChnPort_t stDispChnInputPort0;
    stDispChnInputPort0.eModId = E_MI_MODULE_ID_DISP;
    stDispChnInputPort0.u32DevId = 0;
    stDispChnInputPort0.u32ChnId = 0;
    stDispChnInputPort0.u32PortId = 0;

    MI_SYS_Init();

    for(u32ChnId = 0 ; u32ChnId < DISP_CHN_NUM ; u32ChnId ++)
    {
        fd[u32ChnId] = open("/mnt/test/720_576_yuv422.yuv", O_RDWR, 0666);
        if(!fd[u32ChnId])
        {
            printf("open file fail\n");
            return 0;
        }
    }

    while(1)
    {
        for(u32ChnId = 0 ; u32ChnId < DISP_CHN_NUM ; u32ChnId ++)
        {
            int n = 0;
            stDispChnInputPort0.u32ChnId = u32ChnId;
            stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
            stBufConf.stFrameCfg.u16Height = 720;
            stBufConf.stFrameCfg.u16Width = 576;

            stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
            stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stDispChnInputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                stBufInfo.stFrameData.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE;
                stBufInfo.stFrameData.eFieldType = E_MI_SYS_FIELDTYPE_NONE;
                stBufInfo.stFrameData.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE;

                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                n = read(fd[u32ChnId] , stBufInfo.stFrameData.pVirAddr[0] , size);
                if(n < size)
                {
                    lseek(fd[u32ChnId], 0, SEEK_SET);
                    n = read(fd[u32ChnId] , stBufInfo.stFrameData.pVirAddr[0] , size);
                }
                MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0],size);
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo ,FALSE);
                if(u32ChnId == DISP_CHN_NUM - 1)
                    count ++;
            }
            else
            {
                usleep(20 * 1000);
            }
        }
        if(count >= FrameMaxCount)
            break;
    }

    for(u32ChnId = 0 ; u32ChnId < DISP_CHN_NUM ; u32ChnId ++)
    {
        if(fd[u32ChnId])
            close(fd[u32ChnId]);
    }
    
    printf("disp test end\n");
    
    return 0;
}
