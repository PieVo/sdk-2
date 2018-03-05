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
#define HAL_VIP_C


//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
// Common Definition
#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_utility.h"
// Internal Definition
#include "hal_scl_reg.h"
#include "drv_scl_vip.h"
#include "hal_scl_util.h"
#include "hal_scl_vip.h"
#include "drv_scl_irq_st.h"
#include "drv_scl_irq.h"
#include "drv_scl_pq_define.h"
#include "Infinity_Iq.h"             // table config parameter
//-------------------------------------------------------------------------------------------------
//  Driver Compiler Options
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define HAL_SCLVIP_DBG(x)
#define HAL_SCLVIP_MUTEX_LOCK()            DrvSclOsObtainMutex(_HalVIP_Mutex,SCLOS_WAIT_FOREVER)
#define HAL_SCLVIP_MUTEX_UNLOCK()          DrvSclOsReleaseMutex(_HalVIP_Mutex)
#define WDR_SRAM_NUM 8
#define WDR_SRAM_BYTENUM 0
#define WDR_SRAM_USERBYTENUM (81)
#define GAMMAY_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblY_SRAM_SIZE_Main/4)
#define GAMMAU_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblU_SRAM_SIZE_Main/4)
#define GAMMAV_SRAM_BYTENUM (PQ_IP_YUV_Gamma_tblV_SRAM_SIZE_Main/4)
#define GAMMA10to12R_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_SIZE_Main/4)
#define GAMMA10to12G_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_SIZE_Main/4)
#define GAMMA10to12B_SRAM_BYTENUM (PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_SIZE_Main/4)
#define GAMMA12to10R_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_SIZE_Main/4)
#define GAMMA12to10G_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_SIZE_Main/4)
#define GAMMA12to10B_SRAM_BYTENUM (PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_SIZE_Main/4)
#define NRBUFFERCNT 8
//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
//keep
u32 VIP_RIU_BASE = 0;
bool gbCMDQ = 0;
s32 _HalVIP_Mutex = -1;

//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
void HalSclVipExit(void)
{
    if(_HalVIP_Mutex != -1)
    {
        DrvSclOsDeleteMutex(_HalVIP_Mutex);
        _HalVIP_Mutex = -1;
    }
}

