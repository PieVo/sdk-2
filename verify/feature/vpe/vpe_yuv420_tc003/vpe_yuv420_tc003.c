#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../mi_vpe_test.h"
//#define TEST_OUT_FILE(a, b, c, d) "/dev/block/mtdblock2"

#define MAX_TEST_CHANNEL (1)
static test_vpe_Config stTest003[] =
#if 0
{
    #if 0
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 1920x1080),
        .stSrcWin   = {0, 0, 1920, 1080},
        .stCropWin  = {0, 0, 1920, 1080},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 480x288),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 480, 288},
            },
        },
    },

#endif
#if 0
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 1280x720),
        .stSrcWin   = {0, 0, 1280, 720},
        .stCropWin  = {0, 0, 1280, 720},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 480x288),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 480, 288},
            },
        },
    },
    #endif


    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 960x540),
        .stSrcWin   = {0, 0, 960, 540},
        .stCropWin  = {0, 0, 960, 540},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 480x288),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 480, 288},
            },
        },
    },


    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 480x288),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 480, 288},
            },
        },
    },

};
#else
{
    #if 1
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 1280x720),
        .stSrcWin   = {0, 0, 1280, 720},
        .stCropWin  = {0, 0, 1280, 720},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 720x576),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 720, 576},
            },
        },
    },
#endif
#if 1
{
    .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 960x540),
    .stSrcWin   = {0, 0, 960, 540},
    .stCropWin  = {0, 0, 960, 540},
    .stOutPort  = {
        {
            .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 720x576),
            .bEnable    = TRUE,
            .stPortWin  = {0, 0, 720, 576},
        },
    },
},
#endif
    {
        .inputFile  = TEST_VPE_CHNN_FILE420(003, 0, 640x360),
        .stSrcWin   = {0, 0, 640, 360},
        .stCropWin  = {0, 0, 640, 360},
        .stOutPort  = {
            {
                .outputFile = TEST_VPE_PORT_OUT_FILE(003, 0, 0, 720x576),
                .bEnable    = TRUE,
                .stPortWin  = {0, 0, 720, 576},
            },
        },
    },

};
#endif

//int test_vpe_TestCase003_main(int argc, const char *argv[])
int main(int argc, const char *argv[])
{
    MI_S32 s32Ret = E_MI_ERR_FAILED;
    MI_U32 i = 0;
    MI_SYS_ChnPort_t stVpeChnInputPort0;
    MI_SYS_ChnPort_t stVpeChnOutputPort0;
    MI_S32 count = 0;
    time_t stTime = 0;
    char src_file[256];
    char dest_file[256];
    const char *pbaseDir = NULL;
    int src_fd, dest_fd;
    MI_SYS_FrameData_t framedata;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hHandle;
    int frame_size = 0;
    int width  = 0;
    int height = 0;
    int y_size = 0;
    int uv_size = 0;
    memset(&framedata, 0, sizeof(framedata));
    memset(&stTime, 0, sizeof(stTime));

    if (argc < 3)
    {
        printf("%s <test_dir> <count>.\n", argv[0]);
        printf("%s.\n", VPE_TEST_003_DESC);
        for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
        {
            printf("Channel: %d.\n", 0);
            printf("InputFile: %s.\n", stTest003[i].inputFile);
            printf("OutputFile: %s.\n", stTest003[i].stOutPort[0].outputFile);
        }
        return 0;
    }

    pbaseDir = argv[1];
    count = atoi(argv[2]);
    printf("%s %s %d.\n", argv[0], pbaseDir, count);

    for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
    {
        sprintf(src_file, "%s/%s", pbaseDir, stTest003[i].inputFile);
        ExecFunc(test_vpe_OpenSourceFile(src_file, &stTest003[i].src_fd), TRUE);
        if (i == 0)
        {
            sprintf(dest_file, "%s", stTest003[i].stOutPort[0].outputFile);
            ExecFunc(test_vpe_OpenDestFile(dest_file, &stTest003[i].stOutPort[0].dest_fd), TRUE);
        }
        else
        {
            stTest003[i].stOutPort[0].dest_fd = stTest003[0].stOutPort[0].dest_fd;
        }
        stTest003[i].count = 0;
        stTest003[i].src_offset = 0;
        stTest003[i].src_count  = 0;
        stTest003[i].stOutPort[0].dest_offset = 0;
        stTest003[i].product = 0;
    }

    // init MI_SYS
    ExecFunc(test_vpe_SysEnvInit(), TRUE);
#if 0
    // Create VPE channel
    for (i = 0; i < MAX_TEST_CHANNEL; i++)
    {
        test_vpe_CreatChannel(i, 0, &stTest003[i].stCropWin, &stTest003[i].stOutPort[0].stPortWin);
        // set vpe port buffer depth
        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = i;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 5, 10);
    }
#else
        test_vpe_CreatChannel(0, 0, &stTest003[0].stCropWin, &stTest003[0].stOutPort[0].stPortWin,E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420,E_MI_SYS_PIXEL_FRAME_YUV422_YUYV);

        // set vpe port buffer depth
        stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
        stVpeChnOutputPort0.u32DevId = 0;
        stVpeChnOutputPort0.u32ChnId = 0;
        stVpeChnOutputPort0.u32PortId = 0;
        MI_SYS_SetChnOutputPortDepth(&stVpeChnOutputPort0, 1, 3);
