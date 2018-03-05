////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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
#define __DRV_SCL_DMA_IO_WRAPPER_C__


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------

#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"

#include "drv_scl_verchk.h"
#include "drv_scl_dma_m.h"
#include "drv_scl_hvsp_m.h"
#include "drv_scl_dma_io_st.h"
#include "drv_scl_dma_io_wrapper.h"
#include "drv_scl_ctx_m.h"
#include "drv_scl_ctx_st.h"
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------

#define DRV_SCLDMA_IO_LOCK_MUTEX(x)    DrvSclOsObtainMutex(x, SCLOS_WAIT_FOREVER)
#define DRV_SCLDMA_IO_UNLOCK_MUTEX(x)  DrvSclOsReleaseMutex(x)


//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    s32 s32Id;
    DrvSclDmaIoIdType_e enIdType;
    MDrvSclCtxLockConfig_t *pLockCfg;
    MDrvSclCtxConfig_t  *pCmdqCtx;
    MDrvSclCtxIdType_e enCtxId;
}DrvSclDmaIoCtxConfig_t;

typedef struct
{
    s32 s32Handle;
    DrvSclDmaIoIdType_e enSclDmaId;
    DrvSclDmaIoCtxConfig_t stCtxCfg;
}DrvSclDmaIoHandleConfig_t;

typedef struct
{
    u32 u32StructSize;
    u32 *pVersion;
    u32 u32VersionSize;
}DrvSclDmaIoVersionChkConfig_t;


//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
DrvSclDmaIoHandleConfig_t _gstSclDmaHandler[DRV_SCLDMA_HANDLER_MAX];

DrvSclDmaIoFunctionConfig_t _gstSclDmaFunc;
s32 s32SclDmaIoHandlerMutex = -1;

//-------------------------------------------------------------------------------------------------
//  Local Functions
//-------------------------------------------------------------------------------------------------
void _DrvSclDmaIoFillVersionChkStruct(u32 u32StructSize,u32 u32VersionSize,u32 *pVersion,DrvSclDmaIoVersionChkConfig_t *stVersion)
{
    stVersion->u32StructSize  = (u32)u32StructSize;
    stVersion->u32VersionSize = (u32)u32VersionSize;
    stVersion->pVersion      = (u32 *)pVersion;
}

s32 _DrvSclDmaIoVersionCheck(DrvSclDmaIoVersionChkConfig_t *stVersion)
{
    if ( CHK_VERCHK_HEADER(stVersion->pVersion) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( stVersion->pVersion, DRV_SCLDMA_VERSION) )
        {

            VERCHK_ERR("[SCLDMA] Version(%04lx) < %04x!!! \n",
                *(stVersion->pVersion) & VERCHK_VERSION_MASK,
                DRV_SCLDMA_VERSION);

            return -1;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &stVersion->u32VersionSize, stVersion->u32StructSize) == 0 )
            {
                VERCHK_ERR("[SCLDMA] Size(%04lx) != %04lx!!! \n",
                    stVersion->u32StructSize,
                    stVersion->u32VersionSize);

                return -1;
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_ELSE, "[SCLDMA] Size(%ld) \n",stVersion->u32StructSize );
                return 0;
            }
        }
    }
    else
    {
        VERCHK_ERR("[SCLDMA] No Header !!! \n");
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return -1;
    }
}

u32 _DrvSclDmaIoGetIdOpenTime(DrvSclDmaIoIdType_e enDmaId)
{
    s16 i = 0;
    u32 u32Cnt = 0;
    for(i = 0; i < DRV_SCLDMA_HANDLER_MAX; i++)
    {
        if(_gstSclDmaHandler[i].enSclDmaId == enDmaId && _gstSclDmaHandler[i].s32Handle != -1)
        {
            u32Cnt ++;
        }
    }
    return u32Cnt;
}

DrvSclDmaIoCtxConfig_t *_DrvSclDmaIoGetCtxConfig(s32 s32Handler)
{
    s16 i;
    s16 s16Idx = -1;
    DrvSclDmaIoCtxConfig_t *pCtxCfg;

    DRV_SCLDMA_IO_LOCK_MUTEX(s32SclDmaIoHandlerMutex);

    for(i = 0; i < DRV_SCLDMA_HANDLER_MAX; i++)
    {
        if(_gstSclDmaHandler[i].s32Handle == s32Handler)
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
        pCtxCfg = &_gstSclDmaHandler[i].stCtxCfg;
    }

    DRV_SCLDMA_IO_UNLOCK_MUTEX(s32SclDmaIoHandlerMutex);
    return pCtxCfg;
}

bool _DrvSclDmaIoIsLockFree(DrvSclDmaIoCtxConfig_t *pstCfg)
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

bool _DrvSclDmaIoGetMdrvIdType(s32 s32Handler, MDrvSclDmaIdType_e *penSclDmaId)
{
    s16 i;
    s16 s16Idx = -1;
    bool bRet = TRUE;

    for(i = 0; i < DRV_SCLDMA_HANDLER_MAX; i++)
    {
        if(_gstSclDmaHandler[i].s32Handle == s32Handler)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        *penSclDmaId = E_MDRV_SCLDMA_ID_NUM;
        bRet = FALSE;
    }
    else
    {
        bRet = TRUE;
        switch(_gstSclDmaHandler[s16Idx].enSclDmaId)
        {
            case E_DRV_SCLDMA_IO_ID_1:
                *penSclDmaId = E_MDRV_SCLDMA_ID_1;
                break;
            case E_DRV_SCLDMA_IO_ID_2:
                *penSclDmaId = E_MDRV_SCLDMA_ID_2;
                break;
            case E_DRV_SCLDMA_IO_ID_3:
                *penSclDmaId = E_MDRV_SCLDMA_ID_3;
                break;
            case E_DRV_SCLDMA_IO_ID_4:
                *penSclDmaId = E_MDRV_SCLDMA_ID_MDWIN;
                break;
            default:
                bRet = FALSE;
                *penSclDmaId = E_MDRV_SCLDMA_ID_NUM;
                break;
        }

    }

    return bRet;
}

