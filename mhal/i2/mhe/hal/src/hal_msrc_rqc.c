#include "hal_mhe_ops.h"
#include "hal_mhe_utl.h"
#include "hal_msrc_rqc.h"
#include "hal_mhe_msmath.h"
#include "hal_mhe_global.h"
#include "../../drv/pub/drv_mhe_kernel.h"


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

#define MIN_QP                      10
#define MAX_QP                      51

// Mode penalties when RDO cost comparison
//#define PENALTY_BITS   (16)
#define PENALTY_SHIFT  (2)

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

#define COMPLEX_I   (1<<24)
#define COMPLEX_P   (1<<20)

#define MAX_SUMQS   ((2UL<<31)-1)
#define ALIGN2CTU(l) (((l)+31)>>5)

// P-frame min, max QP
static uint32 g_uiRcPicMinQp = MIN_QP;
static uint32 g_uiRcPicMaxQp = MAX_QP;
// I-frame min, max QP
static uint32 g_uiRcIPicMinQp = MIN_QP;
static uint32 g_uiRcIPicMaxQp = MAX_QP;

// Picture delta Qp range
static uint32 g_uiRcPicClipRange = 4; // default

// min/max frame bit ratio relative to avg bitrate
static int g_iRcMaxIPicBitRatio = 60; // default
static int g_iRcMinIPicBitRatio = 1; // default
static int g_iRcMaxPicBitRatio = 10; // default
static int g_iRcMinPicBitRatio = 0; // default

static int g_iHWTextCplxAccum;

static int g_iRcStatsTime = 1;

/************************************************************************/
/* Local structures                                                     */
/************************************************************************/

#define PICTYPES 2

typedef struct MsrcRqc_t
{
    mhe_rqc    rqcx;
    int     i_method;           /* Rate Control Method */
    int     i_btrate;           /* Bitrate */
    int     i_btratio;          /* Bitrate Ratio (50~100) */
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


// Generate from C-Model (search "g_DefaultLambdaByQp")
// Q16.16
//I frame qp to lambda table
static const uint32 g_DefaultLambdaByQp[52] =
{
    2334,   2941,   3706,   4669,   5883,
    7412,   9338,   11766,  14824,  18677,
    23532,  29649,  37355,  47065,  59298,
    74711,  94130,  118596, 149422, 188260,
    237192, 298844, 376520, 474385, 597688,
    753040, 948771, 1195376,    1506080,    1897542,
    2390753,    3012160,    3795084,    4781506,    6024320,
    7590168,    9563013,    12048641,   15180337,   19126026,
    24097283,   30360674,   38252052,   48194566,   60721348,
    76504104,   96389132,   121442696,  153008209,  192778264,
    242885393,  306016419,
};

//P frame qp to lambda table
static const uint32 g_PFrameLambdaByQp[52] =
{
    1893,       2386,         3006,       3787,       4772,
    6013,       7575,         9545,       12026,     15151,
    19090,     24052,       30303,     38180,     48104,
    60607,     76360,      96208,     121215,   152721,
    192417,   242430,     305443,   384834,   484861,
    610887,   769669,     969723,   1221774,       1539339,
    1939446, 2443549,   3078678, 3878892,       4887098,
    6157357, 7757784,   9774196, 12314715, 15515569,
    19548392, 24629431, 31031138, 39096784, 49258862,
    62062277, 78193569, 98517724, 124124554, 156387139,
    197035448, 248249109,
};


/************************************************************************/
/* Local prototypes                                                     */
/************************************************************************/

char* MheMsrcDescribe(void);

static int  _MsrcSeqSync(rqct_ops*);
static int  _MsrcSeqDone(rqct_ops*);
static int  _MsrcSeqConf(rqct_ops*);
static int  _MsrcEncConf(rqct_ops*, mhve_job*);
static int  _MsrcEncDone(rqct_ops*, mhve_job*);

static void _MsrcOpsFree(rqct_ops* rqct)
{
    MEM_FREE(rqct);
}

/* Fixed Point Rate Control */
static uint32 _LambdaToBpp_SCALED(ms_rc_top* ptMsRc, uint32 uiLambda_SCALED, int picType, int isRTcModel);
static void _calcAvgLambdaFromHist(ms_rc_top* ptMsRc);
static uint32 _EstPicLambdaByBits(ms_rc_top* ptMsRc, int picType, int picBit);
static uint32 _EstimatePicQpByLambda(ms_rc_top* ptMsRc, int picType, uint32 lambda_SCALED);
static int _EstBitsForIntraFrame(ms_rc_top* ptMsRc);
static void _Est1stFrameParamByQp(ms_rc_top* ptMsRc, int picType, uint32 uiQp, int isRTcModel, int* p_retBit, uint32 *p_retLambda_SCALED);
static void _msrc_gop_start(ms_rc_top* ptMsRcm, msrc_rqc* pMsRc);
static void _msrc_pic_start(ms_rc_top* ptMsRc, int picType);
static void _msrc_update_stats(ms_rc_top* ptMsRc, int picEncBit);
static void _msrc_update_model(ms_rc_top* ptMsRc, int picActualBit, int picType);

///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

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

//------------------------------------------------------------------------------
//  Function    : ROISetMapValue
//  Description :
//------------------------------------------------------------------------------
static void _ROISetMapValue(char value, char mode, char *map, struct roirec* rec, int ihorzctbnum, int ivertctbnum, int ctbnum)
{
    char *pRoiMap, *pRoiMapHor;
    int startX, startY, width, height;
    int stride = ihorzctbnum * 2; //1 CTB need 2 bytes
    int x, y;

    startX = rec->i_posx;
    startY = rec->i_posy;
    width = rec->i_recw;
    height = rec->i_rech;

    //CamOsPrintf("[%s %d] (%d, %d, %d, %d) (%d %d %d) \n", __FUNCTION__, __LINE__, startX, startY, width, height, ihorzctbnum, ivertctbnum, ctbnum);
    //mode = 7;
    //value = 0;

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

            //        bool bCtbForceMode = !bCtbForceIntra; // 0:force intra, 1:Zmv; Force Intra has higher priority
            //        bool bCtbForceEn = bCtbForceIntra | bCtbZmv;
            //        unsigned char first_byte = iEntryOffset&0xFF;
            //        unsigned char second_byte = (((char)bCtbZmv)<<2) | (((char)bCtbForceMode)<<1) | ((char)bCtbForceEn);

            _ROISetMapValue(iMapValue, ctbforcemode, qpmap, rec, ctbw, ctbh, ctbn);

            enable = 1;
        }
    }

    //print_hex("QP MAP", attr->p_dqmkptr, attr->i_dqmsize);

    return enable;
}

//------------------------------------------------------------------------------
//  Function    : MheMrqcRoiDraw
//  Description :
//------------------------------------------------------------------------------
int MheMrqcRoiDraw(rqct_ops* rqct, mhve_job* mjob)
{
    mhe_rqc* rqcx = (mhe_rqc*)rqct;
    mhe_reg* regs = (mhe_reg*)mjob;

    if(0 > rqcx->attr.b_dqmstat)
    {
        /* Edit qmap and enable it */

        //Initial ROI MAP
        MEM_SETV((char*)rqcx->attr.p_dqmkptr, 0, rqcx->attr.i_dqmsize);

        if(_draw(&rqcx->attr))
        {
            regs->hev_bank1.reg_mbr_lut_roi_on = 1;//g_bROIQpEn;
            regs->hev_bank1.reg_txip_ctb_force_on = 0;//g_bForcedIntraEn | g_bROIZmvEn;

            regs->hev_bank1.reg_mbr_gn_read_map_st_addr_low = rqcx->attr.u_dqmphys >> 4;
            regs->hev_bank1.reg_mbr_gn_read_map_st_addr_high = rqcx->attr.u_dqmphys >> 16;

            rqcx->attr.b_dqmstat = 1;
        }
        else
        {
            regs->hev_bank1.reg_mbr_lut_roi_on = 0;
            regs->hev_bank1.reg_txip_ctb_force_on = 0;

            rqcx->attr.b_dqmstat = 0;
        }
    }

    return (rqcx->attr.b_dqmstat);
}

/************************************************************************/
/* Functions                                                            */
/************************************************************************/

