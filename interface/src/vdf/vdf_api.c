#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>


#define VDF_TEST_ON_I3          0
#define VDF_SAVE_IMAGE_ON_I3    0
#define MD_OD_SINGLE_PTHREAD    0
#define VDF_DBG_LOG_ENABLE      0

#if (VDF_TEST_ON_I3)
#include "mi_venc.h"
#include "mi_vi.h"
#else
#include "mi_shadow.h"
#endif

#include "mi_md.h"
#include "mi_od.h"
#include "mi_vdf.h"

#include "mi_vdf.h"
#include "mi_vdf_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_common_datatype.h"



#if 0
#define MI_VDF_FUNC_ENTRY()        if(1){printf("%s In\n", __func__);}
#define MI_VDF_FUNC_EXIT()         if(1){printf("%s Exit\n", __func__);}
#define MI_VDF_FUNC_ENTRY2(ViChn)  if(1){printf("%s In:chn=%d\n", __func__,ViChn);}
#define MI_VDF_FUNC_EXIT2(ViChn)   if(1){printf("%s Exit:chn=%d\n", __func__, ViChn);}
#else
#define MI_VDF_FUNC_ENTRY()
#define MI_VDF_FUNC_EXIT()
#define MI_VDF_FUNC_ENTRY2()
#define MI_VDF_FUNC_EXIT2()
#endif


static MI_VDF_MdResult_t MI_VDF_MD_RST_LIST[MI_VDF_CHANNEL_NUM_MAX];
static MI_VDF_OdResult_t MI_VDF_OD_RST_LIST[MI_VDF_CHANNEL_NUM_MAX];
static MI_VDF_EntryNode_t MI_VDF_ENTRY_MODE_LIST[MI_VDF_CHANNEL_NUM_MAX];


S32 g_width = 320;
S32 g_height = 180;
static MI_U8 g_MDEnable = 0;
static MI_U8 g_ODEnable = 0;
static BOOL g_VdfYuvTaskExit = FALSE;
static BOOL g_VdfAPPTaskExit[MI_VDF_CHANNEL_NUM_MAX];


#if !(VDF_TEST_ON_I3)
//MI_SYS_DRV_HANDLE miSysVDFHandle = NULL;
//mi_sys_ModuleDevInfo_t stVDFInfo = { 0 };
//mi_sys_ModuleDevBindOps_t stVDFBindOps = { 0 };

MI_SHADOW_RegisterDevParams_t _stVDFRegDevInfo;
MI_SHADOW_HANDLE _hVDFDev = 0;
#endif

typedef struct _VDF_PTHREAD_MUTEX_S
{
    MI_U8   u8Enable;
    MI_U32  u32YImage_size;
    MI_U64  u64Pts;

    MI_VDF_MDParamsOut_t param_out;
    MI_MD_Result_t   stMdDetectRst;
    MI_MD_Result_t*  pstMdRstList;
    MI_OD_Result_t   stOdDetectRst;
    MI_OD_Result_t*  pstOdRstList;
    MI_VDF_MdRstHdlList_t* pstMdRstHdlList;
    MI_VDF_OdRstHdlList_t* pstOdRstHdlList;
    unsigned char*  pu8YBuffer;
#if !(VDF_TEST_ON_I3)
    MI_SYS_BUF_HANDLE hInputBufHandle;
#endif
    pthread_mutex_t mutexMDRW;
    pthread_mutex_t mutexODRW;
    pthread_mutex_t mutexRUN;
    pthread_mutex_t mutexVDFAPP;
    pthread_cond_t  condVideoSrcChnY;
    pthread_mutex_t mutexVideoSrcChnY;
} VDF_PTHREAD_MUTEX_S;

static pthread_t pthreadVDFYUV;
static pthread_t pthreadVDFAPP[MI_VDF_CHANNEL_NUM_MAX];
VDF_PTHREAD_MUTEX_S VDF_PTHREAD_MUTEX[MI_VDF_CHANNEL_NUM_MAX];

static pthread_mutex_t mutexRUN = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutexVDF = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  condVDF = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex_VDF_cfg = PTHREAD_MUTEX_INITIALIZER;


unsigned int _MI_VDF_GetTimeInMs(void)
{
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    unsigned int T = (1000000 * (t1.tv_sec) + (t1.tv_nsec) / 1000) / 1000 ;
    return T;
}


inline MI_S32 _MI_VDF_MD_RunAndSetRst(MI_U8 u8ViSrcChn, MI_VDF_NodeList_t* pstVDFNode)
{
    MI_U8 i, j;
    MI_U32 T0, T1;

    MI_VDF_FUNC_ENTRY();

    //TODO: check if Width*Height*1.5=videoFrame.vFrame.bufLen
    pstVDFNode->u32New_t = _MI_VDF_GetTimeInMs(); // get system time in ms

#if (MD_OD_SINGLE_PTHREAD)
    MI_MD_SetTime((pstVDFNode->u32New_t > pstVDFNode->u32Old_t) ?
                  (pstVDFNode->u32New_t - pstVDFNode->u32Old_t) :
                  (0xFFFFFFFF - pstVDFNode->u32Old_t + pstVDFNode->u32New_t));
#else
    MI_MD_SetTime(pstVDFNode->phandle,
                  (pstVDFNode->u32New_t > pstVDFNode->u32Old_t) ?
                  (pstVDFNode->u32New_t - pstVDFNode->u32Old_t) :
                  (0xFFFFFFFF - pstVDFNode->u32Old_t + pstVDFNode->u32New_t));
#endif

    pstVDFNode->u32Old_t = pstVDFNode->u32New_t;

    T0 = _MI_VDF_GetTimeInMs();
    pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
#if (MD_OD_SINGLE_PTHREAD)
    pstVDFNode->s32Ret = MI_MD_Run(VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);
#else
    pstVDFNode->s32Ret = MI_MD_Run(pstVDFNode->phandle, VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);
#endif
    T1 = _MI_VDF_GetTimeInMs();

    if(-1 == pstVDFNode->s32Ret)
    {
        printf("[MD] %s:%d ViSrc=%u Run MD error\n", __func__, __LINE__, u8ViSrcChn);
        pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
        return pstVDFNode->s32Ret;
    }
    else if(0 == pstVDFNode->s32Ret)
    {
        //LOG_DEBUG(DBG_MODULE_SAMPLE," thMD: No motion detected!!!\n");
        if(VDF_DBG_LOG_ENABLE)
            printf("[MD] %s:%d ViSrc=%u No motion detected\n", __func__, __LINE__, u8ViSrcChn);

        pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
        return pstVDFNode->s32Ret;
    }

    memset(&VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst, 0x00, sizeof(MI_MD_Result_t));
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u8WideDiv = pstVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv;
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u8HightDiv = pstVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv;
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64Pts  = VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts;


    //take care:
    //  1, The number of divisions of window in horizontal direction must smaller than 16(u16W_div<=16)
    //  2, The number of divisions of window in vertical direction must smaller than 12(u16H_div<=12)
    for(j = 0; j < pstVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv; j++)
    {
        for(i = 0; i < pstVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv; i++)
        {
            memset(&VDF_PTHREAD_MUTEX[u8ViSrcChn].param_out, 0x00, sizeof(MDParamsOut_t));
#if (MD_OD_SINGLE_PTHREAD)
            MI_MD_GetWindowParamsOut(i, j, &VDF_PTHREAD_MUTEX[u8ViSrcChn].param_out);
#else
            MI_MD_GetWindowParamsOut(pstVDFNode->phandle, i, j, &VDF_PTHREAD_MUTEX[u8ViSrcChn].param_out);
#endif

            //LOG_DEBUG(DBG_MODULE_SAMPLE,"thMD: OUT: [%d,%d]=(%u,%u)\n", i, j, param_out.md_result, param_out.obj_cnt);
            if(VDF_PTHREAD_MUTEX[u8ViSrcChn].param_out.md_result)
            {
                //printf("thMD: OUT: [%d,%d]=(%u,%u)\n", i, j, param_out.md_result, param_out.obj_cnt);
                VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[j] |= 1 << i;
            }
        }
    }

    pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));

#if (VDF_DBG_LOG_ENABLE)
    printf("[%s:%d] [VdfChn=%d, hdl=%p ViSrc=%d] pts=0x%llx (%d, %d) Get MD-Rst data:\n", __func__, __LINE__, \
           pstVDFNode->VdfChn,     \
           pstVDFNode->phandle,    \
           u8ViSrcChn,             \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64Pts,   \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u8WideDiv,  \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u8HightDiv);
    printf("0x%016llx  0x%016llx  0x%016llx  0x%016llx\n",
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[0], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[1], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[2], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[3]);
    printf("0x%016llx  0x%016llx  0x%016llx  0x%016llx\n",
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[4], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[5], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[6], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[7]);
    printf("0x%016llx  0x%016llx  0x%016llx  0x%016llx\n",
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[8], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[9], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[10], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst.u64MdResult[11]);
#endif

    pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));

    //add the MD detect result to the list
    if(NULL != MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList)
    {
        VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList = MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList;

        //find the MD handle from the Handle list
        while(NULL != VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList)
        {
            if((pstVDFNode->VdfChn == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->VdfChn) && \
               (pstVDFNode->phandle == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->pMdHandle))
            {
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstList = VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->pstMdResultList;

                //如果写指针大于buf长度：1)打印警告信息; 2)重新对写指针赋值
                if(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst > pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum)
                {
                    printf("[MD] %s:%d VdfChn=%u write overflow, D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", \
                           __func__, __LINE__, pstVDFNode->VdfChn,                     \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum);

                    VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst = \
                            VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst % pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum;
                }

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstList += VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst;

                //如果写指针对应buf不为空，打印警告信息并覆盖该数据
                if((1 == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstList->u8Enable) || \
                   (VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst == pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum))
                {
                    printf("[MD] %s:%d VdfChn=%u the MD detect data(pos=%u) will be covered\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst);
                    printf("[MD] %s:%d VdfChn=%u D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn,              \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum);

                }

                memcpy(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstList, &VDF_PTHREAD_MUTEX[u8ViSrcChn].stMdDetectRst, sizeof(MI_MD_Result_t));
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstList->u8Enable = 1;  //take care of this control bit, if not set APP will not get the result

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst++;
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst = \
                        VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst % pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum;

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData++;

                if(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData > pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum)
                    VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData = pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum;

                if(VDF_DBG_LOG_ENABLE)
                {
                    printf("[MD] %s:%d VdfChn=%u D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn,              \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum);
                }

                break; //break the while loop
            }

            VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList = (MI_VDF_MdRstHdlList_t*)VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList->next;
        }

        if(NULL == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstMdRstHdlList)
        {
            printf("[MD] %s:%d VdfChn=%u Cannot find the Result handle list\n", __func__, __LINE__, pstVDFNode->VdfChn);
        }
    }
    else
    {
        printf("[MD] %s:%d ViSrc=%u Get Result handle list error\n", __func__, __LINE__, u8ViSrcChn);
    }

    pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));

    MI_VDF_FUNC_EXIT();
    return 0;
}


