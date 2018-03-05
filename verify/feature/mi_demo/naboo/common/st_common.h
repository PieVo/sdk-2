#ifndef _ST_COMMON_H
#define _ST_COMMON_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_sys.h"
#include "mi_hdmi_datatype.h"
#include "mi_vdec_datatype.h"
#include "mi_venc_datatype.h"

/*
 * MACROS FUNC
 */
#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("Test [%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("Test [%d]exec function pass\n", __LINE__);\
    }
#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }

#define STDBG_ENTER() \
    printf("\n"); \
    printf("[IN] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define STDBG_LEAVE() \
    printf("\n"); \
    printf("[OUT] [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define ST_RUN() \
    printf("\n"); \
    printf("[RUN] ok [%s:%s:%d] \n", __FILE__, __FUNCTION__, __LINE__); \
    printf("\n"); \

#define COLOR_NONE          "\033[0m"
#define COLOR_BLACK         "\033[0;30m"
#define COLOR_BLUE          "\033[0;34m"
#define COLOR_GREEN         "\033[0;32m"
#define COLOR_CYAN          "\033[0;36m"
#define COLOR_RED           "\033[0;31m"
#define COLOR_YELLOW        "\033[1;33m"
#define COLOR_WHITE         "\033[1;37m"

#define ST_NOP(fmt, args...)
#define ST_DBG(fmt, args...) ({do{printf(COLOR_GREEN"[DBG]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_WARN(fmt, args...) ({do{printf(COLOR_YELLOW"[WARN]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_INFO(fmt, args...) ({do{printf("[INFO]:%s[%d]: \n", __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})
#define ST_ERR(fmt, args...) ({do{printf(COLOR_RED"[ERR]:%s[%d]: "COLOR_NONE, __FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);})

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])

#define MAKE_YUYV_VALUE(y,u,v) ((y) << 24) | ((u) << 16) | ((y) << 8) | (v)
#define YUYV_BLACK MAKE_YUYV_VALUE(0,128,128)
#define YUYV_WHITE MAKE_YUYV_VALUE(255,128,128)
#define YUYV_RED MAKE_YUYV_VALUE(76,84,255)
#define YUYV_GREEN MAKE_YUYV_VALUE(149,43,21)
#define YUYV_BLUE MAKE_YUYV_VALUE(29,225,107)

/***************************************************************************************************
 * MACROS VAL
 **************************************************************************************************/
#define MAX_CHANNEL_NUM 16
#define MAX_FILENAME_LEN 128
#define ALIGN_N(x, align)           (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_BACK(x, a)            (((x) / (a)) * (a))

#define PREFIX_PATH                 "/mnt/"

#define ST_4K_H264_FILE             "4K.h264"
#define ST_4K_H265_FILE             "4K.h265"

#define ST_8MP_H264_25_FILE         "8MP.h264"
#define ST_8MP_H265_25_FILE         "8MP.h265"
#define ST_4MP_H264_25_FILE         "4MP.h264"
#define ST_4MP_H265_25_FILE         "4MP.h265"
#define ST_1080P_H264_25_FILE       "1080P25.h264"
#define ST_1080P_H265_25_FILE       "1080P25.h265"
#define ST_1080P_H264_30_FILE       "1080P30.h264"
#define ST_1080P_H265_30_FILE       "1080P30.h265"

#define ST_720P_H264_25_FILE        "720P25.h264"
#define ST_720P_H265_25_FILE        "720P25.h265"
#define ST_720P_H264_30_FILE        "720P30.h264"
#define ST_720P_H265_30_FILE        "720P30.h265"

#define ST_VGA_H264_25_FILE         "VGA.h264"

#define ST_D1_H264_25_FILE          "D125.h264"
#define ST_D1_H265_25_FILE          "D125.h265"
#define ST_D1_H264_30_FILE          "D130.h264"
#define ST_D1_H265_30_FILE          "D130.h265"

#define ST_AA_H264_25_FILE          "640_360.h264"

#define ST_MJPEG_1600_1200_FILE     "1600_1200_baseline.mjpeg"
#define ST_MJPEG_320_240_P_FILE     "320_240_progessive.mjpeg"
#define ST_MJPEG_1024_768_P_FILE    "1024_768_Progessive.mjpeg"
#define ST_MJPEG_1080P_P_FILE       "1080P_Progessive.mjpeg"



#define MAX_SUB_DESC_NUM            16
#define MAX_CHN_NUM                 36
#define NALU_PACKET_SIZE            512*1024
#define VIF_MAX_CHN_NUM 32 //16Main Vif + 16Sub Vif

#define DISP_PORT 0
#define VDISP_PORT 1
#define MAIN_VENC_PORT 2
#define SUB_VENC_PORT 3

/***************************************************************************************************
 * ENUM
 **************************************************************************************************/