//------------------------------------------------------------------------------
//  Function    : MheMsrcAllocate
//  Description :
//------------------------------------------------------------------------------
void* MheMsrcAllocate(void)
{

    rqct_ops* rqct = NULL;
    mhe_rqc* rqcx;
    msrc_rqc* msrc;

    if(!(rqct = MEM_ALLC(sizeof(msrc_rqc))))
        return NULL;

    MEM_COPY(rqct->name, MSRC_NAME, 5);

    /* Link member function */
    rqct->release  = _MsrcOpsFree;
    rqct->seq_sync = _MsrcSeqSync;
    rqct->seq_done = _MsrcSeqDone;
    rqct->set_rqcf = MheMrqcSetRqcf;
    rqct->get_rqcf = MheMrqcGetRqcf;
    rqct->seq_conf = _MsrcSeqConf;
    rqct->enc_buff = MheMrqcEncBuff;
    rqct->enc_conf = _MsrcEncConf;
    rqct->enc_done = _MsrcEncDone;
    rqct->i_enc_nr = 0;
    rqct->i_enc_bs = 0;

    /* Initialize Basic RC Attribute */
    rqcx = (mhe_rqc*)rqct;
    rqcx->attr.i_pict_w = 0;
    rqcx->attr.i_pict_h = 0;
    rqcx->attr.i_method = RQCT_MODE_CQP;
    rqcx->attr.i_btrate = 0;
    rqcx->attr.i_leadqp = -1;
    rqcx->attr.i_deltaq = QP_IFRAME_DELTA;
    rqcx->attr.i_iupperq = QP_UPPER;
    rqcx->attr.i_ilowerq = QP_LOWER;
    rqcx->attr.i_pupperq = QP_UPPER;
    rqcx->attr.i_plowerq = QP_LOWER;
    rqcx->attr.n_fmrate = 30;
    rqcx->attr.d_fmrate = 1;
    rqcx->attr.i_period = 0;
    rqcx->attr.b_logoff = LOGOFF_DEFAULT;

    /* Initialize Penalties */
    rqcx->attr.b_ia8xlose = 0;
    rqcx->attr.b_ir8xlose = 0;
    rqcx->attr.b_ia16lose = 0;
    rqcx->attr.b_ir16lose = 0;
    rqcx->attr.b_ir16mlos = 0;
    rqcx->attr.b_ir16slos = 0;
    rqcx->attr.b_ir16mslos = 0;
    rqcx->attr.b_ia32lose = 0;
    rqcx->attr.b_ir32mlos = 0;
    rqcx->attr.b_ir32mslos = 0;

    rqcx->attr.u_ia8xpen = 0;
    rqcx->attr.u_ir8xpen = 0;
    rqcx->attr.u_ia16pen = 0;
    rqcx->attr.u_ir16pen = 0;
    rqcx->attr.u_ir16mpen = 0;
    rqcx->attr.u_ir16spen = 0;
    rqcx->attr.u_ir16mspen = 0;
    rqcx->attr.u_ia32pen = 0;
    rqcx->attr.u_ir32mpen = 0;
    rqcx->attr.u_ir32mspen = 0;

    rqcx->i_config = 0;
    rqcx->i_pcount = 0;
    rqcx->i_period = 0;
    rqcx->b_passiveI = 0;
    rqcx->b_seqhead = 1;

    msrc = (msrc_rqc*)rqcx;
    msrc->i_levelq = 36;

    CamOsPrintf("%s\n", MheMsrcDescribe());

    return rqct;

}

//------------------------------------------------------------------------------
//  Function    : MheMsrcDescribe
//  Description :
//------------------------------------------------------------------------------
char* MheMsrcDescribe(void)
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

    mhe_rqc* rqcx = (mhe_rqc*)rqct;

    rqcx->i_pcount = 0;
    rqcx->b_seqhead = 1;

    return 0;
}