void _DrvSclDmaIoFillBufferConfig(DrvSclDmaIoBufferConfig_t *stIODMABufferCfg,MDrvSclDmaBufferConfig_t *stDMABufferCfg)
{
    stDMABufferCfg->u8Flag = stIODMABufferCfg->u8Flag;
    stDMABufferCfg->enBufMDType = stIODMABufferCfg->enBufMDType;
    stDMABufferCfg->enColorType = stIODMABufferCfg->enColorType;
    stDMABufferCfg->enMemType = stIODMABufferCfg->enMemType;
    stDMABufferCfg->u16BufNum = stIODMABufferCfg->u16BufNum;
    stDMABufferCfg->u16Height = stIODMABufferCfg->u16Height;
    stDMABufferCfg->u16Width = stIODMABufferCfg->u16Width;
    stDMABufferCfg->bHFlip  = stIODMABufferCfg->bHFlip;
    stDMABufferCfg->bVFlip  = stIODMABufferCfg->bVFlip;
    DrvSclOsMemcpy(stDMABufferCfg->u32Base_Y,stIODMABufferCfg->u32Base_Y,sizeof(unsigned long)*BUFFER_BE_ALLOCATED_MAX);
    DrvSclOsMemcpy(stDMABufferCfg->u32Base_C,stIODMABufferCfg->u32Base_C,sizeof(unsigned long)*BUFFER_BE_ALLOCATED_MAX);
    DrvSclOsMemcpy(stDMABufferCfg->u32Base_V,stIODMABufferCfg->u32Base_V,sizeof(unsigned long)*BUFFER_BE_ALLOCATED_MAX);

}




DrvSclOsClkIdType_e _DrvSclDmaIoTransClkId(MDrvSclDmaIdType_e enMDrvSclDmaId)
{
    DrvSclOsClkIdType_e enClkId = E_DRV_SCLOS_CLK_ID_NUM;
    switch(enMDrvSclDmaId)
    {
        case E_MDRV_SCLDMA_ID_1:
            enClkId = E_DRV_SCLOS_CLK_ID_DMA1;
            break;

        case E_MDRV_SCLDMA_ID_2:
            enClkId = E_DRV_SCLOS_CLK_ID_DMA2;
            break;

        case E_MDRV_SCLDMA_ID_3:
            enClkId = E_DRV_SCLOS_CLK_ID_DMA3;
            break;

        case E_MDRV_SCLDMA_ID_PNL:
        case E_MDRV_SCLDMA_ID_MDWIN:
            enClkId = E_DRV_SCLOS_CLK_ID_DMA4;
            break;

        case E_MDRV_SCLDMA_ID_NUM:
            enClkId = E_DRV_SCLOS_CLK_ID_NUM;
            break;
    }
    return enClkId;
}
DrvSclosDevType_e _DrvSclDmaGetDevId(DrvSclDmaIoIdType_e enSclDmaId)
{
    DrvSclosDevType_e enDev;
    switch(enSclDmaId)
    {
        case E_DRV_SCLDMA_IO_ID_1:
            enDev = E_DRV_SCLOS_DEV_DMA_1;
            break;
        case E_DRV_SCLDMA_IO_ID_2:
            enDev = E_DRV_SCLOS_DEV_DMA_2;
            break;
        case E_DRV_SCLDMA_IO_ID_3:
            enDev = E_DRV_SCLOS_DEV_DMA_3;
            break;
        case E_DRV_SCLDMA_IO_ID_4:
            enDev = E_DRV_SCLOS_DEV_DMA_4;
            break;
        default:
            enDev = E_DRV_SCLOS_DEV_MAX;
                break;
    }
    return enDev;
}

MDrvSclDmaIdType_e _DrvSclDmaGetDmaId(DrvSclDmaIoIdType_e enSclDmaId)
{
    MDrvSclDmaIdType_e enDev;
    switch(enSclDmaId)
    {
        case E_DRV_SCLDMA_IO_ID_1:
            enDev = E_MDRV_SCLDMA_ID_1;
            break;
        case E_DRV_SCLDMA_IO_ID_2:
            enDev = E_MDRV_SCLDMA_ID_2;
            break;
        case E_DRV_SCLDMA_IO_ID_3:
            enDev = E_MDRV_SCLDMA_ID_3;
            break;
        case E_DRV_SCLDMA_IO_ID_4:
            enDev = E_MDRV_SCLDMA_ID_MDWIN;
            break;
        default:
            enDev = E_DRV_SCLOS_DEV_MAX;
                break;
    }
    return enDev;
}
void _DrvSclDmaIoFreeDmem(const char* name, unsigned int size, void *virt, u32 addr)
{
    DrvSclOsDirectMemFree(name, size, virt, addr);
}
void _DrvSclDmaIoMemFreeYCMbuffer(MDrvSclCtxCmdqConfig_t *pvCtx)
{
    MDrvSclCtxNrBufferGlobalSet_t *pstGlobalSet;
    pstGlobalSet = &(((MDrvSclCtxGlobalSet_t*)(pvCtx->pgstGlobalSet))->stNrBufferCfg);
    if(pstGlobalSet->pvMcnrYCMVirAddr != 0)
    {
        SCL_DBG(SCL_DBG_LV_HVSP()&EN_DBGMG_HVSPLEVEL_HVSP1, "[SCLDMA] YC free %lx\n",(u32)pstGlobalSet->pvMcnrYCMVirAddr);
        SCL_ERR("[SCLDMA] YC free %lx\n",(u32)pstGlobalSet->pvMcnrYCMVirAddr);
        _DrvSclDmaIoFreeDmem(pstGlobalSet->u8McnrYCMName,
                            PAGE_ALIGN(pstGlobalSet->u32McnrSize),
                            pstGlobalSet->pvMcnrYCMVirAddr,
                            pstGlobalSet->PhyMcnrYCMAddr);

        pstGlobalSet->pvMcnrYCMVirAddr = 0;
        pstGlobalSet->PhyMcnrYCMAddr = 0;
        DrvSclOsSetSclFrameBufferAlloced(DrvSclOsGetSclFrameBufferAlloced()&(~E_DRV_SCLOS_FBALLOCED_YCM));
    }
    else
    {
        SCL_ERR("[SCLDMA]not alloced YCMbuffer\n");
    }
}

