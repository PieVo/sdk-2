//<MStar Software>
//******************************************************************************
// MStar Software
// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.
// All software, firmware and related documentation herein ("MStar Software") are
// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by
// law, including, but not limited to, copyright law and international treaties.
// Any use, modification, reproduction, retransmission, or republication of all
// or part of MStar Software is expressly prohibited, unless prior written
// permission has been granted by MStar.
//
// By accessing, browsing and/or using MStar Software, you acknowledge that you
// have read, understood, and agree, to be bound by below terms ("Terms") and to
// comply with all applicable laws and regulations:
//
// 1. MStar shall retain any and all right, ownership and interest to MStar
//    Software and any modification/derivatives thereof.
//    No right, ownership, or interest to MStar Software and any
//    modification/derivatives thereof is transferred to you under Terms.
//
// 2. You understand that MStar Software might include, incorporate or be
//    supplied together with third party`s software and the use of MStar
//    Software may require additional licenses from third parties.
//    Therefore, you hereby agree it is your sole responsibility to separately
//    obtain any and all third party right and license necessary for your use of
//    such third party`s software.
//
// 3. MStar Software and any modification/derivatives thereof shall be deemed as
//    MStar`s confidential information and you agree to keep MStar`s
//    confidential information in strictest confidence and not disclose to any
//    third party.
//
// 4. MStar Software is provided on an "AS IS" basis without warranties of any
//    kind. Any warranties are hereby expressly disclaimed by MStar, including
//    without limitation, any warranties of merchantability, non-infringement of
//    intellectual property rights, fitness for a particular purpose, error free
//    and in conformity with any international standard.  You agree to waive any
//    claim against MStar for any loss, damage, cost or expense that you may
//    incur related to your use of MStar Software.
//    In no event shall MStar be liable for any direct, indirect, incidental or
//    consequential damages, including without limitation, lost of profit or
//    revenues, lost or damage of data, and unauthorized system use.
//    You agree that this Section 4 shall still apply without being affected
//    even if MStar Software has been modified by MStar in accordance with your
//    request or instruction for your use, except otherwise agreed by both
//    parties in writing.
//
// 5. If requested, MStar may from time to time provide technical supports or
//    services in relation with MStar Software to you for your use of
//    MStar Software in conjunction with your or your customer`s product
//    ("Services").
//    You understand and agree that, except otherwise agreed by both parties in
//    writing, Services are provided on an "AS IS" basis and the warranty
//    disclaimer set forth in Section 4 above shall apply.
//
// 6. Nothing contained herein shall be construed as by implication, estoppels
//    or otherwise:
//    (a) conferring any license or right to use MStar name, trademark, service
//        mark, symbol or any other identification;
//    (b) obligating MStar or any of its affiliates to furnish any person,
//        including without limitation, you and your customers, any assistance
//        of any kind whatsoever, or any information; or
//    (c) conferring any license or right under any intellectual property right.
//
// 7. These terms shall be governed by and construed in accordance with the laws
//    of Taiwan, R.O.C., excluding its conflict of law rules.
//    Any and all dispute arising out hereof or related hereto shall be finally
//    settled by arbitration referred to the Chinese Arbitration Association,
//    Taipei in accordance with the ROC Arbitration Law and the Arbitration
//    Rules of the Association by three (3) arbitrators appointed in accordance
//    with the said Rules.
//    The place of arbitration shall be in Taipei, Taiwan and the language shall
//    be English.
//    The arbitration award shall be final and binding to both parties.
//
//******************************************************************************
//<MStar Software>
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#define HAL_VPE_C


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
// SC Definition
#include "drv_scl_os.h"
#include "drv_scl_dma_io_st.h"
#include "drv_scl_dma_io_wrapper.h"
#include "drv_scl_hvsp_io_st.h"
#include "drv_scl_hvsp_io_wrapper.h"
#include "drv_scl_vip_io_st.h"
#include "drv_scl_vip_io_wrapper.h"
#include "drv_scl_verchk.h"
#include "drv_scl_irq.h"
#include "mhal_vpe.h"
#include "infinity2_reg_isp0.h"
#include <mhal_common.h>

//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define minmax(v,a,b)       (((v)<(a))? (a) : ((v)>(b)) ? (b) : (v))

#if defined (SCLOS_TYPE_LINUX_KERNEL)
#else
#ifdef abs
#undef abs
#endif
#define abs(a)              ((a)<0?-(a):(a))

#ifdef min
#undef min
#endif
#define min(x,y) \
  ( x < y ?  x :  y)
#ifdef max
#undef max
#endif
#define max(x,y) \
  ( x > y ? x : y)
#endif
//-------------------------------------------------------------------------------------------------
//  SC Defines & Macro
//-------------------------------------------------------------------------------------------------
#define VPE_HANDLER_INSTANCE_NUM    (64)
#define VPE_RETURN_ERR 0
#define VPE_RETURN_OK 1
#define VPE_DBG_LV_SC(enLevel)               (gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_SC]&enLevel)
#define VPE_DBG_LV_IQ(enLevel)             (gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_IQ]&enLevel)
#define VPE_DBG_LV_ISP()                (gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_ISP])
#define VPEIRQ_MSK_SC3_ENG_FRM_END      (((u64)1)<<1)
#define VPEIRQ_SC3_ENG_FRM_END          1

//ne buffer offset
#define NEBUF_SADY          0
#define NEBUF_SUMY          16
#define NEBUF_CNTY          48
#define NEBUF_SADC          64
#define NEBUF_SUMC          80
#define NEBUF_CNTC          112
#define NEBUF_TARGET        128
#define NEBUF_TH_GAIN       130
#define NEBUF_BLK_NUM       132
#define NEBUF_SAD_MIN       133
#define NEBUF_NOISE_MEAN    134
#define NEBUF_LEARN         135
#define NEBUF_YGAIN         136
#define NEBUF_CGAIN         137

//qmap config offset
#define QMAP_CONFIG_MCNR_YTBL       0
#define QMAP_CONFIG_MCNR_CTBL       16
#define QMAP_CONFIG_MCNR_MV_YTBL    32
#define QMAP_CONFIG_MCNR_MV_CTBL    48
#define QMAP_CONFIG_NLM_SHIFT       64
#define QMAP_CONFIG_NLM_MAIN        65
#define QMAP_CONFIG_NLM_WB          81
#define QMAP_CONFIG_XNR             97
#define QMAP_CONFIG_YEE_ETCP        101
#define QMAP_CONFIG_YEE_ETCM        107
#define QMAP_CONFIG_SIZE            113

//qmap onoff offset
#define QMAP_ONOFF_NLM_EN      0
#define QMAP_ONOFF_XNR_EN      1
#define QMAP_ONOFF_YEE_EN      2
#define QMAP_ONOFF_NLM_SHIFT   3
#define QMAP_ONOFF_NLM_MAIN    4
#define QMAP_ONOFF_NLM_WB      20
#define QMAP_ONOFF_WDR_EN      36
#define QMAP_ONOFF_UVM_EN      37
#define QMAP_ONOFF_UVM         38
#define QMAP_ONOFF_SIZE        43

//qmap process offset
#define QMAP_PROCESS_MCNR_YGAIN     0
#define QMAP_PROCESS_MCNR_CGAIN     1
#define QMAP_PROCESS_MCNR_LUMAY     2
#define QMAP_PROCESS_MCNR_LUMAC     10
#define QMAP_PROCESS_MCNR_EN        18  //for video start, need to delay 1 frame to enable mcnr
#define QMAP_PROCESS_WDR_STR        19
#define QMAP_PROCESS_SIZE           21

//qmap process offset
#define QMAP_PROCESS_NE_MEANTHY     0
#define QMAP_PROCESS_NE_MEANTHC     16
#define QMAP_PROCESS_NE_SADMINY     32
#define QMAP_PROCESS_NE_SADMINC     48
#define QMAP_PROCESS_NE_MEANY       64
#define QMAP_PROCESS_NE_MEANC       80
#define QMAP_PROCESS_NE_SIZE        96
#if defined (SCLOS_TYPE_LINUX_KERNEL)

#define VPE_ERR(_fmt, _args...)       printk(KERN_WARNING _fmt, ## _args)
#define VPE_DBG(dbglv, _fmt, _args...)             \
    do                                             \
    if(dbglv)                                      \
    {                                              \
            printk(KERN_WARNING _fmt, ## _args);       \
    }while(0)

#else
#define VPE_ERR(_fmt, _args...)        UartSendTrace(_fmt,##_args)
#define VPE_DBG(dbglv, _fmt, _args...)             \
    do                                             \
    if(dbglv)                                      \
    {                                              \
            UartSendTrace(_fmt, ## _args);       \
    }while(0)

#endif
//-------------------------------------------------------------------------------------------------
//  Enum
//-------------------------------------------------------------------------------------------------
typedef enum
{
    E_HAL_SCLHVSP_ID_1,      ///< ID_1
    E_HAL_SCLHVSP_ID_2,      ///< ID_2
    E_HAL_SCLHVSP_ID_3,      ///< ID_3
    E_HAL_SCLHVSP_ID_4,      ///< ID_4
    E_HAL_SCLDMA_ID_1,      ///< ID_1
    E_HAL_SCLDMA_ID_2,      ///< ID_2
    E_HAL_SCLDMA_ID_3,      ///< ID_3
    E_HAL_SCLDMA_ID_4,      ///< ID_4
    E_HAL_ID_MAX,      ///< ID_4
} DrvSclIdType_e;
typedef enum
{
    E_HAL_INIT_DEINIT = 0,
    E_HAL_INIT_BY_PROBE,      ///< ID_1
    E_HAL_INIT_BY_CREATE,      ///< ID_2
    E_HAL_INIT_TYPE,      ///< ID_4
} DrvSclInitType_e;

//-------------------------------------------------------------------------------------------------
//  Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    //hw control
    u8 in_reg_blk_sample_step;     //8
    u8 in_reg_blk_meanY_lb;        //16
    u8 in_reg_blk_meanY_ub;        //240
    u8 in_reg_blk_sadY_dc_ub;      //8
    u16 in_reg_blk_sad_ub;         //2560

    //hw output
    u16 in_reg_blk_sad_min_tmp_Y[8];
    u16 in_reg_blk_sad_min_tmp_C[8];
    u32 in_reg_sum_noise_mean_Y[8];
    u32 in_reg_sum_noise_mean_C[8];
    u16 in_reg_count_noise_mean_Y[8];
    u16 in_reg_count_noise_mean_C[8];

    //sw control
    u16 in_blk_sample_num_target;      //32768
    u8 in_noise_mean_th_gain;          //6
    u8 in_inten_blknum_lb;             //16
    u8 in_blk_sad_min_lb;              //32
    u8 in_noise_mean_lb;               //32
    u8 in_learn_rate;                  // 2
    u8 in_dnry_gain;                   // 4
    u8 in_dnrc_gain;                   // 4

    u16 out_reg_noise_mean_th_Y[8];     //192, 192, 192, 192, 192, 192, 192, 192
    u16 out_reg_noise_mean_th_C[8];

    u16 out_reg_blk_sad_min_Y[8];
    u16 out_reg_blk_sad_min_C[8];
    u16 out_reg_noise_mean_Y[8];
    u16 out_reg_noise_mean_C[8];

    //out 3dnr setting
    u8 out_reg_dnry_gain;
    u8 out_reg_dnrc_gain;
    u8 out_reg_lumaLutY[8];
    u8 out_reg_lumaLutC[8];
} NoiseEst_t;

typedef struct
{
    MHalVpeSclCropConfig_t stCropCfg;
    MHalVpeSclWinSize_t stOutputCfg[E_MHAL_SCL_OUTPUT_MAX];
    MHalVpeSclWinSize_t stInputCfg;
    MHalPixelFormat_e      enOutFormat[E_MHAL_SCL_OUTPUT_MAX];
}MHalSclCtxConfig_t;
typedef struct
{
    bool bUsed;
    s32 s32Handle[E_HAL_ID_MAX];
    u16 u16InstId;
    MHalSclCtxConfig_t stCtx;
} MHalSclHandleConfig_t;
typedef struct
{
    bool bUsed;
    s32 s32Handle;
    u16 u16InstId;
    MHalVpeIqWdrRoiReport_t stWdrBuffer;
    void *pvNrBuffer;
    u32 u32BaseAddr[WDR_HIST_BUFFER];  //for wdr roi
    u32 u32VirBaseAddr[WDR_HIST_BUFFER]; // for wdr roi
    u32 u32DlcHistAddr; // for dlc hist
    u32 u32VirDlcHistAddr; // for dlc hist
    bool bEsEn;
    bool bNREn;
    u8 pu8UVM[4];
    u8 u8NLMShift_ESOff;
    u8 pu8NLM_ESOff[16];
    u8 u8NLMShift_ESOn;
    u8 pu8NLM_ESOn[16];
    bool bProcess;
    u16 pu16NE_blk_sad_min_Y[8];
    u16 pu16NE_blk_sad_min_C[8];
    u16 pu16NE_noise_mean_Y[8];
    u16 pu16NE_noise_mean_C[8];
    bool bWDREn;
    bool bWDRActive;
    u8 u8Wdr_Str;
    u8 u8Wdr_Slope;

} MHalIqHandleConfig_t;
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MHalSclHandleConfig_t *pgstScHandler[VPE_HANDLER_INSTANCE_NUM];
MHalIqHandleConfig_t *pgstIqHandler[VPE_HANDLER_INSTANCE_NUM];
bool gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE] = {E_MHAL_VPE_DEBUG_OFF,E_MHAL_VPE_DEBUG_OFF,E_MHAL_VPE_DEBUG_OFF};
char KEY_DMEM_IQ_HIST[20] = "IQROIDLC";
DrvSclInitType_e genScinit = E_HAL_INIT_DEINIT;
#if defined(USE_USBCAM)
MhalInterruptCb _gpMahlIntCb = NULL;
#endif

//-------------------------------------------------------------------------------------------------
//  Functions ISP Internal Calc
//-------------------------------------------------------------------------------------------------
extern void _DrvSclVpeModuleDeInit(void);
extern void _DrvSclVpeModuleInit(void);

void _IQNoiseEstimationDriver(u16 *reg_blk_sad_min_tmp, u32 *reg_sum_noise_mean, u16 *reg_count_noise_mean, NoiseEst_t _reg, u16 *blk_sad_min, u16 *noise_mean, u16 *reg_noise_mean_th)
{
    u16 *blkSADMin = reg_blk_sad_min_tmp;
    u32 *sum_noiseMean = reg_sum_noise_mean;
    u16 *count_noiseMean = reg_count_noise_mean;
    int noiseMean[8];
    int noise_mean_left = 0 , noise_mean_right = 0;
    int delta;
    int i, j;

    //-------------------------------------------------
    // Init estimation (blkSADMin, noiseMean)
    //-------------------------------------------------
    for (i = 0; i < 8; i++)
    {
        // blkSAD_min
        blkSADMin[i] = max(blkSADMin[i], (u16)_reg.in_blk_sad_min_lb);

        // noiseMean
        if (count_noiseMean[i] > _reg.in_inten_blknum_lb)
            noiseMean[i] = sum_noiseMean[i] / max((int)count_noiseMean[i], 1);
        else
            noiseMean[i] = 0;
    }

    //-------------------------------------------------
    // Filling missing segment (noiseMean)
    //-------------------------------------------------
    if (noiseMean[0] == 0)
    {
        for (i = 1; i < 8; i++)
        {
            if (noiseMean[i] != 0)
            {
                noiseMean[0] = noiseMean[i];
                break;
            }
            else if ((i == 7) && (noiseMean[7] == 0))
            {
                noiseMean[0] = _reg.in_noise_mean_lb;
            }
        }
    }

    if (noiseMean[7] == 0)
    {
        for (i = 6; i >= 0; i--)
        {
            if (noiseMean[i] != 0)
            {
                noiseMean[7] = noiseMean[i];
                break;
            }
        }
    }

    for (i = 1; i < 7; i++)
    {
        if (noiseMean[i] == 0)
        {
            for (j = i - 1; j >= 0; j--)
            {
                if (noiseMean[j] != 0)
                {
                    noise_mean_left = noiseMean[j];
                    break;
                }
            }

            for (j = i + 1; j < 8; j++)
            {
                if (noiseMean[j] != 0)
                {
                    noise_mean_right = noiseMean[j];
                    break;
                }
            }

            noiseMean[i] = (noise_mean_left + noise_mean_right) >> 1;
        }
    }

    //--------------------------------------------
    // Noise profile stabilization and output
    //--------------------------------------------
    for (i = 0; i < 8; i++)
    {
        // blkSAD_min
        delta = blkSADMin[i] - blk_sad_min[i];

        if (delta < 0)
            blk_sad_min[i] = blk_sad_min[i] - ((_reg.in_learn_rate * abs(delta)) >> 2);
        else
            blk_sad_min[i] = blk_sad_min[i] + ((_reg.in_learn_rate * delta) >> 2);

        // noiseMean
        delta = noiseMean[i] - noise_mean[i];

        if (delta < 0)
            noise_mean[i] = noise_mean[i] - ((_reg.in_learn_rate * abs(delta)) >> 2);
        else
            noise_mean[i] = noise_mean[i] + ((_reg.in_learn_rate * delta) >> 2);
    }

    //--------------------------------------------
    // Calculate "noiseMean_Th"
    //--------------------------------------------
    for (i = 0; i < 8; i++)
        reg_noise_mean_th[i] = blk_sad_min[i] * _reg.in_noise_mean_th_gain;
}

void _IQCalcNRDnrGainAndLumaLut(u16 *reg_noise_mean_Y, u16 *reg_noise_mean_C, u8 reg_ne_dnry_gain, u8 reg_ne_dnrc_gain, u8 *reg_nr_dnry_gain, u8 *reg_nr_dnrc_gain, u8 *reg_lumaLutY, u8 *reg_lumaLutC)
{
    int noise_level_min_Y = reg_noise_mean_Y[0];
    int noise_level_min_C = reg_noise_mean_C[0];
    int i, a, b;

    //--- Calculate nr_dnry_gain ---
    for (i = 1; i < 8; i++)
    {
        if (reg_noise_mean_Y[i] < noise_level_min_Y)
            noise_level_min_Y = reg_noise_mean_Y[i];

        if (reg_noise_mean_C[i] < noise_level_min_C)
            noise_level_min_C = reg_noise_mean_C[i];
    }

    a = ((reg_ne_dnry_gain << 12) + (noise_level_min_Y >> 1)) / max((int)noise_level_min_Y, 1);
    b = ((reg_ne_dnrc_gain << 12) + (noise_level_min_C >> 1)) / max((int)noise_level_min_C, 1);
    *reg_nr_dnry_gain = minmax(a, 0, 255);
    *reg_nr_dnrc_gain = minmax(b, 0, 255);

    //--- Calculate lumaLut ---
    for (i = 0; i < 8; i++)
    {
        reg_lumaLutY[i] = ((noise_level_min_Y << 4) + (reg_noise_mean_Y[i] >> 1)) / max((int)reg_noise_mean_Y[i], 1);
        reg_lumaLutC[i] = ((noise_level_min_C << 4) + (reg_noise_mean_C[i] >> 1)) / max((int)reg_noise_mean_C[i], 1);
    }
}

