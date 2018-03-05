#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/resource.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_sys.h"
#include "st_hdmi.h"
#include "st_common.h"
#include "st_disp.h"
#include "st_vpe.h"
#include "st_vdisp.h"
#include "st_vif.h"
#include "mi_venc.h"
#include "st_fb.h"

#include "i2c.h"

pthread_t pt;
static MI_BOOL _bThreadRunning = FALSE;

static MI_BOOL g_subExit = FALSE;
static MI_BOOL g_bExit = FALSE;
static MI_U32 g_u32SubCaseIndex = 0;

static MI_U32 g_u32CaseIndex = 0;
static MI_U32 g_u32LastSubCaseIndex = 0;
static MI_U32 g_u32CurSubCaseIndex = 0;

#define MAX_VIF_DEV_NUM 4
#define MAX_VIF_CHN_NUM 16

#define SUPPORT_VIDEO_ENCODE

#define SUPPORT_UVC

#ifdef SUPPORT_UVC
#include "mi_uvc.h"
#endif

typedef struct
{
    MI_VENC_CHN vencChn;
    MI_U32 u32MainWidth;
    MI_U32 u32MainHeight;
    MI_VENC_ModType_e eType;
    int vencFd;
} VENC_Attr_t;

typedef struct
{
    pthread_t ptGetEs;
    pthread_t ptFillYuv;
    VENC_Attr_t stVencAttr[MAX_VIF_CHN_NUM];
    MI_U32 u32ChnNum;
    MI_BOOL bRunFlag;
} Venc_Args_t;

Venc_Args_t g_stVencArgs[MAX_VIF_CHN_NUM];

typedef struct ST_ChnInfo_s
{
    MI_S32 s32VideoFormat; //720P 1080P ...
    MI_S32 s32VideoType; //CVI TVI AHD CVBS ...
    MI_S32 s32VideoLost;
    MI_U8 u8EnChannel;
    MI_U8 u8ViDev;
    MI_U8 u8ViChn;
    MI_U8 u8ViPort; //main or sub
} ST_ChnInfo_t;

typedef struct ST_TestInfo_s
{
    ST_ChnInfo_t stChnInfo[MAX_VIF_CHN_NUM];
} ST_TestInfo_t;

//Config logic chn trans to phy chn
ST_VifChnConfig_t stVifChnCfg[VIF_MAX_CHN_NUM] = {
    {0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, //16main
    {0, 4, 0}, {0, 5, 0}, {0, 6, 0}, {0, 7, 0},
    {0, 8, 0}, {0, 9, 0}, {0, 10, 0}, {0, 11, 0},
    {0, 12, 0}, {0, 13, 0}, {0, 14, 0}, {0, 15, 0},
    {0, 0, 1}, {0, 1, 1}, {0, 2, 1}, {0, 3, 1}, //16sub
    {0, 4, 1}, {0, 5, 1}, {0, 6, 1}, {0, 7, 1},
    {0, 8, 1}, {0, 9, 1}, {0, 10, 1}, {0, 11, 1},
    {0, 12, 1}, {0, 13, 1}, {0, 14, 1}, {0, 15, 1},
};

ST_CaseDesc_t g_stVifCaseDesc[] =
{
    {
        .stDesc =
        {
            .u32CaseIndex = 0,
            .szDesc = "AD0-1x1080P Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 8,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1600x1200 timing",
                .eDispoutTiming = E_ST_TIMING_1600x1200_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 1,
            .szDesc = "AD1-1x1080P Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_JPEGE,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 2,
            .szDesc = "AD2-1x1080P Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H265E,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 3,
            .szDesc = "AD3-1x1080P Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 4,
            .szDesc = "4Chx1080P Video Capture",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_H264E,
                    }
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 5,
            .szDesc = "4ChxD1-AD0 Video Capture",
            .u32WndNum = 4,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 240,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 720,
                        .u16CapHeight = 240,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 2,
                        .u16CapWidth = 720,
                        .u16CapHeight = 240,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 3,
                        .u16CapWidth = 720,
                        .u16CapHeight = 240,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            }
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 6,
            .szDesc = "AD0-Chn0-1xD1 Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 720,
                        .u16CapHeight = 240,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 7,
            .szDesc = "AD0-Chn0-1x720P Video Capture",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1280,
                        .u16CapHeight = 720,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 8,
            .szDesc = "AD0-2x720P Video Capture",
            .u32WndNum = 2,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 3,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            }
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1280,
                        .u16CapHeight = 720,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 1,
                        .u16CapWidth = 1280,
                        .u16CapHeight = 720,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 9,
            .szDesc = "AD0-1x1080P Video Capture JPEG",
            .u32WndNum = 1,
        },
        .eDispoutTiming = E_ST_TIMING_1080P_60,
        .u32SubCaseNum = 8,
        .stSubDesc =
        {
            {
                .u32CaseIndex = 0,
                .szDesc = "change to 720P timing",
                .eDispoutTiming = E_ST_TIMING_720P_60,
            },
            {
                .u32CaseIndex = 1,
                .szDesc = "change to 1080P timing",
                .eDispoutTiming = E_ST_TIMING_1080P_60,
            },
            {
                .u32CaseIndex = 2,
                .szDesc = "change to 1280x1024 timing",
                .eDispoutTiming = E_ST_TIMING_1280x1024_60,
            },
            {
                .u32CaseIndex = 3,
                .szDesc = "change to 1024x768 timing",
                .eDispoutTiming = E_ST_TIMING_1024x768_60,
            },
            {
                .u32CaseIndex = 4,
                .szDesc = "change to 1600x1200 timing",
                .eDispoutTiming = E_ST_TIMING_1600x1200_60,
            },
            {
                .u32CaseIndex = 5,
                .szDesc = "change to 1440x900 timing",
                .eDispoutTiming = E_ST_TIMING_1440x900_60,
            },
            {
                .u32CaseIndex = 6,
                .szDesc = "exit",
                .eDispoutTiming = 0,
            },
            {
                .u32CaseIndex = 7,
                .szDesc = "zoom",
                .eDispoutTiming = 0,
            },
        },
        .stCaseArgs =
        {
            {
                .eVideoChnType = E_ST_VIF_CHN,
                .uChnArg =
                {
                    .stVifChnArg =
                    {
                        .u32Chn = 0,
                        .u16CapWidth = 1920,
                        .u16CapHeight = 1080,
                        .s32FrmRate = E_MI_VIF_FRAMERATE_FULL,
                        .eType = E_MI_VENC_MODTYPE_JPEGE,
                    }
                }
            },
        },
    },
    {
        .stDesc =
        {
            .u32CaseIndex = 10,
            .szDesc = "exit",
            .u32WndNum = 0,
        }
    }
};