//-------------------------------------------------------------------------------------------------
//  Global Functions
//-------------------------------------------------------------------------------------------------
bool _DrvSclDmaIoDeInit(DrvSclDmaIoIdType_e enSclDmaId)
{
    DrvSclosProbeType_e enType;
    enType = (enSclDmaId==E_DRV_SCLDMA_IO_ID_1) ? E_DRV_SCLOS_INIT_DMA_1 :
            (enSclDmaId==E_DRV_SCLDMA_IO_ID_2) ? E_DRV_SCLOS_INIT_DMA_2 :
            (enSclDmaId==E_DRV_SCLDMA_IO_ID_3) ?   E_DRV_SCLOS_INIT_DMA_3 :
             (enSclDmaId==E_DRV_SCLDMA_IO_ID_4) ?    E_DRV_SCLOS_INIT_DMA_4:
                E_DRV_SCLOS_INIT_NONE;
    DrvSclOsClearProbeInformation(enType);
    if(DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_ALL) == 0)
    {
        MDrvSclDmaExit(1);
    }
    else if(!DrvSclOsGetProbeInformation(E_DRV_SCLOS_INIT_DMA))
    {
        MDrvSclDmaExit(0);
    }
    if(s32SclDmaIoHandlerMutex != -1)
    {
         DrvSclOsDeleteMutex(s32SclDmaIoHandlerMutex);
         s32SclDmaIoHandlerMutex = -1;
    }
    return TRUE;
}
bool _DrvSclDmaIoInit(DrvSclDmaIoIdType_e enSclDmaId)
{
    MDrvSclDmaInitConfig_t stSCLDMAInitCfg;
    MDrvSclDmaIdType_e enMSclDmaId;
    u16 i, start, end;
    MDrvSclCtxConfig_t *pvCfg;
    DrvSclOsMemset(&stSCLDMAInitCfg,0,sizeof(MDrvSclDmaInitConfig_t));
    if(enSclDmaId >= E_DRV_SCLDMA_IO_ID_NUM)
    {
        SCL_ERR("%s %d, Id out of range: %d\n", __FUNCTION__, __LINE__, enSclDmaId);
        return FALSE;
    }

    if(s32SclDmaIoHandlerMutex == -1)
    {
        s32SclDmaIoHandlerMutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, "SCLDMA_IO", SCLOS_PROCESS_SHARED);

        if(s32SclDmaIoHandlerMutex == -1)
        {
            SCL_ERR("%s %d, Create Mutex Fail\n", __FUNCTION__, __LINE__);
            return FALSE;
        }
    }
    enMSclDmaId = _DrvSclDmaGetDmaId(enSclDmaId);
    // Handler
    start = enSclDmaId * DRV_SCLDMA_HANDLER_INSTANCE_NUM;
    end = start + DRV_SCLDMA_HANDLER_INSTANCE_NUM;

    for(i=start; i<end; i++)
    {
        _gstSclDmaHandler[i].s32Handle = -1;
        _gstSclDmaHandler[i].enSclDmaId = E_DRV_SCLDMA_IO_ID_NUM;
        _gstSclDmaHandler[i].stCtxCfg.s32Id = -1;
        _gstSclDmaHandler[i].stCtxCfg.enIdType = E_DRV_SCLDMA_IO_ID_NUM;
        _gstSclDmaHandler[i].stCtxCfg.pLockCfg = NULL;
        _gstSclDmaHandler[i].stCtxCfg.pCmdqCtx = NULL;
    }
    //Ctx Init
    if( MDrvSclCtxInit() == FALSE)
    {
        SCL_ERR("%s %d, Init Ctx\n", __FUNCTION__, __LINE__);
        return FALSE;
    }
    //Dma Function
    if(enSclDmaId == E_DRV_SCLDMA_IO_ID_1)
    {
        DrvSclOsMemset(&_gstSclDmaFunc, 0, sizeof(DrvSclDmaIoFunctionConfig_t));
        _gstSclDmaFunc.DrvSclDmaIoSetInBufferConfig           = _DrvSclDmaIoSetInBufferConfig;
        _gstSclDmaFunc.DrvSclDmaIoSetInTriggerConfig          = _DrvSclDmaIoSetInTriggerConfig;
        _gstSclDmaFunc.DrvSclDmaIoSetOutBufferConfig          = _DrvSclDmaIoSetOutBufferConfig;
        _gstSclDmaFunc.DrvSclDmaIoSetOutTriggerConfig         = _DrvSclDmaIoSetOutTriggerConfig;
        _gstSclDmaFunc.DrvSclDmaIoGetInformationConfig        = _DrvSclDmaIoGetInformationConfig;
        _gstSclDmaFunc.DrvSclDmaIoGetInActiveBufferConfig     = _DrvSclDmaIoGetInActiveBufferConfig;
        _gstSclDmaFunc.DrvSclDmaIoGetOutActiveBufferConfig    = _DrvSclDmaIoGetOutActiveBufferConfig;
        _gstSclDmaFunc.DrvSclDmaIoBufferQueueHandleConfig     = _DrvSclDmaIoBufferQueueHandleConfig;
        _gstSclDmaFunc.DrvSclDmaIoGetPrivateIdConfig          = _DrvSclDmaIoGetPrivateIdConfig;
        _gstSclDmaFunc.DrvSclDmaIoCreateInstConfig            = _DrvSclDmaIoCreateInstConfig;
        _gstSclDmaFunc.DrvSclDmaIoDestroyInstConfig           = _DrvSclDmaIoDestroyInstConfig;
        _gstSclDmaFunc.DrvSclDmaIoInstProcessConfig           = _DrvSclDmaIoInstProcess;
        _gstSclDmaFunc.DrvSclDmaIoGetVersion                  = _DrvSclDmaIoGetVersion;
    }

    //ToDo each Dma init
    stSCLDMAInitCfg.u32Riubase = 0x1F000000; //ToDo
    stSCLDMAInitCfg.u32IRQNUM     = DrvSclOsGetIrqIDSCL(E_DRV_SCLOS_SCLIRQ_SC0);
    stSCLDMAInitCfg.u32CMDQIRQNUM = DrvSclOsGetIrqIDCMDQ(E_DRV_SCLOS_CMDQIRQ_CMDQ0);
    pvCfg = MDrvSclCtxGetDefaultCtx();
    stSCLDMAInitCfg.pvCtx   = (void *)&(pvCfg->stCtx);
    if( MDrvSclDmaInit(enMSclDmaId, &stSCLDMAInitCfg) == 0)
    {
        return -EFAULT;
    }
    return TRUE;
}