inline MI_S32 _MI_VDF_OD_RunAndSetRst(MI_U8 u8ViSrcChn, MI_VDF_NodeList_t* pstVDFNode)
{
    MI_U8 i, j;

    MI_VDF_FUNC_ENTRY();

    //TODO: check if Width*Height*1.5=videoFrame.vFrame.bufLen
    pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
    pstVDFNode->s32Ret = MI_OD_Run(pstVDFNode->phandle, VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);

    if(-1 == pstVDFNode->s32Ret)
    {
        printf("[OD] %s:%d ViSrc=%d run OD error\n", __func__, __LINE__, u8ViSrcChn);
        pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
        return pstVDFNode->s32Ret;
    }
    else if(0 == pstVDFNode->s32Ret)
    {
        if(VDF_DBG_LOG_ENABLE)
            printf("[OD] %s:%d ViSrc=%u No OD detected!\n", __func__, __LINE__, u8ViSrcChn);

        pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
        return pstVDFNode->s32Ret;
    }

    //deal with the OD detect result
    memset(&VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst, 0x00, sizeof(MI_OD_Result_t));
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8WideDiv = pstVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u16WideDiv;
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8HightDiv = pstVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u16HightDiv;
    VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u64Pts  = VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts;


    //take care:
    //  1, The number of divisions of window in horizontal direction must smaller than 3(u16W_div<=3)
    //  2, The number of divisions of window in vertical direction must smaller than 3(u16H_div<=3)
    for(j = 0; j < pstVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u16HightDiv; j++)
    {
        for(i = 0; i < pstVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u16WideDiv; i++)
        {
            VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[j][i] = MI_OD_GetWindowResult(pstVDFNode->phandle, i, j);
            //VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[j][i] = ((stOdDetectRst.u64Pts>>(j*8))>>i)&0x01;
        }
    }

    pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));

#if (VDF_DBG_LOG_ENABLE)
    printf("[%s:%d] [VdfChn=%d, hdl=%p ViSrc=%d] pts=0x%llx  (%d, %d)] Get OD-Rst data:\n", __func__, __LINE__, \
           pstVDFNode->VdfChn,     \
           pstVDFNode->phandle,    \
           u8ViSrcChn,             \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u64Pts,   \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8WideDiv,  \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8HightDiv);
    printf("{(%d  %d  %d) (%d  %d  %d) (%d  %d  %d)}\n", \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[0][0], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[0][1], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[0][2], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[1][0], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[1][1], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[1][2], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[2][0], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[2][1], \
           VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst.u8RgnAlarm[2][2]);
#endif

    //add the OD detect result to the list
    pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));

    if(NULL != MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList)
    {
        VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList = MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList;

        //find the OD handle from the Handle list
        while(NULL != VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList)
        {
            if((pstVDFNode->VdfChn == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->VdfChn) && \
               (pstVDFNode->phandle == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->pOdHandle))
            {
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstList = VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->pstOdResultList;

                //如果写指针大于buf长度：1)打印警告信息; 2)重新对写指针赋值
                if(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst > pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum)
                {
                    printf("[OD] %s:%d VdfChn=%u OD write overflow, D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n",  \
                           __func__, __LINE__, pstVDFNode->VdfChn,                     \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum);
                    VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst = \
                            VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst % pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum;
                }

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstList += VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst;

                //如果写指针对应buf不为空，打印警告信息并覆盖该数据
                if((1 == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstList->u8Enable) || \
                   (VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst >= pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum))
                {
                    printf("[OD] %s:%d VdfChn=%u the OD detect data(pos=%u) will be covered\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst);
                    printf("[OD] %s:%d VdfChn=%u D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn,              \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum);
                }

                memcpy(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstList, &VDF_PTHREAD_MUTEX[u8ViSrcChn].stOdDetectRst, sizeof(MI_OD_Result_t));
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstList->u8Enable = 1; //take care of this control bit, if not set APP will not get the result

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst++;
                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst = \
                        VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst % pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum;

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData++;

                if(VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData > pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum)
                    VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData = pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum;

                if(VDF_DBG_LOG_ENABLE)
                {
                    printf("[OD] %s:%d VdfChn=%u D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__, \
                           pstVDFNode->VdfChn,              \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->s8DeltData,  \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16WritePst, \
                           VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->u16ReadPst,  \
                           pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum);
                }

                break; //break the while loop
            }

            VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList = (MI_VDF_OdRstHdlList_t*)VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList->next;
        }

        if(NULL == VDF_PTHREAD_MUTEX[u8ViSrcChn].pstOdRstHdlList)
        {
            printf("[OD] %s:%d VdfChn=%u Cannot find the OD Result handle list\n", __func__, __LINE__, pstVDFNode->VdfChn);
        }
    }
    else
    {
        printf("[OD] %s:%d ViSrc=%d Get OD Result handle list error\n", __func__, __LINE__, u8ViSrcChn);
    }

    pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));

    MI_VDF_FUNC_EXIT();
    return 0;
}


void* vdf_YUV_task(void *argu)
{
    MI_S32 ret = 0;
    MI_U8  u8ViSrcChn = 0;

    MI_VDF_FUNC_ENTRY();

#if (VDF_SAVE_IMAGE_ON_I3)
    FILE *pfile = NULL;
    pfile = fopen("sample_vi.raw", "wb");

    if(NULL == pfile)
        printf("[OD] %s:%d ViSrc=%d  --->Open sample_vi.raw fail\n", __func__, __LINE__, u8ViSrcChn);

#endif

    printf("[VDF] %s:%d, VDF_YUV_TASK+\n", __func__, __LINE__);

    while(FALSE == g_VdfYuvTaskExit)
    {
        ret = pthread_mutex_trylock(&mutexVDF);

        if(ret == EBUSY)
        {
            //LOG_DEBUG(DBG_MODULE_SAMPLE," th0: locked by other th\n");
            //printf("[VDF] %s:%d try to get phtread mutex-mutexVDF, ret=EBUSY\n",__func__,__LINE__);
            usleep(10 * 1000);
            continue;
        }
        else if(ret != 0)
        {
            //LOG_ERROR(DBG_MODULE_SAMPLE, " th0: pthread_mutex_trylock\n");
            //printf("[VDF] %s:%d try to get phtread mutex-mutexVDF, ret1=0\n",__func__,__LINE__);
            usleep(10 * 1000);
            continue;
        }
        else
        {
            pthread_mutex_unlock(&mutexVDF);;
        }


        for(u8ViSrcChn = 0; u8ViSrcChn < 4; u8ViSrcChn++)
            //for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
        {
#if VDF_TEST_ON_I3
            VideoFrameInfo_t videoFrame;
            VI_CHN chn = 5;

            if(MI_RET_SUCCESS != MI_VI_GetFrame(chn, &videoFrame))
            {
                printf("[VDF] %s:%d MI_VI_GetFrame(Chn=%u) fail\n", __func__, __LINE__, u8ViSrcChn);
                continue;
            }

            //printf("\n[VDF] %s:%d MI_VI_GetFrame(Chn=%u, frm_pts=0x%llx)\n",__func__,__LINE__, u8ViSrcChn,videoFrame.vFrame.pts);
#else
            MI_U16 u32PortId = 0;
            MI_SYS_BufInfo_t stInputBufInfo;
            MI_SYS_BUF_HANDLE hInputBufHandle = MI_HANDLE_NULL;

            // 1st, get the video src channel image
            if(MI_SUCCESS != MI_SHADOW_WaitOnInputTaskAvailable(_hVDFDev, 1000))
            {
                printf("SHADOW wait buffer time out\n");
                continue;
            }

            memset(&stInputBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));

            //if (MI_SUCCESS != MI_SHADOW_GetInputPortBuf(_hVDFDev, u8ViSrcChn, u32PortId, &stInputBufInfo, &hInputBufHandle))
            if(MI_SUCCESS != MI_SHADOW_GetInputPortBuf(_hVDFDev, 0, u32PortId, &stInputBufInfo, &hInputBufHandle))
            {
                printf("get buffer failed\n");
                continue;
            }

#endif
            ret = pthread_mutex_trylock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));

            if(EBUSY == ret)
            {
                printf("[VDF_YUV_TASK] ViSrc(%u) busy\n", u8ViSrcChn);
            }
            else if(0 != ret)
            {
            }
            else
            {
#if VDF_TEST_ON_I3
                //if(NULL != VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer)
                {
                    memcpy(VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer, \
                    (char*)(videoFrame.vFrame.bufAddr), \
                    g_width * g_height);
                    VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts = videoFrame.vFrame.pts;

#if (VDF_SAVE_IMAGE_ON_I3)
                    //write video frame to file
                    fwrite(videoFrame.vFrame.bufAddr, 1,  videoFrame.vFrame.bufLen, pfile);
#endif
                }
#else
                /*save_yuv422_data(stInputBufInfo.stFrameData.pVirAddr[0], stInputBufInfo.stFrameData.u16Width, stInputBufInfo.stFrameData.u16Height, stInputBufInfo.stFrameData.u32Stride[0], u32ChnId);
                if (TRUE)
                {
                    MI_SHADOW_FinishBuf(_hVDFDev, hInputBufHandle);
                }
                else
                {
                    MI_SHADOW_RewindBuf(_hVDFDev, hInputBufHandle);
                }*/

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer = stInputBufInfo.stFrameData.pVirAddr[0];
                VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts = stInputBufInfo.u64Pts;
                VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size = stInputBufInfo.stFrameData.u16Width * stInputBufInfo.stFrameData.u16Height;
                g_width = stInputBufInfo.stFrameData.u16Width;
                g_height = stInputBufInfo.stFrameData.u16Height;
                VDF_PTHREAD_MUTEX[u8ViSrcChn].hInputBufHandle = hInputBufHandle;
#endif
                pthread_cond_signal(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].condVideoSrcChnY));
                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));

                //              printf("[VDF_YUV_TASK] ViSrc=%u, pts=0x%llx pthread send mutexVideoSrcChnY signal done ...\n",\
                //                        u8ViSrcChn, VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts);
            }

#if VDF_TEST_ON_I3
            MI_VI_ReleaseFrame(chn, &videoFrame);
#else
            //            MI_SHADOW_FinishBuf(_hVDFDev, hInputBufHandle);  //discard Output port data
#endif
        }
    }

#if (VDF_SAVE_IMAGE_ON_I3)

    if(NULL != pfile)
    {
        fclose(pfile);
    }

#endif

    printf("[VDF] %s:%d, VDF_YUV_TASK-\n", __func__, __LINE__);

    MI_VDF_FUNC_EXIT();
    return NULL;
}


