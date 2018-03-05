/*
 * hal_mfe_device.c
 *
 *  Created on: Aug 14, 2017
 *      Author: giggs.huang
 */

#include "drv_mfe_kernel.h"
#include "mdrv_mfe_io.h"
#include "mdrv_mfe_st.h"
#include "mdrv_rqct_st.h"
#include "mdrv_rqct_io.h"
#include "drv_mfe_ctx.h"
#include "drv_mfe_dev.h"
#include "hal_mfe_api.h"
#include "mhal_mfe.h"
#include "mhal_venc.h"
#include "mhal_ut_wrapper.h"
#include "mhve_pmbr_cfg.h"
#ifdef SUPPORT_CMDQ_SERVICE
#include "mhal_cmdq.h"
#endif

#define CBR_UPPER_QP        48
#define CBR_LOWER_QP        12

#define SPECVERSIONMAJOR 1
#define SPECVERSIONMINOR 0
#define SPECREVISION 0
#define SPECSTEP 0

#ifdef USE_PHYSICAL_ADDR
#define ADDR_TYPE_SELECT(buf)       (CamOsDirectMemPhysToMiu((void*)(u32)buf))
#else
#define ADDR_TYPE_SELECT(buf)       (buf)
#endif


void _MfeSetHeader(VOID* header, MS_U32 size) {
    MHAL_VENC_Version_t* ver = (MHAL_VENC_Version_t*)header;
    ver->u32Size = size;

    ver->s.u8VersionMajor = SPECVERSIONMAJOR;
    ver->s.u8VersionMinor = SPECVERSIONMINOR;
    ver->s.u8Revision = SPECREVISION;
    ver->s.u8Step = SPECSTEP;
}