s32 _DrvSclDmaIoOpen(DrvSclDmaIoIdType_e enSclDmaId)
{
    s32 s32Handle = -1;
    s16 s16Idx = -1;
    s16 i ;
    MDrvSclCtxIdType_e enCtxId;
    DRV_SCLDMA_IO_LOCK_MUTEX(s32SclDmaIoHandlerMutex);
    for(i=0; i<DRV_SCLDMA_HANDLER_MAX; i++)
    {
        if(_gstSclDmaHandler[i].s32Handle == -1)
        {
            s16Idx = i;
            break;
        }
    }
#if I2_DVR
    enCtxId = E_MDRV_SCL_CTX_ID_SC_ALL;
#else
    if(enSclDmaId == E_DRV_SCLDMA_IO_ID_3)
    {
        enCtxId = E_MDRV_SCL_CTX_ID_SC_3;
    }
    else
    {
        enCtxId = E_MDRV_SCL_CTX_ID_SC_ALL;
    }
#endif

    if(s16Idx == -1)
    {
        s32Handle = -1;
        SCL_ERR("[SCLDMA]: Handler is not empyt\n");
    }
    else
    {
        s32Handle = s16Idx | DRV_SCLDMA_HANDLER_PRE_FIX | (enSclDmaId<<(HANDLER_PRE_FIX_SHIFT));
        _gstSclDmaHandler[s16Idx].s32Handle = s32Handle ;
        _gstSclDmaHandler[s16Idx].enSclDmaId = enSclDmaId;
        _gstSclDmaHandler[s16Idx].stCtxCfg.enCtxId = enCtxId;
        _gstSclDmaHandler[s16Idx].stCtxCfg.enIdType = enSclDmaId;
        _gstSclDmaHandler[s16Idx].stCtxCfg.s32Id = s32Handle;
        _gstSclDmaHandler[s16Idx].stCtxCfg.pLockCfg = MDrvSclCtxGetLockConfig(enCtxId);
        _gstSclDmaHandler[s16Idx].stCtxCfg.pCmdqCtx = MDrvSclCtxGetDefaultCtx();

    }
    DRV_SCLDMA_IO_UNLOCK_MUTEX(s32SclDmaIoHandlerMutex);
    return s32Handle;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoRelease(s32 s32Handler)
{
    s16 s16Idx = -1;
    s16 i ;
    u16 u16loop = 0;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    for(i=0; i<DRV_SCLDMA_HANDLER_MAX; i++)
    {
        if(_gstSclDmaHandler[i].s32Handle == s32Handler)
        {
            s16Idx = i;
            break;
        }
    }

    if(s16Idx == -1)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        eRet = E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    else
    {

        MDrvSclDmaIdType_e enMdrvIdType;

        if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
        {
            SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
            eRet = E_DRV_SCLDMA_IO_ERR_INVAL;
        }
        else
        {
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_SC1HLEVEL, "[SCLDMA %d] Release:: == %lx == \n",  enMdrvIdType,s32Handler);
            _gstSclDmaHandler[s16Idx].s32Handle = -1;
            _gstSclDmaHandler[s16Idx].enSclDmaId = E_DRV_SCLDMA_IO_ID_NUM;
            _gstSclDmaHandler[s16Idx].stCtxCfg.s32Id = -1;
            _gstSclDmaHandler[s16Idx].stCtxCfg.enIdType = E_DRV_SCLDMA_IO_ID_NUM;
            _gstSclDmaHandler[s16Idx].stCtxCfg.pCmdqCtx = NULL;
            _gstSclDmaHandler[s16Idx].stCtxCfg.pLockCfg = NULL;
            for(i = 0; i < DRV_SCLDMA_HANDLER_MAX; i++)
            {
                if(_gstSclDmaHandler[i].s32Handle != -1)
                {
                    u16loop = 1;
                    break;
                }
            }
            if(!u16loop)
            {
                MDrvSclDmaRelease(enMdrvIdType,  (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType)));
                MDrvSclDmaReSetHw(enMdrvIdType,  (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType)));
            }
            eRet = E_DRV_SCLDMA_IO_ERR_OK;
        }
    }
    return eRet;
}
void _DrvSclDmaIoPollLinux(MDrvSclDmaIdType_e enMdrvIdType, DrvSclDmaIoWrapperPollConfig_t *pstIoInCfg)
{
    bool bFRMDone;
    MDrvSclDmaBufferDoneConfig_t stDoneCfg;
    DrvSclOsMemset(&stDoneCfg,0,sizeof(MDrvSclDmaBufferDoneConfig_t));
    pstIoInCfg->u8retval = 0;

    MDrvSclDmaSetPollWait(&pstIoInCfg->stPollWaitCfg);
    if(enMdrvIdType == E_MDRV_SCLDMA_ID_3 || enMdrvIdType == E_MDRV_SCLDMA_ID_PNL)
    {
        bFRMDone = 0;
        if(MDrvSclDmaGetOutBufferDoneEvent(enMdrvIdType, E_MDRV_SCLDMA_MEM_FRM, &stDoneCfg))
        {
            bFRMDone = stDoneCfg.bDone ? 1: 0;
        }
        else
        {
            bFRMDone = 2;
        }

        if(bFRMDone == 1)
        {
            pstIoInCfg->u8retval |= (enMdrvIdType == E_MDRV_SCLDMA_ID_3) ?
                                    (SCLOS_POLLIN | SCLOS_POLLOUT) :
                                    (SCLOS_POLLOUT | SCLOS_POLLWRNORM);
        }
        else
        {
            pstIoInCfg->u8retval = 0;
        }
    }
    else
    {
        bFRMDone = 2;

        if(MDrvSclDmaGetOutBufferDoneEvent(enMdrvIdType, E_MDRV_SCLDMA_MEM_FRM, &stDoneCfg))
        {
            bFRMDone = (stDoneCfg.bDone == 0x1 ) ? 0x1 :
                       (stDoneCfg.bDone == 0x2 ) ? 0x2 :
                       (stDoneCfg.bDone == 0x3 ) ? 0x3 :
                       (stDoneCfg.bDone == 0xF ) ? 0xF : 0;
        }
        else
        {
            bFRMDone = 0;
        }
        if(bFRMDone &&bFRMDone!=0xF)
        {
            if(bFRMDone& 0x1)
            {
                pstIoInCfg->u8retval |= SCLOS_POLLIN; /* read */
            }
            if(bFRMDone& 0x2)
            {
                pstIoInCfg->u8retval |= SCLOS_POLLPRI;
            }
        }
        else if(bFRMDone ==0)
        {
            pstIoInCfg->u8retval = 0;
        }
        else
        {
            pstIoInCfg->u8retval = SCLOS_POLLERR;
        }
    }
}