void HalSclVipSetRiuBase(u32 u32riubase)
{
    char word[] = {"_HalVIP_Mutex"};
    VIP_RIU_BASE = u32riubase;
    _HalVIP_Mutex = DrvSclOsCreateMutex(E_DRV_SCLOS_FIFO, word, SCLOS_PROCESS_SHARED);

    if (_HalVIP_Mutex == -1)
    {
        SCL_ERR("[DRVHVSP]%s(%d): create mutex fail\n", __FUNCTION__, __LINE__);
    }
}
void HalSclVipSeMcnrIPMRead(bool bEn)
{
}
void _HalSclVipSramDumpIhcIcc(HalSclVipSramDumpType_e endump,u32 u32reg)
{
    u8  u8sec = 0;
    HalSclVipSramSecNum_e enSecNum;
    u16 u16addr = 0,u16tvalue = 0,u16tcount = 0,u16readdata;
    HalUtilityW2BYTEMSK(u32reg, BIT0, BIT0);//IOenable
    for(u8sec=0;u8sec<4;u8sec++)
    {
        switch(u8sec)
        {
            case 0:
                enSecNum = E_HAL_SCLVIP_SRAM_SEC_0;
                break;
            case 1:
                enSecNum = E_HAL_SCLVIP_SRAM_SEC_1;
                break;
            case 2:
                enSecNum = E_HAL_SCLVIP_SRAM_SEC_2;
                break;
            case 3:
                enSecNum = E_HAL_SCLVIP_SRAM_SEC_3;
                break;
            default:
                break;

        }
        HalUtilityW2BYTEMSK(u32reg, u8sec<<1, BIT1|BIT2);//sec
        for(u16addr=0;u16addr<enSecNum;u16addr++)
        {
            if(endump == E_HAL_SCLVIP_SRAM_DUMP_IHC)
            {
                u16tvalue = MST_VIP_IHC_CRD_SRAM_Main[0][u16tcount] | (MST_VIP_IHC_CRD_SRAM_Main[0][u16tcount+1]<<8);
            }
            else
            {
                u16tvalue = MST_VIP_ICC_CRD_SRAM_Main[0][u16tcount] | (MST_VIP_ICC_CRD_SRAM_Main[0][u16tcount+1]<<8);
            }
            HalUtilityW2BYTEMSK(u32reg+2, u16addr, 0x01FF);//addr
            HalUtilityW2BYTEMSK(u32reg+4, (u16)u16tvalue, 0x3FF);//data
            HalUtilityW2BYTEMSK(u32reg+4, BIT15, BIT15);//wen
            //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
            {
                HalUtilityW2BYTEMSK(u32reg+2, BIT15, BIT15);//ren
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[ICCIHC]tval:%hx\n", u16tvalue);
                u16readdata=HalUtilityR2BYTEDirect(u32reg+6);
                SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[ICCIHC]reg tval:%hx\n", u16readdata);
            }
            u16tcount+=2;
        }
    }

    HalUtilityW2BYTEMSK(u32reg, 0, BIT0);//IOenable
}
u16 _HalSclVipGetWdrTvalue(u8 u8sec, u16 u16tcount, u32 u32Sram)
{
    u16 u16tvalue = 0;
    u8 *p8buffer;
    u32 u32Val = 0;
    if(u32Sram)
    {
        p8buffer= (u8 *)u32Sram;
        u32Val = WDR_SRAM_USERBYTENUM*2 *u8sec ;
        u16tvalue = (u16)(*(p8buffer+u32Val+u16tcount) | (*(p8buffer+u32Val+u16tcount+1)<<8));
        SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[WDRTvalue]val:%ld\n",u32Val+u16tcount);
    }
    else
    {
    }
    return u16tvalue;
}
void _HalSclVipSetWriteRegType(bool bEn)
{
    if(bEn && VIPSETRULE())
    {
        if(DrvSclVipGetEachDmaEn()&& !DrvSclVipGetIsBlankingRegion())
        {
            gbCMDQ = 1;
        }
        else
        {
            gbCMDQ = 0;
        }
    }
    else
    {
        gbCMDQ = 0;
    }
}
void _HalSclVipWriteReg(u32 u32Reg,u16 u16Val,u16 u16Mask)
{
    if(gbCMDQ)
    {
        //ToDo :CMDQ set Sram
    }
    else
    {
        HalUtilityW2BYTEMSK(u32Reg, u16Val, u16Mask);//sec
    }
}
bool _HalSclVipCheckMonotonicallyIncreasing
    (u16 u16tvalueeven,u16 u16tvalueodd,u16 u16chkodd)
{
    if((u16tvalueodd < u16tvalueeven)||(u16tvalueeven < u16chkodd))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
u16 _HalSclVipGetGammaYuvTvalue(u16 u16sec, u16 u16tcount,u32 u32Sram)
{
    u16 u16tvalue;
    u8 *p8buffer;
    if(u32Sram)
    {
        p8buffer= (u8 *)u32Sram;
        u16tvalue = (u16)(*(p8buffer+u16tcount) | (*(p8buffer+u16tcount+1)<<8));
    }
    else
    {
        switch(u16sec)
        {
            case 0:
                u16tvalue = MST_YUV_Gamma_tblY_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblY_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 2:
                u16tvalue = MST_YUV_Gamma_tblU_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblU_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 4:
                u16tvalue = MST_YUV_Gamma_tblV_SRAM_Main[0][u16tcount] | (MST_YUV_Gamma_tblV_SRAM_Main[0][u16tcount+1]<<8);
                break;
            default:
                u16tvalue = 0;
                break;
        }
    }
    return u16tvalue;
}

bool _HalSclVipSramDumpGammaYuv(HalSclVipSramDumpType_e endump,u32 u32Sram,u32 u32reg)
{
    u16  u16sec = 0;
    u8  bRet = 1;
    u16  u16chkodd = 0;
    u16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16RegMask,u16size;
    u32 u32rega, u32regd ;
    switch(endump)
    {
        case E_HAL_SCLVIP_SRAM_DUMP_GAMMA_Y:
            u16RegMask = 0x3;
            u32rega = u32reg+2;
            u32regd = u32reg+8;
            u16size = GAMMAY_SRAM_BYTENUM;
            u16sec = 0;
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GAMMA_U:
            u16RegMask = 0xC;
            u32rega = u32reg+4;
            u32regd = u32reg+12;
            u16size = GAMMAU_SRAM_BYTENUM;
            u16sec = 2;
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GAMMA_V:
            u16RegMask = 0x30;
            u32rega = u32reg+6;
            u32regd = u32reg+16;
            u16size = GAMMAV_SRAM_BYTENUM;
            u16sec = 4;
            break;
        default:
            u16RegMask = 0;
            u32rega = 0;
            u32regd = 0;
            u16size = 0;
            break;
    }
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "YUVGamma:%hhx\n", u16sec);
    HAL_SCLVIP_MUTEX_LOCK();
    _HalSclVipSetWriteRegType(1);
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!DrvSclVipGetIsBlankingRegion() && !gbCMDQ)
        {
            _HalSclVipSetWriteRegType(0);
            _HalSclVipSetWriteRegType(1);
            if(VIPSETRULE())
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "bCMDQ Open YUVGamma:%hhx\n",u16sec);
            }
            else
            {
                _HalSclVipSetWriteRegType(0);
                HAL_SCLVIP_MUTEX_UNLOCK();
                return 0;
            }
        }
        u16tvalueeven = _HalSclVipGetGammaYuvTvalue(u16sec, u16tcount,u32Sram);
        u16tvalueodd = _HalSclVipGetGammaYuvTvalue(u16sec, u16tcount+2,u32Sram);
        if(bRet)
        {
            bRet = _HalSclVipCheckMonotonicallyIncreasing(u16tvalueeven,u16tvalueodd,u16chkodd);
            if(!bRet)
            {
                //SCL_ERR("[HALVIP]YUVGamma:%hhx @:%hx NOT Monotonically Increasing  (%hx,%hx,%hx)\n"
                //    ,u16sec,u16addr,u16chkodd,u16tvalueeven,u16tvalueodd);
            }
        }
        u16chkodd = u16tvalueodd;
        _HalSclVipWriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
        _HalSclVipWriteReg(u32regd, (u16)u16tvalueeven, 0xFFF);//data
        _HalSclVipWriteReg(u32regd+2, (u16)u16tvalueodd, 0xFFF);//data
        _HalSclVipWriteReg(u32reg, u16RegMask, u16RegMask);//wen
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            _HalSclVipWriteReg(u32reg-2, (u16sec<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
            //_HalSclVipWriteReg(u32reg-2,BIT2 ,BIT2 );
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "even val:%hx\n", u16tvalueeven);
            u16readdata=HalUtilityR2BYTEDirect(u32reg+28);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n", u16readdata);
            if(!gbCMDQ)
            {
                while(u16readdata!=u16tvalueeven)
                {
                    SCL_DBGERR("[HALVIP]EVEN YUVGamma:%hhx\n",u16sec);
                    SCL_DBGERR( "[HALVIP]EVEN val:%hx\n",u16tvalueodd);
                    SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                    if(!DrvSclVipGetIsBlankingRegion())
                    {
                        //I3 patch
                        //DrvSclOsWaitEvent(DrvSclIrqGetIrqSYNCEventID(), E_DRV_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_DRV_SCLOS_EVENT_MD_OR, 200); // get status: FRM END
                    }
                    _HalSclVipWriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
                    _HalSclVipWriteReg(u32regd, (u16)u16tvalueeven, 0xFFF);//data
                    _HalSclVipWriteReg(u32regd+2, (u16)u16tvalueodd, 0xFFF);//data
                    _HalSclVipWriteReg(u32reg, u16RegMask, u16RegMask);//wen
                    _HalSclVipWriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx //ren bit2
                    u16readdata=HalUtilityR2BYTEDirect(u32reg+28);
                    bRet++;
                    if(bRet>10)
                    {
                        bRet = 0;
                        break;
                    }
                }
            }
            _HalSclVipWriteReg(u32reg-2, ((u16sec+1)<<3)|BIT2, BIT2|BIT3|BIT4|BIT5);//ridx
            //_HalSclVipWriteReg(u32reg-2, BIT2, BIT2);//ren
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "odd val:%hx\n", u16tvalueodd);
            u16readdata=HalUtilityR2BYTEDirect(u32reg+28);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n", u16readdata);
            if(!gbCMDQ)
            {
                while(u16readdata!=u16tvalueodd)
                {
                    SCL_DBGERR("[HALVIP]ODD YUVGamma:%hhx\n",u16sec);
                    SCL_DBGERR( "[HALVIP]odd val:%hx\n",u16tvalueodd);
                    SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                    if(!DrvSclVipGetIsBlankingRegion())
                    {
                        //I3 patch
                        //DrvSclOsWaitEvent(DrvSclIrqGetIrqSYNCEventID(), E_DRV_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_DRV_SCLOS_EVENT_MD_OR, 200); // get status: FRM END
                    }
                    _HalSclVipWriteReg(u32rega, (((u16addr)<<8)|u16addr), 0x7F7F);//addr
                    _HalSclVipWriteReg(u32regd+2, (u16)u16tvalueodd, 0xFFF);//data
                    _HalSclVipWriteReg(u32regd, (u16)u16tvalueeven, 0xFFF);//data
                    _HalSclVipWriteReg(u32reg, u16RegMask, u16RegMask);//wen
                    _HalSclVipWriteReg(u32reg-2, (((u16sec+1)<<3)|(u16sec<<3)|BIT2), BIT2|BIT3|BIT4|BIT5);//ridx
                    u16readdata=HalUtilityR2BYTEDirect(u32reg+28);
                    bRet++;
                    if(bRet>10)
                    {
                        bRet = 0;
                        break;
                    }
                }
            }
        }
        u16tcount+=4;
    }
    _HalSclVipSetWriteRegType(0);
    HAL_SCLVIP_MUTEX_UNLOCK();
    return bRet;
}
u16 Hal_VIP_GetGammaRGBTvalue(u16 u16sec, u16 u16tcount,u32 u32Sram,u8  u8type)
{
    u16 u16tvalue;
    u8 *p8buffer;
    if(u32Sram)
    {
        p8buffer= (u8 *)u32Sram;
        u16tvalue = (u16)(*(p8buffer+u16tcount) | (*(p8buffer+u16tcount+1)<<8));
    }
    else
    {
        switch(u16sec+u8type)
        {
            case 0:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_R_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_R_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 2:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_G_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_G_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 4:
                u16tvalue = MST_ColorEng_GM10to12_Tbl_B_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM10to12_Tbl_B_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 1:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_R_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_R_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 3:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_G_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_G_SRAM_Main[0][u16tcount+1]<<8);
                break;
            case 5:
                u16tvalue = MST_ColorEng_GM12to10_CrcTbl_B_SRAM_Main[0][u16tcount] | (MST_ColorEng_GM12to10_CrcTbl_B_SRAM_Main[0][u16tcount+1]<<8);
                break;
            default:
                u16tvalue = 0;
                break;
        }
    }
    return u16tvalue;
}