void ST_VifUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32Size = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 i = 0;

    for (i = 0; i < u32Size; i ++)
    {
        printf("%d)\t %s\n", pstCaseDesc[i].stDesc.u32CaseIndex + 1, pstCaseDesc[i].stDesc.szDesc);
    }
}

void ST_CaseSubUsage(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 i = 0;

    if (u32CaseIndex < 0 || u32CaseIndex > u32CaseSize)
    {
        return;
    }

    for (i = 0; i < pstCaseDesc[u32CaseIndex].u32SubCaseNum; i ++)
    {
        printf("\t%d) %s\n", pstCaseDesc[u32CaseIndex].stSubDesc[i].u32CaseIndex + 1,
            pstCaseDesc[u32CaseIndex].stSubDesc[i].szDesc);
    }
}

#ifdef SUPPORT_UVC
MI_S32 UVC_Init(void *uvc)
{
    return MI_SUCCESS;
}

MI_S32 UVC_Deinit(void *uvc)
{
    return MI_SUCCESS;
}

MI_S32 _UVC_FillBuffer_Encoded(MI_UVC_Device_t *uvc, MI_U32 *length, void *buf)
{
    MI_SYS_ChnPort_t stVencChnInputPort;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    stVencChnInputPort.u32DevId = 2;
    if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[g_u32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stVencChnInputPort.u32DevId = 4;
    }
    else if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[g_u32CaseIndex].stCaseArgs[0].uChnArg.stVifChnArg.eType)
    {
        stVencChnInputPort.u32DevId = 2;
    }
    stVencChnInputPort.u32ChnId = 0;
    stVencChnInputPort.u32PortId = 0;

    hHandle = MI_HANDLE_NULL;
    memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

    if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle)))
    {
        if(hHandle == NULL)
        {
            printf("%s %d NULL output port buffer handle.\n", __func__, __LINE__);
        }
        else if(stBufInfo.stRawData.pVirAddr == NULL)
        {
            printf("%s %d unable to read buffer. VA==0\n", __func__, __LINE__);
        }
        else if(stBufInfo.stRawData.u32ContentSize >= 200 * 1024)  //MAX_OUTPUT_ES_SIZE in KO
        {
            printf("%s %d unable to read buffer. buffer overflow\n", __func__, __LINE__);
        }


        // len = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
        printf("send buf length : %d\n", stBufInfo.stRawData.u32ContentSize);
        *length = stBufInfo.stRawData.u32ContentSize;
        memcpy(buf, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);

        //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
        //    len, stBufInfo.stRawData.u32ContentSize);

        MI_SYS_ChnOutputPortPutBuf(hHandle);
    }
}

MI_S32 UVC_FillBuffer(void *uvc_dev,MI_U32 *length,void *buf)
{
    MI_UVC_Device_t *uvc = (MI_UVC_Device_t*)uvc_dev;

    switch(uvc->stream_param.fcc)
    {
        case V4L2_PIX_FMT_H264:
        {
            _UVC_FillBuffer_Encoded(uvc, length, buf);
        }
        break;

        case V4L2_PIX_FMT_MJPEG:
        {
            _UVC_FillBuffer_Encoded(uvc,length,buf);
        }
        break;

        default:
        {
            _UVC_FillBuffer_Encoded(uvc,length,buf);
            //printf("%s %d, not support this type, %d\n", __func__, __LINE__, uvc->stream_param.fcc);
        }
        break;
    }

    return MI_SUCCESS;
}

MI_S32 UVC_StartCapture(void *uvc,Stream_Params_t format)
{
    return MI_SUCCESS;
}

MI_S32 UVC_StopCapture(void *uvc)
{
    return MI_SUCCESS;
}

void ST_UVCInit()
{
    MI_U32 u32ChnId  = 0;
    MI_U32 u32PortId = 0;

    MI_UVC_Setting_t pstSet={4,USB_ISOC_MODE};
    MI_UVC_OPS_t fops = { UVC_Init ,
                          UVC_Deinit,
                          UVC_FillBuffer,
                          UVC_StartCapture,
                          UVC_StopCapture};

    MI_UVC_ChnAttr_t pstAttr ={pstSet,fops};
    MI_UVC_Init("/dev/video0");
    MI_UVC_CreateDev(u32ChnId,u32PortId,&pstAttr);
    MI_UVC_StartDev();
}
#endif

