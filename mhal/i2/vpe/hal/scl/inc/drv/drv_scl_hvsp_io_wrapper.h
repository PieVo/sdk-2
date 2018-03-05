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
#ifndef _DRV_SCL_HVSP_IO_WRAPPER_H__
#define _DRV_SCL_HVSP_IO_WRAPPER_H__

//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCLHVSP_HANDLER_PRE_FIX         SCLHVSP_HANDLER_PRE_FIX
#define DRV_SCLHVSP_HANDLER_PRE_MASK        0xFFFF0000
#define DRV_SCLHVSP_HANDLER_INSTANCE_NUM    (64)
#define DRV_SCLHVSP_HANDLER_MAX             (4 * DRV_SCLHVSP_HANDLER_INSTANCE_NUM)
#define DRV_SCLHVSP_HANDLER_DEV_MASK         0xF000

#define FHDWidth   1920
#define FHDHeight  1080
#define _3MWidth   2048
#define _3MHeight  1536


//-------------------------------------------------------------------------------------------------
//  structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    bool bWaitQueue;
    DrvSclOsPollWaitConfig_t stPollWaitCfg;
    PollCB *pCBFunc;
    u8 u8pollval;
    u8 u8retval;
}DrvSclHvspIoWrapperPollConfig_t;


//-------------------------------------------------------------------------------------------------
//  Prototype
//-------------------------------------------------------------------------------------------------

#ifndef __DRV_SCL_HVSP_IO_WRAPPER_C__
#define INTERFACE extern
#else
#define INTERFACE
#endif

INTERFACE bool                  _DrvSclHvspIoInit(DrvSclHvspIoIdType_e enSclHvspId);
INTERFACE bool _DrvSclHvspIoDeInit(DrvSclHvspIoIdType_e enSclHvspId);
INTERFACE s32                   _DrvSclHvspIoOpen(DrvSclHvspIoIdType_e enSclHvspId);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoRelease(s32 s32Handler);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoPoll(s32 s32Handler, DrvSclHvspIoWrapperPollConfig_t *pstIoPollCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetInputConfig(s32 s32Handler, DrvSclHvspIoInputConfig_t *pstIoInCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetOutputConfig(s32 s32Handler, DrvSclHvspIoOutputConfig_t *pstIoOutCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetScalingConfig(s32 s32Handler, DrvSclHvspIoScalingConfig_t *pstIOSclCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoReqmemConfig(s32 s32Handler, DrvSclHvspIoReqMemConfig_t*pstReqMemCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetMiscConfig(s32 s32Handler, DrvSclHvspIoMiscConfig_t *pstIOMiscCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetPostCropConfig(s32 s32Handler, DrvSclHvspIoPostCropConfig_t *pstIOPostCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoGetPrivateIdConfig(s32 s32Handler, DrvSclHvspIoPrivateIdConfig_t *pstIOCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoGetInformConfig(s32 s32Handler, DrvSclHvspIoScInformConfig_t *pstIOInfoCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoReleaseMemConfig(s32 s32Handler);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetOsdConfig(s32 s32Handler, DrvSclHvspIoOsdConfig_t *pstIOOSDCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetPriMaskConfig(s32 s32Handler, DrvSclHvspIoPriMaskConfig_t *pstIOPriMaskCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoPirMaskTrigerConfig(s32 s32Handler, DrvSclHvspIoPriMaskTriggerConfig_t *pstIOPriMaskTrigCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetFbConfig(s32 s32Handler, DrvSclHvspIoSetFbManageConfig_t *pstIOFbMgCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoGetVersion(s32 s32Hander, DrvSclHvspIoVersionConfig_t *pstIOVersionCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetRotateConfig(s32 s32Handler, DrvSclHvspIoRotateConfig_t *pstIoRotateCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetLockConfig(s32 s32Handler, DrvSclHvspIoLockConfig_t *pstIoLockCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetUnlockConfig(s32 s32Handler, DrvSclHvspIoLockConfig_t *pstIoLockCfg);
INTERFACE void _DrvSclHvspIoMemFree(void);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetVtrackConfig(s32 s32Handler, DrvSclHvspIoVtrackConfig_t *pstCfg);
INTERFACE DrvSclHvspIoErrType_e _DrvSclHvspIoSetVtrackOnOffConfig(s32 s32Handler, DrvSclHvspIoVtrackOnOffConfig_t *pstCfg);

#undef INTERFACE

#endif
