
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/file.h>
#include "mhal_venc.h"
#include "mhal_mfe.h"
#include "mhal_mhe.h"
#include "linux/time.h"
#include "cam_os_wrapper.h"
#include "NV12_cif_frames.h"
//#include "H264_qvga_10frame_qp25.h"
//#include "H264_qvga_10frame_qp45.h"
#include "file_access.h"
#ifdef SUPPORT_CMDQ_SERVICE
#include "mhal_cmdq.h"
#endif

MODULE_LICENSE("GPL");

#define ENCODE_H264
//#define ENCODE_H265

#if defined(ENCODE_H264)
    #define IP_NAME     "MFE"
    #define MHAL_VENC_CTRL_ID_RESOLUTION    E_MHAL_VENC_264_RESOLUTION
    #define MHAL_VENC_CTRL_ID_RC            E_MHAL_VENC_264_RC
    #define MHAL_VENC_CTRL_ID_ENTROPY       E_MHAL_VENC_264_ENTROPY
    #define MHAL_VENC_CTRL_ID_USER_DATA     E_MHAL_VENC_264_USER_DATA

    #define MHAL_VENC_CreateDevice          MHAL_MFE_CreateDevice
    #define MHAL_VENC_DestroyDevice         MHAL_MFE_DestroyDevice
    #define MHAL_VENC_GetDevConfig          MHAL_MFE_GetDevConfig
    #define MHAL_VENC_CreateInstance        MHAL_MFE_CreateInstance
    #define MHAL_VENC_DestroyInstance       MHAL_MFE_DestroyInstance
    #define MHAL_VENC_SetParam              MHAL_MFE_SetParam
    #define MHAL_VENC_GetParam              MHAL_MFE_GetParam
    #define MHAL_VENC_EncodeOneFrame        MHAL_MFE_EncodeOneFrame
    #define MHAL_VENC_EncodeFrameDone       MHAL_MFE_EncodeFrameDone
    #define MHAL_VENC_QueryBufSize          MHAL_MFE_QueryBufSize
    #define MHAL_VENC_IsrProc               MHAL_MFE_IsrProc

    #define E_MHAL_CMDQ_ID                  E_MHAL_CMDQ_ID_H264_VENC0
#elif defined(ENCODE_H265)
    #define IP_NAME     "MFE"
    #define MHAL_VENC_CTRL_ID_RESOLUTION    E_MHAL_VENC_265_RESOLUTION
    #define MHAL_VENC_CTRL_ID_RC            E_MHAL_VENC_265_RC
    #define MHAL_VENC_CTRL_ID_USER_DATA     E_MHAL_VENC_265_USER_DATA

    #define MHAL_VENC_CreateDevice          MHAL_MHE_CreateDevice
    #define MHAL_VENC_DestroyDevice         MHAL_MHE_DestroyDevice
    #define MHAL_VENC_GetDevConfig          MHAL_MHE_GetDevConfig
    #define MHAL_VENC_CreateInstance        MHAL_MHE_CreateInstance
    #define MHAL_VENC_DestroyInstance       MHAL_MHE_DestroyInstance
    #define MHAL_VENC_SetParam              MHAL_MHE_SetParam
    #define MHAL_VENC_GetParam              MHAL_MHE_GetParam
    #define MHAL_VENC_EncodeOneFrame        MHAL_MHE_EncodeOneFrame
    #define MHAL_VENC_EncodeFrameDone       MHAL_MHE_EncodeFrameDone
    #define MHAL_VENC_QueryBufSize          MHAL_MHE_QueryBufSize
    #define MHAL_VENC_IsrProc               MHAL_MHE_IsrProc

    #define E_MHAL_CMDQ_ID                  E_MHAL_CMDQ_ID_H265_VENC0
#endif

#define YUV_WIDTH   352
#define YUV_HEIGHT  288
#define YUV_FRAME_SIZE  (YUV_WIDTH*YUV_HEIGHT*3/2)

#define SPECVERSIONMAJOR 1
#define SPECVERSIONMINOR 0
#define SPECREVISION 0
#define SPECSTEP 0

CamOsTsem_t stDev0FrameDone;

#ifdef SUPPORT_CMDQ_SERVICE
MHAL_CMDQ_CmdqInterface_t *stCmdQInf;
#endif

void* gMhalDev0 = NULL;
void* gMhalInst0 = NULL;
void* gMhalInst1 = NULL;

static unsigned int core_id = 0;
module_param(core_id, uint, S_IRUGO|S_IWUSR);