typedef enum
{
    E_ST_SPLIT_MODE_ONE = 0,
    E_ST_SPLIT_MODE_TWO,
    E_ST_SPLIT_MODE_FOUR,
    E_ST_SPLIT_MODE_SIX,
    E_ST_SPLIT_MODE_EIGHT,
    E_ST_SPLIT_MODE_NINE,
    E_ST_SPLIT_MODE_NINE_EX,        /* 9画面扩展不规则分屏,实际显示6画面(1大+5小) */
    E_ST_SPLIT_MODE_SIXTEEN,
    E_ST_SPLIT_MODE_SIXTEEN_EX,
    E_ST_SPLIT_MODE_MAX,
} ST_SplitMode_e;

typedef enum
{
    E_ST_CHNTYPE_FILE = 0,
    E_ST_CHNTYPE_VIF,
    E_ST_CHNTYPE_VDEC_LIVE,
    E_ST_CHNTYPE_VDEC_PLAYBACK,
    E_ST_CHNTYPE_VIF_ZOOM,
    E_ST_CHNTYPE_VDEC_ZOOM,
    E_ST_CHNTYPE_MAX,
} ST_ChnType_e;

typedef enum
{
    E_ST_SWITCH_LIVE = 0, /* 实时切屏 */
    E_ST_SWITCH_PLAYBACK, /* 回放切屏 */
    E_ST_SWITCH_MAX,
} ST_SwitchMode_e;

typedef enum
{
    E_ST_CHANNEL_HIDE = 0,
    E_ST_CHANNEL_SHOW,
    E_ST_CHANNEL_MAX,
} ST_ChannelStatus_e;

typedef enum
{
    E_ST_FMT_ARGB1555 = 0,
    E_ST_FMT_ARGB8888,
    E_ST_FMT_YUV422,
    E_ST_FMT_YUV420,
    E_ST_FMT_YUV444,
    E_ST_FMT_MAX,
} ST_ColorFormat_e;

typedef enum
{
    E_ST_TIMING_720P_50 = 0,
    E_ST_TIMING_720P_60,
    E_ST_TIMING_1080P_50,
    E_ST_TIMING_1080P_60,
    E_ST_TIMING_1600x1200_60,
    E_ST_TIMING_1440x900_60,
    E_ST_TIMING_1280x1024_60,
    E_ST_TIMING_1024x768_60,
    E_ST_TIMING_3840x2160_30,
    E_ST_TIMING_3840x2160_60,
    E_ST_TIMING_MAX,
} ST_DispoutTiming_e;

typedef enum
{
    E_ST_VIF_CHN = 0,
    E_ST_VDEC_CHN,
    E_ST_VIF_VENC_CHN,
    E_ST_VIF_RGN_CHN,
    E_ST_VDEC_RGN_CHN,
    E_ST_CHN_TYPE_MAX,
} ST_VideoChnType_e;

/***************************************************************************************************
 * STRUCTURES
 **************************************************************************************************/
typedef struct ST_SwitchScreenInfo_s
{
    ST_SplitMode_e eSplitMode;
    ST_SwitchMode_e eSwitchMode;
    MI_U8 au8ShowChnIndex[MAX_CHANNEL_NUM];
} ST_SwitchScreenInfo_t;

typedef struct ST_Sys_Rect_s
{
    MI_S32 s32X;
    MI_S32 s32Y;
    MI_U16 u16PicW;
    MI_U16 u16PicH;
} ST_Rect_t;

typedef struct ST_Sys_TestDataInfo_s
{
    ST_ColorFormat_e eColorFmt;
    MI_U16 u16PicWidth;
    MI_U16 u16PicHeight;
    MI_U32 u32RunTimes;
    MI_U8 au8FileName[MAX_FILENAME_LEN];
    MI_U8 au8OutputFileName[MAX_FILENAME_LEN]; //port*4-->max
} ST_Sys_TestDataInfo_t; //Yuv test file info

typedef struct ST_Sys_SplitRect_s
{
    ST_SplitMode_e eSplitMode;
    MI_S32 s32ChnNum;
    MI_U16 u16SplitDivW;
    MI_U16 u16SplitDivH; //2*2 3*3 4*4
} ST_Sys_SplitRect_t;

typedef struct ST_VifChnConfig_s
{
    MI_U8 u8ViDev;
    MI_U8 u8ViChn;
    MI_U8 u8ViPort; //main or sub
} ST_VifChnConfig_t;

typedef struct ST_Sys_ChnInfo_s
{
    MI_S32 s32ChnId;
    ST_ChnType_e eChnType;
    MI_BOOL bIsShow; //1->show 0->hide
    MI_BOOL bVdecChnEnable; //vdec
    MI_S32 s32PushDataMs;
    MI_VDEC_CodecType_e eCodecType;
    ST_Rect_t stDispRect;
    MI_S32 s32VifFrameRate; //bind port frame rate
    MI_S32 s32VpeFrameRate;
    MI_S32 s32VdecFrameRate;
    MI_S32 s32DivpFrameRate;
    MI_S32 s32VdispFrameRate;
    MI_S32 s32DispFrameRate;
    MI_SYS_ChnPort_t stChnPort[E_MI_MODULE_ID_MAX]; /* Save Chn relation module */
    ST_VifChnConfig_t stVifChnCfg;
    ST_Sys_TestDataInfo_t stTestDataInfo; //stTestDataInfo.size ---> stDispRect
} ST_Sys_ChnInfo_t;

