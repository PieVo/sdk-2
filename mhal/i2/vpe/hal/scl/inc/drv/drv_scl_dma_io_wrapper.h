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


//=============================================================================
#ifndef _DRV_SCL_DMA_IO_WRAPPER_H__
#define _DRV_SCL_DMA_IO_WRAPPER_H__

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCLDMA_HANDLER_PRE_FIX          SCLDMA_HANDLER_PRE_FIX
#define DRV_SCLDMA_HANDLER_PRE_MASK         0xFFFF0000
#define DRV_SCLDMA_HANDLER_DEV_MASK         0xF000
#define DRV_SCLDMA_HANDLER_INSTANCE_NUM     64
#define DRV_SCLDMA_HANDLER_MAX              (4 * DRV_SCLDMA_HANDLER_INSTANCE_NUM)
#define IsMDrvScldmaIdType_1(x)          (x == E_MDRV_SCLDMA_ID_1)
#define IsMDrvScldmaIdType_2(x)          (x == E_MDRV_SCLDMA_ID_2)
#define IsMDrvScldmaIdType_3(x)          (x == E_MDRV_SCLDMA_ID_3)
#define IsMDrvScldmaIdType_PNL(x)        (x == E_MDRV_SCLDMA_ID_PNL)
#define IsMDrvScldmaIdType_Max(x)        (x == E_MDRV_SCLDMA_ID_NUM)

//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    bool bWaitQueue;
    DrvSclOsPollWaitConfig_t stPollWaitCfg;
    pDrvSclDmaIoPollCb pfnCb;
    u8 u8pollval;
    u8 u8retval;
}DrvSclDmaIoWrapperPollConfig_t;


//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifndef __DRV_SCL_DMA_IO_WRAPPER_C__
#define INTERFACE extern
#else
#define INTERFACE
#endif
INTERFACE bool                 _DrvSclDmaIoInit(DrvSclDmaIoIdType_e enSclDmaId);
INTERFACE bool                  _DrvSclDmaIoDeInit(DrvSclDmaIoIdType_e enSclDmaId);
INTERFACE s32                  _DrvSclDmaIoOpen(DrvSclDmaIoIdType_e enSclDmaId);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoRelease(s32 s32Handler);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoPoll(s32 s32Handler ,DrvSclDmaIoWrapperPollConfig_t *pstCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSuspend(s32 s32Handler);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoResume(s32 s32Handler);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSetInBufferConfig(s32 s32Handler, DrvSclDmaIoBufferConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSetInTriggerConfig(s32 s32Handler, DrvSclDmaIoTriggerConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSetOutBufferConfig(s32 s32Handler, DrvSclDmaIoBufferConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSetOutTriggerConfig(s32 s32Handler, DrvSclDmaIoTriggerConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoGetInformationConfig(s32 s32Handler, DrvSclDmaIoGetInformationConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoGetInActiveBufferConfig(s32 s32Handler, DrvSclDmaIoActiveBufferConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoGetOutActiveBufferConfig(s32 s32Handler, DrvSclDmaIoActiveBufferConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoBufferQueueHandleConfig(s32 s32Handler, DrvSclDmaIoBufferQueueConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoGetPrivateIdConfig(s32 s32Handler, DrvSclDmaIoPrivateIdConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoCreateInstConfig(s32 s32Handler, DrvSclDmaIoLockConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoDestroyInstConfig(s32 s32Handler, DrvSclDmaIoLockConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoGetVersion(s32 s32Handler, DrvSclDmaIoVersionConfig_t*pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoSetExternalInputConfig(s32 s32Handler, DrvSclDmaIoExternalInputConfig_t *pstIoInCfg);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoInstProcess(s32 s32Handler, DrvSclDmaIoProcessConfig_t *pstIoInCfg);
INTERFACE void _DrvSclDmaIoKeepCmdqFunction(DrvSclOsCmdqInterface_t *pstCmdq);
INTERFACE DrvSclDmaIoErrType_e _DrvSclDmaIoInstFlip(s32 s32Handler);

#undef INTERFACE

#endif
