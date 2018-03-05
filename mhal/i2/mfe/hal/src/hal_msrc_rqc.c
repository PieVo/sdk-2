
#include "hal_mfe_ops.h"
#include "hal_mfe_utl.h"
#include <linux/limits.h>   //derek check
#include <linux/module.h>
#include "hal_msrc_rqc.h"
#include "hal_mfe_msmath.h"
#include "hal_mfe_global.h"


/************************************************************************/
/* Version Declaration                                                  */
/************************************************************************/

#define MSRC_NAME               "MSRC"
#define MSRC_VER_MJR            1
#define MSRC_VER_MNR            1
#define MSRC_VER_EXT            01
#define _EXP(expr)              #expr
#define _STR(expr)              _EXP(expr)

/************************************************************************/
/* Macros                                                               */
/************************************************************************/

#define UP_QP_DIFF_LIMIT        3
#define LF_QP_DIFF_LIMIT        3

#define LOGOFF_DEFAULT          1
#define LOWERQ_DEFAULT          12
#define UPPERQ_DEFAULT          48
#define BOUNDQ_DEFAULT          -1

// MFE HW limit
#define MIN_QP                      5
#define MAX_QP                      48

#define ALIGN2CB(l) (((l)+15)>>4)

static inline int imin(int a, int b)
{
    return ((a) < (b)) ? (a) : (b);
}

static inline int imax(int a, int b)
{
    return ((a) > (b)) ? (a) : (b);
}

static inline int iabs(int a)
{
    return ((a) < 0) ? -(a) : (a);
}

static inline int iClip3(int low, int high, int x)
{
    x = imax(x, low);
    x = imin(x, high);
    return x;
}

static inline uint32 uimin(uint32 a, uint32 b)
{
    return ((a) < (b)) ? (a) : (b);
}

static inline uint32 uimax(uint32 a, uint32 b)
{
    return ((a) > (b)) ? (a) : (b);
}

static inline uint32 uiClip3(uint32 low, uint32 high, uint32 x)
{
    x = uimax(x, low);
    x = uimin(x, high);
    return x;
}

/************************************************************************/
/* Configuration                                                        */
/************************************************************************/

/************************************************************************/
/* Constant                                                             */
/************************************************************************/

// P-frame min, max QP
static uint32 g_uiRcPicMinQp = MIN_QP;
static uint32 g_uiRcPicMaxQp = MAX_QP;
// I-frame min, max QP
static uint32 g_uiRcIPicMinQp = MIN_QP;
static uint32 g_uiRcIPicMaxQp = MAX_QP;

// Picture delta Qp range
static uint32 g_uiRcPicClipRange = 3; // default

static int g_iRcStatsTime = 1;

// min/max frame bit ratio relative to avg bitrate
static int g_iRcMaxIPicBitRatio = 60; // default
static int g_iRcMinIPicBitRatio = 1; // default
static int g_iRcMaxPicBitRatio = 10; // default
static int g_iRcMinPicBitRatio = 0; // default

static int g_iHWTextCplxAccum;

/************************************************************************/
/* Local structures                                                     */
/************************************************************************/

#define PICTYPES 2

typedef struct MsrcRqc_t
{
    mfe6_rqc    rqcx;
    int     i_method;           /* Rate Control Method */
    int     i_btrate;           /* Bitrate */
    int     i_btratio;          /* Bitrate Ratio (50 ~ 100) */
    int     i_levelq;           /* Frame Level QP */
    int     i_deltaq;           /* QP Delta Value */
    int     n_fmrate;           /* Frame Rate Numerator */
    int     d_fmrate;           /* Frame Rate Denominator */
    int     i_fmrate;           /* Frame Rate */
    int     i_pixels;           /* Picture Pixel Count */
    int     i_blocks;           /* Picture CTU Count */
    short   b_mbkadp, i_limitq;
    short   i_frmdqp, i_blkdqp;
    short   i_iupperq, i_ilowerq;
    short   i_pupperq, i_plowerq;
    int     i_gopbit;           /* Target GOP Bit Count */
    int     i_frmbit;           /* Target Frame Bit Count */
    int     i_pixbit;           /* Target Pixel Bit Count */
    int     i_budget;           /* Frame Budget Bit Count */
    int     i_ipbias;
    int     i_smooth;
    int     i_bucket;
    int     i_upperb, i_lowerb;
    int     i_margin;
    int     i_radius;
    int     i_degree;
    int     i_errpro;
    int     i_imbase;
    int     i_imbits;           // bit-pos:imaginary
    int     i_rebits;           // bit-pos:real

    iir_t   iir_rqprod[PICTYPES];
    acc_t   acc_rqprod;
    acc_t   acc_bitcnt;
    int     i_intrabit;

    ms_rc_top msrc_top;         /* MSRC top Attribute */

} msrc_rqc;

/************************************************************************/
/* Local prototypes                                                     */
/************************************************************************/

char* MsrcDescribe(void);

static int  _MsrcSeqSync(rqct_ops*);
static int  _MsrcSeqDone(rqct_ops*);
static int  _MsrcSeqConf(rqct_ops*);
static int  _MsrcEncConf(rqct_ops*, mhve_job*);
static int  _MsrcEncDone(rqct_ops*, mhve_job*);

static void _MsrcOpsFree(rqct_ops* rqct)
{
    MEM_FREE(rqct);
}

//--------------------------------------
static void _calcAvgQpFromHist(ms_rc_top* ptMsRc);
static void _EstModelParas(ms_rc_top* ptMsRc, int windowSize, int *pbRejected);
static uint32 _CalcIPicQp(ms_rc_top* ptMsRc);
static uint32 _CalcPPicQp(ms_rc_top* ptMsRc, int picBit);
static uint32 _EstPicQpByBits(ms_rc_top* ptMsRc, int targetBit);
static uint64 _fSqrt_UInt64(uint64 X);
static void _msrc_pic_start(ms_rc_top* ptMsRc, int picType);
static void _msrc_gop_start(ms_rc_top* ptMsRc, msrc_rqc* pMsRc);
static void _msrc_update_stats(ms_rc_top* ptMsRc, int picEncBit);
static void _msrc_update_model(ms_rc_top* ptMsRc, int picActualBit, int picType);

///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

#if 0
static void print_hex(char* title, void* buf, int num)
{
    int i;
    char *data = (char *) buf;

    CamOsPrintf(
        "%s\nOffset(h)  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
        "----------------------------------------------------------",
        title);
    for(i = 0; i < num; i++)
    {
        if(i % 16 == 0)
            CamOsPrintf("\n%08X   ", i);
        CamOsPrintf("%02X ", data[i]);
    }
    CamOsPrintf("\n");
}
#endif