static unsigned int loop_run = 0;
module_param(loop_run, uint, S_IRUGO|S_IWUSR);

static unsigned int cmdq_en = 1;
module_param(cmdq_en, uint, S_IRUGO|S_IWUSR);

static unsigned int multi_stream = 0;
module_param(multi_stream, uint, S_IRUGO|S_IWUSR);

static unsigned int partial_out = 0;
module_param(partial_out, uint, S_IRUGO|S_IWUSR);

static unsigned int user_data = 0;
module_param(user_data, uint, S_IRUGO|S_IWUSR);

void setHeader(VOID* header, MS_U32 size) {
    MHAL_VENC_Version_t* ver = (MHAL_VENC_Version_t*)header;
    ver->u32Size = size;

    ver->s.u8VersionMajor = SPECVERSIONMAJOR;
    ver->s.u8VersionMinor = SPECVERSIONMINOR;
    ver->s.u8Revision = SPECREVISION;
    ver->s.u8Step = SPECSTEP;
}

void print_hex(void* buf, int num) {
    int i, j;
    char *data = (char *) buf;
    char ascii[17] = "";

    CamOsPrintf("\nOffset(h)  00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\n"
                "-----------------------------------------------------------");
    for (i = 0; i < num; i++)
    {
        if (i % 16 == 0)
        {
            CamOsPrintf("\n%08X  ", i);
            memset(ascii, 0x00, sizeof(ascii));
        }
        if (i % 8 == 0)
        {
            CamOsPrintf(" ");
        }

        CamOsPrintf("%02X ", data[i]);

        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char*)data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }

        if (i % 16 == 15)
        {
            CamOsPrintf(" |%s|", ascii);
        }
        else if (i == num-1)
        {
            for (j = (i+1) % 16; j < 16; ++j)
            {
                CamOsPrintf("   ");
            }
            if ((i+1) % 16 <= 8)
            {
                CamOsPrintf(" ");
            }
            CamOsPrintf(" |%s|", ascii);
        }
    }
    CamOsPrintf("\n");
}

int _CompareArrays(u8* a, u8* b, int len)
{
    int i = 0;
    for(i = 0; i < len; i++)
    {
        if (a[i] != b[i])
            return -1;
    }
    return 0;
}

void VencNotifyCallbackFunc(unsigned long nBufAddr, unsigned long nOffset, unsigned long nNotifySize, unsigned short bFrameDone)
{
    CamOsPrintf("%s: nBufAddr = 0x%x, nOffset = 0x%x, nNotifySize = 0x%x, bFrameDone = %u\n", __FUNCTION__, nBufAddr, nOffset, nNotifySize, bFrameDone);
}

static irqreturn_t _VencUtIsr(int irq, void* priv)
{
    //CamOsPrintf("_VencUtIsr\n");
    if (MHAL_VENC_IsrProc(priv) == 0)
        CamOsTsemUp(&stDev0FrameDone);

    return IRQ_HANDLED;
}

