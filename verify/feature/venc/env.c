#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "env.h"

#define DECL_STR(name, strVal) {.szName = #name , .u32Len = MAX_CFG_STR_LEN,  .v.str = strVal}
#define DECL_INT(name, s32Val) {.szName = #name , .u32Len = 0,                .v.s32 = s32Val}
typedef struct VENC_Config_s
{
    char* szName;
    //MI_BOOL bIsStr;
    MI_U32 u32Len; //0 for integer, > 0 for string
    union {
        MI_S32 s32;
        char   str[MAX_CFG_STR_LEN];
    }v;
} VENC_Config_t;

int VENC_GLOB_TRACE;
int VENC_GLOB_TRACE_QUERY;

//The indent here is intended to represent that lower level CFG is only active while
//upper level of CFG is active. Note that this might not always aligned with the code because
//the level of controlling is coded manually.
VENC_Config_t CFG[] =
{
    DECL_STR(VENC_GLOB_RESULT, "result"),//Used to interact with test script.
    DECL_STR(VENC_GLOB_OUT_PATH, "/tmp/"),
    DECL_STR(VENC_GLOB_IN_FILE,  ""), //assume always NV12
    DECL_INT(VENC_GLOB_IN_FILE_CACHE_MB,  30),// If input file is <= this MiB, cached it into memory
    DECL_INT(VENC_GLOB_IN_FPS,  30),
    DECL_INT(VENC_GLOB_IN_W, BUILD_YUV_W),
    DECL_INT(VENC_GLOB_IN_H, BUILD_YUV_H),
    DECL_INT(VENC_GLOB_AUTO_STOP, 0),//[0-1], Set 1 to auto stop with EOS while input YUV ends.
    DECL_INT(VENC_GLOB_MAX_FRAMES, 5),
    //[0-3] output debug message level: 0:none, 1:ch,
    //2:ch+bytes with first few bytes with I and P detection, 3:first 32 bytes.
    DECL_INT(VENC_GLOB_OUT_MSG_LEVEL, 0),
    DECL_INT(VENC_GLOB_IN_MSG_LEVEL, 0),
    DECL_INT(VENC_GLOB_TRACE, 0),//Set 1 to trace ExecFunc macro even if it passed.
    DECL_INT(VENC_GLOB_TRACE_QUERY, 0),//Set 1 to trace VencQuery to monitor queried state
    DECL_INT(VENC_CH00_QP, DEF_QP),
    DECL_INT(VENC_GLOB_DUMP_ES, 20),//N frames to be recorded.
    // CODEC setting, originally from avc.cfg
    DECL_INT(VENC_GLOB_APPLY_CFG,  0),
        DECL_INT(DEBLOCK_FILTER_CONTROL, 0), ///< --- controls the following deblocking parameters
            DECL_INT(disable_deblocking_filter_idc, 1), //[0,2]
            DECL_INT(slice_alpha_c0_offset_div2, -6), //[-6,6] H264 only
            DECL_INT(slice_beta_offset_div2, -6), //[-6,6]
            DECL_INT(slice_tc_offset_div2, 0), //H265 [-6, 6] only
        DECL_INT(nRows, 0),//0 or 1????????//h265 [0,[1,h/32]], h264[0,[1,h/16]], 0:off
        DECL_INT(VENC_INTRA, 0), ///< --- controls the following parameters
            //DECL_INT(bIntra16x16PredEn, 1), //bIntraNxNPredEn must be 1 for 264,
            DECL_INT(constrained_intra_pred_flag, 0),//[0,1] H264 only
            //DECL_INT(bIpcmEn, 0), //bIpcmEn must be 0
            DECL_INT(u32Intra32x32Penalty, 0), //             H265[0-65535]
            DECL_INT(u32Intra16x16Penalty, 0), //H264[0-255], H265[0-65535]
            DECL_INT(u32Intra8x8Penalty, 0),   //             H265[0-65535]
            DECL_INT(u32Intra4x4Penalty, 0),   //H264[0,255], H265[0-65535], case: 0 2555
            DECL_INT(bIntraPlanarPenalty, 0),  //H264[0,255],
        DECL_INT(VENC_INTER, 0), ///< --- controls the following parameters
            DECL_INT(nDmv_X, 32),//[1,32],    H265:(X,Y)=(96,48), (32,16)
            DECL_INT(nDmv_Y, 16),//8 or 16
            DECL_INT(bInter8x8PredEn, 1),
            DECL_INT(bInter8x16PredEn, 1),
            DECL_INT(bInter16x8PredEn, 1),
            DECL_INT(bInter16x16PredEn, 1),  //bExtedgeEn must be 1 MI_CHECK, nBits must be 1
        DECL_INT(qpoffset, 0), //avc: [-12,12]
        DECL_INT(Qfactor, -1), //-1: don't set q factor [-1, 1-100]
        DECL_INT(VENC_ROI0_EN, 0),
            DECL_INT(VENC_ROI0_X, 0),
            DECL_INT(VENC_ROI0_Y, 0),
            DECL_INT(VENC_ROI0_W, 0),
            DECL_INT(VENC_ROI0_H, 0),
            DECL_INT(VENC_ROI0_Qp, 0),
        DECL_INT(VENC_ROI1_EN, 0),
            DECL_INT(VENC_ROI1_X, 0),
            DECL_INT(VENC_ROI1_Y, 0),
            DECL_INT(VENC_ROI1_W, 0),
            DECL_INT(VENC_ROI1_H, 0),
            DECL_INT(VENC_ROI1_Qp, 0),
        DECL_INT(VENC_GLOB_VUI, 0), //0: disabled, 1: test case 1 on H264/H265.
    DECL_INT(Cabac, 1),
    DECL_INT(VENC_RC, 0), ///< --- controls the following parameters
        DECL_STR(RcType, "Cbr"), ///< Case sensitive "Cbr", "Vbr", "FixQp", refer env'VENC_CH00_QP' for FixQP
        DECL_INT(Bitrate, 1000000),       //u32MaxBitRate@Vbr, u32BitRate@cbr, u32AvgBitRate@Abr
        DECL_INT(AbrMaxBitrate, 1000000), //                                   u32MaxBitRate@Abr
        DECL_INT(VbrMinQp, 20), //min Qp@Vbr
        DECL_INT(VbrMaxQp, 40), //max Qp@Vbr
    DECL_INT(VENC_GOP, 7), //GOP setting, note that this is not controlled by $VENC_RC
    DECL_INT(VENC_Crop, 0),
        DECL_INT(VENC_Crop_X, 0),
        DECL_INT(VENC_Crop_Y, 0),
        DECL_INT(VENC_Crop_W, 0),
        DECL_INT(VENC_Crop_H, 0),
    DECL_INT(Idr, 0),  //0:Not use IDR, (>1): every (Idr) would request one IDR.
    DECL_INT(EnableIdr, -1),  //-1: don't control EnableIdr. 1:, EnableIdr(1) at frame $EnableIdrFrame. 0: EnableIdr(0) at frame $EnableIdrFrame
        DECL_INT(EnableIdrFrame, 0), //Set EnableIdr at this frame. (start from 0)
    DECL_STR(VENC_GLOB_USER_DATA, ""), //Insert one time frame of user data.
};

