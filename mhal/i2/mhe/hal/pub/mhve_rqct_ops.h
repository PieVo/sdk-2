
// mstar hardware video encoder I/F (mhveif)

#ifndef _RQCT_OPS_H_
#define _RQCT_OPS_H_

#include "mhve_rqct_cfg.h"

typedef struct rqct_ops rqct_ops;

struct rqct_ops
{
    char name[32];
    void (*release)(rqct_ops* rqct);
    int (*seq_sync)(rqct_ops* rqct);
    int (*seq_done)(rqct_ops* rqct);
    int (*set_rqcf)(rqct_ops* rqct, rqct_cfg* rqcf);
    int (*get_rqcf)(rqct_ops* rqct, rqct_cfg* rqcf);
    int (*seq_conf)(rqct_ops* rqct);
    int (*enc_buff)(rqct_ops* rqct, rqct_buf* buff);
    int (*enc_conf)(rqct_ops* rqct, mhve_job* mjob);
    int (*enc_done)(rqct_ops* rqct, mhve_job* mjob);
#define RQCT_LINE_SIZE    128
    char print_line[RQCT_LINE_SIZE];
    enum
    {
        RQCT_PICTYP_I = 0,
        RQCT_PICTYP_P,
        RQCT_PICTYP_B,
    }   i_pictyp;   /* Current Frame Type */
#define IS_IPIC(t)    (RQCT_PICTYP_I==(t))
#define IS_PPIC(t)    (RQCT_PICTYP_P==(t))
#define IS_BPIC(t)    (RQCT_PICTYP_B==(t))
    int i_enc_qp;   /* Current Frame Level QP */
    int i_enc_lamda;/* Current Frame center lamda */
    int b_unrefp;   /* Indicate the frame will not be reference frame */
    int i_bitcnt;   /* Coded Bitstream Bit Count (Current Frame) */
    int i_enc_nr;   /* Encoded Frame Count */
    int i_enc_bs;   /* Coded Bitstream Byte Count (Total Frame) */

    unsigned int *auiQps;
    unsigned int *auiLambdas_SCALED;
    unsigned int *auiBits;
    unsigned int *aiIdcHist;
};

typedef struct rqct_att
{
    short   i_pict_w, i_pict_h; /* Picture Width/Height */
    short   n_fmrate, d_fmrate; /* Frame Rate */
    short   i_method, i_period; /* Rate Control Method */ /* GOP */
    short   i_leadqp, i_deltaq; /* Frame Level QP */ /* QP Delta Value */
    short   i_iupperq, i_ilowerq;/* QP Upper Boundary */ /* QP Lower Boundary */
    short   i_pupperq, i_plowerq;/* QP Upper Boundary */ /* QP Lower Boundary */
    int     i_btrate;           /* Bitrate */
    int     i_btratio;          /* Bitrate Ratio (50 ~ 100) */
    int     b_logoff;           /* Log Enable */
    short   b_ia8xlose, b_ir8xlose;   /* Avoid CU8 intra to be chosenv */ /* Avoid CU8 AMVP to be chosen */
    short   b_ia16lose, b_ir16lose;   /* Avoid CU16 intra to be chosen */ /* Avoid CU16 AMVP to be chosen */
    short   b_ir16mlos, b_ir16slos;   /* Avoid CU16 Merge to be chosen */ /* Avoid CU16 AMVP no-coef to be chosen. */
    short   b_ir16mslos, b_ia32lose;  /* Avoid CU16 Merge no-coef to be chosen*/ /*/* Avoid CU32 intra to be chosen */
    short   b_ir32mlos, b_ir32mslos;  /* Avoid CU32 Merge to be chosen */ /* Avoid CU32 Merge no-coef to be chosen.*/
    unsigned short int  u_ia8xpen, u_ir8xpen;    /* CU8 intra penalty */ /* CU8 AMVP penalty */
    unsigned short int  u_ia16pen, u_ir16pen;    /* CU16 intra penalty */ /* CU16 AMVP penalty */
    unsigned short int  u_ir16mpen, u_ir16spen;   /* CU16 Merge penalty */ /* CU16 AMVP no-coef penalty */
    unsigned short int  u_ir16mspen, u_ia32pen;  /* CU16 Merge no-coef penalty */ /* CU32 intra penalty */
    unsigned short int  u_ir32mpen, u_ir32mspen;   /* CU32 Merge penalty */ /* CU32 Merge no-coef penalty */

#define RQCT_ROI_NR     8
    unsigned int    u_roienb;              /* ROI Enable */
    short           i_absqp[RQCT_ROI_NR];  /* ROI ABS QP flag */
    signed char     i_roiqp[RQCT_ROI_NR];  /* ROI QP */
    struct roirec
    {
        short   i_posx, i_posy;
        short   i_recw, i_rech;
    }               m_roirec[RQCT_ROI_NR];  /* ROI Rectangle */

    short           i_dqmw, i_dqmh;         /* DQM MB Width/Height */
    int             b_dqmstat;  /* DQM Draw Status */
    int             i_dqmunit;  /* Not Used */
    void*           p_dqmkptr;  /* DQM Buffer Virtual Address */
    unsigned int    u_dqmphys;  /* DQM Buffer Physical Address */
    unsigned int    i_dqmsize;  /* DQM Buffer Size */
} rqct_att;

#endif //_RQCT_OPS_H_
