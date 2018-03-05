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


#include "hal_utility.h"
#ifndef _DRVPQ_TEMPLATE_H_
#define _DRVPQ_TEMPLATE_H_

#define PQ_INSTALL_FUNCTION


#ifndef UNUSED //to avoid compile warnings...
#define UNUSED(var) (void)((var) = (var))
#endif

#if(PQ_ONLY_SUPPORT_BIN == 0)
static PQTABLE_INFO _Scl_PQTableInfo;
#define MAX_IP_NUM  (PQ_IP_NUM_Main)
static u8 _u8SclPQTabIdx[MAX_IP_NUM];    // store all TabIdx of all IPs

#if (ENABLE_PQ_MLOAD)

static bool _bSclMLoadEn = FALSE;

#define MLOAD_MAX_CMD   30
static u32 _u32SclMLoadCmd[MLOAD_MAX_CMD];
static u16 _u16SclMLoadMsk[MLOAD_MAX_CMD];
static u16 _u16SclMLoadVal[MLOAD_MAX_CMD];
static u16 _u16SclMLoadCmdCnt = 0;
#endif

#if(ENABLE_PQ_LOAD_TABLE_INFO)
static MS_PQ_LOAD_TABLE_INFO g_PQLoadTableInfo;
#endif
#endif

// to save loading SRAM table time, SRAM are only downloaded
// when current SRAM table is different to previous SRAM table
static u8 _u8ICC_CRD_Table = 0xFF;
static u8 _u8IHC_CRD_Table = 0xFF;

static u8 _u8SRAM1Table=0xFF;
static u8 _u8SRAM2Table=0xFF;
static u8 _u8SRAM3Table=0xFF;
static u8 _u8SRAM4Table=0xFF;
static u8 _u8CSRAM1Table=0xFF;
static u8 _u8CSRAM2Table=0xFF;
static u8 _u8CSRAM3Table=0xFF;
static u8 _u8CSRAM4Table=0xFF;

static bool _bcheckReg    = 0 ;
static bool _bcheckSclPQsize = 0 ;
#if (ENABLE_PQ_CMDQ)
#define CMDQ_MAX_CMD_SIZE 512
static u8 gu8debugflag = 0;
#endif


#if (PQ_ENABLE_CHECK == 0)
    #define PQ_REG_FUNC_READ( u32Reg)  MApi_XC_ReadByte(u32Reg)
    #define PQ_REG_FUNC( u32Reg, u8Value, u8Mask )   \
        do{\
        HalUtilityWBYTEMSK( u32Reg, u8Value, u8Mask );\
        }while(0)\

    #define PQ_REG_FUNC_xc( u32Reg, u8Value, u8Mask ) \
    do{ \
            if(u32Reg %2){ \
                    MApi_XC_W2BYTEMSK(u32Reg-1, ((u16)u8Value)<<8, ((u16)u8Mask)<<8); \
                }else{ \
                    MApi_XC_W2BYTEMSK(u32Reg, u8Value, u8Mask); \
                }\
        }while(0)

    #define PQ_REG_MLOAD_WRITE_CMD(u32Reg,u8Value,u8Mask) \
            do{ \
                if(u32Reg%2) \
                { \
                    _u32SclMLoadCmd[_u16SclMLoadCmdCnt] = u32Reg-1; \
                    _u16SclMLoadMsk[_u16SclMLoadCmdCnt] = ((u16)u8Mask)<<8; \
                    _u16SclMLoadVal[_u16SclMLoadCmdCnt] = ((u16)u8Value)<<8; \
                } \
                else \
                { \
                    _u32SclMLoadCmd[_u16SclMLoadCmdCnt] = u32Reg; \
                    _u16SclMLoadMsk[_u16SclMLoadCmdCnt] = ((u16)u8Mask); \
                    _u16SclMLoadVal[_u16SclMLoadCmdCnt] = ((u16)u8Value); \
                } \
                _u16SclMLoadCmdCnt++; \
                if(_u16SclMLoadCmdCnt >= MLOAD_MAX_CMD)\
                {\
                    sclprintf("[PQ ERROR] ====ML overflow !!! \r\n");\
                    _u16SclMLoadCmdCnt = MLOAD_MAX_CMD - 1;        \
                }\
            }while(0)

    #define PQ_REG_MLOAD_FUNC(u32Reg,u8Value,u8Mask) \
            do{ \
                if(u32Reg%2) \
                { \
                    MApi_XC_MLoad_WriteCmd_And_Fire(u32Reg-1, ((u16)u8Value)<<8, ((u16)u8Mask)<<8); \
                } \
                else \
                { \
                    MApi_XC_MLoad_WriteCmd_And_Fire(u32Reg, u8Value, u8Mask); \
                } \
            }while(0)

    #define PQ_SWREG_FUNC( u16Reg, u8Value, u8Mask )  SWReg_WriteByteMask( u16Reg, u8Value, u8Mask )


#else // #if(PQ_ENABLE_CHECK == 1)

static u8 _u8PQfunction = PQ_FUNC_DUMP_REG;

static void _MDrv_Scl_PQ_SetFunction(u8 u8Func)
{
    _u8PQfunction = u8Func;
}

#define PQ_REG_FUNC( u16Reg, u8Value, u8Mask ) \
    do{ \
        if (_u8PQfunction == PQ_FUNC_DUMP_REG){ \
            MApi_XC_WriteByteMask( (u32)u16Reg, u8Value, u8Mask ); \
        }else{ \
            if ((MApi_XC_ReadByte((u32)u16Reg) & u8Mask) != ((u8Value) & (u8Mask))){ \
                sclprintf("[PQRegErr] "); \
                if (((u16Reg) >> 8) == 0x2F){ \
                    sclprintf("bk=%02x, ", (u16)SC_BK_CURRENT); \
                } \
                else if (((u16Reg) >> 8) == 0x36){ \
                    sclprintf("bk=%02x, ", (u16)COMB_BK_CURRENT); \
                } \
                sclprintf("addr=%04x, mask=%02x, val=%02x[%02x]\r\n", \
                    u16Reg, (u16)u8Mask, (u16)MApi_XC_ReadByte((u32)u16Reg), (u16)u8Value); \
            } \
        } \
    }while(0)