void *ST_VencGetEsBufferProc(void *args)
{
    Venc_Args_t *pArgs = (Venc_Args_t *)args;

    MI_SYS_ChnPort_t stVencChnInputPort;
    char szFileName[128];
    int fd = -1;
    MI_SYS_BUF_HANDLE hHandle;
    MI_SYS_BufInfo_t stBufInfo;
    MI_S32 s32Ret = MI_SUCCESS;
    MI_S32 len = 0;

    stVencChnInputPort.eModId = E_MI_MODULE_ID_VENC;
    if (E_MI_VENC_MODTYPE_H264E == pArgs->stVencAttr[0].eType)
    {
        stVencChnInputPort.u32DevId = 2;
    }
    else if (E_MI_VENC_MODTYPE_H265E == pArgs->stVencAttr[0].eType)
    {
        stVencChnInputPort.u32DevId = 0;
    }
    else if (E_MI_VENC_MODTYPE_JPEGE == pArgs->stVencAttr[0].eType)
    {
        stVencChnInputPort.u32DevId = 4;
    }
    stVencChnInputPort.u32ChnId = pArgs->stVencAttr[0].vencChn;
    stVencChnInputPort.u32PortId = 0;

    memset(szFileName, 0, sizeof(szFileName));
    snprintf(szFileName, sizeof(szFileName) - 1, "venc_dev%d_chn%d_port%d_%dx%d_%s.es",
            stVencChnInputPort.u32DevId, stVencChnInputPort.u32ChnId, stVencChnInputPort.u32PortId,
            pArgs->stVencAttr[0].u32MainWidth, pArgs->stVencAttr[0].u32MainHeight,
            (pArgs->stVencAttr[0].eType == E_MI_VENC_MODTYPE_H264E) ? "h264" : "h265");

    fd = open(szFileName, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd <= 0)
    {
        printf("%s %d create %s error\n", __func__, __LINE__, szFileName);
        return NULL;
    }

    printf("%s %d create %s success\n", __func__, __LINE__, szFileName);

    while (1)
    {
        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        if(MI_SUCCESS == (s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChnInputPort, &stBufInfo, &hHandle)))
        {
            if(hHandle == NULL)
            {
                printf("%s %d NULL output port buffer handle.\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.pVirAddr == NULL)
            {
                printf("%s %d unable to read buffer. VA==0\n", __func__, __LINE__);
            }
            else if(stBufInfo.stRawData.u32ContentSize >= 200 * 1024)  //MAX_OUTPUT_ES_SIZE in KO
            {
                printf("%s %d unable to read buffer. buffer overflow\n", __func__, __LINE__);
            }

            len = write(fd, stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);

            //printf("%s %d, chn:%d write frame len=%d, real len=%d\n", __func__, __LINE__, stVencChnInputPort.u32ChnId,
            //    len, stBufInfo.stRawData.u32ContentSize);

            MI_SYS_ChnOutputPortPutBuf(hHandle);
        }
        else
        {
             //printf("%s %d, MI_SYS_ChnOutputPortGetBuf error, %X\n", __func__, __LINE__, s32Ret);
            usleep(10 * 1000);//sleep 1 ms
        }
    }

    close(fd);
}

void *st_GetOutputDataThread(void * args)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    MI_S32 s32Ret = MI_SUCCESS, s32VoChannel = 0;
    MI_S32 s32TimeOutMs = 20;
    MI_S32 s32ReadCnt = 0;
    FILE *fp = NULL;

    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VPE;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = s32VoChannel;
    stChnPort.u32PortId = 2;
    printf("..st_GetOutputDataThread.s32VoChannel(%d)...\n", s32VoChannel);

    s32ReadCnt = 0;

    MI_SYS_SetChnOutputPortDepth(&stChnPort, 1, 3); //Default queue frame depth--->20
    fp = fopen("vpe_2.yuv","wb");
    while (!_bThreadRunning)
    {
        if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hHandle))
        {
            if (E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420 == stBufInfo.stFrameData.ePixelFormat)
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u16Width;

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                        fwrite(stBufInfo.stFrameData.pVirAddr[1], size/2, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
            else
            {
                int size = stBufInfo.stFrameData.u16Height * stBufInfo.stFrameData.u32Stride[0];

                if (0 == (s32ReadCnt++ % 30))
                {
                    if (fp)
                    {
                        fwrite(stBufInfo.stFrameData.pVirAddr[0], size, 1, fp);
                    }
                    printf("\t\t\t\t\t vif(%d) get buf cnt (%d)...w(%d)...h(%d)..\n", s32VoChannel, s32ReadCnt, stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height);
                }
            }
            MI_SYS_ChnOutputPortPutBuf(hHandle);
            if(stBufInfo.bEndOfStream)
                break;
        }
        usleep(10*1000);
    }
    printf("\n\n");
    usleep(3000000);

    return NULL;
}

int ST_SplitWindow()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32LastWndNum = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 1;
    MI_U16 u16DispWidth = 0, u16DispHeight = 0;
    MI_S32 s32HdmiTiming = 0, s32DispTiming = 0;
    ST_Sys_BindInfo_t stBindInfo;
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    MI_SYS_WindowRect_t stVpeRect;

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    /*
    VDEC->DIVP->VDISP->DISP

    (1) unbind VDEC DIVP
    (2) VDISP disable input port
    (3) divp set output port attr
    (4) vdisp set input port attr
    (5) bind vdec divp
    (6) enable vdisp input port

    VDEC->DIVP->VPE->VDISP->DISP
    (1) unbind vdec divp
    (2) unbind divp vpe
    (3) vdisp disable input port
    (4) divp set chn attr
    (5) vpe set port mode
    (6) vdisp set input port attr
    (7) bind vdec divp
    (8) bind divp vpe
    (9) enable vdisp input port
    */

    u32CurWndNum = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    u32LastWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    if (u32CurWndNum == u32LastWndNum)
    {
        printf("same wnd num, skip\n");
        return 0;
    }
    else
    {
        printf("split window from %d to %d\n", u32LastWndNum, u32CurWndNum);
    }

    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[u32CaseIndex].eDispoutTiming,
                &s32HdmiTiming, &s32DispTiming, &u16DispWidth, &u16DispHeight));

    printf("%s %d, u16DispWidth:%d,u16DispHeight:%d\n", __func__, __LINE__, u16DispWidth,
        u16DispHeight);

    // 1, unbind VDEC to DIVP
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    // 2, unbind DIVP to VPE
    for (i = 0; i < u32LastWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    // 3, disable vdisp input port
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_VDISP_PORT vdispPort = 0;
    MI_VDISP_InputPortAttr_t stInputPortAttr;

#if 1
    // stop divp and vpe
    for (i = 0; i < u32LastWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StopChn(i));
    }

    for (i = 0; i < u32LastWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_DisablePort(vpeChn, 0));
    }
#endif

    for (i = 0; i < u32LastWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
    }

    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }

    // 4, divp set output port chn attr
#if 0
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;
    MI_DIVP_CHN divpChn = 0;
    for (i = 0; i < u32CurWndNum; i++)
    {
        divpChn = i;
        STCHECKRESULT(MI_DIVP_GetOutputPortAttr(divpChn, &stOutputPortAttr));
        printf("change divp from %dx%d ", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        stOutputPortAttr.u32Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stOutputPortAttr.u32Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        printf("to %dx%d\n", stOutputPortAttr.u32Width, stOutputPortAttr.u32Height);
        STCHECKRESULT(MI_DIVP_SetOutputPortAttr(divpChn, &stOutputPortAttr));
    }
#endif

    // 5, set vpe port mode
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16DispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }

    // 6, set vdisp input port attribute
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16DispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16DispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16DispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16DispHeight / u32Square, 2);

        printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
             stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }

    // start divp and vpe
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_DIVP_StartChn(i));
    }

    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;
        STCHECKRESULT(MI_VPE_EnablePort(vpeChn, 0));
    }

    // 9, enable vdisp input port
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
    }

    // 7, bind VDEC to DIVP
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 0;
        stBindInfo.u32DstFrmrate = 0;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    // 8, bind DIVP to VPE
    for (i = 0; i < u32CurWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_DIVP;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i; //only equal zero
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;

        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

    pstCaseDesc[u32CaseIndex].u32ShowWndNum =
        pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].u32WndNum;
    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;

    return 0;
}

