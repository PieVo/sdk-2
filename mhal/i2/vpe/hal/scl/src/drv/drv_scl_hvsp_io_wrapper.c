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
#define __DRV_SCL_HVSP_IO_WRAPPER_C__


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"

#include "drv_scl_verchk.h"
#include "drv_scl_irq_st.h"
#include "drv_scl_hvsp_m.h"
#include "drv_scl_dma_m.h"
#include "drv_scl_hvsp_m.h"
#include "drv_scl_hvsp_io_st.h"
#include "drv_scl_hvsp_io_wrapper.h"
#include "drv_scl_ctx_m.h"
#include "drv_scl_ctx_st.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

#if 0

#define DRV_SCLHVSP_IO_LOCK_MUTEX(x)    \
    SCL_ERR("+++ [MUTEX_LOCK][%s]_1_[%d] \n", __FUNCTION__, __LINE__); \
    DrvSclOsObtainMutex(x, SCLOS_WAIT_FOREVER); \
    SCL_ERR("+++ [MUTEX_LOCK][%s]_2_[%d] \n", __FUNCTION__, __LINE__);

#define DRV_SCLHVSP_IO_UNLOCK_MUTEX(x)  \
    SCL_ERR("--- [MUTEX_LOCK][%s]   [%d] \n", __FUNCTION__, __LINE__); \
    DrvSclOsReleaseMutex(x);



#else
#define DRV_SCLHVSP_IO_LOCK_MUTEX(x)    DrvSclOsObtainMutex(x, SCLOS_WAIT_FOREVER)
#define DRV_SCLHVSP_IO_UNLOCK_MUTEX(x)  DrvSclOsReleaseMutex(x)

#endif

//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    s32 s32Id;
    DrvSclHvspIoIdType_e enIdType;
    MDrvSclCtxLockConfig_t *pLockCfg;
    MDrvSclCtxIdType_e enCtxId;
}DrvSclHvspIoCtxConfig_t;


typedef struct
{
    s32 s32Handle;
    DrvSclHvspIoIdType_e enSclHvspId;
    DrvSclHvspIoCtxConfig_t stCtxCfg;
} DrvSclHvspIoHandleConfig_t;

typedef struct
{
    u32 u32StructSize;
    u32 *pVersion;
    u32 u32VersionSize;
} DrvSclHvspIoVersionChkConfig_t;


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
MDrvSclCtxNrBufferGlobalSet_t *gstGlobalSet;


//keep
DrvSclHvspIoHandleConfig_t _gstSclHvspHandler[DRV_SCLHVSP_HANDLER_MAX];
DrvSclHvspIoFunctionConfig_t _gstSclHvspIoFunc;
s32 _s32SclHvspIoHandleMutex = -1;
char KEY_DMEM_SCL_MCNR_YC[20] = "SCL_MCNR_YC";
u8  gbdbgmessage[EN_DBGMG_NUM_CONFIG];//extern

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------

void* _DrvSclHvspIoAllocDmem(char* name, u32 size, DrvSclOsDmemBusType_t *addr)
{
    void *pDmem = NULL;;

    pDmem = DrvSclOsDirectMemAlloc(name, size, addr);

    if(pDmem)
    {
        gstGlobalSet->u32McnrReleaseSize = gstGlobalSet->u32McnrSize;
    }

    return pDmem;
}

void _DrvSclHvspIoFreeDmem(const char* name, unsigned int size, void *virt, u32 addr)
{
    DrvSclOsDirectMemFree(name, size, virt, addr);
}