#if 1
//------------------------------------------------------------------------------
//  Function    : ROISetMapValue
//  Description :
//------------------------------------------------------------------------------
static void _ROISetMapValue(char value, char mode, char *map, struct roirec* rec, int ihorzctbnum, int ivertctbnum, int ctbnum)
{
    char *pRoiMap, *pRoiMapHor;
    int startX, startY, width, height;
    int stride = ihorzctbnum * 2; //1 MB need 2 bytes
    int x, y;

    startX = rec->i_posx;
    startY = rec->i_posy;
    width = rec->i_recw;
    height = rec->i_rech;

    //CamOsPrintf("[%s %d] (%d, %d, %d, %d) (%d %d %d) \n", __FUNCTION__, __LINE__, startX, startY, width, height, ihorzctbnum, ivertctbnum, ctbnum);
    //mode = 7;

    pRoiMap = &map[startY * stride + startX * 2];

    for(y = 0; y < height; y++)
    {
        pRoiMapHor = pRoiMap;
        for(x = 0; x < width; x++)
        {
            *pRoiMapHor = value & 0xFF;
            pRoiMapHor++;
            *pRoiMapHor = mode & 0xFF;
            pRoiMapHor++;
        }
        pRoiMap += stride;
    }
}

//------------------------------------------------------------------------------
//  Function    : _draw
//  Description :
//------------------------------------------------------------------------------

static int _draw(rqct_att* attr)
{

    int i = 0, enable = 0;
    char* qpmap = (char*)attr->p_dqmkptr;
    int ctbw = attr->i_dqmw;
    int ctbh = attr->i_dqmh;
    int ctbn = ctbw * ctbh;
    struct roirec* roi = attr->m_roirec;
    signed char* roidqp = attr->i_roiqp;
    struct roirec* rec;
    signed char qpoffset, iMapValue;
    int iMapId;
    int ctbforcemode;
    // ROI feature id
#define ROI_QP_MAP_ID    0
#define ROI_INTRA_MAP_ID 1
#define ROI_ZMV_MAP_ID   2

    for(i = 0; i < RQCT_ROI_NR; i++)
    {
        rec = &roi[i];
        qpoffset = roidqp[i];

        if(qpoffset != 0)
        {
            switch(iMapId)
            {
                case ROI_QP_MAP_ID:
                    //iMapValue = qpoffset;
                    if(i < 7)
                        iMapValue = i - 7;
                    if(i == 7)
                        iMapValue = 1;
                    ctbforcemode = 0;
                    break;
                case ROI_INTRA_MAP_ID:
                    iMapValue = 1;
                    ctbforcemode = 5;
                    break;
                case ROI_ZMV_MAP_ID:
                    iMapValue = 1;
                    ctbforcemode = 3;
                    break;
            }

            _ROISetMapValue(iMapValue, ctbforcemode, qpmap, rec, ctbw, ctbh, ctbn);

            enable = 1;
        }
    }

    return enable;
}

//------------------------------------------------------------------------------
//  Function    : MheMrqcRoiDraw
//  Description :
//------------------------------------------------------------------------------
int MfeMrqcRoiDraw(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;

    if(0 > rqcx->attr.b_dqmstat)
    {
        /* Edit qmap and enable it */

        //Initial ROI MAP
        MEM_SETV((char*)rqcx->attr.p_dqmkptr, 0, rqcx->attr.i_dqmsize);

        if(_draw(&rqcx->attr))
        {
            regs->regf2_g_roi_en = 1;
            regs->regf2_g_roi_range_sw_limit = 0;
            regs->reg10c_mbr_lut_roi_on = 1;
            regs->regf2_g_zeromv_en = 0;
            //regs->regf2_g_i16pln_en = 0;

            if(regs->reg10c_mbr_lut_roi_on)
            {
                regs->regfb_g_roi_qmap_adr_lo = (((u32)rqcx->attr.u_dqmphys) >> 8) & 0xFFFF;
                regs->regfc_g_roi_qmap_adr_hi = ((u32)rqcx->attr.u_dqmphys) >> (8 + 16);

                regs->reg114_mbr_gn_read_map_st_addr_low = (((u32)rqcx->attr.u_dqmphys) >> 4) & 0x0FFF;
                regs->reg115_mbr_gn_read_map_st_addr_high = ((u32)rqcx->attr.u_dqmphys) >> 16;
            }

            if(regs->regf2_g_zeromv_en)
            {
                regs->reg86_g_zmvmap_base_lo = (((u32)rqcx->attr.u_dqmphys) >> 8) & 0xFFFF;
                regs->reg87_g_zmvmap_base_hi = ((u32)rqcx->attr.u_dqmphys) >> (8 + 16);
            }

            //print_hex("QP MAP", rqcx->attr.p_dqmkptr, rqcx->attr.i_dqmsize);

            rqcx->attr.b_dqmstat = 1;
        }
        else
        {
            regs->regf2_g_roi_en = 0;
            regs->reg10c_mbr_lut_roi_on = 0;
            regs->regf2_g_zeromv_en = 0;

            rqcx->attr.b_dqmstat = 0;
        }
    }

    return (rqcx->attr.b_dqmstat);
}
#else
//------------------------------------------------------------------------------
//  Function    : _draw
//  Description :
//------------------------------------------------------------------------------
static int _draw(rqct_att* attr)
{
    int i = 0, enable = 0;
    char*  tbl = (char*)attr->p_dqmkptr + 1;
    uchar* idc = (uchar*)attr->p_dqmkptr + 16;
    int mbw = attr->i_dqmw;
    struct roirec* roi = attr->m_roirec;

    while(i < RQCT_ROI_NR)
    {
        struct  roirec* rec = roi++;
        uchar   h, l;
        int     w, j, k, m, n;

        tbl[i] = attr->i_roidqp[i];
        if(tbl[i++] == 0)
            continue;

        enable++;
        n = rec->i_rech;
        j = rec->i_posy * mbw + rec->i_posx;
        k = rec->i_recw + j;
        l = (uchar)i;
        h = l << 4;

        for(m = 0; m < n; m++)
        {
            uchar* p = idc + (j >> 1), q;
            q = *p;
            w = j;

            /* Update QP map for one row */
            do
            {
                switch(w & 1)
                {
                    case 0:
                        if(!(q & 0x0F))  q += l;  // EROY CHECK
                        break;
                    case 1:
                        if(!(q & 0xF0))  q += h;  // EROY CHECK
                        *p++ = q;
                        q = *p;
                        break;
                }
            }
            while(++w < k);

            *p++ = q;
            j += mbw;
            k += mbw;
        }
    }

    idc[0] &= 0xF0;

    return enable;
}