MHAL_ErrCode_e _MfeCheckHeader(VOID* pHeader, MS_U32 u32Size)
{
    MHAL_ErrCode_e eError = 0;

    MHAL_VENC_Version_t* ver;

    if (pHeader == NULL) {
        CamOsPrintf("In %s the header is null\n",__func__);
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    ver = (MHAL_VENC_Version_t*)pHeader;

    if(ver->u32Size != u32Size) {
        CamOsPrintf("In %s the header has a wrong size %i should be %i\n",__func__,ver->u32Size,u32Size);
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    if(ver->s.u8VersionMajor != SPECVERSIONMAJOR ||
            ver->s.u8VersionMinor != SPECVERSIONMINOR) {
        CamOsPrintf("The version does not match\n");
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    return eError;
}

int _MfeGenSeiNalu(char* UserData, int Len, int Idx, char* SeiData)
{
    int i=0;
    int data_len = Len;
    int total_len = data_len + 16;
    int ren_len = total_len;
    int offset=0;

    SeiData[0] = SeiData[1] = SeiData[2] = 0x00;
    SeiData[3] = 0x01;
    SeiData[4] = 0x06;
    SeiData[5] = 0x05; //user_data_unregistered type

    offset=6;
    while( ren_len >=255 ){
        SeiData[offset] = 0xFF;
        ren_len -=255;
        offset++;
    }
    SeiData[offset++] = ren_len;

    for (i=offset; i<=offset+15; i++)
    {
        if( Idx ==0 )
            SeiData[i] = 0xAA; //uuid_iso_iec_11578 , 16 bytes
        else if( Idx ==1 )
            SeiData[i] = 0x55; //uuid_iso_iec_11578 , 16 bytes
        else if( Idx ==2 )
            SeiData[i] = 0x66; //uuid_iso_iec_11578 , 16 bytes
        else if( Idx ==3 )
            SeiData[i] = 0x77; //uuid_iso_iec_11578 , 16 bytes
    }
    offset+=16;

    for( i=0; i<data_len; i++)
    {
        SeiData[offset++]= UserData[i];     //user_data_payload_byte
    }

    SeiData[offset++] = 0x80;   //rbsp_trailing_bits

    return offset;
}

MS_S32 MHAL_MFE_CreateDevice(MS_U32 u32DevId, MHAL_VENC_DEV_HANDLE *phDev)
{
    mmfe_dev *mdev = NULL;
    mhve_ios* mios = NULL;
    mhve_reg mregs = {0};

    *phDev = NULL;

    do
    {
        if (!(mdev = CamOsMemAlloc(sizeof(mmfe_dev))))
            break;

        CamOsMutexInit(&mdev->m_mutex);
        CamOsTsemInit(&mdev->tGetBitsSem, 1);
        CamOsTsemInit(&mdev->m_wqh, MMFE_DEV_STATE_IDLE);

        if (!(mdev->p_asicip = MfeIosAcquire("mfe5")))
            break;
        mios = mdev->p_asicip;

        if (0 != MfeDevGetResourceMem(u32DevId, &mregs.base, &mregs.size))
        {
            CamOsPrintf("MHAL_MFE_CreateDevice bank init fail\n");
            break;
        }
        mregs.i_id = 0;
        mios->set_bank(mios, &mregs);
        mdev->p_reg_base = mregs.base;

        if (0 != MfeDevClkInit(mdev, u32DevId))
        {
            CamOsPrintf("MHAL_MFE_CreateDevice clock init fail\n");
            //break;    // fixme: not break for FPGA easy test
        }

        if (0 != MfeDevGetResourceIrq(u32DevId, &mdev->i_irq))
        {
            CamOsPrintf("MHAL_MFE_CreateDevice irq fail (0)\n");
            break;
        }

        mdev->i_rctidx = 2;

        *phDev = (MHAL_VENC_DEV_HANDLE)mdev;

        return 0;
    }
    while (0);

    if (mdev)
    {
        mhve_ios* mios = mdev->p_asicip;
        if (mios)
            mios->release(mios);
        CamOsMemRelease(mdev);
    }

    return E_MHAL_ERR_ILLEGAL_PARAM;
}

MS_S32 MHAL_MFE_DestroyDevice(MHAL_VENC_DEV_HANDLE hDev)
{
    MHAL_ErrCode_e eError = 0;
    mmfe_dev *mdev = (mmfe_dev *)hDev;
    mhve_ios* mios = NULL;

    if (mdev)
    {
        mios = mdev->p_asicip;

        clk_put(mdev->p_clocks[0]);
        if (mios)
            mios->release(mios);
        CamOsMemRelease(mdev);

        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MFE_GetDevConfig(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    mmfe_dev *mdev = (mmfe_dev *)hDev;
    MHAL_ErrCode_e eError;
    MHAL_VENC_ParamInt_t* pVEncInt = NULL;

    if (mdev)
    {
        switch (eType)
        {
        case E_MHAL_VENC_HW_IRQ_NUM:
            if(pstParam == NULL)
                return E_MHAL_ERR_NULL_PTR;

            pVEncInt = (MHAL_VENC_ParamInt_t* )pstParam;

            if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamInt_t))))
            {
                break;
            }

            pVEncInt->u32Val = mdev->i_irq;

            eError = 0;
            break;

        case E_MHAL_VENC_HW_CMDQ_BUF_LEN:
            if(pstParam == NULL)
                return E_MHAL_ERR_NULL_PTR;

            pVEncInt = (MHAL_VENC_ParamInt_t* )pstParam;

            if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamInt_t))))
            {
                break;
            }

            pVEncInt->u32Val = 0x4000;

            eError = 0;
            break;
        }
    }
    else
    {
        return E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MFE_CreateInstance(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_INST_HANDLE *phInst)
{
    mmfe_dev* mdev = (mmfe_dev *)hDev;
    mmfe_ctx* mctx;

    if (!mdev)
        return E_MHAL_ERR_NULL_PTR;

    if (!(mctx = MfeCtxAcquire(mdev)))
        return E_MHAL_ERR_BUSY;
    if (0 <= MfeDevRegister(mdev, mctx))
    {
        *phInst = mctx;
        return 0;
    }

    mctx->release(mctx);

    MfeCtxActions(mctx, IOCTL_MMFE_STREAMOFF, NULL);

    return E_MHAL_ERR_ILLEGAL_PARAM;
}

MS_S32 MHAL_MFE_DestroyInstance(MHAL_VENC_INST_HANDLE hInst)
{
    MHAL_ErrCode_e eError = 0;
    mmfe_ctx* mctx = (mmfe_ctx*)hInst;

    if (mctx)
    {
        MfeCtxActions(mctx, IOCTL_MMFE_STREAMOFF, NULL);
        MfeDevUnregister(mctx->p_device, mctx);
        mctx->release(mctx);
        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MFE_SetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    MHAL_ErrCode_e eError = 0;
    mmfe_ctx* mctx = (mmfe_ctx* )hInst;

    mmfe_parm mfeparam;
    mmfe_ctrl vctrl;
    rqct_conf rqcnf;

    memset((void *)&mfeparam, 0, sizeof(mfeparam));
    memset((void *)&vctrl, 0, sizeof(vctrl));
    memset((void *)&rqcnf, 0, sizeof(rqcnf));

    if(NULL == mctx)
    {
        eError = E_MHAL_ERR_NULL_PTR;
        CamOsPrintf("In %s parameter mctx is null! err = %x\n",__func__, eError);
        return eError;
    }

    switch (eType)
    {
        case E_MHAL_VENC_IDX_STREAM_ON:
            {
                MHAL_VENC_InternalBuf_t *pVEncBuf = (MHAL_VENC_InternalBuf_t *)pstParam;

                if (pstParam)
                {
                    if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_InternalBuf_t))))
                    {
                        break;
                    }

                    if (pVEncBuf->pu8IntrAlVirBuf &&
                        pVEncBuf->phyIntrAlPhyBuf &&
                        pVEncBuf->u32IntrAlBufSize &&
                        pVEncBuf->phyIntrRefPhyBuf &&
                        pVEncBuf->u32IntrRefBufSize)
                    {
                        mctx->p_ialva = (unsigned char*)pVEncBuf->pu8IntrAlVirBuf;
                        mctx->p_ialma = (unsigned char*)(u32)ADDR_TYPE_SELECT(pVEncBuf->phyIntrAlPhyBuf);
                        mctx->i_ialsz = pVEncBuf->u32IntrAlBufSize;
                        mctx->p_ircma = (unsigned char*)(u32)ADDR_TYPE_SELECT(pVEncBuf->phyIntrRefPhyBuf);
                        mctx->i_ircms = pVEncBuf->u32IntrRefBufSize;
                    }
                }
                MfeCtxActions(mctx, IOCTL_MMFE_STREAMON, NULL);

                /*            CamOsPrintf("mctx->i_ialsz = %d, mctx->p_ialva = %p, mctx->p_ialma = %p, mctx->p_ircma = %p, mctx->i_ircms = %d\n",
                            mctx->i_ialsz,mctx->p_ialva,mctx->p_ialma, mctx->p_ircma, mctx->i_ircms);*/

                break;
            }
        case E_MHAL_VENC_IDX_STREAM_OFF:
            {
                MfeCtxActions(mctx, IOCTL_MMFE_STREAMOFF, NULL);
                break;
            }
            //Cfg setting
        case E_MHAL_VENC_PARTIAL_NOTIFY:
            {
                MHAL_VENC_PartialNotify_t* pVEncPartialNotify = (MHAL_VENC_PartialNotify_t* )pstParam;
                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_PartialNotify_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                mfeparam.res.i_NotifySize = pVEncPartialNotify->u32NotifySize;
                mfeparam.res.notifyFunc = pVEncPartialNotify->notifyFunc;
                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_RESOLUTION:
            {

                MHAL_VENC_Resoluton_t *pVEncRes = (MHAL_VENC_Resoluton_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_Resoluton_t))))
                {
                    break;
                }

                // resolution
                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                mfeparam.res.i_pict_w = pVEncRes->u32Width;
                mfeparam.res.i_pict_h = pVEncRes->u32Height;
                mfeparam.res.i_cropen = 0;
                mfeparam.res.i_crop_offset_x = 0;
                mfeparam.res.i_crop_offset_y = 0;
                mfeparam.res.i_crop_w = 0;
                mfeparam.res.i_crop_h = 0;
                mfeparam.res.i_pixfmt = pVEncRes->eFmt;
                mfeparam.res.i_outlen = -1;
                mfeparam.res.i_flags = 0;
                //mfeparam.res.i_ClkEn = 0; //pstParam->sClock.nClkEn;     // Giggs Check
                //mfeparam.res.i_ClkSor = 0; //pstParam->sClock.nClkSor;   // Giggs Check
                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_CROP:
            {

                MHAL_VENC_CropCfg_t* pVEncCrop = (MHAL_VENC_CropCfg_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_CropCfg_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                if((mfeparam.res.i_pict_w == 0) || (mfeparam.res.i_pict_h == 0))
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                mfeparam.res.i_cropen =   pVEncCrop->bEnable;
                mfeparam.res.i_crop_offset_x = pVEncCrop->stRect.u32X;
                mfeparam.res.i_crop_offset_y = pVEncCrop->stRect.u32Y;
                mfeparam.res.i_crop_w = pVEncCrop->stRect.u32W;
                mfeparam.res.i_crop_h = pVEncCrop->stRect.u32H;
                mfeparam.res.i_outlen = -1;    // rewrite again
                mfeparam.res.i_flags = 0;

                if((mfeparam.res.i_crop_w == 0) || (mfeparam.res.i_crop_h == 0))
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                if(((mfeparam.res.i_crop_offset_x + mfeparam.res.i_crop_w) > mfeparam.res.i_pict_w ) ||
                    (( mfeparam.res.i_crop_offset_y +  mfeparam.res.i_crop_h) > mfeparam.res.i_pict_h))
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);
                break;
            }
        case E_MHAL_VENC_264_REF:
            {
                MHAL_VENC_ParamRef_t* pVEncRef = (MHAL_VENC_ParamRef_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamRef_t))))
                {
                    break;
                }

                pVEncRef = pVEncRef;

                //TODO

                break;
            }
        case E_MHAL_VENC_264_VUI:
            {
                MHAL_VENC_ParamH264Vui_t* pVEncVui = (MHAL_VENC_ParamH264Vui_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Vui_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_VUI;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                mfeparam.vui.i_sar_w = pVEncVui->stVuiAspectRatio.u16SarWidth;
                mfeparam.vui.i_sar_h = pVEncVui->stVuiAspectRatio.u16SarHeight;

                mfeparam.vui.b_timing_info_pres = pVEncVui->stVuiTimeInfo.u8TimingInfoPresentFlag;

                mfeparam.vui.b_video_full_range = pVEncVui->stVuiVideoSignal.u8VideoFullRangeFlag;
                mfeparam.vui.b_video_signal_pres = pVEncVui->stVuiVideoSignal.u8VideoSignalTypePresentFlag;
                mfeparam.vui.i_video_format = pVEncVui->stVuiVideoSignal.u8VideoFormat;
                mfeparam.vui.b_colour_desc_pres = pVEncVui->stVuiVideoSignal.u8ColourDescriptionPresentFlag;

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_DBLK:
            {
                MHAL_VENC_ParamH264Dblk_t* pVEncDblk = (MHAL_VENC_ParamH264Dblk_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Dblk_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                //deblocking
                mfeparam.avc.i_disable_deblocking_idc = pVEncDblk->disable_deblocking_filter_idc;
                mfeparam.avc.i_alpha_c0_offset = pVEncDblk->slice_alpha_c0_offset_div2;
                mfeparam.avc.i_beta_offset = pVEncDblk->slice_beta_offset_div2;
                mfeparam.avc.b_deblock_filter_control = 1;

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_ENTROPY:
            {
                MHAL_VENC_ParamH264Entropy_t* pVEncEntropy = (MHAL_VENC_ParamH264Entropy_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Entropy_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                mfeparam.avc.b_cabac_i = pVEncEntropy->u32EntropyEncModeI;
                mfeparam.avc.b_cabac_p = pVEncEntropy->u32EntropyEncModeP;
                if (mfeparam.avc.b_cabac_i || mfeparam.avc.b_cabac_p)
                    mfeparam.avc.i_profile = MMFE_AVC_PROFILE_MP;

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_TRANS:
            {
                MHAL_VENC_ParamH264Trans_t* pVEncTrans = (MHAL_VENC_ParamH264Trans_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Trans_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                //pVEncTrans->u32IntraTransMode = pVEncTrans->u32IntraTransMode;    // Not Support
                //pVEncTrans->u32InterTransMode = pVEncTrans->u32InterTransMode;    // Not Support
                mfeparam.avc.i_chroma_qp_index_offset = pVEncTrans->s32ChromaQpIndexOffset;

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_INTRA_PRED:
            {
                MHAL_VENC_ParamH264IntraPred_t* pVEncIntraPred = (MHAL_VENC_ParamH264IntraPred_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264IntraPred_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                //Intra prediction
                mfeparam.avc.b_constrained_intra_pred = pVEncIntraPred->constrained_intra_pred_flag;
                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);


                rqcnf.type = RQCT_CONF_PEN;
                MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);

                rqcnf.pen.i_peni4x = pVEncIntraPred->u32Intra4x4Penalty;
                rqcnf.pen.i_peni16 = pVEncIntraPred->u32Intra16x16Penalty;
                rqcnf.pen.i_penYpl = pVEncIntraPred->bIntraPlanarPenalty;
                rqcnf.pen.i_penCpl = pVEncIntraPred->bIntraPlanarPenalty;

                MfeCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);

                break;
            }
        case E_MHAL_VENC_264_INTER_PRED:
            {
                MHAL_VENC_ParamH264InterPred_t* pVEncInterPred = (MHAL_VENC_ParamH264InterPred_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264InterPred_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_MOT;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                mfeparam.mot.i_dmv_x = pVEncInterPred->u32HWSize;
                mfeparam.mot.i_dmv_y = pVEncInterPred->u32VWSize;
                mfeparam.mot.i_subpel = MMFE_SUBPEL_QUATER;    // Giggs Check
                mfeparam.mot.i_mvblks[1] = 0;
                if (pVEncInterPred->bInter8x8PredEn)
                {
                    mfeparam.mot.i_mvblks[0] |= MMFE_MVBLK_8x8;
                }
                else
                {
                    mfeparam.mot.i_mvblks[0] &= ~MMFE_MVBLK_8x8;
                }
                if (pVEncInterPred->bInter8x16PredEn)
                {
                    mfeparam.mot.i_mvblks[0] |= MMFE_MVBLK_8x16;
                }
                else
                {
                    mfeparam.mot.i_mvblks[0] &= ~MMFE_MVBLK_8x16;
                }
                if (pVEncInterPred->bInter16x8PredEn)
                {
                    mfeparam.mot.i_mvblks[0] |= MMFE_MVBLK_16x8;
                }
                else
                {
                    mfeparam.mot.i_mvblks[0] &= ~MMFE_MVBLK_16x8;
                }
                if (pVEncInterPred->bInter16x16PredEn)
                {
                    mfeparam.mot.i_mvblks[0] |= MMFE_MVBLK_16x16;
                }
                else
                {
                    mfeparam.mot.i_mvblks[0] &= ~MMFE_MVBLK_16x16;
                }

                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_I_SPLIT_CTL:
            {
                MHAL_VENC_ParamSplit_t* pVEncSplit = (MHAL_VENC_ParamSplit_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamSplit_t))))
                {
                    break;
                }

                vctrl.type = MMFE_CTRL_SPL;
                MfeCtxActions(mctx, IOCTL_MMFE_G_CTRL, &vctrl);

                if(pVEncSplit->bSplitEnable)
                {
                    vctrl.spl.i_rows = pVEncSplit->u32SliceRowCount;
                    vctrl.spl.i_bits = 0;   //derek check
                }

                MfeCtxActions(mctx, IOCTL_MMFE_S_CTRL, &vctrl);

                break;
            }
        case E_MHAL_VENC_264_ROI:
            {
                MHAL_VENC_RoiCfg_t* pVEncRoiCfg = (MHAL_VENC_RoiCfg_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RoiCfg_t))))
                {
                    break;
                }

                vctrl.type = MMFE_CTRL_ROI;
                vctrl.roi.i_index = pVEncRoiCfg->u32Index;

                MfeCtxActions(mctx, IOCTL_MMFE_G_CTRL, (VOID *)&vctrl);

                if(pVEncRoiCfg->bEnable)
                {
                    vctrl.roi.i_absqp = pVEncRoiCfg->bAbsQp;
                    vctrl.roi.i_roiqp = pVEncRoiCfg->s32Qp;
                    vctrl.roi.i_mbx = (pVEncRoiCfg->stRect.u32X+15)/16;
                    vctrl.roi.i_mby = (pVEncRoiCfg->stRect.u32Y+15)/16;
                    vctrl.roi.i_mbw = (pVEncRoiCfg->stRect.u32W+15)/16;
                    vctrl.roi.i_mbh = (pVEncRoiCfg->stRect.u32H+15)/16;
                }
                else
                {
                    vctrl.roi.i_absqp = 0;
                    vctrl.roi.i_roiqp = 0;
                    vctrl.roi.i_mbx = 0;
                    vctrl.roi.i_mby = 0;
                    vctrl.roi.i_mbw = 0;
                    vctrl.roi.i_mbh = 0;
                }

                MfeCtxActions(mctx, IOCTL_MMFE_S_CTRL, (VOID *)&vctrl);

                break;
            }
        case E_MHAL_VENC_264_RC:
            {
                MHAL_VENC_RcInfo_t* pVEncRcInfo = (MHAL_VENC_RcInfo_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RcInfo_t))))
                {
                    break;
                }

                if(E_MHAL_VENC_RC_MODE_H264CBR == pVEncRcInfo->eRcMode)
                {
                    // Set BPS
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.bps.i_method = RQCT_METHOD_CBR;
                    mfeparam.bps.i_ref_qp = -1;        // Giggs Check
                    mfeparam.bps.i_bps = pVEncRcInfo->stAttrH264Cbr.u32BitRate;
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set FPS
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.fps.i_num = pVEncRcInfo->stAttrH264Cbr.u32SrcFrmRate;
                    mfeparam.fps.i_den = 1;
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set GOP
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.gop.i_pframes = pVEncRcInfo->stAttrH264Cbr.u32Gop;
                    mfeparam.gop.i_bframes = 0;
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set RQCT
                    rqcnf.type = RQCT_CONF_QPR;
                    MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                    rqcnf.type = RQCT_CONF_QPR;
                    rqcnf.qpr.i_iupperq = CBR_UPPER_QP;
                    rqcnf.qpr.i_ilowerq = CBR_LOWER_QP;
                    rqcnf.qpr.i_pupperq = CBR_UPPER_QP;
                    rqcnf.qpr.i_plowerq = CBR_LOWER_QP;
                    rqcnf.type = RQCT_CONF_QPR;
                    MfeCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
                }
                else if(E_MHAL_VENC_RC_MODE_H264VBR == pVEncRcInfo->eRcMode)
                {
                    // Set BPS
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.bps.i_method = RQCT_METHOD_VBR;
                    mfeparam.bps.i_ref_qp = -1;        // Giggs Check
                    mfeparam.bps.i_bps = pVEncRcInfo->stAttrH264Vbr.u32MaxBitRate;
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set FPS
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.fps.i_num = pVEncRcInfo->stAttrH264Vbr.u32SrcFrmRate;
                    mfeparam.fps.i_den = 1;
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set GOP
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.gop.i_pframes = pVEncRcInfo->stAttrH264Vbr.u32Gop;
                    mfeparam.gop.i_bframes = 0;
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set RQCT
                    rqcnf.type = RQCT_CONF_QPR;
                    MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                    rqcnf.type = RQCT_CONF_QPR;
                    rqcnf.qpr.i_iupperq = pVEncRcInfo->stAttrH264Vbr.u32MaxQp;
                    rqcnf.qpr.i_ilowerq = pVEncRcInfo->stAttrH264Vbr.u32MinQp;
                    rqcnf.qpr.i_pupperq = pVEncRcInfo->stAttrH264Vbr.u32MaxQp;
                    rqcnf.qpr.i_plowerq = pVEncRcInfo->stAttrH264Vbr.u32MinQp;
                    rqcnf.type = RQCT_CONF_QPR;
                    MfeCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
                }
                else if(E_MHAL_VENC_RC_MODE_H264FIXQP == pVEncRcInfo->eRcMode)
                {
                    // Set BPS
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.bps.i_method = RQCT_METHOD_CQP;
                    mfeparam.bps.i_ref_qp = pVEncRcInfo->stAttrH264FixQp.u32IQp;        // Giggs Check
                    mfeparam.bps.i_bps = 0;
                    mfeparam.type = MMFE_PARM_BPS ;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set FPS
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.fps.i_num = pVEncRcInfo->stAttrH264FixQp.u32SrcFrmRate;
                    mfeparam.fps.i_den = 1;
                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                    // Set GOP
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    mfeparam.gop.i_pframes = pVEncRcInfo->stAttrH264FixQp.u32Gop;
                    mfeparam.gop.i_bframes = 0;
                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);


                    //rqcnf.type = RQCT_CONF_QPR;
                    //MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                    //rqcnf.type = RQCT_CONF_QPR;
                    //MfeCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
                }
                else
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                break;
            }
        case E_MHAL_VENC_264_USER_DATA:
            {
                MHAL_VENC_UserData_t* pVEncUserData = (MHAL_VENC_UserData_t *)pstParam;
                char SeiData[MMFE_SEI_MAX_LEN];
                int SeiLen = 0;
                mmfe_buff ibuff = {0};

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_UserData_t))))
                {
                    break;
                }

                if (mctx->i_usrcn >= 4 || pVEncUserData->u32Len > 1024)
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                SeiLen = _MfeGenSeiNalu(pVEncUserData->pu8Data, pVEncUserData->u32Len, mctx->i_usrcn, SeiData);

                ibuff.i_memory = MMFE_MEMORY_USER;
                ibuff.i_planes = 1;
                ibuff.planes[0].i_size = MMFE_SEI_MAX_LEN;
                ibuff.planes[0].mem.uptr = SeiData;
                ibuff.planes[0].i_used = SeiLen;
                MfeCtxActions(mctx, IOCTL_MMFE_S_DATA, (VOID *)&ibuff);

                break;
            }
        case E_MHAL_VENC_ENABLE_IDR:
            {
                MHAL_VENC_EnableIdr_t* pVEncEnableIdr = (MHAL_VENC_EnableIdr_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_EnableIdr_t))))
                {
                    break;
                }

                // Set GOP
                mfeparam.type = MMFE_PARM_GOP;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                mfeparam.gop.b_passiveI = ((pVEncEnableIdr->bEnable==0)?1:0);
                //CamOsPrintf("%s:%d pVEncEnableIdr->bEnable %d  b_passiveI %d\n", __FUNCTION__, __LINE__, pVEncEnableIdr->bEnable, mfeparam.gop.b_passiveI);
                mfeparam.type = MMFE_PARM_GOP;
                MfeCtxActions(mctx, IOCTL_MMFE_S_PARM, (VOID *)&mfeparam);

                break;
            }
        case E_MHAL_VENC_264_FRAME_LOST:
            {
                MHAL_VENC_ParamFrameLost_t* pVencParamFrameLost = (MHAL_VENC_ParamFrameLost_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamFrameLost_t))))
                {
                    break;
                }

                pVencParamFrameLost = pVencParamFrameLost;

                break;
            }
        case E_MHAL_VENC_264_FRAME_CFG:
            {
                MHAL_VENC_SuperFrameCfg_t* pVencSuperFrameCfg = (MHAL_VENC_SuperFrameCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_SuperFrameCfg_t))))
                {
                    break;
                }

                pVencSuperFrameCfg = pVencSuperFrameCfg;

                break;
            }
        case E_MHAL_VENC_264_RC_PRIORITY:
            {
                MHAL_VENC_RcPriorityCfg_t* pVencRcPriorityCfg = (MHAL_VENC_RcPriorityCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RcPriorityCfg_t))))
                {
                    break;
                }

                pVencRcPriorityCfg = pVencRcPriorityCfg;

                break;
            }
        default:
            CamOsPrintf("In %s command is not support! err = %x\n",__func__, eError);
            eError = E_MHAL_ERR_NOT_SUPPORT;
            break;

    }

    return eError;
}

