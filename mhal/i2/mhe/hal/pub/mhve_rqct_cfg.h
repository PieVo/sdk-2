
#ifndef _RQCT_CFG_H_
#define _RQCT_CFG_H_

typedef union  rqct_cfg rqct_cfg;
typedef struct rqct_buf rqct_buf;

union rqct_cfg
{
    enum rqct_cfg_e
    {
        RQCT_CFG_SEQ = 0,
        RQCT_CFG_DQP,
        RQCT_CFG_QPR,
        RQCT_CFG_LOG,
        RQCT_CFG_PEN,
        RQCT_CFG_SPF,
        RQCT_CFG_RES = 32,
        RQCT_CFG_FPS,
        RQCT_CFG_ROI,
        RQCT_CFG_DQM,
        RQCT_CFG_RCM,
    } type;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_SEQ"
        enum rqct_mode
        {
            RQCT_MODE_CQP = 0,
            RQCT_MODE_CBR,
            RQCT_MODE_VBR,
        }               i_method;           /* Rate Control Method */
        int             i_period;           /* GOP */
        int             i_leadqp;           /* Frame Level QP */
        int             i_btrate;           /* Bitrate */
        int             b_passiveI;         /* Passive encode I frame */
    } seq;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_DQP"
        int             i_dqp;              /* QP Delta Range */
    } dqp;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_QPR"
        int             i_iupperq;          /* QP Upper Bound */
        int             i_ilowerq;          /* QP Lower Bound */
        int             i_pupperq;          /* QP Upper Bound */
        int             i_plowerq;          /* QP Lower Bound */
    } qpr;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_LOG"
        int             b_logm;             /* Log Enable */
    } log;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_PEN"
        short               b_ia8xlose;   /* Avoid CU8 intra to be chosenv */
        short               b_ir8xlose;   /* Avoid CU8 AMVP to be chosen */
        short               b_ia16lose;   /* Avoid CU16 intra to be chosen */
        short               b_ir16lose;   /* Avoid CU16 AMVP to be chosen */
        short               b_ir16mlos;   /* Avoid CU16 Merge to be chosen */
        short               b_ir16slos;   /* Avoid CU16 AMVP no-coef to be chosen. forcing coef to be all 0's */
        short               b_ir16mslos;  /* Avoid CU16 Merge no-coef to be chosen. forcing coef to be all 0's */
        short               b_ia32lose;   /* Avoid CU32 intra to be chosen */
        short               b_ir32mlos;   /* Avoid CU32 Merge to be chosen */
        short               b_ir32mslos;  /* Avoid CU32 Merge no-coef to be chosen. forcing coef to be all 0's */

        unsigned short int  u_ia8xpen;    /* CU8 intra. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir8xpen;    /* CU8 AMVP. Must >=0. Larger value means less preferred */
        unsigned short int  u_ia16pen;    /* CU16 intra. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir16pen;    /* CU16 AMVP. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir16mpen;   /* CU16 Merge. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir16spen;   /* CU16 AMVP no-coef. Must >=0. Larger value means less preferred. forcing coef to be all 0's */
        unsigned short int  u_ir16mspen;  /* CU16 Merge no-coef. Must >=0. Larger value means less preferred. forcing coef to be all 0's */
        unsigned short int  u_ia32pen;    /* CU32 intra. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir32mpen;   /* CU32 Merge. Must >=0. Larger value means less preferred */
        unsigned short int  u_ir32mspen;  /* CU32 Merge no-coef. Must >=0. Larger value means less preferred., forcing coef to be all 0's */

    } pen;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_RCM"
        enum rqct_spfrm_mode
        {
            RQCT_SPFRM_NONE = 0,            //!< super frame mode none.
            RQCT_SPFRM_DISCARD,             //!< super frame mode discard.
            RQCT_SPFRM_REENCODE,            //!< super frame mode re-encode.
        } e_spfrm;
        int             i_IfrmThr;
        int             i_PfrmThr;
        int             i_BfrmThr;
    } spf;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_RES"
        short           i_picw;             /* Picture Width */
        short           i_pich;             /* Picture Height */
    } res;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_FPS"
        short           n_fps;              /* FPS Numerator */
        short           d_fps;              /* FPS Denominator */
    } fps;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_ROI"
        unsigned int    u_roienb;           /* ROI Rectangle Enable */
        short           i_roiidx;           /* ROI Rectangle Index */
        short           i_absqp;            /* ROI Rectangle Absolute QP Flag */
        short           i_roiqp;            /* ROI Rectangle QP (abs or offset) */
        short           i_posx;             /* ROI Rectangle StartX */
        short           i_posy;             /* ROI Rectangle StartY */
        short           i_recw;             /* ROI Rectangle Width */
        short           i_rech;             /* ROI Rectangle Height */
    } roi;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_DQM"
        unsigned int    u_phys;             /* DQM Buffer Physical Address */
        void*           p_kptr;             /* DQM Buffer Virtual Address */
        short           i_dqmw;             /* DQM MB Width */
        short           i_dqmh;             /* DQM MB Height */
        int             i_size;             /* DQM Buffer Size */
        int             i_unit;
    } dqm;

    struct
    {
        enum rqct_cfg_e i_type;             //!< MUST BE "RQCT_CFG_RCM"
        unsigned int    u_phys;
        void*           p_kptr;
        int             i_size;
    } rcm;
};

struct  rqct_buf
{
    unsigned int    u_config;   /* Not Used Now */
};

#endif //_RQCT_CFG_H_