void _IQSyncESSetting(void *pCtx)
{
    int i=0;
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler = pCtx;

    //use nlm setting to do edge smooth effect
    if (pstIqHandler->bEsEn) {
        if (pstIqHandler->bNREn) {
            pstIqHandler->u8NLMShift_ESOn = minmax(pstIqHandler->u8NLMShift_ESOff, 3, 15);
            for (i=0; i<16; i++) {
                pstIqHandler->pu8NLM_ESOn[i] = (pstIqHandler->pu8NLM_ESOff[i]>4) ? (pstIqHandler->pu8NLM_ESOff[i]): (4);
            }
        }
        else {
            pstIqHandler->u8NLMShift_ESOn = 3;
            for (i=0; i<16; i++) {
                pstIqHandler->pu8NLM_ESOn[i] = 4;
            }
        }
    }

    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);
}

void _IQBufHLInverse(u8 *pSrc, u16 size)
{
    int i;
    u8 tmp;

    for (i=0; i<size; i+=2)
    {
        tmp = pSrc[i+0];
        pSrc[i+0] = pSrc[i+1];
        pSrc[i+1] = tmp;
    }
}
//-------------------------------------------------------------------------------------------------
//  VPE local function
//-------------------------------------------------------------------------------------------------
void _MHalVpeCleanScInstBuffer(MHalSclHandleConfig_t *pstScHandler)
{
    s16 s16Idx = -1;
    s16 i ;
    //clean buffer
    s16Idx = pstScHandler->u16InstId;
    pstScHandler->bUsed = 0;
    pstScHandler->u16InstId = 0;
    for(i = 0; i < E_HAL_ID_MAX; i++)
    {
        pstScHandler->s32Handle[i] = 0;
    }
    DrvSclOsVirMemFree(pstScHandler);
    pgstScHandler[s16Idx] = NULL;
}
void _MHalVpeIqMemNaming(char *pstIqName,s16 s16Idx)
{
    pstIqName[0] = 48+(((s16Idx&0xFFF)%1000)/100);
    pstIqName[1] = 48+(((s16Idx&0xFFF)%100)/10);
    pstIqName[2] = 48+(((s16Idx&0xFFF)%10));
    pstIqName[3] = '_';
    pstIqName[4] = '\0';
    DrvSclOsStrcat(pstIqName,KEY_DMEM_IQ_HIST);
}
void _MHalVpeCleanIqInstBuffer(MHalIqHandleConfig_t *pstIqHandler)
{
    s16 s16Idx = -1;
    u16 i;
    u16 u16Size = 0;
    char sg_Iq_Roi_name[16];
    //clean buffer
    u16Size = DRV_SCLVIP_IO_WDR_HIST1_BUFFER_SIZE + DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE*3 +DRV_SCLVIP_IO_DLC_HIST_SIZE;
    if(u16Size%0x1000)
    {
        u16Size = ((u16Size/0x1000)+1)*0x1000;
    }
    s16Idx = pstIqHandler->u16InstId;
    pstIqHandler->bUsed = 0;
    pstIqHandler->u16InstId = 0;
    pstIqHandler->s32Handle = 0;
    DrvSclOsVirMemFree(pstIqHandler->pvNrBuffer);
    pstIqHandler->pvNrBuffer = NULL;
    _MHalVpeIqMemNaming(sg_Iq_Roi_name,s16Idx);
    DrvSclOsDirectMemFree
        (sg_Iq_Roi_name,u16Size,(void *)pgstIqHandler[s16Idx]->u32VirBaseAddr[0],pgstIqHandler[s16Idx]->u32BaseAddr[0]);
    for(i=0;i<ROI_WINDOW_MAX;i++)
    {
        pgstIqHandler[s16Idx]->u32BaseAddr[i] = 0;
        pgstIqHandler[s16Idx]->u32VirBaseAddr[i] = 0;
        pstIqHandler->stWdrBuffer.u32Y[i] =0;
    }
    DrvSclOsVirMemFree(pstIqHandler);
    pgstIqHandler[s16Idx] = NULL;
}
void _MHalVpeIqKeepCmdqFunction(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    if(pstCmdQInfo)
    {
        _DrvSclVipIoKeepCmdqFunction(pstCmdQInfo);
    }
}
void _MHalVpeSclKeepCmdqFunction(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    _DrvSclDmaIoKeepCmdqFunction(pstCmdQInfo);
}
void _MHalVpeKeepAllocFunction(const MHalAllocPhyMem_t *pstAlloc)
{
    DrvSclOsAllocPhyMem_t pf;
    DrvSclOsMemset(&pf,0,sizeof(DrvSclOsAllocPhyMem_t));
    if(pstAlloc)
    {
        pf.SclOsAlloc = (void *)pstAlloc->alloc;
        pf.SclOsFree = (void *)pstAlloc->free;
        pf.SclOsmap = (void *)pstAlloc->map;
        pf.SclOsunmap = (void *)pstAlloc->unmap;
        pf.SclOsflush_cache= (void *)pstAlloc->flush_cache;
        DrvSclOsKeepAllocFunction(&pf);
    }
    else
    {
        pf.SclOsAlloc = NULL;
        pf.SclOsFree = NULL;
        pf.SclOsmap = NULL;
        pf.SclOsunmap = NULL;
        pf.SclOsflush_cache= NULL;
        DrvSclOsKeepAllocFunction(&pf);
    }
}
bool _MHalVpeReqmemConfig(s32 s32Handle,const MHalVpeSclWinSize_t *stMaxWin)
{
    DrvSclHvspIoReqMemConfig_t stReqMemCfg;
    DrvSclOsMemset(&stReqMemCfg,0,sizeof(DrvSclHvspIoReqMemConfig_t));
    stReqMemCfg.u16Pitch   = stMaxWin->u16Width;
    stReqMemCfg.u16Vsize   = stMaxWin->u16Height;
    stReqMemCfg.u32MemSize = (stMaxWin->u16Width * stMaxWin->u16Height * 4);
    FILL_VERCHK_TYPE(stReqMemCfg, stReqMemCfg.VerChk_Version, stReqMemCfg.VerChk_Size, DRV_SCLHVSP_VERSION);
    if(_DrvSclHvspIoReqmemConfig(s32Handle,&stReqMemCfg))
    {
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
DrvSclHvspIoColorType_e _MHalVpeTranHvspColorFormat(MHalPixelFormat_e enType)
{
    DrvSclHvspIoColorType_e enColor;
    switch(enType)
    {
        case E_MHAL_PIXEL_FRAME_YUV422_YUYV:
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_422:
            enColor = E_DRV_SCLHVSP_IO_COLOR_YUV422;
            break;
        case E_MHAL_PIXEL_FRAME_ARGB8888:
            enColor = E_DRV_SCLHVSP_IO_COLOR_RGB;
            break;
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420:
        case E_MHAL_PIXEL_FRAME_YUV_MST_420:
            enColor = E_DRV_SCLHVSP_IO_COLOR_YUV420;
            break;
        default :
            enColor = E_DRV_SCLHVSP_IO_COLOR_NUM;
            break;
    }
    return enColor;
}
DrvSclHvspIoColorType_e _MHalVpeTranMdwinColorFormat(MHalPixelFormat_e enType,MHalPixelCompressMode_e enCompress)
{
    DrvSclHvspIoColorType_e enColor;
    switch(enType)
    {
        case E_MHAL_PIXEL_FRAME_YUV422_YUYV:
            if(enCompress==E_MHAL_COMPRESS_MODE_NONE)
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV422;
            }
            else
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV422CE;
            }
            break;
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_422:
            if(enCompress==E_MHAL_COMPRESS_MODE_NONE)
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV422;
            }
            else
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV422CE;
            }
            break;
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            if(enCompress==E_MHAL_COMPRESS_MODE_NONE)
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV420;
            }
            else
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV420CE;
            }
            break;
        case E_MHAL_PIXEL_FRAME_YUV_MST_420:
            if(enCompress==E_MHAL_COMPRESS_MODE_NONE)
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV420;
            }
            else
            {
                enColor = E_DRV_MDWIN_IO_COLOR_YUV420CE;
            }
            break;
        case E_MHAL_PIXEL_FRAME_ARGB8888:
                enColor = E_DRV_MDWIN_IO_COLOR_ARGB;
            break;
        case E_MHAL_PIXEL_FRAME_ABGR8888:
                enColor = E_DRV_MDWIN_IO_COLOR_ABGR;
            break;
        default :
            enColor = E_DRV_SCLDMA_IO_COLOR_NUM;
            break;
    }
    return enColor;
}
DrvSclHvspIoColorType_e _MHalVpeTranDmaColorFormat(MHalPixelFormat_e enType,MHalPixelCompressMode_e enCompress)
{
    DrvSclHvspIoColorType_e enColor;
    switch(enType)
    {
        case E_MHAL_PIXEL_FRAME_YUV422_YUYV:
            enColor = E_DRV_SCLDMA_IO_COLOR_YUV422;
            break;
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_422:
            enColor = E_DRV_SCLDMA_IO_COLOR_YCSep422;
            break;
        case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420:
            enColor = E_DRV_SCLDMA_IO_COLOR_YUV420;
            break;
        default :
            enColor = E_DRV_SCLDMA_IO_COLOR_NUM;
            break;
    }
    return enColor;
}
bool _MHalVpeScIsInputSizeNeedReSetting(MHalSclHandleConfig_t* pCtx ,const MHalVpeSclInputSizeConfig_t *pCfg)
{
    if(((pCfg->u16Height != pCtx->stCtx.stInputCfg.u16Height)&& pCfg->u16Height) ||
        ((pCfg->u16Width != pCtx->stCtx.stInputCfg.u16Width)&& pCfg->u16Width))
    {
        return VPE_RETURN_OK;
    }
    return VPE_RETURN_ERR;
}
bool _MHalVpeScIsOutputSizeNeedReSetting(MHalSclHandleConfig_t* pCtx ,const MHalVpeSclOutputSizeConfig_t *pCfg)
{
    if(((pCfg->u16Height != pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Height)&&
        pCfg->u16Height) ||
        ((pCfg->u16Width != pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Width)&&
        pCfg->u16Width))
    {
        return VPE_RETURN_OK;
    }
    return VPE_RETURN_ERR;
}
bool _MHalVpeScIsCropSizeNeedReSetting(MHalSclHandleConfig_t* pCtx ,const MHalVpeSclCropConfig_t *pCfg)
{
    if(((pCfg->stCropWin.u16Height != pCtx->stCtx.stCropCfg.stCropWin.u16Height)&&pCfg->stCropWin.u16Height) ||
        ((pCfg->stCropWin.u16Width != pCtx->stCtx.stCropCfg.stCropWin.u16Width)&&pCfg->stCropWin.u16Width)||
        (pCfg->stCropWin.u16X != pCtx->stCtx.stCropCfg.stCropWin.u16X)||
        (pCfg->stCropWin.u16Y != pCtx->stCtx.stCropCfg.stCropWin.u16Y)||
        (pCfg->bCropEn != pCtx->stCtx.stCropCfg.bCropEn))
    {
        return VPE_RETURN_OK;
    }
    return VPE_RETURN_ERR;
}
void _MHalVpeScInputSizeReSetting(MHalSclHandleConfig_t* pCtx ,u32 u32Width,u32 u32Height)
{
    pCtx->stCtx.stInputCfg.u16Height = u32Height;
    pCtx->stCtx.stInputCfg.u16Width = u32Width;
    if(pCtx->stCtx.stCropCfg.bCropEn)
    {
        if(pCtx->stCtx.stCropCfg.stCropWin.u16Height >= u32Height)
        {
            pCtx->stCtx.stCropCfg.stCropWin.u16Height = u32Height;
        }
        if(pCtx->stCtx.stCropCfg.stCropWin.u16Width >= u32Width)
        {
            pCtx->stCtx.stCropCfg.stCropWin.u16Width = u32Width;
        }
        if(pCtx->stCtx.stCropCfg.stCropWin.u16X +pCtx->stCtx.stCropCfg.stCropWin.u16Width > u32Width)
        {
            pCtx->stCtx.stCropCfg.stCropWin.u16X = 0;
        }
        if(pCtx->stCtx.stCropCfg.stCropWin.u16Y +pCtx->stCtx.stCropCfg.stCropWin.u16Height> u32Height)
        {
            pCtx->stCtx.stCropCfg.stCropWin.u16Y = 0;
        }
        if(pCtx->stCtx.stCropCfg.stCropWin.u16Width == u32Width && pCtx->stCtx.stCropCfg.stCropWin.u16Height == u32Height)
        {
            pCtx->stCtx.stCropCfg.bCropEn = 0;
        }
    }
    else
    {
        pCtx->stCtx.stCropCfg.stCropWin.u16Height = u32Height;
        pCtx->stCtx.stCropCfg.stCropWin.u16Width = u32Width;
    }
}
void _MHalVpeScCropSizeReSetting(MHalSclHandleConfig_t* pCtx ,const MHalVpeSclCropConfig_t *stCropCfg)
{
    if(stCropCfg->stCropWin.u16Height)
    {
        pCtx->stCtx.stCropCfg.stCropWin.u16Height = stCropCfg->stCropWin.u16Height;
    }
    if(stCropCfg->stCropWin.u16Width)
    {
        pCtx->stCtx.stCropCfg.stCropWin.u16Width = stCropCfg->stCropWin.u16Width;
    }
    if(pCtx->stCtx.stCropCfg.stCropWin.u16Width%2)
    {
        pCtx->stCtx.stCropCfg.stCropWin.u16Width--;
    }
    pCtx->stCtx.stCropCfg.stCropWin.u16X = stCropCfg->stCropWin.u16X;
    if(pCtx->stCtx.stCropCfg.stCropWin.u16X%2)
    {
        pCtx->stCtx.stCropCfg.stCropWin.u16X++;
    }
    pCtx->stCtx.stCropCfg.stCropWin.u16Y = stCropCfg->stCropWin.u16Y;
    pCtx->stCtx.stCropCfg.bCropEn= stCropCfg->bCropEn;
}
void _MHalVpeScOutputSizeReSetting(MHalSclHandleConfig_t* pCtx, const MHalVpeSclOutputSizeConfig_t *pCfg)
{
    pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Height = pCfg->u16Height;
    pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Width = pCfg->u16Width;
}
void _MHalVpeFillHVSP1ScaleCfg(MHalSclHandleConfig_t* pCtx,DrvSclHvspIoScalingConfig_t *pstCfg)
{
    pstCfg->u16Src_Width = pCtx->stCtx.stInputCfg.u16Width;
    pstCfg->u16Src_Height = pCtx->stCtx.stInputCfg.u16Height;
    pstCfg->u16Dsp_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Width;
    pstCfg->u16Dsp_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Height;
    pstCfg->bCropEn = pCtx->stCtx.stCropCfg.bCropEn;
    pstCfg->stCropWin.u16X = pCtx->stCtx.stCropCfg.stCropWin.u16X;
    pstCfg->stCropWin.u16Y = pCtx->stCtx.stCropCfg.stCropWin.u16Y;
    pstCfg->stCropWin.u16Width = pCtx->stCtx.stCropCfg.stCropWin.u16Width;
    pstCfg->stCropWin.u16Height = pCtx->stCtx.stCropCfg.stCropWin.u16Height;
    if(((pstCfg->u16Src_Width != pstCfg->stCropWin.u16Width)||
        (pstCfg->u16Src_Height != pstCfg->stCropWin.u16Height))&&!pstCfg->bCropEn)
    {
        pstCfg->bCropEn = 1;
    }
}
void _MHalVpeFillHVSP2ScaleCfg(MHalSclHandleConfig_t* pCtx,DrvSclHvspIoScalingConfig_t *pstCfg)
{
    pstCfg->u16Src_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Width;
    pstCfg->u16Src_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Height;
    pstCfg->u16Dsp_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT1].u16Width;
    pstCfg->u16Dsp_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT1].u16Height;
}
void _MHalVpeFillHVSP3ScaleCfg(MHalSclHandleConfig_t* pCtx,DrvSclHvspIoScalingConfig_t *pstCfg)
{
    pstCfg->u16Src_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Width;
    pstCfg->u16Src_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Height;
    pstCfg->u16Dsp_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT2].u16Width;
    pstCfg->u16Dsp_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT2].u16Height;
}
void _MHalVpeFillHVSP4ScaleCfg(MHalSclHandleConfig_t* pCtx,DrvSclHvspIoScalingConfig_t *pstCfg)
{
    pstCfg->u16Src_Width = pCtx->stCtx.stCropCfg.stCropWin.u16Width;
    pstCfg->u16Src_Height = pCtx->stCtx.stCropCfg.stCropWin.u16Height;
    pstCfg->u16Dsp_Width = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT3].u16Width;
    pstCfg->u16Dsp_Height = pCtx->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT3].u16Height;
}
void _MHalVpeFillHVSPScaleCfg(DrvSclIdType_e enId,MHalSclHandleConfig_t* pCtx,DrvSclHvspIoScalingConfig_t *pstCfg)
{
    switch(enId)
    {
        case E_HAL_SCLHVSP_ID_1:
            _MHalVpeFillHVSP1ScaleCfg(pCtx,pstCfg);
            break;
        case E_HAL_SCLHVSP_ID_2:
            _MHalVpeFillHVSP2ScaleCfg(pCtx,pstCfg);
            break;
        case E_HAL_SCLHVSP_ID_3:
            _MHalVpeFillHVSP3ScaleCfg(pCtx,pstCfg);
            break;
        case E_HAL_SCLHVSP_ID_4:
            _MHalVpeFillHVSP4ScaleCfg(pCtx,pstCfg);
            break;
        default:
            VPE_ERR("[VPESCL]%s ID Not Support\n",__FUNCTION__);
            break;
    }
}
void _MHalVpeScScalingConfig(DrvSclIdType_e enId,MHalSclHandleConfig_t* pCtx)
{
    DrvSclHvspIoScalingConfig_t stHvspScaleCfg;
    DrvSclOsMemset(&stHvspScaleCfg,0,sizeof(DrvSclHvspIoScalingConfig_t));
    _MHalVpeFillHVSPScaleCfg(enId,pCtx,&stHvspScaleCfg);
    stHvspScaleCfg = FILL_VERCHK_TYPE(stHvspScaleCfg, stHvspScaleCfg.VerChk_Version,
        stHvspScaleCfg.VerChk_Size,DRV_SCLHVSP_VERSION);
    _DrvSclHvspIoSetScalingConfig(pCtx->s32Handle[enId], &stHvspScaleCfg);
}
void _MHalVpeSclOutputPort0SizeConfig(void *pCtx)
{
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_1,pCtx);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_2,pCtx);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_3,pCtx);
}
void _MHalVpeSclOutputPort1SizeConfig(void *pCtx)
{
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_2,pCtx);
}
void _MHalVpeSclOutputPort2SizeConfig(void *pCtx)
{
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_3,pCtx);
}
void _MHalVpeSclOutputPort3SizeConfig(void *pCtx)
{
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_4,pCtx);
}
void _MHalVpeSclOutputSizeConfig(void *pCtx, const MHalVpeSclOutputSizeConfig_t *pCfg)
{
    switch(pCfg->enOutPort)
    {
        case E_MHAL_SCL_OUTPUT_PORT0:
            _MHalVpeSclOutputPort0SizeConfig(pCtx);
            break;
        case E_MHAL_SCL_OUTPUT_PORT1:
            _MHalVpeSclOutputPort1SizeConfig(pCtx);
            break;
        case E_MHAL_SCL_OUTPUT_PORT2:
            _MHalVpeSclOutputPort2SizeConfig(pCtx);
            break;
        case E_MHAL_SCL_OUTPUT_PORT3:
            _MHalVpeSclOutputPort3SizeConfig(pCtx);
            break;
        default:
            VPE_ERR("[VPESCL]%s PORT Not Support\n",__FUNCTION__);
            break;
    }
}
bool _MHalVpeDmaConfig
    (DrvSclIdType_e enId,MHalSclHandleConfig_t* pCtx, const MHalVpeSclOutputDmaConfig_t *pCfg)
{
    DrvSclDmaIoBufferConfig_t stSCLDMACfg;
    bool bRet = VPE_RETURN_OK;
    DrvSclOsMemset(&stSCLDMACfg,0,sizeof(DrvSclDmaIoBufferConfig_t));
    stSCLDMACfg.enBufMDType = E_DRV_SCLDMA_IO_BUFFER_MD_SINGLE;
    stSCLDMACfg.enMemType   = E_DRV_SCLDMA_IO_MEM_FRM;
    if(pCfg->enOutPort!=E_MHAL_SCL_OUTPUT_PORT3)
    {
        stSCLDMACfg.enColorType = _MHalVpeTranDmaColorFormat(pCfg->enOutFormat,pCfg->enCompress);
    }
    else
    {
        stSCLDMACfg.enColorType = _MHalVpeTranMdwinColorFormat(pCfg->enOutFormat,pCfg->enCompress);
    }
    stSCLDMACfg.u16Height   = pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Height;
    stSCLDMACfg.u16Width    = pCtx->stCtx.stOutputCfg[pCfg->enOutPort].u16Width;
    stSCLDMACfg.u16BufNum    = 1;
    stSCLDMACfg.u8Flag = 0;
    stSCLDMACfg.bHFlip = 0;
    stSCLDMACfg.bVFlip = 0;
    if(pCfg->enCompress && pCfg->enOutPort!=E_MHAL_SCL_OUTPUT_PORT3)
    {
        VPE_ERR("[VPESCL]%s Compress Not Support\n",__FUNCTION__);
        bRet = VPE_RETURN_ERR;
    }

    if(stSCLDMACfg.enColorType ==E_DRV_SCLDMA_IO_COLOR_NUM)
    {
        VPE_ERR("[VPESCL]%s COLOR Not Support\n",__FUNCTION__);
        bRet = VPE_RETURN_ERR;
    }
    else
    {
        stSCLDMACfg = FILL_VERCHK_TYPE(stSCLDMACfg, stSCLDMACfg.VerChk_Version, stSCLDMACfg.VerChk_Size,DRV_SCLDMA_VERSION);
        _DrvSclDmaIoSetOutBufferConfig(pCtx->s32Handle[enId], &stSCLDMACfg);
    }
    return bRet;
}
bool _MHalVpeHvspInConfig
    (DrvSclIdType_e enId,MHalSclHandleConfig_t* pCtx, const MHalVpeSclInputSizeConfig_t *pCfg)
{
    DrvSclHvspIoInputConfig_t   stHvspInCfg;
    DrvSclOsMemset(&stHvspInCfg,0,sizeof(DrvSclHvspIoInputConfig_t));
    stHvspInCfg.stCaptureWin.u16X = 0;
    stHvspInCfg.stCaptureWin.u16Y = 0;
    stHvspInCfg.stCaptureWin.u16Width = pCfg->u16Width;
    stHvspInCfg.stCaptureWin.u16Height = pCfg->u16Height;
    stHvspInCfg.enColor = _MHalVpeTranHvspColorFormat(pCfg->ePixelFormat);
    if(enId ==E_HAL_SCLHVSP_ID_1)
    {
        if(stHvspInCfg.enColor == E_DRV_SCLHVSP_IO_COLOR_RGB)
        {
            stHvspInCfg.enSrcType = E_DRV_SCLHVSP_IO_SRC_PAT_TGEN;
        }
        else
        {
            stHvspInCfg.enSrcType = E_DRV_SCLHVSP_IO_SRC_ISP;
        }
    }
    else
    {
        stHvspInCfg.enSrcType = E_DRV_SCLHVSP_IO_SRC_HVSP;
    }
    stHvspInCfg = FILL_VERCHK_TYPE(stHvspInCfg, stHvspInCfg.VerChk_Version, stHvspInCfg.VerChk_Size,DRV_SCLHVSP_VERSION);
    if(_DrvSclHvspIoSetInputConfig(pCtx->s32Handle[enId], &stHvspInCfg))
    {
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
void _MHalVpeDeInit(void)
{
    if(genScinit== E_HAL_INIT_BY_CREATE)
    //if(genScinit)
    {
        VPE_ERR("[VPESCL]%s Remove start\n",__FUNCTION__);
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_VIP)))
        {
            _DrvSclVipIoDeInit();
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_4)))
        {
            _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_4);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_3)))
        {
            _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_3);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_2)))
        {
            _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_2);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_1)))
        {
            _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_1);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_4)))
        {
            _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_4);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_3)))
        {
            _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_3);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_2)))
        {
            _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_2);
        }
        if((DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_1)))
        {
            _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_1);
        }
        _DrvSclVpeModuleDeInit();
        genScinit = E_HAL_INIT_DEINIT;
    }
}
void _MHalVpeInit(void)
{
    u16 i;
    if(genScinit == 0)
    {
        genScinit = E_HAL_INIT_BY_PROBE;
        for(i = 0; i < VPE_HANDLER_INSTANCE_NUM; i++)
        {
            pgstScHandler[i] = NULL;
            pgstIqHandler[i] = NULL;
        }
        if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_ALL)==E_DRV_SCLOS_INIT_ALL))
        {
            //init all
            _DrvSclVpeModuleInit();
            DrvSclOsSetAccessRegMode(E_DRV_SCLOS_AccessReg_CPU);
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_1)))
            {
                _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_1);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_HVSP_1);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_2)))
            {
                _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_2);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_HVSP_2);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_3)))
            {
                _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_3);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_HVSP_3);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP_4)))
            {
                _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_4);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_HVSP_4);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_1)))
            {
                _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_1);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_DMA_1);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_2)))
            {
                _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_2);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_DMA_2);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_3)))
            {
                _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_3);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_DMA_3);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA_4)))
            {
                _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_4);
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_DMA_4);
            }
            if(!(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_VIP)))
            {
                _DrvSclVipIoInit();
                DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_VIP);
            }
            genScinit = E_HAL_INIT_BY_CREATE;
        }

    }
}
bool _MHalVpeFindEmptyInst(s16 *s16Idx)
{
    u16 i;
    for(i = 0; i < VPE_HANDLER_INSTANCE_NUM; i++)
    {
        if(pgstScHandler[i] == NULL)
        {
            pgstScHandler[i] = DrvSclOsVirMemalloc(sizeof(MHalSclHandleConfig_t));
            if(pgstScHandler[i]==NULL)
            {
                return VPE_RETURN_ERR;
            }
            *s16Idx = i;
            break;
        }
    }
    return VPE_RETURN_OK;
}
bool _MHalVpeFindEmptyIqInst(s16 *s16Idx)
{
    u16 i;
    for(i = 0; i < VPE_HANDLER_INSTANCE_NUM; i++)
    {
        if(pgstIqHandler[i] == NULL)
        {
            pgstIqHandler[i] = DrvSclOsVirMemalloc(sizeof(MHalIqHandleConfig_t));
            if(pgstIqHandler[i]==NULL)
            {
                return VPE_RETURN_ERR;
            }
            *s16Idx = i;
            break;
        }
    }
    return VPE_RETURN_OK;
}
bool _MHalVpeCloseDevice(s32 *s32Handle)
{
    _DrvSclHvspIoRelease(s32Handle[E_HAL_SCLHVSP_ID_1]);
    _DrvSclHvspIoRelease(s32Handle[E_HAL_SCLHVSP_ID_2]);
    _DrvSclHvspIoRelease(s32Handle[E_HAL_SCLHVSP_ID_3]);
    _DrvSclHvspIoRelease(s32Handle[E_HAL_SCLHVSP_ID_4]);
    _DrvSclDmaIoRelease(s32Handle[E_HAL_SCLDMA_ID_1]);
    _DrvSclDmaIoRelease(s32Handle[E_HAL_SCLDMA_ID_2]);
    _DrvSclDmaIoRelease(s32Handle[E_HAL_SCLDMA_ID_3]);
    _DrvSclDmaIoRelease(s32Handle[E_HAL_SCLDMA_ID_4]);
    return VPE_RETURN_OK;
}
bool _MHalVpeOpenIqDevice(s16 s16Idx)
{
    u16 i;
    char sg_Iq_Roi_name[16];
    u16 u16Size = 0;
    u16Size = DRV_SCLVIP_IO_WDR_HIST1_BUFFER_SIZE + DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE*3 +DRV_SCLVIP_IO_DLC_HIST_SIZE;
    if(u16Size%0x1000)
    {
        u16Size = ((u16Size/0x1000)+1)*0x1000;
    }
    pgstIqHandler[s16Idx]->bUsed = 1;
    pgstIqHandler[s16Idx]->u16InstId = s16Idx;
    pgstIqHandler[s16Idx]->s32Handle = _DrvSclVipIoOpen(E_DRV_SCLVIP_IO_ID_1);
    pgstIqHandler[s16Idx]->pvNrBuffer = DrvSclOsVirMemalloc(DRV_SCLVIP_IO_NR_SIZE);
    if(pgstIqHandler[s16Idx]->pvNrBuffer ==NULL)
    {
        VPE_ERR("[VPE]%s Alloc pvNrBuffer fail\n",__FUNCTION__);
        return VPE_RETURN_ERR;
    }
    else
    {
        VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2),"[VPE]%s Alloc pvNrBuffer %lx\n",__FUNCTION__,(u32)pgstIqHandler[s16Idx]->pvNrBuffer);
    }
    _MHalVpeIqMemNaming(sg_Iq_Roi_name,s16Idx);
    pgstIqHandler[s16Idx]->u32VirBaseAddr[0] =
        (u32)DrvSclOsDirectMemAlloc(sg_Iq_Roi_name,(u32)u16Size,(DrvSclOsDmemBusType_t *)&pgstIqHandler[s16Idx]->u32BaseAddr[0]);
    pgstIqHandler[s16Idx]->u32VirBaseAddr[1] = pgstIqHandler[s16Idx]->u32VirBaseAddr[0]+DRV_SCLVIP_IO_WDR_HIST1_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32VirBaseAddr[2] = pgstIqHandler[s16Idx]->u32VirBaseAddr[1]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32VirBaseAddr[3] = pgstIqHandler[s16Idx]->u32VirBaseAddr[2]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32BaseAddr[1] = pgstIqHandler[s16Idx]->u32BaseAddr[0]+DRV_SCLVIP_IO_WDR_HIST1_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32BaseAddr[2] = pgstIqHandler[s16Idx]->u32BaseAddr[1]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32BaseAddr[3] = pgstIqHandler[s16Idx]->u32BaseAddr[2]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    //alloc dlc buffer
    pgstIqHandler[s16Idx]->u32DlcHistAddr = pgstIqHandler[s16Idx]->u32BaseAddr[3]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    pgstIqHandler[s16Idx]->u32VirDlcHistAddr = pgstIqHandler[s16Idx]->u32VirBaseAddr[3]+DRV_SCLVIP_IO_WDR_HIST_BUFFER_SIZE;
    for(i=0;i<ROI_WINDOW_MAX;i++)
    {
        pgstIqHandler[s16Idx]->stWdrBuffer.u32Y[i] =0;
    }
    if(pgstIqHandler[s16Idx]->s32Handle==-1)
    {
        VPE_ERR("[VPE]%s Open Device handler fail\n",__FUNCTION__);
        _MHalVpeCleanIqInstBuffer(pgstIqHandler[s16Idx]);
        return VPE_RETURN_ERR;
    }
    //init iq
    pgstIqHandler[s16Idx]->bEsEn = 0;
    pgstIqHandler[s16Idx]->bNREn = 0;
    pgstIqHandler[s16Idx]->bProcess = 0;
    pgstIqHandler[s16Idx]->pu8UVM[0] = 0x20;
    pgstIqHandler[s16Idx]->pu8UVM[1] = 0x00;
    pgstIqHandler[s16Idx]->pu8UVM[2] = 0x00;
    pgstIqHandler[s16Idx]->pu8UVM[3] = 0x20;
    pgstIqHandler[s16Idx]->u8NLMShift_ESOn = 0;
    pgstIqHandler[s16Idx]->u8NLMShift_ESOff = 0;
    for (i=0; i<16; i++) {
        pgstIqHandler[s16Idx]->pu8NLM_ESOn[i] = 0;
        pgstIqHandler[s16Idx]->pu8NLM_ESOff[i] = 0;
    }
    pgstIqHandler[s16Idx]->bWDREn = 0;
    pgstIqHandler[s16Idx]->bWDRActive= 0;
    pgstIqHandler[s16Idx]->u8Wdr_Str= 0;
    pgstIqHandler[s16Idx]->u8Wdr_Slope= 0;

    return VPE_RETURN_OK;
}

