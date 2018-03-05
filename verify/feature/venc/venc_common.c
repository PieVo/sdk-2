/** @file venc_common.c
 *
 *  The function puts here would be shared with feature/vif.
 *  The code needs to be synchronized.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "mi_venc.h"
#include "mi_sys.h"
#include "env.h"
#include "venc_common.h"


#if 0
#define SIMPLE_OUTPUT (1) //Set 1 for shorter message and print out I or P frame

//Common state for many different CODEC
struct venc_state
{
    struct rate_ctl
    {
        MI_U32 u32Gop;
        MI_U8 u8QpI;
        MI_U8 u8QpP;
    } rc;
} venc_state =
{
    .rc = { .u32Gop = 7, .u8QpI = 20, .u8QpP = 20}
};

//Set up common video encoder rate control parameters
int setup_venc_fixed_rc(MI_U32 u32Gop, MI_U8 u8QpI, MI_U8 u8QpP)
{
    venc_state.rc.u32Gop = u32Gop;
    venc_state.rc.u8QpI = u8QpI;
    venc_state.rc.u8QpP = u8QpP;
    return 0;
}
#endif

#if USE_MI_VENC_GET_DEVID == 0
MI_S8 map_core_id(int iCh, MI_VENC_ModType_e eModType)
{
    switch (eModType) {
    case E_MI_VENC_MODTYPE_H265E:
    case E_MI_VENC_MODTYPE_H264E:
        return iCh & 1;
        break;
    case E_MI_VENC_MODTYPE_JPEGE:
        return 0;
        break;
    default:
        break;
    }
    DBG_ERR("Unsupported core map: ch:%d, mod:%d\n", iCh, (int) eModType);
    return -1;
}


//s8Core: <0: undefined,   0: core 0,   1: core 1,  >1: don't care
MI_S32 get_venc_dev_id(MI_VENC_ModType_e eModType, MI_S8 s8Core)
{
    MI_S32 s32DevId = -1;

    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H265E:
            if(s8Core == 0)
                s32DevId = 0;//E_MI_VENC_DEV_MHE0

            if(s8Core == 1)
                s32DevId = 1;//E_MI_VENC_DEV_MHE1
            break;
        case E_MI_VENC_MODTYPE_H264E:
            if(s8Core == 0)
                s32DevId = 2;//E_MI_VENC_DEV_MFE0
            if(s8Core == 1)
                s32DevId = 3;//E_MI_VENC_DEV_MFE1
            break;
        case E_MI_VENC_MODTYPE_JPEGE:
            if(s8Core == 0)
                s32DevId = 4;//E_MI_VENC_DEV_MHE0
            break;
        default:
            break;
    }

    return s32DevId;
}
#endif


int create_venc_channel(MI_VENC_ModType_e eModType, MI_VENC_CHN VencChannel, MI_U32 u32Width,
                        MI_U32 u32Height, VENC_Rc_t *pstVencRc)
{
    MI_VENC_ChnAttr_t stChannelVencAttr;
    //MI_SYS_ChnPort_t stSysChnOutPort;
    const MI_U32 u32QueueLen = 40;
    //MI_S32 s32Ret;// = E_MI_ERR_FAILED;
    MI_S32 s32DevId;
    //MI_S8 s8Core;

    if(NULL == pstVencRc)
    {
        return -1;
    }
    printf("CH%2d %dx%d mod:%d, rc:%d\n", VencChannel, u32Width, u32Height, eModType, pstVencRc->eRcMode);
    memset(&stChannelVencAttr, 0, sizeof(stChannelVencAttr));

    switch(eModType)
    {
        case E_MI_VENC_MODTYPE_H264E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H264E;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH264e.u32MaxPicHeight = u32Height;
            break;

        case E_MI_VENC_MODTYPE_H265E:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_H265E;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrH265e.u32MaxPicHeight = u32Height;
            break;

        case E_MI_VENC_MODTYPE_JPEGE:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_JPEGE;
            stChannelVencAttr.stVeAttr.stAttrMjpeg.u32PicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrMjpeg.u32PicHeight = u32Height;
            stChannelVencAttr.stVeAttr.stAttrMjpeg.u32MaxPicWidth = u32Width;
            stChannelVencAttr.stVeAttr.stAttrMjpeg.u32MaxPicHeight = u32Height;
            break;
        case E_MI_VENC_MODTYPE_MAX:
            stChannelVencAttr.stVeAttr.eType = E_MI_VENC_MODTYPE_MAX;
            break;
        default:
            break;
    }

    stChannelVencAttr.stRcAttr.eRcMode = pstVencRc->eRcMode;
    switch(pstVencRc->eRcMode)
    {
        case E_MI_VENC_RC_MODE_H264FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH264FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H265FIXQP:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 FixQP is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32IQp = pstVencRc->u32FixQp;
            stChannelVencAttr.stRcAttr.stAttrH265FixQp.u32PQp = pstVencRc->u32FixQp;
            break;
        case E_MI_VENC_RC_MODE_H264CBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH264Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265CBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 CBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32BitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32FluctuateLevel = 0;
            stChannelVencAttr.stRcAttr.stAttrH265Cbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H264VBR:
            if(eModType != E_MI_VENC_MODTYPE_H264E)
            {
                DBG_ERR("H264 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH264Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_H265VBR:
            if(eModType != E_MI_VENC_MODTYPE_H265E)
            {
                DBG_ERR("H265 VBR is set but module is %d\n", eModType);
                return -1;
            }
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32Gop = pstVencRc->u32Gop;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32SrcFrmRate = pstVencRc->u32SrcFrmRate;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxBitRate = pstVencRc->u32Bitrate;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MinQp = pstVencRc->u32VbrMinQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32MaxQp = pstVencRc->u32VbrMaxQp;
            stChannelVencAttr.stRcAttr.stAttrH265Vbr.u32StatTime = 0;
            break;
        case E_MI_VENC_RC_MODE_MJPEGFIXQP:
            if(eModType != E_MI_VENC_MODTYPE_JPEGE)
            {
                DBG_ERR("JPEG RC is set but module is %d\n", eModType);
                return -1;
            }
            break;
        default:
            if(eModType == E_MI_VENC_MODTYPE_MAX)
                break;
            DBG_ERR("Unknown RC type:%d\n", pstVencRc->eRcMode);
            return -3;
            break;
    }

    ExecFunc(MI_VENC_CreateChn(VencChannel, &stChannelVencAttr), MI_SUCCESS);
    if(0)
    {
        MI_U32 u32Devid;
        ExecFunc(MI_VENC_GetChnDevid(VencChannel, &u32Devid), MI_SUCCESS);
        DBG_INFO("CH%2d, DevId:%d\n", VencChannel, u32Devid);
    }
    //Set port
#if USE_MI_VENC_GET_DEVID
    ExecFunc(MI_VENC_GetChnDevid(VencChannel, (MI_U32*)&s32DevId), MI_SUCCESS);
#else
    s8Core = map_core_id(VencChannel, eModType);
    s32DevId = get_venc_dev_id(eModType, s8Core);
#endif

    if(s32DevId >= 0)
    {
#if USE_MI_VENC_GET_DEVID == 0
        _astChn[VencChannel].u32DevId = s32DevId;
#endif
        //stSysChnOutPort.u32DevId = s32DevId;
    }
    else
    {
        DBG_ERR("Can not find device ID %X\n", s32DevId);
        return -2;
    }

#if 1
    ExecFunc(MI_VENC_SetMaxStreamCnt(VencChannel, u32QueueLen), MI_SUCCESS);
#else
    stSysChnOutPort.eModId = E_MI_MODULE_ID_VENC;
    stSysChnOutPort.u32ChnId = VencChannel;
    stSysChnOutPort.u32PortId = 0;//port is always 0 in VENC
    //This was set to (5, 10) and might be too big for kernel
    s32Ret = MI_SYS_SetChnOutputPortDepth(&stSysChnOutPort, 5, u32QueueLen);
    if(s32Ret != 0)
    {
        DBG_ERR("Unable to set output port depth %X\n", s32Ret);
        return -3;
    }
#endif

    return 0;
}


int destroy_venc_channel(MI_VENC_CHN VencChannel)
{
    /*****************************/
    /*  call sys bind interface */
    /*****************************/
    ExecFunc(MI_VENC_StopRecvPic(VencChannel), MI_SUCCESS);
    //printf("sleeping...");
    //usleep(2 * 1000 * 1000);//wait for stop channel
    //printf("sleep done\n");

    /*****************************/
    /*  call sys unbind interface */
    /*****************************/
    ExecFunc(MI_VENC_DestroyChn(VencChannel), MI_SUCCESS);

    return 0;
}