#define PQ_SWREG_FUNC( u16Reg, u8Value, u8Mask ) \
    do{ \
        if (_u8PQfunction == PQ_FUNC_DUMP_REG){ \
            SWReg_WriteByteMask( u16Reg, u8Value, u8Mask ); \
        }else{ \
            if (SWReg[u16Reg] & (u8Mask) != (u8Value) & (u8Mask)){ \
                sclprintf("[PQSWRegErr] "); \
                sclprintf("addr=%04x, mask=%02x, val=%02x[%02x]\r\n", \
                    u16Reg, (u16)u8Mask, (u16)SWReg[u16Reg], (u16)u8Value); \
            } \
        } \
    }while(0)

#endif  //#if (PQ_ENABLE_CHECK)


#if(PQ_ONLY_SUPPORT_BIN == 0)
#if 0
static void _MDrv_Scl_PQ_Set_MLoadEn(bool bEn)
{
    u16 i;
    if(bEn == DISABLE)
    {
        //if spread reg, no need to use mutex, but it's ok to use mutex
        //   (because it's not MApi_XC_W2BYTE(), which has mutex already)
        //if not spread reg, must use mutex to protect MLoad_trigger func.
        SC_BK_STORE_MUTEX;

        if(_u16SclMLoadCmdCnt)
        {
            for(i=1; i<_u16SclMLoadCmdCnt; i++)
            {
                if(_u32SclMLoadCmd[i-1] == _u32SclMLoadCmd[i])
                {
                    _u16SclMLoadMsk[i] |= _u16SclMLoadMsk[i-1];
                    _u16SclMLoadVal[i] |= _u16SclMLoadVal[i-1];
                }
            }
    #if(ENABLE_PQ_MLOAD)

            MApi_XC_MLoad_WriteCmds_And_Fire(
                &_u32SclMLoadCmd[0], &_u16SclMLoadVal[0], &_u16SclMLoadMsk[0], _u16SclMLoadCmdCnt);
    #endif
            _u16SclMLoadCmdCnt = 0;
        }

        SC_BK_RESTORE_MUTEX;
    }
    else
    {
        //_u16SclMLoadCmdCnt = 0;
    }

    _bSclMLoadEn = bEn;
}
#endif

#if PQ_ENABLE_UNUSED_FUNC

static void _MDrv_Scl_PQ_PreInitLoadTableInfo(void)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)

    u16 u16IPIdx;
    g_PQLoadTableInfo._u16CurInputSrcType = -1;
    for( u16IPIdx = 0; u16IPIdx < MAX_IP_NUM; ++ u16IPIdx )
    {
        g_PQLoadTableInfo._au8IPGroupIdx[u16IPIdx] = 0xFF;
    }
#endif

}

static void _MDrv_Scl_PQ_Set_LoadTableInfo_IP_Tab(u16 u16IPIdx, u8 u8TabIdx)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)
    g_PQLoadTableInfo._au8IPGroupIdx[u16IPIdx] = u8TabIdx;
#else
    UNUSED(u16IPIdx);
    UNUSED(u8TabIdx);
#endif

}

static u8 _MDrv_Scl_PQ_Get_LoadTableInfo_IP_Tab(u16 u16IPIdx)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)
    return g_PQLoadTableInfo._au8IPGroupIdx[u16IPIdx];
#else
    UNUSED(u16IPIdx);
    return 0xFF;
#endif
}

static void _MDrv_Scl_PQ_Set_LoadTableInfo_SrcType(u16 u16SrcType)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)
   g_PQLoadTableInfo._u16CurInputSrcType = u16SrcType;
#else
   UNUSED(u16SrcType);
#endif
}

static u16 _MDrv_Scl_PQ_Get_LoadTableInfo_SrcType(void)
{
#if(ENABLE_PQ_LOAD_TABLE_INFO)
   return g_PQLoadTableInfo._u16CurInputSrcType;
#else
   return 0xFFFF;
#endif
}
#endif

static void _MDrv_Scl_PQ_DumpGeneralRegTable(EN_IP_Info* pIP_Info)
{
    u32 u32Addr;
    u8 u8Mask;
    u8 u8Value;

    PQ_DUMP_DBG(sclprintf("tab: general\r\n"));
    if (pIP_Info->u8TabIdx >= pIP_Info->u8TabNums){
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: General Reg Table\r\n"));
        return;
    }

    while (1)
    {
         u32Addr = (pIP_Info->pIPTable[0]<<8) + pIP_Info->pIPTable[1];
         u8Mask  = pIP_Info->pIPTable[2];
         u8Value = pIP_Info->pIPTable[REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabIdx];

         if (u32Addr == _END_OF_TBL_) // check end of table
             break;

         u32Addr |= NON_PM_BASE;
         PQ_DUMP_DBG(sclprintf("[addr=%04lx, msk=%02x, val=%02x]\r\n", u32Addr, u8Mask, u8Value));
         PQ_REG_FUNC(u32Addr, u8Value, u8Mask);

         pIP_Info->pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // next
    }
}

#if PQ_ENABLE_UNUSED_FUNC

static void _MDrv_Scl_PQ_DumpCombRegTable(EN_IP_Info* pIP_Info)
{
    u32 u32Addr;
    u8 u8Mask;
    u8 u8Value;
    u8 u8CurBank = 0xff;
    COMB_BK_STORE;

    PQ_DUMP_DBG(sclprintf("tab: comb\r\n"));
    if (pIP_Info->u8TabIdx >= pIP_Info->u8TabNums){
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: Comb Reg Table\r\n"));
        return;
    }

    while (1)
    {
        u32Addr = (pIP_Info->pIPTable[0]<<8) + pIP_Info->pIPTable[1];
        u8Mask  = pIP_Info->pIPTable[2];
        u8Value = pIP_Info->pIPTable[REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabIdx];

        if (u32Addr == _END_OF_TBL_) // check end of table
            break;

        u8CurBank = (u8)(u32Addr >> 8);
        if (u8CurBank != COMB_BK_CURRENT)
        {
            PQ_DUMP_DBG(sclprintf("<<bankswitch=%02x>>\r\n", u8CurBank));
            COMB_BK_SWITCH(u8CurBank);
        }

        u32Addr = COMB_REG_BASE | (u32Addr & 0x00FF);

        PQ_DUMP_DBG(sclprintf("[addr=%04x, msk=%02x, val=%02x]\r\n", (int)u32Addr, u8Mask, u8Value));

/*
//Only works for T4, obsolete in main trunk.
    #if (CHIP_FAMILY_TYPE == CHIP_FAMILY_S7LD)
        // patch Comb register H part can not be written by 16 bits riu
        if((u32Addr == (COMB_REG_BASE + 0x47)) ||
           (u32Addr == (COMB_REG_BASE + 0x5F)) ||
           (u32Addr == (COMB_REG_BASE + 0x8F)) ||
           (u32Addr == (COMB_REG_BASE + 0xD7)) ||
           (u32Addr == (COMB_REG_BASE + 0xDF)))
        {
            u8 u8low, u8High;
            u16 u16value;

            u8low = MApi_XC_ReadByte(u32Addr-1);
            u8High = MApi_XC_ReadByte(u32Addr);
            u16value = (u8High & ~u8Mask) | (u8Value & u8Mask);
            u16value = (u16value<<8) | u16value;

            MApi_XC_Write2ByteMask(u32Addr-1, u16value, 0xFFFF);
            PQ_REG_FUNC(u32Addr-1, u8low, 0xFF);
        }
        else
    #endif
*/
        {
            PQ_REG_FUNC(u32Addr, u8Value, u8Mask);
        }

        pIP_Info->pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // next
    }

    COMB_BK_RESTORE;
}
#endif