bool  _DrvSclHvspIoCheckModifyMemSize(DrvSclHvspIoReqMemConfig_t  *stReqMemCfg)
{
    if( (stReqMemCfg->u16Vsize & (15)) || (stReqMemCfg->u16Pitch & (15)))
    {
        SCL_ERR(
            "[HVSP] Size must be align 16, Vsize=%d, Pitch=%d\n", stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch);
        if((stReqMemCfg->u16Pitch & (15)) && (stReqMemCfg->u16Vsize & (15)))
        {
            stReqMemCfg->u32MemSize = (((stReqMemCfg->u16Vsize / 16) + 1) * 16) * ((stReqMemCfg->u16Pitch / 16) + 1) * 16 * 4;
        }
        else if(stReqMemCfg->u16Pitch & (15))
        {
            stReqMemCfg->u32MemSize = (stReqMemCfg->u16Vsize) * ((stReqMemCfg->u16Pitch / 16) + 1) * 16 * 4;
        }
        else if(stReqMemCfg->u16Vsize & (15))
        {
            stReqMemCfg->u32MemSize = (((stReqMemCfg->u16Vsize / 16) + 1) * 16) * stReqMemCfg->u16Pitch * 4;
        }
    }
    else
    {
        stReqMemCfg->u32MemSize = (stReqMemCfg->u16Vsize) * (stReqMemCfg->u16Pitch) * 4;
    }
    if((DrvSclOsGetSclFrameBufferNum()) == 1)
    {
        stReqMemCfg->u32MemSize = stReqMemCfg->u32MemSize / 2 ;
    }
    gstGlobalSet->u32McnrSize = stReqMemCfg->u32MemSize;
    if((DrvSclOsGetSclFrameBufferNum()) == 1)
    {
        if((u32)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 2) > stReqMemCfg->u32MemSize)
        {
            SCL_ERR( "[HVSP] Memory size is too small, Vsize*Pitch*2=%lx, MemSize=%lx\n",
                     (u32)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 2), stReqMemCfg->u32MemSize);
            return FALSE;
        }
    }
    else if((u32)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 4) > stReqMemCfg->u32MemSize)
    {
        SCL_ERR( "[HVSP] Memory size is too small, Vsize*Pitch*4=%lx, MemSize=%lx\n",
                 (u32)(stReqMemCfg->u16Vsize * stReqMemCfg->u16Pitch * 4), stReqMemCfg->u32MemSize);
        return FALSE;
    }
    SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1,
            "[HVSP], Vsize=%d, Pitch=%d, Size=%lx\n", stReqMemCfg->u16Vsize, stReqMemCfg->u16Pitch, stReqMemCfg->u32MemSize);
    return TRUE;
}
void _DrvSclHvspIoMemNaming(DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg)
{
    gstGlobalSet->u8McnrYCMName[0] = 48+(((pstSclHvspCtxCfg->s32Id&0xFFF)%1000)/100);
    gstGlobalSet->u8McnrYCMName[1] = 48+(((pstSclHvspCtxCfg->s32Id&0xFFF)%100)/10);
    gstGlobalSet->u8McnrYCMName[2] = 48+(((pstSclHvspCtxCfg->s32Id&0xFFF)%10));
    gstGlobalSet->u8McnrYCMName[3] = '_';
    gstGlobalSet->u8McnrYCMName[4] = '\0';
    DrvSclOsStrcat(gstGlobalSet->u8McnrYCMName,KEY_DMEM_SCL_MCNR_YC);
}
void _DrvSclHvspIoMemFreeYCMbuffer(void)
{
    if(gstGlobalSet->pvMcnrYCMVirAddr != 0)
    {
        SCL_ERR("[SCLHVSP] YC free %lx\n",(u32)gstGlobalSet->pvMcnrYCMVirAddr);
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP] YC free\n");
        _DrvSclHvspIoFreeDmem(gstGlobalSet->u8McnrYCMName,
                            PAGE_ALIGN(gstGlobalSet->u32McnrSize),
                            gstGlobalSet->pvMcnrYCMVirAddr,
                            gstGlobalSet->PhyMcnrYCMAddr);

        gstGlobalSet->pvMcnrYCMVirAddr = 0;
        gstGlobalSet->PhyMcnrYCMAddr = 0;
        DrvSclOsSetSclFrameBufferAlloced(DrvSclOsGetSclFrameBufferAlloced()&(~E_DRV_SCLOS_FBALLOCED_YCM));
    }
}

bool _DrvSclHvspIoMemAllocate(void)
{

    SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[HVSP] allocate memory\n");
    if (!(gstGlobalSet->pvMcnrYCMVirAddr = _DrvSclHvspIoAllocDmem(gstGlobalSet->u8McnrYCMName,
                                      PAGE_ALIGN(gstGlobalSet->u32McnrSize),
                                      &gstGlobalSet->PhyMcnrYCMAddr)))
    {
        SCL_ERR( "%s: unable to allocate YC memory %lx\n", __FUNCTION__,(u32)gstGlobalSet->PhyMcnrYCMAddr);
        return 0;
    }
    else
    {
        DrvSclOsSetSclFrameBufferAlloced((DrvSclOsGetSclFrameBufferAlloced()|E_DRV_SCLOS_FBALLOCED_YCM));
    }

    SCL_ERR( "[HVSP]: MCNR YC: Phy:%x  Vir:%lx\n", gstGlobalSet->PhyMcnrYCMAddr, (u32)gstGlobalSet->pvMcnrYCMVirAddr);

    return 1;
}

void _DrvSclHvspIoMemFree(void)
{
    _DrvSclHvspIoMemFreeYCMbuffer();
}

void _DrvSclhvspFrameBufferMemoryAllocate(void)
{
    if(DrvSclOsGetSclFrameBufferAlloced() == 0)
    {
        _DrvSclHvspIoMemAllocate();
    }
    else if(DrvSclOsGetSclFrameBufferAlloced() != 0 &&
        gstGlobalSet->u32McnrSize > gstGlobalSet->u32McnrReleaseSize)
    {
        _DrvSclHvspIoMemFree();
        DrvSclOsSetSclFrameBufferAlloced(E_DRV_SCLOS_FBALLOCED_NON);
        _DrvSclHvspIoMemAllocate();
    }
}

void _DrvSclHvspIoFillIPMStructForDriver(DrvSclHvspIoReqMemConfig_t *pstReqMemCfg,MDrvSclHvspIpmConfig_t *stIPMCfg)
{
    stIPMCfg->u16Height = pstReqMemCfg->u16Vsize;
    stIPMCfg->u16Width  = pstReqMemCfg->u16Pitch;
    stIPMCfg->u32MemSize = pstReqMemCfg->u32MemSize;
    if(DrvSclOsGetSclFrameBufferAlloced()&E_DRV_SCLOS_FBALLOCED_YCM)
    {
        if((stIPMCfg->u16Height <= (FHDHeight + 8)) && (stIPMCfg->u16Width <= FHDWidth))
        {
            stIPMCfg->enRW       = E_MDRV_SCLHVSP_MCNR_YCM_W;
        }
        else
        {
            stIPMCfg->enRW       = E_MDRV_SCLHVSP_MCNR_YCM_RW;
        }
        gstGlobalSet->u32McnrSize   = pstReqMemCfg->u32MemSize;
    }
    else
    {
        stIPMCfg->enRW       = E_MDRV_SCLHVSP_MCNR_NON;
        gstGlobalSet->u32McnrSize   = pstReqMemCfg->u32MemSize;
    }
    if(gstGlobalSet->PhyMcnrYCMAddr)
    {
        stIPMCfg->u32YCPhyAddr = (gstGlobalSet->PhyMcnrYCMAddr);
    }
}