//------------------------------------------------------------------------------
//  Function    : MrqcRoiDraw
//  Description :
//------------------------------------------------------------------------------
int MfeMrqcRoiDraw(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;

    if(0 > rqcx->attr.b_dqmstat)
    {
        /* Edit qmap and enable it */
        MEM_SETV((char*)rqcx->attr.p_dqmkptr + 16, 0, rqcx->attr.i_dqmsize - 16);

        if(_draw(&rqcx->attr))
        {
            regs->regf2_g_roi_en = 1;
            regs->reg10c_mbr_lut_roi_on = 1;
            regs->regf2_g_zeromv_en = 0;

            regs->regfb_g_roi_qmap_adr_lo = (((u32)rqcx->attr.u_dqmphys) >> 8) & 0xFFFF;
            regs->regfc_g_roi_qmap_adr_hi = ((u32)rqcx->attr.u_dqmphys) >> (8 + 16);

            //print_hex("QP MAP", rqcx->attr.p_dqmkptr, rqcx->attr.i_dqmsize);
            rqcx->attr.b_dqmstat = 1;
        }
        else
        {
            regs->regf2_g_roi_en = 0;
            regs->reg10c_mbr_lut_roi_on = 1;
            regs->regf2_g_zeromv_en = 0;
            regs->regfb_g_roi_qmap_adr_lo = 0;
            regs->regfc_g_roi_qmap_adr_hi = 0;
            rqcx->attr.b_dqmstat = 0;
        }
    }
    return (rqcx->attr.b_dqmstat);
}
#endif
/************************************************************************/
/* Functions                                                            */
/************************************************************************/

//------------------------------------------------------------------------------
//  Function    : MsrcAllocate
//  Description :
//------------------------------------------------------------------------------
void* MsrcAllocate(void)
{

    rqct_ops* rqct = NULL;
    mfe6_rqc* rqcx;
    msrc_rqc* msrc;

    if(!(rqct = MEM_ALLC(sizeof(msrc_rqc))))
        return NULL;

    MEM_COPY(rqct->name, MSRC_NAME, 5);

    /* Link member function */
    rqct->release  = _MsrcOpsFree;
    rqct->seq_sync = _MsrcSeqSync;
    rqct->seq_done = _MsrcSeqDone;
    rqct->set_rqcf = MrqcSetRqcf;
    rqct->get_rqcf = MrqcGetRqcf;
    rqct->seq_conf = _MsrcSeqConf;
    rqct->enc_buff = MrqcEncBuff;
    rqct->enc_conf = _MsrcEncConf;
    rqct->enc_done = _MsrcEncDone;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;

    /* Initialize Basic RC Attribute */
    rqcx = (mfe6_rqc*)rqct;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.i_method = RQCT_MODE_CQP;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_leadqp = -1;
    rqcx->attr.i_deltaq = 0;    //QP_IFRAME_DELTA;
    rqcx->attr.i_iupperq = QP_UPPER;
    rqcx->attr.i_ilowerq = QP_LOWER;
    rqcx->attr.i_pupperq = QP_UPPER;
    rqcx->attr.i_plowerq = QP_LOWER;
    rqcx->attr.n_fmrate = 30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_period = 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;

    /* Initialize Penalties */
    rqcx->attr.b_i16pln = 1;
    rqcx->attr.i_peni4x = 0;
    rqcx->attr.i_peni16 = 0;
    rqcx->attr.i_penint = 0;
    rqcx->attr.i_penYpl = 0;
    rqcx->attr.i_penCpl = 0;
    rqcx->i_config = 0;
    rqcx->i_pcount = 0;
    rqcx->i_period = 0;
    rqcx->b_passiveI = 0;
    rqcx->b_seqhead = 1;

    msrc = (msrc_rqc*)rqcx;
    msrc->i_levelq = 36;

    CamOsPrintf("%s\n", MsrcDescribe());

    return rqct;
}

//------------------------------------------------------------------------------
//  Function    : MsrcDescribe
//  Description :
//------------------------------------------------------------------------------
char* MsrcDescribe(void)
{
    static char line[64];
    sprintf(line, "%s@v%d.%d-%02d:r&d analysis.", MSRC_NAME, MSRC_VER_MJR, MSRC_VER_MNR, MSRC_VER_EXT);
    return line;
}



//------------------------------------------------------------------------------
//  Function    : _MsrcSeqDone
//  Description :
//------------------------------------------------------------------------------
static int _MsrcSeqDone(rqct_ops* rqct)
{
    return 0;
}
//------------------------------------------------------------------------------
//  Function    : _MsrcSeqSync
//  Description :
//------------------------------------------------------------------------------
static int _MsrcSeqSync(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;

    rqcx->i_pcount = 0;
    rqcx->b_seqhead = 1;

    return 0;
}