void print_es(char* title, void* buf, int iCh, int max_bytes)
{
    char *data = (char *) buf;
    int i;
    int msg_level;
    MI_BOOL bErr;

    msg_level = get_cfg_int("VENC_GLOB_OUT_MSG_LEVEL", &bErr);

    if(max_bytes > 0 && max_bytes < 10)//short message, assume it's dummy
    {//print it as a string.
        if(msg_level > 0)
            DBG_INFO("^^^^ CH%02d:%s\n", iCh, (char*)buf);
        return;
    }

    if(msg_level == 0)
    {
        return;
    }
    if(msg_level == 1)
    {
        printf("    CH%02d\n", iCh);
    }
    else if(msg_level == 2)
    {
        printf("^^^^ CH%02d got %5d Bytes:%02X %02X %02X %02X %02X %s\n", iCh, max_bytes,
               data[0], data[1], data[2], data[3], data[4], (data[4] == 0x67 || data[4] == 0x40) ? " I" : "");
    }
    else
    {
        printf("%s\nCH%02d Offset(h) \n"
                "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
                "-----------------------------------------------", title, iCh);
        for (i = 0; i < 32; i++)
        {
            if (i % 16 == 0)
            {
                printf("\n");
            }
            printf("%02X ", data[i]);
        }
        printf("\n");
    }
}