void _DrvSclHvspIoFillVersionChkStruct
    (u32 u32StructSize, u32 u32VersionSize, u32 *pVersion,DrvSclHvspIoVersionChkConfig_t *stVersion)
{
    stVersion->u32StructSize  = (u32)u32StructSize;
    stVersion->u32VersionSize = (u32)u32VersionSize;
    stVersion->pVersion      = (u32 *)pVersion;
}

s32 _DrvSclHvspIoVersionCheck(DrvSclHvspIoVersionChkConfig_t *stVersion)
{
    if ( CHK_VERCHK_HEADER(stVersion->pVersion) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( stVersion->pVersion, DRV_SCLHVSP_VERSION) )
        {

            VERCHK_ERR("[HVSP] Version(%04lx) < %04x!!! \n",
                       *(stVersion->pVersion) & VERCHK_VERSION_MASK,
                       DRV_SCLHVSP_VERSION);

            return -1;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &stVersion->u32VersionSize, stVersion->u32StructSize) == 0 )
            {
                VERCHK_ERR("[HVSP] Size(%04lx) != %04lx!!! \n",
                           stVersion->u32StructSize,
                           stVersion->u32VersionSize);

                return -1;
            }
            else
            {
                return VersionCheckSuccess;
            }
        }
    }
    else
    {
        VERCHK_ERR("[HVSP] No Header !!! \n");
        SCL_ERR( "[HVSP]   %s  \n", __FUNCTION__);
        return -1;
    }
}
u8 _DrvSclHvspIoGetIdOpenTime(DrvSclHvspIoIdType_e enHvspId)
{
    s16 i = 0;
    u8 u8Cnt = 0;
    for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
    {
        if(_gstSclHvspHandler[i].enSclHvspId == enHvspId && _gstSclHvspHandler[i].s32Handle != -1)
        {
            u8Cnt ++;
        }
    }
    return u8Cnt;
}
bool _DrvSclHvspIoGetMdrvIdType(s32 s32Handler, MDrvSclHvspIdType_e *penHvspId)
{
    s16 i;
    s16 s16Idx = -1;
    bool bRet = TRUE;

    for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
    {
        if(_gstSclHvspHandler[i].s32Handle == s32Handler)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        *penHvspId = E_MDRV_SCLHVSP_ID_MAX;
        bRet = FALSE;
    }
    else
    {
        bRet = TRUE;
        switch(_gstSclHvspHandler[s16Idx].enSclHvspId)
        {
            case E_DRV_SCLHVSP_IO_ID_1:
                *penHvspId = E_MDRV_SCLHVSP_ID_1;
                break;
            case E_DRV_SCLHVSP_IO_ID_2:
                *penHvspId = E_MDRV_SCLHVSP_ID_2;
                break;
            case E_DRV_SCLHVSP_IO_ID_3:
                *penHvspId = E_MDRV_SCLHVSP_ID_3;
                break;
            case E_DRV_SCLHVSP_IO_ID_4:
                *penHvspId = E_MDRV_SCLHVSP_ID_4;
                break;
            default:
                *penHvspId = E_MDRV_SCLHVSP_ID_MAX;
                bRet = FALSE;
                break;
        }
    }
    return bRet;
}

DrvSclHvspIoCtxConfig_t *_DrvSclHvspIoGetCtxConfig(s32 s32Handler)
{
    s16 i;
    s16 s16Idx = -1;
    DrvSclHvspIoCtxConfig_t * pCtxCfg;

    DRV_SCLHVSP_IO_LOCK_MUTEX(_s32SclHvspIoHandleMutex);

    for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
    {
        if(_gstSclHvspHandler[i].s32Handle == s32Handler)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        pCtxCfg = NULL;
    }
    else
    {
        pCtxCfg = &_gstSclHvspHandler[s16Idx].stCtxCfg;
    }

    DRV_SCLHVSP_IO_UNLOCK_MUTEX(_s32SclHvspIoHandleMutex);
    return pCtxCfg;
}

bool _DrvSclHvspIoIsLockFree(DrvSclHvspIoCtxConfig_t *pstCfg)
{
    bool bLockFree = FALSE;
    bool bFound = 0;
    u8 i;

    if(pstCfg->pLockCfg->bLock == FALSE)
    {
        bLockFree = TRUE;
    }
    else
    {
        for(i=0;i< pstCfg->pLockCfg->u8IdNum; i++)
        {
            if(pstCfg->s32Id == pstCfg->pLockCfg->s32Id[i])
            {
                bFound = 1;
                break;
            }
        }

        bLockFree = bFound ? TRUE : FALSE;
    }

    return bLockFree;
}

