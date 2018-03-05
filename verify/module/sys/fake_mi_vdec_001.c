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


void *ChnOutputPortGetBuf(void *arg)
{
    int fd = -1;

    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;

    MI_SYS_ChnPort_t stVdecChn0OutputPort0;
    stVdecChn0OutputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn0OutputPort0.u32DevId = 0;
    stVdecChn0OutputPort0.u32ChnId = 0;
    stVdecChn0OutputPort0.u32PortId = 0;

    MI_SYS_SetChnOutputPortDepth(&stVdecChn0OutputPort0,3,5);

    fd =open("/mnt/test/yuv2.yuv",O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(fd > 0)
    {
        while(1)
        {
            if(MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVdecChn0OutputPort0,&stBufInfo,&hHandle))
            {
                MI_U32 u32Size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];
                write(fd,stBufInfo.stFrameData.pVirAddr[0],u32Size);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                if(stBufInfo.bEndOfStream)
                    break;
            }
            else
            {
                //printf("yipint test get out port buf fail\n");
            }

            //usleep(10*1000);

        }
    }
    if(fd > 0) close(fd);
    
    MI_SYS_SetChnOutputPortDepth(&stVdecChn0OutputPort0,0,10);

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

    MI_SYS_ChnPort_t stVdecChn0InputPort0;
    stVdecChn0InputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn0InputPort0.u32DevId = 0;
    stVdecChn0InputPort0.u32ChnId = 0;
    stVdecChn0InputPort0.u32PortId = 0;

    MI_SYS_ChnPort_t stVdecChn0OutputPort0;
    stVdecChn0OutputPort0.eModId = E_MI_MODULE_ID_VDEC;
    stVdecChn0OutputPort0.u32DevId = 0;
    stVdecChn0OutputPort0.u32ChnId = 0;
    stVdecChn0OutputPort0.u32PortId = 0;

    pthread_t tid;
    pthread_create(&tid, NULL, ChnOutputPortGetBuf, NULL);

    int fd = open("/mnt/test/720_576_yuv422.yuv", O_RDWR, 0666);
    if(fd > 0)
    {
        while(1)
        {
            int n = 0;

            memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
            stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
            MI_SYS_GetCurPts(&stBufConf.u64TargetPts);
            stBufConf.stRawCfg.u32Size = 102400;

            if(MI_SUCCESS == MI_SYS_ChnInputPortGetBuf(&stVdecChn0InputPort0,&stBufConf,&stBufInfo,&hHandle , -1))
            {
                stBufInfo.bEndOfStream = FALSE;

                n = read(fd , stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32BufSize);
                if(n < stBufInfo.stRawData.u32BufSize)
                {
                    stBufInfo.bEndOfStream = TRUE;
                    MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
                    break;
                }
                MI_SYS_FlushCache(stBufInfo.stRawData.pVirAddr,stBufInfo.stRawData.u32BufSize);
                MI_SYS_ChnInputPortPutBuf(hHandle ,&stBufInfo , FALSE);
            }
            usleep(10 * 1000);
        }
    }
    if(fd > 0)
        close(fd);
    printf("Vdec test end\n");
    
    pthread_join(tid,NULL);
    return 0;
}