static int _MsrcSeqConf(rqct_ops* rqct)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;
    uint64 uiremainder;
    ms_rc_pic *ptRcPic;

    msrc->i_method = (int)rqcx->attr.i_method;

    if(msrc->i_method == RQCT_MODE_VBR)
    {
        msrc->i_btratio = 80;//(int) rqcx->attr.i_btratio;
        msrc->i_iupperq = (int) rqcx->attr.i_iupperq;
        msrc->i_ilowerq = (int) rqcx->attr.i_ilowerq;
        msrc->i_pupperq = (int) rqcx->attr.i_pupperq;
        msrc->i_plowerq = (int) rqcx->attr.i_plowerq;

        g_uiRcPicMaxQp = msrc->i_pupperq;
        g_uiRcPicMinQp = msrc->i_plowerq;
        g_uiRcIPicMaxQp = msrc->i_iupperq;
        g_uiRcIPicMinQp = msrc->i_ilowerq;
    }

    memset((void *)&msrc->msrc_top, 0, sizeof(ms_rc_top));

    msrc->msrc_top.uiTargetBitrate = (uint32)rqcx->attr.i_btrate;
    msrc->n_fmrate = (int)rqcx->attr.n_fmrate;
    msrc->d_fmrate = (int)rqcx->attr.d_fmrate;
    CDBZ(msrc->n_fmrate);
    CDBZ(msrc->d_fmrate);

    msrc->msrc_top.uiFps = (msrc->n_fmrate + (msrc->d_fmrate >> 1)) / msrc->d_fmrate; // rounding to integer
    rqcx->i_pcount = rqcx->i_period = (int)rqcx->attr.i_period;
    rqcx->b_seqhead = 1;
    msrc->msrc_top.uiGopSize = (uint32)rqcx->i_period;
    msrc->msrc_top.uiPicPixelNum = (int)rqcx->attr.i_pict_w * rqcx->attr.i_pict_h;
    CDBZ(msrc->msrc_top.uiFps);

    msrc->i_frmbit = (int)CamOsMathDivU64((uint64)msrc->msrc_top.uiTargetBitrate * msrc->d_fmrate, msrc->n_fmrate, &uiremainder);
    msrc->msrc_top.iPicAvgBit = msrc->i_frmbit;
    msrc->msrc_top.uiPicCtuNum = ALIGN2CB(rqcx->attr.i_pict_w) * ALIGN2CB(rqcx->attr.i_pict_h);
    msrc->msrc_top.uiIntraMdlCplx = msrc->msrc_top.uiPicPixelNum * 10;

    if(rqcx->attr.i_leadqp > QP_MIN && rqcx->attr.i_leadqp < QP_MAX)
        msrc->i_levelq = (int)rqcx->attr.i_leadqp;

    msrc->msrc_top.uiInitialQp = (uint32)msrc->i_levelq;

    ptRcPic = &msrc->msrc_top.tRcPic;
    ptRcPic->uiPicQp = 0;

    msrc->msrc_top.uiMaxPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMaxPicBitRatio;
    msrc->msrc_top.uiMinPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMinPicBitRatio;
    msrc->msrc_top.uiMaxIPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMaxIPicBitRatio;
    msrc->msrc_top.uiMinIPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMinIPicBitRatio;

    //==============================

    MFE_MSG(MFE_MSG_DEBUG, "uiInitialQp = %d\n", msrc->msrc_top.uiInitialQp);
    MFE_MSG(MFE_MSG_DEBUG, "uiTargetBitrate = %d\n", msrc->msrc_top.uiTargetBitrate);
    MFE_MSG(MFE_MSG_DEBUG, "uiFps = %d\n", msrc->msrc_top.uiFps);
    MFE_MSG(MFE_MSG_DEBUG, "uiGopSize = %d\n", msrc->msrc_top.uiGopSize);
    MFE_MSG(MFE_MSG_DEBUG, "uiPicPixelNum = %d\n", msrc->msrc_top.uiPicPixelNum);
    MFE_MSG(MFE_MSG_DEBUG, "iPicAvgBit = %d\n", msrc->msrc_top.iPicAvgBit);
    MFE_MSG(MFE_MSG_DEBUG, "uiPicCtuNum = %d\n", msrc->msrc_top.uiPicCtuNum);


    if(!msrc->msrc_top.uiInitialQp)
        MFE_MSG(MFE_MSG_WARNING, "uiInitialQp is %d !!\n", msrc->msrc_top.uiInitialQp);
    if(!msrc->msrc_top.uiTargetBitrate)
        MFE_MSG(MFE_MSG_WARNING, "uiTargetBitrate is %d !!\n", msrc->msrc_top.uiTargetBitrate);
    if(!msrc->msrc_top.uiFps)
        MFE_MSG(MFE_MSG_WARNING, "uiFps is %d !!\n", msrc->msrc_top.uiFps);
    if(!msrc->msrc_top.uiGopSize)
        MFE_MSG(MFE_MSG_WARNING, "uiGopSize is %d !!\n", msrc->msrc_top.uiGopSize);
    if(!msrc->msrc_top.uiPicPixelNum)
        MFE_MSG(MFE_MSG_WARNING, "uiPicPixelNum is %d !!\n", msrc->msrc_top.uiPicPixelNum);
    if(!msrc->msrc_top.iPicAvgBit)
        MFE_MSG(MFE_MSG_WARNING, "iPicAvgBit is %d !!\n", msrc->msrc_top.iPicAvgBit);
    if(!msrc->msrc_top.uiPicCtuNum)
        MFE_MSG(MFE_MSG_WARNING, "uiPicCtuNum is %d !!\n", msrc->msrc_top.uiPicCtuNum);

    return 0;
}

static int _MsrcEncConf(rqct_ops* rqct, mhve_job* mjob)
{

    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;
    ms_rc_top* ptMsRc = &msrc->msrc_top;

    MfeMrqcRoiDraw(rqct, mjob);

    switch(msrc->i_method)
    {
        case RQCT_MODE_CQP:
            rqct->i_enc_qp = msrc->i_levelq;
            if(IS_IPIC(rqct->i_pictyp))
                rqct->i_enc_qp -= msrc->i_deltaq;
            break;
        case RQCT_MODE_CBR:
        case RQCT_MODE_VBR:
            //CamOsPrintf("[%s %d] rqct->i_pictyp = %d\n", __FUNCTION__, __LINE__,rqct->i_pictyp);

            if(IS_IPIC(rqct->i_pictyp))
            {
                _msrc_gop_start(ptMsRc, msrc);
            }

            _msrc_pic_start(ptMsRc, rqct->i_pictyp);

            rqct->i_enc_qp = ptMsRc->tRcPic.uiPicQp;

            break;
        default:
            break;
    }

#define UP_DQP  3
#define LF_DQP  3

    /* Initialize Current Frame RC Config */
    regs->reg00_g_mbr_en = 0;
    regs->reg26_s_mbr_pqp_dlimit = UP_DQP;
    regs->reg26_s_mbr_uqp_dlimit = LF_DQP;
    regs->reg00_g_qscale = rqct->i_enc_qp;
    regs->reg27_s_mbr_frame_qstep = 0;
    regs->reg26_s_mbr_tmb_bits = 0;
    regs->reg2a_s_mbr_qp_min = 0;
    regs->reg2a_s_mbr_qp_max = 0;
    regs->reg6e_s_mbr_qstep_min = 0;
    regs->reg6f_s_mbr_qstep_max = 0;

    /* Initialize Penalties of MB */
    regs->regf2_g_i16pln_en = (0 != rqcx->attr.b_i16pln);
    regs->reg87_g_intra4_penalty = rqcx->attr.i_peni4x;
    regs->reg88_g_intra16_penalty = rqcx->attr.i_peni16;
    regs->reg88_g_inter_penalty = rqcx->attr.i_penint;
    regs->reg89_g_planar_penalty_luma = rqcx->attr.i_penYpl;
    regs->reg89_g_planar_penalty_cbcr = rqcx->attr.i_penCpl;

    return 0;
}