static int _MsrcSeqConf(rqct_ops* rqct)
{
    mhe_rqc* rqcx = (mhe_rqc*)rqct;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;

    uint64 uiremainder;
    uint32 uiSeqTargetBpp_SCALED;
    ms_rc_pic *ptRcPic;

    msrc->i_method = (int)rqcx->attr.i_method;

    if(msrc->i_method == RQCT_MODE_VBR)
    {
        msrc->i_btratio = 80;   //(int) rqcx->attr.i_btratio;
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

    if(msrc->i_method != RQCT_MODE_CQP)
    {
        msrc->i_frmbit = (int)CamOsMathDivU64((uint64)msrc->msrc_top.uiTargetBitrate * msrc->d_fmrate, msrc->n_fmrate, &uiremainder);
        msrc->msrc_top.iPicAvgBit = msrc->i_frmbit;
    }

    msrc->msrc_top.uiPicCtuNum = ALIGN2CTU(rqcx->attr.i_pict_w) * ALIGN2CTU(rqcx->attr.i_pict_h);
    msrc->msrc_top.uiIntraMdlCplx = msrc->msrc_top.uiPicPixelNum * 10;

    if(rqcx->attr.i_leadqp > QP_MIN && rqcx->attr.i_leadqp < QP_MAX)
        msrc->i_levelq = (int)rqcx->attr.i_leadqp;

    msrc->msrc_top.uiInitialQp = (uint32)msrc->i_levelq;

    ptRcPic = &msrc->msrc_top.tRcPic;
    ptRcPic->uiPicLambda_SCALED = 0;
    ptRcPic->uiPicQp = 0;

    msrc->msrc_top.iLambdaMinClipMul = (int)MhefPow64(2 << PREC_SCALE_BITS, -(int)(g_uiRcPicClipRange << PREC_SCALE_BITS) / 3);
    msrc->msrc_top.iLambdaMaxClipMul = (int)MhefPow64(2 << PREC_SCALE_BITS, (g_uiRcPicClipRange << PREC_SCALE_BITS) / 3);

    msrc->msrc_top.uiMaxPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMaxPicBitRatio;
    msrc->msrc_top.uiMinPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMinPicBitRatio;
    msrc->msrc_top.uiMaxIPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMaxIPicBitRatio;
    msrc->msrc_top.uiMinIPicBit = msrc->msrc_top.iPicAvgBit * g_iRcMinIPicBitRatio;

    // RC model parameter initial
    msrc->msrc_top.atModelPara[I_SLICE].Alpha_SCALED = ALPHA_SCALED;
    msrc->msrc_top.atModelPara[I_SLICE].Beta_SCALED = BETA2_SCALED;
    msrc->msrc_top.atModelPara[P_SLICE].Alpha_SCALED = ALPHA_P_SCALED;
    msrc->msrc_top.atModelPara[P_SLICE].Beta_SCALED = BETA_P_SCALED;

    CDBZ(msrc->msrc_top.uiPicPixelNum);
    uiSeqTargetBpp_SCALED = (uint32)CamOsMathDivU64((uint64)msrc->msrc_top.iPicAvgBit << PREC_SCALE_BITS, msrc->msrc_top.uiPicPixelNum, &uiremainder);

    // Pre-multiplied magic numbers.
    // NEED_CHANGE_BY_PREC_SCALE_BITS
    if(uiSeqTargetBpp_SCALED < 1966)
    {
        msrc->msrc_top.uiStepAlpha_SCALED = 655;
        msrc->msrc_top.uiStepBeta_SCALED = 328;
    }
    else if(uiSeqTargetBpp_SCALED < 5242)
    {
        msrc->msrc_top.uiStepAlpha_SCALED = 3277;
        msrc->msrc_top.uiStepBeta_SCALED  = 1638;
    }
    else if(uiSeqTargetBpp_SCALED < 13107)
    {
        msrc->msrc_top.uiStepAlpha_SCALED = 6554;
        msrc->msrc_top.uiStepBeta_SCALED  = 3277;
    }
    else if(uiSeqTargetBpp_SCALED < 32768)
    {
        msrc->msrc_top.uiStepAlpha_SCALED = 13107;
        msrc->msrc_top.uiStepBeta_SCALED  = 6554;
    }
    else
    {
        msrc->msrc_top.uiStepAlpha_SCALED = 26214;
        msrc->msrc_top.uiStepBeta_SCALED  = 13107;
    }

    msrc->msrc_top.tMbrLut.auiQps = rqct->auiQps;
    msrc->msrc_top.tMbrLut.auiLambdas_SCALED = rqct->auiLambdas_SCALED;
    msrc->msrc_top.tMbrLut.auiBits = rqct->auiBits;
    msrc->msrc_top.tMbrLut.aiIdcHist = rqct->aiIdcHist;

    MHE_MSG(MHE_MSG_DEBUG, "uiInitialQp = %d\n", msrc->msrc_top.uiInitialQp);
    MHE_MSG(MHE_MSG_DEBUG, "uiTargetBitrate = %d\n", msrc->msrc_top.uiTargetBitrate);
    MHE_MSG(MHE_MSG_DEBUG, "uiFps = %d\n", msrc->msrc_top.uiFps);
    MHE_MSG(MHE_MSG_DEBUG, "uiGopSize = %d\n", msrc->msrc_top.uiGopSize);
    MHE_MSG(MHE_MSG_DEBUG, "uiPicPixelNum = %d\n", msrc->msrc_top.uiPicPixelNum);
    MHE_MSG(MHE_MSG_DEBUG, "iPicAvgBit = %d\n", msrc->msrc_top.iPicAvgBit);
    MHE_MSG(MHE_MSG_DEBUG, "uiPicCtuNum = %d\n", msrc->msrc_top.uiPicCtuNum);

    if(!msrc->msrc_top.uiInitialQp)
        MHE_MSG(MHE_MSG_WARNING, "uiInitialQp is %d !!\n", msrc->msrc_top.uiInitialQp);
    if(!msrc->msrc_top.uiTargetBitrate)
        MHE_MSG(MHE_MSG_WARNING, "uiTargetBitrate is %d !!\n", msrc->msrc_top.uiTargetBitrate);
    if(!msrc->msrc_top.uiFps)
        MHE_MSG(MHE_MSG_WARNING, "uiFps is %d !!\n", msrc->msrc_top.uiFps);
    if(!msrc->msrc_top.uiGopSize)
        MHE_MSG(MHE_MSG_WARNING, "uiGopSize is %d !!\n", msrc->msrc_top.uiGopSize);
    if(!msrc->msrc_top.uiPicPixelNum)
        MHE_MSG(MHE_MSG_WARNING, "uiPicPixelNum is %d !!\n", msrc->msrc_top.uiPicPixelNum);
    if(!msrc->msrc_top.iPicAvgBit)
        MHE_MSG(MHE_MSG_WARNING, "iPicAvgBit is %d !!\n", msrc->msrc_top.iPicAvgBit);
    if(!msrc->msrc_top.uiPicCtuNum)
        MHE_MSG(MHE_MSG_WARNING, "uiPicCtuNum is %d !!\n", msrc->msrc_top.uiPicCtuNum);

    return 0;
}

static int _MsrcEncConf(rqct_ops* rqct, mhve_job* mjob)
{

    mhe_rqc* rqcx = (mhe_rqc*)rqct;
    mhe_reg* regs = (mhe_reg*)mjob;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;
    ms_rc_top* ptMsRc = &msrc->msrc_top;

    MheMrqcRoiDraw(rqct, mjob);

    switch(msrc->i_method)
    {
        case RQCT_MODE_CQP:
            rqct->i_enc_qp = msrc->i_levelq;
            if(IS_IPIC(rqct->i_pictyp))
            {
                rqct->i_enc_qp -= msrc->i_deltaq;
                rqct->i_enc_lamda = g_DefaultLambdaByQp[rqct->i_enc_qp];
            }
            else if(IS_PPIC(rqct->i_pictyp))
            {
                rqct->i_enc_lamda = g_PFrameLambdaByQp[rqct->i_enc_qp];
            }

            // Qp
            regs->hev_bank1.reg_mbr_const_qp_en = 1;
            regs->hev_bank0.reg_hev_ec_qp_delta_enable_flag = 0; //enable PMBR, derek check
            regs->hev_bank0.reg_hev_slice_qp = rqct->i_enc_qp;

            break;
        case RQCT_MODE_CBR:
        case RQCT_MODE_VBR:
            if(IS_IPIC(rqct->i_pictyp))
            {
                _msrc_gop_start(ptMsRc, msrc);
            }

            _msrc_pic_start(ptMsRc, rqct->i_pictyp);

            rqct->i_enc_qp = ptMsRc->tRcPic.uiPicQp;
            rqct->i_enc_lamda = ptMsRc->tRcPic.uiPicLambda_SCALED;

            // Qp
            regs->hev_bank1.reg_mbr_const_qp_en = 1;
            regs->hev_bank0.reg_hev_ec_qp_delta_enable_flag = 1; //enable PMBR, derek check
            regs->hev_bank0.reg_hev_slice_qp = rqct->i_enc_qp;

            break;
        default:
            break;
    }

    /* Initialize Penalties of CTB */
    regs->hev_bank0.reg_hev_txip_cu8_intra_lose              = rqcx->attr.b_ia8xlose;
    regs->hev_bank0.reg_hev_txip_cu8_inter_lose              = rqcx->attr.b_ir8xlose;
    regs->hev_bank0.reg_hev_txip_cu16_intra_lose             = rqcx->attr.b_ia16lose;
    regs->hev_bank0.reg_hev_txip_cu16_inter_mvp_lose         = rqcx->attr.b_ir16lose;
    regs->hev_bank0.reg_hev_txip_cu16_inter_merge_lose       = rqcx->attr.b_ir16mlos;
    regs->hev_bank0.reg_hev_txip_cu16_inter_mvp_nores_lose   = rqcx->attr.b_ir16slos;
    regs->hev_bank0.reg_hev_txip_cu16_inter_merge_nores_lose = rqcx->attr.b_ir16mslos;
    regs->hev_bank0.reg_hev_txip_cu32_intra_lose             = rqcx->attr.b_ia32lose;
    regs->hev_bank0.reg_hev_txip_cu32_inter_merge_lose       = rqcx->attr.b_ir32mlos;
    regs->hev_bank0.reg_hev_txip_cu32_inter_merge_nores_lose = rqcx->attr.b_ir32mslos;

    // Mode penalties when RDO cost comparison
    regs->hev_bank0.reg_hev_txip_cu8_intra_penalty              = rqcx->attr.u_ia8xpen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu8_inter_penalty              = rqcx->attr.u_ir8xpen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu16_intra_penalty             = rqcx->attr.u_ia16pen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu16_inter_mvp_penalty         = rqcx->attr.u_ir16pen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu16_inter_merge_penalty       = rqcx->attr.u_ir16mpen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu16_inter_mvp_nores_penalty   = rqcx->attr.u_ir16spen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu16_inter_merge_nores_penalty = rqcx->attr.u_ir16mspen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu32_intra_penalty             = rqcx->attr.u_ia32pen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu32_inter_merge_penalty       = rqcx->attr.u_ir32mpen >> PENALTY_SHIFT;
    regs->hev_bank0.reg_hev_txip_cu32_inter_merge_nores_penalty = rqcx->attr.u_ir32mspen >> PENALTY_SHIFT;

    return 0;
}


static int _MsrcEncDone(rqct_ops* rqct, mhve_job* mjob)
{
    mhe_rqc* rqcx = (mhe_rqc*)rqct;
    mhe_reg* regs = (mhe_reg*)mjob;
    msrc_rqc* msrc = (msrc_rqc*)rqcx;
    ms_rc_top* ptMsRc = &msrc->msrc_top;

    regs->hev_bank1.reg6d = regs->hev_bank1.reg6e = 0;
    regs->mhe_bank0.reg74 = regs->mhe_bank0.reg75 = 0;

    g_iHWTextCplxAccum = regs->pmbr_tc_accum;

    rqct->i_bitcnt  = mjob->i_bits;
    rqct->i_enc_bs += mjob->i_bits / 8;
    rqct->i_enc_nr++;
    rqcx->i_refcnt++;

#ifdef RC_SIG_CHECK
    _rc_sig_check(ptMsRc, rqct, rqct->i_pictyp);
#endif

    MHE_MSG(MHE_MSG_DEBUG, "mjob->i_bits = %d\n", mjob->i_bits);

    ptMsRc->tMbrLut.auiQps = rqct->auiQps;
    ptMsRc->tMbrLut.auiBits = rqct->auiBits;
    ptMsRc->tMbrLut.auiLambdas_SCALED = rqct->auiLambdas_SCALED;
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

#ifdef RC_SIG_CHECK
void _rc_sig_check(ms_rc_top* ptMsRc, rqct_ops* rqct, int picType)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    int idx;
    //char lut_type[64];
    uint32 hw_lut_lambda;


    if(picType == I_SLICE)
    {
        //assert_check_err_range(ptRcPic->iTargetBit, g_iPicAllocBit, "target bit");
        CamOsPrintf("ptRcPic->iTargetBit = %d\n", ptRcPic->iTargetBit);
    }
    else
    {
        //assert_check(ptRcPic->iTargetBit, g_iPicAllocBit, "target bit");
        CamOsPrintf("ptRcPic->iTargetBit = %d\n", ptRcPic->iTargetBit);
    }

    //assert_check_err_range(ptRcPic->uiPicLambda_SCALED >> PREC_SCALE_BITS, g_iPicEstLambda, "pic Lambda");
    //assert_check_err(ptRcPic->uiPicQp, g_iPicEstQp, "pic Qp");

    CamOsPrintf("ptRcPic->uiPicLambda_SCALED = %d\n", ptRcPic->uiPicLambda_SCALED);
    CamOsPrintf("ptRcPic->uiPicQp = %d\n", ptRcPic->uiPicQp);

//    for (idx = 0; idx < MBR_LUT_SIZE; idx++)
//    {
//        hw_lut_lambda = ptPmbr->m_auiLUTLambdas_SCALED[idx] >> 6;
//        sprintf(lut_type, "lut target%02d", idx);
//        assert_check(ptPmbr->m_nLUTTarget[idx], g_aiHWLUTTarget[idx], lut_type);
//        sprintf(lut_type, "lut lambda%02d", idx);
//        assert_check_err_range(hw_lut_lambda, g_aiHWLUTLambda[idx], lut_type);
//        sprintf(lut_type, "lut qp%02d", idx);
//        assert_check_err((int) ptPmbr->m_nLUTQp[idx], g_aiHWLUTQp[idx], lut_type);
//    }


    CamOsPrintf("\n");
    CamOsPrintf("hw_lut_lambda  :");
    for(idx = 0; idx < MBR_LUT_SIZE; idx++)
    {
        //hw_lut_lambda = ptPmbr->m_auiLUTLambdas_SCALED[idx] >> 6;
        hw_lut_lambda = rqct->auiLambdas_SCALED[idx] >> 6;
        //assert_check_err_range(hw_lut_lambda, g_aiHWLUTLambda[idx], lut_type);
        CamOsPrintf("%d ", hw_lut_lambda);
    }

    CamOsPrintf("\n");
    CamOsPrintf("m_nLUTTarget   :");
    for(idx = 0; idx < MBR_LUT_SIZE; idx++)
    {
        //assert_check(ptPmbr->m_nLUTTarget[idx], g_aiHWLUTTarget[idx], lut_type);
        CamOsPrintf("%d ", rqct->auiBits[idx]);
    }

    CamOsPrintf("\n");
    CamOsPrintf("m_nLUTQp       :");
    for(idx = 0; idx < MBR_LUT_SIZE; idx++)
    {
        //assert_check_err((int) ptPmbr->m_nLUTQp[idx], g_aiHWLUTQp[idx], lut_type);
        CamOsPrintf("%d ", rqct->auiQps[idx]);
    }

    CamOsPrintf("\n");
    CamOsPrintf("aiIdcHist       :");
    for(idx = 0; idx < MBR_LUT_SIZE; idx++)
    {
        //assert_check_err((int) ptPmbr->aiIdcHist[idx], g_aiHWLUTQp[idx], lut_type);
        CamOsPrintf("%d ", rqct->aiIdcHist[idx]);
    }
    CamOsPrintf("\n");

    CamOsPrintf("\n==================================\n");
}
#endif
//--------------------------------------

// CModel: LambdaToBpp
// Lambda = Alpha * Bpp^Beta
static uint32 _LambdaToBpp_SCALED(ms_rc_top* ptMsRc, uint32 uiLambda_SCALED, int picType, int isRTcModel)
{
    rc_model_para *ptRcMdlPara = &ptMsRc->atModelPara[picType];
    int alpha_SCALED = ptRcMdlPara->Alpha_SCALED;
    int beta_SCALED = ptRcMdlPara->Beta_SCALED;
    uint32 uiEstBpp_SCALED;
    uint64 uiremainder;
    int64 remainder;
    uint32 picPelNum;
    uint32 IframeCplx;
    uint32 CplxPerPel_SCALE;
    uint64 uiNormTC_SCALED;
    uint32 lambda_div_alpha;
    uint32 scal_8bit;
    int neg_inv_beta_SCALED;
    uint64 pow_lambda_x256_div_alpha;
    uint32 lambda_div_alphaSCALED;
    int inv_betaSCALED;


    isRTcModel &= (picType == I_SLICE);

    if(isRTcModel)
    {
        picPelNum = ptMsRc->uiPicPixelNum;
        IframeCplx = ptMsRc->uiIntraMdlCplx;

        CDBZ(picPelNum);
        CplxPerPel_SCALE = CamOsMathDivU64(((uint64) IframeCplx << PREC_SCALE_BITS), picPelNum, &uiremainder);
        uiNormTC_SCALED = MhefPow64(CplxPerPel_SCALE, BETA1_SCALED);

        CDBZ(alpha_SCALED);
        lambda_div_alpha = (uint32) CamOsMathDivU64(((uint64) uiLambda_SCALED << PREC_SCALE_BITS), alpha_SCALED, &uiremainder);
        scal_8bit = ((uint32) 256 << PREC_SCALE_BITS);

        CDBZ(beta_SCALED);
        neg_inv_beta_SCALED = (int) - (CamOsMathDivS64(((int64) 1 << (PREC_SCALE_BITS << 1)), beta_SCALED, &remainder));

        // org
        // uint64 lambda_div_alpha_x256 = ((uint64)uiLambda_SCALED<<(PREC_SCALE_BITS+8))/alpha_SCALED;

        assert(lambda_div_alpha);
        pow_lambda_x256_div_alpha = (MhefPow64(lambda_div_alpha, neg_inv_beta_SCALED) * MhefPow64(scal_8bit, neg_inv_beta_SCALED)) >> PREC_SCALE_BITS;

        uiEstBpp_SCALED = (uint32)((uiNormTC_SCALED * pow_lambda_x256_div_alpha) >> PREC_SCALE_BITS);
    }
    else
    {
        CDBZ(alpha_SCALED);
        lambda_div_alphaSCALED = CamOsMathDivU64(((uint64) uiLambda_SCALED << PREC_SCALE_BITS), alpha_SCALED, &uiremainder);
        CDBZ(beta_SCALED);
        inv_betaSCALED = (int) CamOsMathDivS64(((int64) 1 << (PREC_SCALE_BITS << 1)), beta_SCALED, &remainder);

        uiEstBpp_SCALED = (uint32)MhefPow64(lambda_div_alphaSCALED, inv_betaSCALED);
    }

    return uiEstBpp_SCALED;
}

// CModel: calcAvgLambdaFromHist
// Update average QP and lambda by LUT histogram
static void _calcAvgLambdaFromHist(ms_rc_top* ptMsRc)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    mbr_lut *ptMbrLut = &ptMsRc->tMbrLut;
    int64 iTotalLambda_SCALED = 0;  // sum of log-of-lambda_SCALED (for geometric mean)
    uint32 uiTotalQP = 0;
    uint32 uiTotalCnt = 0, idc;
    uint32 uiAvgTotalLamdba, uiActAvgLambda_SCALED, uiActAvgQP;
    int64 remainder;
    uint32 uiEntryCnt;

    for(idc = 0; idc < MBR_LUT_SIZE; idc++)
    {
        uiEntryCnt = ptMbrLut->aiIdcHist[idc];              //m_nLUTEntryHist
        uiTotalCnt += uiEntryCnt;

        uiTotalQP += uiEntryCnt * ptMbrLut->auiQps[idc];    //m_nLUTQp

        //CamOsPrintf("IdcHist[%d] auiQps[%d] auiLambdas_SCALED[%d] = %d, %d, %d\n", idc, idc, idc, ptMbrLut->aiIdcHist[idc],  ptMbrLut->auiQps[idc], ptMbrLut->auiLambdas_SCALED[idc]);
        iTotalLambda_SCALED += ((int64) uiEntryCnt * MhefLog(ptMbrLut->auiLambdas_SCALED[idc]));   //m_auiLUTLambdas_SCALED
    }

    MHE_MSG(MHE_MSG_DEBUG, "uiTotalQP = %d \n", uiTotalQP);
    MHE_MSG(MHE_MSG_DEBUG, "uiTotalCnt = %d \n", uiTotalCnt);
    if(!uiTotalCnt)
        MHE_MSG(MHE_MSG_WARNING, "uiTotalCnt is 0 !!!!!\n");

    // Update
    CDBZ(uiTotalCnt);
    uiAvgTotalLamdba = (uint32) CamOsMathDivU64(iTotalLambda_SCALED, uiTotalCnt, &remainder);
    uiActAvgLambda_SCALED = (uint32) MhefExp(uiAvgTotalLamdba);
    uiActAvgQP = (uiTotalQP + (uiTotalCnt >> 1)) / uiTotalCnt;

    //CamOsPrintf("uiActAvgQP = %d, uiTotalQP = %d, uiTotalCnt = %d\n", uiActAvgQP, uiTotalQP, uiTotalCnt);
    //CamOsPrintf("uiActAvgLambda_SCALED = %d\n", uiActAvgLambda_SCALED);

    ptRcPic->auiLevelLambda_SCALED[ptRcPic->picType] = uiActAvgLambda_SCALED;
    ptRcPic->auiLevelQp[ptRcPic->picType] = uiActAvgQP;
    ptRcPic->uiPicLambda_SCALED = uiActAvgLambda_SCALED;
    ptRcPic->uiPicQp = uiActAvgQP;
}