static void _MDrv_Scl_PQ_DumpSclaerRegTableByData(EN_IP_Info* pIP_Info, PQ_DATA_INFO *pData)
{
    u32 u32Addr;
    u8 u8Mask;
    u8 u8Value;
    u8 u8CurBank = 0xff;
    u16 u16DataIdx;

    u8CurBank = u8CurBank;

    SC_BK_STORE_MUTEX;
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]%s %d:: \n"
        , __FUNCTION__, __LINE__);
    if (pIP_Info->u8TabIdx >= pIP_Info->u8TabNums)
    {
        PQ_ERR(sclprintf("[PQ]IP_Info error: Scaler Reg Table\r\n"));

        SC_BK_RESTORE_MUTEX;

        return;
    }

    u16DataIdx = 0;
    while (1)
    {
        u32Addr = (pIP_Info->pIPTable[0]<<8) + pIP_Info->pIPTable[1];
        u8Mask  = pIP_Info->pIPTable[2];
        u8Value =  pData->pBuf[u16DataIdx];

        if (u32Addr == _END_OF_TBL_) // check end of table
        {
            break;
        }
        u8CurBank = (u8)(u32Addr >> 8);

    #if (SCALER_REGISTER_SPREAD)
        if(pIP_Info->u8TabType == PQ_TABTYPE_NE)
        {
            u32Addr = BK_NE_BASE | u32Addr;
        }
        else
        {
            u32Addr = BK_SCALER_BASE | u32Addr;
        }
    #else
        if (u8CurBank != SC_BK_CURRENT)
        {
            SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "<<bankswitch=%02x>>\r\n",u8CurBank);
            SC_BK_SWITCH(u8CurBank);
        }
        if(pIP_Info->u8TabType == PQ_TABTYPE_NE)
        {
            u32Addr = BK_NE_BASE | (u32Addr & 0x00FF);
        }
        else
        {
            u32Addr =  BK_SCALER_BASE | (u32Addr & 0x00FF);
        }
    #endif
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "BK =%04X, addr=%02X, msk=%02X, value=%02X\r\n"
        ,(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)u8Value);

    //SCL_ERR( "BK =%04X, addr=%02X, msk=%02X, value=%02X\r\n"
    //    ,(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)u8Value);
    #if (ENABLE_PQ_CMDQ)

        if(_bcheckReg)
        {
            u8 u8MaskL;
            u8 u8Value_CMDQ;
            u32 u32Addr_CMDQ;
            u8Value_CMDQ=PQ_REG_FUNC_READ(u32Addr);
            if((u8Value_CMDQ&u8Mask)!=(u8Value&u8Mask))
            {
                PQ_REG_FUNC(u32Addr, u8Value, u8Mask);
                (SCL_DBGERR("BK =%04X, addr=%02X, msk=%02X, value=%02X ckvalue=%02X\r\n",(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)(u8Value&u8Mask), (u16)(u8Value_CMDQ)));
            }
            else if(((u8Value)==(0xFF))&&((u8Value_CMDQ&u8Mask)!=u8Mask))
            {
                PQ_REG_FUNC(u32Addr, u8Value, u8Mask);
                (SCL_DBGERR("BK =%04X, addr=%02X, msk=%02X, value=%02X ckvalue=%02X\r\n",(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)(u8Value&u8Mask), (u16)(u8Value_CMDQ)));
            }
            else
            {
                SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "pass BK =%04X, addr=%02X value=%02X ckvalue=%02X\n"
                    ,(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)(u8Value&u8Mask), (u16)(u8Value_CMDQ&u8Mask));
            }
            pIP_Info->pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // next
            u32Addr_CMDQ = (pIP_Info->pIPTable[0]<<8) + pIP_Info->pIPTable[1];
            if(((u32Addr%2)&&(((u32Addr&0xFF)-1)==((u32Addr_CMDQ&0xFF))))&&(u32Addr_CMDQ != _END_OF_TBL_))//is H level and next L level
            {
                u8MaskL = pIP_Info->pIPTable[2];
                if(u8MaskL==0xFF)
                {
                    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[warning HL]BK =%04X, addr=%02X\r\n"
                        ,(u16)((u32Addr&0xFFFF00)>>8),(u16)(u32Addr&0xFF));
                }
            }
            pIP_Info->pIPTable-=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // roll back
        }
        else if(_bcheckSclPQsize)
        {
            if((u8Mask & u8Value)==0 &&(u8Value!=0))
            {
                (sclprintf("[PQ-major]BK =%04X, addr=%02X, msk=%02X, value=%02X ,msk&val=%02X \r\n",(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask,(u16)(u8Value) ,(u16)(u8Value&u8Mask)));
            }
            else if((u8Value!=0 && u8Value!=0xFF) &&(~u8Mask & u8Value))
            {
                SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ-minor]BK =%04X, addr=%02X, msk=%02X, value=%02X ,msk&val=%02X \r\n"
                    ,(u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask,(u16)(u8Value) ,(u16)(u8Value&u8Mask));
            }
        }
        else
    #endif
        {
            PQ_REG_FUNC(u32Addr, u8Value, u8Mask);
        }

        pIP_Info->pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // next
        u16DataIdx++;
        if(u16DataIdx > pData->u16BufSize)
        {
            sclprintf("%s %d, Data size is smaller than PQ table, idx:%d size:%d\n",
                __FUNCTION__, __LINE__, u16DataIdx, pData->u16BufSize);
            break;
        }
    }

    if(u16DataIdx != pData->u16BufSize)
    {
        sclprintf("%s %d, Data size is bigger than PQ table, idx:%d, size:%d\n",
            __FUNCTION__, __LINE__, u16DataIdx, pData->u16BufSize);
    }

    SC_BK_RESTORE_MUTEX;
}