#ifdef SIMULATE_ON_I3
int g_aiHWLutIdcHist_test[15 * 10] =
{
    0, 0, 0, 0, 0, 0, 0, 396, 0, 0, 0, 0, 0, 0, 0,
    4, 41, 30, 27, 32, 22, 34, 33, 27, 18, 28, 26, 28, 21, 25,
    5, 45, 26, 28, 30, 22, 35, 22, 27, 33, 21, 24, 25, 26, 27,
    6, 42, 29, 30, 25, 33, 28, 31, 22, 22, 34, 21, 22, 23, 28,
    11, 39, 29, 24, 29, 34, 21, 35, 20, 25, 16, 29, 28, 19, 37,
    8, 47, 26, 41, 29, 28, 26, 20, 30, 24, 22, 21, 28, 18, 28,
    5, 42, 25, 33, 45, 17, 26, 26, 24, 29, 25, 23, 23, 24, 29,
    8, 43, 28, 25, 31, 26, 39, 31, 19, 18, 30, 23, 22, 23, 30,
    8, 38, 23, 39, 30, 29, 30, 26, 22, 30, 31, 19, 23, 21, 27,
    4, 42, 27, 25, 33, 30, 21, 27, 29, 28, 28, 22, 27, 21, 32,
};

#endif

static int _MsrcEncDone(rqct_ops* rqct, mhve_job* mjob)
{
    mfe6_rqc* rqcx = (mfe6_rqc*)rqct;
    mfe6_reg* regs = (mfe6_reg*)mjob;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;
    ms_rc_top* ptMsRc = &msrc->msrc_top;

#ifdef SIMULATE_ON_I3
    int i;
#endif

#if 1
    regs->reg28 = regs->reg29 = 0;
    regs->regf5 = regs->regf6 = 0;
    regs->reg42 = regs->reg43 = 0;
#endif
    //fake
#ifdef SIMULATE_ON_I3
    if(rqct->i_enc_nr == 0)
        mjob->i_bits = 91032;
    else if(rqct->i_enc_nr == 1)
        mjob->i_bits = 14808;
    else if(rqct->i_enc_nr == 2)
        mjob->i_bits = 13872;
    else if(rqct->i_enc_nr == 3)
        mjob->i_bits = 15320;
    else if(rqct->i_enc_nr == 4)
        mjob->i_bits = 14856;
    else if(rqct->i_enc_nr == 5)
        mjob->i_bits = 15408;
    else if(rqct->i_enc_nr == 6)
        mjob->i_bits = 16792;
    else if(rqct->i_enc_nr == 7)
        mjob->i_bits = 18984;
    else if(rqct->i_enc_nr == 8)
        mjob->i_bits = 10168;
    else if(rqct->i_enc_nr == 9)
        mjob->i_bits = 19480;
    else
        mjob->i_bits = 15720;

    for(i = 0; i < 15; i++)
    {
        rqct->aiIdcHist[i] = g_aiHWLutIdcHist_test[rqct->i_enc_nr * 15 + i];
    }

    CamOsPrintf("\n==========hw_enc_frame (%d)============\n", rqct->i_enc_nr);

    CamOsPrintf("\ng_aiHWTextCplxHist\n");
    for(i = 0; i < 32; i++)
    {
        CamOsPrintf("%d ", regs->pmbr_tc_hist[i]);
    }
    CamOsPrintf("\n=======================================\n");
#endif

    g_iHWTextCplxAccum = regs->pmbr_tc_accum;

    rqct->i_bitcnt  = mjob->i_bits;
    rqct->i_enc_bs += mjob->i_bits / 8;
    rqct->i_enc_nr++;
    rqcx->i_refcnt++;

    //_rc_sig_check(ptMsRc, rqct,rqct->i_pictyp);

    MFE_MSG(MFE_MSG_DEBUG, "mjob->i_bits = %d\n", mjob->i_bits);

    ptMsRc->tMbrLut.auiQps = rqct->auiQps;
    ptMsRc->tMbrLut.auiBits = rqct->auiBits;
    ptMsRc->tMbrLut.aiIdcHist = rqct->aiIdcHist;

    switch(msrc->i_method)
    {
        case RQCT_MODE_CQP:
            break;
        case RQCT_MODE_CBR:
        case RQCT_MODE_VBR:
            _msrc_update_stats(ptMsRc, rqct->i_bitcnt);
            _msrc_update_model(ptMsRc, rqct->i_bitcnt, rqct->i_pictyp);
            break;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Fixed point rate control functions
///////////////////////////////////////////////////////////////////////////////

// CModel: LambdaToBpp
// Lambda = Alpha * Bpp^Beta

//--------------------------------------
#if 0
void assert_check(int val, int golden, char* checkType)
{
    if(val != golden)
    {
        CamOsPrintf("%s err! test:%d gold:%d\n", checkType, val, golden);
    }
}

void assert_check_err_range(int val, int golden, char* checkType)
{
#define ERR_PERCENT_THR ((double)0.02)
    int abs_diff = abs(val - golden);
    int err_thr = (int)(golden * ERR_PERCENT_THR);
    if(abs_diff > err_thr)
    {
        CamOsPrintf("%s err! test:%d gold:%d\n", checkType, val, golden);
    }
#undef DIFF_THR
}
void assert_check_err(int val, int golden, char* checkType)
{
#define ERR_VAL_THE ((int)1)
    int abs_diff = abs(val - golden);
    int err_thr = ERR_VAL_THE;
    if(abs_diff > err_thr)
    {
        CamOsPrintf("%s err! test:%d gold:%d\n", checkType, val, golden);
    }
#undef DIFF_THR
}
#endif


void _rc_sig_check(ms_rc_top* ptMsRc, rqct_ops* rqct, int picType)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    //int idx;

    if(picType == I_SLICE)
    {
        //assert_check(ptRcPic->iTargetBit, g_iPicAllocBit, "target bit");
        CamOsPrintf("ptRcPic->iTargetBit = %d\n", ptRcPic->iTargetBit);
    }
    else
    {
        //assert_check(ptRcPic->iTargetBit, g_iPicAllocBit, "target bit");
        CamOsPrintf("ptRcPic->iTargetBit = %d\n", ptRcPic->iTargetBit);
    }

    //assert_check(ptRcPic->uiPicQp, g_iPicEstQp, "pic Qp");
    CamOsPrintf("ptRcPic->uiPicQp = %d\n", ptRcPic->uiPicQp);

    /*    for(idx=0; idx<MBR_LUT_SIZE; idx++)
        {
          char lut_type[64];
          sprintf(lut_type, "lut target%02d", idx);
          assert_check(ptPmbr->m_nLUTTarget[idx], g_aiHWLUTTarget[idx], lut_type);
          sprintf(lut_type, "lut qp%02d", idx);
          assert_check((int)ptPmbr->m_nLUTQp[idx], g_aiHWLUTQp[idx], lut_type);
        }*/


}

// Larger range of integer square root
// Adpater for Q16.16 square root function
// input/output are all 64 bit integer
static uint64 _fSqrt_UInt64(uint64 X)
{
    uint64 ui64SqrtX;
    if(X > (((uint64) 1) << 48))
    {
        uint32 X_Div48bit = (uint32)(X >> 32);
        ui64SqrtX = ((uint64) fSqrt(X_Div48bit)) << 8;
    }
    else if(X > (((uint64) 1) << 32))
    {
        uint32 X_Div32bit = (uint32)(X >> 16);
        ui64SqrtX = (uint64) fSqrt(X_Div32bit);
    }
    else
    {
        uint32 X_Div16bit = (uint32) X;
        ui64SqrtX = (uint64) fSqrt(X_Div16bit) >> 8;
    }
    return ui64SqrtX;
}
// ------------ Inter-frame Rate control --------------------
void _msrc_pic_start(ms_rc_top* ptMsRc, int picType)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;

    uint32 uiPicQp;
    int picTargetBit = ptMsRc->iPicAvgBit;
    int gopAvgPicBit = ptRcGop->iPicAllocBit;

    if(picType == I_SLICE)
    {
        uiPicQp = _CalcIPicQp(ptMsRc);
    }
    else // P_SLICE
    {
        picTargetBit = ptRcGop->iGopBitLeft / ptRcGop->iGopFrameLeft;

        if(picTargetBit < MIN_PIC_BIT)
        {
            picTargetBit = MIN_PIC_BIT;
        }

        picTargetBit = (g_uiWeightOrgPicBit_SCALED * (uint64) gopAvgPicBit + (PREC_SCALE_FACTOR - g_uiWeightOrgPicBit_SCALED) * (uint64) picTargetBit) >> PREC_SCALE_BITS;
        picTargetBit = iClip3(ptMsRc->uiMinPicBit, ptMsRc->uiMaxPicBit, picTargetBit); // min/max picture bit clipping

        uiPicQp = _CalcPPicQp(ptMsRc, picTargetBit);
    }

    // ----------------------------
    // update status
    ptRcPic->iTargetBit = picTargetBit;
    ptRcPic->picType = picType;
    ptRcPic->auiLevelQp[picType] = uiPicQp;
    ptRcPic->uiPicQp = uiPicQp;
    ptRcGop->iPPicQpSum += (picType == P_SLICE) ? uiPicQp : 0;
}