u8 Hal_VIP_SRAMDumpGammaRGB(HalSclVipSramDumpType_e endump,u32 u32Sram,u32 u32reg)
{
    u16  u16sec = 0;
    u8  u8type = 0;
    u8  bRet = 1;
    u16 u16addr = 0,u16tvalueodd = 0,u16tvalueeven = 0,u16tcount = 0,u16readdata,u16size;
    u16 u16chkodd = 0;
    switch(endump)
    {
        case E_HAL_SCLVIP_SRAM_DUMP_GM10to12_R:
            u16size = GAMMA10to12R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 0;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_R:%lx\n",u32reg);
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GM10to12_G:
            u16size = GAMMA10to12G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 0;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_G:%lx\n",u32reg);
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GM10to12_B:
            u16size = GAMMA10to12B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 0;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM10to12_B:%lx\n",u32reg);
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GM12to10_R:
            u16size = GAMMA12to10R_SRAM_BYTENUM;
            u16sec = 0;
            u8type = 1;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10R:%lx\n",u32reg);
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GM12to10_G:
            u16size = GAMMA12to10G_SRAM_BYTENUM;
            u16sec = 2;
            u8type = 1;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10G:%lx\n",u32reg);
            break;
        case E_HAL_SCLVIP_SRAM_DUMP_GM12to10_B:
            u16size = GAMMA12to10B_SRAM_BYTENUM;
            u16sec = 4;
            u8type = 1;
            SCL_DBG(SCL_DBG_LV_IOCTL()&EN_DBGMG_IOCTLEVEL_VIP, "GM12to10B:%lx\n",u32reg);
            break;
        default:
            u16size = 0;
            u16sec = 0;
            u8type = 0;
            break;
    }
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "RGBGamma:%hhx\n",u16sec+u8type);
    HAL_SCLVIP_MUTEX_LOCK();
    _HalSclVipSetWriteRegType(1);
    for(u16addr=0;u16addr<u16size;u16addr++)
    {
        if(!DrvSclVipGetIsBlankingRegion() && !gbCMDQ)
        {
            _HalSclVipSetWriteRegType(0);
            _HalSclVipSetWriteRegType(1);
            if(VIPSETRULE())
            {
                SCL_DBG(SCL_DBG_LV_DRVCMDQ()&EN_DBGMG_CMDQEVEL_ISR, "bCMDQ Open RGBGamma:%hhx\n",u16sec+u8type);
            }
            else
            {
                _HalSclVipSetWriteRegType(0);
                HAL_SCLVIP_MUTEX_UNLOCK();
                return 0;
            }
        }
        u16tvalueeven = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount,u32Sram,u8type);
        u16tvalueodd = Hal_VIP_GetGammaRGBTvalue(u16sec, u16tcount+2,u32Sram,u8type);
        if(bRet)
        {
            bRet = _HalSclVipCheckMonotonicallyIncreasing(u16tvalueeven,u16tvalueodd,u16chkodd);
            if(!bRet)
            {
                //SCL_ERR("[HALVIP]RGBGamma:%hhx @:%hx NOT Monotonically Increasing  (%hx,%hx,%hx)\n"
                //    ,u16sec+u8type,u16addr,u16chkodd,u16tvalueeven,u16tvalueodd);
            }
        }
        u16chkodd = u16tvalueodd;
        //_HalSclVipWriteReg(u32reg, u16addr, 0x007F);//addr
        //_HalSclVipWriteReg(u32reg, u16sec<<8, BIT8|BIT9|BIT10);//sec
        _HalSclVipWriteReg(u32reg+2, (u16)u16tvalueeven, 0xFFF);//data
        _HalSclVipWriteReg(u32reg, ((u16addr)|((u16)u16sec<<8)|BIT11), 0xFFFF);//wen
        _HalSclVipWriteReg(u32reg, ((u16addr)|((u16)u16sec<<8)|BIT12), 0xFFFF);//ren
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            //_HalSclVipWriteReg(u32reg, BIT12, BIT12);//ren
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "even val:%hx\n",u16tvalueeven);
            u16readdata=HalUtilityR2BYTEDirect(u32reg+4);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n",u16readdata);
        }

        if(!gbCMDQ)
        {
            while(u16readdata!=u16tvalueeven)
            {
                SCL_DBGERR("[HALVIP]EvenRGBGamma:%hhx\n",u16sec+u8type);
                SCL_DBGERR( "[HALVIP]even val:%hx\n",u16tvalueeven);
                SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                if(!DrvSclVipGetIsBlankingRegion())
                {
                    //I3 patch
                    //DrvSclOsWaitEvent(DrvSclIrqGetIrqSYNCEventID(), E_DRV_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_DRV_SCLOS_EVENT_MD_OR, 200); // get status: FRM END
                }
                _HalSclVipWriteReg(u32reg+2, (u16)u16tvalueeven, 0xFFF);//data
                _HalSclVipWriteReg(u32reg, ((u16addr)|((u16)u16sec<<8)|BIT11), 0xFFFF);//wen
                _HalSclVipWriteReg(u32reg, ((u16addr)|((u16)u16sec<<8)|BIT12), 0xFFFF);//ren
                u16readdata=HalUtilityR2BYTEDirect(u32reg+4);
                bRet++;
                if(bRet>10)
                {
                    bRet = 0;
                    break;
                }
            }
        }
        //Hal_VIP_WriteReg(u32reg, u16addr, 0x007F);//addr
        //Hal_VIP_WriteReg(u32reg, (u16sec+1)<<8, BIT8|BIT9|BIT10);//sec
        _HalSclVipWriteReg(u32reg+2, (u16)u16tvalueodd, 0xFFF);//data
        //Hal_VIP_WriteReg(u32reg, BIT11, BIT11);//wen
        _HalSclVipWriteReg(u32reg, ((u16addr)|(((u16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
        _HalSclVipWriteReg(u32reg, ((u16addr)|(((u16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
        //if(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG)
        {
            //_HalSclVipWriteReg(u32reg, BIT12, BIT12);//ren
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "odd val:%hx\n",u16tvalueodd);
            u16readdata=HalUtilityR2BYTEDirect(u32reg+4);
            SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "read val:%hx\n",u16readdata);
        }
        if(!gbCMDQ)
        {
            while(u16readdata!=u16tvalueodd)
            {
                SCL_DBGERR("[HALVIP]odd RGBGamma:%hhx\n",u16sec+u8type);
                SCL_DBGERR( "[HALVIP]odd val:%hx\n",u16tvalueodd);
                SCL_DBGERR( "[HALVIP]read val:%hx\n",u16readdata);
                if(!DrvSclVipGetIsBlankingRegion())
                {
                    //I3 patch
                    //DrvSclOsWaitEvent(DrvSclIrqGetIrqSYNCEventID(), E_DRV_SCLIRQ_EVENT_FRMENDSYNC, &u32Events, E_DRV_SCLOS_EVENT_MD_OR, 200); // get status: FRM END
                }
                _HalSclVipWriteReg(u32reg+2, (u16)u16tvalueodd, 0xFFF);//data
                _HalSclVipWriteReg(u32reg, ((u16addr)|(((u16)u16sec+1)<<8)|BIT11), 0xFFFF);//wen
                _HalSclVipWriteReg(u32reg, ((u16addr)|(((u16)u16sec+1)<<8)|BIT12), 0xFFFF);//ren
                u16readdata=HalUtilityR2BYTEDirect(u32reg+4);
                bRet++;
                if(bRet>10)
                {
                    bRet = 0;
                    break;
                }
            }
        }
        u16tcount+=4;
    }
    _HalSclVipSetWriteRegType(0);
    HAL_SCLVIP_MUTEX_UNLOCK();
    return bRet;
}

bool _HalSclVipSetSramDump(HalSclVipSramDumpType_e endump,u32 u32Sram,u32 u32reg)
{
    if(endump == E_HAL_SCLVIP_SRAM_DUMP_IHC || endump == E_HAL_SCLVIP_SRAM_DUMP_ICC)
    {
        _HalSclVipSramDumpIhcIcc(endump,u32reg);
    }
    else if(endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_Y || endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_U||
        endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_V)
    {
        return _HalSclVipSramDumpGammaYuv(endump,u32Sram,u32reg);
    }
    else
    {
        return Hal_VIP_SRAMDumpGammaRGB(endump,u32Sram,u32reg);
    }
    return 1;
}
bool HalSclVipSetWdrTbl(HalSclVipWdrTblType_e enWdrType,void *pTbl)
{
    bool bRet = 1;
    u16 u16idx;
    u16 u16cnt;
    u16 u16Tvalue;
    u16 *p16Tbllocal;
    u32 u32reg = enWdrType == E_HAL_SCLVIP_WDR_TBL_NL ? REG_VIP_WDR_40_L :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_SAT ?  REG_VIP_WDR_51_L :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_FMAP ? REG_VIP_WDR1_40_L:
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_ALPHA? REG_VIP_WDR1_51_L:
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_L ? REG_VIP_WDR1_62_L:
                        0;
    if(pTbl==NULL)
    {
        p16Tbllocal = enWdrType == E_HAL_SCLVIP_WDR_TBL_NL ? MST_ColorEng_WDR_nonlinear_c_SRAM_Main :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_SAT ?  MST_ColorEng_WDR_sat_c_SRAM_Main :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_FMAP ? MST_ColorEng_WDR_f_c_SRAM_Main :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_ALPHA? MST_ColorEng_WDR_alpha_c_SRAM_Main :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_L ? MST_ColorEng_WDR_linear_c_SRAM_Main:
                        pTbl;
    }
    else
    {
        p16Tbllocal = enWdrType == E_HAL_SCLVIP_WDR_TBL_NL ? pTbl :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_SAT ?  pTbl+SCLVIP_WDR_TBL_NL_SIZE :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_FMAP ? pTbl+SCLVIP_WDR_TBL_NL_SIZE+SCLVIP_WDR_TBL_SAT_SIZE :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_ALPHA? pTbl+SCLVIP_WDR_TBL_NL_SIZE+SCLVIP_WDR_TBL_SAT_SIZE
                        +SCLVIP_WDR_TBL_FMAP_SIZE :
                    enWdrType == E_HAL_SCLVIP_WDR_TBL_L ? pTbl+SCLVIP_WDR_TBL_NL_SIZE+SCLVIP_WDR_TBL_SAT_SIZE
                        +SCLVIP_WDR_TBL_FMAP_SIZE +SCLVIP_WDR_TBL_ALPHA_SIZE:
                        pTbl;
    }
    if(enWdrType== E_HAL_SCLVIP_WDR_TBL_SAT)
    {
        for(u16idx = 0;u16idx<SCLVIP_WDR_TBL_T;u16idx++)
        {
            if(u16idx<(SCLVIP_WDR_TBL_T-1))
            {
                for(u16cnt = 0;u16cnt<(SCLVIP_WDR_TBL_CNT/2);u16cnt++)
                {
                    u16Tvalue = *(p16Tbllocal+u16cnt+(u16idx*(SCLVIP_WDR_TBL_CNT/2)));
                    HalUtilityW2BYTETbl(u32reg+2+u16cnt*2,u16Tvalue,0xFFFF);
                }
            }
            else
            {
                u16cnt = 0;
                u16Tvalue = (u8)*(p16Tbllocal+u16cnt+(u16idx*(SCLVIP_WDR_TBL_CNT/2)));
                HalUtilityW2BYTETbl(u32reg+2+u16cnt*2,u16Tvalue,0x00FF);
            }
            HalUtilityW2BYTETbl(u32reg,(0x1100+u16idx),0xFFFF);
        }
    }
    else
    {
        for(u16idx = 0;u16idx<SCLVIP_WDR_TBL_T;u16idx++)
        {
            if(u16idx<(SCLVIP_WDR_TBL_T-1))
            {
                for(u16cnt = 0;u16cnt<(SCLVIP_WDR_TBL_CNT);u16cnt++)
                {
                    u16Tvalue = *(p16Tbllocal+u16cnt+(u16idx*SCLVIP_WDR_TBL_CNT));
                    HalUtilityW2BYTETbl(u32reg+2+u16cnt*2,u16Tvalue,0xFFFF);
                }
            }
            else
            {
                u16cnt = 0;
                u16Tvalue = *(p16Tbllocal+u16cnt+(u16idx*SCLVIP_WDR_TBL_CNT));
                HalUtilityW2BYTETbl(u32reg+2+u16cnt*2,u16Tvalue,0xFFFF);
            }
            HalUtilityW2BYTETbl(u32reg,(0x1100+u16idx),0xFFFF);
        }
    }
    return bRet;
}
bool HalSclVipSramDump(HalSclVipSramDumpType_e endump,u32 u32Sram)
{
    u16 u16clkreg;
    bool bRet;
    u32 u32reg = endump == E_HAL_SCLVIP_SRAM_DUMP_IHC ? REG_VIP_ACE2_7C_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_ICC ?  REG_VIP_ACE2_78_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_Y ?  REG_VIP_SCNR_41_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_U ?  REG_VIP_SCNR_41_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GAMMA_V ?  REG_VIP_SCNR_41_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM10to12_R ?  REG_SCL_HVSP1_7A_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM10to12_G ?  REG_SCL_HVSP1_7A_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM10to12_B ?  REG_SCL_HVSP1_7A_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM12to10_R ?  REG_SCL_HVSP1_7D_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM12to10_G ?  REG_SCL_HVSP1_7D_L :
                    endump == E_HAL_SCLVIP_SRAM_DUMP_GM12to10_B ?  REG_SCL_HVSP1_7D_L :
                        0;
    //clk open
    u16clkreg = HalUtilityR2BYTEDirect(REG_SCL_CLK_51_L);
    HalUtilityW2BYTEMSKDirect(REG_SCL_CLK_51_L,0x0,0x1F);
    bRet = _HalSclVipSetSramDump(endump,u32Sram,u32reg);

    //clk close
    HalUtilityW2BYTEMSKDirect(REG_SCL_CLK_51_L,u16clkreg,0x1F);
    return bRet;
}

//-----------------------DLC
void HalSclVipDlcHistVarOnOff(u16 u16var)
{
    HalUtilityW2BYTEMSK(REG_VIP_DLC_04_L,u16var,0x25a2);
}

void HalSclVipSetDlcstatMIU(u8 u8value,u32 u32addr1,u32 u32addr2)
{
    HalUtilityW2BYTEMSK(REG_VIP_MWE_15_L,u8value,0x0001);
    HalUtilityW4BYTE(REG_VIP_MWE_18_L,u32addr1>>4);
    HalUtilityW4BYTE(REG_VIP_MWE_1A_L,u32addr2>>4);
}
bool HalSclVipGetWdrOnOff(void)
{
    return HalUtilityR2BYTE(REG_VIP_WDR1_00_L)&BIT0;
}
void HalSclVipHwReset(void)
{
    HalUtilityW2BYTEMSKDirect(REG_VIP_MWE_15_L,0,0x0001);
    HalUtilityW2BYTEMSKDirect(REG_SCL_ARBSHP_00_L,0,BIT0);
    HalSclVipInitY2R();
}
void HalSclVipSetRoiHistSrc(DrvSclVipWdrRoiSrcType_e enType)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,(enType==E_DRV_SCLVIP_ROISRC_BEFORE_WDR) ? 0 : BIT1,BIT1);
}
void _HalSclVipSetRoiHistWin0Cfg(DrvSclVipWdrRoiConfig_t *stRoiCfg)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,(stRoiCfg->bEnSkip) ? BIT2 : 0,BIT2);
    //xy

    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_01_L,(stRoiCfg->u16RoiAccX[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_02_L,(stRoiCfg->u16RoiAccY[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_03_L,(stRoiCfg->u16RoiAccX[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_04_L,(stRoiCfg->u16RoiAccY[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_05_L,(stRoiCfg->u16RoiAccX[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_06_L,(stRoiCfg->u16RoiAccY[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_07_L,(stRoiCfg->u16RoiAccX[3]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_08_L,(stRoiCfg->u16RoiAccY[3]),0x1FFF);
}
void _HalSclVipSetRoiHistWin1Cfg(DrvSclVipWdrRoiConfig_t *stRoiCfg)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,(stRoiCfg->bEnSkip) ? BIT3 : 0,BIT3);
    //xy

    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_13_L,(stRoiCfg->u16RoiAccX[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_14_L,(stRoiCfg->u16RoiAccY[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_15_L,(stRoiCfg->u16RoiAccX[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_16_L,(stRoiCfg->u16RoiAccY[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_17_L,(stRoiCfg->u16RoiAccX[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_18_L,(stRoiCfg->u16RoiAccY[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_19_L,(stRoiCfg->u16RoiAccX[3]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_1A_L,(stRoiCfg->u16RoiAccY[3]),0x1FFF);
}
void _HalSclVipSetRoiHistWin2Cfg(DrvSclVipWdrRoiConfig_t *stRoiCfg)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,(stRoiCfg->bEnSkip) ? BIT4 : 0,BIT4);
    //xy

    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_25_L,(stRoiCfg->u16RoiAccX[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_26_L,(stRoiCfg->u16RoiAccY[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_27_L,(stRoiCfg->u16RoiAccX[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_28_L,(stRoiCfg->u16RoiAccY[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_29_L,(stRoiCfg->u16RoiAccX[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_2A_L,(stRoiCfg->u16RoiAccY[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_2B_L,(stRoiCfg->u16RoiAccX[3]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_2C_L,(stRoiCfg->u16RoiAccY[3]),0x1FFF);
}
void _HalSclVipSetRoiHistWin3Cfg(DrvSclVipWdrRoiConfig_t *stRoiCfg)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,(stRoiCfg->bEnSkip) ? BIT5 : 0,BIT5);
    //xy

    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_37_L,(stRoiCfg->u16RoiAccX[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_38_L,(stRoiCfg->u16RoiAccY[0]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_39_L,(stRoiCfg->u16RoiAccX[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_3A_L,(stRoiCfg->u16RoiAccY[1]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_3B_L,(stRoiCfg->u16RoiAccX[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_3C_L,(stRoiCfg->u16RoiAccY[2]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_3D_L,(stRoiCfg->u16RoiAccX[3]),0x1FFF);
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_3E_L,(stRoiCfg->u16RoiAccY[3]),0x1FFF);
}
void HalSclVipSetRoiHistCfg(u16 idx, DrvSclVipWdrRoiConfig_t *stRoiCfg)
{
    switch(idx)
    {
        case 0:
            _HalSclVipSetRoiHistWin0Cfg(stRoiCfg);
            break;
        case 1:
            _HalSclVipSetRoiHistWin1Cfg(stRoiCfg);
            break;
        case 2:
            _HalSclVipSetRoiHistWin2Cfg(stRoiCfg);
            break;
        case 3:
            _HalSclVipSetRoiHistWin3Cfg(stRoiCfg);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            break;
    }
}
void HalSclVipSetRoiHistBaseAddr(u16 idx, u32 u32BaseAddr)
{
    u32 u32Addr;
    u32 Reg = 0;
    u32Addr = ((u32BaseAddr)>>5);//256bit MIU
    switch(idx)
    {
        case 0://2048byte
            Reg = REG_SCL_ARBSHP_49_L;
            break;
        case 1://256byte
            Reg = REG_SCL_ARBSHP_4B_L;
            break;
        case 2://256byte
            Reg = REG_SCL_ARBSHP_4D_L;
            break;
        case 3://256byte
            Reg = REG_SCL_ARBSHP_4F_L;
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    HalUtilityW2BYTEMSK(Reg,u32Addr,0xFFFF);
    HalUtilityW2BYTEMSK(Reg+2,(u32Addr>>16),0x7FF);
}
void HalSclVipReSetRoiHistCfg(u16 idx)
{
    DrvSclVipWdrRoiConfig_t stRoiCfg;
    DrvSclOsMemset(&stRoiCfg,0,sizeof(DrvSclVipWdrRoiConfig_t));
    switch(idx)
    {
        case 0:
            _HalSclVipSetRoiHistWin0Cfg(&stRoiCfg);
            break;
        case 1:
            _HalSclVipSetRoiHistWin1Cfg(&stRoiCfg);
            break;
        case 2:
            _HalSclVipSetRoiHistWin2Cfg(&stRoiCfg);
            break;
        case 3:
            _HalSclVipSetRoiHistWin3Cfg(&stRoiCfg);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            break;
    }
}
void HalSclVipSetRoiHistOnOff(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,bEn ? BIT0 : 0,BIT0);
}
void HalSclVipSetWdrMultiSensor(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_VIP_WDR1_00_L,bEn ? BIT4 : 0,BIT4);
}
void HalSclVipSetWdrMloadConfig(bool bEn,u32 u32WdrBuf)
{
    u32 u32Addr = (u32WdrBuf>>4);
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_0B_L,0xa302,0xFFFF);//B
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_00_L,0x000a,0x1F);//0
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_03_L,(u32Addr&0xFFFF),0xFFFF);// 3
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_04_L,((u32Addr>>16)),0xFFFF);// 4
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_01_L,0x00ff,0xFFFF);// 1
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_02_L,0x0000,0xFFFF);// 2
    HalUtilityW2BYTEMSK(REG_SCL_MLOAD_09_L,bEn ? 0x0003 : 0x1,0xF);// 9
}
void HalSclVipSetWdrMloadBuffer(u32 u32Buffer)
{
    u32 u32Addr;
    u32Addr = ((u32Buffer)>>5);//256bit MIU
    HalUtilityW2BYTEMSK(REG_VIP_WDR1_7E_L,u32Addr,0xFFFF);
    HalUtilityW2BYTEMSK(REG_VIP_WDR1_7F_L,(u32Addr>>16),0x7FF);
}
void HalSclVipSetNrHistOnOff(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_VIP_NE_19_L,bEn ?  BIT0: 0,BIT0);
}
s32 HalSclVipGetWdrHistogram(u16 idx)
{
    s32 s32Y = 0;
    switch(idx)
    {
        case 0:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_0F_L);
            break;
        case 1:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_21_L);
            break;
        case 2:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_33_L);
            break;
        case 3:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_45_L);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    return s32Y;
}
s32 HalSclVipGetWdrRHistogram(u16 idx)
{
    s32 s32Y=0;
    switch(idx)
    {
        case 0:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_09_L);
            break;
        case 1:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_1B_L);
            break;
        case 2:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_2D_L);
            break;
        case 3:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_3F_L);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    return s32Y;
}
s32 HalSclVipGetWdrGHistogram(u16 idx)
{
    s32 s32Y=0;
    switch(idx)
    {
        case 0:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_0B_L);
            break;
        case 1:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_1D_L);
            break;
        case 2:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_2F_L);
            break;
        case 3:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_41_L);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    return s32Y;
}
s32 HalSclVipGetWdrBHistogram(u16 idx)
{
    s32 s32Y=0;
    switch(idx)
    {
        case 0:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_0D_L);
            break;
        case 1:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_1F_L);
            break;
        case 2:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_31_L);
            break;
        case 3:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_43_L);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    return s32Y;
}
s32 HalSclVipGetWdrYCntHistogram(u16 idx)
{
    s32 s32Y=0;
    switch(idx)
    {
        case 0:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_11_L);
            break;
        case 1:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_23_L);
            break;
        case 2:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_35_L);
            break;
        case 3:
            s32Y = HalUtilityR4BYTEDirect(REG_SCL_ARBSHP_47_L);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s idx not support\n",__FUNCTION__);
            SCLOS_BUG();
            break;
    }
    return s32Y;
}
void HalSclVipSetWDRMaskOnOff(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_SCL_ARBSHP_00_L,bEn ?BIT6 :0,BIT6);
}
void HalSclVipSetNRMaskOnOff(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_VIP_NE_19_L,bEn ?0 : BIT1,BIT1);  //0 close update
}
void HalSclVipSetNRHistogramYCSel(HalSclVipYCSelType_e enType)
{
    switch(enType)
    {
        case E_HAL_SCLVIP_YCSel_Y:
            HalUtilityW2BYTEMSKDirect(REG_VIP_NE_18_L,0,BIT0);
            break;
        case E_HAL_SCLVIP_YCSel_C:
            HalUtilityW2BYTEMSKDirect(REG_VIP_NE_18_L,BIT0,BIT0);
            break;
        default:
            SCL_ERR("[HALSCLVIP]%s enType not support\n",__FUNCTION__);
            break;
    }
}
void HalSclVipGetNRHistogramDebug(void *pvCfg)
{
    u16 *u16pBuf;
    u16 idx;
    u16pBuf = (u16 *)pvCfg;
    SCL_ERR("[HALVIP] %s\n",__FUNCTION__);
    for(idx = 0;idx<NRBUFFERCNT;idx++)
    {
        SCL_ERR("[HALVIP] %hd:%hx @%lx\n",idx,*(u16pBuf+idx),(u32)(u16pBuf+idx));
        SCL_ERR("[HALVIP] %hd:%hx\n",idx,*(u16pBuf+((idx*2)+NRBUFFERCNT)));
        SCL_ERR("[HALVIP] %hd:%hx\n",idx,*(u16pBuf+((idx*2)+NRBUFFERCNT)+1));
        SCL_ERR("[HALVIP] %hd:%hx\n",idx,*(u16pBuf+(idx+(NRBUFFERCNT*3))));
    }
}
void HalSclVipGetNRDummy(void *pvCfg)
{
    u16 *u16pBuf;
    u16pBuf = (u16 *)pvCfg;
    *(u16pBuf) = HalUtilityR2BYTEDirect(REG_VIP_NE_10_L);
    *(u16pBuf+1) = HalUtilityR2BYTEDirect(REG_VIP_NE_16_L);
    *(u16pBuf+2) = HalUtilityR2BYTEDirect(REG_VIP_NE_70_L);
    *(u16pBuf+3) = HalUtilityR2BYTEDirect(REG_VIP_NE_71_L);
    *(u16pBuf+4) = HalUtilityR2BYTEDirect(REG_VIP_NE_72_L);
}
void HalSclVipGetNRHistogram(void *pvCfg)
{
    u16 *u16pBuf;
    u16 idx;
    u16pBuf = (u16 *)pvCfg;
    for(idx = 0;idx<NRBUFFERCNT;idx++)
    {
        *(u16pBuf+idx) = HalUtilityR2BYTEDirect(REG_VIP_NE_30_L+(idx*2));
        *(u16pBuf+((idx*2)+NRBUFFERCNT)) = HalUtilityR2BYTEDirect(REG_VIP_NE_38_L+(idx*4));
        *(u16pBuf+((idx*2)+NRBUFFERCNT)+1) = HalUtilityR2BYTEDirect(REG_VIP_NE_38_L+(idx*4)+2);
        *(u16pBuf+(idx+(NRBUFFERCNT*3))) = HalUtilityR2BYTEDirect(REG_VIP_NE_48_L+(idx*2));
    }
    //HalSclVipGetNRHistogramDebug(pvCfg);
}
void HalSclVipSetDlcShift(u8 u8value)
{
    HalUtilityW2BYTEMSK(REG_VIP_DLC_03_L,u8value,0x0007);
}
void HAlSclVipSetDlcMode(u8 u8value)
{
    HalUtilityW2BYTEMSK(REG_VIP_MWE_1C_L,u8value<<2,0x0004);
}
void HalSclVipSetDlcActWin(bool bEn,u16 u16Vst,u16 u16Hst,u16 u16Vnd,u16 u16Hnd)
{
    HalUtilityW2BYTEMSK(REG_VIP_DLC_08_L,bEn<<7,0x0080);
    HalUtilityW2BYTEMSK(REG_VIP_MWE_01_L,u16Vst,0x03FF);
    HalUtilityW2BYTEMSK(REG_VIP_MWE_02_L,u16Vnd,0x03FF);
    HalUtilityW2BYTEMSK(REG_VIP_MWE_03_L,u16Hst,0x01FF);
    HalUtilityW2BYTEMSK(REG_VIP_MWE_04_L,u16Hnd,0x01FF);
}
void HalSclVipDlcHistSetRange(u8 u8value,u8 u8range)
{
    u16 u16tvalue;
    u16 u16Mask;
    u32 u32Reg;
    u8range = u8range-1;
    u32Reg = REG_VIP_DLC_0C_L+(((u8range)/2)*2);
    if((u8range%2) == 0)
    {
        u16Mask     = 0x00FF;
        u16tvalue   = ((u16)u8value);

    }
    else
    {
        u16Mask     = 0xFF00;
        u16tvalue   = ((u16)u8value)<<8;
    }

    HalUtilityW2BYTEMSK(u32Reg,(u16tvalue),u16Mask);

}
u32 HalSclVipDlcHistGetRange(u8 u8range)
{
    u32 u32tvalue;
    u32 u32Reg;
    u32Reg      = REG_VIP_MWE_20_L+(((u8range)*2)*2);
    u32tvalue   = HalUtilityR4BYTEDirect(u32Reg);
    return u32tvalue;

}

u8 HalSclVipDlcGetBaseIdx(void)
{
    u8 u8tvalue;
    u8tvalue = HalUtilityR2BYTEDirect(REG_VIP_MWE_15_L);
    u8tvalue = (u8)(u8tvalue&0x80)>>7;
    return u8tvalue;
}

u32 HalSclVipDlcGetPC(void)
{
    u32 u32tvalue;
    u32tvalue = HalUtilityR4BYTEDirect(REG_VIP_MWE_08_L);
    return u32tvalue;
}

u32 HalSclVipDlcGetPW(void)
{
    u32 u32tvalue;
    u32tvalue = HalUtilityR4BYTEDirect(REG_VIP_MWE_0A_L);
    return u32tvalue;
}

u8 HalSclVipDlcGetMinP(void)
{
    u16 u16tvalue;
    u16tvalue = HalUtilityR2BYTEDirect(REG_VIP_DLC_62_L);
    u16tvalue = (u16tvalue>>8);
    return (u8)u16tvalue;
}

u8 HalSclVipDlcGetMaxP(void)
{
    u16 u16tvalue;
    u16tvalue = HalUtilityR2BYTEDirect(REG_VIP_DLC_62_L);
    return (u8)u16tvalue;
}
void HalSclVipMcnrInit(void)
{
    //HalUtilityW2BYTEMSK(REG_SCL_DNR1_7F_L, 0x0007, BIT2|BIT1|BIT0); //for I3e ECO
}
void HalSclVipAipDB(u8 u8En)
{
    HalUtilityW2BYTEMSK(REG_VIP_SCNR_7F_L, u8En ? 0 : BIT0, BIT0);
}

void HalSclVipSetAutoDownloadAddr(u32 u32baseadr,u16 u16iniaddr,u8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            HalUtilityW2BYTEMSK(REG_SCL1_73_L, (u16)(u32baseadr>>4), 0xFFFF);
            HalUtilityW2BYTEMSK(REG_SCL1_74_L, (u16)(u32baseadr>>20), 0x01FF);
            HalUtilityW2BYTEMSK(REG_SCL1_77_L, ((u16)u16iniaddr), 0xFFFF);
            break;
        default:
            break;
    }
}

void HalSclVipSetAutoDownloadReq(u16 u16depth,u16 u16reqlen,u8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            HalUtilityW2BYTEMSK(REG_SCL1_76_L, ((u16)u16reqlen), 0xFFFF);
            HalUtilityW2BYTEMSK(REG_SCL1_75_L, ((u16)u16depth), 0xFFFF);
            break;
        default:
            break;
    }
}

void HalSclVipSetAutoDownload(u8 bCLientEn,u8 btrigContinue,u8 u8cli)
{
    switch(u8cli)
    {
        case 9:
            HalUtilityW2BYTEMSK(REG_SCL1_72_L, bCLientEn|(btrigContinue<<1), 0x0003);
            break;
        default:
            break;
    }
}

void HalSclVipSetAutoDownloadTimer(u8 bCLientEn)
{
    HalUtilityW2BYTEMSK(REG_SCL1_78_L, bCLientEn<<15, 0x8000);
}
void HalSclVipGetNlmSram(u16 u16entry)
{
    u32 u32tvalue1,u32tvalue2;
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, u16entry, 0x07FF);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x8000, 0x8000);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x2000, 0x2000);
    u32tvalue1 = HalUtilityR2BYTEDirect(REG_SCL_NLM0_64_L);
    u32tvalue2 = HalUtilityR2BYTEDirect(REG_SCL_NLM0_65_L);
    u32tvalue1 |= ((u32tvalue2&0x00F0)<<12);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x0000, 0x8000);
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[Get_SRAM]entry%hx :%lx\n",u16entry,u32tvalue1);
}

void HalSclVipSetNlmSrambyCPU(u16 u16entry,u32 u32tvalue)
{
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, u16entry, 0x07FF);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x8000, 0x8000);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_63_L, (u16)u32tvalue, 0xFFFF);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_65_L, (u16)(u32tvalue>>16), 0x000F);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x4000, 0x4000);
    HalUtilityW2BYTEMSK(REG_SCL_NLM0_62_L, 0x0000, 0x8000);
    SCL_DBG(SCL_DBG_LV_DRVVIP()&EN_DBGMG_VIPLEVEL_VIPLOG, "[Set_SRAM]entry%hx :%lx\n",u16entry,u32tvalue);
}
void HalSclVipInitNeDummy(void)
{
    HalUtilityW2BYTEMSKDirect(REG_VIP_NE_10_L, 38400, 0xFFFF);
    HalUtilityW2BYTEMSKDirect(REG_VIP_NE_16_L, 6, 0x000F);
    HalUtilityW2BYTEMSKDirect(REG_VIP_NE_70_L, 0x2010, 0xFFFF);
    HalUtilityW2BYTEMSKDirect(REG_VIP_NE_71_L, 0x0220, 0xFFFF);
    HalUtilityW2BYTEMSKDirect(REG_VIP_NE_72_L, 0x0404, 0xFFFF);
}
void HalSclVipSetVpsSRAMEn(bool bEn)
{
    HalUtilityW2BYTEMSK(REG_VIP_PK_10_L, bEn ? BIT7 : 0, BIT7);
}
void HalSclVipInitY2R(void)
{
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_60_L, 0x0A01, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_61_L, 0x59E, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_62_L, 0x401, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_63_L, 0x1FFF, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_64_L, 0x1D24, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_65_L, 0x400, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_66_L, 0x1E9F, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_67_L, 0x1FFF, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_68_L, 0x400, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP2_69_L, 0x719, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_6C_L, 0x181, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_6D_L, 0x1FF, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_6E_L, 0x1E54, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_6F_L, 0x1FAD, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_70_L, 0x132, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_71_L, 0x259, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_72_L, 0x75, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_73_L, 0x1F53, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_74_L, 0x1EAD, 0x1FFF);
    HalUtilityW2BYTEMSKDirect(REG_SCL_HVSP1_75_L, 0x1FF, 0x1FFF);
}

bool HalSclVipGetVipBypass(void)
{
    bool bRet;
    bRet = HalUtilityR2BYTE(REG_VIP_LCE_70_L)&BIT0;
    return bRet;
}
bool HalSclVipGetMcnrBypass(void)
{
    bool bRet;
    bRet = HalUtilityR2BYTE(REG_VIP_MCNR_01_L)&(BIT0|BIT1);
    bRet = (((bRet)>>1))? 0: 1;
    return bRet;
}
bool HalSclVipGetNlmBypass(void)
{
    bool bRet;
    bRet = (HalUtilityR2BYTE(REG_SCL_NLM0_01_L)&BIT0)? 0: 1;
    return bRet;
}