MI_S32 ST_SubExit()
{
    MI_U32 u32WndNum = 0, u32ShowWndNum;
    MI_S32 i = 0;
    ST_Sys_BindInfo_t stBindInfo;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    u32WndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;
    u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].u32ShowWndNum;

    /************************************************
    step1:  unbind VIF to VPE
    *************************************************/
    for (i = 0; i < u32WndNum; i++)
    {
        STCHECKRESULT(ST_Vif_StopPort(stVifChnCfg[i].u8ViChn, stVifChnCfg[i].u8ViPort));
    }
    for (i = 0; i < u32ShowWndNum; i++)
    {
        memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));

        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = stVifChnCfg[i].u8ViDev;
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = stVifChnCfg[i].u8ViPort;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step4:  unbind VPE to VDISP
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = 0;

        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;

        STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));
    }

    /************************************************
    step5:  destroy vif  vpe vdisp disp
    *************************************************/
    for (i = 0; i < u32ShowWndNum; i++)
    {
        STCHECKRESULT(ST_Vpe_StopPort(i, 0));
        STCHECKRESULT(ST_Vpe_StopChannel(i));
        STCHECKRESULT(ST_Vpe_DestroyChannel(i));
    }

    STCHECKRESULT(ST_Vif_DisableDev(0));//0 1 2 3?

    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0 , u32ShowWndNum)); //disp input port 0
    STCHECKRESULT(ST_Hdmi_DeInit(E_MI_HDMI_ID_0));
    STCHECKRESULT(ST_Fb_DeInit());
    STCHECKRESULT(ST_Sys_Exit());

    return MI_SUCCESS;
}

int ST_ChangeDisplayTiming()
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32CaseSize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32CurSubCaseIndex = g_u32CurSubCaseIndex;
    MI_U32 u32LastSubCaseIndex = g_u32LastSubCaseIndex;
    ST_DispoutTiming_e eLastDispoutTiming = E_ST_TIMING_MAX;
    ST_DispoutTiming_e eCurDispoutTiming = E_ST_TIMING_MAX;
    MI_S32 s32LastHdmiTiming, s32CurHdmiTiming;
    MI_S32 s32LastDispTiming, s32CurDispTiming;
    MI_U16 u16LastDispWidth = 0, u16LastDispHeight = 0;
    MI_U16 u16CurDispWidth = 0, u16CurDispHeight = 0;
    MI_U32 u32CurWndNum = 0;
    MI_U32 u32TotalWnd = 0;
    MI_U32 i = 0;
    MI_U32 u32Square = 0;
    ST_Sys_BindInfo_t stBindInfo;

    if (u32CurSubCaseIndex < 0 || u32LastSubCaseIndex < 0)
    {
        printf("error index\n");
        return 0;
    }

    eCurDispoutTiming = pstCaseDesc[u32CaseIndex].stSubDesc[u32CurSubCaseIndex].eDispoutTiming;
    eLastDispoutTiming = pstCaseDesc[u32CaseIndex].eDispoutTiming;

    if (eCurDispoutTiming == eLastDispoutTiming)
    {
        printf("the same timing, skip\n");
        return 0;
    }

    u32CurWndNum = pstCaseDesc[u32CaseIndex].u32ShowWndNum;

    STCHECKRESULT(ST_GetTimingInfo(eCurDispoutTiming,
                &s32CurHdmiTiming, &s32CurDispTiming, &u16CurDispWidth, &u16CurDispHeight));

    STCHECKRESULT(ST_GetTimingInfo(eLastDispoutTiming,
                &s32LastHdmiTiming, &s32LastDispTiming, &u16LastDispWidth, &u16LastDispHeight));

    printf("change from %dx%d to %dx%d\n", u16LastDispWidth, u16LastDispHeight, u16CurDispWidth,
                u16CurDispHeight);

    /*
    (1), stop HDMI
    (2), stop VDISP
    (3), stop DISP
    (4), set vpe port mode
    (5), set vdisp input port chn attr
    (6), start disp
    (7), start vdisp
    (8), star HDMI
    */

    if (u32CurWndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32CurWndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32CurWndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32CurWndNum <= 16)
    {
        u32Square = 4;
    }

    // stop hdmi
    ExecFunc(MI_HDMI_Stop(E_MI_HDMI_ID_0), MI_SUCCESS);

    // stop vdisp
    MI_VDISP_DEV vdispDev = MI_VDISP_DEV_0;
    MI_S32 s32FrmRate = 30;
    MI_S32 s32OutputPort = 0;
    MI_VDISP_PORT vdispPort = 0;

    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    STCHECKRESULT(ST_Sys_UnBind(&stBindInfo));

    STCHECKRESULT(ST_Vdisp_StopDevice(vdispDev));

    // stop disp
    STCHECKRESULT(ST_Disp_DeInit(ST_DISP_DEV0, 0, DISP_MAX_CHN));

    // set vpe port mode
    MI_VPE_CHANNEL vpeChn = 0;
    MI_VPE_PortMode_t stVpeMode;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vpeChn = i;

        memset(&stVpeMode, 0, sizeof(MI_VPE_PortMode_t));
        ExecFunc(MI_VPE_GetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
        stVpeMode.u16Width = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stVpeMode.u16Height = ALIGN_BACK(u16CurDispHeight / u32Square, 2);
        ExecFunc(MI_VPE_SetPortMode(vpeChn, 0, &stVpeMode), MI_VPE_OK);
    }
    // start vdisp
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_DisableInputPort(vdispDev, vdispPort));
    }
    ST_Rect_t stdispRect, stRect;
    stdispRect.u16PicW = u16CurDispWidth;
    stdispRect.u16PicH = u16CurDispHeight;
    STCHECKRESULT(ST_Vdisp_SetOutputPortAttr(vdispDev, s32OutputPort, &stdispRect, s32FrmRate, 1));

    // set vdisp input port chn attr
    MI_VDISP_InputPortAttr_t stInputPortAttr;
    for (i = 0; i < u32CurWndNum; i++)
    {
        vdispPort = i;

        memset(&stInputPortAttr, 0, sizeof(MI_VDISP_InputPortAttr_t));

        STCHECKRESULT(MI_VDISP_GetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));

        stInputPortAttr.u32OutX = ALIGN_BACK((u16CurDispWidth / u32Square) * (i % u32Square), 2);
        stInputPortAttr.u32OutY = ALIGN_BACK((u16CurDispHeight / u32Square) * (i / u32Square), 2);
        stInputPortAttr.u32OutWidth = ALIGN_BACK(u16CurDispWidth / u32Square, 2);
        stInputPortAttr.u32OutHeight = ALIGN_BACK(u16CurDispHeight / u32Square, 2);

        // printf("%s %d, u32OutWidth:%d,u32OutHeight:%d,u32Square:%d\n", __func__, __LINE__, stInputPortAttr.u32OutWidth,
        //     stInputPortAttr.u32OutHeight, u32Square);
        STCHECKRESULT(MI_VDISP_SetInputPortAttr(vdispDev, vdispPort, &stInputPortAttr));
    }
    for (i = 0; i < u32CurWndNum; i++)
    {
        STCHECKRESULT(MI_VDISP_EnableInputPort(vdispDev, vdispPort));
    }

    STCHECKRESULT(ST_Vdisp_StartDevice(vdispDev));

    // start disp
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, s32CurDispTiming));
    memset(&stBindInfo, 0, sizeof(ST_Sys_BindInfo_t));
    stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VDISP;
    stBindInfo.stSrcChnPort.u32DevId = 0;
    stBindInfo.stSrcChnPort.u32ChnId = 0;
    stBindInfo.stSrcChnPort.u32PortId = 0;

    stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stBindInfo.stDstChnPort.u32DevId = 0;
    stBindInfo.stDstChnPort.u32ChnId = 0;
    stBindInfo.stDstChnPort.u32PortId = 0;
    stBindInfo.u32SrcFrmrate = 30;
    stBindInfo.u32DstFrmrate = 30;
    STCHECKRESULT(ST_Sys_Bind(&stBindInfo));

    // start HDMI
    STCHECKRESULT(ST_Hdmi_Start(E_MI_HDMI_ID_0, s32CurHdmiTiming));

    pstCaseDesc[u32CaseIndex].eDispoutTiming = eCurDispoutTiming;

    g_u32LastSubCaseIndex = g_u32CurSubCaseIndex;
}