void* vdf_APP_task(void *argu)
{
    MI_U8 u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;

    MI_VDF_FUNC_ENTRY();

    u8ViSrcChn = (MI_U8)argu;

    if((u8ViSrcChn < 0) && (u8ViSrcChn > MI_VDF_CHANNEL_NUM_MAX))
    {
        printf("[VDF] %s:%d, Get the ViSrc(%d) is out of range\n", __func__, __LINE__, u8ViSrcChn);
        return NULL;
    }

    if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList))
    {
        printf("[VDF] %s:%d, ViSrc=%u Get the VDF Node list is NULL\n", __func__, __LINE__, u8ViSrcChn);
        return NULL;
    }

    printf("[VDF] %s:%d, ViSrc=%u, argu=%u VDF_APP_TASK+\n", __func__, __LINE__, u8ViSrcChn, (MI_U32)argu);

    while(FALSE == g_VdfAPPTaskExit[u8ViSrcChn])
    {
        if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList))
        {
            printf("[VDF] %s:%d, Get the VDF Node list(%d) is NULL\n", __func__, __LINE__, u8ViSrcChn);
            usleep(10 * 1000);
        }

        pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));
        pthread_cond_wait(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].condVideoSrcChnY), \
                          & (VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));

        if(TRUE == g_VdfAPPTaskExit[u8ViSrcChn])
        {
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));
            break; //break the 1st while loop
        }


        while(NULL != pstTmpVDFNode)
        {
            pstTmpVDFNode->u8FrameCnt++;

            if((0 == pstTmpVDFNode->u8FrameInterval) || \
               (0 == (pstTmpVDFNode->u8FrameCnt % pstTmpVDFNode->u8FrameInterval)))
            {
                pstTmpVDFNode->u8FrameCnt = 0;

                if(g_MDEnable && (E_MI_VDF_WORK_MODE_MD == pstTmpVDFNode->stAttr.enWorkMode) && \
                   (u8ViSrcChn == pstTmpVDFNode->stAttr.unAttr.stMdAttr.u8SrcChnNum))
                {
                    //printf("[VDF] %s:%d, start do MD Calculate(Chn=%u, pts=0x%llx) ...\n",__func__,__LINE__, u8ViSrcChn,VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts);
                    _MI_VDF_MD_RunAndSetRst(u8ViSrcChn, pstTmpVDFNode);
                }

                if(g_ODEnable && (E_MI_VDF_WORK_MODE_OD == pstTmpVDFNode->stAttr.enWorkMode) && \
                   (u8ViSrcChn == pstTmpVDFNode->stAttr.unAttr.stOdAttr.u8SrcChnNum))
                {
                    //printf("[VDF] %s:%d, start do OD Calculate(Chn=%u, pts=0x%llx) ...\n",__func__,__LINE__, u8ViSrcChn,VDF_PTHREAD_MUTEX[u8ViSrcChn].u64Pts);
                    _MI_VDF_OD_RunAndSetRst(u8ViSrcChn, pstTmpVDFNode);
                }
            }

            pstTmpVDFNode = (MI_VDF_NodeList_t*)pstTmpVDFNode->next;
        }

#if !(VDF_TEST_ON_I3)
        MI_SHADOW_FinishBuf(_hVDFDev, VDF_PTHREAD_MUTEX[u8ViSrcChn].hInputBufHandle);  //discard Output port data
#endif
        pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));
    }

    printf("[VDF] %s:%d, ViSrc=%u, argu=%u VDF_APP_TASK-\n", __func__, __LINE__, u8ViSrcChn, (MI_U32)argu);

    MI_VDF_FUNC_EXIT();
    return NULL;
}


MI_S32 _MI_VDF_CheckChnValid(MI_VDF_CHANNEL VdfChn)
{
    //MI_S32 ret = 0;
    MI_U8 u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;

    MI_VDF_FUNC_ENTRY();

    if((VdfChn < 0) || (VdfChn >= MI_VDF_CHANNEL_MAX))
    {
        printf("[VDF] %s:%d, The input VdfChn is Out of Range\n", __func__, __LINE__);
        return -1;
    }

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList))
        {
            continue;
        }

        do
        {
            if(VdfChn == pstTmpVDFNode->VdfChn)
            {
                MI_VDF_FUNC_EXIT();
                return 0;
            }

            pstTmpVDFNode = pstTmpVDFNode->next;

        }
        while(NULL != pstTmpVDFNode);
    }

    return -1;
}


MI_S32 _MI_VDF_AddVDFNode(const MI_VDF_ChnAttr_t* pstAttr, MI_VDF_CHANNEL VdfChn, void* phandle)
{
    MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;
    MI_VDF_NodeList_t* pstNewVDFNode = NULL;

    MI_VDF_FUNC_ENTRY();

    if((NULL == pstAttr) || (NULL == phandle))
    {
        printf("[VDF] %s:%d, The input Pointer is NULL\n", __func__, __LINE__);
        return -1;
    }

    enWorkMode = pstAttr->enWorkMode;

    if((E_MI_VDF_WORK_MODE_MD != enWorkMode) && (E_MI_VDF_WORK_MODE_OD != enWorkMode))
    {
        printf("[VDF] %s:%d, Get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
        return -1;
    }

    pstNewVDFNode = (MI_VDF_NodeList_t*)malloc(sizeof(MI_VDF_NodeList_t));

    if(NULL == pstNewVDFNode)
    {
        printf("[VDF] %s:%d, malloc buffer for New VDF NODE error\n", __func__, __LINE__);
        return -1;
    }

    memset(pstNewVDFNode, 0x00, sizeof(MI_VDF_NodeList_t));
    pstNewVDFNode->phandle = phandle;
    pstNewVDFNode->VdfChn = VdfChn;
    memcpy(&pstNewVDFNode->stAttr, pstAttr, sizeof(MI_VDF_ChnAttr_t));
    pstNewVDFNode->next = NULL;

    if(E_MI_VDF_WORK_MODE_MD == enWorkMode)
        pstNewVDFNode->u8FrameInterval = pstAttr->unAttr.stMdAttr.u32VDFIntvl;
    else if(E_MI_VDF_WORK_MODE_OD == enWorkMode)
        pstNewVDFNode->u8FrameInterval = pstAttr->unAttr.stOdAttr.u32VDFIntvl;

    // add the new node to the tail of VDF Node list
    pthread_mutex_lock(&mutex_VDF_cfg);

    if(E_MI_VDF_WORK_MODE_MD == enWorkMode)
    {
        if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[pstAttr->unAttr.stMdAttr.u8SrcChnNum].pstVdfNodeList))
        {
            MI_VDF_ENTRY_MODE_LIST[pstAttr->unAttr.stMdAttr.u8SrcChnNum].pstVdfNodeList = pstNewVDFNode;
            //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstNewVDFNode=%p\n",__func__,__LINE__,VdfChn, (MI_U32*)phandle, pstNewVDFNode);
            pthread_mutex_unlock(&mutex_VDF_cfg);
            MI_VDF_FUNC_EXIT();
            return 0;
        }
    }
    else if(E_MI_VDF_WORK_MODE_OD == enWorkMode)
    {
        if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[pstAttr->unAttr.stOdAttr.u8SrcChnNum].pstVdfNodeList))
        {
            MI_VDF_ENTRY_MODE_LIST[pstAttr->unAttr.stOdAttr.u8SrcChnNum].pstVdfNodeList = pstNewVDFNode;
            //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstNewVDFNode=%p\n",__func__,__LINE__,VdfChn, (MI_U32*)phandle, pstNewVDFNode);
            pthread_mutex_unlock(&mutex_VDF_cfg);
            MI_VDF_FUNC_EXIT();
            return 0;
        }
    }

    while(NULL != pstTmpVDFNode)
    {
        //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstTmpVDFNode=%p\n",__func__,__LINE__,pstTmpVDFNode->VdfChn, pstTmpVDFNode->phandle, pstTmpVDFNode);
        if(NULL == pstTmpVDFNode->next)
        {
            pstTmpVDFNode->next = pstNewVDFNode;
            break;
        }

        pstTmpVDFNode = (MI_VDF_NodeList_t*)pstTmpVDFNode->next;
    }

    //pstTmpVDFNode->next = pstNewVDFNode;
    //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstNewVDFNode=%p\n",__func__,__LINE__,pstNewVDFNode->VdfChn, pstNewVDFNode->phandle, pstNewVDFNode);

    pthread_mutex_unlock(&mutex_VDF_cfg);

    MI_VDF_FUNC_EXIT();
    return 0;
}


MI_S32 _MI_VDF_GetVDFNode(MI_VDF_CHANNEL VdfChn, MI_VDF_NodeList_t** pstVDFNode)
{
    //MI_S32 ret = 0;
    MI_U8 u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;

    MI_VDF_FUNC_ENTRY();


    if(NULL == pstVDFNode)
    {
        printf("[VDF] %s:%d, The input Pointer is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_CheckChnValid(VdfChn))
    {
        printf("[VDF] %s:%d, check VdfChn(%d) error\n", __func__, __LINE__, VdfChn);
        return -1;
    }

    pthread_mutex_lock(&mutex_VDF_cfg);

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        if(NULL == (pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList))
        {
            continue;
        }

        do
        {
            if(VdfChn == pstTmpVDFNode->VdfChn)
            {
                *pstVDFNode = pstTmpVDFNode;
                //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstTmpVDFNode=%p\n",__func__,__LINE__,pstTmpVDFNode->VdfChn, pstTmpVDFNode->phandle, pstTmpVDFNode);
                pthread_mutex_unlock(&mutex_VDF_cfg);
                MI_VDF_FUNC_EXIT();
                return 0;
            }

            //printf("[VDF] %s:%d VdfChn=%d, phandle=0x%x pstTmpVDFNode=%p\n",__func__,__LINE__,pstTmpVDFNode->VdfChn, pstTmpVDFNode->phandle, pstTmpVDFNode);

            pstTmpVDFNode = pstTmpVDFNode->next;

        }
        while(NULL != pstTmpVDFNode);
    }

    *pstVDFNode = NULL;
    pthread_mutex_unlock(&mutex_VDF_cfg);

    return -1;
}


MI_S32 _MI_VDF_DelVDFNode(MI_VDF_CHANNEL VdfChn)
{
    MI_U8 u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pVdfNode_tmp = NULL;
    MI_VDF_NodeList_t* pVdfNode_pro = NULL;

    MI_VDF_FUNC_ENTRY();

    if(0 != _MI_VDF_CheckChnValid(VdfChn))
    {
        printf("[VDF] %s:%d, The input VdfChn(%d) is invalid\n", __func__, __LINE__, VdfChn);
        return -1;
    }

    pthread_mutex_lock(&mutex_VDF_cfg);

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        if(NULL == (pVdfNode_tmp = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList))
        {
            continue;
        }

        // check the 1st node
        if(VdfChn == MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList->VdfChn)
        {
            MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList = pVdfNode_tmp->next;
            free(pVdfNode_tmp);
            pthread_mutex_unlock(&mutex_VDF_cfg);
            MI_VDF_FUNC_EXIT();
            return 0;
        }

        pVdfNode_pro = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList;
        pVdfNode_tmp = pVdfNode_pro->next;

        while(NULL != pVdfNode_tmp)
        {
            if(VdfChn == pVdfNode_tmp->VdfChn)
            {
                pVdfNode_pro = pVdfNode_tmp->next;
                free(pVdfNode_tmp);
                pthread_mutex_unlock(&mutex_VDF_cfg);
                MI_VDF_FUNC_EXIT();
                return 0;
            }

            pVdfNode_pro = pVdfNode_tmp;
            pVdfNode_tmp = pVdfNode_tmp->next;
        }
    }

    pthread_mutex_unlock(&mutex_VDF_cfg);

    return -1;
}


MI_S32 MI_VDF_Init(void)
{
    MI_S32 ret = 0;
    MI_U8 u8ViSrcChn = 0;

    MI_VDF_FUNC_ENTRY();


    g_MDEnable = 0;
    g_ODEnable = 0;
    g_VdfYuvTaskExit = FALSE;

    memset(MI_VDF_MD_RST_LIST, 0x00, sizeof(MI_VDF_MD_RST_LIST));
    memset(MI_VDF_OD_RST_LIST, 0x00, sizeof(MI_VDF_OD_RST_LIST));
    memset(MI_VDF_ENTRY_MODE_LIST, 0x00, sizeof(MI_VDF_ENTRY_MODE_LIST));

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        g_VdfAPPTaskExit[u8ViSrcChn] = FALSE;
        pthread_mutex_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW), NULL);
        pthread_mutex_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW), NULL);
        pthread_mutex_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN), NULL);
        pthread_mutex_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVDFAPP), NULL);
        pthread_mutex_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY), NULL);
        pthread_cond_init(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].condVideoSrcChnY), NULL);
    }


    printf("===== This libmi_vdf.so is only run on K6L platform [");
    printf(__DATE__);
    printf("  ");
    printf(__TIME__);
    printf("] =====\n");