static int VencProbe(void)
{
    MHAL_VENC_ParamInt_t param;
    MHAL_VENC_Resoluton_t ResCtl;
    MHAL_VENC_RcInfo_t RcCtl;
    MHAL_VENC_EnableIdr_t EnableIdrCtl;
#if defined(ENCODE_H264)
    MHAL_VENC_ParamH264Entropy_t EntropyCtl;
#endif
    MHAL_VENC_UserData_t UserData;
    MHAL_VENC_InternalBuf_t stVencIntrBuf;
    MHAL_VENC_InOutBuf_t Inst0InOutBuf;
    MHAL_VENC_EncResult_t Inst0EncRet;
    void* pYuvBufVitr = NULL;
    void* pYuvBufPhys = NULL;
    void* pYuvBufMiu = NULL;
    void* pInst0BsBufVitr = NULL;
    void* pInst0BsBufPhys = NULL;
    void* pInst0BsBufMiu = NULL;
    void *pInst0InternalBuf1Virt = NULL;
    void *pInst0InternalBuf1Phys = NULL;
    void *pInst0InternalBuf1Miu = NULL;
    void *pInst0InternalBuf2Virt = NULL;
    void *pInst0InternalBuf2Phys = NULL;
    void *pInst0InternalBuf2Miu = NULL;
    u32 u32Inst0TotalLen = 0;
    MHAL_VENC_InOutBuf_t Inst1InOutBuf;
    MHAL_VENC_EncResult_t Inst1EncRet;
    void* pInst1BsBufVitr = NULL;
    void* pInst1BsBufPhys = NULL;
    void* pInst1BsBufMiu = NULL;
    void *pInst1InternalBuf1Virt = NULL;
    void *pInst1InternalBuf1Phys = NULL;
    void *pInst1InternalBuf1Miu = NULL;
    void *pInst1InternalBuf2Virt = NULL;
    void *pInst1InternalBuf2Phys = NULL;
    void *pInst1InternalBuf2Miu = NULL;
    u32 u32Inst1TotalLen = 0;
    int frame_cnt;
    int VencIrq = 0;
#ifdef SUPPORT_CMDQ_SERVICE
    MHAL_CMDQ_BufDescript_t stCmdqBufDesp;
#endif
    MHAL_VENC_PartialNotify_t PartialNotify;
    struct file *fd0;
    struct file *fd1;
    char szFileName[64];
    //int test_run;
    struct timeval start_tv, end_tv;
    u32 total_us = 0;
    char UserDataBuf[1024];
    char szDmemName[32];

    //u32 nTestRun = 0;

    CamOsTsemInit(&stDev0FrameDone, 0);

    if (core_id > 1)
    {
        CamOsPrintf("[KUT] %s not support core%d\n", IP_NAME, core_id);
        return 0;
    }

    if (MHAL_VENC_CreateDevice(core_id, &gMhalDev0))
    {
        CamOsPrintf("[KUT] MHAL_%s_CreateDevice Fail\n", IP_NAME);
        return 0;
    }

    setHeader(&param, sizeof(param));
    if (MHAL_VENC_GetDevConfig(gMhalDev0, E_MHAL_VENC_HW_IRQ_NUM, &param))
    {
        CamOsPrintf("[KUT] MHAL_%s_GetDevConfig Fail\n", IP_NAME);
        return 0;
    }

    VencIrq = param.u32Val;
    CamOsPrintf("[KUT] irq %d\n", VencIrq);
    if (0 != request_irq(VencIrq, _VencUtIsr, IRQF_SHARED, "_VencUtIsr", gMhalDev0))
    {
        CamOsPrintf("[KUT] request_irq(%d) Fail\n", param.u32Val);
        return 0;
    }

    setHeader(&param, sizeof(param));
    if (MHAL_VENC_GetDevConfig(gMhalDev0, E_MHAL_VENC_HW_CMDQ_BUF_LEN, &param))
    {
        CamOsPrintf("[KUT] MHAL_%s_GetDevConfig Fail\n", IP_NAME);
        return 0;
    }
#ifdef SUPPORT_CMDQ_SERVICE
    if (cmdq_en)
    {
        stCmdqBufDesp.u32CmdqBufSize = param.u32Val;
        stCmdqBufDesp.u32CmdqBufSizeAlign = 16;
        stCmdqBufDesp.u32MloadBufSize = 0;
        stCmdqBufDesp.u16MloadBufSizeAlign = 16;
        stCmdQInf = MHAL_CMDQ_GetSysCmdqService(E_MHAL_CMDQ_ID, &stCmdqBufDesp, FALSE);
        CamOsPrintf("Call MHAL_CMDQ_GetSysCmdqService: 0x%08X 0x%08X\n", stCmdQInf, stCmdQInf->MHAL_CMDQ_CheckBufAvailable);
    }
#endif


    if (MHAL_VENC_CreateInstance(gMhalDev0, &gMhalInst0))
    {
        CamOsPrintf("[KUT] MHAL_%s_CreateInstance Fail\n", IP_NAME);
        return 0;
    }


    /* Set Venc parameter */
    memset(&ResCtl, 0, sizeof(ResCtl));
    setHeader(&ResCtl, sizeof(ResCtl));
    ResCtl.u32Width = YUV_WIDTH;
    ResCtl.u32Height = YUV_HEIGHT;
    ResCtl.eFmt = E_MHAL_VENC_FMT_NV12;
    MHAL_VENC_SetParam(gMhalInst0, MHAL_VENC_CTRL_ID_RESOLUTION, &ResCtl);


    memset(&ResCtl, 0, sizeof(ResCtl));
    setHeader(&ResCtl, sizeof(ResCtl));
    MHAL_VENC_GetParam(gMhalInst0, MHAL_VENC_CTRL_ID_RESOLUTION, &ResCtl);
    CamOsPrintf("[KUT] resolution %dx%d\n", ResCtl.u32Width, ResCtl.u32Height);


    memset(&RcCtl, 0, sizeof(RcCtl));
    setHeader(&RcCtl, sizeof(RcCtl));
#if defined(ENCODE_H264)
    RcCtl.eRcMode = E_MHAL_VENC_RC_MODE_H264FIXQP;
    RcCtl.stAttrH264FixQp.u32SrcFrmRate = 30;
    RcCtl.stAttrH264FixQp.u32Gop = 9;
    RcCtl.stAttrH264FixQp.u32IQp = 25;
    RcCtl.stAttrH264FixQp.u32PQp = 25;
#elif defined(ENCODE_H265)
    RcCtl.eRcMode = E_MHAL_VENC_RC_MODE_H265FIXQP;
    RcCtl.stAttrH265FixQp.u32SrcFrmRate = 30;
    RcCtl.stAttrH265FixQp.u32Gop = 9;
    RcCtl.stAttrH265FixQp.u32IQp = 25;
    RcCtl.stAttrH265FixQp.u32PQp = 25;
#endif
    MHAL_VENC_SetParam(gMhalInst0, MHAL_VENC_CTRL_ID_RC, &RcCtl);


    memset(&RcCtl, 0, sizeof(RcCtl));
    setHeader(&RcCtl, sizeof(RcCtl));
    MHAL_VENC_GetParam(gMhalInst0, MHAL_VENC_CTRL_ID_RC, &RcCtl);
#if defined(ENCODE_H264)
    CamOsPrintf("[KUT] RC %d %d %d\n", RcCtl.stAttrH264FixQp.u32Gop, RcCtl.stAttrH264FixQp.u32IQp, RcCtl.stAttrH264FixQp.u32PQp);
#elif defined(ENCODE_H265)
    CamOsPrintf("[KUT] RC %d %d %d\n", RcCtl.stAttrH265FixQp.u32Gop, RcCtl.stAttrH265FixQp.u32IQp, RcCtl.stAttrH265FixQp.u32PQp);
#endif


    memset(&EnableIdrCtl, 0, sizeof(EnableIdrCtl));
    setHeader(&EnableIdrCtl, sizeof(EnableIdrCtl));
    EnableIdrCtl.bEnable = 1;
    MHAL_VENC_SetParam(gMhalInst0, E_MHAL_VENC_ENABLE_IDR, &EnableIdrCtl);


    memset(&EnableIdrCtl, 0, sizeof(EnableIdrCtl));
    setHeader(&EnableIdrCtl, sizeof(EnableIdrCtl));
    MHAL_VENC_GetParam(gMhalInst0, E_MHAL_VENC_ENABLE_IDR, &EnableIdrCtl);
    CamOsPrintf("[KUT] EnableIdr %d\n", EnableIdrCtl.bEnable);

#if defined(ENCODE_H264)

    memset(&EntropyCtl, 0, sizeof(EntropyCtl));
    setHeader(&EntropyCtl, sizeof(EntropyCtl));
    EntropyCtl.u32EntropyEncModeI = 1;
    EntropyCtl.u32EntropyEncModeP = 1;
    MHAL_VENC_SetParam(gMhalInst0, MHAL_VENC_CTRL_ID_ENTROPY, &EntropyCtl);


    memset(&EntropyCtl, 0, sizeof(EntropyCtl));
    setHeader(&EntropyCtl, sizeof(EntropyCtl));
    MHAL_VENC_GetParam(gMhalInst0, MHAL_VENC_CTRL_ID_ENTROPY, &EntropyCtl);
    CamOsPrintf("[KUT] Entropy %d %d\n", EntropyCtl.u32EntropyEncModeI, EntropyCtl.u32EntropyEncModeP);
#endif


    memset(&stVencIntrBuf, 0, sizeof(MHAL_VENC_InternalBuf_t));
    setHeader(&stVencIntrBuf, sizeof(stVencIntrBuf));
    MHAL_VENC_QueryBufSize(gMhalInst0, &stVencIntrBuf);
    CamOsPrintf("[KUT] internal buffer size %d %d\n", stVencIntrBuf.u32IntrAlBufSize, stVencIntrBuf.u32IntrRefBufSize);

    if (partial_out)
    {
        memset(&PartialNotify, 0, sizeof(PartialNotify));
        setHeader(&PartialNotify, sizeof(PartialNotify));
        PartialNotify.u32NotifySize = partial_out;
        PartialNotify.notifyFunc = VencNotifyCallbackFunc;
        MHAL_VENC_SetParam(gMhalInst0, E_MHAL_VENC_PARTIAL_NOTIFY, &PartialNotify);


        memset(&PartialNotify, 0, sizeof(PartialNotify));
        setHeader(&PartialNotify, sizeof(PartialNotify));
        MHAL_VENC_GetParam(gMhalInst0, E_MHAL_VENC_PARTIAL_NOTIFY, &PartialNotify);
        CamOsPrintf("[KUT] partial notify size %u\n", PartialNotify.u32NotifySize);
    }

    /* Allocate Venc internal buffer and assign to driver */
    CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sINTRALBUF0", IP_NAME);
    CamOsDirectMemAlloc(szDmemName, stVencIntrBuf.u32IntrAlBufSize, &pInst0InternalBuf1Virt, &pInst0InternalBuf1Phys, &pInst0InternalBuf1Miu);
    CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sINTRREFBUF0", IP_NAME);
    CamOsDirectMemAlloc(szDmemName, stVencIntrBuf.u32IntrRefBufSize, &pInst0InternalBuf2Virt, &pInst0InternalBuf2Phys, &pInst0InternalBuf2Miu);
    stVencIntrBuf.pu8IntrAlVirBuf = (MS_U8 *)pInst0InternalBuf1Virt;
    stVencIntrBuf.phyIntrAlPhyBuf = (MS_PHYADDR)(u32)pInst0InternalBuf1Miu;
    stVencIntrBuf.phyIntrRefPhyBuf = (MS_PHYADDR)(u32)pInst0InternalBuf2Miu;
    MHAL_VENC_SetParam(gMhalInst0, E_MHAL_VENC_IDX_STREAM_ON, &stVencIntrBuf);


    if (multi_stream && cmdq_en)
    {
        if (MHAL_VENC_CreateInstance(gMhalDev0, &gMhalInst1))
        {
            CamOsPrintf("[KUT] MHAL_%s_CreateInstance Fail\n", IP_NAME);
            return 0;
        }


        /* Set Venc parameter */
        memset(&ResCtl, 0, sizeof(ResCtl));
        setHeader(&ResCtl, sizeof(ResCtl));
        ResCtl.u32Width = YUV_WIDTH;
        ResCtl.u32Height = YUV_HEIGHT;
        ResCtl.eFmt = E_MHAL_VENC_FMT_NV12;
        MHAL_VENC_SetParam(gMhalInst1, MHAL_VENC_CTRL_ID_RESOLUTION, &ResCtl);


        memset(&ResCtl, 0, sizeof(ResCtl));
        setHeader(&ResCtl, sizeof(ResCtl));
        MHAL_VENC_GetParam(gMhalInst1, MHAL_VENC_CTRL_ID_RESOLUTION, &ResCtl);
        CamOsPrintf("[KUT] resolution %dx%d\n", ResCtl.u32Width, ResCtl.u32Height);


        memset(&RcCtl, 0, sizeof(RcCtl));
        setHeader(&RcCtl, sizeof(RcCtl));
#if defined(ENCODE_H264)
        RcCtl.eRcMode = E_MHAL_VENC_RC_MODE_H264FIXQP;
        RcCtl.stAttrH264FixQp.u32SrcFrmRate = 30;
        RcCtl.stAttrH264FixQp.u32Gop = 9;
        RcCtl.stAttrH264FixQp.u32IQp = 45;
        RcCtl.stAttrH264FixQp.u32PQp = 45;
#elif defined(ENCODE_H265)
        RcCtl.eRcMode = E_MHAL_VENC_RC_MODE_H265FIXQP;
        RcCtl.stAttrH265FixQp.u32SrcFrmRate = 30;
        RcCtl.stAttrH265FixQp.u32Gop = 9;
        RcCtl.stAttrH265FixQp.u32IQp = 45;
        RcCtl.stAttrH265FixQp.u32PQp = 45;
#endif
        MHAL_VENC_SetParam(gMhalInst1, MHAL_VENC_CTRL_ID_RC, &RcCtl);


        memset(&RcCtl, 0, sizeof(RcCtl));
        setHeader(&RcCtl, sizeof(RcCtl));
        MHAL_VENC_GetParam(gMhalInst1, MHAL_VENC_CTRL_ID_RC, &RcCtl);
#if defined(ENCODE_H264)
        CamOsPrintf("[KUT] RC %d %d %d\n", RcCtl.stAttrH264FixQp.u32Gop, RcCtl.stAttrH264FixQp.u32IQp, RcCtl.stAttrH264FixQp.u32PQp);
#elif defined(ENCODE_H265)
        CamOsPrintf("[KUT] RC %d %d %d\n", RcCtl.stAttrH265FixQp.u32Gop, RcCtl.stAttrH265FixQp.u32IQp, RcCtl.stAttrH265FixQp.u32PQp);
#endif


        memset(&EnableIdrCtl, 0, sizeof(EnableIdrCtl));
        setHeader(&EnableIdrCtl, sizeof(EnableIdrCtl));
        EnableIdrCtl.bEnable = 1;
        MHAL_VENC_SetParam(gMhalInst0, E_MHAL_VENC_ENABLE_IDR, &EnableIdrCtl);


        memset(&EnableIdrCtl, 0, sizeof(EnableIdrCtl));
        setHeader(&EnableIdrCtl, sizeof(EnableIdrCtl));
        MHAL_VENC_GetParam(gMhalInst0, E_MHAL_VENC_ENABLE_IDR, &EnableIdrCtl);
        CamOsPrintf("[KUT] EnableIdr %d\n", EnableIdrCtl.bEnable);


        memset(&stVencIntrBuf, 0, sizeof(MHAL_VENC_InternalBuf_t));
        setHeader(&stVencIntrBuf, sizeof(stVencIntrBuf));
        MHAL_VENC_QueryBufSize(gMhalInst1, &stVencIntrBuf);
        CamOsPrintf("[KUT] internal buffer size %d %d\n", stVencIntrBuf.u32IntrAlBufSize, stVencIntrBuf.u32IntrRefBufSize);


        /* Allocate Venc internal buffer and assign to driver */
        CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sINTRALBUF1", IP_NAME);
        CamOsDirectMemAlloc(szDmemName, stVencIntrBuf.u32IntrAlBufSize, &pInst1InternalBuf1Virt, &pInst1InternalBuf1Phys, &pInst1InternalBuf1Miu);
        CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sINTRREFBUF1", IP_NAME);
        CamOsDirectMemAlloc(szDmemName, stVencIntrBuf.u32IntrRefBufSize, &pInst1InternalBuf2Virt, &pInst1InternalBuf2Phys, &pInst1InternalBuf2Miu);
        stVencIntrBuf.pu8IntrAlVirBuf = (MS_U8 *)pInst1InternalBuf1Virt;
        stVencIntrBuf.phyIntrAlPhyBuf = (MS_PHYADDR)(u32)pInst1InternalBuf1Miu;
        stVencIntrBuf.phyIntrRefPhyBuf = (MS_PHYADDR)(u32)pInst1InternalBuf2Miu;
        MHAL_VENC_SetParam(gMhalInst1, E_MHAL_VENC_IDX_STREAM_ON, &stVencIntrBuf);
    }


    CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sIBUFF", IP_NAME);
    CamOsDirectMemAlloc(szDmemName, YUV_FRAME_SIZE, &pYuvBufVitr, &pYuvBufPhys, &pYuvBufMiu);
    CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sOBUFF0", IP_NAME);
    CamOsDirectMemAlloc(szDmemName, YUV_FRAME_SIZE, &pInst0BsBufVitr, &pInst0BsBufPhys, &pInst0BsBufMiu);
    Inst0InOutBuf.pu32RegBase0 = NULL;
    Inst0InOutBuf.pu32RegBase1 = NULL;
#ifdef SUPPORT_CMDQ_SERVICE
    if (cmdq_en)
    {
        Inst0InOutBuf.pCmdQ = stCmdQInf;
    }
    else
    {
        Inst0InOutBuf.pCmdQ = NULL;
    }
#else
    Inst0InOutBuf.pCmdQ = NULL;
#endif
    Inst0InOutBuf.bRequestI = 0;
    Inst0InOutBuf.phyInputYUVBuf1 = (MS_PHYADDR)(u32)pYuvBufMiu;
    Inst0InOutBuf.u32InputYUVBuf1Size = YUV_FRAME_SIZE;
    Inst0InOutBuf.phyOutputBuf = (MS_PHYADDR)(u32)pInst0BsBufMiu;
    Inst0InOutBuf.virtOutputBuf = (MS_PHYADDR)(u32)pInst0BsBufVitr;
    Inst0InOutBuf.u32OutputBufSize = YUV_FRAME_SIZE;


    CamOsSnprintf(szFileName, sizeof(szFileName), "/vendor/i2_enc/kernel_ut_ch%d_frm%d.es", 0, frame_cnt);
    fd0 = OpenFile(szFileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if(IS_ERR(fd0))
    {
        CamOsPrintf("kernel open file fail, err %d\n", PTR_ERR(fd0));
        return 0;
    }
    else
    {
        CamOsPrintf("kernel open file success\n");
    }


    if (multi_stream && cmdq_en)
    {
        CamOsSnprintf(szDmemName, sizeof(szDmemName), "%sOBUFF1", IP_NAME);
        CamOsDirectMemAlloc(szDmemName, YUV_FRAME_SIZE, &pInst1BsBufVitr, &pInst1BsBufPhys, &pInst1BsBufMiu);
        Inst1InOutBuf.pu32RegBase0 = NULL;
        Inst1InOutBuf.pu32RegBase1 = NULL;
#ifdef SUPPORT_CMDQ_SERVICE
        if (cmdq_en)
        {
            Inst1InOutBuf.pCmdQ = stCmdQInf;
        }
        else
        {
            Inst1InOutBuf.pCmdQ = NULL;
        }
#else
        Inst1InOutBuf.pCmdQ = NULL;
#endif
        Inst1InOutBuf.bRequestI = 0;
        Inst1InOutBuf.phyInputYUVBuf1 = (MS_PHYADDR)(u32)pYuvBufMiu;
        Inst1InOutBuf.u32InputYUVBuf1Size = YUV_FRAME_SIZE;
        Inst1InOutBuf.phyOutputBuf = (MS_PHYADDR)(u32)pInst1BsBufMiu;
        Inst1InOutBuf.virtOutputBuf = (MS_PHYADDR)(u32)pInst1BsBufVitr;
        Inst1InOutBuf.u32OutputBufSize = YUV_FRAME_SIZE;


        CamOsSnprintf(szFileName, sizeof(szFileName), "/vendor/i2_enc/kernel_ut_ch%d_frm%d.es", 1, frame_cnt);
        fd1 = OpenFile(szFileName, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if(IS_ERR(fd1))
        {
            CamOsPrintf("kernel open file fail, err %d\n", PTR_ERR(fd1));
            return 0;
        }
        else
        {
            CamOsPrintf("kernel open file success\n");
        }
    }

    do_gettimeofday(&start_tv);
    while (1)
    {
        //for (test_run=0; test_run<100; test_run++)
        {
        for (frame_cnt=0; frame_cnt < (sizeof(gYUV)/YUV_FRAME_SIZE); frame_cnt++)
        {
            memcpy(pYuvBufVitr, gYUV+frame_cnt*YUV_FRAME_SIZE, YUV_FRAME_SIZE);

            if (user_data)
            {
                memset(&UserData, 0, sizeof(UserData));
                setHeader(&UserData, sizeof(UserData));
                CamOsSnprintf(UserDataBuf, sizeof(UserDataBuf), "This is Frame %d", frame_cnt);
                UserData.pu8Data = UserDataBuf;
                UserData.u32Len = strlen(UserDataBuf);
                MHAL_VENC_SetParam(gMhalInst0, MHAL_VENC_CTRL_ID_USER_DATA, &UserData);
            }
            MHAL_VENC_EncodeOneFrame(gMhalInst0, &Inst0InOutBuf);
            if (multi_stream && cmdq_en)
            {
                if (user_data)
                {
                    memset(&UserData, 0, sizeof(UserData));
                    setHeader(&UserData, sizeof(UserData));
                    CamOsSnprintf(UserDataBuf, sizeof(UserDataBuf), "This is Frame %d", frame_cnt);
                    UserData.pu8Data = UserDataBuf;
                    UserData.u32Len = strlen(UserDataBuf);
                    MHAL_VENC_SetParam(gMhalInst1, MHAL_VENC_CTRL_ID_USER_DATA, &UserData);
                }
                MHAL_VENC_EncodeOneFrame(gMhalInst1, &Inst1InOutBuf);
            }

#ifdef SUPPORT_CMDQ_SERVICE
            if (cmdq_en)
            {
                stCmdQInf->MHAL_CMDQ_KickOffCmdq(stCmdQInf);
            }
#endif

            CamOsTsemDown(&stDev0FrameDone);    // wait encode frame done
            if (multi_stream && cmdq_en)
            {
                CamOsTsemDown(&stDev0FrameDone);    // wait encode frame done
            }

            MHAL_VENC_EncodeFrameDone(gMhalInst0, &Inst0EncRet);
            if (multi_stream && cmdq_en)
            {
                MHAL_VENC_EncodeFrameDone(gMhalInst1, &Inst1EncRet);
            }

            if(!IS_ERR(fd0))
            {
                WriteFile(fd0, pInst0BsBufVitr, Inst0EncRet.u32OutputBufUsed);
            }

            CamOsPrintf("frame:%d, size:%d \n", frame_cnt, Inst0EncRet.u32OutputBufUsed);
            //print_hex("bitstream", pInst0BsBufVitr, Inst0EncRet.u32OutputBufUsed);
            /*if(Inst0EncRet.u32OutputBufUsed && _CompareArrays((u8 *)(gH264_qp45 + u32Inst0TotalLen), (u8 *)pInst0BsBufVitr, Inst0EncRet.u32OutputBufUsed) == 0)
                CamOsPrintf("Inst0 Encode Result Compare OK!!!\n");
            else
                CamOsPrintf("Inst0 Encode Result Compare Fail!!!\n");*/
            u32Inst0TotalLen += Inst0EncRet.u32OutputBufUsed;
            if (multi_stream && cmdq_en)
            {
                if(!IS_ERR(fd1))
                {
                    WriteFile(fd1, pInst1BsBufVitr, Inst1EncRet.u32OutputBufUsed);
                }

                CamOsPrintf("frame:%d, size:%d \n", frame_cnt, Inst1EncRet.u32OutputBufUsed);
                //print_hex("bitstream", pInst1BsBufVitr, Inst1EncRet.u32OutputBufUsed);
                /*if(Inst1EncRet.u32OutputBufUsed && _CompareArrays((u8 *)(gH264_qp25 + u32Inst1TotalLen), (u8 *)pInst1BsBufVitr, Inst1EncRet.u32OutputBufUsed) == 0)
                    CamOsPrintf("Inst1 Encode Result Compare OK!!!\n");
                else
                    CamOsPrintf("Inst1 Encode Result Compare Fail!!!\n");*/
                u32Inst1TotalLen += Inst1EncRet.u32OutputBufUsed;
            }
        }

        u32Inst0TotalLen = 0;
        if (multi_stream && cmdq_en)
        {
            u32Inst1TotalLen = 0;
        }

        }

        if(!IS_ERR(fd0))
        {
            CloseFile(fd0);
        }

        if (multi_stream && cmdq_en)
        {
            if(!IS_ERR(fd1))
            {
                CloseFile(fd1);
            }
        }

        if (!loop_run)
            break;
    }
    do_gettimeofday(&end_tv);

    total_us = (end_tv.tv_sec - start_tv.tv_sec)*1000000 + end_tv.tv_usec - start_tv.tv_usec;
    CamOsPrintf("FPS: %d\n", 1000*1000000/total_us);

    if (MHAL_VENC_DestroyInstance(gMhalInst0))
    {
        CamOsPrintf("[KUT] MHAL_%s_DestroyInstance Fail\n", IP_NAME);
        return 0;
    }
    if (multi_stream && cmdq_en)
    {
        if (MHAL_VENC_DestroyInstance(gMhalInst1))
        {
            CamOsPrintf("[KUT] MHAL_%s_DestroyInstance Fail\n", IP_NAME);
            return 0;
        }
    }

    free_irq(VencIrq, gMhalDev0);

    if (MHAL_VENC_DestroyDevice(gMhalDev0))
    {
        CamOsPrintf("[KUT] MHAL_%s_DestroyDevice Fail\n", IP_NAME);
        return 0;
    }

    CamOsDirectMemRelease(pYuvBufVitr, YUV_FRAME_SIZE);
    CamOsDirectMemRelease(pInst0BsBufVitr, YUV_FRAME_SIZE);
    CamOsDirectMemRelease(pInst0InternalBuf1Virt, stVencIntrBuf.u32IntrAlBufSize);
    CamOsDirectMemRelease(pInst0InternalBuf2Virt, stVencIntrBuf.u32IntrRefBufSize);
    if (multi_stream && cmdq_en)
    {
        CamOsDirectMemRelease(pInst1BsBufVitr, YUV_FRAME_SIZE);
        CamOsDirectMemRelease(pInst1InternalBuf1Virt, stVencIntrBuf.u32IntrAlBufSize);
        CamOsDirectMemRelease(pInst1InternalBuf2Virt, stVencIntrBuf.u32IntrRefBufSize);
    }

#ifdef SUPPORT_CMDQ_SERVICE
    if (cmdq_en)
    {
        CamOsPrintf("Call MHAL_CMDQ_ReleaseSysCmdqService\n");
        MHAL_CMDQ_ReleaseSysCmdqService(E_MHAL_CMDQ_ID);
    }
#endif

    return 0;
}

static int VencRemove(void)
{
    return 0;
}

static int  __init venc_init(void)
{
    VencProbe();
    return 0;
}

static void __exit venc_exit(void)
{
    VencRemove();
}

module_init(venc_init);
module_exit(venc_exit);