MS_S32 MHAL_MFE_GetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    MHAL_ErrCode_e eError;
    mmfe_ctx* mctx = (mmfe_ctx*)hInst;

    mmfe_parm mfeparam;
    mmfe_ctrl vctrl;
    rqct_conf rqcnf;

    memset((void *)&mfeparam, 0 , sizeof(mmfe_parm));
    memset((void *)&vctrl, 0, sizeof(mmfe_ctrl));
    memset((void *)&rqcnf, 0, sizeof(rqct_conf));

    if(mctx == NULL)
        return E_MHAL_ERR_NULL_PTR;
    if(pstParam == NULL)
        return E_MHAL_ERR_NULL_PTR;

    switch (eType)
    {
        //Cfg setting
        case E_MHAL_VENC_PARTIAL_NOTIFY:
            {
                MHAL_VENC_PartialNotify_t* pVEncPartialNotify = (MHAL_VENC_PartialNotify_t* )pstParam;
                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_PartialNotify_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncPartialNotify->u32NotifySize = mfeparam.res.i_NotifySize;
                pVEncPartialNotify->notifyFunc = mfeparam.res.notifyFunc;

                break;
            }
        case E_MHAL_VENC_264_RESOLUTION:
            {
                MHAL_VENC_Resoluton_t* pVEncRes = (MHAL_VENC_Resoluton_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_Resoluton_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncRes->u32Width = mfeparam.res.i_pict_w;
                pVEncRes->u32Height = mfeparam.res.i_pict_h;
                pVEncRes->eFmt = mfeparam.res.i_pixfmt;

                break;
            }
        case E_MHAL_VENC_264_CROP:
            {
                MHAL_VENC_CropCfg_t* pVEncCropCfg = (MHAL_VENC_CropCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_CropCfg_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_RES;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncCropCfg->bEnable = mfeparam.res.i_cropen;
                pVEncCropCfg->stRect.u32X = mfeparam.res.i_crop_offset_x;
                pVEncCropCfg->stRect.u32Y = mfeparam.res.i_crop_offset_y;
                pVEncCropCfg->stRect.u32W = mfeparam.res.i_crop_w;
                pVEncCropCfg->stRect.u32H = mfeparam.res.i_crop_h;
                break;
            }
        case E_MHAL_VENC_264_REF:
            {
                MHAL_VENC_ParamRef_t* pVEncRef = (MHAL_VENC_ParamRef_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamRef_t))))
                {
                    break;
                }

                pVEncRef->u32RefLayerMode = 0;

                break;
            }
        case E_MHAL_VENC_264_VUI:
            {
                MHAL_VENC_ParamH264Vui_t* pVEncVui = (MHAL_VENC_ParamH264Vui_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Vui_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_VUI;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncVui->stVuiAspectRatio.u16SarWidth = mfeparam.vui.i_sar_w;
                pVEncVui->stVuiAspectRatio.u16SarHeight = mfeparam.vui.i_sar_h;

                pVEncVui->stVuiTimeInfo.u8TimingInfoPresentFlag = mfeparam.vui.b_timing_info_pres;

                pVEncVui->stVuiVideoSignal.u8VideoFullRangeFlag = mfeparam.vui.b_video_full_range;
                pVEncVui->stVuiVideoSignal.u8VideoSignalTypePresentFlag = mfeparam.vui.b_video_signal_pres;
                pVEncVui->stVuiVideoSignal.u8VideoFormat = mfeparam.vui.i_video_format;
                pVEncVui->stVuiVideoSignal.u8ColourDescriptionPresentFlag = mfeparam.vui.b_colour_desc_pres;

                break;
            }
        case E_MHAL_VENC_264_DBLK:
            {
                MHAL_VENC_ParamH264Dblk_t* pVEncDblk = (MHAL_VENC_ParamH264Dblk_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Dblk_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncDblk->disable_deblocking_filter_idc = mfeparam.avc.i_disable_deblocking_idc;
                pVEncDblk->slice_alpha_c0_offset_div2 = mfeparam.avc.i_alpha_c0_offset;
                pVEncDblk->slice_beta_offset_div2 = mfeparam.avc.i_beta_offset;

                break;
            }
        case E_MHAL_VENC_264_ENTROPY:
            {
                MHAL_VENC_ParamH264Entropy_t* pVEncEntropy =  (MHAL_VENC_ParamH264Entropy_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Entropy_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncEntropy->u32EntropyEncModeI = mfeparam.avc.b_cabac_i;    //derek check
                pVEncEntropy->u32EntropyEncModeP = mfeparam.avc.b_cabac_p;    //derek check

                break;
            }
        case E_MHAL_VENC_264_TRANS:
            {
                MHAL_VENC_ParamH264Trans_t* pVEncTrans = (MHAL_VENC_ParamH264Trans_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264Trans_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncTrans->u32IntraTransMode = 0;      // Not Support
                pVEncTrans->u32InterTransMode = 0;      // Not Support
                pVEncTrans->s32ChromaQpIndexOffset = mfeparam.avc.i_chroma_qp_index_offset;

                break;
            }
        case E_MHAL_VENC_264_INTRA_PRED:
            {
                MHAL_VENC_ParamH264IntraPred_t* pVEncIntraPred =  (MHAL_VENC_ParamH264IntraPred_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264IntraPred_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_AVC;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncIntraPred->constrained_intra_pred_flag = mfeparam.avc.b_constrained_intra_pred;

                rqcnf.type = RQCT_CONF_PEN;
                MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);

                pVEncIntraPred->bIntra16x16PredEn = 1;
                pVEncIntraPred->bIntraNxNPredEn = 1;
                pVEncIntraPred->u32Intra4x4Penalty = rqcnf.pen.i_peni4x;
                pVEncIntraPred->u32Intra16x16Penalty = rqcnf.pen.i_peni16;
                pVEncIntraPred->bIntraPlanarPenalty = rqcnf.pen.i_penYpl;
                pVEncIntraPred->bIpcmEn = 0;

                break;
            }
        case E_MHAL_VENC_264_INTER_PRED:
            {
                MHAL_VENC_ParamH264InterPred_t* pVEncInterPred = (MHAL_VENC_ParamH264InterPred_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH264InterPred_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_MOT;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                pVEncInterPred->u32HWSize = mfeparam.mot.i_dmv_x;
                pVEncInterPred->u32VWSize = mfeparam.mot.i_dmv_y;

                if (mfeparam.mot.i_mvblks[0] | MMFE_MVBLK_8x8)
                {
                    pVEncInterPred->bInter8x8PredEn = 1;
                }
                if (mfeparam.mot.i_mvblks[0] | MMFE_MVBLK_8x16)
                {
                    pVEncInterPred->bInter8x16PredEn = 1;
                }
                if (mfeparam.mot.i_mvblks[0] | MMFE_MVBLK_16x8)
                {
                    pVEncInterPred->bInter16x8PredEn = 1;
                }
                if (mfeparam.mot.i_mvblks[0] | MMFE_MVBLK_16x16)
                {
                    pVEncInterPred->bInter16x16PredEn = 1;
                }

                pVEncInterPred->bExtedgeEn = 1;

                break;
            }
        case E_MHAL_VENC_264_I_SPLIT_CTL:
            {
                MHAL_VENC_ParamSplit_t* pVEncSplit = (MHAL_VENC_ParamSplit_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamSplit_t))))
                {
                    break;
                }

                vctrl.type = MMFE_CTRL_SPL;
                MfeCtxActions(mctx, IOCTL_MMFE_G_CTRL, &vctrl);

                pVEncSplit->u32SliceRowCount = vctrl.spl.i_rows;

                if(pVEncSplit->u32SliceRowCount != 0)
                    pVEncSplit->bSplitEnable = 1;   //derek check
                else
                    pVEncSplit->bSplitEnable = 0;

                break;
            }
        case E_MHAL_VENC_264_ROI:
            {
                MHAL_VENC_RoiCfg_t* pVEncRoiCfg = (MHAL_VENC_RoiCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RoiCfg_t))))
                {
                    break;
                }

                vctrl.type = MMFE_CTRL_ROI;
                vctrl.roi.i_index = pVEncRoiCfg->u32Index;
                MfeCtxActions(mctx, IOCTL_MMFE_G_CTRL, (VOID *)&vctrl);

                if(pVEncRoiCfg->s32Qp == 0)
                    pVEncRoiCfg->bEnable = 0;
                else
                    pVEncRoiCfg->bEnable = 1;

                pVEncRoiCfg->bAbsQp = vctrl.roi.i_absqp;
                pVEncRoiCfg->s32Qp = vctrl.roi.i_roiqp;
                pVEncRoiCfg->stRect.u32X = vctrl.roi.i_mbx << 4;
                pVEncRoiCfg->stRect.u32Y = vctrl.roi.i_mby << 4;
                pVEncRoiCfg->stRect.u32W = vctrl.roi.i_mbw << 4;
                pVEncRoiCfg->stRect.u32H = vctrl.roi.i_mbh << 4;
                pVEncRoiCfg->RoiBgCtl.s32SrcFrmRate = 0;
                pVEncRoiCfg->RoiBgCtl.s32DstFrmRate = 0;
                pVEncRoiCfg->pDaQpMap = NULL;

                break;
            }
        case E_MHAL_VENC_264_RC:
            {
                MHAL_VENC_RcInfo_t* pVEncRcInfo = (MHAL_VENC_RcInfo_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RcInfo_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_BPS ;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);

                if(RQCT_METHOD_CBR == mfeparam.bps.i_method)
                {
                    pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H264CBR;
                    pVEncRcInfo->stAttrH264Cbr.u32BitRate = mfeparam.bps.i_bps;

                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264Cbr.u32SrcFrmRate = mfeparam.fps.i_num;

                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264Cbr.u32Gop = mfeparam.gop.i_pframes;

                    //pVEncRcInfo->stAttrH264Cbr.FluctuateLevel
                    //pVEncRcInfo->stAttrH264Cbr.StatTime
                }
                else if(RQCT_METHOD_VBR == mfeparam.bps.i_method)
                {
                    pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H264VBR;
                    pVEncRcInfo->stAttrH264Vbr.u32MaxBitRate = mfeparam.bps.i_bps;

                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264Vbr.u32SrcFrmRate = mfeparam.fps.i_num;

                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264Vbr.u32Gop = mfeparam.gop.i_pframes;

                    rqcnf.type = RQCT_CONF_QPR;
                    MfeCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                    pVEncRcInfo->stAttrH264Vbr.u32MaxQp = rqcnf.qpr.i_iupperq;
                    pVEncRcInfo->stAttrH264Vbr.u32MinQp = rqcnf.qpr.i_ilowerq;
                    pVEncRcInfo->stAttrH264Vbr.u32MaxQp = rqcnf.qpr.i_pupperq;
                    pVEncRcInfo->stAttrH264Vbr.u32MinQp = rqcnf.qpr.i_plowerq;

                }
                else if(RQCT_METHOD_CQP == mfeparam.bps.i_method)
                {
                    pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H264FIXQP;
                    pVEncRcInfo->stAttrH264FixQp.u32IQp = mfeparam.bps.i_ref_qp;
                    pVEncRcInfo->stAttrH264FixQp.u32PQp = mfeparam.bps.i_ref_qp;

                    mfeparam.type = MMFE_PARM_FPS;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264FixQp.u32SrcFrmRate = mfeparam.fps.i_num;

                    mfeparam.type = MMFE_PARM_GOP;
                    MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                    pVEncRcInfo->stAttrH264FixQp.u32Gop = mfeparam.gop.i_pframes;
                }
                else
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                break;
            }
        case E_MHAL_VENC_ENABLE_IDR:
            {
                MHAL_VENC_EnableIdr_t* pVEncEnableIdr = (MHAL_VENC_EnableIdr_t *)pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_EnableIdr_t))))
                {
                    break;
                }

                mfeparam.type = MMFE_PARM_GOP;
                MfeCtxActions(mctx, IOCTL_MMFE_G_PARM, (VOID *)&mfeparam);
                pVEncEnableIdr->bEnable = (mfeparam.gop.b_passiveI)?0:1;

                break;
            }
        case E_MHAL_VENC_264_FRAME_LOST:
            {
                MHAL_VENC_ParamFrameLost_t* pVEncFrameLost =  (MHAL_VENC_ParamFrameLost_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_ParamFrameLost_t))))
                {
                    break;
                }

                pVEncFrameLost = pVEncFrameLost;

                //TODO
                //            pVEncFrameLost->FrmLostMode = VEncFrameLostNormal;
                //            pVEncFrameLost->bFrmLostOpen = 0;
                //            pVEncFrameLost->u32FrmLostBpsThr = 0;
                //            pVEncFrameLost->u32EncFrmGaps = 0;

                break;
            }
        case E_MHAL_VENC_264_FRAME_CFG:
            {
                MHAL_VENC_SuperFrameCfg_t* pVEncFrameCfg =  (MHAL_VENC_SuperFrameCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_SuperFrameCfg_t))))
                {
                    break;
                }

                pVEncFrameCfg = pVEncFrameCfg;

                //TODO
                //            pVEncFrameCfg->SuperFrmMode = E_MI_VENC_SUPERFRM_NONE;
                //            pVEncFrameCfg->SuperIFrmBitsThr = 0;
                //            pVEncFrameCfg->SuperPFrmBitsThr = 0;
                //            pVEncFrameCfg->SuperBFrmBitsThr = 0;

                break;
            }
        case E_MHAL_VENC_264_RC_PRIORITY:
            {
                MHAL_VENC_RcPriorityCfg_t* pVEncRCPriority = (MHAL_VENC_RcPriorityCfg_t* )pstParam;

                if ((eError = _MfeCheckHeader(pstParam, sizeof(MHAL_VENC_RcPriorityCfg_t))))
                {
                    break;
                }

                pVEncRCPriority = pVEncRCPriority;

                //TODO
                //pVEncRCPriority->RcPriority = VEncRCPriorityBitrateFirst;

                break;
            }
        default:
            eError = E_MHAL_ERR_NOT_SUPPORT;
            break;
    }

    return eError;
}