// CModel: estimatePicLambda
//    Return lambda_SCALED
static uint32 _EstPicLambdaByBits(ms_rc_top* ptMsRc, int picType, int picBit)
{

    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    uint32 picPelNum = ptMsRc->uiPicPixelNum;
    int alpha_SCALED = ptMsRc->atModelPara[picType].Alpha_SCALED;
    int beta_SCALED = ptMsRc->atModelPara[picType].Beta_SCALED;
    uint32 estLambda_SCALED, lastPicLambda_SCALED;
    uint64 uiremainder;

    uint64 picBit_SCALED = (uint64) picBit << PREC_SCALE_BITS;
    uint32 bpp_SCALED;
    uint32 IframeCplx;
    int32 tcPerPel;
    uint32 pow_tcPerPel_beta1_SCALED;
    uint32 uiInvBpp_SCALED;
    uint64 pow_InvBpp_beta;
    uint64 pow_tcPerPel_beta;
    uint64 pow_TcDivBit_beta;
    uint64 bpp_pow_beta;
    uint32 lastLevelLambda_SCALED;
    uint64 ui64EstLambda_SCALED;
    uint32 uiMinValue;
    uint32 uiMaxValue;
    uint32 uiMaxPicQp, uiMinPicQp;
    uint32 GlobalMinLambda_SCALED, GlobalMaxLambda_SCALED;

    CDBZ(picPelNum);
    bpp_SCALED = (uint32) CamOsMathDivU64(picBit_SCALED, picPelNum, &uiremainder);

    if(picType == I_SLICE)
    {
        IframeCplx = ptMsRc->uiIntraMdlCplx;

        CDBZ(picPelNum);
        tcPerPel = CamOsMathDivU64(((uint64) IframeCplx << PREC_SCALE_BITS), picPelNum, &uiremainder);
        pow_tcPerPel_beta1_SCALED = (uint32)MhefPow64(tcPerPel, BETA1_SCALED);

        CDBZ(bpp_SCALED);
        uiInvBpp_SCALED = (uint32)(CamOsMathDivU64(((uint64) PREC_SCALE_FACTOR << PREC_SCALE_BITS), bpp_SCALED, &uiremainder));

        pow_InvBpp_beta = MhefPow64(uiInvBpp_SCALED, beta_SCALED);
        pow_tcPerPel_beta = MhefPow64(pow_tcPerPel_beta1_SCALED, beta_SCALED);
        pow_TcDivBit_beta = (pow_tcPerPel_beta * pow_InvBpp_beta) >> PREC_SCALE_BITS;

        estLambda_SCALED = (uint32)((alpha_SCALED * pow_TcDivBit_beta) >> (PREC_SCALE_BITS + 8));
    }
    else // P_Slice
    {
        //double estLambda = alpha * pow(bpp, beta);
        bpp_pow_beta = MhefPow64(bpp_SCALED, beta_SCALED);
        ui64EstLambda_SCALED = (alpha_SCALED * bpp_pow_beta) >> PREC_SCALE_BITS;
        estLambda_SCALED = (ui64EstLambda_SCALED > (uint64)UINT_MAX) ? (uint32)UINT_MAX : (uint32)ui64EstLambda_SCALED;
    }

    lastPicLambda_SCALED = ptRcPic->uiPicLambda_SCALED;

    // If last frame is I and current frame is P
    ptRcPic->isFirstPpic = (ptRcPic->picType == I_SLICE && picType == P_SLICE) ? 1 : 0;

    if(ptMsRc->uiFrameCnt > ptMsRc->uiGopSize && ptRcPic->isFirstPpic == 0)
    {
        //lastLevelLambda = dClip3( 0.1, 10000.0, lastLevelLambda );
        //estLambda = dClip3( lastLevelLambda * pow( 2.0, -3.0/3.0 ), lastLevelLambda * pow( 2.0, 3.0/3.0 ), estLambda);

        lastLevelLambda_SCALED = ptRcPic->auiLevelLambda_SCALED[picType];
        lastLevelLambda_SCALED = uiClip3(PREC_SCALE_FACTOR / 10, PREC_SCALE_FACTOR * 10000, lastLevelLambda_SCALED);
        estLambda_SCALED = uiClip3(lastLevelLambda_SCALED >> 1, lastLevelLambda_SCALED << 1, estLambda_SCALED);
    }



    if(lastPicLambda_SCALED > 0)
    {
        // const uint32 uiMinValue = (lastPicLambda_SCALED*LAMBDA_MIN_CLIP_NUM)>>LAMBDA_MIN_CLIP_SHIFT;
        //const uint32 uiMaxValue = (lastPicLambda_SCALED*LAMBDA_MAX_CLIP_NUM)>>LAMBDA_MAX_CLIP_SHIFT;
        uiMinValue = ((int64) lastPicLambda_SCALED * ptMsRc->iLambdaMinClipMul) >> PREC_SCALE_BITS;
        uiMaxValue = ((int64) lastPicLambda_SCALED * ptMsRc->iLambdaMaxClipMul) >> PREC_SCALE_BITS;

        estLambda_SCALED = uiClip3(uiMinValue, uiMaxValue, estLambda_SCALED);
    }
    else
    {
        const uint32 uiMinValue = PREC_SCALE_FACTOR / 10;
        const uint32 uiMaxValue = PREC_SCALE_FACTOR * 10000;
        estLambda_SCALED = uiClip3(uiMinValue, uiMaxValue, estLambda_SCALED);
    }

    if(estLambda_SCALED < MIN_EST_LAMBA_SCALED)  // 0.1*65536
    {
        estLambda_SCALED = MIN_EST_LAMBA_SCALED;
    }

    // picture lambda clipping according to global picture Qp clipping
    //int iMaxPicQp = (eSliceType == I_SLICE) ? g_iRcIPicMaxQp : g_iRcPicMaxQp;
    //int iMinPicQp = (eSliceType == I_SLICE) ? g_iRcIPicMinQp : g_iRcPicMinQp;
    //double dGlobalMinLambda = QpToLambda(iMinPicQp - 0.5);
    //double dGlobalMaxLambda = QpToLambda(iMaxPicQp + 0.5);
    //estLambda = Clip3(dGlobalMinLambda, dGlobalMaxLambda, estLambda);
    {
        if(picType == I_SLICE)
        {
            uiMaxPicQp = g_uiRcIPicMaxQp;
            uiMinPicQp = g_uiRcIPicMinQp;
        }
        else
        {
            uiMaxPicQp = g_uiRcPicMaxQp;
            uiMinPicQp = g_uiRcPicMinQp;
        }

        GlobalMinLambda_SCALED = QpToLambdaScaled(uiMinPicQp - 1);
        GlobalMaxLambda_SCALED = QpToLambdaScaled(uiMaxPicQp + 1);
        estLambda_SCALED = uiClip3(GlobalMinLambda_SCALED, GlobalMaxLambda_SCALED, estLambda_SCALED);
    }

    return estLambda_SCALED;
}

