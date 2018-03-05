#include "hal_mhe_ops.h"

#define _ALIGN(b,x)     (((x)+(1<<(b))-1)&(~((1<<(b))-1)))
#define _BITS_(s)       ((2<<(1?s))-(1<<(0?s)))
#define _MAX_(a,b)      ((a)>(b)?(a):(b))
#define _MIN_(a,b)      ((a)<(b)?(a):(b))

#define MVBLK_DEFAULT   \
    (MHVE_INTER_SKIP|MHVE_INTER_16x16|MHVE_INTER_16x8|MHVE_INTER_8x16|\
     MHVE_INTER_8x8|MHVE_INTER_8x4|MHVE_INTER_4x8|MHVE_INTER_4x4)

#define MHVE_FLAGS_CTRL (MHVE_FLAGS_FORCEI|MHVE_FLAGS_DISREF)

#define CTB_SIZE    32
#define CROP_UNIT_X 2
#define CROP_UNIT_Y 2

#define HEVC_HEADER_BY_FDC

#define INT_PREC        0
#define HALF_PREC       1
#define QUARTER_PREC    2

// Mode penalties when RDO cost comparison
//#define PENALTY_BITS   (16)
#define PENALTY_SHIFT  (2)

#if 0
#define HW_ME_WINDOW_X  (32)
#define HW_ME_WINDOW_Y  (16)
#else
#define HW_ME_WINDOW_X  (96)
#define HW_ME_WINDOW_Y  (48)
#endif

#define HEV_ME_4X4_DIS    (1<<0)
#define HEV_ME_8X4_DIS    (1<<1)
#define HEV_ME_4X8_DIS    (1<<2)
#define HEV_ME_16X8_DIS   (1<<3)
#define HEV_ME_8X16_DIS   (1<<4)
#define HEV_ME_8X8_DIS    (1<<5)
#define HEV_ME_16X16_DIS  (1<<6)

// These are only used when g_bNewMSME=1
// Define the center "static" area
#define HW_ME_STATIC_X   (16)
#define HW_ME_STATIC_Y   (16)

#ifdef MODULE_PARAM_SUPPORT
extern unsigned int tmvp_enb;
#endif

static inline int imin(int a, int b)
{
    return ((a) < (b)) ? (a) : (b);
}

static inline int imax(int a, int b)
{
    return ((a) > (b)) ? (a) : (b);
}

static inline int iClip3(int low, int high, int x)
{
    x = imax(x, low);
    x = imin(x, high);
    return x;
}

//------------------------------------------------------------------------------
//  Function    : _RqctOps
//  Description : Get rqct_ops handle.
//------------------------------------------------------------------------------
static void* _RqctOps(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    return mhe->p_rqct;
}

//------------------------------------------------------------------------------
//  Function    : _PmbrOps
//  Description : Get pmbr_ops handle.
//------------------------------------------------------------------------------
static void* _PmbrOps(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    return mhe->p_pmbr;
}

//------------------------------------------------------------------------------
//  Function    : _MheJob
//  Description : Get mhve_job object.
//------------------------------------------------------------------------------
static void* _MheJob(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    return mhe->p_regs;
}

//------------------------------------------------------------------------------
//  Function    : __OpsFree
//  Description : Release this object.
//------------------------------------------------------------------------------
static void __OpsFree(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    rqct_ops* rqct = mhe->p_rqct;
    pmbr_ops* pmbr = mhe->p_pmbr;
    mhe_reg* regs = mhe->p_regs;
    h265_enc* h265 = mhe->h265;

    if(regs)
        MEM_FREE(regs);
    if(rqct)
        rqct->release(rqct);
    if(h265)
        h265->release(h265);
    if(pmbr)
        pmbr->release(pmbr);
    MEM_FREE(mops);
}

//------------------------------------------------------------------------------
//  Function    : _SeqDone
//  Description : Finish sequence encoding.
//------------------------------------------------------------------------------
static int _SeqDone(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    rqct_ops* rqct = mhe->p_rqct;
    pmbr_ops* pmbr = mhe->p_pmbr;

    /* RQCT : Do nothing now */
    rqct->seq_done(rqct);
    /* PMBR : Do nothing now */
    pmbr->seq_done(pmbr);

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : _SeqSync
//  Description : Start sequence encoding.
//------------------------------------------------------------------------------
static int _SeqSync(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    rqct_ops* rqct = mhe->p_rqct;
    pmbr_ops* pmbr = mhe->p_pmbr;
    mhve_job* mjob = (mhve_job*)mhe->p_regs;
    h265_enc* h265 = mhe->h265;
    pps_t* pps;
    rqct_cfg  rqcf;
    int err = 0;

    /* Initialize H265 Header parameters */
    if(0 != (err = h265_seq_init(h265, mhe->i_rpbn)))
    {
        mops->seq_done(mops);

        return err;
    }
    mhe->i_seqn = 0;
    mhe->b_seqh = OPS_SEQH_START;

    /* RQCT : Reset encoded picture count */
    rqct->seq_sync(rqct);

    /* PMBR : Reset encoded picture count and Initialize global attribute */
    pmbr->seq_sync(pmbr, mjob);

    /* Reset statistic data */
    mhe->i_obits = 0;
    mhe->i_total = 0;

    rqcf.type = RQCT_CFG_SEQ;
    rqct->get_rqcf(rqct, &rqcf);

    //reference C-mdoel
    /*    if ( m_RCEnableRateControl )
        {
          m_cPPS.setUseDQP(true);
          m_cPPS.setMaxCuDQPDepth( 0 );
          m_cPPS.setMinCuDQPSize( m_cPPS.getSPS()->getMaxCUWidth() >> ( m_cPPS.getMaxCuDQPDepth()) );
        }*/
    pps = h265_find_set(h265, HEVC_PPS, 0);
    if(rqcf.seq.i_method != RQCT_MODE_CQP)
        pps->b_cu_qp_delta_enabled = 1;
    else
        pps->b_cu_qp_delta_enabled = 0;

#ifdef MODULE_PARAM_SUPPORT
    if(tmvp_enb)
        pps->p_sps->b_temporal_mvp_enable = 1;
    else
        pps->p_sps->b_temporal_mvp_enable = 0;
#endif

    return err;

}

//------------------------------------------------------------------------------
//  Function    : _EncBuff
//  Description : Enqueue video buffer.
//                Prepare the VPS/SPS/PPS/Slice Header before encode.
//                Reset the Reconstructed/Reference buffer attribute.
//------------------------------------------------------------------------------
static int _EncBuff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    rqct_ops* rqct = mhe->p_rqct;
    h265_enc* h265 = mhe->h265;
    mhve_job* mjob = (mhve_job*)mhe->p_regs;
    pmbr_ops* pmbr = mhe->p_pmbr;
    pmbr_cfg  pmbrcfg;
    rqct_cfg  rqcf;
    rqct_buf  rqcb;
    int type, rpsi = 0;
    int err = 0, idx;
    int i, roiidx, maxqp, minqp;
    slice_t *sh = h265_find_set(h265, HEVC_SLICE, 0);

    /* Link input video buffer */
    mhe->m_encp = *mvpb;
    mvpb = &mhe->m_encp;

    if(mhe->b_seqh & OPS_SEQH_START)
    {
        rqct->seq_sync(rqct);
        rqct->seq_conf(rqct);
        //h265->m_pps.i_init_qp = rqct->i_enc_qp;   //derek check
        mhe->b_seqh = OPS_SEQH_RESET;
    }
    else if(mhe->b_seqh & OPS_SEQH_RESET)
    {
        rqct->seq_sync(rqct);
    }

    if(mvpb->u_flags & MHVE_FLAGS_FORCEI)
        rqct->seq_sync(rqct);

    if(mvpb->u_flags & MHVE_FLAGS_DISREF)
        rqct->b_unrefp = 1;

    rqcb.u_config = 0;
    /* RQCT : Decide current picture type (I,P,B) */
    if(0 != (err = rqct->enc_buff(rqct, &rqcb)))
        return err;

    if(mhe->b_seqh & OPS_SEQH_RESET)
    {
        h265_seq_conf(h265);
        mhe->b_seqh = OPS_SEQH_WRITE;
    }

    /* RQCT : Update QP, MBR, penalty setting */
    rqct->enc_conf(rqct, mjob);

    rqcf.type = RQCT_CFG_QPR;
    rqct->get_rqcf(rqct, &rqcf);

    maxqp = IS_IPIC(rqct->i_pictyp) ? (rqcf.qpr.i_iupperq) : (rqcf.qpr.i_pupperq);
    minqp = IS_IPIC(rqct->i_pictyp) ? (rqcf.qpr.i_ilowerq) : (rqcf.qpr.i_plowerq);

    /* PMBR : If ROI enable, disable PMBR */
    rqcf.type = RQCT_CFG_ROI;
    rqcf.roi.i_roiidx = 0;
    rqct->get_rqcf(rqct, &rqcf);
    //CamOsPrintf("Check ROI enable = 0x%x\n", rqcf.roi.u_roienb);

    //Disable PMBR
    pmbrcfg.type = RQCT_CFG_SEQ;
    pmbr->get_conf(pmbr, &pmbrcfg);

    //If ROI enabled, disable PMBR
    if(rqcf.roi.u_roienb)
    {
        pmbrcfg.seq.i_enable = 0;
        pmbr->set_conf(pmbr, &pmbrcfg);

        //Fill ROI QP to PMBR LUT
        for(i = 0; i < RQCT_ROI_NR; i++)
        {
            rqcf.type = RQCT_CFG_ROI;
            rqcf.roi.i_roiidx = i;
            rqct->get_rqcf(rqct, &rqcf);

            pmbr->i_LutRoiQp[i] = 0;

            if(rqcf.roi.i_roiqp != 0)
            {
                roiidx = i == 7 ? 8 : i;

                if(rqcf.roi.i_absqp)
                    pmbr->i_LutRoiQp[roiidx] = iClip3(minqp, maxqp, rqcf.roi.i_roiqp);
                else    //Offset QP
                    pmbr->i_LutRoiQp[roiidx] = iClip3(minqp, maxqp, rqct->i_enc_qp + rqcf.roi.i_roiqp);

//                CamOsPrintf("%d: (%d %d) - (%d %d) = %d\n", roiidx, minqp, maxqp,
//                        rqct->i_enc_qp, rqcf.roi.i_roiqp,
//                        pmbr->i_LutRoiQp[roiidx]);
            }
        }

        pmbr->i_LutTyp = 2;
    }
    else
    {
        pmbrcfg.seq.i_enable = 1;
        pmbr->set_conf(pmbr, &pmbrcfg);
        pmbr->i_LutTyp = 0;
    }

    /* PMBR : Update QP, LUT setting */
    rqcf.type = RQCT_CFG_SEQ;
    rqct->get_rqcf(rqct, &rqcf);
    if(rqcf.seq.i_method == RQCT_MODE_CQP)
        pmbr->i_LutTyp = 1;

    pmbr->i_FrmQP = rqct->i_enc_qp;
    pmbr->i_FrmLamdaScaled = rqct->i_enc_lamda;
    pmbr->i_PicTyp = rqct->i_pictyp;
    pmbr->i_MaxQP = maxqp;
    pmbr->i_MinQP = minqp;
    pmbr->enc_conf(pmbr, mjob);

    /* Update Slice header parameters and write to bitstream buffer */
    if(IS_IPIC(rqct->i_pictyp))
    {
        sh->i_slice_type = SLICE_TYPE_I;

        type = HEVC_ISLICE;
        mvpb->u_flags = MHVE_FLAGS_FORCEI;

        //write sequence header:VPS,SPS,PPS for each I frame
        mhe->b_seqh = OPS_SEQH_WRITE;
    }
    else
    {
        sh->i_slice_type = SLICE_TYPE_P;

        type = HEVC_PSLICE;
        if(mvpb->u_flags & MHVE_FLAGS_DISREF)
            rpsi = !rqct->b_unrefp;
    }

    sh->i_qp = rqct->i_enc_qp;
    h265->i_picq = rqct->i_enc_qp;

    //MultiSlice
    if(mhe->i_rows > 0)
    {
        sh = h265_find_set(h265, HEVC_SLICE, 0);
        sh->slice_loop_filter_across_slices_enabled_flag = 0;
    }

    if(rqct->b_unrefp == 0)
        mvpb->u_flags &= ~MHVE_FLAGS_DISREF;

    if(0 <= (idx = h265_enc_buff(h265, type, !rqct->b_unrefp)))
    {
        rpb_t* rpb = mhe->m_rpbs + idx;
        rpb->i_index = mvpb->i_index;
        if(!rpb->b_valid)
        {
            rpb->u_phys[RPB_YPIX] = mvpb->planes[0].u_phys;
            rpb->u_phys[RPB_CPIX] = mvpb->planes[1].u_phys;
            mvpb->i_index = -1;
        }
    }

    rqcf.type = RQCT_CFG_SEQ;
    rqct->get_rqcf(rqct, &rqcf);
    //    sh->i_poc = rqct->i_enc_nr%rqcf.seq.i_period;
    if(rqcf.seq.i_period != 1)  //fixed for I-only
        sh->i_poc = h265->i_poc;

    //CamOsPrintf("[%s %d]rqcf.seq.i_period = %d, h265->i_poc = %d, sh->i_poc = %d\n", __FUNCTION__, __LINE__ , rqcf.seq.i_period, h265->i_poc, sh->i_poc);

    h265->i_swsh = h265_sh_writer(h265);

    if(rqcf.seq.i_period == 1)  //fixed for I-only
        sh->i_poc++;

    return err;
}