static void _MDrv_Scl_PQ_DumpScalerRegTable(EN_IP_Info* pIP_Info)
{
    u32 u32Addr;
    u8 u8Mask;
    u8 u8Value;
    u8 u8CurBank = 0xff;

    u8CurBank = u8CurBank;

#ifdef SCLOS_TYPE_LINUX
    #if(ENABLE_PQ_MLOAD)
    pthread_mutex_lock(&_Scl_PQ_MLoad_Mutex);
    #endif
#endif

    SC_BK_STORE_MUTEX;
    PQ_DUMP_DBG(sclprintf("tab: sc\r\n"));
    if (pIP_Info->u8TabIdx >= pIP_Info->u8TabNums)
    {
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: Scaler Reg Table\r\n"));

        SC_BK_RESTORE_MUTEX;
#ifdef SCLOS_TYPE_LINUX
        #if(ENABLE_PQ_MLOAD)
        pthread_mutex_unlock(&_Scl_PQ_MLoad_Mutex);
        #endif
#endif

        return;
    }

    while (1)
    {
        u32Addr = (pIP_Info->pIPTable[0]<<8) + pIP_Info->pIPTable[1];
        u8Mask  = pIP_Info->pIPTable[2];
        u8Value = pIP_Info->pIPTable[REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabIdx];

        if (u32Addr == _END_OF_TBL_) // check end of table
            break;

        u8CurBank = (u8)(u32Addr >> 8);
        PQ_DUMP_DBG(sclprintf("XC bk =%x, addr=%x, msk=%x, value=%x\r\n", (u16)((u32Addr&0xFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)u8Value));
#if(ENABLE_PQ_MLOAD)
        if(_bSclMLoadEn)
        {
            //sclprintf("MLad: %lx, %x, %x\r\n", u32Addr, u8Value, u8Mask);
            PQ_REG_MLOAD_WRITE_CMD(u32Addr, u8Value, u8Mask);
            //PQ_REG_MLOAD_FUNC(u32Addr, u8Value, u8Mask);
        }
        else
#endif
        {
            #if (SCALER_REGISTER_SPREAD)
            if(pIP_Info->u8TabType == PQ_TABTYPE_NE)
            {
                u32Addr = BK_NE_BASE | u32Addr;
            }
            else
            {
                u32Addr = BK_SCALER_BASE | u32Addr;
            }
            #else
            if (u8CurBank != SC_BK_CURRENT)
            {
                PQ_DUMP_DBG(sclprintf("<<bankswitch=%02x>>\r\n", u8CurBank));
                SC_BK_SWITCH(u8CurBank);
            }

            if(pIP_Info->u8TabType == PQ_TABTYPE_NE)
            {
                u32Addr = BK_NE_BASE | (u32Addr & 0x00FF);
            }
            else
            {
                u32Addr =  BK_SCALER_BASE | (u32Addr & 0x00FF);
            }
            #endif
            PQ_REG_FUNC(u32Addr, u8Value, u8Mask);
            //SCL_ERR("XC bk =%x, addr=%x, msk=%x, value=%x\r\n", (u16)((u32Addr&0xFFFF00)>>8), (u16)(u32Addr&0xFF), (u16)u8Mask, (u16)u8Value);
        }
        pIP_Info->pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+pIP_Info->u8TabNums); // next
    }


    SC_BK_RESTORE_MUTEX;

#ifdef SCLOS_TYPE_LINUX
    #if(ENABLE_PQ_MLOAD)
    pthread_mutex_unlock(&_Scl_PQ_MLoad_Mutex);
    #endif
#endif
}

static void _MDrv_Scl_PQ_DumpFilterTable(EN_IP_Info* pIP_Info)
{

    PQ_DUMP_FILTER_DBG(sclprintf("tab: sram\r\n"));
    if (pIP_Info->u8TabIdx >= pIP_Info->u8TabNums){
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: SRAM Table\r\n"));
        return;
    }
    PQ_DUMP_FILTER_DBG(sclprintf("pIP_Info->u8TabType: %u\r\n",pIP_Info->u8TabType));
    switch(pIP_Info->u8TabType)
    {
        case PQ_TABTYPE_SRAM_COLOR_INDEX:
            Hal_Scl_PQ_set_sram_color_index_table(_Scl_PQTableInfo.eWin, SC_FILTER_SRAM_COLOR_INDEX, (void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_SRAM_COLOR_INDEX)]));
            break;

        case PQ_TABTYPE_SRAM_COLOR_GAIN_SNR:
            Hal_Scl_PQ_set_sram_color_gain_snr_table(_Scl_PQTableInfo.eWin, SC_FILTER_SRAM_COLOR_GAIN_SNR, (void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_SRAM_COLOR_GAIN_SNR)]));
            break;

        case PQ_TABTYPE_SRAM_COLOR_GAIN_DNR:
            Hal_Scl_PQ_set_sram_color_gain_snr_table(_Scl_PQTableInfo.eWin, SC_FILTER_SRAM_COLOR_GAIN_DNR, (void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_SRAM_COLOR_GAIN_DNR)]));
            break;

        case PQ_TABTYPE_VIP_ICC_CRD_SRAM:
            Hal_Scl_PQ_set_sram_icc_crd_table(_Scl_PQTableInfo.eWin, SC_FILTER_SRAM_ICC_CRD, (void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_SRAM_ICC_CRD)]));
            break;

        case PQ_TABTYPE_VIP_IHC_CRD_SRAM:
            Hal_Scl_PQ_set_sram_ihc_crd_table(_Scl_PQTableInfo.eWin, SC_FILTER_SRAM_IHC_CRD, (void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_SRAM_IHC_CRD)]));
            break;

        case PQ_TABTYPE_SRAM1:
            Hal_Scl_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM1 ,(void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_Y_SRAM1)]));
            break;

        case PQ_TABTYPE_SRAM2:
            Hal_Scl_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM2 ,(void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_Y_SRAM2)]));
            break;

        case PQ_TABTYPE_SRAM3:
            Hal_Scl_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM3 ,(void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_Y_SRAM3)]));
            break;

        case PQ_TABTYPE_SRAM4:
            Hal_Scl_PQ_set_yc_sram(FILTER_SRAM_SC1, SC_FILTER_Y_SRAM4 ,(void *)((u32)&pIP_Info->pIPTable[pIP_Info->u8TabIdx * Hal_Scl_PQ_get_sram_size(SC_FILTER_Y_SRAM4)]));
            break;

        default:
            sclprintf("[PQ]Unknown sram type %u\r\n", pIP_Info->u8TabType);
            break;
    }
    PQ_DUMP_FILTER_DBG(sclprintf("tab load finish\r\n"));
}