void _DrvSclDmaIoPollRtk(MDrvSclDmaIdType_e enMdrvIdType, DrvSclDmaIoWrapperPollConfig_t *pstIoInCfg)
{
    bool bFRMDone;
    MDrvSclDmaBufferDoneConfig_t stDoneCfg;
    u32 u32Timeout;
    u32 u32Time = 0;
    u32Time = (((u32)DrvSclOsGetSystemTimeStamp()));
    bFRMDone = 2;
    while(1)
    {
        if(MDrvSclDmaGetOutBufferDoneEvent(enMdrvIdType, E_MDRV_SCLDMA_MEM_FRM, &stDoneCfg))
        {
            bFRMDone = (stDoneCfg.bDone == 0x1 )? 0x1 :
                       (stDoneCfg.bDone == 0x2 )? 0x2 :
                       (stDoneCfg.bDone == 0x3 )? 0x3 :
                       (stDoneCfg.bDone == 0xF ) ? 0xF : 0;
        }
        else
        {
            bFRMDone = 0;
        }
        if(bFRMDone &&bFRMDone!=0xF)
        {
            if(bFRMDone& 0x1)
            {
                pstIoInCfg->u8retval |= 0x1; /* read */
            }
            if(bFRMDone& 0x2)
            {
                pstIoInCfg->u8retval |= 0x2;
            }
        }
        else if(bFRMDone ==0)
        {
            pstIoInCfg->u8retval = 0;
        }
        else
        {
            pstIoInCfg->u8retval = 0x8;//POLLERR
        }
        pstIoInCfg->u8retval &= (pstIoInCfg->u8pollval|0x8);

        if( pstIoInCfg->pfnCb )
        {
            if(pstIoInCfg->u8retval)
            {
                pstIoInCfg->pfnCb();
            }
            break;
        }
        else
        {
            if(pstIoInCfg->u8retval)
            {
                break;
            }
            else
            {
                u32Timeout = MDrvSclDmaSetPollWait(&pstIoInCfg->stPollWaitCfg);
                if(u32Timeout)
                {
                    SCL_ERR("%s %d, POLL TIMEOUT :%d @:%lu\n",__FUNCTION__, __LINE__,enMdrvIdType,u32Time);
                    break;
                }
            }
        }
    }
}

DrvSclDmaIoErrType_e _DrvSclDmaIoPoll(s32 s32Handler, DrvSclDmaIoWrapperPollConfig_t *pstIoInCfg)
{
    MDrvSclDmaIdType_e enMdrvIdType;

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(pstIoInCfg->bWaitQueue)
    {
        _DrvSclDmaIoPollLinux(enMdrvIdType, pstIoInCfg);
    }
    else
    {
        _DrvSclDmaIoPollRtk(enMdrvIdType, pstIoInCfg);
    }

    return E_DRV_SCLDMA_IO_ERR_OK;
}
void _DrvSclDmaIoKeepCmdqFunction(DrvSclOsCmdqInterface_t *pstCmdq)
{
    MDrvSclCtxKeepCmdqFunction(pstCmdq);
}