bool _MHalVpeOpenDevice(s16 s16Idx, const MHalVpeSclWinSize_t *stMaxWin)
{
    u16 i;
    pgstScHandler[s16Idx]->bUsed = 1;
    pgstScHandler[s16Idx]->u16InstId = s16Idx;
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLHVSP_ID_1] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_1);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLHVSP_ID_2] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_2);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLHVSP_ID_3] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_3);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLHVSP_ID_4] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_4);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLDMA_ID_1] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_1);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLDMA_ID_2] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_2);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLDMA_ID_3] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_3);
    pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLDMA_ID_4] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_4);
    for(i = 0; i < E_HAL_ID_MAX; i++)
    {
        if(pgstScHandler[s16Idx]->s32Handle[i]==-1)
        {
            VPE_ERR("[VPE]%s Open Device handler fail\n",__FUNCTION__);
            _MHalVpeCloseDevice(pgstScHandler[s16Idx]->s32Handle);
            _MHalVpeCleanScInstBuffer(pgstScHandler[s16Idx]);
            return VPE_RETURN_ERR;
        }
    }//init Ctx
    pgstScHandler[s16Idx]->stCtx.stCropCfg.bCropEn = 0;
    pgstScHandler[s16Idx]->stCtx.stCropCfg.stCropWin.u16Height = stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stCropCfg.stCropWin.u16Width = stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.stCropCfg.stCropWin.u16X = 0;
    pgstScHandler[s16Idx]->stCtx.stCropCfg.stCropWin.u16Y = 0;
    pgstScHandler[s16Idx]->stCtx.stInputCfg.u16Height = stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stInputCfg.u16Width = stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Height = stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT0].u16Width = stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT1].u16Height = (stMaxWin->u16Height >720) ? 720 : stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT1].u16Width = (stMaxWin->u16Width > 1280) ? 1280 : stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT2].u16Height = (stMaxWin->u16Height > 1080) ? 1080 : stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT2].u16Width = (stMaxWin->u16Width > 1920) ? 1920 : stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT3].u16Height = stMaxWin->u16Height;
    pgstScHandler[s16Idx]->stCtx.stOutputCfg[E_MHAL_SCL_OUTPUT_PORT3].u16Width = stMaxWin->u16Width;
    pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT0] = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT1] = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT2] = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT3] = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    return VPE_RETURN_OK;
}
bool _MHalVpeCreateInst(s32 *s32Handle)
{
    DrvSclDmaIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclDmaIoLockConfig_t));
    stIoInCfg.ps32IdBuf = s32Handle;
    stIoInCfg.u8BufSize = E_HAL_ID_MAX;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclDmaIoCreateInstConfig(s32Handle[E_HAL_SCLDMA_ID_1],&stIoInCfg))
    {
        VPE_ERR("[VPE]%s Create Inst Fail\n",__FUNCTION__);
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
bool _MHalVpeCreateIqInst(s32 *ps32Handle)
{
    DrvSclVipIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclVipIoLockConfig_t));
    stIoInCfg.ps32IdBuf = ps32Handle;
    stIoInCfg.u8BufSize = 1;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclVipIoCreateInstConfig(*ps32Handle,&stIoInCfg))
    {
        VPE_ERR("[VPE]%s Create Inst Fail\n",__FUNCTION__);
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
bool _MHalVpeDestroyInst(s32 *s32Handle)
{
    DrvSclDmaIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclDmaIoLockConfig_t));
    stIoInCfg.ps32IdBuf = s32Handle;
    stIoInCfg.u8BufSize = E_HAL_ID_MAX;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclDmaIoDestroyInstConfig(s32Handle[E_HAL_SCLDMA_ID_1],&stIoInCfg))
    {
        VPE_ERR("[VPE]%s Destroy Inst Fail\n",__FUNCTION__);
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
bool _MHalVpeDestroyIqInst(s32 *s32Handle)
{
    DrvSclVipIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclVipIoLockConfig_t));
    stIoInCfg.ps32IdBuf = s32Handle;
    stIoInCfg.u8BufSize = 1;
    //free wdr mload
    _DrvSclVipIoFreeWdrMloadBuffer(*s32Handle);
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclVipIoDestroyInstConfig(*s32Handle,&stIoInCfg))
    {
        VPE_ERR("[VPE]%s Destroy Inst Fail\n",__FUNCTION__);
        return VPE_RETURN_ERR;
    }
    return VPE_RETURN_OK;
}
void _MHalVpeSclInstFillProcessCfg
    (DrvSclDmaIoProcessConfig_t *stProcess,DrvSclOutputPort_e enPort,const MHalVpeSclOutputBufferConfig_t *pstIoInCfg)
{
    u8 j;
    stProcess->stCfg.bEn = pstIoInCfg->stCfg[enPort].bEn;
    stProcess->stCfg.enMemType = E_DRV_SCLDMA_IO_MEM_FRM;
    for(j =0;j<3;j++)
    {
        stProcess->stCfg.stBufferInfo.u64PhyAddr[j]=
            pstIoInCfg->stCfg[enPort].stBufferInfo.u64PhyAddr[j];
        stProcess->stCfg.stBufferInfo.u32Stride[j]=
            pstIoInCfg->stCfg[enPort].stBufferInfo.u32Stride[j];
    }
}
void _MHalVpeSclProcess(MHalSclHandleConfig_t *pstScHandler,const MHalVpeSclOutputBufferConfig_t *pBuffer)
{
    DrvSclDmaIoProcessConfig_t stIoCfg;
    DrvSclOsMemset(&stIoCfg,0,sizeof(DrvSclDmaIoProcessConfig_t));
    _MHalVpeSclInstFillProcessCfg(&stIoCfg,E_DRV_SCL_OUTPUT_PORT0,pBuffer);
    FILL_VERCHK_TYPE(stIoCfg, stIoCfg.VerChk_Version, stIoCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    _DrvSclDmaIoInstProcess(pstScHandler->s32Handle[E_HAL_SCLDMA_ID_1],&stIoCfg);
    _MHalVpeSclInstFillProcessCfg(&stIoCfg,E_DRV_SCL_OUTPUT_PORT1,pBuffer);
    FILL_VERCHK_TYPE(stIoCfg, stIoCfg.VerChk_Version, stIoCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    _DrvSclDmaIoInstProcess(pstScHandler->s32Handle[E_HAL_SCLDMA_ID_2],&stIoCfg);
    _MHalVpeSclInstFillProcessCfg(&stIoCfg,E_DRV_SCL_OUTPUT_PORT2,pBuffer);
    FILL_VERCHK_TYPE(stIoCfg, stIoCfg.VerChk_Version, stIoCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    _DrvSclDmaIoInstProcess(pstScHandler->s32Handle[E_HAL_SCLDMA_ID_3],&stIoCfg);
    _MHalVpeSclInstFillProcessCfg(&stIoCfg,E_DRV_SCL_OUTPUT_PORT3,pBuffer);
    FILL_VERCHK_TYPE(stIoCfg, stIoCfg.VerChk_Version, stIoCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    _DrvSclDmaIoInstProcess(pstScHandler->s32Handle[E_HAL_SCLDMA_ID_4],&stIoCfg);
    _DrvSclDmaIoInstFlip(pstScHandler->s32Handle[E_HAL_SCLDMA_ID_1]);
}
void _MHalVpeSclProcessDbg(MHalSclHandleConfig_t *pstScHandler,const MHalVpeSclOutputBufferConfig_t *pBuffer)
{
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL5), "[VPESCL]%s Inst %hu Prot0 En:%hhd PhyAddr:{0x%llx,0x%llx,0x%llx},Stride:{0x%x,0x%x,0x%x}\n",
        __FUNCTION__,pstScHandler->u16InstId,pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].bEn,
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u64PhyAddr[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u64PhyAddr[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u64PhyAddr[2],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u32Stride[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u32Stride[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT0].stBufferInfo.u32Stride[2]);
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL6), "[VPESCL]%s Inst %hu Prot1 En:%hhd PhyAddr:{0x%llx,0x%llx,0x%llx},Stride:{0x%x,0x%x,0x%x}\n",
        __FUNCTION__,pstScHandler->u16InstId,pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].bEn,
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u64PhyAddr[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u64PhyAddr[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u64PhyAddr[2],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u32Stride[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u32Stride[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT1].stBufferInfo.u32Stride[2]);
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL7), "[VPESCL]%s Inst %hu Prot2 En:%hhd PhyAddr:{0x%llx,0x%llx,0x%llx},Stride:{0x%x,0x%x,0x%x}\n",
        __FUNCTION__,pstScHandler->u16InstId,pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].bEn,
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u64PhyAddr[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u64PhyAddr[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u64PhyAddr[2],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u32Stride[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u32Stride[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT2].stBufferInfo.u32Stride[2]);
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL8), "[VPESCL]%s Inst %hu Prot3 En:%hhd PhyAddr:{0x%llx,0x%llx,0x%llx},Stride:{0x%x,0x%x,0x%x}\n",
        __FUNCTION__,pstScHandler->u16InstId,pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].bEn,
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u64PhyAddr[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u64PhyAddr[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u64PhyAddr[2],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u32Stride[0],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u32Stride[1],
        pBuffer->stCfg[E_MHAL_SCL_OUTPUT_PORT3].stBufferInfo.u32Stride[2]);
}
void _MHalVpeSclForISPInputAlign(MHalSclHandleConfig_t *pstScHandler,MHalVpeSclInputSizeConfig_t *pCfg)
{
    if(pCfg->u16Width&0x1F)
    {
        pCfg->u16Width = ((pCfg->u16Width&0xFFE0)+0x20);
        VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL1), "[VPESCL]%s New Width:%hu\n",
            __FUNCTION__,pCfg->u16Width);
    }
}
//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
//ISP
typedef struct
{
    //MHalVpeIspInputFormat_e enInType;
    //MHalVpeIspRotationType_e enRotType;
    MS_U32 uCropX;
    MS_U32 uCropY;
    MS_U32 uImgW;
    MS_U32 uImgH;
    MHalVpeIspRotationType_e uRot;
    MHalPixelFormat_e ePixelFmt;
}MHalVpeCtx_t;

#define BANK_TO_ADDR(bank) (bank<<9)
//#define RIU_ISP_REG(bank,offset) ((bank<<9)+(offset<<2)) //register table to RIU address
#define CMQ_RIU_REG(bank,offset) ((bank<<8)+(offset<<1)) //register address from CMQ
#define CPU_RIU_REG(bank,offset) ((bank<<9)+(offset<<2)) //register address from CPU

#define BANK_ISP0 0x1509
#define BANK_ISP7 0x1510
#define BANK_DMAG0 0x1517
#define BANK_DMA_IRQ 0x153B
#define BANK_SC_FLIP    0x153E
#define BANK_SC_DWIN    0x13F2
#define BANK_SC_MCNR    0x1532
#define BANK_SC_WDR0    0x151B
#define BANK_SC_WDR1    0x151C
#define BANK_SC_ARB_ROI 0x1539

/* IO REMAP */
void *gpVpeIoBase = NULL;
MS_BOOL _VpeIsCmdq(MHAL_CMDQ_CmdqInterface_t* pstCmdQInfo)
{
    if(pstCmdQInfo==NULL|| pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq==NULL)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

void _VpeIspRegW(MHAL_CMDQ_CmdqInterface_t* pstCmdQInfo,unsigned short uBank,unsigned short uOffset,unsigned short uVal)
{
    if(pstCmdQInfo==NULL || pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq==NULL)
        *(unsigned short*)((unsigned long)gpVpeIoBase + CPU_RIU_REG(uBank,uOffset)) = uVal;
    else
        pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,CMQ_RIU_REG(uBank,uOffset),uVal);
}
void _VpeIspRegWMsk(MHAL_CMDQ_CmdqInterface_t* pstCmdQInfo,unsigned short uBank,unsigned short uOffset,unsigned short uVal,unsigned short uMsk)
{
    if(pstCmdQInfo==NULL || pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq==NULL)
    {
        *(unsigned short*)((unsigned long)gpVpeIoBase + CPU_RIU_REG(uBank,uOffset)) =
        ((*(unsigned short*)((unsigned long)gpVpeIoBase + CPU_RIU_REG(uBank,uOffset)) & (~uMsk))|(uVal&uMsk));
    }
    else
        pstCmdQInfo->MHAL_CMDQ_WriteRegCmdqMask(pstCmdQInfo,CMQ_RIU_REG(uBank,uOffset),uVal,uMsk);
}

int _VpeIspRegPollWait(MHAL_CMDQ_CmdqInterface_t* pstCmdQInfo,unsigned short uBank,unsigned short uOffset,unsigned short uMask,unsigned short uVal)
{
    if(pstCmdQInfo==NULL || pstCmdQInfo->MHAL_CMDQ_CmdqPollRegBits==NULL)
        return VPE_RETURN_ERR;
    else
        pstCmdQInfo->MHAL_CMDQ_CmdqPollRegBits(pstCmdQInfo,CMQ_RIU_REG(uBank,uOffset),uVal,uMask,TRUE);
    return 0;
}

unsigned short _VpeIspRegR(unsigned short uBank,unsigned short uOffset)
{
    return *(unsigned short*)((unsigned long)gpVpeIoBase + CPU_RIU_REG(uBank,uOffset));
}

void set_script_wdr_table(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    //nonlinear
    if(!gpVpeIoBase) //workaround before MHalGlobalInit/MHalGlobalDeinit released.
    {
        //gpVpeIoBase = ioremap(0x1F000000,0x400000);//remap whole IO region
        gpVpeIoBase = (void*) 0xFD000000;
    }
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x41, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x42, 0x0155);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x43, 0x02aa);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x44, 0x03bb);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x45, 0x04cc);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x46, 0x05ac);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x47, 0x068b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x48, 0x0745);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x41, 0x07ff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x42, 0x089d);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x43, 0x093a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x44, 0x09c1);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x45, 0x0a48);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x46, 0x0abd);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x47, 0x0b32);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x48, 0x0b99);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1001);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1101);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x41, 0x0bff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x42, 0x0c5a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x43, 0x0cb4);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x44, 0x0d04);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x45, 0x0d54);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x46, 0x0d9c);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x47, 0x0de4);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x48, 0x0e25);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1002);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1102);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x41, 0x0e65);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x42, 0x0ea0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x43, 0x0eda);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x44, 0x0f10);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x45, 0x0f45);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x46, 0x0f76);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x47, 0x0fa6);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x48, 0x0fd3);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1103);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x41, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1004);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x40, 0x1104);
    //sat
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x52, 0x261e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x54, 0x180f);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x56, 0x281e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x58, 0x3730);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x52, 0x4640);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x54, 0x4850);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x56, 0x585f);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x58, 0x7866);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1001);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1101);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x52, 0x8780);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x54, 0x9690);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x56, 0xa8a0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x58, 0xb8af);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1002);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1102);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x52, 0xc8be);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x54, 0xd7d0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x56, 0xe6e0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x58, 0xf8f0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1103);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x52, 0x00ff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1004);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR0, 0x51, 0x1104);
    //f map
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x41, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x42, 0x0115);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x43, 0x020a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x44, 0x02e2);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x45, 0x03a4);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x46, 0x0453);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x47, 0x04f2);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x48, 0x0583);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x41, 0x0608);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x42, 0x0682);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x43, 0x06f2);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x44, 0x075b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x45, 0x07bd);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x46, 0x081a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x47, 0x0872);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x48, 0x08c7);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1001);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1101);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x41, 0x091a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x42, 0x096c);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x43, 0x09be);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x44, 0x0a10);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x45, 0x0a64);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x46, 0x0aba);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x47, 0x0b14);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x48, 0x0b72);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1002);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1102);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x41, 0x0bd5);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x42, 0x0c3e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x43, 0x0cad);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x44, 0x0d24);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x45, 0x0da3);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x46, 0x0e2b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x47, 0x0ebc);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x48, 0x0f58);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1103);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x41, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1004);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x40, 0x1104);
    //alpha map
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x52, 0x059e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x53, 0x083d);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x54, 0x0bfe);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x55, 0x0e9b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x56, 0x0f9e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x57, 0x0fe8);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x58, 0x0ffa);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x59, 0x0ffe);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x52, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x53, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x54, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x55, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x56, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x57, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x58, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x59, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1001);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1101);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x52, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x53, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x54, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x55, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x56, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x57, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x58, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x59, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1002);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1102);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x52, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x53, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x54, 0x0ffe);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x55, 0x0ffb);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x56, 0x0fee);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x57, 0x0fc0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x58, 0x0f2c);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x59, 0x0da9);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1103);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x52, 0x0b04);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1004);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x51, 0x1104);
    //linear
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x63, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x64, 0x002d);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x65, 0x0059);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x66, 0x008a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x67, 0x00ba);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x68, 0x00f0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x69, 0x0125);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x6A, 0x0160);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x63, 0x019a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x64, 0x01db);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x65, 0x021b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x66, 0x0263);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x67, 0x02ab);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x68, 0x02fb);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x69, 0x034b);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x6A, 0x03a6);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1001);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1101);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x63, 0x0400);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x64, 0x0467);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x65, 0x04cd);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x66, 0x0542);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x67, 0x05b7);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x68, 0x063e);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x69, 0x06c4);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x6A, 0x0762);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1002);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1102);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x63, 0x0800);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x64, 0x08ba);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x65, 0x0974);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x66, 0x0a54);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x67, 0x0b33);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x68, 0x0c44);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x69, 0x0d55);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x6A, 0x0eaa);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1103);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x63, 0x0fff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1004);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x62, 0x1104);
}