static void _MDrv_Scl_PQ_ClearTableIndex(void)
{

	_u8ICC_CRD_Table   = 0xFF;
	_u8IHC_CRD_Table   = 0xFF;
	_u8SRAM1Table  = 0xFF;
	_u8SRAM2Table  = 0xFF;
	_u8SRAM3Table  =0xFF;
	_u8SRAM4Table  = 0xFF;
	_u8CSRAM1Table = 0xFF;
	_u8CSRAM2Table = 0xFF;
	_u8CSRAM3Table = 0xFF;
	_u8CSRAM4Table = 0xFF;
}


static void _MDrv_Scl_PQ_DumpTable(EN_IP_Info* pIP_Info)
{
    // to save loading SRAM table time, SRAM are only downloaded
    // when current SRAM table is different to previous SRAM table
    if (pIP_Info->pIPTable == NULL)
    {
        PQ_DUMP_DBG(sclprintf("NULL Table\r\n"));
        return;
    }
    PQ_DUMP_DBG(sclprintf("Table Type =%x, Index =%x\r\n", (u16)pIP_Info->u8TabType, (u16)pIP_Info->u8TabIdx));
    switch(pIP_Info->u8TabType )
    {

        case PQ_TABTYPE_SCALER:
        case PQ_TABTYPE_NE:
            _MDrv_Scl_PQ_DumpScalerRegTable(pIP_Info);
            break;

#if PQ_ENABLE_UNUSED_FUNC
        case PQ_TABTYPE_COMB:
            _MDrv_Scl_PQ_DumpCombRegTable(pIP_Info);
            break;
#endif

        case PQ_TABTYPE_SRAM1:
            if (_u8SRAM1Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old  sram1: %u, new  sram1: %u\r\n",
                    (u16)_u8SRAM1Table, (u16)pIP_Info->u8TabIdx));
                _u8SRAM1Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  sram1: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;
        case PQ_TABTYPE_SRAM2:
            if (_u8SRAM2Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old  sram2: %u, new  sram2: %u\r\n",
                    (u16)_u8SRAM2Table, (u16)pIP_Info->u8TabIdx));
                _u8SRAM2Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  sram2: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;

        case PQ_TABTYPE_SRAM3:
            if (_u8SRAM3Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old  sram3: %u, new  sram3: %u\r\n",
                    (u16)_u8SRAM3Table, (u16)pIP_Info->u8TabIdx));
                _u8SRAM3Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  sram3: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;

        case PQ_TABTYPE_SRAM4:
            if (_u8SRAM4Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old s ram4: %u, new  sram4: %u\r\n",
                    (u16)_u8SRAM4Table, (u16)pIP_Info->u8TabIdx));
                _u8SRAM4Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the  same sram4: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;


        case PQ_TABTYPE_C_SRAM1:
            if (_u8CSRAM1Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old Csram1: %u, new  Csram1: %u\r\n",
                    (u16)_u8CSRAM1Table, (u16)pIP_Info->u8TabIdx));
                _u8CSRAM1Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  Csram1: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;
        case PQ_TABTYPE_C_SRAM2:
            if (_u8CSRAM2Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old  Csram2: %u, new  Csram2: %u\r\n",
                    (u16)_u8CSRAM2Table, (u16)pIP_Info->u8TabIdx));
                _u8CSRAM2Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  Csram2: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;

        case PQ_TABTYPE_C_SRAM3:
            if (_u8CSRAM3Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old Csram3: %u, new Csram3: %u\r\n",
                    (u16)_u8CSRAM3Table, (u16)pIP_Info->u8TabIdx));
                _u8CSRAM3Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  sram3: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;

        case PQ_TABTYPE_C_SRAM4:
            if (_u8SRAM4Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old  Csram4: %u, new  Csram4: %u\r\n",
                    (u16)_u8CSRAM4Table, (u16)pIP_Info->u8TabIdx));
                _u8CSRAM4Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);
            }
            else
            {
                PQ_DUMP_FILTER_DBG(sclprintf("use the same  Csram4: %u\r\n", (u16)pIP_Info->u8TabIdx));
            }
            break;

        case PQ_TABTYPE_VIP_ICC_CRD_SRAM:
            if( _u8ICC_CRD_Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old ICC_CRD_SRAM: %u, new ICC_CRD_SRAM: %u\r\n",
                    (u16)_u8ICC_CRD_Table, (u16)pIP_Info->u8TabIdx));
                _u8ICC_CRD_Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);

            }
            break;

        case PQ_TABTYPE_VIP_IHC_CRD_SRAM:
            if( _u8IHC_CRD_Table != pIP_Info->u8TabIdx)
            {
                PQ_DUMP_FILTER_DBG(sclprintf("old IHC_CRD_SRAM: %u, new IHC_CRD_SRAM: %u\r\n",
                    (u16)_u8IHC_CRD_Table, (u16)pIP_Info->u8TabIdx));
                _u8IHC_CRD_Table = pIP_Info->u8TabIdx;
                _MDrv_Scl_PQ_DumpFilterTable(pIP_Info);

            }
            break;

        case PQ_TABTYPE_GENERAL:
            _MDrv_Scl_PQ_DumpGeneralRegTable(pIP_Info);
            break;

        case PQ_TABTYPE_PICTURE_1:
        case PQ_TABTYPE_PICTURE_2:
            break;

        default:
            PQ_DUMP_DBG(sclprintf("[PQ]DumpTable:unknown type: %u\r\n", pIP_Info->u8TabType));
            break;
    }
}