void dump_cfg(void)
{
    unsigned int i;
    for(i = 0; i < sizeof(CFG) / sizeof(CFG[0]); ++i)
    {
        printf("%32s=", CFG[i].szName);
        if(CFG[i].u32Len > 0)
        {
            printf("'%s'\n", CFG[i].v.str);
        }
        else
        {
            printf("%d\n", CFG[i].v.s32);
        }
    }
}

void get_cfg_from_env(void)
{
    unsigned int i;
    char *szEnv;
    size_t size;
    MI_BOOL bFound = FALSE;

    for(i = 0; i < sizeof(CFG) / sizeof(CFG[0]); ++i)
    {
        szEnv = getenv(CFG[i].szName);
        if (szEnv == NULL)
            continue;

        if(bFound == FALSE)
        {
            bFound = TRUE;
            printf("--------------------------------------------------------------------------------\n");
        }

        if(CFG[i].u32Len > 0)
        {
            size = strlen(szEnv);
            strncpy(CFG[i].v.str, szEnv, CFG[i].u32Len - 1);
            if (size == CFG[i].u32Len)
            {
                printf("Value in var'%s' is too long!\n", CFG[i].szName);
                CFG[i].v.str[CFG[i].u32Len - 1] = '\0';
            }
            printf("set %s='%s'\n", CFG[i].szName, CFG[i].v.str);
        }
        else
        {
            CFG[i].v.s32 = atoi(szEnv);
            printf("set %s=%d\n", CFG[i].szName, CFG[i].v.s32);
        }
    }
    if (bFound)
    {
        printf("--------------------------------------------------------------------------------\n");
    }
}

int find_cfg_idx(char *szCfg)
{
    unsigned int i;
    for(i = 0; i < sizeof(CFG) / sizeof(CFG[0]); ++i)
    {
        if(strcmp(szCfg, CFG[i].szName) == 0)
        {
            return i;
        }
    }
    printf("Unknown CFG:'%s'\n", szCfg);
    return -1;
}

MI_S32 get_cfg_int(char *szCfg, MI_BOOL *pbError)
{
    int idx;
    idx = find_cfg_idx(szCfg);
    if(idx == -1)
    {
        *pbError = TRUE;
        return 0;
    }
    *pbError = FALSE;
    return CFG[idx].v.s32;
}

char* get_cfg_str(char *szCfg, MI_BOOL *pbError)
{
    int idx;
    idx = find_cfg_idx(szCfg);
    if(idx == -1)
    {
        *pbError = TRUE;
        return NULL;
    }
    *pbError = FALSE;
    return CFG[idx].v.str;
}

MI_BOOL set_cfg_str(char *szCfg, char *szValue)
{
    int idx;
    idx = find_cfg_idx(szCfg);
    if(idx == -1)
    {
        return TRUE;
    }
    strncpy(CFG[idx].v.str, szValue, CFG[idx].u32Len);
    CFG[idx].v.str[CFG[idx].u32Len - 1] = '\0';
    return CFG[idx].v.str;
    return FALSE;
}

//#include <stdio.h>
void set_result_int(char *szCfg, int value)
{
    MI_BOOL bErr;
    FILE *fp;
    char szLine[64];
    char *szResultFn;
    int nLine;

    szResultFn = get_cfg_str("VENC_GLOB_RESULT", &bErr);
    fp = fopen(szResultFn, "a");
    if(fp)
    {
        nLine = snprintf(szLine, sizeof(szLine) - 1, "%s=%d;\n", szCfg, value);
        fwrite(szLine, nLine, 1, fp);
        fclose(fp);
    }
    else
    {
        printf("[ERR] unable to open output file\n");
    }
}