// CModel: estimatePicQP
static uint32 _EstimatePicQpByLambda(ms_rc_top* ptMsRc, int picType, uint32 lambda_SCALED)
{
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    uint32 lastLevelQP;
    uint32 uiMinQp;
    uint32 uiMaxQp;
    uint32 QpMin;
    uint32 QpMax;
    uint32 lastPicQP;
    uint32 Qp;

    Qp = LambdaScaledToQp(lambda_SCALED);
    //int isFirstPpic = (ptRcPic->picType==I_SLICE && picType==P_SLICE) ? 1 : 0;
    lastPicQP = ptRcPic->uiPicQp;

    if((ptMsRc->uiFrameCnt > ptMsRc->uiGopSize) && (ptRcPic->isFirstPpic == 0))
    {
        lastLevelQP = ptRcPic->auiLevelQp[picType];
        uiMinQp = (lastLevelQP > g_uiRcPicMinQp + 3 ? lastLevelQP - 3 : g_uiRcPicMinQp);
        uiMaxQp = (lastLevelQP < g_uiRcPicMaxQp - 3 ? lastLevelQP + 3 : g_uiRcPicMaxQp);
        Qp = uiClip3(uiMinQp, uiMaxQp, Qp);
    }


    {
        QpMin = lastPicQP >= g_uiRcPicMinQp + g_uiRcPicClipRange ? lastPicQP - g_uiRcPicClipRange : g_uiRcPicMinQp;
        QpMax = lastPicQP <= g_uiRcPicMaxQp - g_uiRcPicClipRange ? lastPicQP + g_uiRcPicClipRange : g_uiRcPicMaxQp;
        Qp = uiClip3(QpMin, QpMax, Qp);
    }

    return Qp;
}