DrvSclOsClkIdType_e _DrvSclHvspIoTransClkId(MDrvSclHvspIdType_e enMDrvSclHvspId)
{
    DrvSclOsClkIdType_e enClkId = E_DRV_SCLOS_CLK_ID_NUM;
    switch(enMDrvSclHvspId)
    {
        case E_MDRV_SCLHVSP_ID_1:
            enClkId = E_DRV_SCLOS_CLK_ID_HVSP1;
            break;

        case E_MDRV_SCLHVSP_ID_2:
            enClkId = E_DRV_SCLOS_CLK_ID_HVSP2;
            break;

        case E_MDRV_SCLHVSP_ID_3:
            enClkId = E_DRV_SCLOS_CLK_ID_HVSP3;
            break;
        case E_MDRV_SCLHVSP_ID_4:
            enClkId = E_DRV_SCLOS_CLK_ID_HVSP4;
            break;
        default:
            enClkId = E_DRV_SCLOS_CLK_ID_NUM;
            break;
    }
    return enClkId;
}
MDrvSclHvspIdType_e _DrvSclHvspGetHvspId(DrvSclHvspIoIdType_e enSclHvspId)
{
    MDrvSclHvspIdType_e enDev;
    switch(enSclHvspId)
    {
        case E_DRV_SCLHVSP_IO_ID_1:
            enDev = E_MDRV_SCLHVSP_ID_1;
            break;
        case E_DRV_SCLHVSP_IO_ID_2:
            enDev = E_MDRV_SCLHVSP_ID_2;
            break;
        case E_DRV_SCLHVSP_IO_ID_3:
            enDev = E_MDRV_SCLHVSP_ID_3;
            break;
        case E_DRV_SCLHVSP_IO_ID_4:
            enDev = E_MDRV_SCLHVSP_ID_4;
            break;
        default:
            enDev = E_MDRV_SCLHVSP_ID_MAX;
                break;
    }
    return enDev;
}
void _DrvSclHvspIoSetGlobal(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    gstGlobalSet = &(((MDrvSclCtxGlobalSet_t*)(pvCtx->pgstGlobalSet))->stNrBufferCfg);
}
void _DrvSclHvspIoInitGlobal(MDrvSclCtxIdType_e enCtxId)
{
    void *pvCtx;
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    _DrvSclHvspIoSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    gstGlobalSet->u32McnrSize = 1920 * 1080 * 2 * 2 ;
    gstGlobalSet->PhyMcnrYCMAddr = 0;
    gstGlobalSet->pvMcnrYCMVirAddr = NULL;
    gstGlobalSet->u32McnrReleaseSize = 0;
}
//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
bool _DrvSclHvspIoDeInit(DrvSclHvspIoIdType_e enSclHvspId)
{
    DrvSclosProbeType_e enType;
    enType = (enSclHvspId==E_DRV_SCLHVSP_IO_ID_1) ? E_DRV_SCLOS_INIT_HVSP_1 :
            (enSclHvspId==E_DRV_SCLHVSP_IO_ID_2) ? E_DRV_SCLOS_INIT_HVSP_2 :
            (enSclHvspId==E_DRV_SCLHVSP_IO_ID_3) ?   E_DRV_SCLOS_INIT_HVSP_3 :
             (enSclHvspId==E_DRV_SCLHVSP_IO_ID_4) ?    E_DRV_SCLOS_INIT_HVSP_4:
                E_DRV_SCLOS_INIT_NONE;
    DrvSclOsClearProbeInformation(enType);
    if(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_ALL) == 0)
    {
        //Ctx DeInit
        MDrvSclHvspExit(1);
    }
    else if(!DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_HVSP))
    {
        MDrvSclHvspExit(0);
    }
    if(_s32SclHvspIoHandleMutex != -1)
    {
         DrvSclOsDeleteMutex(_s32SclHvspIoHandleMutex);
         _s32SclHvspIoHandleMutex = -1;
    }
    return TRUE;
}
bool _DrvSclHvspIoInit(DrvSclHvspIoIdType_e enSclHvspId)
{
    u16 i, start, end;
    MDrvSclHvspInitConfig_t stHVSPInitCfg;
    MDrvSclHvspIdType_e enSclMHvspId;
    MDrvSclCtxConfig_t *pvCfg;
    DrvSclOsMemset(&stHVSPInitCfg,0,sizeof(MDrvSclHvspInitConfig_t));
    if(enSclHvspId >= E_DRV_SCLHVSP_IO_ID_NUM)
    {
        SCL_ERR("%s %d, Id out of range %d\n", __FUNCTION__, __LINE__, enSclHvspId);
        return FALSE;
    }

    if(_s32SclHvspIoHandleMutex == -1)
    {
        _s32SclHvspIoHandleMutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, "SCLHVSP_IO", SCLOS_PROCESS_SHARED);
        if(_s32SclHvspIoHandleMutex == -1)
        {
            SCL_ERR("%s %d, Create Mutex Fail\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }
    enSclMHvspId = _DrvSclHvspGetHvspId(enSclHvspId);

    start = (u16)enSclHvspId * DRV_SCLHVSP_HANDLER_INSTANCE_NUM;
    end = start + DRV_SCLHVSP_HANDLER_INSTANCE_NUM;

    for(i = start; i < end; i++)
    {
        _gstSclHvspHandler[i].s32Handle = -1;
        _gstSclHvspHandler[i].enSclHvspId = E_DRV_SCLHVSP_IO_ID_NUM;

        _gstSclHvspHandler[i].stCtxCfg.s32Id = -1;
        _gstSclHvspHandler[i].stCtxCfg.enIdType = E_DRV_SCLHVSP_IO_ID_NUM;
        _gstSclHvspHandler[i].stCtxCfg.pLockCfg = NULL;
    }

    //Ctx Init
    if( MDrvSclCtxInit() == FALSE)
    {
        SCL_ERR("%s %d, Init Ctx\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
#if I2_DVR
    _DrvSclHvspIoInitGlobal(E_MDRV_SCL_CTX_ID_SC_ALL);
#else
    _DrvSclHvspIoInitGlobal(E_MDRV_SCL_CTX_ID_SC_GEN);
#endif
    if(enSclHvspId == E_DRV_SCLHVSP_IO_ID_1)
    {
        DrvSclOsMemset(&_gstSclHvspIoFunc, 0, sizeof(DrvSclHvspIoFunctionConfig_t));
        _gstSclHvspIoFunc.DrvSclHvspIoSetInputConfig        = _DrvSclHvspIoSetInputConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetOutputConfig       = _DrvSclHvspIoSetOutputConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetScalingConfig      = _DrvSclHvspIoSetScalingConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoReqmemConfig          = _DrvSclHvspIoReqmemConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetMiscConfig         = _DrvSclHvspIoSetMiscConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetPostCropConfig     = _DrvSclHvspIoSetPostCropConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoGetPrivateIdConfig    = _DrvSclHvspIoGetPrivateIdConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoGetInformConfig       = _DrvSclHvspIoGetInformConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoReleaseMemConfig      = _DrvSclHvspIoReleaseMemConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetOsdConfig          = _DrvSclHvspIoSetOsdConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetPriMaskConfig      = _DrvSclHvspIoSetPriMaskConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoPirMaskTrigerConfig   = _DrvSclHvspIoPirMaskTrigerConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetFbConfig           = _DrvSclHvspIoSetFbConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoGetVersion            = _DrvSclHvspIoGetVersion;
        _gstSclHvspIoFunc.DrvSclHvspIoSetLockConfig         = _DrvSclHvspIoSetLockConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetUnlockConfig       = _DrvSclHvspIoSetUnlockConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetVtrackConfig       = _DrvSclHvspIoSetVtrackConfig;
        _gstSclHvspIoFunc.DrvSclHvspIoSetVtrackOnOffConfig  = _DrvSclHvspIoSetVtrackOnOffConfig;
    }


    //ToDo Init
    stHVSPInitCfg.u32IRQNUM     = DrvSclOsGetIrqIDSCL(E_DRV_SCLOS_SCLIRQ_SC0);
    stHVSPInitCfg.u32CMDQIRQNUM = DrvSclOsGetIrqIDCMDQ(E_DRV_SCLOS_CMDQIRQ_CMDQ0);
    pvCfg = MDrvSclCtxGetDefaultCtx();
    stHVSPInitCfg.pvCtx = (void *)&(pvCfg->stCtx);
    if( MDrvSclHvspInit(enSclMHvspId, &stHVSPInitCfg) == 0)
    {
        return -EFAULT;
    }
    return TRUE;
}

s32 _DrvSclHvspIoOpen(DrvSclHvspIoIdType_e enSclHvspId)
{
    s32 s32Handle = -1;
    s16 s16Idx = -1;
    s16 i ;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;
#if I2_DVR
        enCtxId = E_MDRV_SCL_CTX_ID_SC_ALL;
#else
        if(enSclHvspId == E_DRV_SCLHVSP_IO_ID_3)
        {
            enCtxId = E_MDRV_SCL_CTX_ID_SC_3;
        }
        else
        {
            enCtxId = E_MDRV_SCL_CTX_ID_SC_ALL;
        }
#endif
    DRV_SCLHVSP_IO_LOCK_MUTEX(_s32SclHvspIoHandleMutex);
    for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
    {
        if(_gstSclHvspHandler[i].s32Handle == -1)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        s32Handle = -1;
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
    }
    else
    {
        s32Handle = s16Idx | DRV_SCLHVSP_HANDLER_PRE_FIX | (enSclHvspId<<(HANDLER_PRE_FIX_SHIFT));
        _gstSclHvspHandler[s16Idx].s32Handle = s32Handle ;
        _gstSclHvspHandler[s16Idx].enSclHvspId = enSclHvspId;
        _gstSclHvspHandler[s16Idx].stCtxCfg.enCtxId= enCtxId;
        _gstSclHvspHandler[s16Idx].stCtxCfg.s32Id = s32Handle;
        _gstSclHvspHandler[s16Idx].stCtxCfg.enIdType = enSclHvspId;
        _gstSclHvspHandler[s16Idx].stCtxCfg.pLockCfg = MDrvSclCtxGetLockConfig(enCtxId);
        _DrvSclHvspIoGetMdrvIdType(s32Handle, &enMdrvIdType);
        MDrvSclHvspOpen(enMdrvIdType);
    }
    DRV_SCLHVSP_IO_UNLOCK_MUTEX(_s32SclHvspIoHandleMutex);
    return s32Handle;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoRelease(s32 s32Handler)
{
    s16 s16Idx = -1;
    s16 i ;
    u16 u16loop = 0;
    DrvSclHvspIoErrType_e eRet = TRUE;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxCmdqConfig_t *pvCfg;
    DRV_SCLHVSP_IO_LOCK_MUTEX(_s32SclHvspIoHandleMutex);
    for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
    {
        if(_gstSclHvspHandler[i].s32Handle == s32Handler)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        eRet = E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    else
    {

        //ToDo Free Ctx

        _gstSclHvspHandler[s16Idx].s32Handle = -1;
        _gstSclHvspHandler[s16Idx].enSclHvspId = E_DRV_SCLHVSP_IO_ID_NUM;

        _gstSclHvspHandler[s16Idx].stCtxCfg.s32Id = -1;
        _gstSclHvspHandler[s16Idx].stCtxCfg.enIdType = E_DRV_SCLHVSP_IO_ID_NUM;
        _gstSclHvspHandler[s16Idx].stCtxCfg.pLockCfg = NULL;
        for(i = 0; i < DRV_SCLHVSP_HANDLER_MAX; i++)
        {
            if(_gstSclHvspHandler[i].s32Handle != -1)
            {
                u16loop = 1;
                break;
            }
        }
        if(!u16loop)
        {
            _DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType);
            MDrvSclCtxSetLockConfig(s32Handler,_gstSclHvspHandler[s16Idx].stCtxCfg.enCtxId);
            pvCfg = MDrvSclCtxGetConfigCtx(_gstSclHvspHandler[s16Idx].stCtxCfg.enCtxId);
            _DrvSclHvspIoSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCfg);
            _DrvSclHvspIoMemFree();
            MDrvSclCtxSetUnlockConfig(s32Handler,_gstSclHvspHandler[s16Idx].stCtxCfg.enCtxId);
            MDrvSclHvspRelease(enMdrvIdType,pvCfg);
            MDrvSclHvspReSetHw(pvCfg);
        }
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    DRV_SCLHVSP_IO_UNLOCK_MUTEX(_s32SclHvspIoHandleMutex);
    return eRet;
}


DrvSclHvspIoErrType_e _DrvSclHvspIoPoll(s32 s32Handler, DrvSclHvspIoWrapperPollConfig_t *pstIoPollCfg)
{
    MDrvSclHvspIdType_e enMdrvIdType;

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    if(pstIoPollCfg->bWaitQueue)
    {
        //_DrvSclHvspIoPollLinux(enMdrvIdType, pstIoPollCfg);
    }
    else
    {
        //_DrvSclHvspIoPollRtk(enMdrvIdType, pstIoPollCfg);
    }
    return E_DRV_SCLHVSP_IO_ERR_OK;
}
DrvSclHvspIoErrType_e _DrvSclHvspIoSetInputConfig(s32 s32Handler, DrvSclHvspIoInputConfig_t *pstIoInCfg)
{
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    MDrvSclHvspInputConfig_t stInCfg;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;

    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoInputConfig_t),
                 pstIoInCfg->VerChk_Size,
                 &pstIoInCfg->VerChk_Version,&stVersion);
    DrvSclOsMemset(&stInCfg,0,sizeof(MDrvSclHvspInputConfig_t));
    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    stInCfg.enColor       = pstIoInCfg->enColor;
    stInCfg.enSrcType     = pstIoInCfg->enSrcType;
    DrvSclOsMemcpy(&stInCfg.stCaptureWin, &pstIoInCfg->stCaptureWin, sizeof(MDrvSclHvspWindowConfig_t));
    DrvSclOsMemcpy(&stInCfg.stTimingCfg, &pstIoInCfg->stTimingCfg, sizeof(MDrvSclHvspTimingConfig_t));

    stInCfg.stclk = (MDrvSclHvspClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclHvspIoTransClkId(enMdrvIdType));
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stInCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclHvspSetInputConfig(enMdrvIdType,  &stInCfg))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetOutputConfig(s32 s32Handler, DrvSclHvspIoOutputConfig_t *pstIoOutCfg)
{
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;

    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoOutputConfig_t),
                 pstIoOutCfg->VerChk_Size,
                 &pstIoOutCfg->VerChk_Version,&stVersion);
    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s  \n", __FUNCTION__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    return E_DRV_SCLHVSP_IO_ERR_OK;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetScalingConfig(s32 s32Handler, DrvSclHvspIoScalingConfig_t *pstIOSclCfg)
{
    MDrvSclHvspScalingConfig_t stSclCfg;
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;

    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoScalingConfig_t),
                pstIOSclCfg->VerChk_Size,
                &pstIOSclCfg->VerChk_Version,&stVersion);
    DrvSclOsMemset(&stSclCfg,0,sizeof(MDrvSclHvspScalingConfig_t));
    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s  \n", __FUNCTION__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    stSclCfg.stclk = (MDrvSclHvspClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclHvspIoTransClkId(enMdrvIdType));
    stSclCfg.stCropWin.bEn = pstIOSclCfg->bCropEn;
    stSclCfg.stCropWin.u16Height = pstIOSclCfg->stCropWin.u16Height;
    stSclCfg.stCropWin.u16Width = pstIOSclCfg->stCropWin.u16Width;
    stSclCfg.stCropWin.u16X = pstIOSclCfg->stCropWin.u16X;
    stSclCfg.stCropWin.u16Y = pstIOSclCfg->stCropWin.u16Y;
    stSclCfg.u16Dsp_Height = pstIOSclCfg->u16Dsp_Height;
    stSclCfg.u16Dsp_Width = pstIOSclCfg->u16Dsp_Width;
    stSclCfg.u16Src_Height = pstIOSclCfg->u16Src_Height;
    stSclCfg.u16Src_Width = pstIOSclCfg->u16Src_Width;
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stSclCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);

    if(!MDrvSclHvspSetScalingConfig(enMdrvIdType,  &stSclCfg ))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}