DrvSclDmaIoErrType_e _DrvSclDmaIoSetInBufferConfig(s32 s32Handler, DrvSclDmaIoBufferConfig_t *pstIoInCfg)
{
    MDrvSclDmaBufferConfig_t stDMABufferCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stDMABufferCfg,0,sizeof(MDrvSclDmaBufferConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoBufferConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    //scldma1, scldma2 not support
    if(IsMDrvScldmaIdType_1(enMdrvIdType) || IsMDrvScldmaIdType_2(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA] Not Support %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    enCtxId = pstSclDmaCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    //do when id is scldma3 & scldma4
    _DrvSclDmaIoFillBufferConfig(pstIoInCfg,&stDMABufferCfg);
    stDMABufferCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclDmaSetDmaReadClientConfig(enMdrvIdType,  &stDMABufferCfg))
    {
        MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);

    return E_DRV_SCLDMA_IO_ERR_OK;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoSetInTriggerConfig(s32 s32Handler, DrvSclDmaIoTriggerConfig_t *pstIoInCfg)
{
    MDrvSclDmaTriggerConfig_t stDrvTrigCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stDrvTrigCfg,0,sizeof(MDrvSclDmaTriggerConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoTriggerConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    //scldma1, scldma2 not support
    if(IsMDrvScldmaIdType_1(enMdrvIdType) || IsMDrvScldmaIdType_2(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA] Not Support %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    //do when id is scldma3 & scldma4
    enCtxId = pstSclDmaCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stDrvTrigCfg.bEn = pstIoInCfg->bEn;
    stDrvTrigCfg.enMemType = pstIoInCfg->enMemType;
    stDrvTrigCfg.stclk = (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType));
    stDrvTrigCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclDmaSetDmaReadClientTrigger(enMdrvIdType,  &stDrvTrigCfg))
    {
        MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);

    return E_DRV_SCLDMA_IO_ERR_OK;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoSetOutBufferConfig(s32 s32Handler, DrvSclDmaIoBufferConfig_t *pstIoInCfg)
{
    MDrvSclDmaBufferConfig_t stDMABufferCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stDMABufferCfg,0,sizeof(MDrvSclDmaBufferConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoBufferConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }


    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    //scldma4 not support
    if(IsMDrvScldmaIdType_PNL(enMdrvIdType) )
    {
        SCL_ERR( "[SCLDMA] Not Support %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);
    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    enCtxId = pstSclDmaCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    _DrvSclDmaIoFillBufferConfig(pstIoInCfg,&stDMABufferCfg);
    stDMABufferCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclDmaSetDmaWriteClientConfig(enMdrvIdType,  &stDMABufferCfg))
    {
        MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return E_DRV_SCLDMA_IO_ERR_OK;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoSetOutTriggerConfig(s32 s32Handler, DrvSclDmaIoTriggerConfig_t *pstIoInCfg)
{
    //DrvSclDmaIoErrType_e bret = FALSE;
    MDrvSclDmaTriggerConfig_t stDrvTrigCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stDrvTrigCfg,0,sizeof(MDrvSclDmaTriggerConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoTriggerConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    //scldma4 not support
    if(IsMDrvScldmaIdType_PNL(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA] Not support %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }



    //do when id is scldma1, scldma2 & scldma3
    enCtxId = pstSclDmaCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stDrvTrigCfg.bEn = pstIoInCfg->bEn;
    stDrvTrigCfg.enMemType = pstIoInCfg->enMemType;
    stDrvTrigCfg.stclk = (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType));
    stDrvTrigCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclDmaSetDmaWriteClientTrigger(enMdrvIdType,  &stDrvTrigCfg))
    {
        MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return E_DRV_SCLDMA_IO_ERR_OK;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoGetInformationConfig(s32 s32Handler, DrvSclDmaIoGetInformationConfig_t *pstIoInCfg)
{
    MDrvSclDmaAttrType_t stDmaInfo;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    u32 u32Bufferidx;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    void *pvCtx;
    MDrvSclDmaGetConfig_t stGetCfg;
    DrvSclOsMemset(&stGetCfg,0,sizeof(MDrvSclDmaGetConfig_t));
    DrvSclOsMemset(&stDmaInfo,0,sizeof(MDrvSclDmaAttrType_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoGetInformationConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    MDrvSclCtxSetLockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    pvCtx = (void *)MDrvSclCtxGetConfigCtx(pstSclDmaCtxCfg->enCtxId);

    if(pstIoInCfg->enMemType != E_DRV_SCLDMA_IO_MEM_NUM)
    {
        stGetCfg.bReadDMAMode = 0;
        stGetCfg.enMemType = pstIoInCfg->enMemType;
        stGetCfg.enSCLDMA_ID = enMdrvIdType;
        stGetCfg.pvCtx = pvCtx;
        MDrvSclDmaGetDmaInformationByClient(&stGetCfg,&stDmaInfo);
    }
    else
    {
        SCL_ERR( "[SCLDMA] Not Support %s %d\n", __FUNCTION__, __LINE__);
    }
    pstIoInCfg->enBufMDType = stDmaInfo.enBufMDType;
    pstIoInCfg->enColorType = stDmaInfo.enColorType;
    pstIoInCfg->u16BufNum = stDmaInfo.u16BufNum;
    pstIoInCfg->u16DMAH = stDmaInfo.u16DMAH;
    pstIoInCfg->u16DMAV = stDmaInfo.u16DMAV;
    for(u32Bufferidx=0;u32Bufferidx<BUFFER_BE_ALLOCATED_MAX;u32Bufferidx++)
    {
        pstIoInCfg->u32Base_C[u32Bufferidx] = stDmaInfo.u32Base_C[u32Bufferidx];
        pstIoInCfg->u32Base_V[u32Bufferidx] = stDmaInfo.u32Base_V[u32Bufferidx];
        pstIoInCfg->u32Base_Y[u32Bufferidx] = stDmaInfo.u32Base_Y[u32Bufferidx];
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    return E_DRV_SCLDMA_IO_ERR_OK;

}
DrvSclDmaIoErrType_e _DrvSclDmaIoGetInActiveBufferConfig(s32 s32Handler, DrvSclDmaIoActiveBufferConfig_t *pstIoInCfg)
{
    MDrvSclDmaActiveBufferConfig_t stActiveCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    DrvSclOsMemset(&stActiveCfg,0,sizeof(MDrvSclDmaActiveBufferConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoActiveBufferConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    //scldma1, scldma2 not support
    if(IsMDrvScldmaIdType_1(enMdrvIdType) || IsMDrvScldmaIdType_2(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA] Not support %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if((pstSclDmaCtxCfg) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  Lock Busy\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    stActiveCfg.enMemType = pstIoInCfg->enMemType;
    stActiveCfg.u8ActiveBuffer = pstIoInCfg->u8ActiveBuffer;
    stActiveCfg.stOnOff.stclk = (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType));
    MDrvSclCtxSetLockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    stActiveCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(pstSclDmaCtxCfg->enCtxId);
    if(!MDrvSclDmaGetDmaReadBufferActiveIdx(enMdrvIdType,  &stActiveCfg))
    {
        eRet = E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    else
    {
        pstIoInCfg->u8ActiveBuffer  = stActiveCfg.u8ActiveBuffer;
        pstIoInCfg->enMemType       = stActiveCfg.enMemType;
        pstIoInCfg->u8ISPcount      = stActiveCfg.u8ISPcount;
        pstIoInCfg->u64FRMDoneTime  = stActiveCfg.u64FRMDoneTime;
        eRet = E_DRV_SCLDMA_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    return eRet;

}
DrvSclDmaIoErrType_e _DrvSclDmaIoGetOutActiveBufferConfig(s32 s32Handler, DrvSclDmaIoActiveBufferConfig_t *pstIoInCfg)
{
    MDrvSclDmaActiveBufferConfig_t stActiveCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    DrvSclOsMemset(&stActiveCfg,0,sizeof(MDrvSclDmaActiveBufferConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoActiveBufferConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(IsMDrvScldmaIdType_Max(enMdrvIdType))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    //scldma4 not support
    if(IsMDrvScldmaIdType_PNL(enMdrvIdType) )
    {
        SCL_ERR( "[SCLDMA] Notsupport %s %d\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    stActiveCfg.enMemType = pstIoInCfg->enMemType;
    stActiveCfg.u8ActiveBuffer = pstIoInCfg->u8ActiveBuffer;
    stActiveCfg.stOnOff.stclk = (MDrvSclDmaClkConfig_t *)DrvSclOsClkGetConfig(_DrvSclDmaIoTransClkId(enMdrvIdType));
    MDrvSclCtxSetLockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    stActiveCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(pstSclDmaCtxCfg->enCtxId);

    if(!MDrvSclDmaGetDmaWriteBufferAcitveIdx(enMdrvIdType, &stActiveCfg))
    {
        eRet = E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    else
    {
        pstIoInCfg->u8ActiveBuffer  = stActiveCfg.u8ActiveBuffer;
        pstIoInCfg->enMemType       = stActiveCfg.enMemType;
        pstIoInCfg->u8ISPcount      = stActiveCfg.u8ISPcount;
        pstIoInCfg->u64FRMDoneTime  = stActiveCfg.u64FRMDoneTime;
        eRet = E_DRV_SCLDMA_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    return eRet;
}
DrvSclDmaIoErrType_e _DrvSclDmaIoBufferQueueHandleConfig(s32 s32Handler, DrvSclDmaIoBufferQueueConfig_t *pstIoInCfg)
{
    MDrvSclDmaBUfferQueueConfig_t stBufferQCfg;
    DrvSclDmaIoVersionChkConfig_t stVersion;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    MDrvSclCtxIdType_e enCtxId;
    DrvSclOsMemset(&stBufferQCfg,0,sizeof(MDrvSclDmaBUfferQueueConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoBufferQueueConfig_t),
                                                 pstIoInCfg->VerChk_Size,
                                                 &pstIoInCfg->VerChk_Version,&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    enCtxId = pstSclDmaCtxCfg->enCtxId;
    MDrvSclCtxSetLockConfig(s32Handler,enCtxId);
    stBufferQCfg.enMemType = pstIoInCfg->enMemType;
    stBufferQCfg.enUsedType = pstIoInCfg->enUsedType;
    stBufferQCfg.u8EnqueueIdx = pstIoInCfg->u8EnqueueIdx;
    stBufferQCfg.pvCtx = (void *)MDrvSclCtxGetConfigCtx(enCtxId);
    if(!MDrvSclDmaBufferQueueHandle(enMdrvIdType,  &stBufferQCfg))
    {
        eRet = E_DRV_SCLDMA_IO_ERR_FAULT;
    }
    else
    {
        DrvSclOsMemcpy(&pstIoInCfg->stRead,&stBufferQCfg.stRead,MDRV_SCLDMA_BUFFER_QUEUE_OFFSET);
        pstIoInCfg->u8InQueueCount = stBufferQCfg.u8InQueueCount;
        pstIoInCfg->u8EnqueueIdx   = stBufferQCfg.u8EnqueueIdx;
        eRet = E_DRV_SCLDMA_IO_ERR_OK;
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,enCtxId);
    return eRet;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoGetPrivateIdConfig(s32 s32Handler, DrvSclDmaIoPrivateIdConfig_t *pstIoInCfg)
{
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;

    pstSclDmaCtxCfg = _DrvSclDmaIoGetCtxConfig(s32Handler);

    if(pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }

    pstIoInCfg->s32Id = s32Handler;

    return E_DRV_SCLDMA_IO_ERR_OK;
}

DrvSclDmaIoErrType_e _DrvSclDmaIoCreateInstConfig(s32 s32Handler, DrvSclDmaIoLockConfig_t *pstIoInCfg)
{
    DrvSclDmaIoVersionChkConfig_t stVersion;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxInstConfig_t stCtxInst;
    DrvSclOsMemset(&stCtxInst,0,sizeof(MDrvSclCtxInstConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoLockConfig_t),
                                              (pstIoInCfg->VerChk_Size),
                                              &(pstIoInCfg->VerChk_Version),&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s  %d, version fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(pstSclDmaCtxCfg->enIdType != E_DRV_SCLDMA_IO_ID_1)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(pstIoInCfg->u8BufSize == 0 || pstIoInCfg->ps32IdBuf == NULL || pstIoInCfg->u8BufSize > MDRV_SCL_CTX_CLIENT_ID_MAX)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    DRV_SCLDMA_IO_LOCK_MUTEX(s32SclDmaIoHandlerMutex);
    //alloc Ctx handler
    stCtxInst.ps32IdBuf = pstIoInCfg->ps32IdBuf;
    stCtxInst.u8IdNum = pstIoInCfg->u8BufSize;
    pstSclDmaCtxCfg->pCmdqCtx = MDrvSclCtxAllocate(E_MDRV_SCL_CTX_ID_SC_ALL,&stCtxInst);
    //alloc DMA Ctx
    if(pstSclDmaCtxCfg->pCmdqCtx == NULL)
    {
        SCL_ERR("%s %d::Allocate Ctx Fail\n", __FUNCTION__, __LINE__);
        DRV_SCLDMA_IO_UNLOCK_MUTEX(s32SclDmaIoHandlerMutex);

        return E_DRV_SCLDMA_IO_ERR_FAULT;
    }



    DRV_SCLDMA_IO_UNLOCK_MUTEX(s32SclDmaIoHandlerMutex);

    return E_DRV_SCLDMA_IO_ERR_OK;
}
DrvSclDmaIoErrType_e _DrvSclDmaIoDestroyInstConfig(s32 s32Handler, DrvSclDmaIoLockConfig_t *pstIoInCfg)
{
    DrvSclDmaIoVersionChkConfig_t stVersion;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclCtxCmdqConfig_t *pvCfg;

    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoLockConfig_t),
                                              (pstIoInCfg->VerChk_Size),
                                              &(pstIoInCfg->VerChk_Version),&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s  %d, version fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(pstSclDmaCtxCfg->enIdType != E_DRV_SCLDMA_IO_ID_1)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, Not Support\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    if(pstIoInCfg->u8BufSize == 0 || pstIoInCfg->ps32IdBuf == NULL || pstIoInCfg->u8BufSize > MDRV_SCL_CTX_CLIENT_ID_MAX)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    DRV_SCLDMA_IO_LOCK_MUTEX(s32SclDmaIoHandlerMutex);
    //free Ctx handler
    MDrvSclCtxSetLockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    pvCfg = MDrvSclCtxGetConfigCtx(pstSclDmaCtxCfg->enCtxId);
    _DrvSclDmaIoMemFreeYCMbuffer(pvCfg);
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    MDrvSclCtxFree(pstSclDmaCtxCfg->pCmdqCtx);
    //free DMA Ctx

    DRV_SCLDMA_IO_UNLOCK_MUTEX(s32SclDmaIoHandlerMutex);

    return E_DRV_SCLDMA_IO_ERR_OK;
}
void _DrvSclDmaIoInstFillProcessCfg(MDrvSclDmaProcessConfig_t *stProcess, DrvSclDmaIoProcessConfig_t *pstIoInCfg)
{
    u8 j;
    stProcess->stCfg.bEn = pstIoInCfg->stCfg.bEn;
    stProcess->stCfg.enMemType = pstIoInCfg->stCfg.enMemType;
    for(j =0;j<3;j++)
    {
        stProcess->stCfg.stBufferInfo.u64PhyAddr[j]=
            pstIoInCfg->stCfg.stBufferInfo.u64PhyAddr[j];
        stProcess->stCfg.stBufferInfo.u32Stride[j]=
            pstIoInCfg->stCfg.stBufferInfo.u32Stride[j];
    }
}

DrvSclDmaIoErrType_e _DrvSclDmaIoInstProcess(s32 s32Handler, DrvSclDmaIoProcessConfig_t *pstIoInCfg)
{
    DrvSclDmaIoVersionChkConfig_t stVersion;
    DrvSclDmaIoCtxConfig_t *pstSclDmaCtxCfg;
    MDrvSclDmaProcessConfig_t stProcess;
    MDrvSclDmaIdType_e enMdrvIdType;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    DrvSclOsMemset(&stProcess,0,sizeof(MDrvSclDmaProcessConfig_t));
    _DrvSclDmaIoFillVersionChkStruct(sizeof(DrvSclDmaIoProcessConfig_t),
                                              (pstIoInCfg->VerChk_Size),
                                              &(pstIoInCfg->VerChk_Version),&stVersion);

    if(_DrvSclDmaIoVersionCheck(&stVersion))
    {
        SCL_ERR( "[SCLDMA]   %s  %d, version fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    if(_DrvSclDmaIoGetMdrvIdType(s32Handler, &enMdrvIdType) == FALSE)
    {
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    pstSclDmaCtxCfg =_DrvSclDmaIoGetCtxConfig(s32Handler);

    if( pstSclDmaCtxCfg == NULL)
    {
        SCL_ERR( "[SCLDMA]   %s  %d, ctx fail\n", __FUNCTION__, __LINE__);
        return E_DRV_SCLDMA_IO_ERR_INVAL;
    }
    MDrvSclCtxSetLockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    _DrvSclDmaIoInstFillProcessCfg(&stProcess,pstIoInCfg);
    stProcess.pvCtx = (void *)MDrvSclCtxGetConfigCtx(pstSclDmaCtxCfg->enCtxId);
    MDrvSclDmaInstProcess(enMdrvIdType,&stProcess);
    MDrvSclCtxSetUnlockConfig(s32Handler,pstSclDmaCtxCfg->enCtxId);
    return eRet;
}
DrvSclDmaIoErrType_e _DrvSclDmaIoInstFlip(s32 s32Handler)
{
    MDrvSclCtxCmdqConfig_t *pvCtx;
    DrvSclOsAccessRegType_e bAccessRegMode;
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;
    bool bFireMload = 0;
    MDrvSclCtxSetLockConfig(s32Handler,E_MDRV_SCL_CTX_ID_SC_ALL);
    if((s32Handler&HANDLER_PRE_MASK) == SCLM2M_HANDLER_PRE_FIX)
    {
        bAccessRegMode = DrvSclOsGetAccessRegMode();
        DrvSclOsSetAccessRegMode(E_DRV_SCLOS_AccessReg_CPU);
    }
    pvCtx = MDrvSclCtxGetConfigCtx(E_MDRV_SCL_CTX_ID_SC_ALL);
    MDrvSclCtxFire(pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_HSP_Y_SC1,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_VSP_Y_SC1,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_HSP_Y_SC2,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_VSP_Y_SC2,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_HSP_Y_SC3,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_VSP_Y_SC3,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_HSP_Y_SC4,pvCtx);
    bFireMload += MDrvSclCtxSetMload(E_MDRV_SCL_CTX_MLOAD_ID_VSP_Y_SC4,pvCtx);
    MDrvSclCtxFireMload(bFireMload,pvCtx);
    if((s32Handler&HANDLER_PRE_MASK) == SCLM2M_HANDLER_PRE_FIX)
    {
        DrvSclOsSetAccessRegMode(bAccessRegMode);
    }
    MDrvSclCtxSetUnlockConfig(s32Handler,E_MDRV_SCL_CTX_ID_SC_ALL);
    return eRet;
}
DrvSclDmaIoErrType_e _DrvSclDmaIoGetVersion(s32 s32Handler, DrvSclDmaIoVersionConfig_t*pstIoInCfg)
{
    DrvSclDmaIoErrType_e eRet = E_DRV_SCLDMA_IO_ERR_OK;

    if (CHK_VERCHK_HEADER( &(pstIoInCfg->VerChk_Version)) )
    {
        if( CHK_VERCHK_MAJORVERSION_LESS( &(pstIoInCfg->VerChk_Version), DRV_SCLDMA_VERSION) )
        {

            VERCHK_ERR("[SCLDMA] Version(%04lx) < %04x!!! \n",
                pstIoInCfg->VerChk_Version & VERCHK_VERSION_MASK,
                DRV_SCLDMA_VERSION);

            eRet = E_DRV_SCLDMA_IO_ERR_INVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE( &(pstIoInCfg->VerChk_Size),sizeof(DrvSclDmaIoLockConfig_t)) == 0 )
            {
                VERCHK_ERR("[SCLDMA] Size(%04x) != %04lx!!! \n",
                    sizeof(DrvSclDmaIoVersionChkConfig_t),
                    (pstIoInCfg->VerChk_Size));

                eRet = E_DRV_SCLDMA_IO_ERR_INVAL;
            }
            else
            {
                DrvSclDmaIoVersionConfig_t stCfg;

                stCfg = FILL_VERCHK_TYPE(stCfg, stCfg.VerChk_Version, stCfg.VerChk_Size, DRV_SCLDMA_VERSION);
                stCfg.u32Version = DRV_SCLDMA_VERSION;
                DrvSclOsMemcpy(pstIoInCfg, &stCfg, sizeof(DrvSclDmaIoVersionConfig_t));
                eRet = E_DRV_SCLDMA_IO_ERR_OK;
            }
        }
    }
    else
    {
        VERCHK_ERR("[SCLDMA] No Header !!! \n");
        SCL_ERR( "[SCLDMA]   %s %d  \n", __FUNCTION__, __LINE__);
        eRet = E_DRV_SCLDMA_IO_ERR_INVAL;
    }

    return eRet;
}