void ST_WaitSubCmd(void)
{
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    char szCmd[16];
    MI_U32 index = 0;
    MI_U32 u32CaseIndex = g_u32CaseIndex;
    MI_U32 u32SubCaseSize = pstCaseDesc[u32CaseIndex].u32SubCaseNum;

    while (!g_subExit)
    {
        ST_CaseSubUsage();

        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        index = atoi(szCmd);

        if (index <= 0 || index > u32SubCaseSize)
        {
            continue;
        }

        g_u32CurSubCaseIndex = index - 1;

        if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
        {
            ST_ChangeDisplayTiming(); //change timing
        }
        else
        {
            if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
            {
                ST_SubExit();
                return;
            }
            else if (0 == (strncmp(pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
            {
            }
            else
            {
                if (pstCaseDesc[u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
                {
                    ST_SplitWindow(); //switch screen
                }
            }
        }
    }
}

MI_S32 ST_VifToDisp(MI_S32 s32CaseIndex)
{
    MI_VIF_DEV VifDevId = 0;
    MI_U16 u16DispLayerW = 1920;
    MI_U16 u16DispLayerH = 1080;
    ST_VPE_ChannelInfo_t stVpeChannelInfo;
    ST_VIF_PortInfo_t stVifPortInfoInfo;
    ST_Rect_t stdispRect = {0, 0, 1920, 1080};
    ST_VPE_PortInfo_t stPortInfo;
    ST_Rect_t stRect;
    ST_Sys_BindInfo_t stBindInfo;
    MI_HDMI_DeviceId_e eHdmi = E_MI_HDMI_ID_0;
    MI_HDMI_TimingType_e eHdmiTiming = E_MI_HDMI_TIMING_1080_60P;
    MI_DISP_OutputTiming_e eDispoutTiming = E_MI_DISP_OUTPUT_1080P60;
    MI_S32 s32CapChnNum = 0, s32DispChnNum = 0, i = 0;
    MI_S32 s32DevNum = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;
    MI_U32 u32ArraySize = ARRAY_SIZE(g_stVifCaseDesc);
    MI_U32 u32Square = 0;
    MI_U32 u32SubCaseSize = pstCaseDesc[s32CaseIndex].u32SubCaseNum;
    MI_U32 u32WndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    ST_Rect_t stDispWndRect[16] = {0};
    MI_S32 s32AdId[4];
    MI_S32 s32AdWorkMode = 0;

    s32DispChnNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    s32CapChnNum = s32DispChnNum;
    STCHECKRESULT(ST_GetTimingInfo(pstCaseDesc[s32CaseIndex].eDispoutTiming, (MI_S32 *)&eHdmiTiming,
        (MI_S32 *)&eDispoutTiming, (MI_U16*)&stdispRect.u16PicW, (MI_U16*)&stdispRect.u16PicH));
    for (i = 0; i < 4; i++)
    {
        s32AdId[i] = 0;
    }
    if (u32WndNum <= 1)
    {
        u32Square = 1;
    }
    else if (u32WndNum <= 4)
    {
        u32Square = 2;
    }
    else if (u32WndNum <= 9)
    {
        u32Square = 3;
    }
    else if (u32WndNum <= 16)
    {
        u32Square = 4;
    }

    if (6 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 3, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 3, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 2, 2},
            {2, 0, 1, 1},
            {2, 1, 1, 1},
            {0, 2, 1, 1},
            {1, 2, 1, 1},
            {2, 2, 1, 1},
        };//3x3 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
    }
    else if (8 == u32WndNum) //irrgular split
    {
        MI_U16 u16DivW = ALIGN_BACK(stdispRect.u16PicW / 4, 2);
        MI_U16 u16DivH = ALIGN_BACK(stdispRect.u16PicH / 4, 2);
        ST_Rect_t stRectSplit[] =
        {
            {0, 0, 3, 3},
            {3, 0, 1, 1},
            {3, 1, 1, 1},
            {3, 2, 1, 1},
            {0, 3, 1, 1},
            {1, 3, 1, 1},
            {2, 3, 1, 1},
            {3, 3, 1, 1},
        };//4x4 split div

        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X = stRectSplit[i].s32X * u16DivW;
            stDispWndRect[i].s32Y = stRectSplit[i].s32Y * u16DivH;
            stDispWndRect[i].u16PicW = stRectSplit[i].u16PicW * u16DivW;
            stDispWndRect[i].u16PicH = stRectSplit[i].u16PicH * u16DivH;
        }
    }
    else
    {
        for (i = 0; i < u32WndNum; i++)
        {
            stDispWndRect[i].s32X    = ALIGN_BACK((stdispRect.u16PicW / u32Square) * (i % u32Square), 2);
            stDispWndRect[i].s32Y    = ALIGN_BACK((stdispRect.u16PicH / u32Square) * (i / u32Square), 2);
            stDispWndRect[i].u16PicW = ALIGN_BACK((stdispRect.u16PicW / u32Square), 2);
            stDispWndRect[i].u16PicH = ALIGN_BACK((stdispRect.u16PicH / u32Square), 2);
        }
    }
    /************************************************
    Step1:  init SYS
    *************************************************/
    STCHECKRESULT(ST_Sys_Init());

    ExecFunc(vif_i2c_init(), 0);
    /************************************************
    Step2:  init HDMI
    *************************************************/
    STCHECKRESULT(ST_Hdmi_Init());

    /************************************************
    Step3:  init VIF
    *************************************************/
    s32DevNum = s32CapChnNum / 4;

    /*
        case 0: AD_A CHN0 1*FHD
        case 1: AD_B CHN0 1*FHD
        case 2: AD_C CHN0 1*FHD
        case 3: AD_D CHN0 1*FHD
        case 4: AD_A-AD_D CHN0/CHN4/CHN8/CHN12
        case 5: AD_A 4*D1 CHN0/CHN4/CHN8/CHN12
    */
    switch (s32CaseIndex)
    {
        case 0:
        case 9:
            s32AdId[0] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 0;
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        case 1:
            s32AdId[1] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 4;
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        case 2:
            s32AdId[2] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 8;
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        case 3:
            s32AdId[3] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 12;
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        case 4:
            for (i = 0; i < 4; i++)
            {
                s32AdId[i] = 1;
                stVifChnCfg[i].u8ViDev = 0;
                stVifChnCfg[i].u8ViChn = 4 * i; //0 4 8 12
            }
            s32AdWorkMode = SAMPLE_VI_MODE_1_1080P;
            break;
        case 5:
            s32AdId[0] = 1;//AD0 -4XD1
            for (i = 0; i < 4; i++)
            {
                stVifChnCfg[i].u8ViDev = 0;
                stVifChnCfg[i].u8ViChn = i;
            }
            s32AdWorkMode = SAMPLE_VI_MODE_4_D1;
            break;
        case 6:
            s32AdId[0] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 0;
            s32AdWorkMode = SAMPLE_VI_MODE_1_D1;
            break;
        case 7:
            s32AdId[0] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 0;
            s32AdWorkMode = SAMPLE_VI_MODE_4_720P;
            break;
        case 8:
            s32AdId[0] = 1;
            stVifChnCfg[0].u8ViDev = 0;
            stVifChnCfg[0].u8ViChn = 0;
            stVifChnCfg[1].u8ViDev = 0;
            stVifChnCfg[1].u8ViChn = 2;
            s32AdWorkMode = SAMPLE_VI_MODE_4_720P;
            break;
        default:
            ST_DBG("Unkown test case(%d)!\n", s32CaseIndex);
            return 0;
    }
    for (i = 0; i < MAX_VIF_DEV_NUM; i++) //init vif device
    {
        if (s32AdId[i])
        {
            ST_DBG("ST_Vif_CreateDev....DEV(%d)...s32AdWorkMode(%d)...\n", i, s32AdWorkMode);
            STCHECKRESULT(ST_Vif_CreateDev(i, s32AdWorkMode));
        }
    }
    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        memset(&stVifPortInfoInfo, 0x0, sizeof(ST_VIF_PortInfo_t));
        stVifPortInfoInfo.u32RectX = 0;
        stVifPortInfoInfo.u32RectY = 0;
        stVifPortInfoInfo.u32RectWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVifPortInfoInfo.u32RectHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        stVifPortInfoInfo.u32DestWidth = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVifPortInfoInfo.u32DestHeight = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        STCHECKRESULT(ST_Vif_CreatePort(stVifChnCfg[i].u8ViChn, 0, &stVifPortInfoInfo));
        //STCHECKRESULT(ST_Vif_StartPort(stVifChnCfg[i].u8ViChn, 0));

        ST_NOP("============vif channel(%d) x(%d)-y(%d)-w(%d)-h(%d)..\n", i, stVifPortInfoInfo.u32RectX, stVifPortInfoInfo.u32RectY,
            stVifPortInfoInfo.u32RectWidth, stVifPortInfoInfo.u32RectHeight);
    }

    /************************************************
    Step4:  init VPE
    *************************************************/
    for (i = 0; i < s32CapChnNum; i++)
    {
        stVpeChannelInfo.u16VpeMaxW = 1920;
        stVpeChannelInfo.u16VpeMaxH = 1080; // max res support to FHD
        stVpeChannelInfo.u32X = 0;
        stVpeChannelInfo.u32Y = 0;
        stVpeChannelInfo.u16VpeCropW = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth;
        stVpeChannelInfo.u16VpeCropH = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight;
        STCHECKRESULT(ST_Vpe_CreateChannel(i, &stVpeChannelInfo));
        stPortInfo.DepVpeChannel = i;
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV;
        stPortInfo.u16OutputWidth = stDispWndRect[i].u16PicW;
        stPortInfo.u16OutputHeight = stDispWndRect[i].u16PicH;
        STCHECKRESULT(ST_Vpe_CreatePort(DISP_PORT, &stPortInfo)); //default support port0 --->>> vdisp
        STCHECKRESULT(ST_Vpe_StartChannel(i));

#ifdef SUPPORT_VIDEO_ENCODE
        stPortInfo.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
        stPortInfo.u16OutputWidth = ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
        stPortInfo.u16OutputHeight = ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
        STCHECKRESULT(ST_Vpe_CreatePort(MAIN_VENC_PORT, &stPortInfo)); //default support port2 --->>> venc
#endif
        ST_NOP("============vpe channel(%d) x(%d)-y(%d)-w(%d)-h(%d).outw(%d)-outh(%d).\n", i, stVpeChannelInfo.u32X, stVpeChannelInfo.u32Y,
            stVpeChannelInfo.u16VpeCropW, stVpeChannelInfo.u16VpeCropH, stPortInfo.u16OutputWidth, stPortInfo.u16OutputHeight);
    }

    /************************************************
    Step6:  init DISP
    *************************************************/
    STCHECKRESULT(ST_Disp_DevInit(ST_DISP_DEV0, ST_DISP_LAYER0, eDispoutTiming)); //Dispout timing

    ST_DispChnInfo_t stDispChnInfo;
    stDispChnInfo.InputPortNum = s32DispChnNum;
    for (i = 0; i < s32DispChnNum; i++)
    {
        stDispChnInfo.stInputPortAttr[i].u32Port = i;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X = stDispWndRect[i].s32X;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y = stDispWndRect[i].s32Y;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width = stDispWndRect[i].u16PicW;
        stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height = stDispWndRect[i].u16PicH;
        ST_NOP("===========disp channel(%d) x(%d)-y(%d)-w(%d)-h(%d)...\n", i,
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16X,
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Y,
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Width,
            stDispChnInfo.stInputPortAttr[i].stAttr.stDispWin.u16Height);
    }
    STCHECKRESULT(ST_Disp_ChnInit(0, &stDispChnInfo));

    // must init after disp
    ST_Fb_Init(E_MI_FB_COLOR_FMT_ARGB1555);
    ST_FB_Show(FALSE);

    /************************************************
    Step7:  Bind VIF->VPE
    *************************************************/
    for (i = 0; i < s32CapChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VIF;
        stBindInfo.stSrcChnPort.u32DevId = 0; //VIF dev == 0
        stBindInfo.stSrcChnPort.u32ChnId = stVifChnCfg[i].u8ViChn;
        stBindInfo.stSrcChnPort.u32PortId = 0; //Main stream
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = DISP_PORT;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

#ifdef SUPPORT_VIDEO_ENCODE //Create Video encode Channel
    // main+sub venc
    MI_VENC_CHN VencChn = 0;
    MI_S32 s32Ret = 0;
    MI_VENC_ChnAttr_t stChnAttr;
    MI_SYS_ChnPort_t stVencChnOutputPort;

    printf("%s %d, total chn num:%d\n", __func__, __LINE__, s32CapChnNum);

    for (i = 0; i < s32CapChnNum; i ++)
    {
        VencChn = i;

        memset(&stChnAttr, 0, sizeof(MI_VENC_ChnAttr_t));
        memset(&stVencChnOutputPort, 0, sizeof(MI_SYS_ChnPort_t));

        //printf("%s %d, chn:%d,eType:%d\n", __func__, __LINE__, VencChn,
        //    pstCaseDesc[u32CaseIndex].stCaseArgs[i].uChnArg.stVencChnArg.eType);
        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stVencChnOutputPort.u32DevId = 2;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChnAttr.stVeAttr.stAttrH264e.u32BFrameNum = 2; // not support B frame
            stChnAttr.stVeAttr.stAttrH264e.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrH264e.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrH264e.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H264FIXQP;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32Gop = 30;

            stChnAttr.stRcAttr.stAttrH264FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH264FixQp.u32PQp = 25;
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stVencChnOutputPort.u32DevId = 0;
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChnAttr.stVeAttr.stAttrH265e.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrH265e.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrH265e.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_H265FIXQP;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRate =
                pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.s32FrmRate;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32Gop = 30;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32IQp = 25;
            stChnAttr.stRcAttr.stAttrH265FixQp.u32PQp = 25;
        }
        else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            stChnAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stVencChnOutputPort.u32DevId = 4;//E_MI_VENC_DEV_JPEG
            stChnAttr.stRcAttr.eRcMode = E_MI_VENC_RC_MODE_MJPEGFIXQP;
            stChnAttr.stVeAttr.stAttrJpeg.u32PicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32PicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicWidth =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapWidth, 32);
            stChnAttr.stVeAttr.stAttrJpeg.u32MaxPicHeight =
                ALIGN_N(pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.u16CapHeight, 32);
        }
        s32Ret = MI_VENC_CreateChn(VencChn, &stChnAttr);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_CreateChn %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
        if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            MI_VENC_ParamJpeg_t stParamJpeg;

            memset(&stParamJpeg, 0, sizeof(stParamJpeg));
            s32Ret = MI_VENC_GetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
            printf("Get Qf:%d\n", stParamJpeg.u32Qfactor);

            stParamJpeg.u32Qfactor = 100;
            s32Ret = MI_VENC_SetJpegParam(VencChn, &stParamJpeg);
            if(s32Ret != MI_SUCCESS)
            {
                return s32Ret;
            }
        }
        stVencChnOutputPort.eModId = E_MI_MODULE_ID_VENC;
        stVencChnOutputPort.u32ChnId = VencChn;
        stVencChnOutputPort.u32PortId = 0;
        //This was set to (5, 10) and might be too big for kernel
        s32Ret = MI_SYS_SetChnOutputPortDepth(&stVencChnOutputPort, 2, 5);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_SYS_SetChnOutputPortDepth %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }

        s32Ret = MI_VENC_StartRecvPic(VencChn);
        if (MI_SUCCESS != s32Ret)
        {
            printf("%s %d, MI_VENC_StartRecvPic %d error, %X\n", __func__, __LINE__, VencChn, s32Ret);
        }
    }
