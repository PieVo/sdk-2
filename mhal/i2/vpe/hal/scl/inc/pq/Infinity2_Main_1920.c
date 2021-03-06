
 unsigned char QMAP_1920_Main[QM_INPUTTYPE_NUM_Main][PQ_IP_NUM_Main]=
{
    #if PQ_QM_ISP
    {//1920 FHD_YUV422, 0
    PQ_IP_MCNR_OFF_Main, PQ_IP_MCNR_NE_OFF_Main, PQ_IP_NLM_OFF_Main, PQ_IP_422to444_OFF_Main, PQ_IP_VIP_OFF_Main,
    PQ_IP_VIP_pseudo_OFF_Main, PQ_IP_VIP_LineBuffer_OFF_Main, PQ_IP_VIP_HLPF_OFF_Main, PQ_IP_VIP_HLPF_dither_OFF_Main, PQ_IP_VIP_VLPF_coef1_OFF_Main,
    PQ_IP_VIP_VLPF_coef2_OFF_Main, PQ_IP_VIP_VLPF_dither_OFF_Main, PQ_IP_VIP_Peaking_W2_Main, PQ_IP_VIP_Peaking_band_OFF_Main, PQ_IP_VIP_Peaking_adptive_OFF_Main,
    PQ_IP_VIP_Peaking_Pcoring_OFF_Main, PQ_IP_VIP_Peaking_Pcoring_ad_Y_OFF_Main, PQ_IP_VIP_Peaking_gain_0x10_Main, PQ_IP_VIP_Peaking_gain_ad_Y_OFF_Main, PQ_IP_VIP_LCE_OFF_Main,
    PQ_IP_VIP_LCE_dither_OFF_Main, PQ_IP_VIP_LCE_setting_S3_Main, PQ_IP_VIP_LCE_curve_CV1_Main, PQ_IP_VIP_DLC_His_range_OFF_Main, PQ_IP_VIP_DLC_OFF_Main,
    PQ_IP_VIP_DLC_dither_OFF_Main, PQ_IP_VIP_DLC_His_rangeH_90pa_1920_Main, PQ_IP_VIP_DLC_His_rangeV_90pa_1920_Main, PQ_IP_VIP_DLC_PC_OFF_Main, PQ_IP_VIP_YC_gain_offset_OFF_Main,
    PQ_IP_VIP_UVC_OFF_Main, PQ_IP_VIP_FCC_full_range_OFF_Main, PQ_IP_VIP_FCC_bdry_dist_OFF_Main, PQ_IP_VIP_FCC_T1_OFF_Main, PQ_IP_VIP_FCC_T2_OFF_Main,
    PQ_IP_VIP_FCC_T3_OFF_Main, PQ_IP_VIP_FCC_T4_OFF_Main, PQ_IP_VIP_FCC_T5_OFF_Main, PQ_IP_VIP_FCC_T6_OFF_Main, PQ_IP_VIP_FCC_T7_OFF_Main,
    PQ_IP_VIP_FCC_T8_OFF_Main, PQ_IP_VIP_FCC_T9_OFF_Main, PQ_IP_VIP_IHC_OFF_Main, PQ_IP_VIP_IHC_Ymode_OFF_Main, PQ_IP_VIP_IHC_dither_OFF_Main,
    PQ_IP_VIP_IHC_CRD_SRAM_15wins3_Main, PQ_IP_VIP_IHC_SETTING_HDMI_HD_Main, PQ_IP_VIP_ICC_OFF_Main, PQ_IP_VIP_ICC_Ymode_HDMI_HD_Main, PQ_IP_VIP_ICC_dither_OFF_Main,
    PQ_IP_VIP_ICC_CRD_SRAM_15wins3_Main, PQ_IP_VIP_ICC_SETTING_HDMI_HD_Main, PQ_IP_VIP_Ymode_Yvalue_ALL_Y1_Main, PQ_IP_VIP_Ymode_Yvalue_SETTING_Y2_Main, PQ_IP_VIP_IBC_OFF_Main,
    PQ_IP_VIP_IBC_dither_OFF_Main, PQ_IP_VIP_IBC_SETTING_OFF_Main, PQ_IP_VIP_ACK_OFF_Main, PQ_IP_VIP_YCbCr_Clip_OFF_Main, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_SRAM1_InvSinc4Tc4p4Fc85Fstop134Apass01Astop50G13_Main, PQ_IP_SRAM2_InvSinc4Tc4p4Fc45Apass01Astop40_Main,
    PQ_IP_SRAM3_OFF_Main, PQ_IP_SRAM4_OFF_Main, PQ_IP_C_SRAM1_C121_Main, PQ_IP_C_SRAM2_C2121_Main, PQ_IP_C_SRAM3_C161_Main,
    PQ_IP_C_SRAM4_C2121_Main, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL,
    PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_NULL, PQ_IP_YUV_Gamma_tblU_SRAM_OFF_Main,
    PQ_IP_YUV_Gamma_tblV_SRAM_OFF_Main, PQ_IP_YUV_Gamma_tblY_SRAM_OFF_Main, PQ_IP_ColorEng_GM10to12_Tbl_R_SRAM_OFF_Main, PQ_IP_ColorEng_GM10to12_Tbl_G_SRAM_OFF_Main, PQ_IP_ColorEng_GM10to12_Tbl_B_SRAM_OFF_Main,
    PQ_IP_ColorEng_GM12to10_CrcTbl_R_SRAM_OFF_Main, PQ_IP_ColorEng_GM12to10_CrcTbl_G_SRAM_OFF_Main, PQ_IP_ColorEng_GM12to10_CrcTbl_B_SRAM_OFF_Main, PQ_IP_ColorEng_WDR_alpha_c_SRAM_OFF_Main, PQ_IP_ColorEng_WDR_f_c_SRAM_OFF_Main,
    PQ_IP_ColorEng_WDR_linear_c_SRAM_OFF_Main, PQ_IP_ColorEng_WDR_nonlinear_c_SRAM_OFF_Main, PQ_IP_ColorEng_WDR_sat_c_SRAM_OFF_Main, PQ_IP_YEE_OFF_Main, PQ_IP_YEE_AC_LUT_OFF_Main,
    PQ_IP_MXNR_OFF_Main, PQ_IP_UV_ADJUST_OFF_Main, PQ_IP_XNR_OFF_Main, PQ_IP_PFC_OFF_Main, PQ_IP_YC10_UVM10_OFF_Main,
    PQ_IP_Color_Transfer_OFF_Main, PQ_IP_YUV_Gamma_OFF_Main, PQ_IP_ColorEng_422to444_OFF_Main, PQ_IP_ColorEng_YUVtoRGB_OFF_Main, PQ_IP_ColorEng_GM12to12_pre_OFF_Main,
    PQ_IP_ColorEng_CCM_OFF_Main, PQ_IP_ColorEng_HSV_OFF_Main, PQ_IP_ColorEng_GM12to12_post_OFF_Main, PQ_IP_ColorEng_WDR_OFF_Main, PQ_IP_VD_OFF_Main,
    PQ_IP_ARBSHP_OFF_Main, PQ_IP_ColorEng_RGBtoYUV_OFF_Main, PQ_IP_ColorEng_444to422_OFF_Main, PQ_IP_NVR_VPE_Config_OFF_Main, PQ_IP_NVR_VPE_OnOff_OFF_Main,
    PQ_IP_NVR_VPE_Process_OFF_Main, PQ_IP_NVR_VPE_Process_NE_OFF_Main, PQ_IP_SWDriver_ON_Main, PQ_IP_SC_End_End_Main,
    },
    #endif
};