#if (VDF_TEST_ON_I3)

    for(u8ViSrcChn = 0; u8ViSrcChn < 4; u8ViSrcChn++)
    {
        VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer = malloc(g_width * g_height);
    }

#else
    //用户调用下面接口函数设置shadow buff缓存帧大小
    //MI_SYS_SetChnOutputPortDepth(MI_SYS_ChnPort_t *pstChnPort , MI_U32 u32UserFrameDepth , MI_U32 u32BufQueueDepth)
    memset(&_stVDFRegDevInfo, 0x00, sizeof(MI_SHADOW_RegisterDevParams_t));
    _stVDFRegDevInfo.stModDevInfo.eModuleId = E_MI_MODULE_ID_VDF;
    _stVDFRegDevInfo.stModDevInfo.u32DevId = 0;
    _stVDFRegDevInfo.stModDevInfo.u32DevChnNum = 16;
    _stVDFRegDevInfo.stModDevInfo.u32InputPortNum = 1;
    _stVDFRegDevInfo.stModDevInfo.u32OutputPortNum = 0;
    _stVDFRegDevInfo.OnBindInputPort = NULL; //_MI_VDF_OnBindInputPort;
    _stVDFRegDevInfo.OnBindOutputPort = NULL; //_MI_VDF_OnBindOutputPort;
    _stVDFRegDevInfo.OnUnBindInputPort = NULL; //_MI_VDF_OnUnBindInputPort;
    _stVDFRegDevInfo.OnUnBindOutputPort = NULL; //_MI_VDF_OnUnBindOutputPort;

    if(MI_SUCCESS != (ret = MI_SHADOW_RegisterDev(&_stVDFRegDevInfo, &_hVDFDev)))
    {
        printf("[VDF] %s:%d, call MI_SHADOW_RegisterDev() fail!\n", __func__, __LINE__);
        return -1;
    }

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        MI_SYS_ChnPort_t stOutputChnPort;
        stOutputChnPort.eModId = E_MI_MODULE_ID_VPE;
        stOutputChnPort.u32DevId = 0;
        stOutputChnPort.u32ChnId = u8ViSrcChn;
        stOutputChnPort.u32PortId = 3;

        MI_SHADOW_EnableChannel(_hVDFDev, u8ViSrcChn);
        MI_SHADOW_EnableInputPort(_hVDFDev, u8ViSrcChn, 0);
        MI_SYS_ChnPort_t stInputChnPort;
        stInputChnPort.eModId = E_MI_MODULE_ID_VDF;
        stInputChnPort.u32DevId = 0;
        stInputChnPort.u32ChnId = u8ViSrcChn;
        stInputChnPort.u32PortId = 0;

        //MI_SYS_BindChnPort(&stOutputChnPort, &stInputChnPort, 30, 30);
    }

