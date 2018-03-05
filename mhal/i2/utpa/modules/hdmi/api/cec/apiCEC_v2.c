//<MStar Software>//******************************************************************************// MStar Software// Copyright (c) 2010 - 2012 MStar Semiconductor, Inc. All rights reserved.// All software, firmware and related documentation herein ("MStar Software") are// intellectual property of MStar Semiconductor, Inc. ("MStar") and protected by// law, including, but not limited to, copyright law and international treaties.// Any use, modification, reproduction, retransmission, or republication of all// or part of MStar Software is expressly prohibited, unless prior written// permission has been granted by MStar.//// By accessing, browsing and/or using MStar Software, you acknowledge that you// have read, understood, and agree, to be bound by below terms ("Terms") and to// comply with all applicable laws and regulations://// 1. MStar shall retain any and all right, ownership and interest to MStar//    Software and any modification/derivatives thereof.//    No right, ownership, or interest to MStar Software and any//    modification/derivatives thereof is transferred to you under Terms.//// 2. You understand that MStar Software might include, incorporate or be//    supplied together with third party`s software and the use of MStar//    Software may require additional licenses from third parties.//    Therefore, you hereby agree it is your sole responsibility to separately//    obtain any and all third party right and license necessary for your use of//    such third party`s software.//// 3. MStar Software and any modification/derivatives thereof shall be deemed as//    MStar`s confidential information and you agree to keep MStar`s//    confidential information in strictest confidence and not disclose to any//    third party.//// 4. MStar Software is provided on an "AS IS" basis without warranties of any//    kind. Any warranties are hereby expressly disclaimed by MStar, including//    without limitation, any warranties of merchantability, non-infringement of//    intellectual property rights, fitness for a particular purpose, error free//    and in conformity with any international standard.  You agree to waive any//    claim against MStar for any loss, damage, cost or expense that you may//    incur related to your use of MStar Software.//    In no event shall MStar be liable for any direct, indirect, incidental or//    consequential damages, including without limitation, lost of profit or//    revenues, lost or damage of data, and unauthorized system use.//    You agree that this Section 4 shall still apply without being affected//    even if MStar Software has been modified by MStar in accordance with your//    request or instruction for your use, except otherwise agreed by both//    parties in writing.//// 5. If requested, MStar may from time to time provide technical supports or//    services in relation with MStar Software to you for your use of//    MStar Software in conjunction with your or your customer`s product//    ("Services").//    You understand and agree that, except otherwise agreed by both parties in//    writing, Services are provided on an "AS IS" basis and the warranty//    disclaimer set forth in Section 4 above shall apply.//// 6. Nothing contained herein shall be construed as by implication, estoppels//    or otherwise://    (a) conferring any license or right to use MStar name, trademark, service//        mark, symbol or any other identification;//    (b) obligating MStar or any of its affiliates to furnish any person,//        including without limitation, you and your customers, any assistance//        of any kind whatsoever, or any information; or//    (c) conferring any license or right under any intellectual property right.//// 7. These terms shall be governed by and construed in accordance with the laws//    of Taiwan, R.O.C., excluding its conflict of law rules.//    Any and all dispute arising out hereof or related hereto shall be finally//    settled by arbitration referred to the Chinese Arbitration Association,//    Taipei in accordance with the ROC Arbitration Law and the Arbitration//    Rules of the Association by three (3) arbitrators appointed in accordance//    with the said Rules.//    The place of arbitration shall be in Taipei, Taiwan and the language shall//    be English.//    The arbitration award shall be final and binding to both parties.////******************************************************************************//<MStar Software>///////////////////////////////////////////////////////////////////////////////////////////////////////// file    apiMHL.c/// @author MStar Semiconductor Inc./// @brief  MHL driver Function///////////////////////////////////////////////////////////////////////////////////////////////////#ifndef _API_CEC_V2_C_#define _API_CEC_V2_C_//-------------------------------------------------------------------------------------------------//  Include Files//-------------------------------------------------------------------------------------------------// Common Definition#include "MsCommon.h"#include "MsVersion.h"#ifdef MSOS_TYPE_LINUX_KERNEL#include <linux/string.h>#else#include <string.h>#endif#include "MsOS.h"#include "utopia.h"#include "utopia_dapi.h"#include "apiCEC_private.h"#include "apiCEC.h"#include "drvCEC.h"//-------------------------------------------------------------------------------------------------//  Local Defines//-------------------------------------------------------------------------------------------------#if(defined(CONFIG_MLOG))#include "ULog.h"#define MAPI_CEC_MSG_INFO(format, args...)      ULOGI("CEC", format, ##args)#define MAPI_CEC_MSG_WARNING(format, args...)   ULOGW("CEC", format, ##args)#define MAPI_CEC_MSG_DEBUG(format, args...)     ULOGD("CEC", format, ##args)#define MAPI_CEC_MSG_ERROR(format, args...)     ULOGE("CEC", format, ##args)#define MAPI_CEC_MSG_FATAL(format, args...)     ULOGF("CEC", format, ##args)#else#define MAPI_CEC_MSG_INFO(format, args...)      printf(format, ##args)#define MAPI_CEC_MSG_WARNING(format, args...)   printf(format, ##args)#define MAPI_CEC_MSG_DEBUG(format, args...)     printf(format, ##args)#define MAPI_CEC_MSG_ERROR(format, args...)     printf(format, ##args)#define MAPI_CEC_MSG_FATAL(format, args...)     printf(format, ##args)#endif//-------------------------------------------------------------------------------------------------//  Local Structures//-------------------------------------------------------------------------------------------------//-------------------------------------------------------------------------------------------------//  Global Variables//-------------------------------------------------------------------------------------------------//-------------------------------------------------------------------------------------------------//  Local Variables//-------------------------------------------------------------------------------------------------//-------------------------------------------------------------------------------------------------//  Local Functions//-------------------------------------------------------------------------------------------------//-------------------------------------------------------------------------------------------------//  Global Functions//-------------------------------------------------------------------------------------------------//**************************************************************************//  [Function Name]://                  CECOpen()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************MS_U32 CECOpen(void** ppInstance, const void* const pAttribute){    CEC_INSTANT_PRIVATE *psCECInstPri = NULL;    //UTOPIA_TRACE(MS_UTOPIA_DB_LEVEL_TRACE,printf("enter %s %d\n",__FUNCTION__,__LINE__));    // instance is allocated here, also can allocate private for internal use, ex, BDMA_INSTANT_PRIVATE    UtopiaInstanceCreate(sizeof(CEC_INSTANT_PRIVATE), ppInstance);    // setup func in private and assign the calling func in func ptr in instance private    UtopiaInstanceGetPrivate(*ppInstance, (void**)&psCECInstPri);    psCECInstPri->fpDDC2BIGetInfo= (IOCTL_DDC2BI_GET_INFO)MDrv_CEC_DDC2BI_GetInfo;    psCECInstPri->fpCECInitChip = (IOCTL_CEC_INIT_CHIP)MDrv_CEC_InitChip;    psCECInstPri->fpCECPortSelect = (IOCTL_CEC_PORT_SELECT)MDrv_CEC_PortSelect;    psCECInstPri->fpCECExit = (IOCTL_CEC_EXIT)MDrv_CEC_Exit;    psCECInstPri->fpCECSetMyLogicalAddress = (IOCTL_CEC_SET_MY_LOGICAL_ADDRESS)MDrv_CEC_SetMyLogicalAddress;    psCECInstPri->fpCECInit = (IOCTL_CEC_INIT)MDrv_CEC_Init;    psCECInstPri->fpCECCheckExistDevices= (IOCTL_CEC_EXIT)MDrv_CEC_CheckExistDevices;    psCECInstPri->fpCECNextDevice = (IOCTL_CEC_NEXT_DEVICE)MDrv_CEC_NextDevice;    psCECInstPri->fpCECChkRxBuf = (IOCTL_CEC_CHK_RX_BUF)MDrv_CEC_ChkRxBuf;    psCECInstPri->fpCECTxSendMsg = (IOCTL_CEC_TX_SEND_MSG)MDrv_CEC_TxSendMsg;    psCECInstPri->fpCECTxSendMsg2 = (IOCTL_CEC_TX_SEND_MSG2)MDrv_CEC_TxSendMsg2;    psCECInstPri->fpCECTxSendPollingMsg = (IOCTL_CEC_TX_SEND_POLLING_MSG)MDrv_CecTxSendPollingMsg;    psCECInstPri->fpCECMsgActiveSource = (IOCTL_CEC_MSG_ACTIVE_SOURCE)MDrv_CEC_Msg_ActiveSource;    psCECInstPri->fpCECMsgRoutingChange = (IOCTL_CEC_MSG_ROUTING_CHANGE)MDrv_CEC_Msg_RoutingChange;    psCECInstPri->fpCECMsgReqActiveSource = (IOCTL_CEC_MSG_REQ_ACTIVE_SOURCE)MDrv_CEC_Msg_ReqActiveSource;    psCECInstPri->fpCECMsgSetStreamPath = (IOCTL_CEC_MSG_SET_STREAM_PATH)MDrv_CEC_Msg_SetStreamPath;    psCECInstPri->fpCECMsgStandby = (IOCTL_CEC_MSG_STANDBY)MDrv_CEC_Msg_Standby;    psCECInstPri->fpCECMsgRecordOff= (IOCTL_CEC_MSG_RECORD_OFF)MDrv_CEC_Msg_RecordOff;    psCECInstPri->fpCECMsgRecordOn= (IOCTL_CEC_MSG_RECORD_ON)MDrv_CEC_Msg_RecordOn;    psCECInstPri->fpCECMsgReportCECVersion = (IOCTL_CEC_MSG_REPORT_CEC_VERSION)MDrv_CEC_Msg_ReportCECVersion;    psCECInstPri->fpCECMsgReqCECVersion = (IOCTL_CEC_MSG_REQ_CEC_VERSION)MDrv_CEC_Msg_ReqCECVersion;    psCECInstPri->fpCECMsgReportPhycalAddress = (IOCTL_CEC_MSG_REPORT_PHYCAL_ADDRESS)MDrv_CEC_Msg_ReportPhycalAddress;    psCECInstPri->fpCECMsgReqPhycalAddress = (IOCTL_CEC_MSG_REQ_PHYCAL_ADDRESS)MDrv_CEC_Msg_ReqPhycalAddress;    psCECInstPri->fpCECMsgDeckControl = (IOCTL_CEC_MSG_DECK_CONTROL)MDrv_CEC_Msg_DeckControl;    psCECInstPri->fpCECMsgDecStatus = (IOCTL_CEC_MSG_DEC_STATUS)MDrv_CEC_Msg_DecStatus;    psCECInstPri->fpCECMsgGiveDeckStatus = (IOCTL_CEC_MSG_GIVE_DECK_STATUS)MDrv_CEC_MSg_GiveDeckStatus;    psCECInstPri->fpCECMsgDCPlay = (IOCTL_CEC_MSG_DC_PLAY)MDrv_CEC_MSg_DCPlay;    psCECInstPri->fpCECMsgReqMenuStatus = (IOCTL_CEC_MSG_REQ_MENU_STATUS)MDrv_CEC_Msg_ReqMenuStatus;    psCECInstPri->fpCECMsgUserCtrlPressed = (IOCTL_CEC_MSG_USER_CTRL_PRESSED)MDrv_CEC_Msg_UserCtrlPressed;    psCECInstPri->fpCECMsgUserCtrlReleased = (IOCTL_CEC_MSG_USER_CTRL_RELEASED)MDrv_CEC_Msg_UserCtrlReleased;    psCECInstPri->fpCECMsgGiveAudioStatus = (IOCTL_CEC_MSG_GIVE_AUDIO_STATUS)MDrv_CEC_Msg_GiveAudioStatus;    psCECInstPri->fpCECMsgReportPowerStatus = (IOCTL_CEC_MSG_REPORT_POWER_STATUS)MDrv_CEC_Msg_ReportPowerStatus;    psCECInstPri->fpCECMsgReqPowerStatus = (IOCTL_CEC_MSG_REQ_POWER_STATUS)MDrv_CEC_Msg_ReqPowerStatus;    psCECInstPri->fpCECMsgFeatureAbort = (IOCTL_CEC_MSG_FEATURE_ABORT)MDrv_CEC_Msg_FeatureAbort;    psCECInstPri->fpCECMsgAbort = (IOCTL_CEC_MSG_ABORT)MDrv_CEC_Msg_Abort;    psCECInstPri->fpCECMsgSendMenuLanguage = (IOCTL_CEC_MSG_SEND_MENU_LANGUAGE)MDrv_CEC_Msg_SendMenuLanguage;    psCECInstPri->fpCECMsgReqARCInitiation = (IOCTL_CEC_MSG_REQ_ARC_INITIATION)MDrv_CecMsg_ReqARCInitiation;    psCECInstPri->fpCECMsgReqARCTermination = (IOCTL_CEC_MSG_REQ_ARC_INITIATION)MDrv_CecMsg_ReqARCTermination;    psCECInstPri->fpCECMsgAudioModeReq = (IOCTL_CEC_MSG_AUDIO_MODE_REQ)MDrv_CecMsg_AudioModeReq;    psCECInstPri->fpCECCheckFrame = (IOCTL_CEC_CHECK_FRAME)MDrv_CEC_CheckFrame;    psCECInstPri->fpCECConfigWakeUp = (IOCTL_CEC_CONFIG_WAKE_UP)MDrv_CEC_ConfigWakeUp;    psCECInstPri->fpCECEnabled = (IOCTL_CEC_ENABLED)MDrv_CEC_Enabled;    psCECInstPri->fpCECGetTxStatus = (IOCTL_CEC_GET_TX_STATUS)MDrv_CEC_TxStatus;    psCECInstPri->fpCECDeviceIsTx = (IOCTL_CEC_DEVICE_IS_TX)MDrv_CEC_CheckDeviceIsTx;    psCECInstPri->fpCECSetPowerState = (IOCTL_CEC_SET_POWER_STATE)MDrv_CEC_SetPowerState;#if ENABLE_CEC_MULTIPLE    psCECInstPri->fpCECSetMyLogicalAddress2 = (IOCTL_CEC_SET_MY_LOGICAL_ADDRESS2)MDrv_CEC_SetMyLogicalAddress2;    psCECInstPri->fpCECMsgReportPhycalAddress2 = (IOCTL_CEC_MSG_REPORT_PHYCAL_ADDRESS2)MDrv_CEC_Msg_ReportPhycalAddress2;    psCECInstPri->fpCECSetMyPhysicalAddress2= (IOCTL_CEC_SET_MY_PHYSICAL_ADDRESS2)MDrv_CEC_SetMyPhysicalAddress2;    psCECInstPri->fpCECSetInitiator = (IOCTL_CEC_SET_INITIATOR)MDrv_CEC_SetInitiator;#endif    psCECInstPri->fpCECGetHeader = (IOCTL_CEC_GET_HEADER)MDrv_CEC_Get_Header;    psCECInstPri->fpCECGetOpCode = (IOCTL_CEC_GET_OPCODE)MDrv_CEC_Get_OpCode;    psCECInstPri->fpCECGetPara = (IOCTL_CEC_GET_PARA)MDrv_CEC_Get_Para;    psCECInstPri->fpCECGetCmdLen = (IOCTL_CEC_GET_CMD_LEN)MDrv_CEC_GetCmdLen;    psCECInstPri->fpCECIsRxBufEmpty = (IOCTL_CEC_IS_RX_BUF_EMPTY)MDrv_CEC_IsRxBufEmpty;    psCECInstPri->fpCECSetActiveLogicalAddress = (IOCTL_CEC_SET_ACTIVE_LOGICAL_ADDRESS)MDrv_CEC_SetActiveLogicalAddress;    psCECInstPri->fpCECGetActiveLogicalAddress = (IOCTL_CEC_GET_ACTIVE_LOGICAL_ADDRESS)MDrv_CEC_GetActiveLogicalAddress;    psCECInstPri->fpCECGetPowerStatus = (IOCTL_CEC_GET_POWER_STATUS)MDrv_CEC_GetPowerStatus;    psCECInstPri->fpCECGetFifoIdx = (IOCTL_CEC_GET_FIFO_IDX)MDrv_CEC_GetFifoIdx;    psCECInstPri->fpCECSetFifoIdx = (IOCTL_CEC_SET_FIFO_IDX)MDrv_CEC_SetFifoIdx;    psCECInstPri->fpCECSetActivePowerStatus = (IOCTL_CEC_SET_ACTIVE_POWER_STATUS)MDrv_CEC_SetActivePowerStatus;    psCECInstPri->fpCECGetActivePowerStatus = (IOCTL_CEC_GET_ACTIVE_POWER_STATUS)MDrv_CEC_GetActivePowerStatus;    psCECInstPri->fpCECSetActivePhysicalAddress = (IOCTL_CEC_SET_ACTIVE_PHYSICAL_ADDRESS)MDrv_CEC_SetActivePhysicalAddress;    psCECInstPri->fpCECSetActiveDeviceCECVersion = (IOCTL_CEC_SET_ACTIVE_DEVICE_CEC_VERSION)MDrv_CEC_SetActiveDeviceCECVersion;    psCECInstPri->fpCECSetActiveDeviceType = (IOCTL_CEC_SET_ACTIVE_DEVICE_TYPE)MDrv_CEC_SetActiveDeviceType;    psCECInstPri->fpCECGetMsgCnt = (IOCTL_CEC_GET_MSG_CNT)MDrv_CEC_GetMsgCnt;    psCECInstPri->fpCECSetMsgCnt = (IOCTL_CEC_SET_MSG_CNT)MDrv_CEC_SetMsgCnt;    psCECInstPri->fpCECGetRxData = (IOCTL_CEC_GET_RX_DATA)MDrv_CEC_GetRxData;    psCECInstPri->fpCECSetMyPhysicalAddress = (IOCTL_CEC_SET_MY_PHYSICAL_ADDRESS)MDrv_CEC_SetMyPhysicalAddress;    psCECInstPri->fpCECConfigWakeUpInfoVendorID = (IOCTL_CEC_CONFIG_WAKEUP_INFO_VENDOR_ID)MDrv_CEC_ConfigWakeupInfoVendorID;    psCECInstPri->fpCECSetRetryCount = (IOCTL_CEC_SET_RETRY_COUNT)MDrv_CEC_SetRetryCount;    psCECInstPri->fpCECAttachDriverISR = (IOCTL_CEC_ATTACH_DRIVER_ISR)MDrv_CEC_AttachDriverISR;    psCECInstPri->fpCECGetConfiguration = (IOCTL_CEC_GET_CONFIGURATION)MDrv_CEC_GetConfiguration;    psCECInstPri->fpCECSetChipType = (IOCTL_CEC_SET_CHIP_TYPE)MDrv_CEC_SetChipType;    return UTOPIA_STATUS_SUCCESS;}//**************************************************************************//  [Function Name]://                  CECIoctl()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************MS_U32 CECIoctl(void* pInstance, MS_U32 u32Cmd, void* pArgs){    void* pModule = NULL;    MS_U32 ulReturnValue = UTOPIA_STATUS_SUCCESS;    CEC_INSTANT_PRIVATE* psCECInstPri = NULL;    UtopiaInstanceGetModule(pInstance, &pModule);    UtopiaInstanceGetPrivate(pInstance, (void**)&psCECInstPri);    switch(u32Cmd)    {        case MAPI_CMD_DDC2BI_GET_INFO:            {                pstCEC_DDC2BI_GET_INFO pCECArgs = (pstCEC_DDC2BI_GET_INFO)pArgs;                pCECArgs->bGetInfo = psCECInstPri->fpDDC2BIGetInfo(pInstance, pCECArgs->eInfo);            }            break;        case MAPI_CMD_CEC_INIT_CHIP:            {                pstCEC_INIT_CHIP pCECArgs = (pstCEC_INIT_CHIP)pArgs;                psCECInstPri->fpCECInitChip(pInstance, pCECArgs->u32XTAL_CLK_Hz);            }            break;        case MAPI_CMD_CEC_PORT_SELECT:            {                pstCEC_PORT_SELECT pCECArgs = (pstCEC_PORT_SELECT)pArgs;                psCECInstPri->fpCECPortSelect(pInstance, pCECArgs->InputPort);            }            break;        case MAPI_CMD_CEC_EXIT:            {                psCECInstPri->fpCECExit(pInstance);            }            break;        case MAPI_CMD_CEC_SET_MY_LOGICAL_ADDRESS:            {                pstCEC_SET_MY_LOGICAL_ADDRESS pCECArgs = (pstCEC_SET_MY_LOGICAL_ADDRESS)pArgs;                psCECInstPri->fpCECSetMyLogicalAddress(pInstance, pCECArgs->myLA);            }            break;        case MAPI_CMD_CEC_INIT:            {                pstCEC_INIT pCECArgs = (pstCEC_INIT)pArgs;                psCECInstPri->fpCECInit(pInstance, pCECArgs->u32XTAL_CLK_Hz);            }            break;        case MAPI_CMD_CEC_CHECK_EXIST_DEVICES:            {                psCECInstPri->fpCECCheckExistDevices(pInstance);            }            break;        case MAPI_CMD_CEC_NEXT_DEVICE:            {                pstCEC_NEXT_DEVICE pCECArgs = (pstCEC_NEXT_DEVICE)pArgs;                pCECArgs->eNextDeviceLA = psCECInstPri->fpCECNextDevice(pInstance);            }            break;        case MAPI_CMD_CEC_CHK_RX_BUF:            {                psCECInstPri->fpCECChkRxBuf(pInstance);            }            break;        case MAPI_CMD_CEC_TX_SEND_MSG:            {                pstCEC_TX_SEND_MSG pCECArgs = (pstCEC_TX_SEND_MSG)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECTxSendMsg(pInstance, pCECArgs->eDetAddr, pCECArgs->eMsg, pCECArgs->ucCmd, pCECArgs->ucLen);            }            break;        case MAPI_CMD_CEC_TX_SEND_MSG2:            {                pstCEC_TX_SEND_MSG2 pCECArgs = (pstCEC_TX_SEND_MSG2)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECTxSendMsg2(pInstance, pCECArgs->eDetAddr, pCECArgs->eMsg, pCECArgs->ucCmd, pCECArgs->ucLen);            }            break;        case MAPI_CMD_CEC_TX_SEND_POLLING_MSG:            {                pstCEC_TX_SEND_POLLING_MSG pCECArgs = (pstCEC_TX_SEND_POLLING_MSG)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECTxSendPollingMsg(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_ACTIVE_SOURCE:            {                pstCEC_MSG_ACTIVE_SOURCE pCECArgs = (pstCEC_MSG_ACTIVE_SOURCE)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgActiveSource(pInstance);            }            break;        case MAPI_CMD_CEC_MSG_ROUTING_CHANGE:            {                pstCEC_MSG_ROUTING_CHANGE pCECArgs = (pstCEC_MSG_ROUTING_CHANGE)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgRoutingChange(pInstance, pCECArgs->ucOrigAddr, pCECArgs->ucNewAddr);            }            break;        case MAPI_CMD_CEC_MSG_REQ_ACTIVE_SOURCE:            {                pstCEC_MSG_REQ_ACTIVE_SOURCE pCECArgs = (pstCEC_MSG_REQ_ACTIVE_SOURCE)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqActiveSource(pInstance);            }            break;        case MAPI_CMD_CEC_MSG_SET_STREAM_PATH:            {                pstCEC_MSG_SET_STREAM_PATH pCECArgs = (pstCEC_MSG_SET_STREAM_PATH)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgSetStreamPath(pInstance, pCECArgs->ucNewAddr);            }            break;        case MAPI_CMD_CEC_MSG_STANDBY:            {                pstCEC_MSG_STANDBY pCECArgs = (pstCEC_MSG_STANDBY)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgStandby(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_RECORD_OFF:            {                pstCEC_MSG_RECORD_OFF pCECArgs = (pstCEC_MSG_RECORD_OFF)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgRecordOff(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_RECORD_ON:            {                pstCEC_MSG_RECORD_ON pCECArgs = (pstCEC_MSG_RECORD_ON)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgRecordOn(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_REPORT_CEC_VERSION:            {                pstCEC_MSG_REPORT_CEC_VERSION pCECArgs = (pstCEC_MSG_REPORT_CEC_VERSION)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReportCECVersion(pInstance, pCECArgs->eDetAddr, pCECArgs->ucVersion);            }            break;        case MAPI_CMD_CEC_MSG_REQ_CEC_VERSION:            {                pstCEC_MSG_REQ_CEC_VERSION pCECArgs = (pstCEC_MSG_REQ_CEC_VERSION)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqCECVersion(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_REPORT_PHYCAL_ADDRESS:            {                pstCEC_MSG_REPORT_PHYCAL_ADDRESS pCECArgs = (pstCEC_MSG_REPORT_PHYCAL_ADDRESS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReportPhycalAddress(pInstance);            }            break;        case MAPI_CMD_CEC_MSG_REQ_PHYCAL_ADDRESS:            {                pstCEC_MSG_REQ_PHYCAL_ADDRESS pCECArgs = (pstCEC_MSG_REQ_PHYCAL_ADDRESS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqPhycalAddress(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_DECK_CONTROL:            {                pstCEC_MSG_DECK_CONTROL pCECArgs = (pstCEC_MSG_DECK_CONTROL)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgDeckControl(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_DEC_STATUS:            {                pstCEC_MSG_DEC_STATUS pCECArgs = (pstCEC_MSG_DEC_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgDecStatus(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_GIVE_DECK_STATUS:            {                pstCEC_MSG_GIVE_DECK_STATUS pCECArgs = (pstCEC_MSG_GIVE_DECK_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgGiveDeckStatus(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_DC_PLAY:            {                pstCEC_MSG_DC_PLAY pCECArgs = (pstCEC_MSG_DC_PLAY)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgDCPlay(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_REQ_MENU_STATUS:            {                pstCEC_MSG_REQ_MENU_STATUS pCECArgs = (pstCEC_MSG_REQ_MENU_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqMenuStatus(pInstance, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_USER_CTRL_PRESSED:            {                pstCEC_MSG_USER_CTRL_PRESSED pCECArgs = (pstCEC_MSG_USER_CTRL_PRESSED)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgUserCtrlPressed(pInstance, pCECArgs->bUserCtrlEn, pCECArgs->eDetAddr, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_USER_CTRL_RELEASED:            {                pstCEC_MSG_USER_CTRL_RELEASED pCECArgs = (pstCEC_MSG_USER_CTRL_RELEASED)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgUserCtrlReleased(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_GIVE_AUDIO_STATUS:            {                pstCEC_MSG_GIVE_AUDIO_STATUS pCECArgs = (pstCEC_MSG_GIVE_AUDIO_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgGiveAudioStatus(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_REPORT_POWER_STATUS:            {                pstCEC_MSG_REPORT_POWER_STATUS pCECArgs = (pstCEC_MSG_REPORT_POWER_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReportPowerStatus(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_REQ_POWER_STATUS:            {                pstCEC_MSG_REQ_POWER_STATUS pCECArgs = (pstCEC_MSG_REQ_POWER_STATUS)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqPowerStatus(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_FEATURE_ABORT:            {                pstCEC_MSG_FEATURE_ABORT pCECArgs = (pstCEC_MSG_FEATURE_ABORT)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgFeatureAbort(pInstance, pCECArgs->eDetAddr, pCECArgs->eMsg, pCECArgs->eCmd);            }            break;        case MAPI_CMD_CEC_MSG_ABORT:            {                pstCEC_MSG_ABORT pCECArgs = (pstCEC_MSG_ABORT)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgAbort(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_SEND_MENU_LANGUAGE:            {                pstCEC_MSG_SEND_MENU_LANGUAGE pCECArgs = (pstCEC_MSG_SEND_MENU_LANGUAGE)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgSendMenuLanguage(pInstance, pCECArgs->pu8MenulanguageCode);            }            break;        case MAPI_CMD_CEC_MSG_REQ_ARC_INITIATION:            {                pstCEC_MSG_REQ_ARC_INITIATION pCECArgs = (pstCEC_MSG_REQ_ARC_INITIATION)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqARCInitiation(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_REQ_ARC_TERMINATION:            {                pstCEC_MSG_REQ_ARC_TERMINATION pCECArgs = (pstCEC_MSG_REQ_ARC_TERMINATION)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReqARCTermination(pInstance, pCECArgs->eDetAddr);            }            break;        case MAPI_CMD_CEC_MSG_AUDIO_MODE_REQ:            {                pstCEC_MSG_AUDIO_MODE_REQ pCECArgs = (pstCEC_MSG_AUDIO_MODE_REQ)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgAudioModeReq(pInstance, pCECArgs->eDetAddr, pCECArgs->bAmpSwitch, pCECArgs->ucMyAddress);            }            break;        case MAPI_CMD_CEC_MSG_REPORT_PHYCAL_ADDRESS2:            {                pstCEC_MSG_REPORT_PHYCAL_ADDRESS2 pCECArgs = (pstCEC_MSG_REPORT_PHYCAL_ADDRESS2)pArgs;                pCECArgs->eErrorCode = psCECInstPri->fpCECMsgReportPhycalAddress2(pInstance);            }            break;        case MAPI_CMD_CEC_CHECK_FRAME:            {                pstCEC_CHECK_FRAME pCECArgs = (pstCEC_CHECK_FRAME)pArgs;                pCECArgs->bValid = psCECInstPri->fpCECCheckFrame(pInstance, pCECArgs->eMsgType, pCECArgs->ucLen);            }            break;        case MAPI_CMD_CEC_CONFIG_WAKEUP:            {                psCECInstPri->fpCECConfigWakeUp(pInstance);            }            break;        case MAPI_CMD_CEC_ENABLED:            {                pstCEC_ENABLED pCECArgs = (pstCEC_ENABLED)pArgs;                psCECInstPri->fpCECEnabled(pInstance, pCECArgs->bEnableFlag);            }            break;        case MAPI_CMD_CEC_GET_TX_STATUS:            {                pstCEC_GET_TX_STATUS pCECArgs = (pstCEC_GET_TX_STATUS)pArgs;                pCECArgs->ucTxStatus = psCECInstPri->fpCECGetTxStatus(pInstance);            }            break;        case MAPI_CMD_CEC_CHECK_DEVICE_IS_TX:            {                pstCEC_DEVICE_IS_TX pCECArgs = (pstCEC_DEVICE_IS_TX)pArgs;                pCECArgs->bIsTx = psCECInstPri->fpCECDeviceIsTx(pInstance);            }            break;        case MAPI_CMD_CEC_SET_POWER_STATE:            {                pstCEC_SET_POWER_STATE pCECArgs = (pstCEC_SET_POWER_STATE)pArgs;                pCECArgs->u32Status = psCECInstPri->fpCECSetPowerState(pInstance, pCECArgs->ePowerState);            }            break;        case MAPI_CMD_CEC_SET_MY_LOGICAL_ADDRESS2:            {                pstCEC_SET_MY_LOGICAL_ADDRESS2 pCECArgs = (pstCEC_SET_MY_LOGICAL_ADDRESS2)pArgs;                psCECInstPri->fpCECSetMyLogicalAddress2(pInstance, pCECArgs->myLA);            }            break;        case MAPI_CMD_CEC_GET_HEADER:            {                pstCEC_GET_HEADER pCECArgs = (pstCEC_GET_HEADER)pArgs;                pCECArgs->ucValue = psCECInstPri->fpCECGetHeader(pInstance);            }            break;        case MAPI_CMD_CEC_GET_OPCODE:            {                pstCEC_GET_OPCODE pCECArgs = (pstCEC_GET_OPCODE)pArgs;                pCECArgs->ucValue = psCECInstPri->fpCECGetOpCode(pInstance);            }            break;        case MAPI_CMD_CEC_GET_PARA:            {                pstCEC_GET_PARA pCECArgs = (pstCEC_GET_PARA)pArgs;                pCECArgs->ucValue = psCECInstPri->fpCECGetPara(pInstance, pCECArgs->ucIdx);            }            break;        case MAPI_CMD_CEC_GET_CMD_LEN:            {                pstCEC_GET_CMD_LEN pCECArgs = (pstCEC_GET_CMD_LEN)pArgs;                pCECArgs->ucValue = psCECInstPri->fpCECGetCmdLen(pInstance);            }            break;        case MAPI_CMD_CEC_IS_RX_BUF_EMPTY:            {                pstCEC_IS_RX_BUF_EMPTY pCECArgs = (pstCEC_IS_RX_BUF_EMPTY)pArgs;                pCECArgs->bEmpty = psCECInstPri->fpCECIsRxBufEmpty(pInstance);            }            break;        case MAPI_CMD_CEC_SET_ACTIVE_LOGICAL_ADDRESS:            {                pstCEC_SET_ACTIVE_LOGICAL_ADDRESS pCECArgs = (pstCEC_SET_ACTIVE_LOGICAL_ADDRESS)pArgs;                psCECInstPri->fpCECSetActiveLogicalAddress(pInstance, pCECArgs->eAddr);            }            break;        case MAPI_CMD_CEC_GET_ACTIVE_LOGICAL_ADDRESS:            {                pstCEC_GET_ACTIVE_LOGICAL_ADDRESS pCECArgs = (pstCEC_GET_ACTIVE_LOGICAL_ADDRESS)pArgs;                pCECArgs->eAddr = psCECInstPri->fpCECGetActiveLogicalAddress(pInstance);            }            break;        case MAPI_CMD_CEC_GET_POWER_STATUS:            {                pstCEC_GET_POWER_STATUS pCECArgs = (pstCEC_GET_POWER_STATUS)pArgs;                pCECArgs->ePowerStatus = psCECInstPri->fpCECGetPowerStatus(pInstance);            }            break;        case MAPI_CMD_CEC_GET_FIFO_IDX:            {                pstCEC_GET_FIFO_IDX pCECArgs = (pstCEC_GET_FIFO_IDX)pArgs;                pCECArgs->ucFifoIdx = psCECInstPri->fpCECGetFifoIdx(pInstance);            }            break;        case MAPI_CMD_CEC_SET_FIFO_IDX:            {                pstCEC_SET_FIFO_IDX pCECArgs = (pstCEC_SET_FIFO_IDX)pArgs;                psCECInstPri->fpCECSetFifoIdx(pInstance, pCECArgs->ucIdx);            }            break;        case MAPI_CMD_CEC_SET_ACTIVE_POWER_STATUS:            {                pstCEC_SET_ACTIVE_POWER_STATUS pCECArgs = (pstCEC_SET_ACTIVE_POWER_STATUS)pArgs;                psCECInstPri->fpCECSetActivePowerStatus(pInstance, pCECArgs->eStatus);            }            break;        case MAPI_CMD_CEC_GET_ACTIVE_POWER_STATUS:            {                pstCEC_GET_ACTIVE_POWER_STATUS pCECArgs = (pstCEC_GET_ACTIVE_POWER_STATUS)pArgs;                pCECArgs->ePowerStatus = psCECInstPri->fpCECGetActivePowerStatus(pInstance);            }            break;        case MAPI_CMD_CEC_SET_ACTIVE_PHYSICAL_ADDRESS:            {                pstCEC_SET_ACTIVE_PHYSICAL_ADDRESS pCECArgs = (pstCEC_SET_ACTIVE_PHYSICAL_ADDRESS)pArgs;                psCECInstPri->fpCECSetActivePhysicalAddress(pInstance, pCECArgs->ucPara1, pCECArgs->ucPara2);            }            break;        case MAPI_CMD_CEC_SET_ACTIVE_DEVICE_CEC_VERSION:            {                pstCEC_SET_ACTIVE_DEVICE_CEC_VERSION pCECArgs = (pstCEC_SET_ACTIVE_DEVICE_CEC_VERSION)pArgs;                psCECInstPri->fpCECSetActiveDeviceCECVersion(pInstance, pCECArgs->ucVer);            }            break;        case MAPI_CMD_CEC_SET_ACTIVE_DEVICE_TYPE:            {                pstCEC_SET_ACTIVE_DEVICE_TYPE pCECArgs = (pstCEC_SET_ACTIVE_DEVICE_TYPE)pArgs;                psCECInstPri->fpCECSetActiveDeviceType(pInstance, pCECArgs->eType);            }            break;        case MAPI_CMD_CEC_GET_MSG_CNT:            {                pstCEC_GET_MSG_CNT pCECArgs = (pstCEC_GET_MSG_CNT)pArgs;                pCECArgs->ucMsgCnt = psCECInstPri->fpCECGetMsgCnt(pInstance);            }            break;        case MAPI_CMD_CEC_SET_MSG_CNT:            {                pstCEC_SET_MSG_CNT pCECArgs = (pstCEC_SET_MSG_CNT)pArgs;                psCECInstPri->fpCECSetMsgCnt(pInstance, pCECArgs->ucCnt);            }            break;        case MAPI_CMD_CEC_GET_RX_DATA:            {                pstCEC_GET_RX_DATA pCECArgs = (pstCEC_GET_RX_DATA)pArgs;                pCECArgs->ucRxData = psCECInstPri->fpCECGetRxData(pInstance, pCECArgs->ucFifoIdx, pCECArgs->ucIdx);            }            break;        case MAPI_CMD_CEC_SET_MY_PHYSICAL_ADDRESS:            {                pstCEC_SET_MY_PHYSICAL_ADDRESS pCECArgs = (pstCEC_SET_MY_PHYSICAL_ADDRESS)pArgs;                psCECInstPri->fpCECSetMyPhysicalAddress(pInstance, pCECArgs->ucData);            }            break;        case MAPI_CMD_CEC_SET_MY_PHYSICAL_ADDRESS2:            {                pstCEC_SET_MY_PHYSICAL_ADDRESS2 pCECArgs = (pstCEC_SET_MY_PHYSICAL_ADDRESS2)pArgs;                psCECInstPri->fpCECSetMyPhysicalAddress2(pInstance, pCECArgs->ucData);            }            break;        case MAPI_CMD_CEC_SET_INITIATOR:            {                pstCEC_SET_INITIATOR pCECArgs = (pstCEC_SET_INITIATOR)pArgs;                psCECInstPri->fpCECSetInitiator(pInstance, pCECArgs->eIniLa);            }            break;        case MAPI_CMD_CEC_CONFIG_WAKEUP_INFO_VENDOR_ID:            {                pstCEC_CONFIG_WAKEUP_INFO_VENDOR_ID pCECArgs = (pstCEC_CONFIG_WAKEUP_INFO_VENDOR_ID)pArgs;                psCECInstPri->fpCECConfigWakeUpInfoVendorID(pInstance, pCECArgs->ucVendorID);            }            break;        case MAPI_CMD_CEC_SET_RETRY_COUNT:            {                pstCEC_SET_RETRY_COUNT pCECArgs = (pstCEC_SET_RETRY_COUNT)pArgs;                psCECInstPri->fpCECSetRetryCount(pInstance, pCECArgs->ucRetryCount);            }            break;        case MAPI_CMD_CEC_ATTACH_DRIVER_ISR:            {                 pstCEC_ATTACH_DRIVER_ISR pCECArgs = (pstCEC_ATTACH_DRIVER_ISR)pArgs;                 psCECInstPri->fpCECAttachDriverISR(pInstance, pCECArgs->bAttachDrvFlag);            }            break;        case MAPI_CMD_CEC_GET_CONFIGURATION:            {                pstCEC_GET_CONFIGURATION pCECArgs = (pstCEC_GET_CONFIGURATION)pArgs;                pCECArgs->stInitialConfigInfo = psCECInstPri->fpCECGetConfiguration(pInstance);            }            break;        case MAPI_CMD_CEC_SET_CHIP_TYPE:            {                pstCEC_SET_CHIPTYPE pCECArgs = (pstCEC_SET_CHIPTYPE)pArgs;                psCECInstPri->fpCECSetChipType(pInstance, pCECArgs->eChipType);            }            break;        default:            ulReturnValue = UTOPIA_STATUS_FAIL;            break;    };    return ulReturnValue;}//**************************************************************************//  [Function Name]://                  CECClose()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************MS_U32 CECClose(void* pInstance){    UtopiaInstanceDelete(pInstance);    return TRUE;}//**************************************************************************//  [Function Name]://                  CECSTR()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************MS_U32 CECSTR(MS_U32 ulPowerState, void* pModule){    MS_U32 ulReturnValue = UTOPIA_STATUS_FAIL;    ulReturnValue = mdrv_CEC_STREventProc(pModule, (EN_POWER_MODE)ulPowerState);    return ulReturnValue;}#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT//**************************************************************************//  [Function Name]://                  CECMdbIoctl()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************MS_U32 CECMdbIoctl(MS_U32 cmd, const void* const pArgs){    void* pInstance = NULL;    MS_U32 ulReturnValue = UTOPIA_STATUS_SUCCESS;    MDBCMD_CMDLINE_PARAMETER *paraCmdLine;    MDBCMD_GETINFO_PARAMETER *paraGetInfo;    pInstance = UtopiaModuleGetLocalInstantList(MODULE_CEC, pInstance);    //paraCmdLine = (MDBCMD_CMDLINE_PARAMETER *)pArgs;    //MdbPrint(paraCmdLine->u64ReqHdl,"[CECMdbIoctl]pInstance= %d\n",pInstance);    switch(cmd)    {        case MDBCMD_CMDLINE:            paraCmdLine = (MDBCMD_CMDLINE_PARAMETER *)pArgs;            //MdbPrint(paraCmdLine->u64ReqHdl,"u32CmdSize: %d\n", paraCmdLine->u32CmdSize);            mdrv_CEC_MDCMDEchoCommand(pInstance, paraCmdLine->u64ReqHdl, paraCmdLine->pcCmdLine);            paraCmdLine->result = MDBRESULT_SUCCESS_FIN;            break;        case MDBCMD_GETINFO:            paraGetInfo = (MDBCMD_GETINFO_PARAMETER *)pArgs;            mdrv_CEC_MDCMDGetInfo(pInstance, paraGetInfo->u64ReqHdl);            paraGetInfo->result = MDBRESULT_SUCCESS_FIN;            break;        default:            //paraGetInfo = (MDBCMD_GETINFO_PARAMETER *)pArgs;            //MdbPrint(paraGetInfo->u64ReqHdl,"unknown cmd\n", __LINE__);            break;    };    return ulReturnValue;}#endif//**************************************************************************//  [Function Name]://                  CECRegisterToUtopia()//  [Description]////  [Arguments]:////  [Return]:////**************************************************************************void CECRegisterToUtopia(FUtopiaOpen ModuleType){    void* pUtopiaModule = NULL;    void* psResource = NULL;    // 1. deal with module    UtopiaModuleCreate(MODULE_CEC, 0, &pUtopiaModule);    UtopiaModuleRegister(pUtopiaModule);    // register func for module, after register here, then ap call UtopiaOpen/UtopiaIoctl/UtopiaClose can call to these registered standard func    UtopiaModuleSetupFunctionPtr(pUtopiaModule, (FUtopiaOpen)CECOpen, (FUtopiaClose)CECClose, (FUtopiaIOctl)CECIoctl);#ifdef CONFIG_UTOPIA_PROC_DBG_SUPPORT    UtopiaModuleRegisterMdbNode("cec", (FUtopiaMdbIoctl)CECMdbIoctl);#endif#if(defined(MSOS_TYPE_LINUX_KERNEL))    UtopiaModuleSetupSTRFunctionPtr(pUtopiaModule, (FUtopiaSTR)CECSTR);#endif    // 2. deal with resource    // start func to add res, call once will create 2 access in resource. Also can declare BDMA_POOL_ID_BDMA1 for another channel depend on driver owner.    UtopiaModuleAddResourceStart(pUtopiaModule, CEC_POOL);    // resource can alloc private for internal use, ex, BDMA_RESOURCE_PRIVATE    UtopiaResourceCreate("CEC", sizeof(CEC_RESOURCE_PRIVATE), &psResource);    // func to reg res    UtopiaResourceRegister(pUtopiaModule, psResource, CEC_POOL);    UtopiaModuleAddResourceEnd(pUtopiaModule, CEC_POOL);}#endif // _API_CEC_V2_C_