typedef struct ST_Sys_TestParam_s
{
    MI_HDMI_TimingType_e eTiming;
    MI_S32 s32TotalChnNum;
    ST_SwitchMode_e eSwitchMode;
    ST_SplitMode_e eSplitMode;
    MI_S32 s32DispWidth;
    MI_S32 s32DispHeight;
    ST_Sys_ChnInfo_t stChannelInfo[MAX_CHANNEL_NUM];
} ST_Sys_TestParam_t;

typedef struct ST_Sys_BindInfo_s
{
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;
    MI_U32 u32SrcFrmrate;
    MI_U32 u32DstFrmrate;
} ST_Sys_BindInfo_t;

typedef struct
{
    MI_U32 u32CaseIndex;            // case index
    MI_U32 u32Chn;                  // vdec chn
    MI_VDEC_CodecType_e eCodecType; // vdec caode type
    char szFilePath[64];            // h26x file path
    MI_U32 u32MaxWidth;             // max of width
    MI_U32 u32MaxHeight;            // max of height
} ST_VdecChnArgs_t;

typedef struct
{
    MI_U32 u32Chn;          // vif chn num
    MI_U16 u16CapWidth;        // max of width
    MI_U16 u16CapHeight;       // max of height
    MI_S32 s32FrmRate;         // frame rate
    MI_VENC_ModType_e eType;    // encode type
} ST_VifChnArgs_t;

typedef struct
{
    MI_U32 u32CaseIndex;        // case index
    MI_U32 u32Chn;              // vif chn num
    MI_U16 u16MainWidth;        // main venc width
    MI_U16 u16MainHeight;       // main venc height
    MI_VENC_ModType_e eType;    // encode type
    MI_U16 u16SubWidth;         // sub venc width
    MI_U16 u16SubHeight;        // sub venc height
    MI_S32 s32MainBitRate;      // main bitrate
    MI_S32 s32SubBitRate;       // sub bitrate
    MI_S32 s32MainFrmRate;      // main frame rate
    MI_S32 s32SubFrmRate;       // sub frame rate
} ST_VencChnArgs_t;

typedef struct
{
    MI_U32 u32CaseIndex;            // case index
    char szDesc[64];                // case describe
    MI_U32 u32WndNum;               // num of windows
    ST_DispoutTiming_e eDispoutTiming;
    MI_U32 u32VencNum;
    MI_U32 u32VideoChnNum;          // VIF+VDEC chn num
} ST_Desc_t;

typedef union
{
    ST_VdecChnArgs_t stVdecChnArg;
    ST_VifChnArgs_t stVifChnArg;
    ST_VencChnArgs_t stVencChnArg;
} ST_ChnArgs_u;

typedef struct
{
    ST_VideoChnType_e eVideoChnType;            // case index
    ST_ChnArgs_u uChnArg;
} ST_TestCaseArgs_t;

typedef struct
{
    ST_Desc_t stDesc;                           //
    ST_DispoutTiming_e eDispoutTiming;
    MI_S32 s32SplitMode;                        // default split mode
    MI_U32 u32SubCaseNum;                       // nun of sub case
    MI_U32 u32ShowWndNum;
    ST_Desc_t stSubDesc[MAX_SUB_DESC_NUM];      // sub case describe
    ST_TestCaseArgs_t stCaseArgs[MAX_CHN_NUM];
} ST_CaseDesc_t;

/***************************************************************************************************
 * FUNC
 **************************************************************************************************/
MI_S32 ST_Sys_Init(void);
MI_S32 ST_Sys_Exit(void);

MI_S32 ST_Sys_Bind(ST_Sys_BindInfo_t *pstBindInfo);
MI_S32 ST_Sys_UnBind(ST_Sys_BindInfo_t *pstBindInfo);

MI_S32 ST_Sys_SplitChnNum(ST_SplitMode_e eSplitMode);

MI_S32 ST_Sys_CreateDvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode, const ST_Sys_ChnInfo_t *pstChnInfo);
MI_S32 ST_Sys_CreateNvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode, const ST_Sys_ChnInfo_t *pstChnInfo);

MI_S32 ST_Sys_DestroyDvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode);
MI_S32 ST_Sys_DestroyNvrChn(MI_S32 s32Channel, MI_S32 s32SplitMode);

MI_U64 ST_Sys_GetPts(MI_U32 u32FrameRate);
MI_S32 ST_Get_VoRectBySplitMode(MI_S32 s32SplitMode, MI_S32 s32VoChnIndex, MI_U16 u16LayerW, MI_U16 u16LayerH, ST_Rect_t *pstRect);
MI_S32 ST_GetTimingInfo(MI_S32 s32ApTiming, MI_S32 *ps32HdmiTiming, MI_S32 *ps32DispTiming, MI_U16 *pu16DispW, MI_U16 *pu16DispH);

#endif //_ST_COMMON_H