// return total IP count
static u16 _MDrv_Scl_PQ_GetIPNum(void)
{
    PQ_DBG(sclprintf("[PQ]IPNum=%u\r\n",_Scl_PQTableInfo.u8PQ_IP_Num));
    return (u16)_Scl_PQTableInfo.u8PQ_IP_Num;
}

// return total table count of given IP
static u16 _MDrv_Scl_PQ_GetTableNum(u8 u8PQIPIdx)
{
    PQ_DBG(sclprintf("[PQ]TabNum=%u\r\n", _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].u8TabNums));
    return (u16)_Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].u8TabNums;
}

// return current used table index of given IP
static u16 _MDrv_Scl_PQ_GetCurrentTableIndex(u8 u8PQIPIdx)
{
    PQ_DBG(sclprintf("[PQ]CurrTableIdx=%u\r\n", _u8SclPQTabIdx[u8PQIPIdx]));
    return (u16)_u8SclPQTabIdx[u8PQIPIdx];
}

static u16 _MDrv_Scl_PQ_GetTableIndex(u16 u16PQSrcType, u8 u8PQIPIdx)
{
    if (u16PQSrcType >=_Scl_PQTableInfo.u8PQ_InputType_Num){
        PQ_DBG(sclprintf("[PQ]invalid input type\r\n"));
        return PQ_IP_NULL;
    }
    if (u8PQIPIdx >= _Scl_PQTableInfo.u8PQ_IP_Num){
        PQ_DBG(sclprintf("[PQ]invalid ip type\r\n"));
        return PQ_IP_NULL;
    }

    PQ_DBG(sclprintf("[PQ]TableIdx=%u\r\n",(u16)_Scl_PQTableInfo.pQuality_Map_Aray[u16PQSrcType * _Scl_PQTableInfo.u8PQ_IP_Num + u8PQIPIdx]));

    return (u16)_Scl_PQTableInfo.pQuality_Map_Aray[u16PQSrcType * _Scl_PQTableInfo.u8PQ_IP_Num + u8PQIPIdx];
}

static EN_IP_Info _MDrv_Scl_PQ_GetTable(u8 u8TabIdx, u8 u8PQIPIdx)
{
    EN_IP_Info ip_Info;
    _u8SclPQTabIdx[u8PQIPIdx] = u8TabIdx;
    if (u8TabIdx != PQ_IP_NULL && u8TabIdx != PQ_IP_COMM) {
        ip_Info.pIPTable  = _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].pIPTable;
        ip_Info.u8TabNums = _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].u8TabNums;
        ip_Info.u8TabType = _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].u8TabType;
        ip_Info.u8TabIdx = u8TabIdx;
    }
    else if (u8TabIdx == PQ_IP_COMM) {
        ip_Info.pIPTable = _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].pIPCommTable;
        ip_Info.u8TabNums = 1;
        ip_Info.u8TabType = _Scl_PQTableInfo.pIPTAB_Info[u8PQIPIdx].u8TabType;
        ip_Info.u8TabIdx = 0;
    }
    else {
        ip_Info.pIPTable  = 0;
        ip_Info.u8TabNums = 0;
        ip_Info.u8TabType = 0;
        ip_Info.u8TabIdx = 0;
    }
    return ip_Info;
}

#if PQ_ENABLE_UNUSED_FUNC

static void _MDrv_Scl_PQ_LoadTableData(u8 u8TabIdx, u8 u8PQIPIdx, u8 *pTable, u16 u16TableSize)
{
    EN_IP_Info ip_Info;
    u32 u32Addr;
    u16 u16Idx = 0;


    ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
    if (ip_Info.u8TabIdx >= ip_Info.u8TabNums){
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: Scaler Reg Table\r\n"));
        return;
    }

    while (1)
    {
        u32Addr = (ip_Info.pIPTable[0]<<8) + ip_Info.pIPTable[1];
        pTable[u16Idx++] = ip_Info.pIPTable[REG_ADDR_SIZE+REG_MASK_SIZE+ip_Info.u8TabIdx];

        if(u16Idx > u16TableSize || u32Addr == _END_OF_TBL_)
            break;

        ip_Info.pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+ip_Info.u8TabNums); // next
    }
}

static void _MDrv_Scl_PQ_LoadTable(u8 u8TabIdx, u8 u8PQIPIdx)
{
    EN_IP_Info ip_Info;
    ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
    _MDrv_Scl_PQ_DumpTable(&ip_Info);
#if(ENABLE_PQ_LOAD_TABLE_INFO)
    _MDrv_Scl_PQ_Set_LoadTableInfo_IP_Tab(u8PQIPIdx, u8TabIdx);
#endif

}

static bool _MDrv_Scl_PQ_LoadPictureSetting(u8 u8TabIdx, u8 u8PQIPIdx, void *pTable, u16 u16TableSize)
{
    EN_IP_Info ip_Info;
    u32 u32Addr;

    ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
    if (ip_Info.u8TabIdx >= ip_Info.u8TabNums)
    {
        PQ_DUMP_DBG(sclprintf("[PQ]IP_Info error: Scaler Reg Table\r\n"));
        return FALSE;
    }

    //sclprintf("TabType:%d TabNums:%d, TabIdx:%d  addr:%x \r\n", ip_Info.u8TabType, ip_Info.u8TabNums, ip_Info.u8TabIdx, ip_Info.pIPTable);

    u32Addr = u16TableSize * ip_Info.u8TabIdx;
    if(ip_Info.u8TabType == PQ_TABTYPE_PICTURE_1)
    {
        u8 *pu8PQTbl =  (u8 *)ip_Info.pIPTable;
        DrvSclOsMemcpy((u8 *)pTable, &pu8PQTbl[u32Addr], sizeof(u8)*u16TableSize);
    }
    else if(ip_Info.u8TabType == PQ_TABTYPE_PICTURE_2)
    {
        u16 *pu16PQTbl =  (u16 *)ip_Info.pIPTable;
        DrvSclOsMemcpy((u16 *)pTable, &pu16PQTbl[u32Addr], sizeof(u16)*u16TableSize);

    }
    else
    {
        return FALSE;
    }

#if 0
{
    u16 u16Idx = 0;
    for(u16Idx=0; u16Idx<u16TableSize; u16Idx++)
    {
        if(ip_Info.u8TabType == PQ_TABTYPE_PICTURE_1)
            sclprintf("Pic:%02d, %02x \n", u16Idx, ((u8 *)pTable)[u16Idx]);
        else
            sclprintf("Pic:%02d, %04x \n", u16Idx, ((u16 *)pTable)[u16Idx]);
    }
}
#endif
    return TRUE;
}