// CModel: getRefineBitsForIntra
static int _EstBitsForIntraFrame(ms_rc_top* ptMsRc)
{
    int iBufferRate = ptMsRc->iPicAvgBit;
    int iBuffUpperBound = ptMsRc->uiTargetBitrate * 9 / 10;
    int iInitBuffStatus = ptMsRc->uiTargetBitrate * 6 / 10;
    int iBuffDeviateRange = ptMsRc->uiTargetBitrate * 5 / 10;
    int iPreviousBitError = ptMsRc->iStreamBitErr;
    int iCurrBuffFullness = iInitBuffStatus + iPreviousBitError;
    uint64 uiremainder;

    // -- Adaptive PI ratio --
    int iTransPoint = iInitBuffStatus + ((iBuffUpperBound - iInitBuffStatus) / 2);

    int iCurrBuffDeviate = iClip3(0, iBuffDeviateRange, iCurrBuffFullness - iTransPoint);
    // Represent ratio by num. / denorm.
    int iCurrPILambdaRatioNum = (g_uiPILambdaRatioNum * iBuffDeviateRange) + (g_uiPILambdaRatioDenorm - g_uiPILambdaRatioNum) * iCurrBuffDeviate;
    int iCurrPILambdaRatioDenorm = g_uiPILambdaRatioDenorm * iBuffDeviateRange;

    // -- I frame bit allocation according to previous P frames --
    rc_model_para *ptRcMdlPara = &ptMsRc->atModelPara[I_SLICE];
    int alpha_SCALED = ptRcMdlPara->Alpha_SCALED;
    int beta_SCALED = ptRcMdlPara->Beta_SCALED;
    uint32 uiLastLambda_SCALED = ptMsRc->HistoryPLambda_SCALED;
    uint32 uiEstBpp_SCALED;

    // lambda scaling
    uint32 uiIntraLambda_SCALED;
    uint32 picPelNum;
    int iNum, iDenorm;
    int iCompensateBit;
    int iIntraBits, iEstBuffFullness;

    CDBZ(iCurrPILambdaRatioDenorm);
    uiIntraLambda_SCALED = CamOsMathDivU64((uint64) uiLastLambda_SCALED * iCurrPILambdaRatioNum, iCurrPILambdaRatioDenorm, &uiremainder);
    picPelNum = ptMsRc->uiPicPixelNum;

    MHE_MSG(MHE_MSG_DEBUG, "alpha_SCALED = %d \n", alpha_SCALED);
    MHE_MSG(MHE_MSG_DEBUG, "beta_SCALED = %d \n", beta_SCALED);

    uiEstBpp_SCALED = _LambdaToBpp_SCALED(ptMsRc, uiIntraLambda_SCALED, I_SLICE, 1);

    // Check default lambda ratio to fall with [0.1, 1.0]
    assert(g_uiPILambdaRatioNum <= g_uiPILambdaRatioDenorm); // <= 1.0
    assert(g_uiPILambdaRatioNum * 10 >= g_uiPILambdaRatioDenorm); // >= 0.1

    // Must not be the first frame
    assert(ptMsRc->uiFrameCnt > 0);

    // -- Allocated bit surpression --
    iIntraBits = ((uint64) uiEstBpp_SCALED * picPelNum) >> PREC_SCALE_BITS;
    iEstBuffFullness = iCurrBuffFullness - iBufferRate + iIntraBits;
    if(iEstBuffFullness > iBuffUpperBound)  // prevent buffer overflow
    {
        iCompensateBit = iBuffUpperBound + iBufferRate - iCurrBuffFullness;

        iCompensateBit = (iCompensateBit < MIN_PIC_BIT) ? MIN_PIC_BIT : iCompensateBit;

        //orgBitWgt = dClip3(0.5, 0.8, 1.25 - basePILambdaRatio);
        //iIntraBits = (int)(orgBitWgt*iIntraBits + (1.0-orgBitWgt)*iCompensateBit);
        iDenorm = g_uiPILambdaRatioDenorm;
        iNum = g_uiPILambdaRatioNum;
        iNum = iClip3(iDenorm >> 1, (iDenorm << 2) / 5, ((iDenorm * 5) >> 2) - iNum);
        CDBZ(iDenorm);
        iIntraBits = CamOsMathDivU64(((int64) iNum * iIntraBits + (iDenorm - iNum) * iCompensateBit), iDenorm, &uiremainder);

    }

    iIntraBits = iClip3(ptMsRc->uiMinIPicBit, ptMsRc->uiMaxIPicBit, iIntraBits);

    return iIntraBits;
}



// Partial from CModel: compressGOP
// Input: Qp
// Output: lambda (scaled)
static void _Est1stFrameParamByQp(ms_rc_top* ptMsRc, int picType, uint32 uiQp, int isRTcModel, int* p_retBit, uint32 *p_retLambda_SCALED)
{
    //ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;

// Replaced by C-Model generated g_DefaultLambdaAndBppByQp
    /*
     int NumberBFrames = 0;
     double dLambda_scale = 1.0 - dClip3( 0.0, 0.5, 0.05*(double)NumberBFrames);
     double dQPFactor = (picType==I_SLICE) ? 0.57*dLambda_scale : dLambda_scale;
     const int SHIFT_QP = 12;
     double qp_temp = (double)Qp - SHIFT_QP;//(Double) sliceQP + bitdepth_luma_qp_scale - SHIFT_QP;
     double lambda = dQPFactor*pow( 2.0, qp_temp/3.0 );

     double bpp = LambdaToBpp(ptMsRc, lambda, picType, isRTcModel);
     int bits = (int)(ptMsRc->uiPicPixelNum*bpp);

     *p_retBit = bits;
     *p_retLambda = lambda;
     */
    uint32 uiLambda_SCALED;
    uint32 uiBpp_SCALED;
    uint32 uiBits;

    uiLambda_SCALED = g_DefaultLambdaByQp[uiQp];
    //CamOsPrintf("> [%s %d] uiLambda_SCALED = %d\n", __FUNCTION__, __LINE__, uiLambda_SCALED);
    uiBpp_SCALED = _LambdaToBpp_SCALED(ptMsRc, uiLambda_SCALED, picType, isRTcModel);
    uiBits = ((uint64) ptMsRc->uiPicPixelNum * uiBpp_SCALED) >> PREC_SCALE_BITS;

    *p_retBit = uiBits;
    *p_retLambda_SCALED = uiLambda_SCALED;

}