void VencQuery(char * title, MI_VENC_CHN VeChn)
{
    MI_VENC_ChnStat_t stStat;
    MI_S32 s32Ret;
    if(VENC_GLOB_TRACE_QUERY == 0)
        return;

    s32Ret = MI_VENC_Query(VeChn, &stStat);
    if(s32Ret == MI_SUCCESS)
    {
        printf("%16s cp%d lp%d ep%3d rp%d sb%d sf%d\n", title, stStat.u32CurPacks, stStat.u32LeftPics, stStat.u32LeftEncPics,
                stStat.u32LeftRecvPics, stStat.u32LeftStreamBytes, stStat.u32LeftStreamFrames);
    }
    else
    {
        DBG_ERR(":%X\n", s32Ret);
    }
}

#define USE_GET_STREAM (2)
#define USE_FD (1)
#if USE_FD
#include <poll.h>
#endif
#define OUT_PATH get_cfg_str("VENC_GLOB_OUT_PATH", &bErr)
void *venc_channel_func(void *arg)
{
    Chn_t* pstChn = (Chn_t*)arg;
    MI_VENC_CHN VencChannel;
    MI_U32 VencDevId;
    MI_U32 VencPortId;
    MI_SYS_ChnPort_t stVencChn0OutputPort0;
    unsigned int VENC_DUMP_FRAMES;
    int fd = -1;
    int fdJpg = -1;

#if USE_GET_STREAM < 2
    MI_SYS_BufInfo_t stBufInfo;
#endif
#if !USE_GET_STREAM
    MI_SYS_BUF_HANDLE hHandle = MI_HANDLE_NULL;
#endif
    MI_S32 s32Ret;
    MI_BOOL bErr;
    MI_BOOL bSaveEach = FALSE;
    char szOutputPath[128];
    MI_U32 u32Seq = 0;
    VENC_FPS_t stFps = { .bRestart = TRUE, .u32TotalBits = 0, .u32DiffUs = 0 };
    MI_VENC_ChnAttr_t stAttr;
    MI_U32 u32Retry = 0;
#if USE_FD
    MI_S32 s32Fd = -1;
#endif

    if (arg == NULL)
    {
        printf("Null input\r\n");
        return NULL;
    }

    stFps.bRestart = TRUE;

    VENC_DUMP_FRAMES = get_cfg_int("VENC_GLOB_DUMP_ES", &bErr);
    VencChannel = pstChn->u32ChnId;
    s32Ret = MI_VENC_GetChnDevid(VencChannel, &VencDevId);
    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("%Xs32Ret\n", s32Ret);
        return NULL;
    }
    VencPortId = 0;
    DBG_INFO("Start of Thread %d Getting.\n", VencChannel);

    memset(&stVencChn0OutputPort0, 0x0, sizeof(MI_SYS_ChnPort_t));
    stVencChn0OutputPort0.eModId = E_MI_MODULE_ID_VENC;
    stVencChn0OutputPort0.u32DevId = VencDevId;
    stVencChn0OutputPort0.u32ChnId = VencChannel;
    stVencChn0OutputPort0.u32PortId = VencPortId;

    s32Ret = MI_VENC_GetChnAttr(VencChannel, &stAttr);
    if(s32Ret != MI_SUCCESS)
    {
        DBG_ERR("%Xs32Ret\n", s32Ret);
        return NULL;
    }

    if(stAttr.stVeAttr.eType == E_MI_VENC_MODTYPE_JPEGE)
    {
        bSaveEach = TRUE;
    }
    if(VENC_DUMP_FRAMES)
    {
        snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%s/enc_d%dc%02d.es",
            OUT_PATH,
            VencDevId, VencChannel);
        fd = open(szOutputPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        printf("open %d %s\n", fd, szOutputPath);
        if(fd < 0)
        {
            printf("unable to open es\r\n");
        }
    }