void _msrc_gop_start(ms_rc_top* ptMsRc, msrc_rqc* pMsRc)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
    //ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    int iSmoothWindowSize = ptMsRc->uiFps * g_iRcStatsTime;
    int iBitErrCompensate = ptMsRc->iStreamBitErr / iSmoothWindowSize;
    uint32 iPicAvgBit = ptMsRc->iPicAvgBit;
    int GOPSize = ptMsRc->uiGopSize;
    int iAllocPicBit, iAllocGopBit;
    int iBitCompensLimit;
    int vbrMaxPicBit = iPicAvgBit;

    if(pMsRc->i_method == RQCT_MODE_VBR)
    {
        iPicAvgBit = (iPicAvgBit * pMsRc->i_btratio) / 100;   //g_dRcVBRBitRatio;
    }

    // CBR Mode
    if(pMsRc->i_method == RQCT_MODE_CBR)
        iBitCompensLimit = (int)(iPicAvgBit * g_iGOPCompenRatioLimit_MUL100) / 100;

    iBitErrCompensate = iClip3(-iBitCompensLimit, iBitCompensLimit, iBitErrCompensate);

    iAllocPicBit = iPicAvgBit - iBitErrCompensate;

    if(pMsRc->i_method == RQCT_MODE_VBR)
    {
        iAllocPicBit = min(vbrMaxPicBit, iAllocPicBit);
    }

    if(iAllocPicBit < MIN_PIC_BIT)
    {
        iAllocPicBit = MIN_PIC_BIT;
    }

    iAllocGopBit = iAllocPicBit * GOPSize;

    ptRcGop->iGopBitLeft = iAllocGopBit;
    ptRcGop->iPicAllocBit = iAllocPicBit;
    ptRcGop->iGopFrameLeft = GOPSize;

    ptRcGop->uiGopIdx++;
    if(ptRcGop->uiGopIdx == 1)
    {
        ptMsRc->uiPAverageQp = ptMsRc->uiInitialQp;
    }
    else
    {
        // compute the average QP of P frames in the previous GOP
        int iPicNum = GOPSize - 1;
        ptMsRc->uiPAverageQp = (int)((ptRcGop->iPPicQpSum + (iPicNum >> 1)) / iPicNum);
    }

    ptRcGop->iPPicQpSum = 0;

    MFE_MSG(MFE_MSG_DEBUG, "iGopBitLeft = %d \n", ptRcGop->iGopBitLeft);
    MFE_MSG(MFE_MSG_DEBUG, "iPicAllocBit = %d \n", ptRcGop->iPicAllocBit);
    MFE_MSG(MFE_MSG_DEBUG, "iGopFrameLeft = %d \n", ptRcGop->iGopFrameLeft);
}

void _msrc_update_stats(ms_rc_top* ptMsRc, int picEncBit)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
    //ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    //int picType = ptRcPic->picType;

    _calcAvgQpFromHist(ptMsRc);

    // Convert from sum-of-16x16average to sum-of-pixel
    ptMsRc->uiIntraMdlCplx = g_iHWTextCplxAccum << 8;

    ptMsRc->iStreamBitErr += (picEncBit - ptMsRc->iPicAvgBit);
    ptRcGop->iGopBitLeft -= picEncBit;

    ptMsRc->uiFrameCnt++;
    ptRcGop->iGopFrameLeft--;
}