u64 _MfeTimespecDiffNs(CamOsTimespec_t *start, CamOsTimespec_t *stop)
{
    return (stop->nSec - start->nSec)*1000000000 + stop->nNanoSec - start->nNanoSec;
}

MS_S32 MHAL_MFE_EncodeOneFrame(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InOutBuf_t* pstInOutBuf)
{
    MHAL_ErrCode_e eError = 0;
    mmfe_ctx* mctx = (mmfe_ctx*)hInst;
    MS_U32    cmd;
    mmfe_parm param;
    mmfe_buff  vbuff[2];
    mmfe_buff* ibuff = NULL;
    mmfe_buff* obuff = NULL;
    MS_U32 i;
    MS_U32 reg_addr;
    MS_U16 reg_value;
#ifdef SUPPORT_CMDQ_SERVICE
    MHAL_CMDQ_CmdqInterface_t *stCmdQInf = NULL;
    MS_U32 u32CmdQInfRetryCnt = 0;
    MS_U32 u32CmdQStat = 0;
    s32 nCmdqRet = 0;
    MS_U32 u32RegCmdLen = 0;
#endif
    u64 timer_ns_diff;

    if (mctx && pstInOutBuf)
    {
        CamOsGetMonotonicTime(&mctx->p_device->utilization_calc_end);
        if ((timer_ns_diff = _MfeTimespecDiffNs(&mctx->p_device->utilization_calc_start, &mctx->p_device->utilization_calc_end)) > 1000000000)
        {
            CamOsGetMonotonicTime(&mctx->p_device->utilization_calc_start);
            //CamOsPrintf("MFE utilization %d  %d\n", (u32)(mctx->p_device->encode_total_ns0 * 100 / timer_ns_diff), (u32)(mctx->p_device->encode_total_ns1 * 100 / timer_ns_diff));
            mctx->p_device->encode_total_ns0 = 0;
            mctx->p_device->encode_total_ns1 = 0;
        }

        CamOsGetMonotonicTime(&mctx->p_device->encode_start_time);

        // get resolution
        cmd = IOCTL_MMFE_G_PARM;
        param.type = MMFE_PARM_RES;
        MfeCtxActions(mctx, cmd, (VOID *)&param);

        ibuff = &vbuff[0];
        obuff = &vbuff[1];

        memset(vbuff, 0, sizeof(vbuff));
        ibuff->i_timecode = 0;
        ibuff->i_width = param.res.i_pict_w;
        ibuff->i_height = param.res.i_pict_h;
        ibuff->i_memory = MMFE_MEMORY_MMAP;
        switch (param.res.i_pixfmt)
        {
            case E_MHAL_VENC_FMT_NV12:
            case E_MHAL_VENC_FMT_NV21:
                ibuff->i_planes = 2;
                ibuff->planes[0].mem.phys = pstInOutBuf->phyInputYUVBuf1;
                ibuff->planes[1].mem.phys = pstInOutBuf->phyInputYUVBuf2;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height;
                ibuff->planes[1].i_size = ibuff->i_width * ibuff->i_height / 2;
                ibuff->planes[0].i_bias = 0;
                ibuff->planes[1].i_bias = 0;
                if(param.res.i_cropen)
                {
                    ibuff->planes[0].i_bias =(param.res.i_pict_w * param.res.i_crop_offset_y) + param.res.i_crop_offset_x;   // Y
                    ibuff->planes[1].i_bias =((param.res.i_pict_w * param.res.i_crop_offset_y) >> 1) + param.res.i_crop_offset_x;  // C
                }
                break;
            case E_MHAL_VENC_FMT_YUYV:
                ibuff->i_planes = 1;
                ibuff->planes[0].mem.phys = pstInOutBuf->phyInputYUVBuf1;
                ibuff->planes[1].mem.phys = 0;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height * 2;
                ibuff->planes[1].i_size = 0;
                ibuff->planes[0].i_bias = 0;
                ibuff->planes[1].i_bias = 0;
                if(param.res.i_cropen)
                    ibuff->planes[0].i_bias =((param.res.i_pict_w * param.res.i_crop_offset_y) + param.res.i_crop_offset_x) << 1;   // Y
                break;
            default:
                break;
        }
        ibuff->i_flags = 0;
        if (pstInOutBuf->bRequestI)
        {
            pstInOutBuf->bRequestI = 0;
            ibuff->i_flags |= MMFE_FLAGS_IDR;
        }
        ibuff->planes[0].i_used = ibuff->planes[0].i_size;
        ibuff->planes[1].i_used = ibuff->planes[1].i_size;

        obuff->i_memory = MMFE_MEMORY_MMAP;
        obuff->i_planes = 1;
        obuff->planes[0].mem.phys = pstInOutBuf->phyOutputBuf;
        obuff->planes[0].mem.uptr = (void *)(u32)pstInOutBuf->virtOutputBuf;
        obuff->planes[0].mem.pFlushCacheCallback = pstInOutBuf->pFlushCacheCallback;
        obuff->planes[0].i_size = pstInOutBuf->u32OutputBufSize;
        obuff->planes[0].i_used = 0;
        obuff->i_flags = 0;

#ifdef SUPPORT_CMDQ_SERVICE
        if (pstInOutBuf->pCmdQ)
        {
            // Trigger by CmdQ
            u32RegCmdLen = sizeof(mctx->p_regcmd)/sizeof(MS_U32)/2;
            MfeCtxGenCompressRegCmd(mctx, vbuff, mctx->p_regcmd, &u32RegCmdLen);

            //CamOsPrintf("Gen u32RegCmdLen: %d\n", u32RegCmdLen);

            stCmdQInf = (MHAL_CMDQ_CmdqInterface_t *)pstInOutBuf->pCmdQ;
            while (!stCmdQInf->MHAL_CMDQ_CheckBufAvailable(stCmdQInf, u32RegCmdLen))
            {
                stCmdQInf->MHAL_CMDQ_ReadStatusCmdq(stCmdQInf, &u32CmdQStat);
                if((u32CmdQStat & MHAL_CMDQ_ERROR_STATUS) != 0)
                    stCmdQInf->MHAL_CMDQ_CmdqResetEngine(stCmdQInf);

                CamOsMsSleep(2);

                u32CmdQInfRetryCnt++;
                if (u32CmdQInfRetryCnt>1000)
                    BUG();
            }

            for (i=0; i<u32RegCmdLen; i++)
            {
                reg_addr = *(MS_U32*)(mctx->p_regcmd+i*2);
                reg_value = *(MS_U32*)(mctx->p_regcmd+i*2+1);

                //CamOsPrintf("WriteReg(no.%d)    0x%08X  0x%04X\n", i, reg_addr, reg_value);

                // Fill CmdQ here
                nCmdqRet = stCmdQInf->MHAL_CMDQ_WriteRegCmdq(stCmdQInf, (reg_addr&0x00FFFFFF)>>1, reg_value);
            }

            if (((u32)mctx->p_device->p_reg_base&0x00FFFF00) == 0x00264800 ||
                ((u32)mctx->p_device->p_reg_base&0x00FFFF00) == 0x00222000)     /*Fixme: it need some info(bank addr) from hal*/
            {
                stCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(stCmdQInf, E_MHAL_CMDQEVE_CORE0_MFE_TRIG);
            }
            else if (((u32)mctx->p_device->p_reg_base&0x00FFFF00) == 0x002E7200)
            {
                stCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(stCmdQInf, E_MHAL_CMDQEVE_CORE1_MFE_TRIG);
            }
        }
        else
#endif
        {
            if (MfeCtxEncFireAndReturn(mctx, vbuff) != 0)
                eError = E_MHAL_ERR_ILLEGAL_PARAM;
        }

        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MFE_EncodeFrameDone(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_EncResult_t* pstEncRet)
{
    MHAL_ErrCode_e eError = 0;
    mmfe_ctx* mctx = (mmfe_ctx*)hInst;
    MS_U32    cmd;
    mmfe_parm param;
    mmfe_buff  vbuff[2];
    mmfe_buff* ibuff = NULL;
    mmfe_buff* obuff = NULL;
    CamOsTimespec_t timer;

    if (mctx && pstEncRet)
    {
        // get resolution
        cmd = IOCTL_MMFE_G_PARM;
        param.type = MMFE_PARM_RES;
        MfeCtxActions(mctx, cmd, (VOID *)&param);

        ibuff = &vbuff[0];
        obuff = &vbuff[1];

        memset(&vbuff, 0, sizeof(vbuff));
        ibuff->i_timecode = 0;
        ibuff->i_width = param.res.i_pict_w;
        ibuff->i_height = param.res.i_pict_h;
        ibuff->i_memory = MMFE_MEMORY_MMAP;
        switch (param.res.i_pixfmt)
        {
            case MMFE_PIXFMT_NV12:
            case MMFE_PIXFMT_NV21:
                ibuff->i_planes = 2;
                ibuff->planes[1].mem.phys = ibuff->planes[0].mem.phys + ibuff->i_width * ibuff->i_height;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height;
                ibuff->planes[1].i_size = ibuff->i_width * ibuff->i_height / 2;
                break;
            case MMFE_PIXFMT_YUYV:
                ibuff->i_planes = 1;
                ibuff->planes[1].mem.phys = 0;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height * 2;
                ibuff->planes[1].i_size = 0;
                break;
            default:
                break;
        }
        ibuff->i_flags = 0;     // Giggs check: MMFE_FLAGS_DISPOSABLE
        ibuff->planes[0].i_used = ibuff->planes[0].i_size;
        ibuff->planes[1].i_used = ibuff->planes[1].i_size;

        obuff->i_memory = MMFE_MEMORY_MMAP;
        obuff->i_planes = 1;
        obuff->planes[0].i_used = 0;
        obuff->i_flags = 0;

        //MfeCtxActions(mctx, IOCTL_MMFE_ACQUIRE_NONBLOCKINGA, (VOID *)vbuff);
        if (MfeCtxEncPostProc(mctx, vbuff) != 0)
        {
            eError = E_MHAL_ERR_BUF_FULL;
        }

        pstEncRet->u32OutputBufUsed = obuff->planes[0].i_used;

        CamOsGetMonotonicTime(&timer);
        mctx->p_device->encode_total_ns1 += _MfeTimespecDiffNs(&mctx->p_device->encode_start_time, &timer);

        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MFE_QueryBufSize(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InternalBuf_t *pstParam)
{

    MS_U32 size_total_al = 0;
    MS_U32 size_total_ref = 0;
    mmfe_ctx* mctx = (mmfe_ctx*)hInst;
    mhve_ops* mops = mctx->p_handle;
    mhve_cfg mcfg;
    int size_out, size_mbs, size_mbp, size_dqm, size_dump_reg, size_refrec;
    int size_lum, size_chr, mbw, mbh, mbn;
    int size_pmbr, size_pmbr_map, size_pmbr_lut;
    int rpbn;

    if (MMFE_CTX_STATE_NULL != mctx->i_state)
        return E_MHAL_ERR_NULL_PTR;

    mctx->i_dmems = 0;

    mcfg.type = MHVE_CFG_RES;
    mops->get_conf(mops, &mcfg);
    mbw = _ALIGN_(4, mcfg.res.i_pixw)/16;
    mbh = _ALIGN_(4, mcfg.res.i_pixh)/16;
    mbn = mbw*mbh;

    /* Calculate required buffer size */
    size_mbp = _ALIGN_(8, mbw*MB_GN_DATA);
    //size_dqm = _ALIGN_(8, (mbn+1)/2+16);
    size_dqm = _ALIGN_(8, (mbn*2));
    size_mbs = size_mbp + size_dqm;

    size_dump_reg = 256;    // Fixme, 25x64bit on I2 MFE

    size_pmbr_map   = _ALIGN_(8, sizeof(int)*mbn);
    size_pmbr_lut   = _ALIGN_(8, 16 * PMBR_LUT_SIZE);
    size_pmbr       = size_pmbr_lut + (size_pmbr_map * 2);

    size_lum = _ALIGN_(8, mbn*MBPIXELS_Y);
    size_chr = _ALIGN_(8, mbn*MBPIXELS_C);
    size_out = _ALIGN_(12, size_lum);
    if (mctx->i_omode == MMFE_OMODE_MMAP)
        size_out = 0;

    rpbn = mcfg.res.i_rpbn;

    /* Calculate total size */
    size_refrec = 0;
    if (mctx->i_imode == MMFE_IMODE_PURE)
    {
        size_refrec =  _ALIGN_(12,size_lum+size_chr)*rpbn;
    }

    size_total_al = size_out + size_mbs + size_pmbr + size_dump_reg;
    size_total_ref = size_refrec;


    pstParam->u32IntrAlBufSize = size_total_al;
    pstParam->u32IntrRefBufSize = size_total_ref;

    //CamOsPrintf("[%s %d] size_total: %d, %d\n", __func__,__LINE__, pstParam->IntrAlBufSize, pstParam->IntrRefBufSize);

    return 0;
}

MS_S32 MHAL_MFE_IsrProc(MHAL_VENC_DEV_HANDLE hDev)
{
    mmfe_dev *mdev = (mmfe_dev *)hDev;
    CamOsTimespec_t timer;
    MS_S32 ret;

    if (mdev)
    {
        ret = MfeDevIsrFnx(mdev);

        CamOsGetMonotonicTime(&timer);
        mdev->encode_total_ns0 += _MfeTimespecDiffNs(&mdev->encode_start_time, &timer);
        return ret;
    }

    return 0;
}