#endif

    /************************************************
    Step8:  Bind VPE->VDISP
    *************************************************/
    for (i = 0; i < s32DispChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = DISP_PORT;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
        stBindInfo.stDstChnPort.u32DevId = 0;
        stBindInfo.stDstChnPort.u32ChnId = 0;
        stBindInfo.stDstChnPort.u32PortId = i;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }

#ifdef SUPPORT_VIDEO_ENCODE
    for (i = 0; i < s32DispChnNum; i++)
    {
        stBindInfo.stSrcChnPort.eModId = E_MI_MODULE_ID_VPE;
        stBindInfo.stSrcChnPort.u32DevId = 0;
        stBindInfo.stSrcChnPort.u32ChnId = i;
        stBindInfo.stSrcChnPort.u32PortId = MAIN_VENC_PORT;
        stBindInfo.stDstChnPort.eModId = E_MI_MODULE_ID_VENC;
        if (E_MI_VENC_MODTYPE_H264E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            if (i % 2 == 0)
            {
                stBindInfo.stDstChnPort.u32DevId = 2;
            }
            else
            {
                stBindInfo.stDstChnPort.u32DevId = 3;
            }
        }
        else if (E_MI_VENC_MODTYPE_H265E == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            if (i % 2 == 0)
            {
                stBindInfo.stDstChnPort.u32DevId = 0;
            }
            else
            {
                stBindInfo.stDstChnPort.u32DevId = 1;
            }
        }
        else if (E_MI_VENC_MODTYPE_JPEGE == pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType)
        {
            if (i % 2 == 0)
            {
                stBindInfo.stDstChnPort.u32DevId = 4;
            }
            else
            {
                stBindInfo.stDstChnPort.u32DevId = 5;
            }
        }
        stBindInfo.stDstChnPort.u32ChnId = i;
        stBindInfo.stDstChnPort.u32PortId = 0;
        stBindInfo.u32SrcFrmrate = 30;
        stBindInfo.u32DstFrmrate = 30;
        STCHECKRESULT(ST_Sys_Bind(&stBindInfo));
    }