void _msrc_update_model(ms_rc_top* ptMsRc, int picActualBit, int picType)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;

    int abRejected[RC_MODEL_HISTORY] = { 0 };
    //uint32 uiOutLinerThr;
    //uint64 ui64ErrAccum = 0;
    //uint32 auiPredErr[RC_MODEL_HISTORY] = {0};
    int entry;
    //  cache-based RQ sample hisotry update
    uint32 currQp = ptRcPic->uiPicQp;
    int bankNum = 3;
    int bankIndex = currQp % bankNum;
    int entryNumPerBank = RC_MODEL_HISTORY / bankNum;
    int entryIndex = bankIndex * entryNumPerBank;
    //int validSampleCnt = 0;

    if(picType == I_SLICE)
    {
        return;
    }

    // update the R-Q history
    for(entry = (entryIndex + entryNumPerBank - 1); entry > entryIndex; entry--)
    {
        ptMsRc->iSampleQs_SCALED[entry] = ptMsRc->iSampleQs_SCALED[entry - 1];
        ptMsRc->iSampleR[entry] = ptMsRc->iSampleR[entry - 1];
    }
    ptMsRc->iSampleQs_SCALED[entryIndex] = QP2Qstep_SCALED(currQp);
    ptMsRc->iSampleR[entryIndex] = picActualBit;

    for(entry = 0; entry < RC_MODEL_HISTORY; entry++)
    {
        abRejected[entry] = (ptMsRc->iSampleQs_SCALED[entry] == 0) ? 1 : 0;
    }

    _EstModelParas(ptMsRc, RC_MODEL_HISTORY, abRejected);

}