#endif

    printf("[VDF] %s:%d, Start VDF_YUV_Task\n", __func__, __LINE__);
    pthread_create(&pthreadVDFYUV, NULL, vdf_YUV_task, NULL);
    pthread_setname_np(pthreadVDFYUV, "VDF_YUV_Task");

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_Uninit(void)
{
    MI_S32 ret = 0;
    MI_U8 u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;
    MI_VDF_NodeList_t* pstTmpVDFNode_next = NULL;
    //MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();


    //1st: stop vdf_YUV_task & vdf_APP_task
    while(FALSE == g_VdfYuvTaskExit)
    {
        pthread_mutex_lock(&mutexVDF);
        pthread_cond_signal(&condVDF);
        pthread_mutex_unlock(&mutexVDF);

        printf("[VDF] %s:%d, wait for TD exit\n", __func__, __LINE__);

        usleep(10 * 1000);
    }

    pthread_join(pthreadVDFYUV, NULL);

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        while(FALSE == g_VdfAPPTaskExit[u8ViSrcChn])
        {
            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVDFAPP));
            pthread_cond_signal(&VDF_PTHREAD_MUTEX[u8ViSrcChn].condVideoSrcChnY);
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVDFAPP));

            g_VdfAPPTaskExit[u8ViSrcChn] = TRUE;
            usleep(10 * 1000);
        }

        pthread_join(pthreadVDFAPP[u8ViSrcChn], NULL);

        free(VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);
        VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer = NULL;
    }

    //2nd: de-init and free the MD/OD node
    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        pstTmpVDFNode = MI_VDF_ENTRY_MODE_LIST[u8ViSrcChn].pstVdfNodeList;

        while(NULL != pstTmpVDFNode)
        {
            if((NULL != (pstTmpVDFNode->phandle)) && \
               (E_MI_VDF_WORK_MODE_MD == pstTmpVDFNode->stAttr.enWorkMode))
            {
                MI_MD_Uninit(pstTmpVDFNode->phandle);
            }

            if((NULL != (pstTmpVDFNode->phandle)) && \
               (E_MI_VDF_WORK_MODE_OD == pstTmpVDFNode->stAttr.enWorkMode))
            {
                MI_OD_Uninit(pstTmpVDFNode->phandle);
            }

            pstTmpVDFNode_next = pstTmpVDFNode->next;
            free(pstTmpVDFNode);
            pstTmpVDFNode = pstTmpVDFNode_next;
        }
    }

    //3st: free the MD/OD resault buffer
    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        if(NULL != MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList)
        {
            MI_VDF_MdRstHdlList_t* pstMdRstHandleList = NULL;
            MI_VDF_MdRstHdlList_t* pstMdRstHandleList_next = NULL;

            pstMdRstHandleList = MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList;

            while(NULL != pstMdRstHandleList)
            {
                pstMdRstHandleList_next = pstMdRstHandleList->next;
                free(pstMdRstHandleList->pstMdResultList);
                free(pstMdRstHandleList);
                pstMdRstHandleList = pstMdRstHandleList_next;
            }

            MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList = NULL;
        }

        if(NULL != MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList)
        {
            MI_VDF_OdRstHdlList_t* pstOdRstHandleList = NULL;
            MI_VDF_OdRstHdlList_t* pstOdRstHandleList_next = NULL;

            pstOdRstHandleList = MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList;

            while(NULL != pstOdRstHandleList)
            {
                pstOdRstHandleList_next = pstOdRstHandleList->next;
                free(pstOdRstHandleList->pstOdResultList);
                free(pstOdRstHandleList);
                pstOdRstHandleList = pstOdRstHandleList_next;
            }

            MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList = NULL;
        }
    }

    memset(MI_VDF_MD_RST_LIST, 0x00, sizeof(MI_VDF_MD_RST_LIST));
    memset(MI_VDF_MD_RST_LIST, 0x00, sizeof(MI_VDF_OD_RST_LIST));

    for(u8ViSrcChn = 0; u8ViSrcChn < MI_VDF_CHANNEL_NUM_MAX; u8ViSrcChn++)
    {
        g_VdfAPPTaskExit[u8ViSrcChn] = TRUE;
        pthread_mutex_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));
        pthread_mutex_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));
        pthread_mutex_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
        pthread_mutex_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVDFAPP));
        pthread_mutex_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexVideoSrcChnY));
        pthread_cond_destroy(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].condVideoSrcChnY));
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_CreateChn(MI_VDF_CHANNEL VdfChn, const MI_VDF_ChnAttr_t* pstAttr)
{
    MI_S32 ret = 0;
    MI_U8  u8ViSrcChn = 0;
    MI_U8  u8RstBufNum = 0;
    MD_HANDLE pMDHandle = NULL;
    OD_HANDLE pODHandle = NULL;
    MI_VDF_MdRstHdlList_t* pstMdRstHdlListTmp = NULL;
    MI_VDF_MdRstHdlList_t* pstMdRstHdlListNew = NULL;
    MI_VDF_OdRstHdlList_t* pstOdRstHdlListTmp = NULL;
    MI_VDF_OdRstHdlList_t* pstOdRstHdlListNew = NULL;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstAttr)
    {
        printf("[VDF] %s:%d, Input parameter is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 == _MI_VDF_CheckChnValid(VdfChn))
    {
        printf("[VDF] %s:%d, The input VdfChn(%d) has been defined already\n", __func__, __LINE__, VdfChn);
        return -1;
    }

    switch(pstAttr->enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            u8ViSrcChn = pstAttr->unAttr.stMdAttr.u8SrcChnNum;
            u8RstBufNum = pstAttr->unAttr.stMdAttr.u32MdBufNum;

            // prepare the VDF-MD result list
            if((u8ViSrcChn < 0) || (u8ViSrcChn >= MI_VDF_CHANNEL_NUM_MAX))
            {
                printf("[VDF] %s:%d, The input Vi src Chn(%d) is out of range\n", __func__, __LINE__, u8ViSrcChn);
                return -1;
            }

            if((u8RstBufNum < 0) || (u8RstBufNum > MI_VDF_MD_RST_BUF_NUM_MAX))
            {
                printf("[VDF] %s:%d, The input MD buff Num(%d) is out of range\n", __func__, __LINE__, u8RstBufNum);
                return -1;
            }

            pthread_mutex_lock(&mutex_VDF_cfg);
            pMDHandle = MI_MD_Init(pstAttr->unAttr.stMdAttr.stMDObjCfg.u16ImgW,  \
                                   pstAttr->unAttr.stMdAttr.stMDObjCfg.u16ImgH,  \
                                   (MI_U8) pstAttr->unAttr.stMdAttr.enClrType,   \
                                   (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16WideDiv,  \
                                   (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16HightDiv);
            pthread_mutex_unlock(&mutex_VDF_cfg);

            if(NULL == pMDHandle)
            {
                printf("[VDF] %s:%d, Get MD handle error\n", __func__, __LINE__);
                return -1;
            }
            else
            {
                // start point (0, 0), end point (RAW_W-1, RAH_H-1)
                pthread_mutex_lock(&mutex_VDF_cfg);
#if (MD_OD_SINGLE_PTHREAD)
                ret = MI_MD_SetDetectWindow(pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtX,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtY,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16RbX,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16RbY,   \
                                            (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16WideDiv,  \
                                            (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16HightDiv);
#else
                ret = MI_MD_SetDetectWindow(pMDHandle,                                     \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtX,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtY,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16RbX,   \
                                            pstAttr->unAttr.stMdAttr.stMDObjCfg.u16RbY,   \
                                            (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16WideDiv,  \
                                            (MI_U8) pstAttr->unAttr.stMdAttr.stMDRgnSet.u16HightDiv);
#endif
                pthread_mutex_unlock(&mutex_VDF_cfg);

                if(MI_RET_SUCESS != ret)
                {
                    printf("[VDF] %s:%d, Set MD Detect Window error\n", __func__, __LINE__);
                    MI_MD_Uninit(pMDHandle);
                    return -1;
                }

                // add the MD Entry in VDF Node list
                if(0 != _MI_VDF_AddVDFNode(pstAttr, VdfChn, pMDHandle))
                {
                    printf("[VDF] %s:%d, Add MD handle to list error\n", __func__, __LINE__);
                    MI_MD_Uninit(pMDHandle);
                    return -1;
                }
            }


            //Malloc buffer for MD Result Handle list
            pstMdRstHdlListNew = (MI_VDF_MdRstHdlList_t*)malloc(sizeof(MI_VDF_MdRstHdlList_t));

            if(NULL == pstMdRstHdlListNew)
            {
                MI_MD_Uninit(pMDHandle);
                _MI_VDF_DelVDFNode(VdfChn);
                printf("[VDF] %s:%d, Malloc buffer for MD Result Handle list struct error\n", __func__, __LINE__);
                return -1;
            }

            memset(pstMdRstHdlListNew, 0x00, sizeof(MI_VDF_MdRstHdlList_t));
            pstMdRstHdlListNew->pMdHandle = pMDHandle;
            pstMdRstHdlListNew->VdfChn = VdfChn;
            pstMdRstHdlListNew->next = NULL;

            //Malloc buffer for MD Result
            pstMdRstHdlListNew->pstMdResultList = (MI_MD_Result_t*)malloc(sizeof(MI_MD_Result_t) * u8RstBufNum);

            if(NULL == pstMdRstHdlListNew->pstMdResultList)
            {
                printf("[VDF] %s:%d, Malloc buffer for MD result error\n", __func__, __LINE__);
                free(pstMdRstHdlListNew);
                pstMdRstHdlListNew = NULL;
                MI_MD_Uninit(pMDHandle);
                _MI_VDF_DelVDFNode(VdfChn);
                return -1;
            }

            memset(pstMdRstHdlListNew->pstMdResultList, 0x00, sizeof(MI_MD_Result_t) * u8RstBufNum);

            pthread_mutex_lock(&mutex_VDF_cfg);

            //          printf("[VDF] %s:%d, MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList=%p\n",__func__,__LINE__, \
            //                               MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList);
            //          printf("[VDF] %s:%d, VdfChn=%d, pMDHandle=0x%x, pstMdRstHdlListNew=%p\n",__func__,__LINE__,\
            //                      pstMdRstHdlListNew->VdfChn, pstMdRstHdlListNew->pMdHandle, pstMdRstHdlListNew);
            if(NULL == MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList)
            {
                MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList = pstMdRstHdlListNew;
            }
            else
            {
                pstMdRstHdlListTmp = MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList;

                while(NULL != pstMdRstHdlListTmp)
                {
                    //                  printf("[VDF] %s:%d, VdfChn=%d, pMDHandle=0x%x, pstMdRstHdlListTmp=%p\n",__func__,__LINE__,\
                    //                      pstMdRstHdlListTmp->VdfChn, pstMdRstHdlListTmp->pMdHandle, pstMdRstHdlListTmp);
                    pstMdRstHdlListTmp = (MI_VDF_MdRstHdlList_t*)pstMdRstHdlListTmp->next;
                }

                // add new buf to the tail of the list
                pstMdRstHdlListTmp = pstMdRstHdlListNew;
            }

            pthread_mutex_unlock(&mutex_VDF_cfg);

            //          printf("[VDF] %s:%d, MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList=%p\n",__func__,__LINE__, \
            //                               MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList);
            printf("[VDF] %s:%d, Create VDF Node: Chn=%d, md_hdl=%p MdRstHdl=%p\n", __func__, __LINE__, u8ViSrcChn, pMDHandle, pstMdRstHdlListNew);

            //start the APP thread of the video source channel
            if(0 == VDF_PTHREAD_MUTEX[u8ViSrcChn].u8Enable)
            {
                MI_U32 argu = 0;
                MI_S8 cmdbuff[32];

                VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size = pstAttr->unAttr.stMdAttr.stMDObjCfg.u16ImgW * \
                        pstAttr->unAttr.stMdAttr.stMDObjCfg.u16ImgH;

                if(NULL != VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer)
                    free(VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer = (unsigned char *)malloc(VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size);

                if(NULL == VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer)
                {
                    printf("Video src chn(%d) malloc %d faild\n", u8ViSrcChn, VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size);
                    free(pstMdRstHdlListTmp->pstMdResultList);
                    free(pstMdRstHdlListTmp);
                    MI_MD_Uninit(pMDHandle);
                    _MI_VDF_DelVDFNode(VdfChn);
                    return -1;
                }

                memset(cmdbuff, 0x00, sizeof(cmdbuff));
                snprintf(cmdbuff, sizeof(cmdbuff), "vdf_APP_task_Chn%u", u8ViSrcChn);

                argu = u8ViSrcChn;

                printf("[VDF] %s:%d, start %s(%u, %u)\n", __func__, __LINE__, cmdbuff, argu, u8ViSrcChn);
                pthread_create(&(pthreadVDFAPP[u8ViSrcChn]), NULL, vdf_APP_task, (void *)argu);
                pthread_setname_np(pthreadVDFAPP[u8ViSrcChn] , cmdbuff);

                VDF_PTHREAD_MUTEX[u8ViSrcChn].u8Enable = 1;
            }

            break;

        case E_MI_VDF_WORK_MODE_OD:
            u8ViSrcChn = pstAttr->unAttr.stOdAttr.u8SrcChnNum;
            u8RstBufNum = pstAttr->unAttr.stOdAttr.u32OdBufNum;

            if((u8ViSrcChn < 0) || (u8ViSrcChn >= MI_VDF_CHANNEL_NUM_MAX))
            {
                printf("[VDF] %s:%d, The input Source Chn(%d) is out of range\n", __func__, __LINE__, u8ViSrcChn);
                return -1;
            }

            if((u8RstBufNum < 0) || (u8RstBufNum > MI_VDF_OD_RST_BUF_NUM_MAX))
            {
                printf("[VDF] %s:%d, The input OD buff Num(%d) is out of range\n", __func__, __LINE__, u8RstBufNum);
                return -1;
            }

            pthread_mutex_lock(&mutex_VDF_cfg);
            pODHandle = MI_OD_Init(pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgW,  \
                                   pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgH,  \
                                   (ODColor_e)pstAttr->unAttr.stOdAttr.enClrType, \
                                   pstAttr->unAttr.stOdAttr.stODParamsIn.endiv);
            pthread_mutex_unlock(&mutex_VDF_cfg);

            if(NULL == pODHandle)
            {
                printf("[VDF] %s:%d, Get OD handle error\n", __func__, __LINE__);
                return -1;
            }

            pthread_mutex_lock(&mutex_VDF_cfg);
            ret = MI_OD_SetDetectWindow(pODHandle, \
                                        pstAttr->unAttr.stOdAttr.stODObjCfg.u16LtX, \
                                        pstAttr->unAttr.stOdAttr.stODObjCfg.u16LtY, \
                                        pstAttr->unAttr.stOdAttr.stODObjCfg.u16RbX, \
                                        pstAttr->unAttr.stOdAttr.stODObjCfg.u16RbY);
            pthread_mutex_unlock(&mutex_VDF_cfg);

            if(MI_RET_SUCESS != ret)
            {
                printf("[VDF] %s:%d, Set OD Detect Window error\n", __func__, __LINE__);
                MI_OD_Uninit(pODHandle);
                return -1;
            }

            pthread_mutex_lock(&mutex_VDF_cfg);
            ret = MI_OD_SetAttr(pODHandle,                                               \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32ThdTamper,  \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32TamperBlkThd, \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32MinDuration,    \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32Alpha,          \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32M);
            pthread_mutex_unlock(&mutex_VDF_cfg);

            if(MI_RET_SUCESS != ret)
            {
                printf("[VDF] %s:%d, Set OD Attr error\n", __func__, __LINE__);
                MI_OD_Uninit(pODHandle);
                return -1;
            }

            // add the OD handle in VDF list
            if(0 != _MI_VDF_AddVDFNode(pstAttr, VdfChn, pODHandle))
            {
                printf("[VDF] %s:%d, Add OD handle to list error\n", __func__, __LINE__);
                MI_OD_Uninit(pODHandle);
                return -1;
            }


            //Malloc buffer for OD Handle list
            pstOdRstHdlListNew = (MI_VDF_OdRstHdlList_t*)malloc(sizeof(MI_VDF_OdRstHdlList_t));

            if(NULL == pstOdRstHdlListNew)
            {
                MI_OD_Uninit(pODHandle);
                _MI_VDF_DelVDFNode(VdfChn);
                printf("[VDF] %s:%d, Malloc buf for OD Result Handle list struct error\n", __func__, __LINE__);
                return -1;
            }

            memset(pstOdRstHdlListNew, 0x00, sizeof(MI_VDF_OdRstHdlList_t));
            pstOdRstHdlListNew->pOdHandle = pODHandle;
            pstOdRstHdlListNew->VdfChn = VdfChn;
            pstOdRstHdlListNew->next = NULL;

            //Malloc buffer for OD Result
            pstOdRstHdlListNew->pstOdResultList = (MI_OD_Result_t*)malloc(sizeof(MI_OD_Result_t) * u8RstBufNum);

            if(NULL == pstOdRstHdlListNew->pstOdResultList)
            {
                printf("[VDF] %s:%d, Malloc buffer for OD result error\n", __func__, __LINE__);
                MI_OD_Uninit(pODHandle);
                _MI_VDF_DelVDFNode(VdfChn);
                free(pstOdRstHdlListNew);
                pstOdRstHdlListNew = NULL;
                return -1;
            }

            memset(pstOdRstHdlListNew->pstOdResultList, 0x00, sizeof(MI_OD_Result_t) * u8RstBufNum);

            pthread_mutex_lock(&mutex_VDF_cfg);

            //          printf("[VDF] %s:%d, MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList=%p\n",__func__,__LINE__, \
            //                               MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList);
            //          printf("[VDF] %s:%d, VdfChn=%d, pODHandle=0x%x, pstOdRstHdlListNew=%p\n",__func__,__LINE__,\
            //                      pstOdRstHdlListNew->VdfChn, pstOdRstHdlListNew->pOdHandle, pstOdRstHdlListNew);
            if(NULL == MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList)
            {
                MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList = pstOdRstHdlListNew;
            }
            else
            {
                pstOdRstHdlListTmp = MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList;

                while(NULL != pstOdRstHdlListTmp)
                {
                    //                  printf("[VDF] %s:%d, VdfChn=%d, pODHandle=0x%x, pstOdRstHdlListTmp=%p\n",__func__,__LINE__,\
                    //                      pstOdRstHdlListTmp->VdfChn, pstOdRstHdlListTmp->pOdHandle, pstOdRstHdlListTmp);
                    pstOdRstHdlListTmp = (MI_VDF_OdRstHdlList_t*)pstOdRstHdlListTmp->next;
                }

                // add new buf to the tail of the list
                pstOdRstHdlListTmp = pstOdRstHdlListNew;
            }

            pthread_mutex_unlock(&mutex_VDF_cfg);

            //          printf("[VDF] %s:%d, MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList=%p\n",__func__,__LINE__, \
            //                               MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList);
            printf("[VDF] %s:%d, Create VDF Node: Chn=%d, od_hdl=%p OdRstHdl=%p\n", __func__, __LINE__, u8ViSrcChn, pODHandle, pstOdRstHdlListNew);

            //start the APP thread of the video source channel
            if(0 == VDF_PTHREAD_MUTEX[u8ViSrcChn].u8Enable)
            {
                MI_U32 argu = 0;
                MI_U8 cmdbuf[32];

                VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size = pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgW * \
                        pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgH;

                if(NULL != VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer)
                    free(VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer);

                VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer = (unsigned char *)malloc(VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size);

                if(NULL == VDF_PTHREAD_MUTEX[u8ViSrcChn].pu8YBuffer)
                {
                    printf("Video src chn(%d) malloc %d faild\n", u8ViSrcChn, VDF_PTHREAD_MUTEX[u8ViSrcChn].u32YImage_size);
                    free(pstOdRstHdlListTmp->pstOdResultList);
                    free(pstOdRstHdlListTmp);
                    MI_OD_Uninit(pODHandle);
                    _MI_VDF_DelVDFNode(VdfChn);
                    return -1;
                }

                memset(cmdbuf, 0x00, sizeof(cmdbuf));
                snprintf((char*)cmdbuf, sizeof(cmdbuf), "vdf_APP_task_Chn%d", u8ViSrcChn);

                argu = u8ViSrcChn;
                pthread_create(&(pthreadVDFAPP[u8ViSrcChn]), NULL, vdf_APP_task, (void *)argu);
                pthread_setname_np(pthreadVDFAPP[u8ViSrcChn] , cmdbuf);

#if !(VDF_TEST_ON_I3)
                MI_SHADOW_EnableChannel(_hVDFDev, u8ViSrcChn);
                MI_SHADOW_EnableInputPort(_hVDFDev, u8ViSrcChn, 0);
#endif
                VDF_PTHREAD_MUTEX[u8ViSrcChn].u8Enable = 1;
            }

            break;

        default:
            printf("[VDF] %s:%d, set the wrong WorkMode(%d)\n", __func__, __LINE__, pstAttr->enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_DestroyChn(MI_VDF_CHANNEL VdfChn)
{
    MI_S32 ret = 0;
    MI_U8  u8ViSrcChn = 0;
    void * phandle = NULL;
    MI_VDF_NodeList_t* pVdfNode_tmp = NULL;
    MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();


    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pVdfNode_tmp))
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    phandle = pVdfNode_tmp->phandle;
    enWorkMode = pVdfNode_tmp->stAttr.enWorkMode;

    if(E_MI_VDF_WORK_MODE_MD == enWorkMode)
        u8ViSrcChn = pVdfNode_tmp->stAttr.unAttr.stMdAttr.u8SrcChnNum;
    else if(E_MI_VDF_WORK_MODE_OD == enWorkMode)
        u8ViSrcChn = pVdfNode_tmp->stAttr.unAttr.stOdAttr.u8SrcChnNum;

    //1st: un-init the MD/OD func
    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            //TBD:stop the MD
            pthread_mutex_lock(&mutex_VDF_cfg);
            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
            MI_MD_Uninit(phandle);
            _MI_VDF_DelVDFNode(VdfChn);
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
            pthread_mutex_unlock(&mutex_VDF_cfg);

            break;

        case E_MI_VDF_WORK_MODE_OD:
            //TBD:stop the OD
            pthread_mutex_lock(&mutex_VDF_cfg);
            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
            MI_OD_Uninit(phandle);
            _MI_VDF_DelVDFNode(VdfChn);
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexRUN));
            pthread_mutex_unlock(&mutex_VDF_cfg);

            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    //2nd: free the MD/OD resault buffer
    if(E_MI_VDF_WORK_MODE_MD == enWorkMode)
    {
        MI_VDF_MdRstHdlList_t* pMdRstHdlList_tmp = NULL;
        MI_VDF_MdRstHdlList_t* pMdRstHdlList_pro = NULL;

        pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));
        pMdRstHdlList_tmp = MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList;

        // check the 1st node
        if((VdfChn == MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList->VdfChn) && \
           (phandle == MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList->pMdHandle))
        {
            MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList = pMdRstHdlList_tmp->next;
            free(pMdRstHdlList_tmp->pstMdResultList);
            free(pMdRstHdlList_tmp);
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));
            MI_VDF_FUNC_EXIT();
            return 0;
        }

        pMdRstHdlList_pro = MI_VDF_MD_RST_LIST[u8ViSrcChn].pstMdRstHdlList;
        pMdRstHdlList_tmp = pMdRstHdlList_pro->next;

        while(NULL != pMdRstHdlList_tmp)
        {
            if((VdfChn == pMdRstHdlList_tmp->VdfChn) && \
               (phandle == pMdRstHdlList_tmp->pMdHandle))
            {
                pMdRstHdlList_pro = pMdRstHdlList_tmp->next;
                free(pMdRstHdlList_tmp->pstMdResultList);
                free(pMdRstHdlList_tmp);
                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexMDRW));
                MI_VDF_FUNC_EXIT();
                return 0;
            }

            pMdRstHdlList_pro = pMdRstHdlList_tmp;
            pMdRstHdlList_tmp = pMdRstHdlList_tmp->next;
        }
    }
    else if(E_MI_VDF_WORK_MODE_OD == enWorkMode)
    {
        MI_VDF_OdRstHdlList_t* pOdRstHdlList_tmp = NULL;
        MI_VDF_OdRstHdlList_t* pOdRstHdlList_pro = NULL;

        pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));
        pOdRstHdlList_tmp = MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList;

        // check the 1st node
        if((VdfChn == MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList->VdfChn) && \
           (phandle == MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList->pOdHandle))
        {
            MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList = pOdRstHdlList_tmp->next;
            free(pOdRstHdlList_tmp->pstOdResultList);
            free(pOdRstHdlList_tmp);
            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));
            MI_VDF_FUNC_EXIT();
            return 0;
        }

        pOdRstHdlList_pro = MI_VDF_OD_RST_LIST[u8ViSrcChn].pstOdRstHdlList;
        pOdRstHdlList_tmp = pOdRstHdlList_pro->next;

        while(NULL != pOdRstHdlList_tmp)
        {
            if((VdfChn == pOdRstHdlList_tmp->VdfChn) && \
               (phandle == pOdRstHdlList_tmp->pOdHandle))
            {
                pOdRstHdlList_pro = pOdRstHdlList_tmp->next;
                free(pOdRstHdlList_tmp->pstOdResultList);
                free(pOdRstHdlList_tmp);
                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[u8ViSrcChn].mutexODRW));
                MI_VDF_FUNC_EXIT();
                return 0;
            }

            pOdRstHdlList_pro = (MI_VDF_OdRstHdlList_t*)pOdRstHdlList_tmp;
            pOdRstHdlList_tmp = (MI_VDF_OdRstHdlList_t*)pOdRstHdlList_tmp->next;
        }
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_SetChnAttr(MI_VDF_CHANNEL VdfChn, const MI_VDF_ChnAttr_t* pstAttr)
{
    MI_S32 ret = 0;
    MI_U32 i = 0;
    MI_U32 j = 0;
    MI_U16 u16W_div_tmp = 0;
    MI_U16 u16H_div_tmp = 0;
    void * phandle = NULL;
    MI_VDF_ChnAttr_t   stAttrTmp = { 0 };
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;
    MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstAttr)
    {
        printf("[VDF] %s:%d, Input parameter is NULL\n", __func__, __LINE__);
        return -1;
    }


    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstTmpVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    phandle = pstTmpVDFNode->phandle;
    enWorkMode = pstTmpVDFNode->stAttr.enWorkMode;

    memset(&stAttrTmp, 0x00, sizeof(stAttrTmp));
    memcpy(&stAttrTmp, &pstTmpVDFNode->stAttr, sizeof(stAttrTmp));

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            pthread_mutex_lock(&mutex_VDF_cfg);
            //          memcpy(&stAttrTmp.unAttr.stMdAttr.stMDParamsIn, \
            //                 &pstAttr->unAttr.stMdAttr.stMDParamsIn,   \
            //                 sizeof(VDF_MDParamsIn_S));
            //          stAttrTmp.unAttr.stMdAttr.u32VDFIntvl = pstTmpVDFNode->stAttr.unAttr.stMdAttr.u32VDFIntvl;
            u16W_div_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv < pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col) ? \
                           pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv : pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col;

            u16H_div_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv < pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row) ? \
                           pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv : pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row;

            //          stAttrTmp.unAttr.stMdAttr.stMDRgnSet.u32Enable = pstAttr->unAttr.stMdAttr.stMDRgnSet.u32Enable;

            for(j = 0; j < u16H_div_tmp; j++)
            {
                for(i = 0; i < u16W_div_tmp; i++)
                {
#if (MD_OD_SINGLE_PTHREAD)
                    ret = MI_MD_SetWindowParamsIn(i, j, &pstAttr->unAttr.stMdAttr.stMDParamsIn);
#else
                    ret = MI_MD_SetWindowParamsIn(phandle, i, j, &pstAttr->unAttr.stMdAttr.stMDParamsIn);
#endif

                    if(MI_RET_SUCESS == ret)
                    {
#if 0
                        MDParamsIn_t MdParamOut = { 0 };
                        MI_MD_GetWindowParamsIn(phandle,                                   \
                                                pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col, \
                                                pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row, \
                                                &MdParamOut);
                        printf(" thMD: Set: [%u,%u]=(%u,%u,%u,%u,%u)\n",                   \
                               pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col, \
                               pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row, \
                               MdParamOut.enable,                         \
                               MdParamOut.size_perct_thd_min,             \
                               MdParamOut.size_perct_thd_max,             \
                               MdParamOut.learn_rate,                     \
                               MdParamOut.sensitivity);
#endif
                    }
                    else
                    {
                        printf("[VDF] %s:%d, Set MD Sub-Window Parames error:\n", __func__, __LINE__);
                        printf(" thMD: In: [%u,%u]=(%u,%u,%u,%u,%u)\n",                                     \
                               pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col,                  \
                               pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row,                  \
                               pstAttr->unAttr.stMdAttr.stMDParamsIn.u8Enable,             \
                               pstAttr->unAttr.stMdAttr.stMDParamsIn.u8SizePerctThdMin, \
                               pstAttr->unAttr.stMdAttr.stMDParamsIn.u8SizePerctThdMax, \
                               pstAttr->unAttr.stMdAttr.stMDParamsIn.u16LearnRate,        \
                               pstAttr->unAttr.stMdAttr.stMDParamsIn.u8Sensitivity);
                    }
                }
            }

            pthread_mutex_unlock(&mutex_VDF_cfg);
            break;

        case E_MI_VDF_WORK_MODE_OD:
            pthread_mutex_lock(&mutex_VDF_cfg);

            memcpy(&stAttrTmp.unAttr.stOdAttr.stODParamsIn, \
                   &pstAttr->unAttr.stOdAttr.stODParamsIn,   \
                   sizeof(MI_VDF_ODParamsIn_t));
            stAttrTmp.unAttr.stOdAttr.u32VDFIntvl = pstTmpVDFNode->stAttr.unAttr.stOdAttr.u32VDFIntvl;
            stAttrTmp.unAttr.stOdAttr.stODParamsIn.endiv = pstTmpVDFNode->stAttr.unAttr.stOdAttr.stODParamsIn.endiv;
            stAttrTmp.unAttr.stOdAttr.stODRgnSet.u8Row = pstTmpVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u8Row;
            stAttrTmp.unAttr.stOdAttr.stODRgnSet.u8Col = pstTmpVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u8Col;
            stAttrTmp.unAttr.stOdAttr.stODRgnSet.u32Enable = pstTmpVDFNode->stAttr.unAttr.stOdAttr.stODRgnSet.u32Enable;

            ret = MI_OD_SetAttr(phandle,                                                 \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32ThdTamper,     \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32TamperBlkThd, \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32MinDuration,   \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32Alpha,          \
                                pstAttr->unAttr.stOdAttr.stODParamsIn.s32M);
            pthread_mutex_unlock(&mutex_VDF_cfg);

            if(MI_RET_SUCESS != ret)
            {
                printf("[VDF] %s:%d, Set OD Attr error\n", __func__, __LINE__);
                return -1;
            }

            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_GetChnAttr(MI_VDF_CHANNEL VdfChn, MI_VDF_ChnAttr_t* pstAttr)
{
    MI_S32 ret = 0;
    void * phandle = NULL;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;
    MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstAttr)
    {
        printf("[VDF] %s:%d, Input parameter is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstTmpVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    phandle = pstTmpVDFNode->phandle;
    enWorkMode = pstTmpVDFNode->stAttr.enWorkMode;

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
#if (MD_OD_SINGLE_PTHREAD)
            ret = MI_MD_GetWindowParamsIn(pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col, \
                                          pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row, \
                                          &pstAttr->unAttr.stMdAttr.stMDParamsIn);
#else
            ret = MI_MD_GetWindowParamsIn(phandle,                                   \
                                          pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Col, \
                                          pstAttr->unAttr.stMdAttr.stMDRgnSet.u8Row, \
                                          &pstAttr->unAttr.stMdAttr.stMDParamsIn);
#endif

            if(MI_RET_SUCESS != ret)
            {
                printf("[VDF] %s:%d, Get VdfChn(%d) Window Parames error\n", __func__, __LINE__, VdfChn);
                return -1;
            }

#if (MD_OD_SINGLE_PTHREAD)
            ret = MI_MD_GetDetectWindowSize(&pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtX, \
                                            &pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtY, \
                                            &pstAttr->unAttr.stMdAttr.stMDRgnSet.u16WideDiv, \
                                            &pstAttr->unAttr.stMdAttr.stMDRgnSet.u16HightDiv);
#else
            ret = MI_MD_GetDetectWindowSize(phandle,                                      \
                                            &pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtX, \
                                            &pstAttr->unAttr.stMdAttr.stMDObjCfg.u16LtY, \
                                            &pstAttr->unAttr.stMdAttr.stMDRgnSet.u16WideDiv, \
                                            &pstAttr->unAttr.stMdAttr.stMDRgnSet.u16HightDiv);
#endif

            if(MI_RET_SUCESS != ret)
            {
                printf("[VDF] %s:%d, Get VdfChn(%d) Window size error\n", __func__, __LINE__, VdfChn);
                return -1;
            }

            break;

        case E_MI_VDF_WORK_MODE_OD:
            memset(pstAttr, 0x00, sizeof(MI_VDF_ChnAttr_t));
            memcpy(pstAttr, &pstTmpVDFNode->stAttr, sizeof(MI_VDF_ChnAttr_t));
            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_EnableSubWindow(MI_VDF_CHANNEL VdfChn, MI_U8 u8Col, MI_U8 u8Row, MI_U8 u8Enable)
{
    MI_S32 ret = 0;
    MI_U8 u8Col_tmp = 0;
    MI_U8 u8Row_tmp = 0;
    MI_VDF_NodeList_t* pstTmpVDFNode = NULL;
    MI_VDF_WorkMode_e  enWorkMode = E_MI_VDF_WORK_MODE_MAX;
    MDParamsIn_t stMDParamsIn;

    MI_VDF_FUNC_ENTRY();


    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstTmpVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    enWorkMode = pstTmpVDFNode->stAttr.enWorkMode;

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstTmpVDFNode->stAttr.unAttr.stMdAttr.u8SrcChnNum].mutexRUN));

            u8Col_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv < u8Col) ? \
                        pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv : u8Col;

            u8Row_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv < u8Row) ? \
                        pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv : u8Row;

            pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDParamsIn.u8Enable = u8Enable;

            memset(&stMDParamsIn, 0x00, sizeof(stMDParamsIn));
            memcpy(&stMDParamsIn, &pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDParamsIn, sizeof(stMDParamsIn));