#endif
    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        ST_NOP("=================ST_Vif_StartPort...i(%d)..vichn(%d)..===========\n", i, stVifChnCfg[i].u8ViChn);
        STCHECKRESULT(ST_Vif_StartPort(stVifChnCfg[i].u8ViChn, 0));
    }
#ifdef SUPPORT_VIDEO_ENCODE
    for (i = 0; i < s32CapChnNum; i++) //init vif channel
    {
        g_stVencArgs[i].stVencAttr[0].vencChn = i;
        g_stVencArgs[i].stVencAttr[0].u32MainWidth = stDispWndRect[i].u16PicW;
        g_stVencArgs[i].stVencAttr[0].u32MainHeight = stDispWndRect[i].u16PicH;
        g_stVencArgs[i].stVencAttr[0].eType = pstCaseDesc[s32CaseIndex].stCaseArgs[i].uChnArg.stVifChnArg.eType;
        g_stVencArgs[i].stVencAttr[0].vencFd = MI_VENC_GetFd(VencChn);
        g_stVencArgs[i].bRunFlag = TRUE;
        // pthread_create(&g_stVencArgs[i].ptGetEs, NULL, ST_VencGetEsBufferProc, (void *)&g_stVencArgs[i]);
    }
#endif

#ifdef SUPPORT_UVC
    ST_UVCInit();