static void _msrc_gop_start(ms_rc_top* ptMsRc, msrc_rqc* pMsRc)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
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

    MHE_MSG(MHE_MSG_DEBUG, "iBitErrCompensate = %d \n", iBitErrCompensate);

    // CBR Mode
    iBitCompensLimit = (int)(iPicAvgBit * g_iGOPCompenRatioLimit_MUL100) / 100;
    MHE_MSG(MHE_MSG_DEBUG, "iBitCompensLimit = %d \n", iBitCompensLimit);

    if(pMsRc->i_method == RQCT_MODE_CBR)
        iBitErrCompensate = iClip3(-iBitCompensLimit, iBitCompensLimit, iBitErrCompensate);

    MHE_MSG(MHE_MSG_DEBUG, "iBitErrCompensate = %d \n", iBitErrCompensate);
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

    MHE_MSG(MHE_MSG_DEBUG, "iGopBitLeft = %d \n", ptRcGop->iGopBitLeft);
    MHE_MSG(MHE_MSG_DEBUG, "iPicAllocBit = %d \n", ptRcGop->iPicAllocBit);
    MHE_MSG(MHE_MSG_DEBUG, "iGopFrameLeft = %d \n", ptRcGop->iGopFrameLeft);
}

static void _msrc_pic_start(ms_rc_top* ptMsRc, int picType)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;

    int picTargetBit;
    uint32 uiPicQp, picLambda_SCALED;
    int gopAvgPicBit = ptRcGop->iPicAllocBit;

    const uint32 g_uiWeightOrgPicBit_SCALED = (1 << (PREC_SCALE_BITS - 1));

    if(ptMsRc->uiFrameCnt == 0)
    {
        uiPicQp = ptMsRc->uiInitialQp;

        _Est1stFrameParamByQp(ptMsRc, picType, uiPicQp, 1, &picTargetBit, &picLambda_SCALED);

        goto SET_RC_PARA;
    }

    if(picType == I_SLICE)
    {
        picTargetBit = _EstBitsForIntraFrame(ptMsRc);
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
    }

    picLambda_SCALED = _EstPicLambdaByBits(ptMsRc, picType, picTargetBit);
    uiPicQp = _EstimatePicQpByLambda(ptMsRc, picType, picLambda_SCALED);

    // ----------------------------
    // update status
SET_RC_PARA:
    ptRcPic->iTargetBit = picTargetBit;
    ptRcPic->picType = picType;

    assert(picLambda_SCALED >= 0);  // No reason to have lambda < 0

    ptRcPic->auiLevelLambda_SCALED[picType] = picLambda_SCALED;
    ptRcPic->uiPicLambda_SCALED = picLambda_SCALED;
    ptRcPic->auiLevelQp[picType] = uiPicQp;
    ptRcPic->uiPicQp = uiPicQp;

    MHE_MSG(MHE_MSG_DEBUG, "picLambda_SCALED = %d\n", picLambda_SCALED);
    MHE_MSG(MHE_MSG_DEBUG, "uiPicQp = %d\n", uiPicQp);

    if(uiPicQp < QP_MIN || uiPicQp > QP_MAX)
        CamOsPrintf("========== Warring!!! uiPicQp is over range !!! ===========\n");
}

static void _msrc_update_stats(ms_rc_top* ptMsRc, int picEncBit)
{
    ms_rc_gop *ptRcGop = &ptMsRc->tRcGop;
    //ms_rc_pic  *ptRcPic = &ptMsRc->tRcPic;
    //int picType = ptRcPic->picType;

    _calcAvgLambdaFromHist(ptMsRc);

    // Convert from sum-of-16x16average to sum-of-pixel
    ptMsRc->uiIntraMdlCplx = g_iHWTextCplxAccum << 8;

    ptMsRc->iStreamBitErr += (picEncBit - ptMsRc->iPicAvgBit);
    ptRcGop->iGopBitLeft -= picEncBit;

    ptMsRc->uiFrameCnt++;
    ptRcGop->iGopFrameLeft--;

#if 0
    ptMsRc->iStreamBitErr = 0;
    MHE_MSG(MHE_MSG_DEBUG, "ptMsRc->iStreamBitErr = %d\n", ptMsRc->iStreamBitErr);
#endif

    MHE_MSG(MHE_MSG_DEBUG, "ptRcGop->iGopBitLeft = %d\n", ptRcGop->iGopBitLeft);
}