DrvSclHvspIoErrType_e _DrvSclHvspIoReqmemConfig(s32 s32Handler, DrvSclHvspIoReqMemConfig_t*pstReqMemCfg)
{
    MDrvSclHvspIpmConfig_t stIPMCfg;
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;
    void *pvCtx;
    DrvSclOsMemset(&stIPMCfg,0,sizeof(MDrvSclHvspIpmConfig_t));
     _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoReqMemConfig_t),
                 pstReqMemCfg->VerChk_Size,
                 &pstReqMemCfg->VerChk_Version,&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(enMdrvIdType != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    _DrvSclHvspIoSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    _DrvSclHvspIoMemNaming(pstSclHvspCtxCfg);
    DrvSclOsSetSclFrameBufferNum(DNR_BUFFER_MODE);
    _DrvSclHvspIoCheckModifyMemSize(pstReqMemCfg);
    _DrvSclhvspFrameBufferMemoryAllocate();
    _DrvSclHvspIoFillIPMStructForDriver(pstReqMemCfg,&stIPMCfg);
    stIPMCfg.pvCtx = pvCtx;
    if(MDrvSclHvspSetInitIpmConfig(E_MDRV_SCLHVSP_ID_1, &stIPMCfg))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }

    if(DrvSclOsGetSclFrameBufferAlloced() == 0)
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}