#endif

    STCHECKRESULT(ST_Hdmi_Start(eHdmi, eHdmiTiming)); //Hdmi timing
    g_u32LastSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    g_u32CurSubCaseIndex = pstCaseDesc[s32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[s32CaseIndex].u32ShowWndNum = pstCaseDesc[s32CaseIndex].stDesc.u32WndNum;
    //pthread_create(&pt, NULL, st_GetOutputDataThread, NULL);

    return MI_SUCCESS;
}

void ST_DealCase(int argc, char **argv)
{
    MI_U32 u32Index = 0;
    MI_U32 u32SubIndex = 0;
    ST_CaseDesc_t *pstCaseDesc = g_stVifCaseDesc;

    if (argc != 3)
    {
        return;
    }

    u32Index = atoi(argv[1]);
    u32SubIndex = atoi(argv[2]);

    if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))//case num
    {
        printf("case index range (%d~%d)\n", 1, ARRAY_SIZE(g_stVifCaseDesc));
        return;
    }
    g_u32CaseIndex = u32Index - 1;//real array index

    if (u32SubIndex <= 0 || u32SubIndex > pstCaseDesc[g_u32CaseIndex].u32SubCaseNum)
    {
        printf("sub case index range (%d~%d)\n", 1, pstCaseDesc[g_u32CaseIndex].u32SubCaseNum);
        return;
    }

    g_u32LastSubCaseIndex = pstCaseDesc[g_u32CaseIndex].u32SubCaseNum - 1;
    pstCaseDesc[g_u32CaseIndex].u32ShowWndNum = pstCaseDesc[g_u32CaseIndex].stDesc.u32WndNum;

    printf("case index %d, sub case %d-%d\n", g_u32CaseIndex, g_u32CurSubCaseIndex, g_u32LastSubCaseIndex);

    ST_VifToDisp(g_u32CaseIndex);

    g_u32CurSubCaseIndex = u32SubIndex - 1;//select subIndex

    if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].eDispoutTiming > 0)
    {
        ST_ChangeDisplayTiming(); //change timing
    }
    else
    {
        if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "exit", 4)))
        {
            ST_SubExit();
            return;
        }
        else if (0 == (strncmp(pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].szDesc, "zoom", 4)))
        {
        }
        else
        {
            if (pstCaseDesc[g_u32CaseIndex].stSubDesc[g_u32CurSubCaseIndex].u32WndNum > 0)
            {
                ST_SplitWindow(); //switch screen
            }
        }
    }

    ST_WaitSubCmd();
}

MI_S32 test_main(int argc, char **argv)
{
    MI_S32 a = 0, b = 1;
    MI_S32 sum = 0;

    while (1)
    {
        sum = a + b;
    }

    return 0;
}

MI_S32 main(int argc, char **argv)
{
    char szCmd[16];
    MI_U32 u32Index = 0;

    //return test_main(argc, argv);

    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    signal(SIGCHLD, SIG_IGN);

    ST_DealCase(argc, argv);
    while (!g_bExit)
    {
        ST_VifUsage();//case usage
        fgets((szCmd), (sizeof(szCmd) - 1), stdin);

        u32Index = atoi(szCmd);

        if (u32Index <= 0 || u32Index > ARRAY_SIZE(g_stVifCaseDesc))
        {
            continue;
        }
        g_u32CaseIndex = u32Index - 1;
        if (0 == (strncmp(g_stVifCaseDesc[g_u32CaseIndex].stDesc.szDesc, "exit", 4)))
        {
            return MI_SUCCESS;
        }
        ST_VifToDisp(g_u32CaseIndex);
        ST_WaitSubCmd();
    }

    return 0;
}