void set_script_wdr_reg(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x0 , 0x003a);  //[0]: enable
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1 , 0x0040);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2 , 0x0401);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x3 , 0x1006);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x4 , 0x2418);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x5 , 0x00f6);  //[7:0]: strength

    //need to modify image size!!
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x6 , 0x0056);   //tbd..
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x7 , 0x00a0);   //tbd..
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x8 , 0x0202);   //tbd..
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x9 , 0x02fa);   //tbd..
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xA , 0x0199);   //tbd..
    //_VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xB , 0x00ff);   //img w -1
    //_VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xC , 0x01df);   //img h -1

    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xD , 0x0010);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xE , 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0xF , 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x10, 0x007a);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x11, 0x0030);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x12, 0x0010);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x13, 0x0003);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x14, 0x0080);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x15, 0x0030);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x16, 0x0010);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x17, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x18, 0x0080);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x19, 0x0040);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1A, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1B, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1C, 0x0080);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1D, 0x0040);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1E, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x1F, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x20, 0x0090);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x21, 0x0038);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x22, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x23, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x24, 0x0090);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x25, 0x0038);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x26, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x27, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x28, 0x0100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x29, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2A, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2B, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2C, 0x0100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2D, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2E, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x2F, 0x0000);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x30, 0x76c8);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x31, 0x9d78);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x32, 0x80ff);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x33, 0x9494);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x34, 0x00ff);  //[7:0]: sly_slp
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x35, 0x0100);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x36, 0x00b0);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x37, 0x2020);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x38, 0x2020);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x39, 0x2020);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x3A, 0x2020);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x3B, 0x2020);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x7E, 0x0300);
    _VpeIspRegW(pstCmdQInfo, BANK_SC_WDR1, 0x7F, 0x0000);
}
bool MHalVpeCreateIspInstance(const MHalAllocPhyMem_t *pstAlloc ,void **pCtx)
{
    bool bRet = VPE_RETURN_OK;
    *pCtx = (void*)DrvSclOsVirMemalloc(sizeof(MHalVpeCtx_t));
    if(!gpVpeIoBase) //workaround before MHalGlobalInit/MHalGlobalDeinit released.
    {
        //gpVpeIoBase = ioremap(0x1F000000,0x400000);//remap whole IO region
        gpVpeIoBase = (void*) 0xFD000000;
    }
    VPE_ERR("\n\n\n%s\n\n\n",__FUNCTION__);
    return bRet;
}

bool MHalVpeDestroyIspInstance(void *pCtx)
{
    bool bRet = VPE_RETURN_OK;
    DrvSclOsVirMemFree(pCtx);
    return bRet;
}

#define ISP7_REG09_DIPR_NO_ROT     0x0001 //bit 0
#define ISP7_REG09_DIPR_SW_TRIGGER 0x0002 //bit 1
#define ISP7_REG09_DIPR_420_LINEAR 0x0004 //bit 2
#define ISP7_REG09_DIPR_422_ROT    0x0008 //bit 3
#define ISP7_REG09_DIPR_ROT_DIR_CCW 0x0010 //bit 4
#define ISP7_REG09_DIPR_CROP_EN    0x0080 //bit 7
#define ISP7_REG09_DIPR_HW_EN      0x0800 //bit 11
#define _ALIGN32(x) (((x+31)/32)*32)