#if (MD_OD_SINGLE_PTHREAD)
            ret = MI_MD_SetWindowParamsIn(u8Col_tmp, u8Row_tmp, &stMDParamsIn);
#else
            ret = MI_MD_SetWindowParamsIn(pstTmpVDFNode->phandle, u8Col_tmp, u8Row_tmp, &stMDParamsIn);
#endif

            if(MI_RET_SUCESS == ret)
            {
#if 0
                MDParamsIn_t stMDparamOut = { 0 };
                MI_MD_GetWindowParamsIn(pstTmpVDFNode->phandle, u8Col_tmp, u8Row_tmp, &stMDparamOut);
                printf(" thMD: Get: [%u,%u]=(%u,%u,%u,%u,%u)\n",                                \
                       u8Col_tmp, u8Row_tmp,                                   \
                       stMDparamOut.enable,                                    \
                       stMDparamOut.size_perct_thd_min,                        \
                       stMDparamOut.size_perct_thd_max,                        \
                       stMDparamOut.learn_rate,                                \
                       stMDparamOut.sensitivity);
#endif
            }

            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstTmpVDFNode->stAttr.unAttr.stMdAttr.u8SrcChnNum].mutexRUN));
            break;

        case E_MI_VDF_WORK_MODE_OD:

            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstTmpVDFNode->stAttr.unAttr.stOdAttr.u8SrcChnNum].mutexRUN));

            u8Col_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv < u8Col) ? \
                        pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16WideDiv : u8Col;

            u8Row_tmp = (pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv < u8Row) ? \
                        pstTmpVDFNode->stAttr.unAttr.stMdAttr.stMDRgnSet.u16HightDiv : u8Row;

            MI_OD_SetWindowEnable(pstTmpVDFNode->phandle, u8Col_tmp, u8Row_tmp, u8Enable);

            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstTmpVDFNode->stAttr.unAttr.stOdAttr.u8SrcChnNum].mutexRUN));
            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_Run(MI_VDF_WorkMode_e      enWorkMode)
{
    MI_S32 ret = 0;

    MI_VDF_FUNC_ENTRY();

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            pthread_mutex_lock(&mutexRUN);
            g_MDEnable = 1;
            pthread_mutex_unlock(&mutexRUN);

            break;

        case E_MI_VDF_WORK_MODE_OD:
            pthread_mutex_lock(&mutexRUN);
            g_ODEnable = 1;
            pthread_mutex_unlock(&mutexRUN);

            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_Stop(MI_VDF_WorkMode_e      enWorkMode)
{
    MI_S32 ret = 0;

    MI_VDF_FUNC_ENTRY();

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            pthread_mutex_lock(&mutexRUN);
            g_MDEnable = 0;
            pthread_mutex_unlock(&mutexRUN);

            break;

        case E_MI_VDF_WORK_MODE_OD:
            pthread_mutex_lock(&mutexRUN);
            g_ODEnable = 0;
            pthread_mutex_unlock(&mutexRUN);

            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_GetResult(MI_VDF_CHANNEL VdfChn, MI_VDF_Result_t* pstVdfResult, MI_S32 s32MilliSec)
{
    MI_S32 ret = 0;
    MI_U8  u8ViSrcChn = 0;
    MI_VDF_NodeList_t* pstVDFNode = NULL;
    MI_VDF_WorkMode_e  enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstVdfResult)
    {
        printf("[VDF] %s:%d, The input pointer(MI_VDF_RESULT_S*) is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn(%u) Node info error\n", __func__, __LINE__, VdfChn);
        return -1;
    }

    enWorkMode = pstVdfResult->enWorkMode;

    switch(pstVdfResult->enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            u8ViSrcChn = pstVDFNode->stAttr.unAttr.stMdAttr.u8SrcChnNum;

            //Get the MD detect result from the list
            if(NULL != MI_VDF_MD_RST_LIST[pstVdfResult->u8SrcChnNum].pstMdRstHdlList)
            {
                MI_VDF_MdRstHdlList_t* pstMdRstHdlListTmp = NULL;

                pstMdRstHdlListTmp = MI_VDF_MD_RST_LIST[pstVdfResult->u8SrcChnNum].pstMdRstHdlList;

                //find the MD handle from the Handle list
                pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));

                while(NULL != pstMdRstHdlListTmp)
                {
                    if((pstVDFNode->VdfChn == pstMdRstHdlListTmp->VdfChn) && \
                       (pstVDFNode->phandle == pstMdRstHdlListTmp->pMdHandle))
                    {
                        MI_MD_Result_t* pMdRstList = NULL;

                        pMdRstList = pstMdRstHdlListTmp->pstMdResultList;

                        if((0 == pstMdRstHdlListTmp->s8DeltData) && (0 < s32MilliSec))
                        {
                            struct timeval tv;
                            tv.tv_sec  = s32MilliSec / 1000;
                            tv.tv_usec = (s32MilliSec % 1000) * 1000;

                            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));
                            //wait for write MD detect result to the list
                            select(0, NULL, NULL, NULL, &tv);
                            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));
                        }

                        if(pstMdRstHdlListTmp->u16ReadPst < pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum)
                        {
                            memcpy(&pstVdfResult->unVdfResult.stMdResult, \
                                   pMdRstList + pstMdRstHdlListTmp->u16ReadPst, \
                                   sizeof(MI_MD_Result_t));

                            pstVdfResult->unVdfResult.stMdResult.u8DataLen = pstMdRstHdlListTmp->s8DeltData;
                        }
                        else
                        {
                            printf("[VDF] %s:%d VdfChn(%u) D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__, \
                                   pstVDFNode->VdfChn,              \
                                   pstMdRstHdlListTmp->s8DeltData,  \
                                   pstMdRstHdlListTmp->u16WritePst, \
                                   pstMdRstHdlListTmp->u16ReadPst,  \
                                   pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum);
                        }

                        break; //break the while loop
                    }

                    pstMdRstHdlListTmp = (MI_VDF_MdRstHdlList_t*)pstMdRstHdlListTmp->next;
                }

                if(NULL == pstMdRstHdlListTmp)
                {
                    printf("[VDF] %s:%d VdfChn=%u Cannot find the MD Result handle list\n", __func__, __LINE__, pstMdRstHdlListTmp->VdfChn);
                }

                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));
            }
            else
            {
                printf("[VDF] %s:%d VdfChn=%u  Get MD Result handle list error\n", __func__, __LINE__, VdfChn);
            }

            break; //breake the case:E_MI_VDF_WORK_MODE_MD

        case E_MI_VDF_WORK_MODE_OD:
            u8ViSrcChn = pstVDFNode->stAttr.unAttr.stOdAttr.u8SrcChnNum;

            //Get the OD detect result from the list
            if(NULL != MI_VDF_OD_RST_LIST[pstVdfResult->u8SrcChnNum].pstOdRstHdlList)
            {
                MI_VDF_OdRstHdlList_t* pstOdRstHdlListTmp = NULL;

                pstOdRstHdlListTmp = MI_VDF_OD_RST_LIST[pstVdfResult->u8SrcChnNum].pstOdRstHdlList;

                //find the OD handle from the Handle list
                pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));

                while(NULL != pstOdRstHdlListTmp)
                {
                    if((pstVDFNode->VdfChn == pstOdRstHdlListTmp->VdfChn) && \
                       (pstVDFNode->phandle == pstOdRstHdlListTmp->pOdHandle))
                    {
                        MI_OD_Result_t* pOdRstList = NULL;

                        pOdRstList = pstOdRstHdlListTmp->pstOdResultList;

                        if((0 == pstOdRstHdlListTmp->s8DeltData) && (0 < s32MilliSec))
                        {
                            struct timeval tv;
                            tv.tv_sec  = s32MilliSec / 1000;
                            tv.tv_usec = (s32MilliSec % 1000) * 1000;

                            pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));
                            //wait for write OD detect result to the list
                            select(0, NULL, NULL, NULL, &tv);
                            pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));
                        }

                        if(pstOdRstHdlListTmp->u16ReadPst < pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum)
                        {
                            memcpy(&pstVdfResult->unVdfResult.stOdResult, \
                                   pOdRstList + pstOdRstHdlListTmp->u16ReadPst, \
                                   sizeof(MI_OD_Result_t));

                            pstVdfResult->unVdfResult.stOdResult.u8DataLen = pstOdRstHdlListTmp->s8DeltData;
                        }
                        else
                        {
                            printf("[VDF] %s:%d VdfChn(%u) D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", __func__, __LINE__,   \
                                   pstVDFNode->VdfChn,  \
                                   pstOdRstHdlListTmp->s8DeltData,  \
                                   pstOdRstHdlListTmp->u16WritePst, \
                                   pstOdRstHdlListTmp->u16ReadPst,  \
                                   pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum);
                        }

                        break; //break the while loop
                    }

                    pstOdRstHdlListTmp = (MI_VDF_OdRstHdlList_t*)pstOdRstHdlListTmp->next;
                }

                if(NULL == pstOdRstHdlListTmp)
                {
                    printf("[VDF] %s:%d Cannot find the VdfChn(OD:%u) Result handle list\n", __func__, __LINE__, pstOdRstHdlListTmp->VdfChn);
                }

                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));
            }
            else
            {
                printf("[VDF] %s:%d Get VdfChn(%u) OD Result handle list error\n", __func__, __LINE__, VdfChn);
            }

            break; //break the case:E_MI_VDF_WORK_MODE_OD

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, pstVdfResult->enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_PutResult(MI_VDF_CHANNEL VdfChn, MI_VDF_Result_t* pstVdfResult)
{
    MI_S32 ret = 0;
    MI_VDF_NodeList_t* pstVDFNode = NULL;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstVdfResult)
    {
        printf("[VDF] %s:%d, The input pointer(MI_VDF_RESULT_S*) is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn(%u) Node info error\n", __func__, __LINE__, VdfChn);
        return -1;
    }

    switch(pstVdfResult->enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:

            //Get the MD detect result from the list
            if(NULL != MI_VDF_MD_RST_LIST[pstVdfResult->u8SrcChnNum].pstMdRstHdlList)
            {
                MI_VDF_MdRstHdlList_t* pstMdRstHdlListTmp = NULL;

                pstMdRstHdlListTmp = MI_VDF_MD_RST_LIST[pstVdfResult->u8SrcChnNum].pstMdRstHdlList;

                //find the MD handle from the Handle list
                pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));

                while(NULL != pstMdRstHdlListTmp)
                {
                    if((0 < pstMdRstHdlListTmp->s8DeltData) && \
                       (pstVDFNode->VdfChn == pstMdRstHdlListTmp->VdfChn) && \
                       (pstVDFNode->phandle == pstMdRstHdlListTmp->pMdHandle))
                    {
                        int i = 0;
                        MI_MD_Result_t* pstMdResult = NULL;

                        pstMdResult = pstMdRstHdlListTmp->pstMdResultList;

                        //printf("[%s:%d] pstMdResult=%p, sizeof(MI_MD_Result_t)=0x%x\n", __func__,__LINE__, pstMdResult, sizeof(MI_MD_Result_t));
                        for(i = 0; i < pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum; i++)
                        {
                            pstMdResult++;

                            //printf("[%s:%d] pstMdResult=%p i=%u enable=%u pts=0x%llx\n", __func__,__LINE__, \
                            //  pstMdResult, i, pstMdResult->u8Enable, pstMdResult->u64Pts);
                            if(pstVdfResult->unVdfResult.stMdResult.u64Pts == pstMdResult->u64Pts)
                                //if((1 == pstMdResult->u8Enable) && (pstVdfResult->unVdfResult.stMdResult.u64Pts == pstMdResult->u64Pts))
                            {
                                if(i != pstMdRstHdlListTmp->u16ReadPst)
                                {
                                    printf("[%s:%d] The Read position is worong(%u, %u)\n", __func__, __LINE__, i, pstMdRstHdlListTmp->u16ReadPst);
                                }

                                memset(pstMdResult, 0x00, sizeof(MI_MD_Result_t));
                                pstMdRstHdlListTmp->u16ReadPst++;
                                pstMdRstHdlListTmp->s8DeltData--;
                                pstMdRstHdlListTmp->u16ReadPst = pstMdRstHdlListTmp->u16ReadPst % pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum;

                                if(pstMdRstHdlListTmp->s8DeltData < 0)
                                {
                                    pstMdRstHdlListTmp->s8DeltData = 0;
                                }

                                if(VDF_DBG_LOG_ENABLE)
                                {
                                    printf("[VDF] %s:%d MD D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", \
                                           __func__, __LINE__,                       \
                                           pstMdRstHdlListTmp->s8DeltData,           \
                                           pstMdRstHdlListTmp->u16WritePst,          \
                                           pstMdRstHdlListTmp->u16ReadPst,           \
                                           pstVDFNode->stAttr.unAttr.stMdAttr.u32MdBufNum);
                                }

                                break; //break the for loop
                            }
                        }

                        break; //break the while loop
                    }

                    pstMdRstHdlListTmp = (MI_VDF_MdRstHdlList_t*)pstMdRstHdlListTmp->next;
                }

                if(NULL == pstMdRstHdlListTmp)
                {
                    printf("[VDF] %s:%d VdfChn=%u MD Get result list fail\n", __func__, __LINE__, VdfChn);
                }

                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexMDRW));
            }
            else
            {
                printf("[VDF] %s:%d VifSrc=%u MD Get result Handle list fail\n", __func__, __LINE__, pstVdfResult->u8SrcChnNum);
            }

            break; //break the case:E_MI_VDF_WORK_MODE_MD

        case E_MI_VDF_WORK_MODE_OD:

            //Get the OD detect result from the list
            if(NULL != MI_VDF_OD_RST_LIST[pstVdfResult->u8SrcChnNum].pstOdRstHdlList)
            {
                MI_VDF_OdRstHdlList_t* pstOdRstHdlListTmp = NULL;

                pstOdRstHdlListTmp = MI_VDF_OD_RST_LIST[pstVdfResult->u8SrcChnNum].pstOdRstHdlList;

                //find the MD handle from the Handle list
                pthread_mutex_lock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));

                while(NULL != pstOdRstHdlListTmp)
                {
                    if((0 < pstOdRstHdlListTmp->s8DeltData) && \
                       (pstVDFNode->VdfChn == pstOdRstHdlListTmp->VdfChn) && \
                       (pstVDFNode->phandle == pstOdRstHdlListTmp->pOdHandle))
                    {
                        int i = 0;
                        MI_OD_Result_t* pstOdResult = NULL;

                        pstOdResult = pstOdRstHdlListTmp->pstOdResultList;

                        //printf("[%s:%d] pstOdResult=%p, sizeof(MI_OD_Result_t)=0x%x\n", __func__,__LINE__, pstOdResult, sizeof(MI_OD_Result_t));
                        for(i = 0; i < pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum; i++)
                        {
                            pstOdResult++;

                            //printf("[%s:%d] pstOdResult=%p i=%u enable=%u pts=0x%llx\n", __func__,__LINE__,\
                            //  pstOdResult, i,  pstOdResult->u8Enable, pstOdResult->u64Pts);

                            if(pstVdfResult->unVdfResult.stOdResult.u64Pts == pstOdResult->u64Pts)
                                //if((1 == pstOdResult->u8Enable) && (pstVdfResult->unVdfResult.stOdResult.u64Pts == pstOdResult->u64Pts))
                            {
                                if(i != pstOdRstHdlListTmp->u16ReadPst)
                                {
                                    printf("[%s:%d] The OD Read position is worong(%u, %u)\n", __func__, __LINE__, i, pstOdRstHdlListTmp->u16ReadPst);
                                }

                                memset(pstOdResult, 0x00, sizeof(MI_OD_Result_t));
                                pstOdRstHdlListTmp->u16ReadPst++;
                                pstOdRstHdlListTmp->u16ReadPst = pstOdRstHdlListTmp->u16ReadPst % pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum;

                                pstOdRstHdlListTmp->s8DeltData--;

                                if(pstOdRstHdlListTmp->s8DeltData < 0)
                                {
                                    pstOdRstHdlListTmp->s8DeltData = 0;
                                }

                                if(VDF_DBG_LOG_ENABLE)
                                {
                                    printf("[VDF] %s:%d OD D_Delt=%d, W_Ptr=%u, R_Ptr=%u, buf_Size=%u\n", \
                                           __func__, __LINE__,                       \
                                           pstOdRstHdlListTmp->s8DeltData,           \
                                           pstOdRstHdlListTmp->u16WritePst,          \
                                           pstOdRstHdlListTmp->u16ReadPst,           \
                                           pstVDFNode->stAttr.unAttr.stOdAttr.u32OdBufNum);
                                }

                                break; //break the for loop
                            }
                        }

                        break; //break the while loop
                    }

                    pstOdRstHdlListTmp = (MI_VDF_OdRstHdlList_t*)pstOdRstHdlListTmp->next;
                }

                if(NULL == pstOdRstHdlListTmp)
                {
                    printf("[VDF] %s:%d VdfChn=%u OD Get result list fail\n", __func__, __LINE__, VdfChn);
                }

                pthread_mutex_unlock(&(VDF_PTHREAD_MUTEX[pstVdfResult->u8SrcChnNum].mutexODRW));
            }
            else
            {
                printf("[VDF] %s:%d VivSrc=%u OD Get result Handle list fail\n", __func__, __LINE__, pstVdfResult->u8SrcChnNum);
            }

            break; //break the case:E_MI_VDF_WORK_MODE_OD

        default:
            printf("[VDF] %s:%d get the wrong WorkMode(%d)\n", __func__, __LINE__, pstVdfResult->enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_GetLibVersion(MI_VDF_CHANNEL VdfChn, MI_U32* u32VDFVersion)
{
    MI_S32 ret = 0;
    void * phandle = NULL;
    MI_VDF_NodeList_t* pstVDFNode = NULL;
    MI_VDF_WorkMode_e enWorkMode = E_MI_VDF_WORK_MODE_MAX;

    MI_VDF_FUNC_ENTRY();

    if(NULL == u32VDFVersion)
    {
        printf("[VDF] %s:%d, Input parameter is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_GetVDFNode(VdfChn, &pstVDFNode))
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    if(NULL == pstVDFNode)
    {
        printf("[VDF] %s:%d, get VdfChn Node info error\n", __func__, __LINE__);
        return -1;
    }

    phandle = pstVDFNode->phandle;
    enWorkMode = pstVDFNode->stAttr.enWorkMode;

    *u32VDFVersion = 0;

    switch(enWorkMode)
    {
        case E_MI_VDF_WORK_MODE_MD:
            pthread_mutex_lock(&mutexRUN);
            MI_MD_GetLibVersion(phandle);
            //*u32VDFVersion = MI_MD_GetLibVersion(phandle);
            pthread_mutex_unlock(&mutexRUN);

            break;

        case E_MI_VDF_WORK_MODE_OD:
            pthread_mutex_lock(&mutexRUN);
            //*u32VDFVersion = MI_OD_GetLibVersion(phandle);
            pthread_mutex_unlock(&mutexRUN);

            break;

        default:
            printf("[VDF] %s:%d, get the wrong WorkMode(%d)\n", __func__, __LINE__, enWorkMode);
            return -1;
    }

    MI_VDF_FUNC_EXIT();
    return ret;
}


MI_S32 MI_VDF_Query(MI_VDF_CHANNEL VdfChn, MI_VDF_ChnStat_t *pstChnState)
{
    MI_S32 ret = 0;

    MI_VDF_FUNC_ENTRY();

    if(NULL == pstChnState)
    {
        printf("[VDF] %s:%d, Input parameter is NULL\n", __func__, __LINE__);
        return -1;
    }

    if(0 != _MI_VDF_CheckChnValid(VdfChn))
    {
        printf("[VDF] %s:%d, The input VdfChn(%d) is invalid\n", __func__, __LINE__, VdfChn);

        MI_VDF_FUNC_EXIT();
    }

    MI_VDF_FUNC_EXIT();

    return ret;
}