#endif
    // Start capture data
    while (count > 0)
    {
        MI_SYS_BufConf_t stBufConf;
        MI_S32 s32Ret;

        for (i = 0; ((i < sizeof(stTest003)/sizeof(stTest003[0])) && (count > 0)); i++)
        {
            src_fd = stTest003[i].src_fd;
            dest_fd= stTest003[i].stOutPort[0].dest_fd;

            memset(&stVpeChnInputPort0, 0, sizeof(stVpeChnInputPort0));
            stVpeChnInputPort0.eModId = E_MI_MODULE_ID_VPE;
            stVpeChnInputPort0.u32DevId = 0;
            stVpeChnInputPort0.u32ChnId = 0;
            stVpeChnInputPort0.u32PortId = 0;

            memset(&stBufConf ,  0 , sizeof(stBufConf));
            MI_VPE_TEST_DBG("%s()@line%d: Start get chnn %d input buffer.\n", __func__, __LINE__, i);
            stBufConf.eBufType = E_MI_SYS_BUFDATA_FRAME;
            stBufConf.u64TargetPts = time(&stTime);
            stBufConf.stFrameCfg.eFormat = E_MI_SYS_PIXEL_FRAME_YUV_SEMIPLANAR_420;
            stBufConf.stFrameCfg.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE;
            stBufConf.stFrameCfg.u16Width = stTest003[i].stCropWin.u16Width;
            stBufConf.stFrameCfg.u16Height = stTest003[i].stCropWin.u16Height;
            if(MI_SUCCESS  == MI_SYS_ChnInputPortGetBuf(&stVpeChnInputPort0,&stBufConf,&stBufInfo,&hHandle, 0))
            {
                // Start user put int buffer
                width   = stBufInfo.stFrameData.u16Width;
                height  = stBufInfo.stFrameData.u16Height;
                y_size  = width*height;
                uv_size  = width*height/2;


                test_vpe_ShowFrameInfo("Input : ", &stBufInfo.stFrameData);
                if (1 == test_vpe_GetOneFrameYUV420(stTest003[i].src_fd, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.pVirAddr[1], y_size, uv_size))
                {
                    stTest003[i].src_offset += y_size + uv_size;
                    MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[0], y_size);
                    MI_SYS_FlushCache(stBufInfo.stFrameData.pVirAddr[1], uv_size);

                    MI_VPE_TEST_INFO("%s()@line%d: Start put input buffer.\n", __func__, __LINE__);
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, FALSE);
                }
                else
                {
                    s32Ret = MI_SYS_ChnInputPortPutBuf(hHandle,&stBufInfo, TRUE);
                    stTest003[i].src_offset = 0;
                    stTest003[i].src_count = 0;
                    test_vpe_FdRewind(src_fd);
                }
            }

            memset(&stVpeChnOutputPort0, 0, sizeof(stVpeChnOutputPort0));
            stVpeChnOutputPort0.eModId = E_MI_MODULE_ID_VPE;
            stVpeChnOutputPort0.u32DevId = 0;
            stVpeChnOutputPort0.u32ChnId = 0;
            stVpeChnOutputPort0.u32PortId = 0;
            MI_VPE_TEST_DBG("%s()@line%d: Start Get Chnn: %d Port: %d output buffer.\n", __func__, __LINE__, i, stVpeChnOutputPort0.u32PortId);

            if (MI_SUCCESS == MI_SYS_ChnOutputPortGetBuf(&stVpeChnOutputPort0 , &stBufInfo,&hHandle))
            {
                MI_VPE_TEST_DBG("get output buf ok\n");
                MI_VPE_TEST_DBG("@@@--->buf type %d\n", stBufInfo.eBufType);
                // Add user write buffer to file
                width  = stBufInfo.stFrameData.u16Width;
                height = stBufInfo.stFrameData.u16Height;
                frame_size = width*height*YUV422_PIXEL_PER_BYTE;
                // put frame

                test_vpe_ShowFrameInfo("Output: ", &stBufInfo.stFrameData);
                test_vpe_PutOneFrame(dest_fd, stTest003[0].stOutPort[0].dest_offset, stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u32Stride[0], width*YUV422_PIXEL_PER_BYTE, height);
                stTest003[0].stOutPort[0].dest_offset += frame_size;

                MI_VPE_TEST_DBG("%s()@line%d: Start put chnn: %d output buffer.\n", __func__, __LINE__, i);
                MI_SYS_ChnOutputPortPutBuf(hHandle);
                MI_VPE_TEST_INFO("TestOut %d Pass.\n", count);
                count--;

            }
        }

        MI_VPE_TEST_INFO("Calc %d Pass.\n", count);
    }

    sync();
    for (i = 0; i < sizeof(stTest003)/sizeof(stTest003[0]); i++)
    {
        src_fd = stTest003[i].src_fd;
        test_vpe_CloseFd(src_fd);
        if (i == 0)
        {
            dest_fd= stTest003[i].stOutPort[0].dest_fd;
            test_vpe_CloseFd(dest_fd);
            test_vpe_DestroyChannel(i, 0);
        }
    }
    ExecFunc(test_vpe_SysEnvDeinit(), TRUE);

    MI_VPE_TEST_DBG("%s()@line %d pass exit\n", __func__, __LINE__);
    return 0;
}