bool _MHalVpeIspProcessYUV422(void *pCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, const MHalVpeIspVideoInfo_t *pstVidInfo)
{
    bool bRet = VPE_RETURN_OK;
    u32 nBaseAddr;
    MHalVpeCtx_t *ctx = (MHalVpeCtx_t*) pCtx;

	VPE_DBG(VPE_DBG_LV_ISP(),"_MHalVpeIspProcessYUV422 , w=%d, h=%d, Stride=%d\n",ctx->uImgW,ctx->uImgH,pstVidInfo->u32Stride[0]);

    /* Enable ISP for DMA */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x0,0x1,0x1);

    /* YUV422 input mode */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x10,0x0000,MASKOF(reg_isp_sensor_rgb_in));

    /* YC16 bit mode */
    //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x11,0x0050,MASKOF(reg_isp_sensor_yc16bit));

    /* ISP pipe0 raw fetch mode */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x08,0x0002,MASKOF(reg_p0h_rawfetch_mode));

    /* do not reset isp at frame end */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x66,0x0081, MASKOF(reg_fend_rstz_mode)
                                                  |MASKOF(reg_isp_dp_rstz_mode)
                                                  |MASKOF(reg_m3isp_rawdn_clk_force_dis)
                                                  |MASKOF(reg_isp_dp_clk_force_dis)
                   );
    /*  420 to 422 dma disable */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x07,0x0700,MASKOF(reg_mload_miu_sel)
                                                    |MASKOF(reg_40to42_dma_en)//[15] yuv fetch miu sel ; [14] 420 to 422 dma output enable
                                                    |MASKOF(reg_data_store_src_sel) //ch4 test
                                                    );
    /* YUV order:YUYV , YC separate mode:OFF*/
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x11,0x0050,MASKOF(reg_isp_sensor_yc16bit)
                                                    |MASKOF(reg_isp_sensor_yuv_order)
                                                    );

    /* ISP pipe0 rmux sel SIF*/
    _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x04,0x0230,MASKOF(reg_isp_if_rmux_sel));

    /* Crop X*/
    _VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x12,ctx->uCropX);
    /* Crop Y*/
    _VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x13,ctx->uCropY);
    /* Crop W*/
    _VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x14,ctx->uImgW-1);
    /* Crop H*/
    _VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x15,ctx->uImgH-1);

    /* RDMA enable*/
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x00,0x0201);
    /* Width */
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x04,ctx->uImgW-1);
    /* Height */
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x05,ctx->uImgH-1);
    /* Start X */
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x06,ctx->uCropX);
    /* RDMA Pitch */
    //_VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x01,GetRDMAPitch(ctx->ePixelFmt,pstVidInfo->u32Stride[0]));
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x01,((pstVidInfo->u32Stride[0]/2)+15)>>4); //16 data pack 1 miu data

    /* RDMA data mode, and data align 16 bit mode */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_DMAG0,0x18,0x01|0x0,0x7);
    /* DMA buffer address */
    nBaseAddr = pstVidInfo->u64PhyAddr[0];
    nBaseAddr >>= 5;
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x08,nBaseAddr&0xFFFF);
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x09,(nBaseAddr>>16)&0x07FF);

#define MASKOF_RDMA0_DONE (0x0040)
    /* Unmask DMA irq */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_DMA_IRQ,0x035,0x0,MASKOF_RDMA0_DONE);

    /* RDMA trigger */
    _VpeIspRegW(pstCmdQInfo,BANK_DMAG0,0x10,1); //#Bit 2, rdma sw trigger
    /* Wait RDMA done done*/
    //_VpeIspRegPollWait(pstCmdQInfo,BANK_DMA_IRQ,0x038,MASKOF_RDMA0_DONE,0x0040,200000);
	_VpeIspRegPollWait(pstCmdQInfo,BANK_DMA_IRQ,0x038,MASKOF_RDMA0_DONE,0x0040);
    /* Clear DMA interrupt status */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_DMA_IRQ,0x037,0x0040,MASKOF_RDMA0_DONE);
    _VpeIspRegWMsk(pstCmdQInfo,BANK_DMA_IRQ,0x037,0x0,MASKOF_RDMA0_DONE);
    /* Mask DMA status */
    _VpeIspRegWMsk(pstCmdQInfo,BANK_DMA_IRQ,0x035,0x0040,MASKOF_RDMA0_DONE);

    return bRet;
}

bool MHalVpeIspProcess(void *pCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, const MHalVpeIspVideoInfo_t *pstVidInfo)
{
    bool bRet = VPE_RETURN_OK;
    MHalVpeCtx_t *ctx = (MHalVpeCtx_t*) pCtx;
    u16 tmp;
    u16 trigger_ctl = 0;
    int n=0;

    VPE_DBG(VPE_DBG_LV_ISP(), "%s , PixelFmt=%d\n",__FUNCTION__,ctx->ePixelFmt);

    switch(ctx->ePixelFmt)
    {
    case E_MHAL_PIXEL_FRAME_YUV422_YUYV:
        //bRet = VPE_RETURN_ERR;
        VPE_DBG(VPE_DBG_LV_ISP(),"Input YUV422 mode");
        return _MHalVpeIspProcessYUV422(pCtx,pstCmdQInfo,pstVidInfo);
    break;
    case E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420:
        VPE_DBG(VPE_DBG_LV_ISP(),"Input YUV420 mode");
        if(pstVidInfo->u32Stride[0]%32 !=0)
        {
            VPE_DBG(VPE_DBG_LV_ISP(),"Error, Stride must be 32 bytes aligned.");
            SCLOS_BUG();
        }
        if(ctx->uImgW > pstVidInfo->u32Stride[0])
        {
            VPE_DBG(VPE_DBG_LV_ISP(),"Error, Invalid image stride value.");
            SCLOS_BUG();
        }
        if(ctx->uImgW%32)
        {
	        VPE_ERR("Image width:%d is not 32 bytes aligned.\n",ctx->uImgW);
        }
		if(ctx->uImgH%32)
        {
            VPE_DBG(VPE_DBG_LV_ISP(), "Image height:%d is not 32 bytes aligned.\n",ctx->uImgH);
        }
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP7,0x1A,0x1,0x1);
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x10,0x0000); //YUV422 input mode
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x10,0x0000,MASKOF(reg_isp_sensor_rgb_in)); //YUV422 input mode
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x66,0x0081); //do not isp at frame end
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x66,0x0081, MASKOF(reg_fend_rstz_mode)
                                                      |MASKOF(reg_isp_dp_rstz_mode)
                                                      |MASKOF(reg_m3isp_rawdn_clk_force_dis)
                                                      |MASKOF(reg_isp_dp_clk_force_dis)
                                                       ); //do not isp at frame end
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x01,0x816F);
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x01,0x016F, MASKOF(reg_isp_sw_rstz)
        //                                                 |MASKOF(reg_isp_sw_p1_rstz)
        //                                                 |MASKOF(reg_isp_sw_p2_rstz)
        //                                                 |MASKOF(reg_isp_sw_p3_rstz)
        //                                                 );
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x02,0xC000);
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x02,0xC000,MASKOF(reg_db_batch_done)
        //                                                |MASKOF(reg_db_batch_mode)
        //                                                );
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x00,0x0001); //isp enable
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x07,0xC700); //[15] yuv fetch miu sel ; [14] 420 to 422 dma output enable
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x07,0xC700,MASKOF(reg_mload_miu_sel)
                                                        |MASKOF(reg_40to42_dma_en)//[15] yuv fetch miu sel ; [14] 420 to 422 dma output enable
                                                        |MASKOF(reg_data_store_src_sel)	//ch4 test
                                                        );
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP0,0x11,0x0050);
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x11,0x0050,MASKOF(reg_isp_sensor_yc16bit)
                                                        |MASKOF(reg_isp_sensor_yuv_order)
                                                        );

        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x3E,0x08F7); //RDMAY MIU parameter
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x3F,0x0000); //RDMAY output blanking
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x4E,0x08F7); //RDMAC MIU parameter
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x4F,0x0000); //RDMAC output blanking

        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x35,0x0201); //#Bit 0, dmagy rdma enable
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x45,0x0201); //#Bit 0, dmagc rdma enable
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,0x0805); //#Bit 0, dipr_en(Set 1 for no rotate, 0 for rotate)
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x19,0x0001); //arbiter

        if(ctx->uRot == E_MHAL_ISP_ROTATION_90)
        {
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0A,(pstVidInfo->u64PhyAddr[0]>>4)&0xFFFF); //Y buffer , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0B,(pstVidInfo->u64PhyAddr[0]>>20)&0xFFFF); //Y buffer , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0C,(pstVidInfo->u64PhyAddr[1]>>4)&0xFFFF); //UV , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0D,(pstVidInfo->u64PhyAddr[1]>>20)&0xFFFF); //UV , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_SC_FLIP,0x11,0);
            _VpeIspRegWMsk(pstCmdQInfo,BANK_SC_DWIN,0x78,0,0x2);
            trigger_ctl |= ISP7_REG09_DIPR_422_ROT|ISP7_REG09_DIPR_ROT_DIR_CCW;
			trigger_ctl &= ~ISP7_REG09_DIPR_NO_ROT;
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,0x080E); //#Bit 0, dipr_en(Set 1 for no rotate, 0 for rotate), Bit 3 rotate enable
        }
        else if(ctx->uRot == E_MHAL_ISP_ROTATION_270)
        {
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0A,(pstVidInfo->u64PhyAddr[0]>>4)&0xFFFF); //Y buffer , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0B,(pstVidInfo->u64PhyAddr[0]>>20)&0xFFFF); //Y buffer , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0C,(pstVidInfo->u64PhyAddr[1]>>4)&0xFFFF); //UV , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0D,(pstVidInfo->u64PhyAddr[1]>>20)&0xFFFF); //UV , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_SC_FLIP,0x11,0x51);
            _VpeIspRegWMsk(pstCmdQInfo,BANK_SC_DWIN,0x78,0x2,0x2);
            trigger_ctl |= ISP7_REG09_DIPR_422_ROT;
			trigger_ctl &= ~ISP7_REG09_DIPR_NO_ROT;
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,0x080C); //#Bit 0, dipr_en(Set 1 for no rotate, 0 for rotate), Bit 3 rotate enable
        }
        else
        {
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0A,(pstVidInfo->u64PhyAddr[0]>>5)&0xFFFF); //Y buffer , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0B,(pstVidInfo->u64PhyAddr[0]>>21)&0xFFFF); //Y buffer , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0C,(pstVidInfo->u64PhyAddr[1]>>5)&0xFFFF); //UV , L Addr
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0D,(pstVidInfo->u64PhyAddr[1]>>21)&0xFFFF); //UV , H Addr
            _VpeIspRegW(pstCmdQInfo,BANK_SC_FLIP,0x11,0);
            _VpeIspRegWMsk(pstCmdQInfo,BANK_SC_DWIN,0x78,0,0x2);
            trigger_ctl |= ISP7_REG09_DIPR_NO_ROT;
        }

        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x10,pstVidInfo->u32Stride[0]); //Frame Width
        //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x10,ctx->uImgW);           //Frame Width
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x11,ctx->uImgH);               //Frame Height
        /* Stride for MIU access*/
        tmp = (_ALIGN32(ctx->uImgW)/32);
        if(tmp>60)
        {
            for(n=2;n<=13;n++)
            {
                if(tmp%n==0 && tmp/n <=60)
                {
                    tmp /= n;
                    break;
                }
                if(n==13)
                {
                    VPE_ERR("nonsupported image width.");
                }
            }
        }
        tmp-=1;
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x13, ((tmp<<8)&0xFF00)|(tmp&0x00FF)); //request number once for 32 Byte MIU (32/32's factor minus 1 for yuv420 burst length)

        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x0E,0x0010); //ISP ROT output blanking (mul 2)

        //VPE_DBG(VPE_DBG_LV_ISP(),"%s SrcSize W=%d,H=%d, CropSize W=%d,H=%d\n",__FUNCTION__,pstVidInfo->u32Stride[0],ctx->uImgH,ctx->uImgW,ctx->uImgH);
        //VPE_DBG(VPE_DBG_LV_ISP(),"SrcAddrY=0x%x, SrcAddrC=0x%x\n",(unsigned int)(pstVidInfo->u64PhyAddr[0]&0xFFFFFFFF),(unsigned int)(pstVidInfo->u64PhyAddr[1]&0xFFFFFFFF));

        if(pstVidInfo->u32Stride[0] > _ALIGN32(ctx->uImgW))
        {
            /* Enable Crop */
            //VPE_DBG(VPE_DBG_LV_ISP(),"%s SrcSize W=%d,H=%d, CropSize W=%d,H=%d\n",__FUNCTION__,pstVidInfo->u32Stride[0],ctx->uImgH,ctx->uImgW,ctx->uImgH);
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x10,pstVidInfo->u32Stride[0]); //Src Width
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x11,ctx->uImgH); //Src Height
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x14,_ALIGN32(ctx->uImgW)); //Src Width
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x15,ctx->uImgH); //Src Height
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,0x0887); //#Bit 2, rdma sw trigger
            trigger_ctl |= ISP7_REG09_DIPR_HW_EN|ISP7_REG09_DIPR_420_LINEAR|ISP7_REG09_DIPR_SW_TRIGGER|ISP7_REG09_DIPR_CROP_EN;
        }
        else
        {
            /* No Crop */
            //VPE_DBG(VPE_DBG_LV_ISP(),"%s SrcSize W=%d,H=%d, CropSize W=%d,H=%d\n",__FUNCTION__,pstVidInfo->u32Stride[0],ctx->uImgH,ctx->uImgW,ctx->uImgH);
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x10,ctx->uImgW); //Src Width
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x11,ctx->uImgH); //Src Height
            //_VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,0x0807); //#Bit 2, rdma sw trigger
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x14,0); //Src Width
            _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x15,0); //Src Height
            trigger_ctl |= ISP7_REG09_DIPR_HW_EN|ISP7_REG09_DIPR_420_LINEAR|ISP7_REG09_DIPR_SW_TRIGGER;
        }

        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x65,0x0,0x5); //group0 rdma reset enable / wdma reset enable
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x65,0x0,MASKOF(reg_rdpath_swrst));	//group0 rdma reset enable
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x01,0x0,0x1);	//pipe1 reset enable

        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x01,0x0,MASKOF(reg_isp_sw_p1_rstz));	//pipe1 reset enable
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x65,0x5,0x5);
        _VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x65,0x5,MASKOF(reg_rdpath_swrst));	//group0 rdma reset disable
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x01,0x1,0x1);		//pipe reset disable
        //_VpeIspRegWMsk(pstCmdQInfo,BANK_ISP0,0x01,0x1,MASKOF(reg_isp_sw_p1_rstz));	//pipe reset disable
        _VpeIspRegW(pstCmdQInfo,BANK_ISP7,0x09,trigger_ctl); //#Bit 2, rdma sw trigger
    break;
    case E_MHAL_ISP_INPUT_TYPE:
        bRet = VPE_RETURN_ERR;
        VPE_ERR("Unknown input type");
    default:
    break;
    }

    return bRet;
}

bool MHalVpeIspDbgLevel(void *p)
{
    bool bRet = VPE_RETURN_OK;
    gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_ISP] = *((bool *)p);
    VPE_ERR("[VPEISP]%s Dbg Level = %hhx\n",__FUNCTION__,gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_ISP]);
    return bRet;
}



bool MHalVpeIspRotationConfig(void *pCtx, const MHalVpeIspRotationConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalVpeCtx_t *ctx = (MHalVpeCtx_t*) pCtx;
    ctx->uRot = pCfg->enRotType;
	VPE_DBG(VPE_DBG_LV_ISP(),"%s set rot=%d\n",__FUNCTION__,ctx->uRot);
    return bRet;
}

bool MHalVpeIspInputConfig(void *pCtx, const MHalVpeIspInputConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalVpeCtx_t *ctx = (MHalVpeCtx_t*) pCtx;
    ctx->uCropX = 0;
    ctx->uCropY = 0;
    ctx->uImgW = pCfg->u32Width;
    ctx->uImgH = pCfg->u32Height;
    ctx->ePixelFmt = pCfg->ePixelFormat;
    VPE_DBG(VPE_DBG_LV_ISP(),"%s ImgW=%d, ImgH=%d, PixelFmt=%d \n",__FUNCTION__,ctx->uImgW,ctx->uImgH,ctx->ePixelFmt);
    return bRet;
}


//IQ
bool MHalVpeCreateIqInstance(const MHalAllocPhyMem_t *pstAlloc,const MHalVpeSclWinSize_t *pstMaxWin ,void **pCtx)
{
    bool bRet = VPE_RETURN_OK;
    s16 s16Idx = -1;
    DrvSclVipIoSetMaskOnOff_t stCfg;
    MHalVpeIqOnOff_t stOnOff;
    MHalVpeIqConfig_t stIqCfg;
    //keep Alloc
    _MHalVpeKeepAllocFunction(pstAlloc);
    //init
    _MHalVpeInit();
    //find new inst
    bRet = _MHalVpeFindEmptyIqInst(&s16Idx);
    if(bRet == VPE_RETURN_ERR || s16Idx==-1)
    {
        return VPE_RETURN_ERR;
    }
    //open device
    bRet = _MHalVpeOpenIqDevice(s16Idx);
    if(bRet == VPE_RETURN_ERR)
    {
        return bRet;
    }
    //create inst
    bRet = _MHalVpeCreateIqInst(&(pgstIqHandler[s16Idx]->s32Handle));
    if(bRet == VPE_RETURN_ERR)
    {
        _DrvSclVipIoRelease((pgstIqHandler[s16Idx]->s32Handle));
        _MHalVpeCleanIqInstBuffer(pgstIqHandler[s16Idx]);
        return bRet;
    }
    //alloc Wdr Mload buffer
    _DrvSclVipIoReqWdrMloadBuffer(pgstIqHandler[s16Idx]->s32Handle);
    //Default Setting (create Reg tbl and inquire tbl)
    //ToDo
    DrvSclOsMemset(&stOnOff,0,sizeof(MHalVpeIqOnOff_t));
    MHalVpeIqOnOff(pgstIqHandler[s16Idx],&stOnOff);
    DrvSclOsMemset(&stIqCfg,0,sizeof(MHalVpeIqConfig_t));
    MHalVpeIqConfig(pgstIqHandler[s16Idx], &stIqCfg);
    stCfg.bOnOff = 1;
    stCfg.enMaskType = E_DRV_SCLVIP_IO_MASK_WDR;
    _DrvSclVipIoSetMaskOnOff(pgstIqHandler[s16Idx]->s32Handle,&stCfg);
    stCfg.enMaskType = E_DRV_SCLVIP_IO_MASK_NR;
    _DrvSclVipIoSetMaskOnOff(pgstIqHandler[s16Idx]->s32Handle,&stCfg);
    //[wdr] ===================================================
    //assign Ctx
    //======void *pCtx
    //======func(&pCtx)
    *pCtx = pgstIqHandler[s16Idx];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2), "[VPEIQ]%s Create Inst %hu Success\n",  __FUNCTION__,pgstIqHandler[s16Idx]->u16InstId);
    return bRet;
}

bool MHalVpeDestroyIqInstance(void *pCtx)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    //void *pCtx = pCreate;
    //func(pCtx)
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstIqHandler = pCtx;
    //destroy inst
    bRet = _MHalVpeDestroyIqInst(&(pstIqHandler->s32Handle));
    if(bRet == VPE_RETURN_ERR)
    {
        return bRet;
    }
    //close device
    _DrvSclVipIoRelease(pstIqHandler->s32Handle);
    //clean scl inst
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2), "[VPEIQ]%s Destroy Inst %hu Success\n",  __FUNCTION__,pstIqHandler->u16InstId);
    _MHalVpeCleanIqInstBuffer(pstIqHandler);
    return bRet;
}