static void _EstModelParas(ms_rc_top* ptMsRc, int windowSize, int *pbRejected)
{
    int sampleSize = windowSize;
    int idx;
    int oneSampleQ = 0, estX2 = 0;
    int64 a00 = 0, a01 = 0, a10 = 0, a11 = 0, b0 = 0, b1 = 0;
    int64 MatrixValue, div_regulate;
    int64 scale_42bit = ((int64) 1) << 42;
    int64 scale_62bit = ((int64) 1) << 62;
    int64 scale_60bit = ((int64) 1) << 60;
    int64 remainder, divtemp;

    // default RD model estimation results
    ptMsRc->X1 = 0;
    ptMsRc->X2 = 0;
    for(idx = 0; idx < windowSize; idx++)
    {
        // find the number of samples which are not rejected
        if(pbRejected[idx] == 1)
            sampleSize--;
    }

    for(idx = 0; idx < windowSize; idx++)
    {
        if(pbRejected[idx] == 0)
        {
            oneSampleQ = ptMsRc->iSampleQs_SCALED[idx];
            break;
        }
    }
    for(idx = 0; idx < windowSize; idx++)
    {
        // if all non-rejected Q are the same, take 1st order model
        if((ptMsRc->iSampleQs_SCALED[idx] != oneSampleQ) && !pbRejected[idx])
        {
            estX2 = 1;
            break;
        }
    }

    // take 2nd order model to estimate X1 and X2
    if((sampleSize >= 1) && estX2 == 1)
    {
        int64 b1_x_a00, b0_x_a11, b1_x_a01, b0_x_a10;
        for(idx = 0; idx < windowSize; idx++)
        {
            if(pbRejected[idx] == 0)
            {
                int64 Qs_SCALED_square = ((int64) ptMsRc->iSampleQs_SCALED[idx] * ptMsRc->iSampleQs_SCALED[idx]); // 10frac bit
                a00++;
                CDBZ(ptMsRc->iSampleQs_SCALED[idx]);
                a01 += (CamOsMathDivS64(scale_60bit, ptMsRc->iSampleQs_SCALED[idx], &remainder)); // 55frac bit
                CDBZ(Qs_SCALED_square);
                a11 += (CamOsMathDivS64(scale_62bit, Qs_SCALED_square, &remainder));  // 52frac bit
                b0 += ((int64) ptMsRc->iSampleQs_SCALED[idx] * ptMsRc->iSampleR[idx]); // 5frac bit
                b1 += ptMsRc->iSampleR[idx];
            }
        }
        a01 >>= 29;
        a10 = a01;
        a11 >>= 26;

        // solve the equation of AX = B
        // a00: Q4.0
        // a01: Q5.26
        // a10: Q5.26
        // a11: Q5.26
        // b0: Q36.5
        // b1: Q32.0
        MatrixValue = ((a00 * a11) << 14) - ((a01 * a10) >> 12); // 40f
        div_regulate = 1 << (40 - 28);
        MatrixValue += (MatrixValue >= 0) ? div_regulate : -div_regulate;

        b0_x_a11 = b0 * a11;
        b1_x_a01 = b1 * a01;
        b1_x_a00 = b1 * a00;
        b0_x_a10 = b0 * a10;

        // -- adaptive fractional bit precision handle --
        if(b0_x_a11 < ((int64) 1 << 54) && b1_x_a01 < ((int64) 1 << 49))
        {
            CDBZ(MatrixValue);
            // normal case
            ptMsRc->X1 = CamOsMathDivS64(((b0_x_a11 << 9) - (b1_x_a01 << 14)), MatrixValue, &remainder);
        }
        else // in case b0_x_a11, b1_x_a01 overflow
        {
            divtemp = CamOsMathDivS64(MatrixValue, 256, &remainder);
            CDBZ(divtemp);
            ptMsRc->X1 = CamOsMathDivS64(((b0_x_a11 << 1) - (b1_x_a01 << 6)), divtemp, &remainder);
        }

        if(b1_x_a00 < (1 << 23) && b0_x_a10 < ((int64) 1 << 54))
        {
            CDBZ(MatrixValue);
            // normal case
            ptMsRc->X2 = CamOsMathDivS64(((b1_x_a00 << 40) - (b0_x_a10 << 9)), MatrixValue, &remainder);
        }
        else // in case b1_x_a00, b0_x_a10  overflow
        {
            divtemp = CamOsMathDivS64(MatrixValue, 256, &remainder);
            CDBZ(divtemp);
            ptMsRc->X2 = CamOsMathDivS64(((b1_x_a00 << 32) - (b0_x_a10 << 1)), divtemp, &remainder);
        }
        assert(ptMsRc->X1 < ((int64)1 << 38));
        assert(ptMsRc->X2 < ((int64)1 << 38));
    }
    else
    {
        int64 sum_r_div_q = 0;
        int64 sum_inv_q_square = 0;
        for(idx = 0; idx < windowSize; idx++)
        {
            if(pbRejected[idx] == 0)
            {
                int64 Qs_SCALED_square = ((int64) ptMsRc->iSampleQs_SCALED[idx] * ptMsRc->iSampleQs_SCALED[idx]);
                CDBZ(ptMsRc->iSampleQs_SCALED[idx]);
                sum_r_div_q += (CamOsMathDivS64((((int64) ptMsRc->iSampleR[idx]) << 37) , ptMsRc->iSampleQs_SCALED[idx], &remainder));
                CDBZ(Qs_SCALED_square);
                sum_inv_q_square += (CamOsMathDivS64(scale_42bit, Qs_SCALED_square, &remainder));
            }
        }
        CDBZ(sum_inv_q_square);
        ptMsRc->X1 = CamOsMathDivS64(sum_r_div_q, sum_inv_q_square, &remainder);
    }
}
// CModel: _calcAvgQpFromHist
// Update average QP and lambda by LUT histogram
static void _calcAvgQpFromHist(ms_rc_top* ptMsRc)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    mbr_lut *ptMbrLut = &ptMsRc->tMbrLut;
    uint32 uiTotalQP = 0;
    uint32 uiTotalCnt = 0, idc;
    uint32 uiActAvgQP;
    uint32 uiEntryCnt;


    for(idc = 0; idc < MBR_LUT_SIZE; idc++)
    {
        uiEntryCnt = ptMbrLut->aiIdcHist[idc];
        uiTotalCnt += uiEntryCnt;
        uiTotalQP += uiEntryCnt * ptMbrLut->auiQps[idc];
    }

    MFE_MSG(MFE_MSG_DEBUG, "uiTotalQP = %d \n", uiTotalQP);
    MFE_MSG(MFE_MSG_DEBUG, "uiTotalCnt = %d \n", uiTotalCnt);
    if(!uiTotalCnt)
        MFE_MSG(MFE_MSG_WARNING, "uiTotalCnt is 0 !!!!!\n");

    // Update
    uiActAvgQP = (uiTotalQP + (uiTotalCnt >> 1)) / uiTotalCnt;
    ptRcPic->auiLevelQp[ptRcPic->picType] = uiActAvgQP;
    ptRcPic->uiPicQp = uiActAvgQP;
}
static uint32 _CalcIPicQp(ms_rc_top* ptMsRc)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    uint32 Qp;

    if(ptMsRc->uiFrameCnt == 0)
    {
        Qp = ptMsRc->uiInitialQp;
    }
    else
    {
        uint32 QpMin, QpMax;
        uint32 lastPicQP = ptRcPic->uiPicQp;

        //  QP is constrained by QP of previous picture QP
        QpMin = lastPicQP >= g_uiRcIPicMinQp + g_uiRcPicClipRange ? lastPicQP - g_uiRcPicClipRange : g_uiRcIPicMinQp;
        QpMax = lastPicQP <= g_uiRcIPicMaxQp - g_uiRcPicClipRange ? lastPicQP + g_uiRcPicClipRange : g_uiRcIPicMaxQp;
        Qp = uiClip3(QpMin, QpMax, ptMsRc->uiPAverageQp);

        if(ptMsRc->tRcGop.uiGopIdx > 2)
        {
            uint32 uiLastIQp = ptRcPic->auiLevelQp[I_SLICE];
            Qp  += g_iRCIoverPQpDelta;
            // QP is constrained by I frame QP of previous GOP
            Qp  = iClip3(uiLastIQp - g_uiRcIMaxQPChange, uiLastIQp + g_uiRcIMaxQPChange, Qp);
        }
        // Also clipped within range.
        Qp = iClip3(g_uiRcIPicMinQp, g_uiRcIPicMaxQp, Qp);
    }
    return Qp;
}
static uint32 _EstPicQpByBits(ms_rc_top* ptMsRc, int targetBit)
{
    int64 remainder, divtemp;
    int iBestQp, iFormulaQp, iQstep_12bitSCALED;
    int64 X1 = ptMsRc->X1;
    int64 X2 = ptMsRc->X2;
    int64 squareTerm = (X1 * X1) + (X2 * (targetBit << 2));

    if((X2 == 0) || (squareTerm < 0))  // fall back 1st order mode
    {
        CDBZ(targetBit);
        iQstep_12bitSCALED = (int)(CamOsMathDivS64((X1 << 12), targetBit, &remainder));
    }
    else // 2nd order mode
    {
        int64 sqRoot = (int64) _fSqrt_UInt64(squareTerm);
        CDBZ((sqRoot - X1));
        iQstep_12bitSCALED = (int)(CamOsMathDivS64((2 * X2 * 4096), (sqRoot - X1), &remainder));
        CDBZ(targetBit);
        iQstep_12bitSCALED = (iQstep_12bitSCALED <= 0) ? (int)(CamOsMathDivS64((X1 << 12), targetBit, &remainder)) : iQstep_12bitSCALED;
    }

    iFormulaQp = Qstep_12bitSCALED2QP(iQstep_12bitSCALED);
    iBestQp = iFormulaQp;
    {
        int qp, bits, bit_dist, qstep_scale;
        int min_bit_dist = INT_MAX;
        for(qp = (iFormulaQp - 1); qp <= (iFormulaQp + 1); qp++)
        {
            if(qp < 0 || qp > 51)
            {
                continue;
            }
            qstep_scale = QP2Qstep_SCALED(qp);
            // scale up 2 bit for precision
            CDBZ(qstep_scale * qstep_scale);
            divtemp = CamOsMathDivS64(X2 * 4096, qstep_scale * qstep_scale, &remainder);
            CDBZ(qstep_scale);
            bits = (int)((CamOsMathDivS64((X1 * 128), qstep_scale, &remainder)) + divtemp);
            bits >>= 2;
            bit_dist = iabs(targetBit - bits);

            if(bit_dist <= min_bit_dist)
            {
                iBestQp = qp;
                min_bit_dist = bit_dist;
            }
        }
    }
    return iBestQp;
}

// CModel: estimatePicQP
static uint32 _CalcPPicQp(ms_rc_top* ptMsRc, int picBit)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    uint32 Qp, QpMin, QpMax;
    int isFirstPpic = (ptRcPic->picType == I_SLICE) ? 1 : 0;
    uint32 lastPicQP = ptRcPic->uiPicQp;
    if(isFirstPpic == 1)
    {
        Qp = ptMsRc->uiPAverageQp; // same as I frame Qp
    }
    else
    {
        Qp = _EstPicQpByBits(ptMsRc, picBit);
    }

    QpMin = lastPicQP >= g_uiRcPicMinQp + g_uiRcPicClipRange ? lastPicQP - g_uiRcPicClipRange : g_uiRcPicMinQp;
    QpMax = lastPicQP <= g_uiRcPicMaxQp - g_uiRcPicClipRange ? lastPicQP + g_uiRcPicClipRange : g_uiRcPicMaxQp;
    Qp = uiClip3(QpMin, QpMax, Qp);
    return Qp;
}