//------------------------------------------------------------------------------
//  Function    : _DeqBuff
//  Description : Dequeue video buffer which is encoded.
//------------------------------------------------------------------------------
static int _DeqBuff(mhve_ops* mops, mhve_vpb* mvpb)
{
    mhe_ops* mhe = (mhe_ops*)mops;

    *mvpb = mhe->m_encp;

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : _EncDone
//  Description : Finish encode one frame.
//                Update statistic data.
//                Update reference buffer list.
//------------------------------------------------------------------------------
static int _EncDone(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    h265_enc* h265 = mhe->h265;
    rqct_ops* rqct = mhe->p_rqct;
    pmbr_ops* pmbr = mhe->p_pmbr;
    mhe_reg* regs = mhe->p_regs;
    mhve_vpb* vpb = &mhe->m_encp;
    pmbr_cfg  pmbrcfg;
    int idx, err = 0;

#ifdef ENABLE_DUMP_REG
    // Parse dump register here
    int i = 0;
    mhe_dump_reg* dump_reg;

    dump_reg = (mhe_dump_reg *)mhe->p_drvptr;

    regs->hev_bank1.reg6d = dump_reg->hev_bank1_reg6d;    //REGRD(hev_base1, 0x6d, "bits size:lo");
    regs->hev_bank1.reg6e = dump_reg->hev_bank1_reg6e;;   //REGRD(hev_base1, 0x6e, "bits size:hi");
    regs->enc_bitcnt = ((uint)(regs->hev_bank1.reg_ro_hev_ec_bsp_bit_cnt_high) << 16) + regs->hev_bank1.reg_ro_hev_ec_bsp_bit_cnt_low;
    regs->mjob.i_tick = (int)(regs->enc_cycles);
    regs->mjob.i_bits = (int)(regs->enc_bitcnt - regs->bits_delta); // Get the bit counts of pure bitstream and slice header.

    for(i = 0; i < PMBR_LUT_SIZE; i++)
    {
        regs->pmbr_lut_hist[i] = dump_reg->hev_bank1_reg16[i];
    }

    for(i = 0; i < (1 << PMBR_LOG2_HIST_SIZE); i++)
    {
        regs->pmbr_tc_hist[i] = dump_reg->hev_bank2_reg20[i];
        regs->pmbr_pc_hist[i] = dump_reg->hev_bank2_reg00[i];
    }

    regs->pmbr_tc_accum = ((uint)(dump_reg->hev_bank1_reg29) << 16) + dump_reg->hev_bank1_reg2a_low;

    //initial dump register, avoid no dump register when buffer full
    memset(dump_reg, 0, sizeof(mhe_dump_reg));

#endif

    if(CHECK_IRQ_STATUS(regs->irq_status, IRQ_MARB_BSPOBUF_FULL) ||
            (regs->hev_bank1.reg6d == 0 && regs->hev_bank1.reg6d == 0))
    {
        MHE_MSG(MHE_MSG_ERR, "> MHE buffer full \n");
        return -1;
    }

    pmbrcfg.type = PMBR_CFG_LUT;
    pmbr->get_conf(pmbr, &pmbrcfg);

    rqct->auiQps = pmbrcfg.lut.p_kptr[PMBR_LUT_QP];
    rqct->auiBits = pmbrcfg.lut.p_kptr[PMBR_LUT_TARGET];
    rqct->auiLambdas_SCALED = pmbrcfg.lut.p_kptr[PMBR_LUT_LAMBDA_SCALED];
    rqct->aiIdcHist = pmbrcfg.lut.p_kptr[PMBR_LUT_AIIDCHIST];

    /* PMBR : Update statistic data */
    if((err = pmbr->enc_done(pmbr, &regs->mjob)))
    {
        // TBD
    }

    /* feedback to rate-controller */
    /* RQCT : Update statistic data */
    if((err = rqct->enc_done(rqct, &regs->mjob)))
    {
        rpb_t* rpb = &mhe->m_rpbs[h265->p_recn->i_id];

        mhe->i_obits = mhe->i_total = mhe->u_oused = 0;

        if(!rpb->b_valid)
        {
            vpb->i_index = rpb->i_index;
            rpb->i_index = -1;
        }
        return err;
    }

    /* Update statistic data */
    mhe->i_obits = (rqct->i_bitcnt);
    mhe->i_total += (rqct->i_bitcnt) / 8;
    mhe->u_oused += (rqct->i_bitcnt + regs->bits_delta) / 8;

    if(0 <= (idx = h265_enc_done(h265)))
    {
        rpb_t* rpb = mhe->m_rpbs + idx;
        if(!rpb->b_valid)
            vpb->i_index = rpb->i_index;
        rpb->i_index = -1;
    }

    MHE_MSG(MHE_MSG_DEBUG, "[%s %d] mhe->u_oused = %d\n", __FUNCTION__, __LINE__, mhe->u_oused);

    /* Reset the reconstructed buffer attribute */
    mhe->p_recn = NULL;
    mhe->i_seqn++;
    mhe->b_seqh = 0;

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : _OutBuff
//  Description : Output coded buffer.
//------------------------------------------------------------------------------
static int _OutBuff(mhve_ops* mops, mhve_cpb* mcpb)
{

    mhe_ops* mhe = (mhe_ops*)mops;
    mhve_vpb* mvpb = &mhe->m_encp;
    int err = 0;
    if(mcpb->i_index >= 0)
    {
        //mhe->u_used = mhe->u_otrm = 0;
        mhe->u_oused = 0;
        mcpb->planes[0].u_phys = 0;
        mcpb->planes[0].i_size = 0;
        mcpb->i_stamp = 0;
        mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
        return err;
    }

    //err = mhe->u_oused;

    mcpb->i_index = 0;
    mcpb->planes[0].u_phys = mhe->u_obase;//mhe->u_otbs;
    mcpb->planes[0].i_size = mhe->u_oused;//err;
    mcpb->i_stamp = mvpb->i_stamp;
    mcpb->i_flags = (MHVE_FLAGS_SOP | MHVE_FLAGS_EOP);
    if(err > 0)
        mcpb->i_flags |= (mvpb->u_flags & MHVE_FLAGS_CTRL);

    return err;
}

//------------------------------------------------------------------------------
//  Function    : _SeqConf
//  Description : Configure sequence setting.
//------------------------------------------------------------------------------
static int _SeqConf(mhve_ops* mops)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    mhe_reg* regs = mhe->p_regs;
    int g_nHwCoreNum = 1;                  //1:dual core, 0:single core
    int g_iStreamId = 0;
    signed int g_MStarMERange[2][2];       // [x, y][min, max]
    int m_iSearchRange = HW_ME_WINDOW_X;   // ME search range
    int m_iSearchRangeY = HW_ME_WINDOW_Y;  // ME search range Y
    int g_bNewMSME = 1;                    // (Default)0: MHE 1.0 ME; 1: New ME scheme
    int g_DisableMergeSkip = 0;            // (Default)0; 1: disable Merge/skip mode
    int g_MaxFmeMode = 1;                  // (Default)1, 2, or 4(all). How many (partitioning) modes passed from IME to FME

    //int g_iFmePrec = QUARTER_PREC;

    //hev_init_reg
    // -- set constant-nonzero-value register --
    regs->hev_bank0.reg_hev_soft_rstz = 1;
    regs->hev_bank0.reg_hev_enc_mode_hevc = 1;
    regs->hev_bank0.reg_hev_me_4x4_disable = 1;
    regs->hev_bank0.reg_hev_me_ref_en_mode = 1;
    regs->hev_bank0.reg_hev_ime_mesr_max_addr = 0x5D;
    regs->hev_bank0.reg_hev_ime_mvx_min = 0xF0; //0xE0;
    regs->hev_bank0.reg_hev_ime_mvx_max = 0x0F; //0x1F;
    regs->hev_bank0.reg_hev_ime_mvy_min = 0xF0;
    regs->hev_bank0.reg_hev_ime_mvy_max = 0x0F;
    //regs->hev_bank0.reg_hev_fme_quarter_disable = (g_iFmePrec < QUARTER_PREC ? 1 : 0);
    //regs->hev_bank0.reg_hev_fme_half_disable    = (g_iFmePrec < HALF_PREC ? 1 : 0);
    regs->hev_bank0.reg_hev_fme_pipeline_on = 1;
    regs->hev_bank0.reg_hev_mcc_merge32_en = 1;
    regs->hev_bank0.reg_hev_mcc_merge16_en = 1;
    regs->hev_bank0.reg_hev_col_w_en = 1;

    //regs->hev_bank1.reg_mbr_const_qp_en = 1;
    //regs->hev_bank1.reg_mbr_ss_turn_off_perceptual = 1;
    //->hev_bank1.reg_txip_ctb_force_on = 0;

    // Dual-core, dual-stream ctrl
    regs->hev_bank1.reg_mhe_dual_core = (g_nHwCoreNum == 1) ? 0 : 1;
    regs->hev_bank1.reg_mhe_dual_row_dbfprf = 3;    // default value
    regs->hev_bank1.reg_mhe_dual_ctb_dbfprf = 5;    // default value
    regs->hev_bank1.reg_mhe_dual_strm_id = g_iStreamId;
    regs->hev_bank1.reg_mhe_dual_bs0_rstz = 1;
    regs->hev_bank1.reg_mhe_dual_bs1_rstz = 1;

    regs->hev_bank2.reg_hev_newme_h_search_max = 6;
    regs->hev_bank2.reg_hev_newme_v_search_max = 3;

    m_iSearchRange = (mhe->i_dmvx <= 32) ? 32 : 96;
    m_iSearchRangeY = (mhe->i_dmvy <= 16) ? 16 : 48;


    /* Global regs values (codec related) */
    g_MStarMERange[0][0] = 0 - m_iSearchRange;
    g_MStarMERange[0][1] = 0 + m_iSearchRange - 1;
    g_MStarMERange[1][0] = 0 - m_iSearchRangeY;
    g_MStarMERange[1][1] = 0 + m_iSearchRangeY - 1;

//    regs->hev_bank0.reg_hev_me_16x16_disable = !(mhe->i_blkp[0] & MHVE_INTER_16x16);
    regs->hev_bank0.reg_hev_me_16x8_disable = !(mhe->i_blkp[0] & MHVE_INTER_16x8);
    regs->hev_bank0.reg_hev_me_8x16_disable = !(mhe->i_blkp[0] & MHVE_INTER_8x16);
    regs->hev_bank0.reg_hev_me_8x8_disable = !(mhe->i_blkp[0] & MHVE_INTER_8x8);
    regs->hev_bank0.reg_hev_me_8x4_disable = !(mhe->i_blkp[0] & MHVE_INTER_8x4);
    regs->hev_bank0.reg_hev_me_4x8_disable = !(mhe->i_blkp[0] & MHVE_INTER_4x8);
//    regs->hev_bank0.reg_hev_me_4x4_disable = !(mhe->i_blkp[0] & MHVE_INTER_4x4);
    regs->hev_bank0.reg_hev_mesr_adapt = !g_bNewMSME; //IMEAdapt can only be 0 when NewMSME is set; forced to 0.

    regs->hev_bank0.reg_hev_ime_umv_disable = 0;
    regs->hev_bank0.reg_hev_ime_mesr_max_addr = (mhe->i_dmvy == 16 ? 95 : 85);
    regs->hev_bank0.reg_hev_ime_mesr_min_addr = (mhe->i_dmvy == 16 ?  0 : 10);
    regs->hev_bank0.reg_hev_ime_mvx_min = g_MStarMERange[0][0]; //_MAX_(-mhe->i_dmvx+32,-32+32);
    regs->hev_bank0.reg_hev_ime_mvx_max = g_MStarMERange[0][1]; //_MIN_( mhe->i_dmvx+32, 31+31);
    regs->hev_bank0.reg_hev_ime_mvy_min = g_MStarMERange[1][0]; //_MAX_(-mhe->i_dmvy+16,-16+16);
    regs->hev_bank0.reg_hev_ime_mvy_max = g_MStarMERange[1][1]; //_MIN_( mhe->i_dmvy+16, 16+15);
    regs->hev_bank0.reg_hev_ime_sr16 = (mhe->i_dmvx <= 32); //(pcEncTop->getSearchRange()==HW_ME_WINDOW_X_0) ? 1 : 0;
    regs->hev_bank2.reg_hev_newme_en = g_bNewMSME;

    if(g_bNewMSME)
    {
        regs->hev_bank0.reg_hev_ime_mvx_min = -HW_ME_STATIC_X;
        regs->hev_bank0.reg_hev_ime_mvx_max = HW_ME_STATIC_X - 1;
        regs->hev_bank0.reg_hev_ime_mvy_min = -HW_ME_STATIC_Y;
        regs->hev_bank0.reg_hev_ime_mvy_max = HW_ME_STATIC_Y - 1;
        regs->hev_bank2.reg_hev_newme_h_search_max = (m_iSearchRange == 96) ? 6 : 2;
        regs->hev_bank2.reg_hev_newme_v_search_max = (m_iSearchRangeY == 48) ? 3 : 1;
    }

    regs->hev_bank0.reg_hev_fme_quarter_disable = (mhe->i_subp != QUARTER_PREC);
    regs->hev_bank0.reg_hev_fme_half_disable = (mhe->i_subp == INT_PREC);
    regs->hev_bank0.reg_hev_fme_skip = 0 != (mhe->i_blkp[0] & MHVE_INTER_SKIP);

    // FME
    regs->hev_bank0.reg_hev_fme_merge32_en = g_DisableMergeSkip ? 0 : 1;
    regs->hev_bank0.reg_hev_fme_merge16_en = g_DisableMergeSkip ? 0 : 1;
    regs->hev_bank0.reg_hev_fme_mode_no = (g_MaxFmeMode == 2) ? 1 : 0;


    // Reset IMI
    regs->hev_bank2.reg60 = 0;
    regs->hev_bank2.reg61 = 0;
    regs->hev_bank2.reg62 = 0;
    regs->hev_bank2.reg63 = 0;
    regs->hev_bank2.reg64 = 0;
    regs->mhe_bank0.reg00 = 0;
    regs->mhe_bank0.reg01 = 0;
    regs->mhe_bank0.reg40 = 0;
    regs->mhe_bank0.reg45 = 0;

#if defined(MMHE_IMI_BUF_ADDR)
    // Reference Y lbw
    regs->hev_bank2.reg_hev_lbw_mode = 1;
    regs->mhe_bank0.reg_mhe_marb_wp_imi_en = (1 << 7);
    // Reference C lbw
    regs->hev_bank2.reg_hev_mcc_lbw_mode = 1;
    regs->mhe_bank0.reg_mhe_marb_wp_imi_en |= (1 << 4);
    // gn lbw
    regs->mhe_bank0.reg_mhe_marb_wp_imi_en |= (1 << 2);
    regs->mhe_bank0.reg_mhe_marb_rp_imi_en |= (1 << 2);
    // ppu lbw
    regs->mhe_bank0.reg_mhe_marb_wp_imi_en |= (1 << 6);
    regs->mhe_bank0.reg_mhe_marb_rp_imi_en |= (1 << 6);
    // Read/Write histogram enable
    regs->mhe_bank0.reg_mhe_whist_en = 1;
    regs->mhe_bank0.reg_mhe_rhist_en = 1;
    // Read/Write histogram mode ( mode 2 : Latch count)
    regs->mhe_bank0.reg_mhe_whist_mode = 2;
    regs->mhe_bank0.reg_mhe_rhist_mode = 2;
#endif

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : _PutData
//  Description : Put SPS/PPS/User data/Slice header to output buffer before encode start.
//------------------------------------------------------------------------------
static int _PutData(mhve_ops* mops, void* user, int size)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    h265_enc* h265 = mhe->h265;
    mhe_reg* regs = mhe->p_regs;
    void* src;
    void* dst;
    int len = 0;
    int  bcnt = 0;
    int  bits = 0;

    if(size > 0)
    {
        if(!mhe->p_okptr)
        {
            MHE_MSG(MHE_MSG_ERR, "> _PutData p_okptr NULL\n");
            return -1;
        }

        dst = mhe->p_okptr;

        /* copy sequence-header bits */
        if(mhe->b_seqh & OPS_SEQH_WRITE)
        {
            MEM_COPY(dst, h265->m_seqh, h265->i_seqh);
            bcnt += h265->i_seqh * 8;
            dst += h265->i_seqh;
        }

        /* Insert user-data */
        MEM_COPY(dst, user, size);
        bcnt += size * 8;
        dst += size;

        /* Insert slice-header */
        MEM_COPY(dst, h265->m_swsh, h265->i_swsh);
        bits = h265->i_swsh * 8;
        bcnt += bits;;

        mhe->u_oused = (bcnt >> 11) * 256; // Max 2048 bits(256 Bytes) one time due to bits_coded[256]

        // EROY CHECK : Just copy the rest data bits to bits_coded[], previous 256 Bytes is put in output buffer.
        src = mhe->p_okptr + mhe->u_oused;
        dst = (char*)regs->bits_coded;
        MEM_COPY(dst, src, ((bcnt + 7) / 8) & 255);
        regs->bits_count = bcnt & 2047;
        regs->bits_delta = regs->bits_count - bits;

        CamOsMemFlush((void *)mhe->p_okptr, (bcnt + 7) / 8);
    }
    else
    {
        dst = (char*)regs->bits_coded;

        /* copy sequence-header bits */
        if(mhe->b_seqh & OPS_SEQH_WRITE)
        {
#ifdef HEVC_HEADER_BY_FDC
            MEM_COPY(dst, h265->m_seqh, h265->i_seqh);
            bcnt += h265->i_seqh * 8;
            dst += h265->i_seqh;
#else
            dst = (char*)mhe->p_okptr + mhe->u_oused;
            mhe->u_oused += h265->i_seqh;
            MEM_COPY(dst, h265->m_seqh, h265->i_seqh);
            len += h265->i_seqh;
#endif
        }

        /* Insert slice-header */
        MEM_COPY(dst, h265->m_swsh, h265->i_swsh);
        bits = h265->i_swsh * 8;
        bcnt += bits;

        regs->bits_count = bcnt;
        regs->bits_delta = bcnt - bits;
        mhe->u_oused = 0; // No data is put in output buffer previously.
    }

    return len;
}

//------------------------------------------------------------------------------
//  Function    : _EncConf
//  Description : Configure current frame setting.
//------------------------------------------------------------------------------
static int _EncConf(mhve_ops* mops)
{
    //hev_set_main_reg
    mhe_ops* mhe = (mhe_ops*)mops;
    mhve_vpb* encp = &mhe->m_encp;
    h265_enc* h265 = mhe->h265;
    mhe_reg* pReg = mhe->p_regs;
    sps_t* sps = h265_find_set(h265, HEVC_SPS, 0);
    pps_t* pps = h265_find_set(h265, HEVC_PPS, 0);
    slice_t *sh = h265_find_set(h265, HEVC_SLICE, 0);
    pic_t *recn = h265->p_recn;
    uint phys;
    unsigned int value;
    int idc_0, idc_1;



    unsigned int m_maxNumMergeCand = 5;
    unsigned int m_log2ParallelMergeLevelMinus2 = 0;

    unsigned int g_uiMaxCUWidth  = CTB_SIZE;
    unsigned int g_uiMaxCUHeight = CTB_SIZE;

    unsigned int  picWidthIn32Pel;
    unsigned char colAvailble;

    int m_enableTMVPFlag = 0;
    int maxSliceSegmentAddress;
    int bitsSliceSegmentAddress;

//    signed int g_MStarMERange[2][2]; // [x, y][min, max]
//    int g_DisableMergeSkip = 0;
//    int g_MaxFmeMode = 1;
//    signed int        g_ImeModeMask = 63;  // bit0:16x16, bit1:16x8, bit2:8x16, bit3:8x8, bit4:8x4, bit5:4x8
//    unsigned char      g_bImeAdaptWin = 0; // IME search window adaptive-adjustment
    //     If true, the reduced search window is adaptive to previous 16x16 MV location.

//    g_MStarMERange[0][0] = 0xf0;    //0-HW_ME_WINDOW_X;    // x min
//    g_MStarMERange[0][1] = 0xf;     //0+HW_ME_WINDOW_X-1;  // x max
//    g_MStarMERange[1][0] = 0xf0;    //0-HW_ME_WINDOW_Y;    // y min
//    g_MStarMERange[1][1] = 0xf;     //0+HW_ME_WINDOW_Y-1;  // y max


#ifdef MODULE_PARAM_SUPPORT
    m_enableTMVPFlag = tmvp_enb;
#endif

    pReg->coded_framecnt = mhe->i_seqn;

    // Codec type (only support h.265)
    pReg->hev_bank0.reg_hev_enc_mode_hevc = MHE_REG_MOD_HEVC;
    pReg->hev_bank0.reg_hev_enc_mode = MHE_REG_ENC_H265;

    switch(mhe->e_pixf)
    {
        case MHVE_PIX_NV21:
            pReg->hev_bank0.reg_hev_src_chroma_swap = MHE_REG_PLNRLDR_VU;
        case MHVE_PIX_NV12:
            pReg->hev_bank0.reg_hev_src_yuv_format = MHE_REG_PLNRLDR_420;
            pReg->hev_bank0.reg_hev_src_luma_pel_width = mhe->m_encp.i_pitch ;
            pReg->hev_bank0.reg_hev_src_chroma_pel_width = mhe->m_encp.i_pitch;
            break;
        case MHVE_PIX_YVYU:
            pReg->hev_bank0.reg_hev_src_chroma_swap = MHE_REG_PLNRLDR_VU;
        case MHVE_PIX_YUYV:
            pReg->hev_bank0.reg_hev_src_yuv_format = MHE_REG_PLNRLDR_422;
            pReg->hev_bank0.reg_hev_src_luma_pel_width = mhe->m_encp.i_pitch;
            pReg->hev_bank0.reg_hev_src_chroma_pel_width = 0;
            break;
        default:
            break;
    }

    // Picture Info
    pReg->hev_bank0.reg_hev_frame_type = (recn->i_type == HEVC_ISLICE) ? 0 : 1;
    pReg->hev_bank0.reg_hev_slice_id = 0;
    pReg->hev_bank0.reg_hev_enc_pel_width_m1 = mhe->i_pctw - 1;
    pReg->hev_bank0.reg_hev_enc_pel_height_m1 = mhe->i_pcth - 1;
    pReg->hev_bank0.reg_hev_enc_ctb_cnt_m1 = (mhe->i_pctw / g_uiMaxCUWidth) * (mhe->i_pcth / g_uiMaxCUHeight) - 1;
    pReg->hev_bank0.reg_hev_cur_poc_low = sh->i_poc;        //fixed for I-only
    pReg->hev_bank0.reg_hev_cur_poc_high = sh->i_poc >> 16; //fixed for I-only

    //calculate number of bits required for slice address
    maxSliceSegmentAddress = (mhe->i_pctw / g_uiMaxCUWidth) * (mhe->i_pcth / g_uiMaxCUHeight);//pcSlice->getPic()->getNumCUsInFrame();
    bitsSliceSegmentAddress = 0;
    while(maxSliceSegmentAddress > (1 << bitsSliceSegmentAddress))
    {
        bitsSliceSegmentAddress++;
    }

    pReg->hev_bank0.reg_hev_ec_mdc_bits_ctb_num_m1 = bitsSliceSegmentAddress - 1;
    pReg->hev_bank0.reg_hev_ec_mdc_bits_poc_m1 = sps->i_log2_max_poc_lsb - 1;   //pcSlice->getSPS()->getBitsForPOC()-1;
    pReg->hev_bank0.reg_hev_ec_mdc_nuh_id_p1 = 1; //pcSlice->getTLayer()+1;

    // Mvp info
    colAvailble = !(h265->i_poc == 0 || h265->i_poc == 1); // unavailable in I frame and first P frame
    pReg->hev_bank0.reg_hev_col_l0_flag = colAvailble;//m_enableTMVPFlag && colAvailble;   // pcSlice->getColFromL0Flag()  && colAvailble;
    pReg->hev_bank0.reg_hev_col_r_en = m_enableTMVPFlag && colAvailble; // col buffer exist or not
    pReg->hev_bank0.reg_hev_temp_mvp_flag = m_enableTMVPFlag; // pcSlice->getEnableTMVPFlag() && colAvailble;

    pReg->hev_bank0.reg_hev_max_merge_cand_m1 = m_maxNumMergeCand - 1; // pcSlice->getMaxNumMergeCand() - 1;
    pReg->hev_bank0.reg_hev_parallel_merge_level = m_log2ParallelMergeLevelMinus2 + 2;

    // Ref List
    value = (recn->i_type == HEVC_ISLICE) ? 0 : h265->i_poc - 1;
    pReg->hev_bank0.reg_hev_reflst0_poc_low = value;
    pReg->hev_bank0.reg_hev_reflst0_poc_high = value >> 16;

    if(!(recn->i_type == HEVC_ISLICE))
    {
        pReg->hev_bank0.reg_hev_reflst0_lt_fg = 0;
        pReg->hev_bank0.reg_hev_reflst0_st_fg = 1;
        pReg->hev_bank0.reg_hev_reflst0_fbidx = (h265->i_poc - 1) % 5;
    }
    else
    {
        pReg->hev_bank0.reg_hev_reflst0_lt_fg = 0;
        pReg->hev_bank0.reg_hev_reflst0_st_fg = 0;
        pReg->hev_bank0.reg_hev_reflst0_fbidx = 0;
    }

    // NAL unit
    //if(!(recn->i_type == HEVC_ISLICE) && pReg->coded_framecnt == 0)
    if(sh->i_nal_type == IDR_W_RADL)
    {
        pReg->hev_bank0.reg_hev_ec_mdc_nal_unit_type = IDR_W_RADL;  //derek check
        pReg->hev_bank0.reg_hev_ec_mdc_nuh_id_p1 = 1;       //derek check
        pReg->hev_bank0.reg_hev_ec_mdc_is_idr_picture = 1;  //derek check
    }
    else
    {
        pReg->hev_bank0.reg_hev_ec_mdc_nal_unit_type = TRAIL_R;   //derek check //pcSlice->getNalUnitType();
        pReg->hev_bank0.reg_hev_ec_mdc_nuh_id_p1 = 1;       //derek check //pcSlice->getTLayer()+1;
        pReg->hev_bank0.reg_hev_ec_mdc_is_idr_picture = 0;  //derek check
    }

    // Multi-slice mode
    pReg->hev_bank0.reg_hev_ec_multislice_en = (mhe->i_rows == 0) ? 0 : 1;
    pReg->hev_bank0.reg_hev_ec_multislice_1st_ctby = mhe->i_rows;
    pReg->hev_bank0.reg_hev_ec_multislice_ctby = mhe->i_rows;

    // IME
//    pReg->hev_bank0.reg_hev_me_8x4_disable = (g_ImeModeMask & 0x10) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_me_4x8_disable = (g_ImeModeMask & 0x20) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_me_16x8_disable = (g_ImeModeMask & 0x2) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_me_8x16_disable = (g_ImeModeMask & 0x4) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_me_8x8_disable = (g_ImeModeMask & 0x8) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_me_16x16_disable = (g_ImeModeMask & 0x1) ? 0 : 1;
//    pReg->hev_bank0.reg_hev_mesr_adapt = g_bImeAdaptWin;
//    pReg->hev_bank0.reg_hev_ime_mvx_min = g_MStarMERange[0][0]; //0xf0
//    pReg->hev_bank0.reg_hev_ime_mvx_max = g_MStarMERange[0][1]; //0xf
//    pReg->hev_bank0.reg_hev_ime_mvy_min = g_MStarMERange[1][0]; //0xf0
//    pReg->hev_bank0.reg_hev_ime_mvy_max = g_MStarMERange[1][1]; //0xf
//    pReg->hev_bank0.reg_hev_ime_sr16 = 0;//(HW_ME_WINDOW_X==32) ? 1 : 0;

    // FME
//    pReg->hev_bank0.reg_hev_fme_merge32_en = g_DisableMergeSkip ? 0 : 1;
//    pReg->hev_bank0.reg_hev_fme_merge16_en = g_DisableMergeSkip ? 0 : 1;
//    pReg->hev_bank0.reg_hev_fme_mode_no = (g_MaxFmeMode == 2) ? 1 : 0;


    // RDO [Rate control coefficients]
    pReg->hev_bank0.reg_hev_rdo_tu4_intra_c1 = 113;     //g_rate_model_C1[0][0] = 113
    pReg->hev_bank0.reg_hev_rdo_tu4_inter_c1 = 117;     //g_rate_model_C1[1][0] = 117
    pReg->hev_bank0.reg_hev_rdo_tu8_intra_c1 = 81;      //g_rate_model_C1[0][1] = 81
    pReg->hev_bank0.reg_hev_rdo_tu8_inter_c1 = 104;     // g_rate_model_C1[1][1] = 104
    pReg->hev_bank0.reg_hev_rdo_tu16_intra_c1 = 67;     //g_rate_model_C1[0][2] = 67
    pReg->hev_bank0.reg_hev_rdo_tu16_inter_c1 = 97;     //g_rate_model_C1[1][2] = 97
    pReg->hev_bank0.reg_hev_rdo_tu32_intra_c1 = 72;     //g_rate_model_C1[0][3] = 72
    pReg->hev_bank0.reg_hev_rdo_tu32_inter_c1 = 85;     //g_rate_model_C1[1][3] = 85
    pReg->hev_bank0.reg_hev_rdo_tu4_intra_c0 = 54;      //g_rate_model_C0[0][0] = 54
    pReg->hev_bank0.reg_hev_rdo_tu4_intra_beta = 22;    //g_rate_model_beta[0][0] = 22
    pReg->hev_bank0.reg_hev_rdo_tu4_inter_c0 = 82;      //g_rate_model_C0[1][0] = 82
    pReg->hev_bank0.reg_hev_rdo_tu4_inter_beta = 18;    //g_rate_model_beta[1][0] = 18
    pReg->hev_bank0.reg_hev_rdo_tu8_intra_c0 = 252;     //g_rate_model_C0[0][1] = 252
    pReg->hev_bank0.reg_hev_rdo_tu8_intra_beta = 20;    //g_rate_model_beta[0][1] = 20
    pReg->hev_bank0.reg_hev_rdo_tu8_inter_c0 = 226;     //g_rate_model_C0[1][1] = 226
    pReg->hev_bank0.reg_hev_rdo_tu8_inter_beta = 18;    //g_rate_model_beta[1][1] = 18
    pReg->hev_bank0.reg_hev_rdo_tu16_intra_c0 = 467;    //g_rate_model_C0[0][2] = 467
    pReg->hev_bank0.reg_hev_rdo_tu16_intra_beta = 20;   //g_rate_model_beta[0][2] = 20
    pReg->hev_bank0.reg_hev_rdo_tu16_inter_c0 = 229;    //g_rate_model_C0[1][2] = 229
    pReg->hev_bank0.reg_hev_rdo_tu16_inter_beta = 18;   //g_rate_model_beta[1][2] = 18
    pReg->hev_bank0.reg_hev_rdo_tu32_intra_c0 = 764;    //g_rate_model_C0[0][3] = 764
    pReg->hev_bank0.reg_hev_rdo_tu32_intra_beta = 10;   //g_rate_model_beta[0][3] = 10
    pReg->hev_bank0.reg_hev_rdo_tu32_inter_c0 = 371;    //g_rate_model_C0[1][3] = 371
    pReg->hev_bank0.reg_hev_rdo_tu32_inter_beta = 26;   //g_rate_model_beta[1][3] = 26


    // TXIP
//    pReg->hev_bank0.reg_hev_txip_cu8_intra_lose              = 0;   //g_bDisableCU8Intra: Avoid CU8 intra to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu8_inter_lose              = 0;   //g_bDisableCU8Inter: Avoid CU8 AMVP to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu16_intra_lose             = 0;   //g_bDisableCU16Intra: Avoid CU16 intra to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_mvp_lose         = 0;   //g_bDisableCU16Inter: Avoid CU16 AMVP to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_merge_lose       = 0;   //g_bDisableCU16Merge: Avoid CU16 Merge to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_mvp_nores_lose   = 0;   //g_bDisableCU16InterSkip: Avoid CU16 AMVP no-coef to be chosen. forcing coef to be all 0's,
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_merge_nores_lose = 0;   //g_bDisableCU16MergeSkip: Avoid CU16 Merge no-coef to be chosen. forcing coef to be all 0's
//    pReg->hev_bank0.reg_hev_txip_cu32_intra_lose             = 0;   //g_bDisableCU32Intra: Avoid CU32 intra to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu32_inter_merge_lose       = 0;   //g_bDisableCU32Merge: Avoid CU32 Merge to be chosen.
//    pReg->hev_bank0.reg_hev_txip_cu32_inter_merge_nores_lose = 0;   //g_bDisableCU32MergeSkip: Avoid CU32 Merge no-coef to be chosen. forcing coef to be all 0's

    /*
    static int g_penalty_table[7][10] =
    {
        {    0,     0,     0,     0,     0,     0,     0,     0,     0,     0}, // Default
        {65535,     0, 65535,     0,     0, 65535, 65535, 65535, 65535, 65535}, // all amvp
        {    0,     0, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535}, // all cu8
        {65535, 65535,     0,     0,     0,     0,     0, 65535, 65535, 65535}, // all cu16
        {65535, 65535, 65535, 65535, 65535, 65535, 65535,     0,     0,     0}, // all cu32
        {    0, 65535,     0, 65535, 65535, 65535, 65535,     0, 65535, 65535}, // all intra
        {65535, 65535, 65535, 65535, 65535,     0,     0, 65535,     0,     0}  // all merge
    };
    */

    // Mode penalties when RDO cost comparison
//    pReg->hev_bank0.reg_hev_txip_cu8_intra_penalty              = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU8Intra = 0: CU8 intra. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu8_inter_penalty              = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU8Inter = 0: CU8 AMVP. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu16_intra_penalty             = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU16Intra = 0: CU16 intra. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_mvp_penalty         = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU16Inter = 0: CU16 AMVP. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_merge_penalty       = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU16Merge = 0: CU16 Merge. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_mvp_nores_penalty   = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU16InterSkip = 0: CU16 AMVP no-coef. Must >=0. Larger value means less preferred. forcing coef to be all 0's
//    pReg->hev_bank0.reg_hev_txip_cu16_inter_merge_nores_penalty = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU16MergeSkip = 0: CU16 Merge no-coef. Must >=0. Larger value means less preferred. forcing coef to be all 0's
//    pReg->hev_bank0.reg_hev_txip_cu32_intra_penalty             = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU32Intra = 0: CU32 intra. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu32_inter_merge_penalty       = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU32Merge = 0: CU32 Merge. Must >=0. Larger value means less preferred.
//    pReg->hev_bank0.reg_hev_txip_cu32_inter_merge_nores_penalty = 0 >> PENALTY_SHIFT;   //g_iPenaltyCU32MergeSkip = 0: CU32 Merge no-coef. Must >=0. Larger value means less preferred., forcing coef to be all 0's


    // Deblocking
    pReg->hev_bank0.reg_hev_disilf_idc = pps->b_deblocking_filter_disabled ? 1 : 0;
    pReg->hev_bank0.reg_hev_disilf_idc |= (pps->b_loop_filter_across_slices_enabled ? 0 : 1) << 3;
    pReg->hev_bank0.reg_hev_beta_offset = pps->i_beta_offset;
    pReg->hev_bank0.reg_hev_tc_offset = pps->i_tc_offset;
    //pReg->hev_bank0.reg_hev_cb_qp_offset = pcPPS->getChromaCbQpOffset();
    //pReg->hev_bank0.reg_hev_cr_qp_offset = pcPPS->getChromaCrQpOffset();


    picWidthIn32Pel = mhe->i_pctw >> 5;
    pReg->hev_bank0.reg_hev_ppu_fb_pitch = picWidthIn32Pel;
    pReg->hev_bank0.reg_hev_ppu_fb_pitch_lsb = ((picWidthIn32Pel % 4) == 0)
            ? (picWidthIn32Pel * 32) / 128
            : ((picWidthIn32Pel + 3) * 32) / 128;

    // GN
    pReg->hev_bank0.reg_hev_gn_sz_ctb_m1 = ((mhe->i_pctw + 31) >> 5) - 1;

    // Set Output buffer attribute
    pReg->coded_data = pReg->bits_coded;
    pReg->coded_bits = pReg->bits_count;
    pReg->outbs_addr = mhe->u_obase + mhe->u_oused;
    pReg->outbs_size = mhe->u_osize - mhe->u_oused;

    //MHE buffer
    pReg->gn_mem = mhe->gn_mem;
    pReg->ppu_int_b = mhe->ppu_int_b;
    pReg->ppu_int_a = mhe->ppu_int_a;

    // double buffer
    idc_0 = mhe->i_seqn % 2;
    idc_1 = idc_0 ^ 0x01;

    pReg->ppu_y_base_buf[0] = mhe->refconY[idc_0];
    pReg->ppu_c_base_buf[0] = mhe->refconC[idc_0];
    pReg->col_w_sadr_buf[0] = mhe->colsadr[idc_0];

    pReg->ppu_y_base_buf[1] = mhe->refconY[idc_1];
    pReg->ppu_c_base_buf[1] = mhe->refconC[idc_1];
    pReg->col_w_sadr_buf[1] = mhe->colsadr[idc_1];

#if defined(MMHE_IMI_BUF_ADDR)
    pReg->imi_ref_y_buf = mhe->imi_ref_y_buf;
    pReg->imi_ref_c_buf = mhe->imi_ref_c_buf;
#endif

    // Buffer setting: current
    phys = encp->planes[0].u_phys + encp->planes[0].i_bias;
    pReg->hev_bank0.reg_hev_cur_y_adr_low = (ushort)(phys >> 4);
    pReg->hev_bank0.reg_hev_cur_y_adr_high = (ushort)(phys >> 16);
    phys = encp->planes[1].u_phys + encp->planes[1].i_bias;
    pReg->hev_bank0.reg_hev_cur_c_adr_low = (ushort)(phys >> 4);
    pReg->hev_bank0.reg_hev_cur_c_adr_high = (ushort)(phys >> 16);

    // Set Dump Register
    pReg->hev_bank2.reg_hev_dump_reg_sadr_low = (ushort)(mhe->p_drpptr >> 4);
    pReg->hev_bank2.reg_hev_dump_reg_sadr_high = (ushort)(mhe->p_drpptr >> 16);
    pReg->hev_bank2.reg_hev_dump_reg_en = 1;


    // Set CmdQ IRQ
    pReg->hev_bank1.reg2c = 0x00FF;
    pReg->hev_bank1.reg2d = 0xFFFF;

    return 0;
}

//------------------------------------------------------------------------------
//  Function    : _SetConf
//  Description : Apply configure.
//------------------------------------------------------------------------------
static int _SetConf(mhve_ops* mops, mhve_cfg* mcfg)
{
    mhe_ops* mhe = (mhe_ops*)mops;
    rqct_ops* rqct = mops->rqct_ops(mops);
    rqct_cfg  rqcf;
    h265_enc* h265 = mhe->h265;
    int err = -1;
    //int i;

    switch(mcfg->type)
    {
            pps_t* pps;
            sps_t* sps;
        case MHVE_CFG_RES:
            if((unsigned)mcfg->res.e_pixf <= MHVE_PIX_YVYU)
            {
                pps = h265_find_set(h265, HEVC_PPS, 0);
                sps = pps->p_sps;
                mhe->e_pixf = mcfg->res.e_pixf;
                mhe->i_pixw = mcfg->res.i_pixw;
                mhe->i_pixh = mcfg->res.i_pixh;
                mhe->i_pctw = _ALIGN(5, mcfg->res.i_pixw);
                mhe->i_pcth = _ALIGN(5, mcfg->res.i_pixh);
                mhe->i_rpbn = mcfg->res.i_rpbn;
                mhe->u_conf = mcfg->res.u_conf;
                h265->i_picw = mhe->i_pixw;
                h265->i_pich = mhe->i_pixh;
                h265->i_cb_w = (h265->i_picw + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
                h265->i_cb_h = (h265->i_pich + ((1 << sps->i_log2_max_cb_size) - 1)) >> sps->i_log2_max_cb_size;
                mhe->b_seqh |= OPS_SEQH_RESET;
                err = 0;
            }
            break;
        case MHVE_CFG_DMA:
            if((unsigned)mcfg->dma.i_dmem < 2)
            {
                uint addr = mcfg->dma.u_phys;
                int i = mcfg->dma.i_dmem;
                rpb_t* ref = mhe->m_rpbs + i;

                ref->i_index = -1;
                ref->i_state = RPB_STATE_FREE;
                ref->u_phys[RPB_YPIX] = !mcfg->dma.i_size[0] ? 0 : addr;
                addr += mcfg->dma.i_size[0];
                ref->u_phys[RPB_CPIX] = !mcfg->dma.i_size[1] ? 0 : addr;
                addr += mcfg->dma.i_size[1];
                ref->b_valid = ref->u_phys[RPB_YPIX] != 0;
                err = 0;
            }
            else if(mcfg->dma.i_dmem == -1)
            {
                mhe->p_okptr = mcfg->dma.p_vptr;
                mhe->u_obase = mcfg->dma.u_phys;
                mhe->u_osize = mcfg->dma.i_size[0];
                mhe->u_oused = 0;
                err = 0;
            }
            else if(mcfg->dma.i_dmem == -2)
            {
                mhe->u_mbp_base = mcfg->dma.u_phys;
                err = 0;
            }
            break;
        case MHVE_CFG_HEV:
            pps = h265_find_set(h265, HEVC_PPS, 0);
            sps = pps->p_sps;
            h265->i_profile = mcfg->hev.i_profile;
            h265->i_level = mcfg->hev.i_level;
            sps->i_log2_max_cb_size = mcfg->hev.i_log2_max_cb_size;
            sps->i_log2_min_cb_size = mcfg->hev.i_log2_min_cb_size;
            sps->i_log2_max_tr_size = mcfg->hev.i_log2_max_tr_size;
            sps->i_log2_min_tr_size = mcfg->hev.i_log2_min_tr_size;
            sps->i_max_tr_hierarchy_depth_intra = mcfg->hev.i_tr_depth_intra;
            sps->i_max_tr_hierarchy_depth_inter = mcfg->hev.i_tr_depth_inter;
            sps->b_scaling_list_enable = mcfg->hev.b_scaling_list_enable;
            sps->b_sao_enabled = mcfg->hev.b_sao_enable;
            sps->b_strong_intra_smoothing_enabled = mcfg->hev.b_strong_intra_smoothing;
            pps->i_diff_cu_qp_delta_depth = 0;
            pps->b_cu_qp_delta_enabled = mcfg->hev.b_ctu_qp_delta_enable > 0;
            if(pps->b_cu_qp_delta_enabled)
                pps->i_diff_cu_qp_delta_depth = 3 & (mcfg->hev.b_ctu_qp_delta_enable - 1);
            pps->b_constrained_intra_pred = mcfg->hev.b_constrained_intra_pred;
            pps->i_cb_qp_offset = mcfg->hev.i_cqp_offset;
            pps->i_cr_qp_offset = mcfg->hev.i_cqp_offset;
            pps->b_slice_chroma_qp_offsets_pres = (mcfg->hev.i_cqp_offset != 0) ? 1 : 0;
            //de-blocking
            pps->b_deblocking_filter_override_enabled = mcfg->hev.b_deblocking_override_enable;
            pps->b_deblocking_filter_disabled = h265->b_deblocking_disable = mcfg->hev.b_deblocking_disable;
            if(h265->b_deblocking_disable)
                h265->i_tc_offset = h265->i_beta_offset = pps->i_tc_offset = pps->i_beta_offset = 0;
            else
            {
                h265->i_tc_offset = pps->i_tc_offset = mcfg->hev.i_tc_offset_div2 * 2;
                h265->i_beta_offset = pps->i_beta_offset = mcfg->hev.i_beta_offset_div2 * 2;
            }
            mhe->b_seqh |= OPS_SEQH_RESET;
            err = 0;
            break;
        case MHVE_CFG_MOT:
            mhe->i_subp = mcfg->mot.i_subp;
            mhe->i_dmvx = mcfg->mot.i_dmvx;
            mhe->i_dmvy = mcfg->mot.i_dmvy;
            mhe->i_blkp[0] = mcfg->mot.i_blkp[0];
            mhe->i_blkp[1] = 0;
            err = 0;
            break;
        case MHVE_CFG_VUI:
        {
            sps = h265_find_set(h265, HEVC_SPS, 0);
            sps->b_vui_param_pres = 1;  //0
            if(0 != (sps->vui.b_video_full_range = (mcfg->vui.b_video_full_range != 0)))
            {
                sps->vui.b_video_signal_pres = mcfg->vui.b_video_signal_pres;   //1
                sps->vui.i_video_format = mcfg->vui.i_video_format;             //5
                sps->vui.b_colour_desc_pres = mcfg->vui.b_colour_desc_pres;     //0
                //sps->b_vui_param_pres = 1;
            }
            if(0 != (sps->vui.b_timing_info_pres = (mcfg->vui.b_timing_info_pres != 0)))
            {
                rqcf.type = RQCT_CFG_FPS;
                if(!(err = rqct->get_rqcf(rqct, &rqcf)))
                {
                    sps->vui.i_num_units_in_tick = (uint)rqcf.fps.d_fps;
                    sps->vui.i_time_scale = (uint)rqcf.fps.n_fps * 2;
                    //sps->vui.b_fixed_frame_rate = 1;
                }
            }

            sps->vui.i_sar_w = mcfg->vui.i_sar_w;
            sps->vui.i_sar_h = mcfg->vui.i_sar_h;

            mhe->b_seqh |= OPS_SEQH_RESET;
            err = 0;
        }
        break;
        case MHVE_CFG_LFT:
            pps = h265_find_set(h265, HEVC_PPS, 0);
            if(!pps->b_deblocking_filter_override_enabled)
                break;
            if(!mcfg->lft.b_override)
            {
                h265->b_deblocking_override = 0;
                h265->b_deblocking_disable = pps->b_deblocking_filter_disabled;
                h265->i_tc_offset = pps->i_tc_offset;
                h265->i_beta_offset = pps->i_beta_offset;
                err = 0;
                break;
            }
            if(!mcfg->lft.b_disable && ((unsigned)(mcfg->lft.i_offsetA + 6) > 12 || (unsigned)(mcfg->lft.i_offsetB + 6) > 12))
                break;
            err = 0;
            h265->b_deblocking_override = 1;
            if(!(h265->b_deblocking_disable = mcfg->lft.b_disable))
            {
                h265->i_tc_offset  = mcfg->lft.i_offsetA * 2;
                h265->i_beta_offset = mcfg->lft.i_offsetB * 2;
            }
            break;
        case MHVE_CFG_SPL:
            mhe->i_bits = mcfg->spl.i_bits;
            mhe->i_rows = 0;

            if(0 < mcfg->spl.i_rows)
            {
                mhe->i_rows = mcfg->spl.i_rows;
            }
            err = 0;
            break;
        case MHVE_CFG_BAC:
            h265->b_cabac_init = mcfg->bac.b_init;
            err = 0;
            break;
        case MHVE_CFG_DUMP_REG:
            mhe->p_drpptr = (uint)mcfg->dump_reg.u_phys;
            mhe->p_drvptr = (uint)mcfg->dump_reg.p_vptr;
            mhe->u_drsize = mcfg->dump_reg.i_size;
            err = 0;
            break;
        case MHVE_CFG_BUF:
            mhe->gn_mem = mcfg->hev_buff_addr.gn_mem;               // TXIP intermediate data
            mhe->ppu_int_b = mcfg->hev_buff_addr.ppu_int_b;         // PPU intermediate data
            // (Only needed when low-bandwidth mode) In SRAM
            mhe->imi_ref_y_buf = mcfg->hev_buff_addr.imi_ref_y_buf;
            mhe->imi_ref_c_buf = mcfg->hev_buff_addr.imi_ref_c_buf;
            // In DRAM
            mhe->ppu_int_a = mcfg->hev_buff_addr.ppu_int_a;                           // PPU intermediate data
            mhe->refconY[0] = mcfg->hev_buff_addr.refconY[0];             // 0:reconstructed Y buffer, 1:reference Y buffer
            mhe->refconY[1] = mcfg->hev_buff_addr.refconY[1];
            mhe->refconC[0] = mcfg->hev_buff_addr.refconC[0];             // 0:reconstructed C buffer, 1:reference C buffer
            mhe->refconC[1] = mcfg->hev_buff_addr.refconC[1];
            mhe->colsadr[0] = mcfg->hev_buff_addr.colsadr[0];
            mhe->colsadr[1] = mcfg->hev_buff_addr.colsadr[1];
            break;
        default:
            break;
    }

    return err;
}

//------------------------------------------------------------------------------
//  Function    : _GetConf
//  Description : Query configuration.
//------------------------------------------------------------------------------
static int _GetConf(mhve_ops* mops, mhve_cfg* mcfg)
{

    mhe_ops* mhe = (mhe_ops*)mops;
    h265_enc* h265 = mhe->h265;
    int err = -1;

    switch(mcfg->type)
    {
            pps_t* pps;
            sps_t* sps;
        case MHVE_CFG_RES:
            mcfg->res.e_pixf = mhe->e_pixf;
            mcfg->res.i_pixw = mhe->i_pixw;
            mcfg->res.i_pixh = mhe->i_pixh;
            mcfg->res.i_rpbn = mhe->i_rpbn;
            mcfg->res.u_conf = mhe->u_conf;
            err = 0;
            break;
        case MHVE_CFG_MOT:  //from H264
            mcfg->mot.i_subp = mhe->i_subp;
            mcfg->mot.i_dmvx = mhe->i_dmvx;
            mcfg->mot.i_dmvy = mhe->i_dmvy;
            mcfg->mot.i_blkp[0] = mhe->i_blkp[0];
            mcfg->mot.i_blkp[1] = 0;
            err = 0;
            break;
        case MHVE_CFG_HEV:
            pps = h265_find_set(h265, HEVC_PPS, 0);
            sps = pps->p_sps;
            mcfg->hev.i_profile = h265->i_profile;
            mcfg->hev.i_level = h265->i_level;
            mcfg->hev.i_log2_max_cb_size = sps->i_log2_max_cb_size;
            mcfg->hev.i_log2_min_cb_size = sps->i_log2_min_cb_size;
            mcfg->hev.i_log2_max_tr_size = sps->i_log2_max_tr_size;
            mcfg->hev.i_log2_min_tr_size = sps->i_log2_min_tr_size;
            mcfg->hev.i_tr_depth_intra = sps->i_max_tr_hierarchy_depth_intra;
            mcfg->hev.i_tr_depth_inter = sps->i_max_tr_hierarchy_depth_inter;
            mcfg->hev.b_scaling_list_enable = sps->b_scaling_list_enable;
            mcfg->hev.b_sao_enable = sps->b_sao_enabled;
            mcfg->hev.b_strong_intra_smoothing = sps->b_strong_intra_smoothing_enabled;
            mcfg->hev.b_ctu_qp_delta_enable = pps->b_cu_qp_delta_enabled + pps->i_diff_cu_qp_delta_depth;
            mcfg->hev.b_constrained_intra_pred = pps->b_constrained_intra_pred;
            mcfg->hev.i_cqp_offset = pps->i_cb_qp_offset;
            mcfg->hev.b_deblocking_override_enable = pps->b_deblocking_filter_override_enabled;
            mcfg->hev.b_deblocking_disable = pps->b_deblocking_filter_disabled;
            mcfg->hev.i_tc_offset_div2 = pps->i_tc_offset >> 1;
            mcfg->hev.i_beta_offset_div2 = pps->i_beta_offset >> 1;
            err = 0;
            break;
        case MHVE_CFG_VUI:
        {
            sps = h265_find_set(h265, HEVC_SPS, 0);
            mcfg->vui.b_video_full_range = sps->vui.b_video_full_range;
            mcfg->vui.b_video_signal_pres = sps->vui.b_video_signal_pres;
            mcfg->vui.i_video_format = sps->vui.i_video_format;
            mcfg->vui.b_colour_desc_pres = sps->vui.b_colour_desc_pres;
            mcfg->vui.b_timing_info_pres = sps->vui.b_timing_info_pres;
            mcfg->vui.i_sar_w = sps->vui.i_sar_w;
            mcfg->vui.i_sar_h = sps->vui.i_sar_h;
            err = 0;
        }
        break;
        case MHVE_CFG_LFT:
            mcfg->lft.b_override = (signed char)h265->b_deblocking_override;
            mcfg->lft.b_disable = (signed char)h265->b_deblocking_disable;
            mcfg->lft.i_offsetA = (signed char)h265->i_tc_offset / 2;
            mcfg->lft.i_offsetB = (signed char)h265->i_beta_offset / 2;
            err = 0;
            break;
        case MHVE_CFG_SPL:
            mcfg->spl.i_rows = mhe->i_rows;//h265->i_rows;
            mcfg->spl.i_bits = err = 0;
            break;
        case MHVE_CFG_BAC:
            mcfg->bac.b_init = h265->b_cabac_init != 0;
            err = 0;
            break;
        default:
            break;
    }

    return err;
}


//------------------------------------------------------------------------------
//  Function    : MheOpsAcquire
//  Description : Allocate mhe_ops object and link its member function.
//------------------------------------------------------------------------------
void* MheOpsAcquire(int rqc_id)
{

    mhve_ops* mops = NULL;

    while(NULL != (mops = MEM_ALLC(sizeof(mhe_ops))))
    {
        mhe_ops* mhe = (mhe_ops*)mops;

        /* Link member function */
        mops->release = __OpsFree;
        mops->rqct_ops = _RqctOps;
        mops->pmbr_ops = _PmbrOps;
        mops->mhve_job = _MheJob;
        mops->seq_sync = _SeqSync;
        mops->seq_conf = _SeqConf;
        mops->seq_done = _SeqDone;
        mops->enc_buff = _EncBuff;
        mops->deq_buff = _DeqBuff;
        mops->put_data = _PutData;
        mops->enc_conf = _EncConf;
        mops->enc_done = _EncDone;
        mops->out_buff = _OutBuff;
        mops->set_conf = _SetConf;
        mops->get_conf = _GetConf;

        /* Link RQCT and allocate register mirror structure */
        mhe->p_rqct = RqctMheAcquire(rqc_id);
        mhe->p_pmbr = MhePmbrAllocate();
        mhe->p_regs = MEM_ALLC(sizeof(mhe_reg));
        mhe->h265 = h265enc_acquire();
        if(!mhe->p_regs || !mhe->p_rqct || !mhe->p_pmbr || !mhe->h265)
        {
            break;
        }

        /* Initialize common parameters */
        mhe->i_refn = 1;
        mhe->i_dmvx = HW_ME_WINDOW_X;
        mhe->i_dmvy = HW_ME_WINDOW_Y;
        mhe->i_subp = QUARTER_PREC;
        mhe->i_blkp[0] = MVBLK_DEFAULT;
        mhe->i_blkp[1] = 0;

        return mops;
    }

    if(mops)
        mops->release(mops);

    return NULL;
}
//EXPORT_SYMBOL(MheOpsAcquire);