#define VPE_NE_TEST_ON 0

bool MHalVpeIqProcess(void *pCtx, const MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    NoiseEst_t ne_handle;
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoAipConfig_t stCfg;
    MHAL_CMDQ_CmdqInterface_t stCmdQInfo2;
    static u8 u32Vir_data[QMAP_PROCESS_SIZE];
    static u8 u32Vir_dataNE[QMAP_PROCESS_NE_SIZE];
    u8 *pNRBuf;
#if VPE_NE_TEST_ON
    static u8 fcnt = 0;
    u8 *p8;
    u16 *p16;
    u32 *p32;
#endif
    if(pstCmdQInfo)
    {
        DrvSclOsMemcpy(&stCmdQInfo2,pstCmdQInfo,sizeof(MHAL_CMDQ_CmdqInterface_t));
        _MHalVpeIqKeepCmdqFunction(&stCmdQInfo2);
    }
    else
    {
        DrvSclOsMemset(&stCmdQInfo2,0,sizeof(MHAL_CMDQ_CmdqInterface_t));
    }
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstIqHandler = pCtx;
    if (pstIqHandler->pvNrBuffer == NULL) {
        VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s IpvNrBuffer = NULL\n",  __FUNCTION__);
        VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);
        return bRet;
    }

    //[wdr] ===================================================
    //set_script_wdr_table(&stCmdQInfo2);
    //set_script_wdr_reg(&stCmdQInfo2);


    //[ne] noise est ==========================================
    DrvSclOsMemset(&ne_handle,0,sizeof(NoiseEst_t));
    pNRBuf = (u8*)pstIqHandler->pvNrBuffer;

    //[ne] 1. get noise est. reg
    DrvSclOsMemcpy(&ne_handle.in_reg_blk_sad_min_tmp_Y[0],  &pNRBuf[NEBUF_SADY], sizeof(ne_handle.in_reg_blk_sad_min_tmp_Y));
    DrvSclOsMemcpy(&ne_handle.in_reg_sum_noise_mean_Y[0],   &pNRBuf[NEBUF_SUMY], sizeof(ne_handle.in_reg_sum_noise_mean_Y));
    DrvSclOsMemcpy(&ne_handle.in_reg_count_noise_mean_Y[0], &pNRBuf[NEBUF_CNTY], sizeof(ne_handle.in_reg_count_noise_mean_Y));
    DrvSclOsMemcpy(&ne_handle.in_reg_blk_sad_min_tmp_C[0],  &pNRBuf[NEBUF_SADC], sizeof(ne_handle.in_reg_blk_sad_min_tmp_C));
    DrvSclOsMemcpy(&ne_handle.in_reg_sum_noise_mean_C[0],   &pNRBuf[NEBUF_SUMC], sizeof(ne_handle.in_reg_sum_noise_mean_C));
    DrvSclOsMemcpy(&ne_handle.in_reg_count_noise_mean_C[0], &pNRBuf[NEBUF_CNTC], sizeof(ne_handle.in_reg_count_noise_mean_C));

    DrvSclOsMemcpy(&ne_handle.out_reg_blk_sad_min_Y[0], &pstIqHandler->pu16NE_blk_sad_min_Y[0], sizeof(ne_handle.out_reg_blk_sad_min_Y));
    DrvSclOsMemcpy(&ne_handle.out_reg_blk_sad_min_C[0], &pstIqHandler->pu16NE_blk_sad_min_C[0], sizeof(ne_handle.out_reg_blk_sad_min_Y));
    DrvSclOsMemcpy(&ne_handle.out_reg_noise_mean_Y[0], &pstIqHandler->pu16NE_noise_mean_Y[0], sizeof(ne_handle.out_reg_noise_mean_Y));
    DrvSclOsMemcpy(&ne_handle.out_reg_noise_mean_C[0], &pstIqHandler->pu16NE_noise_mean_C[0], sizeof(ne_handle.out_reg_noise_mean_C));

    ne_handle.in_blk_sample_num_target  = pNRBuf[NEBUF_TARGET] + (pNRBuf[NEBUF_TARGET+1]<<8); // 32768;
    ne_handle.in_noise_mean_th_gain     = pNRBuf[NEBUF_TH_GAIN]; // 6;
    ne_handle.in_inten_blknum_lb        = pNRBuf[NEBUF_BLK_NUM]; // 16;
    ne_handle.in_blk_sad_min_lb         = pNRBuf[NEBUF_SAD_MIN]; // 32;
    ne_handle.in_noise_mean_lb          = pNRBuf[NEBUF_NOISE_MEAN]; // 32;
    ne_handle.in_learn_rate             = pNRBuf[NEBUF_LEARN]; // 2;
    ne_handle.in_dnry_gain              = pNRBuf[NEBUF_YGAIN]; // 4;
    ne_handle.in_dnrc_gain              = pNRBuf[NEBUF_CGAIN]; // 4

    //[ne] 2. calc
    _IQNoiseEstimationDriver(
        ne_handle.in_reg_blk_sad_min_tmp_Y, ne_handle.in_reg_sum_noise_mean_Y, ne_handle.in_reg_count_noise_mean_Y,
        ne_handle, ne_handle.out_reg_blk_sad_min_Y, ne_handle.out_reg_noise_mean_Y, ne_handle.out_reg_noise_mean_th_Y);
    _IQNoiseEstimationDriver(
        ne_handle.in_reg_blk_sad_min_tmp_C, ne_handle.in_reg_sum_noise_mean_C, ne_handle.in_reg_count_noise_mean_C,
        ne_handle, ne_handle.out_reg_blk_sad_min_C, ne_handle.out_reg_noise_mean_C, ne_handle.out_reg_noise_mean_th_C);

    _IQCalcNRDnrGainAndLumaLut(
        ne_handle.out_reg_noise_mean_Y, ne_handle.out_reg_noise_mean_C,
        ne_handle.in_dnry_gain, ne_handle.in_dnrc_gain,
        &ne_handle.out_reg_dnry_gain, &ne_handle.out_reg_dnrc_gain, ne_handle.out_reg_lumaLutY, ne_handle.out_reg_lumaLutC);

    //[ne] 3. set noise est. reg
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_MEANTHY], &ne_handle.out_reg_noise_mean_th_Y, sizeof(ne_handle.out_reg_noise_mean_th_Y));
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_MEANTHC], &ne_handle.out_reg_noise_mean_th_C, sizeof(ne_handle.out_reg_noise_mean_th_C));
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_SADMINY], &ne_handle.out_reg_blk_sad_min_Y, sizeof(ne_handle.out_reg_blk_sad_min_Y));
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_SADMINC], &ne_handle.out_reg_blk_sad_min_C, sizeof(ne_handle.out_reg_blk_sad_min_C));
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_MEANY], &ne_handle.out_reg_noise_mean_Y, sizeof(ne_handle.out_reg_noise_mean_Y));
    DrvSclOsMemcpy(&u32Vir_dataNE[QMAP_PROCESS_NE_MEANC], &ne_handle.out_reg_noise_mean_C, sizeof(ne_handle.out_reg_noise_mean_C));
    DrvSclOsMemcpy(&pstIqHandler->pu16NE_blk_sad_min_Y[0], &ne_handle.out_reg_blk_sad_min_Y, sizeof(ne_handle.out_reg_blk_sad_min_Y));
    DrvSclOsMemcpy(&pstIqHandler->pu16NE_blk_sad_min_C[0], &ne_handle.out_reg_blk_sad_min_C, sizeof(ne_handle.out_reg_blk_sad_min_C));
    DrvSclOsMemcpy(&pstIqHandler->pu16NE_noise_mean_Y[0], &ne_handle.out_reg_noise_mean_Y, sizeof(ne_handle.out_reg_noise_mean_Y));
    DrvSclOsMemcpy(&pstIqHandler->pu16NE_noise_mean_C[0], &ne_handle.out_reg_noise_mean_C, sizeof(ne_handle.out_reg_noise_mean_C));

    u32Vir_data[QMAP_PROCESS_MCNR_YGAIN] = ne_handle.out_reg_dnry_gain;
    u32Vir_data[QMAP_PROCESS_MCNR_CGAIN] = ne_handle.out_reg_dnrc_gain;
    DrvSclOsMemcpy(&u32Vir_data[QMAP_PROCESS_MCNR_LUMAY], &ne_handle.out_reg_lumaLutY, sizeof(ne_handle.out_reg_lumaLutY));
    DrvSclOsMemcpy(&u32Vir_data[QMAP_PROCESS_MCNR_LUMAC], &ne_handle.out_reg_lumaLutC, sizeof(ne_handle.out_reg_lumaLutC));
    _IQBufHLInverse(&u32Vir_data[QMAP_PROCESS_MCNR_LUMAY], 8);
    _IQBufHLInverse(&u32Vir_data[QMAP_PROCESS_MCNR_LUMAC], 8);
    if(pstIqHandler->bProcess)
    {
        u32Vir_data[QMAP_PROCESS_MCNR_EN] = pstIqHandler->bNREn << 1;
    }
    else
    {
        u32Vir_data[QMAP_PROCESS_MCNR_EN] = 0;
        pstIqHandler->bProcess = 1;
    }

    //wdr delay 1 frame to active
    if (pstIqHandler->bWDRActive) {
        u32Vir_data[QMAP_PROCESS_WDR_STR]   = pstIqHandler->u8Wdr_Str;
        u32Vir_data[QMAP_PROCESS_WDR_STR+1] = pstIqHandler->u8Wdr_Slope;
    }
    else {
        u32Vir_data[QMAP_PROCESS_WDR_STR]   = 0;
        u32Vir_data[QMAP_PROCESS_WDR_STR+1] = 0;
    }
    if (pstIqHandler->bWDREn)
        pstIqHandler->bWDRActive = 1;
    else
        pstIqHandler->bWDRActive = 0;


#if VPE_NE_TEST_ON
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[VPEIQ]%s:%d, \n",  __FUNCTION__, fcnt);

    //printf input data
    p16 = &ne_handle.in_reg_blk_sad_min_tmp_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_SADY]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p32 = &ne_handle.in_reg_sum_noise_mean_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_SUMY]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,\n",  p32[0],p32[1],p32[2],p32[3],p32[4],p32[5],p32[6],p32[7]);

    p16 = &ne_handle.in_reg_count_noise_mean_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_CNTY]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.in_reg_blk_sad_min_tmp_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_SADC]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p32 = &ne_handle.in_reg_sum_noise_mean_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_SUMC]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,\n",  p32[0],p32[1],p32[2],p32[3],p32[4],p32[5],p32[6],p32[7]);

    p16 = &ne_handle.in_reg_count_noise_mean_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[NEBUF_CNTC]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_blk_sample_num_targe] = %d\n", ne_handle.in_blk_sample_num_target);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_noise_mean_th_gain] = %d\n", ne_handle.in_noise_mean_th_gain);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_inten_blknum_lb] = %d\n", ne_handle.in_inten_blknum_lb);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_blk_sad_min_lb] = %d\n", ne_handle.in_blk_sad_min_lb);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_noise_mean_lb] = %d\n", ne_handle.in_noise_mean_lb);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_learn_rate] = %d\n", ne_handle.in_learn_rate);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_dnry_gain] = %d\n", ne_handle.in_dnry_gain);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[in_dnrc_gain] = %d\n", ne_handle.in_dnrc_gain);

    //printf output data
    p16 = &ne_handle.out_reg_noise_mean_th_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_noise_mean_th_Y]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.out_reg_noise_mean_th_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_noise_mean_th_C]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.out_reg_blk_sad_min_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_blk_sad_min_Y]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.out_reg_blk_sad_min_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_blk_sad_min_C]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.out_reg_noise_mean_Y[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_noise_mean_Y]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p16 = &ne_handle.out_reg_noise_mean_C[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_noise_mean_C]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p16[0],p16[1],p16[2],p16[3],p16[4],p16[5],p16[6],p16[7]);

    p8 = &ne_handle.out_reg_lumaLutY[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_lumaLutY]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p8[0],p8[1],p8[2],p8[3],p8[4],p8[5],p8[6],p8[7]);

    p8 = &ne_handle.out_reg_lumaLutC[0];
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_lumaLutC]\n");
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "%d,%d,%d,%d,%d,%d,%d,%d,\n",  p8[0],p8[1],p8[2],p8[3],p8[4],p8[5],p8[6],p8[7]);

    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_dnry_gain] = %d\n", ne_handle.out_reg_dnry_gain);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[out_reg_dnrc_gain] = %d\n", ne_handle.out_reg_dnrc_gain);


#endif
    //[wdr] wdr ==========================================
    //[wdr] 1. get wdr result

    //[wdr] 2. set wdr result
    stCfg.u32Viraddr = (u32)u32Vir_dataNE;
    stCfg.enAIPType = E_DRV_SCLVIP_IO_AIP_FORVPEPROCESSNE;
    stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size,DRV_SCLVIP_VERSION);
    _DrvSclVipIoSetAipConfig(pstIqHandler->s32Handle, &stCfg);
    stCfg.u32Viraddr = (u32)u32Vir_data;
    stCfg.enAIPType = E_DRV_SCLVIP_IO_AIP_FORVPEPROCESS;
    stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size,DRV_SCLVIP_VERSION);
    _DrvSclVipIoSetAipConfig(pstIqHandler->s32Handle, &stCfg);
    _DrvSclVipIoSetFlip(pstIqHandler->s32Handle);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);
    return bRet;
}

bool MHalVpeIqDbgLevel(void *p)
{
    bool bRet = VPE_RETURN_OK;
    gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_IQ] = *((bool *)p);
    VPE_ERR("[VPEIQ]%s Dbg Level = %hhx\n",__FUNCTION__,gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_IQ]);
    return bRet;
}

bool MHalVpeIqConfig(void *pCtx, const MHalVpeIqConfig_t *pCfg)
{
    int i=0, idx=0, id=0, id_up=0, a=0, b=0, c=0;
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoAipConfig_t  stCfg;
    static u8 u32Vir_data[QMAP_CONFIG_SIZE];
    s16 in_motion_th   = pCfg->u8NRY_BLEND_MOTION_TH;
    s16 in_still_th    = pCfg->u8NRY_BLEND_STILL_TH;
    s16 in_motion_wei  = pCfg->u8NRY_BLEND_MOTION_WEI;
    s16 in_other_wei   = pCfg->u8NRY_BLEND_OTHER_WEI;
    s16 in_still_wei   = pCfg->u8NRY_BLEND_STILL_WEI;
    u8 in_sad_shift    = pCfg->u8NRY_SF_STR;
    u8 in_xnr_dw_th    = pCfg->u8NRC_SF_STR;
    u8 in_contrast     = pCfg->u8Contrast;
    int luma_node = 8;
    int luma_step = 37; //=255/8
    int luma_lut[][16] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
        {50,35,20,10,3,1,0,0,0,0,0,0,0,0,0,0},
        {61,57,50,35,20,10,3,1,0,0,0,0,0,0,0,0},
        {61,57,50,40,28,18,10,6,4,2,2,1,1,1,1,1},
        {61,61,56,48,42,36,28,16,8,4,4,4,4,4,4,4},
        {61,61,58,55,52,48,42,30,18,12,9,8,8,8,8,8},
        {61,61,61,61,59,59,56,48,36,18,14,12,12,12,8,8},
        //{61,61,61,61,60,60,56,48,36,28,24,18,16,12,8,8},
        {61,61,61,61,61,60,60,56,48,36,28,20,16,14,12,12}
    };
    int wdr_str[][3] = {
		{   0,   0,    0},
		{  40, 128, 140},
		{ 255, 255, 255}
	};
	int node = 3;
	int id_0 = 0, id_1 = 0, out_a, out_b;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstIqHandler = pCtx;

    //===DNR_Y_LUT, DNR_Y_Gain===
    id = 0;
    for (i=0; i<luma_node; i++)
    {
        if (pCfg->u8NRY_TF_STR <= luma_step*i)
        {
          id = minmax(i-1, 0, 7);
          id_up = minmax(id+1, 0, 7);
          break;
        }
    }
    for(i = 0 ; i < 16; i++) {
        a = luma_lut[id][i];
        b = luma_lut[id_up][i];
        c = luma_step*id;
        u32Vir_data[QMAP_CONFIG_MCNR_YTBL+i] = a + ((b-a) * (pCfg->u8NRY_TF_STR-c) / luma_step);
    }
    _IQBufHLInverse(&u32Vir_data[QMAP_CONFIG_MCNR_YTBL], 16);
    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_MCNR_MV_YTBL], &u32Vir_data[QMAP_CONFIG_MCNR_YTBL], 16);

    //===DNR_C_LUT, DNR_C_Gain===
    id = 0;
    for (i=0; i<luma_node; i++)
    {
        if (pCfg->u8NRC_TF_STR <= luma_step*i)
        {
          id = minmax(i-1, 0, 7);
          id_up = minmax(id+1, 0, 7);
          break;
        }
    }
    for(i = 0 ; i < 16; i++) {
        a = luma_lut[id][i];
        b = luma_lut[id_up][i];
        c = luma_step*id;
        u32Vir_data[QMAP_CONFIG_MCNR_CTBL+i] = a + ((b-a) * (pCfg->u8NRC_TF_STR-c) / luma_step);
    }
    _IQBufHLInverse(&u32Vir_data[QMAP_CONFIG_MCNR_CTBL], 16);
    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_MCNR_MV_CTBL], &u32Vir_data[QMAP_CONFIG_MCNR_CTBL], 16);

    //===NLM+ES===================================
    //===NLM_main wb===
    if (in_motion_th >= in_still_th) in_still_th = minmax(in_motion_th+1, 0, 15);
    for(idx = 0; idx < 16; idx++)
    {
        if(idx <= in_motion_th)
            pstIqHandler->pu8NLM_ESOff[idx] = in_motion_wei;
        else if(idx >= in_still_th)
            pstIqHandler->pu8NLM_ESOff[idx] = in_still_wei;
        else
            pstIqHandler->pu8NLM_ESOff[idx] = (in_other_wei-in_motion_wei)*(idx-in_motion_th)/(in_still_th-in_motion_th-1)+in_motion_wei;
    }

    //===NLM===
    pstIqHandler->u8NLMShift_ESOff = in_sad_shift/29;

    //sync es
    _IQSyncESSetting(pstIqHandler);
    u32Vir_data[QMAP_CONFIG_NLM_SHIFT] = pstIqHandler->u8NLMShift_ESOff;

    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_NLM_MAIN], &pstIqHandler->pu8NLM_ESOff[0], sizeof(pstIqHandler->pu8NLM_ESOff));
    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_NLM_WB], &pstIqHandler->pu8NLM_ESOff[0], sizeof(pstIqHandler->pu8NLM_ESOff));
    _IQBufHLInverse(&u32Vir_data[QMAP_CONFIG_NLM_MAIN], 16);
    _IQBufHLInverse(&u32Vir_data[QMAP_CONFIG_NLM_WB], 16);
    //===NLM+ES END===============================

    //===XNR===
    u32Vir_data[QMAP_CONFIG_XNR+0] = in_xnr_dw_th;
    u32Vir_data[QMAP_CONFIG_XNR+1] = in_xnr_dw_th;
    u32Vir_data[QMAP_CONFIG_XNR+2] = minmax(in_xnr_dw_th <<1, 0, 255);
    u32Vir_data[QMAP_CONFIG_XNR+3] = minmax(in_xnr_dw_th <<1, 0, 255);

    //===YEE===
    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_YEE_ETCP], pCfg->u8EdgeGain, sizeof(pCfg->u8EdgeGain));
    DrvSclOsMemcpy(&u32Vir_data[QMAP_CONFIG_YEE_ETCM], pCfg->u8EdgeGain, sizeof(pCfg->u8EdgeGain));

    //===Contrast===
    in_contrast = minmax(in_contrast, wdr_str[0][0], wdr_str[node-1][0]);

	for (i = 0; i<node; i++)
	{
		if (in_contrast <= wdr_str[i][0])
		{
			id_1 = i;
			break;
		}
	}
	id_0 = max(id_1 - 1, 0);
	a = max(wdr_str[id_1][0] - wdr_str[id_0][0],1);
	b = in_contrast - wdr_str[id_0][0];

	c = wdr_str[id_1][1] - wdr_str[id_0][1];
	out_a = wdr_str[id_0][1] + (c* b / a);
	out_a = minmax(out_a, wdr_str[0][1], wdr_str[node-1][1]);

	c = wdr_str[id_1][2] - wdr_str[id_0][2];
	out_b = wdr_str[id_0][2] + (c* b / a);
	out_b = minmax(out_b, wdr_str[0][2], wdr_str[node-1][2]);

    pstIqHandler->u8Wdr_Str = out_a; //wdr_strength
    pstIqHandler->u8Wdr_Slope = out_b; //wdr_syn_slp