// lambda-bpp model:
//   I-frame: Lambda = (Alhpa/256) * (TC/Bpp)^Beta
//   P-frame: Lambda = Alpha * Bpp^Beta
// Update:
//   dAlpha, dBeta
//   dLastAlpha, dLastBeta, dLastAlphaIncr, dLastBetaIncr
//   dHitoryPLambda
static void _msrc_update_model(ms_rc_top* ptMsRc, int picActualBit, int picType)
{
    rc_model_para *ptRcMdlPara = &ptMsRc->atModelPara[picType];
    ms_rc_pic *ptRcPic = &ptMsRc->tRcPic;
    int alpha_SCALED = ptRcMdlPara->Alpha_SCALED;
    int beta_SCALED = ptRcMdlPara->Beta_SCALED;
    uint32 picPelNum = ptMsRc->uiPicPixelNum;
    uint64 uiremainder;
    int64 remainder;
    int picTargetBit;
    uint32 IframeCplx;
    uint32 CplxPerPel_SCALE;
    uint32 pow_cplxperpel_SCALED;
    int lnbpp_SCALED;
    uint32 actbit_div_targetbit;
    int64 log_actbit_div_targetbit;
    int iDiffLambda_SCALED;
    int AlphaIncr_SCALED, BetaIncr_SCALED, delAlpha_SCALED, delBeta_SCALED;
    int64 IncrTerm_SCALED;
    uint32 picActualBpp;
    uint32 uiCalLambda_SCALED;
    uint32 uiInputLambda_SCALED;
    uint32 uiStepSizeA_SCALED;
    uint32 uiStepSizeB_SCALED;
    int iAlphaScaleR;
    int iBetaScaleR;
    int64 alphaScale;
    int64 betaScale;
    int alphaUpdate_SCALED;
    int betaUpdate_SCALED;
    int lastDeltaAlpha_SCALED;
    int lastDeltaBeta_SCALED;
    int lastDelAlphaIncr_SCALED;
    int lastDelBetaIncr_SCALED;
    int STEP_LIMIT_A_SCALED;
    int STEP_LIMIT_B_SCALED;
    uint32 uiMaxCalLambdaClip;
    uint64 ui64CalLambda_SCALED;

    if(picType == I_SLICE)
    {
        picTargetBit = ptRcPic->iTargetBit;
        IframeCplx = ptMsRc->uiIntraMdlCplx;
        CDBZ(picPelNum);
        CplxPerPel_SCALE = CamOsMathDivU64((uint64) IframeCplx << PREC_SCALE_BITS, picPelNum, &uiremainder);

        //double lnbpp = log(pow(IframeCplx/picPelNum, g_bRcICostExpt));
        //double diffLambda = beta*(log(picActualBits)-log(picTargetBit));
        CDBZ(CplxPerPel_SCALE);
        pow_cplxperpel_SCALED = (uint32)MhefPow64(CplxPerPel_SCALE, BETA1_SCALED);
        CDBZ(pow_cplxperpel_SCALED);
        lnbpp_SCALED = MhefLog(pow_cplxperpel_SCALED);

        CDBZ(picTargetBit);
        actbit_div_targetbit = (uint32) CamOsMathDivU64((uint64) picActualBit << PREC_SCALE_BITS, picTargetBit, &uiremainder);
        log_actbit_div_targetbit = MhefLog(actbit_div_targetbit);
        iDiffLambda_SCALED = (int)((beta_SCALED * (log_actbit_div_targetbit)) >> PREC_SCALE_BITS);

        //diffLambda = dClip3(-0.125, 0.125, 0.25*diffLambda);
        //alpha = alpha*exp(diffLambda);
        //beta = beta + diffLambda / lnbpp;
        iDiffLambda_SCALED = iClip3(-8192, 8192, iDiffLambda_SCALED / 4); // NEED_CHANGE_BY_PREC_SCALE_BITS
        alpha_SCALED = (alpha_SCALED * (uint64) MhefExp(iDiffLambda_SCALED)) >> PREC_SCALE_BITS;
        CDBZ(lnbpp_SCALED);
        beta_SCALED = beta_SCALED + CamOsMathDivS64((int64) iDiffLambda_SCALED << PREC_SCALE_BITS, lnbpp_SCALED, &remainder);
    }
    else
    {

        CDBZ(picPelNum);
        picActualBpp = (uint32) CamOsMathDivU64((uint64) picActualBit << PREC_SCALE_BITS, picPelNum, &uiremainder);

        lnbpp_SCALED = MhefLog(picActualBpp);

        ui64CalLambda_SCALED = ((int64)alpha_SCALED * MhefPow64(picActualBpp, beta_SCALED)) >> PREC_SCALE_BITS;
        uiCalLambda_SCALED = (ui64CalLambda_SCALED > (uint64)UINT_MAX) ? (uint32)UINT_MAX : (uint32)ui64CalLambda_SCALED;
        //CDBZ(picActualBpp);
        //uiCalLambda_SCALED = (uint32) (((int64) alpha_SCALED * fPow(picActualBpp, beta_SCALED)) >> PREC_SCALE_BITS);
        uiInputLambda_SCALED = ptRcPic->uiPicLambda_SCALED;
        uiStepSizeA_SCALED = ptMsRc->uiStepAlpha_SCALED;
        uiStepSizeB_SCALED = ptMsRc->uiStepBeta_SCALED;

        //  escape Method for in-accurate pow()
        if(picActualBpp <= 131)  // 0.002*65536 // NEED_CHANGE_BY_PREC_SCALE_BITS
        {
            /*
             alpha *= ( 1.0 - m_encRCSeq->getAlphaUpdate() / 2.0 );
             beta  *= ( 1.0 - m_encRCSeq->getBetaUpdate() / 2.0 );
             */
            iAlphaScaleR = uiStepSizeA_SCALED >> 1;
            iBetaScaleR = uiStepSizeB_SCALED >> 1;
            alphaScale = PREC_SCALE_FACTOR + ((uiInputLambda_SCALED > uiCalLambda_SCALED) ? iAlphaScaleR : -iAlphaScaleR);
            betaScale = PREC_SCALE_FACTOR + ((uiInputLambda_SCALED > uiCalLambda_SCALED) ? iBetaScaleR : -iBetaScaleR);

            alphaUpdate_SCALED = (int)(((int64) alpha_SCALED * alphaScale) >> PREC_SCALE_BITS);
            betaUpdate_SCALED = (int)(((int64) beta_SCALED * betaScale) >> PREC_SCALE_BITS);

            alpha_SCALED = iClip3(g_iRCAlphaMinValue_SCALED, g_iRCAlphaMaxValue_SCALED, alphaUpdate_SCALED);
            beta_SCALED = iClip3(g_iRCBetaMinValue_SCALED, g_iRCBetaMaxValue_SCALED, betaUpdate_SCALED);

            goto UPDATE_PARAMETERS;
        }
        //calLambda = dClip3(inputLambda/10.0, inputLambda*10.0, calLambda);
        //AlphaIncr = (log(inputLambda) - log(calLambda)) * alpha;
        uiMaxCalLambdaClip = ((uiInputLambda_SCALED >> PREC_SCALE_BITS) < ((uint32)PREC_SCALE_FACTOR / 10)) ? uiInputLambda_SCALED * 10 : ((uint32)PREC_SCALE_FACTOR / 10) << PREC_SCALE_BITS;

        uiCalLambda_SCALED = uiClip3(uiInputLambda_SCALED / 10, uiMaxCalLambdaClip, uiCalLambda_SCALED);

        IncrTerm_SCALED = (int64)(MhefLog(uiInputLambda_SCALED) - MhefLog(uiCalLambda_SCALED));

        CDBZ(PREC_SCALE_FACTOR);
        AlphaIncr_SCALED = (int) CamOsMathDivS64((IncrTerm_SCALED * alpha_SCALED), PREC_SCALE_FACTOR, &remainder);

        //lnbpp = dClip3(-5.0, -0.1, lnbpp );
        //BetaIncr = (log(inputLambda) - log(calLambda)) * lnbpp;
        lnbpp_SCALED = iClip3(-(PREC_SCALE_FACTOR * 5), -(PREC_SCALE_FACTOR / 10), lnbpp_SCALED);
        CDBZ(PREC_SCALE_FACTOR);
        BetaIncr_SCALED = (int) CamOsMathDivS64((IncrTerm_SCALED * lnbpp_SCALED), PREC_SCALE_FACTOR, &remainder);

        // -- adaptive step size decision --
        if(ptRcPic->isFirstPpic == 0)
        {
            lastDeltaAlpha_SCALED = alpha_SCALED - ptMsRc->LastAlpha_SCALED;
            lastDeltaBeta_SCALED = beta_SCALED - ptMsRc->LastBeta_SCALED;
            lastDelAlphaIncr_SCALED = AlphaIncr_SCALED - ptMsRc->LastAlphaIncr_SCALED;
            lastDelBetaIncr_SCALED = BetaIncr_SCALED - ptMsRc->LastBetaIncr_SCALED;

            STEP_LIMIT_A_SCALED = uiStepSizeA_SCALED >> 2;
            STEP_LIMIT_B_SCALED = uiStepSizeB_SCALED >> 2;

            // numerical regulator: add "1" to prevent divide 0
            lastDelAlphaIncr_SCALED = iabs(lastDelAlphaIncr_SCALED) + 1;
            lastDelBetaIncr_SCALED = iabs(lastDelBetaIncr_SCALED) + 1;
            CDBZ(lastDelAlphaIncr_SCALED);
            uiStepSizeA_SCALED = iabs((int)(CamOsMathDivS64((int64) lastDeltaAlpha_SCALED << PREC_SCALE_BITS, lastDelAlphaIncr_SCALED, &remainder)));
            CDBZ(lastDelBetaIncr_SCALED);
            uiStepSizeB_SCALED = iabs((int)(CamOsMathDivS64((int64) lastDeltaBeta_SCALED << PREC_SCALE_BITS, lastDelBetaIncr_SCALED, &remainder)));

            uiStepSizeA_SCALED = iClip3(STEP_LIMIT_A_SCALED >> 2, STEP_LIMIT_A_SCALED << 2, uiStepSizeA_SCALED);
            uiStepSizeB_SCALED = iClip3(STEP_LIMIT_B_SCALED >> 2, STEP_LIMIT_B_SCALED << 2, uiStepSizeB_SCALED);
        }
        // update
        ptMsRc->LastAlpha_SCALED = alpha_SCALED;
        ptMsRc->LastBeta_SCALED = beta_SCALED;
        ptMsRc->LastAlphaIncr_SCALED = AlphaIncr_SCALED;
        ptMsRc->LastBetaIncr_SCALED = BetaIncr_SCALED;

        CDBZ(PREC_SCALE_FACTOR);
        delAlpha_SCALED = CamOsMathDivS64(((int64) uiStepSizeA_SCALED * AlphaIncr_SCALED), PREC_SCALE_FACTOR, &remainder);
        CDBZ(PREC_SCALE_FACTOR);
        delBeta_SCALED = CamOsMathDivS64(((int64) uiStepSizeB_SCALED * BetaIncr_SCALED), PREC_SCALE_FACTOR, &remainder);

        // ---------------------------------------------------
        alpha_SCALED += delAlpha_SCALED;
        beta_SCALED += delBeta_SCALED;

        alpha_SCALED = iClip3(g_iRCAlphaMinValue_SCALED, g_iRCAlphaMaxValue_SCALED, alpha_SCALED);
        beta_SCALED = iClip3(g_iRCBetaMinValue_SCALED, g_iRCBetaMaxValue_SCALED, beta_SCALED);
    }

UPDATE_PARAMETERS:

    CDBZ(alpha_SCALED);
    CDBZ(beta_SCALED);

    ptRcMdlPara->Alpha_SCALED = alpha_SCALED;
    ptRcMdlPara->Beta_SCALED = beta_SCALED;

    if(picType == P_SLICE)
    {
        uint32 currLambda_SCALED = iClip3(PREC_SCALE_FACTOR / 10, 10000 << PREC_SCALE_BITS, ptRcPic->auiLevelLambda_SCALED[P_SLICE]);
        uint32 updateLambda_SCALED;

        if(ptRcPic->isFirstPpic)
        {
            updateLambda_SCALED = currLambda_SCALED;
        }
        else
        {
            updateLambda_SCALED = (g_iRCWeightHistoryLambda_MUL10 * ptMsRc->HistoryPLambda_SCALED + (10 - g_iRCWeightHistoryLambda_MUL10) * currLambda_SCALED) / 10;
        }
        ptMsRc->HistoryPLambda_SCALED = updateLambda_SCALED;
    }
}
