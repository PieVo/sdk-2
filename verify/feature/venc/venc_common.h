#ifndef __VENC_COMMON_H
#define __VENC_COMMON_H

#include <sys/time.h>

#define ASCII_COLOR_RED                          "\033[1;31m"
#define ASCII_COLOR_WHITE                        "\033[1;37m"
#define ASCII_COLOR_YELLOW                       "\033[1;33m"
#define ASCII_COLOR_BLUE                         "\033[1;36m"
#define ASCII_COLOR_GREEN                        "\033[1;32m"
#define ASCII_COLOR_END                          "\033[0m"


typedef struct Chn_s
{
    //MI_U32 u32DevId;
    MI_VENC_ModType_e eModType;
    MI_U32 u32ChnId;//ID to self so that output thread could have channel ID info
    pthread_t tid;
    MI_SYS_BUF_HANDLE hBuf;
} Chn_t;

typedef struct VENC_FPS_s
{
    struct timeval stTimeStart, stTimeEnd;
    struct timezone stTimeZone;
    MI_U32 u32DiffUs;
    MI_U32 u32TotalBits;
    MI_U32 u32FrameCnt;
    MI_BOOL bRestart;
} VENC_FPS_t;
#define TIMEVAL_US_DIFF(start, end)     ((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec))

typedef struct VENC_Rc_s
{
    MI_VENC_RcMode_e eRcMode;
    MI_U32 u32SrcFrmRate;
    MI_U32 u32Bitrate; //u32MaxBitRate for VBR, u32BitRate for CBR, u32AvgBitRate for ABR
    MI_U32 u32AvrMaxBitrate; //u32MaxBitRate for ABR
    MI_U32 u32Gop;
    MI_U32 u32FixQp;
    MI_U32 u32VbrMinQp;
    MI_U32 u32VbrMaxQp;
} VENC_Rc_t;

#define USE_MI_VENC_GET_DEVID (1)
#if USE_MI_VENC_GET_DEVID == 0
typedef enum
{
    E_VENC_DEV_MHE0,
    E_VENC_DEV_MHE1,
    E_VENC_DEV_MFE0,
    E_VENC_DEV_MFE1,
    E_VENC_DEV_JPGE,
    E_VENC_DEV_DUMMY,
    E_VENC_DEV_MAX
} VENC_Device_e;
#endif

extern const MI_U32 u32QueueLen;

#define VENC_MAX_CHN (16)
extern Chn_t _astChn[VENC_MAX_CHN];

#define DBG_INFO(fmt, args...)     ({do{printf(ASCII_COLOR_WHITE"[APP INFO]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})
#define DBG_ERR(fmt, args...)      ({do{printf(ASCII_COLOR_RED  "[APP ERR ]:%s[%d]: ", __FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);})

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        DBG_ERR("[%4d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else if(VENC_GLOB_TRACE)\
    {\
        printf("(%4d)exec %s passed.\n", __LINE__, __FUNCTION__);\
    }

void VencQuery(char * title, MI_VENC_CHN VeChn);

int setup_venc_fixed_rc(MI_U32 u32Gop, MI_U8 u8QpI, MI_U8 u8QpP);
int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_U32 u32Width,
                        MI_U32 u32Height, VENC_Rc_t *pstVencRc);
void print_es(char* title, void* buf, int iCh, int max_bytes);
void *venc_channel_func(void *arg);
void sleep_ms(int milliseconds);

#endif