#if USE_FD
    s32Fd = MI_VENC_GetFd(VencChannel);
    if(s32Fd <= 0)
    {
        DBG_ERR("Unable to get FD:%d for ch:%2d\n", s32Fd, VencChannel);
        return NULL;
    }
    else
    {
        printf("CH%2d FD%d\n", VencChannel, s32Fd);
    }
#endif

    while(1)
    {
        MI_BOOL bEndOfStream = FALSE;
#if USE_GET_STREAM
        MI_VENC_Stream_t stStream;
        MI_VENC_Pack_t stPack0;
#if USE_FD
        {
            struct pollfd pfd[1] =
            {
                {s32Fd, POLLIN | POLLERR, 0},
            };
            int iTimeOut, rval;
            MI_S32 s32MilliSec = 200;
            MI_VENC_ChnStat_t stStat;

            if(s32MilliSec == -1)
                iTimeOut = 0x7FFFFFFF;
            else
                iTimeOut = s32MilliSec;

            rval = poll(pfd, 1, iTimeOut);

            if(rval <= 0)
            {// time-out (0), or error ( < 0)
                DBG_ERR("CH%2d Time out\r\n", VencChannel);
                u32Retry++;
                continue;
                //return NULL;
            }
            else if((pfd[0].revents & POLLIN) != POLLIN)//any error or not POLLIN
            {
                DBG_ERR("CH%2d Error\r\n", VencChannel);
                return NULL;
            }

            s32Ret = MI_VENC_Query(VencChannel, &stStat);
            VencQuery("after poll", VencChannel);
            if(s32Ret != MI_SUCCESS || stStat.u32CurPacks == 0)
            {
                DBG_ERR("Unexpected query state:%X curPacks:%d\n", s32Ret, stStat.u32CurPacks);
                continue;
            }
        }
#endif
        memset(&stStream, 0, sizeof(stStream));
        memset(&stPack0, 0, sizeof(stPack0));
        stStream.pstPack = &stPack0;
        stStream.u32PackCount = 1;
        s32Ret = MI_VENC_GetStream(VencChannel, &stStream, 0);
        VencQuery("after GetStream", VencChannel);

        if(s32Ret != MI_SUCCESS)
        {
            if(s32Ret != MI_ERR_VENC_NOBUF)
            {
                DBG_ERR("GetStream Err:%X seq:%d\n\n", s32Ret, stStream.u32Seq);
                break;
            }
            u32Retry++;
            //DBG_ERR("CH%2d\n", VencChannel);
        }
#else
        hHandle = MI_HANDLE_NULL;
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

        s32Ret = MI_SYS_ChnOutputPortGetBuf(&stVencChn0OutputPort0, &stBufInfo, &hHandle);
        if(MI_SUCCESS != s32Ret)
        {
            if((((MI_U32)s32Ret) & 0xFF) == E_MI_ERR_NOBUF)//0x0D://E_MI_ERR_NOBUF
            {
                //DBG_INFO("malloc buf fail for CH%d output port%d\n", VencChannel, VencPortId);
            }
            u32Retry++;
            usleep(1 * 1000);//sleep 1 ms
        }
#endif
        else
        {
            MI_U32 u32ContentSize = 0;
#if !USE_GET_STREAM
            //DBG_INFO("[INFO] Get  %d bytes output\n", stBufInfo.stRawData.u32ContentSize);
            if(hHandle == NULL)
            {
                DBG_INFO("ch%02d NULL output port buffer handle.\n", VencChannel);
            }
            else if(stBufInfo.stRawData.pVirAddr == NULL)
            {
                DBG_INFO("ch%02d But unable to read buffer. VA==0\n", VencChannel);
            }
            else if(stBufInfo.stRawData.u32ContentSize >= stBufInfo.stRawData.u32BufSize)  //MAX_OUTPUT_ES_SIZE in KO
            {
                DBG_INFO("ch%02d buffer overflow %d >= %d\n", VencChannel, stBufInfo.stRawData.u32ContentSize,
                        stBufInfo.stRawData.u32ContentSize);
            }
            else
#endif
            {
                void* pVirAddr = NULL;
#if USE_GET_STREAM >= 1
                u32ContentSize = stStream.pstPack[0].u32Len - stStream.pstPack[0].u32Offset;
                bEndOfStream = (stStream.pstPack[0].bFrameEnd & 2)? TRUE: FALSE;
                pVirAddr = stStream.pstPack[0].pu8Addr;
#elif USE_GET_STREAM == 1
                stBufInfo.bEndOfStream = (stStream.pstPack[0].bFrameEnd & 2)? TRUE: FALSE;
                stBufInfo.u64Pts = stStream.pstPack[0].u64PTS;
                stBufInfo.stRawData.pVirAddr = stStream.pstPack[0].pu8Addr;
                stBufInfo.stRawData.phyAddr = stStream.pstPack[0].phyAddr;
                stBufInfo.stRawData.u32BufSize = (MI_U32)(-1);//unable to get buf size
                stBufInfo.stRawData.u64SeqNum = stStream.u32Seq;
                stBufInfo.stRawData.u32ContentSize = stStream.pstPack[0].u32Len - stStream.pstPack[0].u32Offset;
#else
                u32ContentSize = stBufInfo.stRawData.u32ContentSize;
                pVirAddr = stBufInfo.stRawData.pVirAddr;
                bEndOfStream = stBufInfo.bEndOfStream;

                //flush is should not be needed.
                MI_SYS_FlushCache(stBufInfo.stRawData.pVirAddr, stBufInfo.stRawData.u32ContentSize);
#endif

                if(u32Retry > 0)
                {
                    DBG_INFO("Retried %d times on CH%2d\n", u32Retry, VencChannel);
                    u32Retry = 0;
                }

                if(u32ContentSize > 0 && u32ContentSize < 10)
                {
                    int msg_level;
                    msg_level = get_cfg_int("VENC_GLOB_OUT_MSG_LEVEL", &bErr);

                    if(msg_level > 0)
                        DBG_INFO("data:%s\n", (char*)pVirAddr);
                }
                else
                {
                    print_es("[USER SPACE]", pVirAddr, VencChannel, u32ContentSize);
                }

                if(stFps.bRestart)
                {
                    stFps.bRestart = FALSE;
                    stFps.u32TotalBits = 0;
                    stFps.u32FrameCnt = 0;
                    gettimeofday(&stFps.stTimeStart, &stFps.stTimeZone);
                }
                gettimeofday(&stFps.stTimeEnd, &stFps.stTimeZone);
                stFps.u32DiffUs = TIMEVAL_US_DIFF(stFps.stTimeStart, stFps.stTimeEnd);
                stFps.u32TotalBits += u32ContentSize * 8;
                stFps.u32FrameCnt++;
                if (stFps.u32DiffUs > 1000000 && stFps.u32FrameCnt > 1)
                {
                    printf("<%5ld.%06ld> CH %02d get %.2f fps, %d kbps\n",
                            stFps.stTimeEnd.tv_sec, stFps.stTimeEnd.tv_usec,
                            VencChannel, (float) (stFps.u32FrameCnt-1) * 1000000.0f / stFps.u32DiffUs,
                            stFps.u32TotalBits * 1000 / stFps.u32DiffUs);
                    //printf("VENC bps: %d kbps\n", nBitrateCalcTotalBit * 1000 / nBitrateCalcUsDiff);
                    stFps.bRestart = TRUE;
                    set_result_int("FPSx100", (int)((float)stFps.u32FrameCnt*1000000.0f/stFps.u32DiffUs*100));
                }

                if((VENC_DUMP_FRAMES > 0) && (fd > 0))
                {
                    ssize_t ssize;
                    ssize = write(fd, pVirAddr, u32ContentSize);
                    if(ssize < 0)
                    {
                        DBG_INFO("\n==== es is too big:%d ===\n\n", u32ContentSize);
                        close(fd);
                        fd = 0;
                    }
                }

                if(bSaveEach && u32Seq < VENC_DUMP_FRAMES)
                {
                    snprintf(szOutputPath, sizeof(szOutputPath) - 1, "%sd%dc%02d_%03d.jpg",
                        OUT_PATH,
                        VencDevId, VencChannel, u32Seq);
                    fdJpg = open(szOutputPath,O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                    printf("open %d %s\n", fdJpg, szOutputPath);
                    if(fdJpg < 0)
                    {
                        printf("unable to open jpeg\r\n");
                        bSaveEach = FALSE;
                    }
                    else
                    {
                        ssize_t ssize;
                        ssize = write(fdJpg, pVirAddr, u32ContentSize);
                        if(ssize < 0)
                        {
                            printf("\n==== frame is too big:%d ===\n\n", u32ContentSize);
                        }
                        close(fdJpg);
                        fdJpg = 0;
                    }
                }
                u32Seq++;
            }
#if USE_GET_STREAM >= 1
            VencQuery("before RlsStream", VencChannel);
            s32Ret = MI_VENC_ReleaseStream(VencChannel, &stStream);
            VencQuery("after  RlsStream", VencChannel);
#else
            s32Ret = MI_SYS_ChnOutputPortPutBuf(hHandle);
#endif
            if(s32Ret != MI_SUCCESS)
            {
                DBG_ERR("Unable to put output %X\n", s32Ret);
            }
            //DBG_INFO("SEQ:%d pts:%lld sz:%d\n", stStream.u32Seq, stStream.pstPack[0].u64PTS, u32ContentSize);
            //DBG_INFO("SEQ:%lld sz:%d\n", stBufInfo.stRawData.u64SeqNum, stBufInfo.stRawData.u32ContentSize);

            if(VENC_DUMP_FRAMES && fd > 0 && u32Seq >= VENC_DUMP_FRAMES)
            {
                DBG_INFO("\n==== ch%2d %d frames wr done\n\n", VencChannel, u32Seq);
                close(fd);
                fd = 0;
            }

            if(bEndOfStream)
            {
                DBG_INFO("\n==== EOS ====\n\n");
                break;
            }
        }
    }

    if(VENC_DUMP_FRAMES && fd > 0)
    {
        close(fd);
        //fd = 0;//not used
    }
#if USE_FD
    if(s32Fd > 0)
    {
        s32Ret = MI_VENC_CloseFd(VencChannel);
        if(s32Ret != 0)
        {
            DBG_ERR("CH%02d Ret:%X\n", VencChannel, s32Ret);
        }
    }
#endif

    MI_SYS_SetChnOutputPortDepth(&stVencChn0OutputPort0, 0, 3);
    DBG_INFO("Thread Getting %02d exited.\n", VencChannel);

    return NULL;
}

void sleep_ms(int milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