static void _MDrv_Scl_PQ_LoadCommTable(void)
{
    u8 i;
    EN_IP_Info ip_Info;

    PQ_DBG(sclprintf("[PQ]LoadCommTable\r\n"));
    for (i=0; i<_Scl_PQTableInfo.u8PQ_IP_Num; i++)
    {
        PQ_DBG(sclprintf("  IP:%u\r\n", (u16)i));
        ip_Info = _MDrv_Scl_PQ_GetTable(PQ_IP_COMM, i);

    #if (PQTBL_REGTYPE == PQTBL_NORMAL)
        if((ip_Info.u8TabType >= PQ_TABTYPE_SRAM1 && ip_Info.u8TabType <= PQ_TABTYPE_C_SRAM1) ||
            ip_Info.u8TabType == PQ_TABTYPE_VIP_IHC_CRD_SRAM ||
            ip_Info.u8TabType == PQ_TABTYPE_VIP_ICC_CRD_SRAM)
        {
            continue;
        }
    #endif
        _MDrv_Scl_PQ_DumpTable(&ip_Info);
    }
}


static u8 _MDrv_Scl_PQ_GetXRuleTableIndex(u8 u8XRuleType, u8 u8XRuleIdx, u8 u8XRuleIP)
{
    u8 *pArray = _Scl_PQTableInfo.pXRule_Array[u8XRuleType];
    return pArray[((u16)u8XRuleIdx) * _Scl_PQTableInfo.u8PQ_XRule_IP_Num[u8XRuleType] + u8XRuleIP];
}

static u8 _MDrv_Scl_PQ_GetXRuleIPIndex(u8 u8XRuleType, u8 u8XRuleIP)
{
    u8 *pArray = _Scl_PQTableInfo.pXRule_IP_Index[u8XRuleType];
    return pArray[u8XRuleIP];
}

static u8 _MDrv_Scl_PQ_GetXRuleIPNum(u8 u8XRuleType)
{
    return _Scl_PQTableInfo.u8PQ_XRule_IP_Num[u8XRuleType];
}

static u8 _MDrv_Scl_PQ_GetGRule_LevelIndex(u8 u8GRuleType, u8 u8GRuleLvlIndex)
{
    u8 *pArray = _Scl_PQTableInfo.pGRule_Level[u8GRuleType];
    return pArray[u8GRuleLvlIndex];
}

static u8 _MDrv_Scl_PQ_GetGRule_IPIndex(u8 u8GRuleType, u8 u8GRuleIPIndex)
{
     u8 *pArray = _Scl_PQTableInfo.pGRule_IP_Index[u8GRuleType];
    return pArray[u8GRuleIPIndex];
}

static u8 _MDrv_Scl_PQ_GetGRule_TableIndex(u8 u8GRuleType, u8 u8PQSrcType, u8 u8PQ_NRIdx, u8 u8GRuleIPIndex)
{
//    return _Scl_PQTableInfo.pGRule_Array[u8PQSrcType][u8PQ_NRIdx][u8GRuleIPIndex];
    u16 u16index;
    u8 *pArray = _Scl_PQTableInfo.pGRule_Array[u8GRuleType];

    u16index = ((u16)u8PQSrcType) * _Scl_PQTableInfo.u8PQ_GRule_Num[u8GRuleType] * _Scl_PQTableInfo.u8PQ_GRule_IPNum[u8GRuleType] +
               ((u16)u8PQ_NRIdx) * _Scl_PQTableInfo.u8PQ_GRule_IPNum[u8GRuleType] +
               u8GRuleIPIndex;
    return pArray[u16index];
}
#endif

static void _MDrv_Scl_PQ_Set_CmdqCfg(PQ_CMDQ_CONFIG CmdqCfg)
{
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]%s %d:: En=%d, \n",__FUNCTION__, __LINE__, CmdqCfg.bEnFmCnt);
}
static u16 _MDrv_Scl_PQ_GetIPRegCount(u16 u16PQSrcType,u8 u8PQIPIdx)
{
    EN_IP_Info ip_Info;
    u8 u8TabIdx;
    u32 u32Addr;
    u16 u16DataIdx = 0;

    u8TabIdx = (u8)_MDrv_Scl_PQ_GetTableIndex(u16PQSrcType, u8PQIPIdx);

    ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]%s %d::SrcType=%d, IPIdx=%d, TabType=%d\r\n"
        ,__FUNCTION__, __LINE__, u16PQSrcType, u8PQIPIdx, ip_Info.u8TabType);
    while (1)
    {
        u32Addr = (ip_Info.pIPTable[0]<<8) + ip_Info.pIPTable[1];
        if(u32Addr == _END_OF_TBL_) // check end of table
        {
            break;
        }
        ip_Info.pIPTable+=(REG_ADDR_SIZE+REG_MASK_SIZE+ip_Info.u8TabNums); // next
        u16DataIdx++;
    }
    return u16DataIdx;
}
static void _MDrv_Scl_PQ_Check_Type(PQ_CHECK_TYPE EnCheck)
{
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]%s %d:: En=%d\n", __FUNCTION__, __LINE__, EnCheck);
    if(EnCheck == PQ_CHECK_REG)
    {
        _bcheckReg = 1;
    }
    else if(EnCheck == PQ_CHECK_SIZE)
    {
        _bcheckSclPQsize= 1;
    }
    else if(EnCheck == PQ_CHECK_OFF)
    {
        _bcheckReg = 0;
        _bcheckSclPQsize= 0;
    }
}