DrvSclHvspIoErrType_e _DrvSclHvspIoSetMiscConfig(s32 s32Handler, DrvSclHvspIoMiscConfig_t *pstIOMiscCfg)
{
    MDrvSclHvspMiscConfig_t stMiscCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    DrvSclOsMemset(&stMiscCfg,0,sizeof(MDrvSclHvspMiscConfig_t));
    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(enMdrvIdType != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    stMiscCfg.u8Cmd = pstIOMiscCfg->u8Cmd;
    stMiscCfg.u32Size = pstIOMiscCfg->u32Size;
    stMiscCfg.u32Addr = pstIOMiscCfg->u32Addr;

    if(MDrvSclHvspSetMiscConfig(&stMiscCfg))
    {
        return E_DRV_SCLHVSP_IO_ERR_OK;
    }
    else
    {
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetPostCropConfig(s32 s32Handler, DrvSclHvspIoPostCropConfig_t *pstIOPostCfg)
{
    MDrvSclHvspPostCropConfig_t stPostCfg;
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stPostCfg,0,sizeof(MDrvSclHvspPostCropConfig_t));
     _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoPostCropConfig_t),
                pstIOPostCfg->VerChk_Size,
                &pstIOPostCfg->VerChk_Version,&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(enMdrvIdType != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    stPostCfg.bCropEn = pstIOPostCfg->bCropEn;
    stPostCfg.bFmCntEn = pstIOPostCfg->bFmCntEn;
    stPostCfg.stclk = (MDrvSclHvspClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclHvspIoTransClkId(enMdrvIdType));
    stPostCfg.u16Height = pstIOPostCfg->u16Height;
    stPostCfg.u16Width = pstIOPostCfg->u16Width;
    stPostCfg.u16X = pstIOPostCfg->u16X;
    stPostCfg.u16Y = pstIOPostCfg->u16Y;
    stPostCfg.u8FmCnt = pstIOPostCfg->u8FmCnt;
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stPostCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);

    if(!MDrvSclHvspSetPostCropConfig(E_MDRV_SCLHVSP_ID_1,  &stPostCfg))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoGetPrivateIdConfig(s32 s32Handler, DrvSclHvspIoPrivateIdConfig_t *pstIOCfg)
{
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;

    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    pstIOCfg->s32Id = s32Handler;

    return E_DRV_SCLHVSP_IO_ERR_OK;
}


DrvSclHvspIoErrType_e _DrvSclHvspIoGetInformConfig(s32 s32Handler, DrvSclHvspIoScInformConfig_t *pstIOInfoCfg)
{
    MDrvSclHvspScInformConfig_t stInfoCfg;
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    MDrvSclHvspIdType_e enMdrvIdType;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    void *pvCtx;
    DrvSclOsMemset(&stInfoCfg,0,sizeof(MDrvSclHvspScInformConfig_t));
    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    MDrvSclCtxSetLockConfig(s32Handler,pstSclHvspCtxCfg->enCtxId);
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(pstSclHvspCtxCfg->enCtxId);
    stInfoCfg.pvCtx = pvCtx;
    if(!MDrvSclHvspGetSCLInform( enMdrvIdType,  &stInfoCfg))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        DrvSclOsMemcpy(pstIOInfoCfg, &stInfoCfg, sizeof(DrvSclHvspIoScInformConfig_t));
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclHvspCtxCfg->enCtxId);
    return eRet;

}

DrvSclHvspIoErrType_e _DrvSclHvspIoReleaseMemConfig(s32 s32Handler)
{

    MDrvSclHvspIdType_e enMdrvIdType;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    void *pvCtx;
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(enMdrvIdType != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    _DrvSclHvspIoSetGlobal((MDrvSclCtxCmdqConfig_t*)pvCtx);
    _DrvSclHvspIoMemFree();
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return E_DRV_SCLHVSP_IO_ERR_OK;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetOsdConfig(s32 s32Handler, DrvSclHvspIoOsdConfig_t *pstIOOSDCfg)
{
    MDrvSclHvspOsdConfig_t stOSDCfg;
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stOSDCfg,0,sizeof(MDrvSclHvspOsdConfig_t));
     _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoOsdConfig_t),
                (pstIOOSDCfg->VerChk_Size),
                &(pstIOOSDCfg->VerChk_Version),&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    else
    {
        stOSDCfg.enOSD_loc = pstIOOSDCfg->enOSD_loc;
        stOSDCfg.stOsdOnOff.bOSDEn = pstIOOSDCfg->bEn;
        stOSDCfg.stOsdOnOff.bOSDBypass = pstIOOSDCfg->bOSDBypass;
        stOSDCfg.stOsdOnOff.bWTMBypass = pstIOOSDCfg->bWTMBypass;
    }
    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d, Version fail \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }


    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stOSDCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclHvspSetOsdConfig(enMdrvIdType,  &stOSDCfg))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetPriMaskConfig(s32 s32Handler, DrvSclHvspIoPriMaskConfig_t *pstIOPriMaskCfg)
{
    SCL_ERR( "[SCLHVSP]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
    return E_DRV_SCLHVSP_IO_ERR_FAULT;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoPirMaskTrigerConfig(s32 s32Handler, DrvSclHvspIoPriMaskTriggerConfig_t *pstIOPriMaskTrigCfg)
{
    SCL_ERR( "[SCLHVSP]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
    return E_DRV_SCLHVSP_IO_ERR_FAULT;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoSetFbConfig(s32 s32Handler, DrvSclHvspIoSetFbManageConfig_t *pstIOFbMgCfg)
{
    DrvSclHvspIoErrType_e eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclHvspIdType_e enMdrvIdType;

    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoSetFbManageConfig_t),
                (pstIOFbMgCfg->VerChk_Size),
                &(pstIOFbMgCfg->VerChk_Version),&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[HVSP]   %s %d, Version fail \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);

    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[HVSP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(_DrvSclHvspIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }
    if(enMdrvIdType != E_MDRV_SCLHVSP_ID_1)
    {
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    if(!MDrvSclHvspSetFbManageConfig((MDrvSclHvspFbmgSetType_e)pstIOFbMgCfg->enSet))
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    else
    {
        eRet = E_DRV_SCLHVSP_IO_ERR_OK;
    }

    return eRet;
}

DrvSclHvspIoErrType_e _DrvSclHvspIoGetVersion(s32 s32Hander, DrvSclHvspIoVersionConfig_t *psIOVersionCfg)
{
    DrvSclHvspIoErrType_e eRet;

    if (CHK_VERCHK_HEADER( &(psIOVersionCfg->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS( &(psIOVersionCfg->VerChk_Version), DRV_SCLHVSP_VERSION) )
        {

            VERCHK_ERR("[HVSP] Version(%04lx) < %04x!!! \n",
                       psIOVersionCfg->VerChk_Version & VERCHK_VERSION_MASK,
                       DRV_SCLHVSP_VERSION);

            eRet = E_DRV_SCLHVSP_IO_ERR_INVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(psIOVersionCfg->VerChk_Size), sizeof(DrvSclHvspIoVersionConfig_t)) == 0 )
            {
                VERCHK_ERR("[HVSP] Size(%04x) != %04lx!!! \n",
                           sizeof(DrvSclHvspIoVersionConfig_t),
                           (psIOVersionCfg->VerChk_Size));

                eRet = E_DRV_SCLHVSP_IO_ERR_INVAL;
            }
            else
            {
                DrvSclHvspIoVersionConfig_t stCfg;

                stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size, DRV_SCLHVSP_VERSION);
                stCfg.u32Version = DRV_SCLHVSP_VERSION;
                DrvSclOsMemcpy(psIOVersionCfg, &stCfg, sizeof(DrvSclHvspIoVersionConfig_t));
                eRet = E_DRV_SCLHVSP_IO_ERR_OK;
            }
        }
    }
    else
    {
        VERCHK_ERR("[HVSP] No Header %lx!!! \n", psIOVersionCfg->VerChk_Version);
        SCL_ERR( "[HVSP]   %s %d \n", __FUNCTION__, __LINE__);
        eRet = E_DRV_SCLHVSP_IO_ERR_INVAL;
    }

    return eRet;
}
DrvSclHvspIoErrType_e _DrvSclHvspIoSetLockConfig(s32 s32Handler, DrvSclHvspIoLockConfig_t *pstIoLockCfg)
{
    SCL_ERR( "[SCLHVSP]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
    return E_DRV_SCLHVSP_IO_ERR_FAULT;
}


DrvSclHvspIoErrType_e _DrvSclHvspIoSetUnlockConfig(s32 s32Handler, DrvSclHvspIoLockConfig_t *pstIoLockCfg)
{
    SCL_ERR( "[SCLHVSP]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
    return E_DRV_SCLHVSP_IO_ERR_FAULT;
}
DrvSclHvspIoErrType_e _DrvSclHvspIoSetVtrackConfig(s32 s32Handler, DrvSclHvspIoVtrackConfig_t *pstCfg)
{
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoVtrackConfig_t),
                                               (pstCfg->VerChk_Size),
                                               &(pstCfg->VerChk_Version),&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[VIP]   %s  %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);
    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[VIP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    if(pstCfg->bSetKey)
    {
        MDrvSclHvspVtrackSetPayloadData(pstCfg->u16Timecode, pstCfg->u8OperatorID);
        MDrvSclHvspVtrackSetKey(pstCfg->bSetKey, pstCfg->u8SetKey);
    }
    else
    {
        MDrvSclHvspVtrackSetPayloadData(pstCfg->u16Timecode, pstCfg->u8OperatorID);
        MDrvSclHvspVtrackSetKey(0,NULL);
    }
    if(pstCfg->bSetUserDef)
    {
        MDrvSclHvspVtrackSetUserDefindedSetting(pstCfg->bSetUserDef, pstCfg->u8SetUserDef);
    }
    else
    {
        MDrvSclHvspVtrackSetUserDefindedSetting(0,NULL);
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return E_DRV_SCLHVSP_IO_ERR_OK;
}


DrvSclHvspIoErrType_e _DrvSclHvspIoSetVtrackOnOffConfig(s32 s32Handler, DrvSclHvspIoVtrackOnOffConfig_t *pstCfg)
{
    DrvSclHvspIoVersionChkConfig_t stVersion;
    DrvSclHvspIoCtxConfig_t *pstSclHvspCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    _DrvSclHvspIoFillVersionChkStruct(sizeof(DrvSclHvspIoVtrackOnOffConfig_t),
                                               (pstCfg->VerChk_Size),
                                               &(pstCfg->VerChk_Version),&stVersion);

    if(_DrvSclHvspIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[VIP]   %s  %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    pstSclHvspCtxCfg = _DrvSclHvspIoGetCtxConfig(s32Handler);
    if(pstSclHvspCtxCfg == NULL)
    {
        SCL_ERR( "[VIP]   %s %d, Ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    enCtxId = pstSclHvspCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    if(!MDrvSclHvspVtrackEnable(pstCfg->u8framerate,  pstCfg->EnType))
    {
        MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
        return E_DRV_SCLHVSP_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return E_DRV_SCLHVSP_IO_ERR_OK;
}