#if 1
    stCfg.u32Viraddr = (u32)u32Vir_data;
    stCfg.enAIPType = E_DRV_SCLVIP_IO_AIP_FORVPECFG;
    stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size,DRV_SCLVIP_VERSION);
    _DrvSclVipIoSetAipConfig(pstIqHandler->s32Handle, &stCfg);
#endif
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);
    return bRet;
}

bool MHalVpeIqOnOff(void *pCtx, const MHalVpeIqOnOff_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoAipConfig_t  stCfg;
    static u8 u32Vir_data[QMAP_ONOFF_SIZE];
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstIqHandler = pCtx;

    //nr en
    //u32Vir_data[QMAP_ONOFF_MCNR_EN] = pCfg->bNREn << 1;
    u32Vir_data[QMAP_ONOFF_NLM_EN]  = (pCfg->bNREn | pCfg->bESEn);
    u32Vir_data[QMAP_ONOFF_XNR_EN]  = pCfg->bNREn;
    pstIqHandler->bNREn = pCfg->bNREn;

    //edge en
    u32Vir_data[QMAP_ONOFF_YEE_EN] = pCfg->bEdgeEn;

    //es en
    pstIqHandler->bEsEn = pCfg->bESEn;
    _IQSyncESSetting(pstIqHandler);
    if (pCfg->bESEn) {
        u32Vir_data[QMAP_ONOFF_NLM_SHIFT] = pstIqHandler->u8NLMShift_ESOn;
        DrvSclOsMemcpy(&u32Vir_data[QMAP_ONOFF_NLM_MAIN], &pstIqHandler->pu8NLM_ESOn[0], 16*sizeof(u8));
        DrvSclOsMemcpy(&u32Vir_data[QMAP_ONOFF_NLM_WB], &pstIqHandler->pu8NLM_ESOn[0], 16*sizeof(u8));
    }
    else {
        u32Vir_data[QMAP_ONOFF_NLM_SHIFT] = pstIqHandler->u8NLMShift_ESOff;
        DrvSclOsMemcpy(&u32Vir_data[QMAP_ONOFF_NLM_MAIN], &pstIqHandler->pu8NLM_ESOff[0], 16*sizeof(u8));
        DrvSclOsMemcpy(&u32Vir_data[QMAP_ONOFF_NLM_WB], &pstIqHandler->pu8NLM_ESOff[0], 16*sizeof(u8));
    }
    _IQBufHLInverse(&u32Vir_data[QMAP_ONOFF_NLM_MAIN], 16);
    _IQBufHLInverse(&u32Vir_data[QMAP_ONOFF_NLM_WB], 16);

    //contrast en
    u32Vir_data[QMAP_ONOFF_WDR_EN] = pCfg->bContrastEn;
    pstIqHandler->bWDREn = pCfg->bContrastEn;

    //uv invert en
    u32Vir_data[QMAP_ONOFF_UVM_EN] = (!pCfg->bUVInvert) & 0x01;
    if (pCfg->bUVInvert) {
        u32Vir_data[QMAP_ONOFF_UVM+0] = pstIqHandler->pu8UVM[1];
        u32Vir_data[QMAP_ONOFF_UVM+1] = pstIqHandler->pu8UVM[0];
        u32Vir_data[QMAP_ONOFF_UVM+2] = pstIqHandler->pu8UVM[3];
        u32Vir_data[QMAP_ONOFF_UVM+3] = pstIqHandler->pu8UVM[2];
    }
    else {
        u32Vir_data[QMAP_ONOFF_UVM+0] = pstIqHandler->pu8UVM[0];
        u32Vir_data[QMAP_ONOFF_UVM+1] = pstIqHandler->pu8UVM[1];
        u32Vir_data[QMAP_ONOFF_UVM+2] = pstIqHandler->pu8UVM[2];
        u32Vir_data[QMAP_ONOFF_UVM+3] = pstIqHandler->pu8UVM[3];
    }

#if 1
    stCfg.u32Viraddr = (u32)u32Vir_data;
    stCfg.enAIPType = E_DRV_SCLVIP_IO_AIP_FORVPEONOFF;
    stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size,DRV_SCLVIP_VERSION);
    _DrvSclVipIoSetAipConfig(pstIqHandler->s32Handle, &stCfg);
#endif
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL2), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);

    return bRet;
}

bool MHalVpeIqGetWdrRoiHist(void *pCtx, MHalVpeIqWdrRoiReport_t * pstRoiReport)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoWdrRoiReport_t stCfg;
    u64 u64TmpY;
    u64 u64Remaind;
    bool i;
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipIoWdrRoiReport_t));
    pstIqHandler = pCtx;
    _DrvSclVipIoGetWdrHistogram(pstIqHandler->s32Handle,&stCfg);
    for(i = 0;i<ROI_WINDOW_MAX;i++)
    {
        if(stCfg.s32YCnt[i])
        {
            u64TmpY = stCfg.s32Y[i];
            u64TmpY*=4;// for output 12bit size, (I2 original is 10bit)
            pstIqHandler->stWdrBuffer.u32Y[i] =
                (u32)CamOsMathDivU64(u64TmpY,(u64)(stCfg.s32YCnt[i]),&u64Remaind);
            if(u64Remaind>=(stCfg.s32YCnt[i]/2))
            {
                pstIqHandler->stWdrBuffer.u32Y[i]++;
            }
        }
        else
        {
            pstIqHandler->stWdrBuffer.u32Y[i] = (u32)(stCfg.s32Y[i]*4);
        }
        pstRoiReport->u32Y[i] = pstIqHandler->stWdrBuffer.u32Y[i];
    }
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);

    return bRet;
}
bool MHalVpeIqRead3DNRTbl(void *pCtx)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoNrHist_t stCfg;
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipIoNrHist_t));
    pstIqHandler = pCtx;
    stCfg.u32Viraddr = (u32)pstIqHandler->pvNrBuffer;
    _DrvSclVipIoGetNRHistogram(pstIqHandler->s32Handle,&stCfg);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);

    return bRet;
}

bool MHalVpeIqSetWdrRoiHist(void *pCtx, const MHalVpeIqWdrRoiHist_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoWdrRoiHist_t stCfg;
    u16 idx,idx2;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstIqHandler = pCtx;
    //DrvSclOsMemcpy(&stCfg,pCfg,sizeof(MHalVpeIqWdrRoiHist_t));
    stCfg.bEn = pCfg->bEn;
    stCfg.enPipeSrc = pCfg->enPipeSrc;
    stCfg.u8WinCount = pCfg->u8WinCount;
    for(idx=0;idx<DRV_SCLVIP_IO_ROI_WINDOW_MAX;idx++)
    {
        stCfg.stRoiCfg[idx].bEnSkip = 1; //force to enable skip point for avoid 4k/2k issue
        for(idx2=0;idx2<4;idx2++)
        {
            stCfg.stRoiCfg[idx].u16RoiAccX[idx2] = pCfg->stRoiCfg[idx].u16RoiAccX[idx2];
            stCfg.stRoiCfg[idx].u16RoiAccY[idx2] = pCfg->stRoiCfg[idx].u16RoiAccY[idx2];
        }
        if(pstIqHandler->u32BaseAddr[idx])
        {
            stCfg.u32BaseAddr[idx] = pstIqHandler->u32BaseAddr[idx];
        }
    }
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[VPEIQ]%s {X0 X1 X2 X3] [%hu %hu %hu %hu]\n",
        __FUNCTION__,pCfg->stRoiCfg[0].u16RoiAccX[0],pCfg->stRoiCfg[0].u16RoiAccX[1],pCfg->stRoiCfg[0].u16RoiAccX[2],pCfg->stRoiCfg[0].u16RoiAccX[3]);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL3), "[VPEIQ]%s {Y0 Y1 Y2 Y3] [%hu %hu %hu %hu]\n",
        __FUNCTION__,pCfg->stRoiCfg[0].u16RoiAccY[0],pCfg->stRoiCfg[0].u16RoiAccY[1],pCfg->stRoiCfg[0].u16RoiAccY[2],pCfg->stRoiCfg[0].u16RoiAccY[3]);
    _DrvSclVipIoSetRoiConfig(pstIqHandler->s32Handle,&stCfg);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstIqHandler->u16InstId,bRet);

    return bRet;
}

bool MHalVpeIqSetWdrRoiMask(void *pCtx,const bool bEnMask, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoSetMaskOnOff_t stCfg;
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipIoSetMaskOnOff_t));
    _MHalVpeIqKeepCmdqFunction(pstCmdQInfo);
    pstIqHandler = pCtx;
    stCfg.bOnOff = bEnMask;
    stCfg.enMaskType = E_DRV_SCLVIP_IO_MASK_WDR;
    _DrvSclVipIoSetMaskOnOff(pstIqHandler->s32Handle,&stCfg);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu bEnMask :%hhu Ret:%hhd\n",
        __FUNCTION__,pstIqHandler->u16InstId,bEnMask,bRet);

    return bRet;
}
bool MHalVpeIqSetDnrTblMask(void *pCtx,const bool bEnMask, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    bool bRet = VPE_RETURN_OK;
    MHalIqHandleConfig_t *pstIqHandler;
    DrvSclVipIoSetMaskOnOff_t stCfg;
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    DrvSclOsMemset(&stCfg,0,sizeof(DrvSclVipIoSetMaskOnOff_t));
    _MHalVpeIqKeepCmdqFunction(pstCmdQInfo);
    pstIqHandler = pCtx;
    stCfg.bOnOff = bEnMask;
    stCfg.enMaskType = E_DRV_SCLVIP_IO_MASK_NR;
    _DrvSclVipIoSetMaskOnOff(pstIqHandler->s32Handle,&stCfg);
    VPE_DBG(VPE_DBG_LV_IQ(E_MHAL_VPE_DEBUG_LEVEL4), "[VPEIQ]%s Inst %hu bEnMask :%hhu Ret:%hhd\n",
        __FUNCTION__,pstIqHandler->u16InstId,bEnMask,bRet);
    return bRet;
}


//SC
bool MHalVpeCreateSclInstance(const MHalAllocPhyMem_t *pstAlloc, const MHalVpeSclWinSize_t *stMaxWin, void **pCtx)
{
    bool bRet = VPE_RETURN_OK;
    s16 s16Idx = -1;
    MHalVpeSclInputSizeConfig_t stCfg;
    MHalVpeSclOutputDmaConfig_t stDmaCfg;
    DrvSclOsMemset(&stCfg,0,sizeof(MHalVpeSclInputSizeConfig_t));
    DrvSclOsMemset(&stDmaCfg,0,sizeof(MHalVpeSclOutputDmaConfig_t));
    //init
    _MHalVpeInit();
    //keep Alloc
    _MHalVpeKeepAllocFunction(pstAlloc);
    //find new inst
    bRet = _MHalVpeFindEmptyInst(&s16Idx);
    if(bRet == VPE_RETURN_ERR || s16Idx==-1)
    {
        return VPE_RETURN_ERR;
    }
    //open device
    bRet = _MHalVpeOpenDevice(s16Idx,stMaxWin);
    if(bRet == VPE_RETURN_ERR)
    {
        return bRet;
    }
    //create inst
    bRet = _MHalVpeCreateInst(pgstScHandler[s16Idx]->s32Handle);
    if(bRet == VPE_RETURN_ERR)
    {
        _MHalVpeCloseDevice(pgstScHandler[s16Idx]->s32Handle);
        _MHalVpeCleanScInstBuffer(pgstScHandler[s16Idx]);
        return bRet;
    }
    //request MCNR
    bRet = _MHalVpeReqmemConfig(pgstScHandler[s16Idx]->s32Handle[E_HAL_SCLHVSP_ID_1],stMaxWin);
    if(bRet == VPE_RETURN_ERR)
    {
        /*
        _MHalVpeCloseDevice(pgstScHandler[s16Idx]->s32Handle);
        _MHalVpeCleanScInstBuffer(pgstScHandler[s16Idx]);
        return bRet;
            */
        VPE_ERR("[VPESCL]%s Create Inst %hu Allocate Fail\n",  __FUNCTION__,pgstScHandler[s16Idx]->u16InstId);
        bRet = VPE_RETURN_OK;
    }
    //Default Setting (create Reg tbl and inquire tbl)
    //ToDo
    stCfg.u16Height = pgstScHandler[s16Idx]->stCtx.stInputCfg.u16Height;
    stCfg.u16Width = pgstScHandler[s16Idx]->stCtx.stInputCfg.u16Width;
    _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_1,pgstScHandler[s16Idx],&stCfg);
    _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_2,pgstScHandler[s16Idx],&stCfg);
    _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_3,pgstScHandler[s16Idx],&stCfg);
    _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_4,pgstScHandler[s16Idx],&stCfg);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_1,pgstScHandler[s16Idx]);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_2,pgstScHandler[s16Idx]);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_3,pgstScHandler[s16Idx]);
    _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_4,pgstScHandler[s16Idx]);
    stDmaCfg.enOutPort = E_MHAL_SCL_OUTPUT_PORT0;
    stDmaCfg.enOutFormat = pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT0];
    _MHalVpeDmaConfig(E_HAL_SCLDMA_ID_1,pgstScHandler[s16Idx],&stDmaCfg);
    stDmaCfg.enOutPort = E_MHAL_SCL_OUTPUT_PORT1;
    stDmaCfg.enOutFormat = pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT1];
    _MHalVpeDmaConfig(E_HAL_SCLDMA_ID_2,pgstScHandler[s16Idx],&stDmaCfg);
    stDmaCfg.enOutPort = E_MHAL_SCL_OUTPUT_PORT2;
    stDmaCfg.enOutFormat = pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT2];
    _MHalVpeDmaConfig(E_HAL_SCLDMA_ID_3,pgstScHandler[s16Idx],&stDmaCfg);
    stDmaCfg.enOutPort = E_MHAL_SCL_OUTPUT_PORT3;
    stDmaCfg.enOutFormat = pgstScHandler[s16Idx]->stCtx.enOutFormat[E_MHAL_SCL_OUTPUT_PORT3];
    _MHalVpeDmaConfig(E_HAL_SCLDMA_ID_4,pgstScHandler[s16Idx],&stDmaCfg);
    //assign Ctx
    //======void *pCtx
    //======func(&pCtx)
    *pCtx = pgstScHandler[s16Idx];
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Create Inst %hu Success\n",  __FUNCTION__,pgstScHandler[s16Idx]->u16InstId);
    return bRet;
}

bool MHalVpeDestroySclInstance(void *pCtx)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    u16 u16InstId;
    //void *pCtx = pCreate;
    //func(pCtx)
    if(pCtx==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstScHandler = pCtx;
    u16InstId = pstScHandler->u16InstId;
    //release MCNR
    _DrvSclHvspIoReleaseMemConfig(pstScHandler->s32Handle[E_HAL_SCLHVSP_ID_1]);
    //destroy inst
    bRet = _MHalVpeDestroyInst(pstScHandler->s32Handle);
    if(bRet == VPE_RETURN_ERR)
    {
        return bRet;
    }
    //close device
    _MHalVpeCloseDevice(pstScHandler->s32Handle);
    //clean scl inst
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Destroy Inst %hu Success\n",  __FUNCTION__,u16InstId);
    _MHalVpeCleanScInstBuffer(pstScHandler);
    return bRet;
}
bool MHalVpeSclProcess(void *pCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, const MHalVpeSclOutputBufferConfig_t *pBuffer)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    if(pCtx==NULL || pBuffer==NULL)
    {
        return VPE_RETURN_ERR;
    }
    _MHalVpeSclKeepCmdqFunction(pstCmdQInfo);
    pstScHandler = pCtx;
    _MHalVpeSclProcessDbg(pstScHandler,pBuffer);
    _MHalVpeSclProcess(pstScHandler,pBuffer);
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);
    return bRet;
}