static void _MDrv_Scl_PQ_SetPQDebugFlag(u8 u8PQIPIdx)
{
    switch (u8PQIPIdx)
    {
        case PQ_IP_MCNR_Main ... PQ_IP_NLM_Main:
        case PQ_IP_XNR_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_BEFORECROP;
            break;

        case PQ_IP_ColorEng_422to444_Main ... PQ_IP_ColorEng_444to422_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_COLORENG;
            break;
        case PQ_IP_VIP_HLPF_Main ... PQ_IP_VIP_UVC_Main:
        case PQ_IP_VIP_ACK_Main ... PQ_IP_VIP_YCbCr_Clip_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_VIPY;
            break;
        case PQ_IP_VIP_FCC_full_range_Main ... PQ_IP_VIP_IBC_SETTING_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_VIPC;
            break;

        case PQ_IP_YEE_Main ... PQ_IP_UV_ADJUST_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_AIP;
            break;

        case PQ_IP_PFC_Main ... PQ_IP_YUV_Gamma_Main:
            gu8debugflag = EN_DBGMG_PQLEVEL_AIPPOST;
            break;
        default:
            gu8debugflag = EN_DBGMG_PQLEVEL_ELSE;
            break;
    }

}

static void _MDrv_Scl_PQ_LoadTableByData(u16 u16PQSrcType, u8 u8PQIPIdx, PQ_DATA_INFO *pData)
{
    EN_IP_Info ip_Info;
    u8 u8TabIdx = 0;

    u8TabIdx = (u8)_MDrv_Scl_PQ_GetTableIndex(u16PQSrcType, u8PQIPIdx);
    _MDrv_Scl_PQ_SetPQDebugFlag(u8PQIPIdx);
    ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
    SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]%s %d::SrcType=%d, IPIdx=%d, TabType=%d\r\n"
        ,__FUNCTION__, __LINE__, u16PQSrcType, u8PQIPIdx, ip_Info.u8TabType);


    switch(ip_Info.u8TabType)
    {
        case PQ_TABTYPE_SCALER:
        case PQ_TABTYPE_NE:
            _MDrv_Scl_PQ_DumpSclaerRegTableByData(&ip_Info, pData);
            break;

        default:
            sclprintf("[PQ]%s %d: Unsupport TabType:%d\n", __FUNCTION__, __LINE__, ip_Info.u8TabType);
            break;
    }
}


static void _MDrv_Scl_PQ_LoadTableBySrcType(u16 u16PQSrcType, u8 u8PQIPIdx)
{
    EN_IP_Info ip_Info;
    u8 QMIPtype_size,i;
    u8 u8TabIdx = 0;
    //XC_ApiStatusEx stXCStatusEx; Ryan

    if (u8PQIPIdx==PQ_IP_ALL)
    {
        QMIPtype_size=_Scl_PQTableInfo.u8PQ_IP_Num;
        u8PQIPIdx=0;
    }
    else
    {
        QMIPtype_size=1;
    }

    //for debug
    //msAPI_Scaler_SetBlueScreen(DISABLE, 0x00);
    //MApi_XC_GenerateBlackVideo(FALSE);

    for(i=0; i<QMIPtype_size; i++, u8PQIPIdx++)
    {
        if (_Scl_PQTableInfo.pSkipRuleIP[u8PQIPIdx]) {

            SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]SrcType=%u, IPIdx=%u, TabIdx=%u\r\n"
                ,(u16)u16PQSrcType, (u16)u8PQIPIdx, (u16)u8TabIdx);
            PQ_DBG(sclprintf("skip ip idx:%u\r\n", u8PQIPIdx));
            continue;
        }

        #if 0 //Ryan
        DrvSclOsMemset(&stXCStatusEx, 0, sizeof(XC_ApiStatusEx));
        stXCStatusEx.u16ApiStatusEX_Length = sizeof(XC_ApiStatusEx);
        stXCStatusEx.u32ApiStatusEx_Version = API_STATUS_EX_VERSION;

        if(MApi_XC_GetStatusEx(&stXCStatusEx, MAIN_WINDOW) == stXCStatusEx.u16ApiStatusEX_Length)
        {
            //if customer scaling, and now it's going to dump HSD_Y and HSD_C, just skip it.
            if((!stXCStatusEx.bPQSetHSD) &&
                ((u8PQIPIdx == PQ_IP_HSD_C_Main) || (u8PQIPIdx == PQ_IP_HSD_Y_Main)))
            {
                PQ_DBG(sclprintf("skip ip idx:%u because of customer scaling case\r\n", u8PQIPIdx));
                continue;
            }
        }
        #endif

        u8TabIdx = (u8)_MDrv_Scl_PQ_GetTableIndex(u16PQSrcType, u8PQIPIdx);
        SCL_DBG(SCL_DBG_LV_DRVPQ() &gu8debugflag, "[PQ]SrcType=%u, IPIdx=%u, TabIdx=%u\r\n"
            ,(u16)u16PQSrcType, (u16)u8PQIPIdx, (u16)u8TabIdx);

        ip_Info = _MDrv_Scl_PQ_GetTable(u8TabIdx, u8PQIPIdx);
        if(ip_Info.u8TabType == PQ_TABTYPE_SCALER ||ip_Info.u8TabType == PQ_TABTYPE_NE)//paul
        {
            _MDrv_Scl_PQ_DumpTable(&ip_Info);
        }
#if(ENABLE_PQ_LOAD_TABLE_INFO)
        _MDrv_Scl_PQ_Set_LoadTableInfo_IP_Tab(u8PQIPIdx, u8TabIdx);
#endif

        //DrvSclOsDelayTask(1500);
    }
}

static void _MDrv_Scl_PQ_AddTable(PQTABLE_INFO *pPQTableInfo)
{
    _Scl_PQTableInfo = *pPQTableInfo;
}

#if PQ_ENABLE_UNUSED_FUNC

static void _MDrv_Scl_PQ_CheckCommTable(void)
{
#if (PQ_ENABLE_CHECK == 1)
    u8PQfunction = PQ_FUNC_CHK_REG;
    _MDrv_Scl_PQ_LoadCommTable();
    u8PQfunction = PQ_FUNC_DUMP_REG;
#endif
}

static void _MDrv_Scl_PQ_CheckTableBySrcType(u16 u16PQSrcType, u8 u8PQIPIdx)
{
#if (PQ_ENABLE_CHECK == 1)
    u8PQfunction = PQ_FUNC_CHK_REG;
    _MDrv_Scl_PQ_LoadTableBySrcType(u16PQSrcType, u8PQIPIdx);
    u8PQfunction = PQ_FUNC_DUMP_REG;
#else
    UNUSED(u16PQSrcType);
    UNUSED(u8PQIPIdx);
#endif
}
#endif

INSTALL_PQ_FUNCTIONS(PQTABLE_NAME)
#endif

#undef _DRVPQ_TEMPLATE_H_
#endif /* _DRVPQ_TEMPLATE_H_ */