bool MHalVpeSclDbgLevel(void *p)
{
    bool bRet = VPE_RETURN_OK;
    gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_SC] = *((bool *)p);
    VPE_ERR("[VPEIQ]%s Dbg Level = %hhx\n",__FUNCTION__,gbDbgLevel[E_MHAL_VPE_DEBUG_TYPE_SC]);
    return bRet;
}

bool MHalVpeSclCropConfig(void *pCtx, const MHalVpeSclCropConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstScHandler = pCtx;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL1), "[VPESCL]%s Inst %hu (X,Y,W,H)=(%hu,%hu,%hu,%hu) En:%d\n",
        __FUNCTION__,pstScHandler->u16InstId,pCfg->stCropWin.u16X,pCfg->stCropWin.u16Y,
        pCfg->stCropWin.u16Width,pCfg->stCropWin.u16Height,pCfg->bCropEn);
    if(_MHalVpeScIsCropSizeNeedReSetting(pstScHandler,pCfg))
    {
        // set global
        _MHalVpeScCropSizeReSetting(pstScHandler,pCfg);
        // set scaling size
        _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_1,pstScHandler);
        _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_4,pstScHandler);
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);
    return bRet;
}



bool MHalVpeSclInputConfig(void *pCtx, const MHalVpeSclInputSizeConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    MHalVpeSclInputSizeConfig_t stForISPCfg;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    DrvSclOsMemcpy(&stForISPCfg,pCfg,sizeof(MHalVpeSclInputSizeConfig_t));
    pstScHandler = pCtx;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL1), "[VPESCL]%s Inst %hu Height:%hu Width:%hu PixelFormat:%d\n",
        __FUNCTION__,pstScHandler->u16InstId,pCfg->u16Height,pCfg->u16Width,pCfg->ePixelFormat);
    _MHalVpeSclForISPInputAlign(pstScHandler,&stForISPCfg);
    if(_MHalVpeScIsInputSizeNeedReSetting(pstScHandler,&stForISPCfg))
    {
        // set global
        _MHalVpeScInputSizeReSetting(pstScHandler,stForISPCfg.u16Width,stForISPCfg.u16Height);
        // set input
        bRet = _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_1,pstScHandler,&stForISPCfg);
        // set scaling size
        _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_1,pstScHandler);
        _MHalVpeScScalingConfig(E_HAL_SCLHVSP_ID_4,pstScHandler);
    }
    else
    {
        bRet = _MHalVpeHvspInConfig(E_HAL_SCLHVSP_ID_1,pstScHandler,&stForISPCfg);
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);
    return bRet;
}

bool MHalVpeSclOutputSizeConfig(void *pCtx, const MHalVpeSclOutputSizeConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    MHalVpeSclOutputDmaConfig_t stDmaCfg;
    DrvSclIdType_e enId;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstScHandler = pCtx;
    DrvSclOsMemset(&stDmaCfg,0,sizeof(MHalVpeSclOutputDmaConfig_t));
    enId = (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT0) ? E_HAL_SCLDMA_ID_1 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT1) ? E_HAL_SCLDMA_ID_2 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT2) ? E_HAL_SCLDMA_ID_3 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT3) ? E_HAL_SCLDMA_ID_4 :
                    E_HAL_ID_MAX;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL1), "[VPESCL]%s Inst %hu Height:%hu Width:%hu @OutPort:%d\n",
        __FUNCTION__,pstScHandler->u16InstId,pCfg->u16Height,pCfg->u16Width,pCfg->enOutPort);
    if(pCfg->enOutPort==E_MHAL_SCL_OUTPUT_PORT1)
    {
        if(pCfg->u16Width>1280)
        {
            VPE_ERR("[VPE]Port:%d Width over spec :%hu\n",pCfg->enOutPort,pCfg->u16Width);
            return VPE_RETURN_ERR;
        }
    }
    if(pCfg->enOutPort==E_MHAL_SCL_OUTPUT_PORT2)
    {
        if(pCfg->u16Width>1920)
        {
            VPE_ERR("[VPE]Port:%d Width over spec :%hu\n",pCfg->enOutPort,pCfg->u16Width);
            return VPE_RETURN_ERR;
        }
    }
    if(_MHalVpeScIsOutputSizeNeedReSetting(pstScHandler,pCfg))
    {
        // set global
        _MHalVpeScOutputSizeReSetting(pstScHandler,pCfg);
        // set scaling size
        _MHalVpeSclOutputSizeConfig(pstScHandler,pCfg);
        stDmaCfg.enOutPort = pCfg->enOutPort;
        stDmaCfg.enOutFormat = pstScHandler->stCtx.enOutFormat[pCfg->enOutPort];
        _MHalVpeDmaConfig(enId,pstScHandler,&stDmaCfg);
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);
    return bRet;
}
bool MHalVpeSclOutputDmaConfig(void *pCtx, const MHalVpeSclOutputDmaConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    DrvSclIdType_e enId;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstScHandler = pCtx;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu enCompress:%d OutFormat:%d @OutPort:%d\n",
        __FUNCTION__,pstScHandler->u16InstId,pCfg->enCompress,pCfg->enOutFormat,pCfg->enOutPort);
    enId = (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT0) ? E_HAL_SCLDMA_ID_1 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT1) ? E_HAL_SCLDMA_ID_2 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT2) ? E_HAL_SCLDMA_ID_3 :
           (pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT3) ? E_HAL_SCLDMA_ID_4 :
                    E_HAL_ID_MAX;
    if(enId==E_HAL_ID_MAX)
    {
        VPE_ERR("[VPESCL]%s Output Port Num Error\n",__FUNCTION__);
        bRet = VPE_RETURN_ERR;
    }
    else
    {
        pstScHandler->stCtx.enOutFormat[pCfg->enOutPort] = pCfg->enOutFormat;
        bRet = _MHalVpeDmaConfig(enId,pstScHandler,pCfg);
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);
    return bRet;
}

bool MHalVpeSclOutputMDWinConfig(void *pCtx, const MHalVpeSclOutputMDwinConfig_t *pCfg)
{
    bool bRet = VPE_RETURN_OK;
    MHalSclHandleConfig_t *pstScHandler;
    if(pCtx==NULL || pCfg==NULL)
    {
        return VPE_RETURN_ERR;
    }
    pstScHandler = pCtx;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu OutType:%d @OutPort:%d\n",
        __FUNCTION__,pstScHandler->u16InstId,pCfg->enOutType,pCfg->enOutPort);
    if(pCfg->enOutPort !=E_MHAL_SCL_OUTPUT_PORT3 &&pCfg->enOutType ==E_MHAL_SCL_OUTPUT_MDWIN)
    {
        VPE_ERR("[VPESCL]%s Output Port To MDWIN Error\n",__FUNCTION__);
        bRet = VPE_RETURN_ERR;
    }
    if(pCfg->enOutPort ==E_MHAL_SCL_OUTPUT_PORT3 &&pCfg->enOutType !=E_MHAL_SCL_OUTPUT_MDWIN)
    {
        VPE_ERR("[VPESCL]%s Output Port 3 To Dram Error\n",__FUNCTION__);
        bRet = VPE_RETURN_ERR;
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL2), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);

    return bRet;
}
//new
bool MHalVpeSclSetSwTriggerIrq(MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    bool bRet = VPE_RETURN_OK;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Ret:%hhd\n",  __FUNCTION__,bRet);
    _MHalVpeSclKeepCmdqFunction(pstCmdQInfo);
    _VpeIspRegWMsk(pstCmdQInfo,0x1525,0xC,0x2,0x2);
    _VpeIspRegWMsk(pstCmdQInfo,0x1525,0xC,0x0,0x2);
    return bRet;
}
bool MHalVpeSclSetWaitDone(void *pSclCtx, MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo, MHalVpeWaitDoneType_e enWait)
{
    bool bRet = VPE_RETURN_OK;
    u32 wait_cnt = 0;
    MHalSclHandleConfig_t *pstScHandler;
    _MHalVpeSclKeepCmdqFunction(pstCmdQInfo);
    pstScHandler = pSclCtx;
    // wait done
    if(_VpeIsCmdq(pstCmdQInfo))
    {
        VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Inst %hu CMDQ\n",  __FUNCTION__,pstScHandler->u16InstId);
        if(enWait==E_MHAL_VPE_WAITDONE_DMAONLY)
        {
            pstCmdQInfo->MHAL_CMDQ_CmdqAddWaitEventCmd(pstCmdQInfo,E_MHAL_CMDQEVE_SC_TRIG013);
            pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x15253C,0xFFFF);
            pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x15253C,0xFFFB);
        }
        else if(enWait == E_MHAL_VPE_WAITDONE_MDWINONLY)
        {
            pstCmdQInfo->MHAL_CMDQ_CmdqAddWaitEventCmd(pstCmdQInfo,E_MHAL_CMDQEVE_S0_MDW_W_DONE);
            //pstCmdQInfo->MHAL_CMDQ_CmdqPollRegBits(pstCmdQInfo,0x152528,0x2,0x2,1);
            //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x152520,0xFFFF);
            //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x152520,0xFFFD);
        }
        else
        {
            pstCmdQInfo->MHAL_CMDQ_CmdqAddWaitEventCmd(pstCmdQInfo,E_MHAL_CMDQEVE_SC_TRIG013);
            pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x15253C,0xFFFF);
            pstCmdQInfo->MHAL_CMDQ_WriteRegCmdq(pstCmdQInfo,0x15253C,0xFFFB);
            pstCmdQInfo->MHAL_CMDQ_CmdqAddWaitEventCmd(pstCmdQInfo,E_MHAL_CMDQEVE_S0_MDW_W_DONE);
        }
        //isp reset
        //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdqMask(pstCmdQInfo,0x1509CA,0,0x5);
        //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdqMask(pstCmdQInfo,0x150902,0,0x1);
        //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdqMask(pstCmdQInfo,0x1509CA,0x5,0x5);
        //pstCmdQInfo->MHAL_CMDQ_WriteRegCmdqMask(pstCmdQInfo,0x150902,0x1,0x1);
    }
    else
    {
        if(gpVpeIoBase==NULL)
        {
            gpVpeIoBase = (void*)0xFD000000;
        }
        if(enWait==E_MHAL_VPE_WAITDONE_DMAONLY)
        {
            while( !(_VpeIspRegR(0x1525,0x1F)&0x4) )
            {
                if(++wait_cnt > 20)
                {
                    //pr_info("[VPE] wait frame done timeout.\n");
                    break;
                }
                DrvSclOsDelayTask(100); //wait 100 ms
                VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4),"[VPE] wait frame done.\n");
            }
            _VpeIspRegW(NULL,0x1525,0x1E,0xFFFF);
            _VpeIspRegW(NULL,0x1525,0x1E,0xFFFB);
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x0,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x0,0x1);
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x5,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x1,0x1);
        }
        else if(enWait == E_MHAL_VPE_WAITDONE_MDWINONLY)
        {
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x0,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x0,0x1);
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x5,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x1,0x1);
        }
        else
        {
            while( !(_VpeIspRegR(0x1525,0x1F)&0x4) )
            {
                if(++wait_cnt > 20)
                {
                    //pr_info("[VPE] wait frame done timeout.\n");
                    break;
                }
                DrvSclOsDelayTask(100); //wait 100 ms
                VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4),"[VPE] wait frame done.\n");
            }
            _VpeIspRegW(NULL,0x1525,0x1E,0xFFFF);
            _VpeIspRegW(NULL,0x1525,0x1E,0xFFFB);
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x0,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x0,0x1);
            //_VpeIspRegWMsk(NULL,0x1509,0x65,0x5,0x5);
            //_VpeIspRegWMsk(NULL,0x1509,0x01,0x1,0x1);
        }
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Inst %hu Ret:%hhd\n",  __FUNCTION__,pstScHandler->u16InstId,bRet);

    return bRet;
}
bool MHalVpeSclGetIrqNum(unsigned int *pIrqNum)
{
    bool bRet = VPE_RETURN_OK;
    u32 u32IrqNum;
    _MHalVpeInit();
    u32IrqNum = DrvSclOsGetIrqIDSCL(E_DRV_SCLOS_SCLIRQ_SC0);
    *pIrqNum = u32IrqNum;
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s IrqNum=%lu Ret:%hhd\n",  __FUNCTION__,u32IrqNum,bRet);

    return bRet;
}
#if defined(USE_USBCAM)
irqreturn_t  _MHalVpeSclDoneIrq(int eIntNum, void* dev_id)
{

    if (MHalVpeSclCheckIrq() == TRUE)
    {
       MHalVpeSclClearIrq();
       if(_gpMahlIntCb)
        _gpMahlIntCb();
    }

    return IRQ_HANDLED;
}

bool MHalVpeSetIrqCallback(MhalInterruptCb pMhalIntCb)
{
    _gpMahlIntCb = pMhalIntCb;
    return VPE_RETURN_OK;
}
#endif
bool MHalVpeSclEnableIrq(bool bOn)
{
#if defined(USE_USBCAM)
    bool bRet = VPE_RETURN_OK;
    unsigned int  nIrqNum = 0 ;

    if(bOn)
    {
        if(_gpMahlIntCb == NULL)
        {
            VPE_ERR("[DRVSCLIRQ]%s(%d):: Request FUNC NULL\n", __FUNCTION__, __LINE__);
            return FALSE;
        }

        MHalVpeSclGetIrqNum(&nIrqNum);
        if(DrvSclOsAttachInterrupt(nIrqNum, (InterruptCb)_MHalVpeSclDoneIrq ,IRQF_DISABLED, "SCLINTR"))
        {
            VPE_ERR("[DRVSCLIRQ]%s(%d):: Request IRQ Fail\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
        DrvSclOsDisableInterrupt(nIrqNum);
        DrvSclOsEnableInterrupt(nIrqNum);
        DrvSclIrqInterruptEnable(VPEIRQ_SC3_ENG_FRM_END);
    }
    else
    {

        DrvSclIrqDisable(VPEIRQ_SC3_ENG_FRM_END);
        DrvSclOsDisableInterrupt(nIrqNum);
        DrvSclOsDetachInterrupt(nIrqNum);
        _gpMahlIntCb = NULL;
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s bOn= %hhd Ret:%hhd\n",  __FUNCTION__,bOn,bRet);
    return bRet;
#else
    bool bRet = VPE_RETURN_OK;
    if(bOn)
    {
        DrvSclIrqInterruptEnable(VPEIRQ_SC3_ENG_FRM_END);
    }
    else
    {
        DrvSclIrqDisable(VPEIRQ_SC3_ENG_FRM_END);
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s bOn= %hhd Ret:%hhd\n",  __FUNCTION__,bOn,bRet);
    return bRet;
#endif
}
bool MHalVpeSclClearIrq(void)
{
    bool bRet = VPE_RETURN_OK;
    DrvSclIrqSetClear(VPEIRQ_SC3_ENG_FRM_END);
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Ret:%hhd\n",  __FUNCTION__,bRet);
    return bRet;
}
bool MHalVpeSclCheckIrq(void)
{
    bool bRet = VPE_RETURN_OK;
    u64 u64Flag;
    DrvSclIrqGetFlag(VPEIRQ_SC3_ENG_FRM_END, &u64Flag);
    if(!u64Flag)
    {
        bRet = VPE_RETURN_ERR;
    }
    VPE_DBG(VPE_DBG_LV_SC(E_MHAL_VPE_DEBUG_LEVEL4), "[VPESCL]%s Flag = %llx ,Ret:%hhd\n",  __FUNCTION__,u64Flag,bRet);
    return bRet;
}
bool MHalVpeInit(const MHalAllocPhyMem_t *pstAlloc ,MHAL_CMDQ_CmdqInterface_t *pstCmdQInfo)
{
    _MHalVpeKeepAllocFunction(pstAlloc);
    _MHalVpeSclKeepCmdqFunction(pstCmdQInfo);
    _MHalVpeInit();
    return VPE_RETURN_OK;
}
bool MHalVpeDeInit(void)
{
    _MHalVpeDeInit();
    _MHalVpeKeepAllocFunction(NULL);
    _MHalVpeSclKeepCmdqFunction(NULL);
    return VPE_RETURN_OK;
}
#if defined (SCLOS_TYPE_LINUX_KERNEL)

EXPORT_SYMBOL(MHalVpeInit);
EXPORT_SYMBOL(MHalVpeDeInit);
EXPORT_SYMBOL(MHalVpeSclCheckIrq);
EXPORT_SYMBOL(MHalVpeSclSetWaitDone);
EXPORT_SYMBOL(MHalVpeIqRead3DNRTbl);
EXPORT_SYMBOL(MHalVpeSclSetSwTriggerIrq);
EXPORT_SYMBOL(MHalVpeSclClearIrq);
EXPORT_SYMBOL(MHalVpeSclEnableIrq);
EXPORT_SYMBOL(MHalVpeSclGetIrqNum);
EXPORT_SYMBOL(MHalVpeSclOutputSizeConfig);
EXPORT_SYMBOL(MHalVpeSclInputConfig);
EXPORT_SYMBOL(MHalVpeSclOutputDmaConfig);
EXPORT_SYMBOL(MHalVpeSclCropConfig);
EXPORT_SYMBOL(MHalVpeSclDbgLevel);
EXPORT_SYMBOL(MHalVpeSclProcess);
EXPORT_SYMBOL(MHalVpeDestroySclInstance);
EXPORT_SYMBOL(MHalVpeCreateSclInstance);
EXPORT_SYMBOL(MHalVpeIspInputConfig);
EXPORT_SYMBOL(MHalVpeIspRotationConfig);
EXPORT_SYMBOL(MHalVpeIspDbgLevel);
EXPORT_SYMBOL(MHalVpeIspProcess);
EXPORT_SYMBOL(MHalVpeCreateIspInstance);
EXPORT_SYMBOL(MHalVpeDestroyIspInstance);
EXPORT_SYMBOL(MHalVpeIqSetDnrTblMask);
EXPORT_SYMBOL(MHalVpeIqSetWdrRoiMask);
EXPORT_SYMBOL(MHalVpeIqSetWdrRoiHist);
EXPORT_SYMBOL(MHalVpeIqGetWdrRoiHist);
EXPORT_SYMBOL(MHalVpeIqOnOff);
EXPORT_SYMBOL(MHalVpeIqConfig);
EXPORT_SYMBOL(MHalVpeIqDbgLevel);
EXPORT_SYMBOL(MHalVpeIqProcess);
EXPORT_SYMBOL(MHalVpeDestroyIqInstance);
EXPORT_SYMBOL(MHalVpeCreateIqInstance);
#endif
#undef HAL_VPE_C
