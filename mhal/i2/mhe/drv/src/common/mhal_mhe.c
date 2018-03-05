/*
 * mhal_mhe.c
 *
 *  Created on: Aug 14, 2017
 *      Author: derek.lee
 */


#include "drv_mhe_kernel.h"
#include "mdrv_mhe_io.h"
#include "mdrv_mhe_st.h"
#include "mdrv_rqct_st.h"
#include "mdrv_rqct_io.h"
#include "drv_mhe_ctx.h"
#include "drv_mhe_dev.h"
#include "hal_mhe_api.h"
#include "mhal_mhe.h"
#include "mhal_venc.h"
#include "mhal_ut_wrapper.h"
#include "mhve_pmbr_cfg.h"
#ifdef SUPPORT_CMDQ_SERVICE
#include "mhal_cmdq.h"
#endif

#define FPGA_REG_DEBUG 0

#define CBR_UPPER_QP        51  //48
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


#if FPGA_REG_DEBUG
// HW core 0
#define RIU_BANK0_ADDR 0x171d00     // "HEV" sheet
#define RIU_MARB_ADDR  0x171e00
#define RIU_BANK1_ADDR 0x171f00     // "HEV1" sheet
#define RIU_BANK2_ADDR 0x170900     // "HEV2" sheet
// HW core 1
#define RIU_BANK0_ADDR_CORE1 0x173c00   // "HEV" sheet
#define RIU_MARB_ADDR_CORE1  0x173d00
#define RIU_BANK1_ADDR_CORE1 0x173e00   // "HEV1" sheet
#define RIU_BANK2_ADDR_CORE1 0x173f00   // "HEV2" sheet
#endif

void _MheSetHeader(VOID* header, MS_U32 size)
{
    MHAL_VENC_Version_t* ver = (MHAL_VENC_Version_t*)header;
    ver->u32Size = size;

    ver->s.u8VersionMajor = SPECVERSIONMAJOR;
    ver->s.u8VersionMinor = SPECVERSIONMINOR;
    ver->s.u8Revision = SPECREVISION;
    ver->s.u8Step = SPECSTEP;
}

MHAL_ErrCode_e _MheCheckHeader(VOID* pHeader, MS_U32 u32Size)
{
    MHAL_ErrCode_e eError = 0;

    MHAL_VENC_Version_t* ver;

    if(NULL == pHeader)
    {
        CamOsPrintf("In %s the header is null\n", __func__);
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    ver = (MHAL_VENC_Version_t*)pHeader;

    if(ver->u32Size != u32Size)
    {
        CamOsPrintf("In %s the header has a wrong size %i should be %i\n", __func__, ver->u32Size, u32Size);
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    if(ver->s.u8VersionMajor != SPECVERSIONMAJOR ||
            ver->s.u8VersionMinor != SPECVERSIONMINOR)
    {
        CamOsPrintf("The version does not match\n");
        return E_MHAL_ERR_ILLEGAL_PARAM;
    }

    return eError;
}

int _MheGenSeiNalu(char* UserData, int Len, int Idx, char* SeiData)
{
    int i=0;
    int data_len = Len;
    int total_len = data_len + 16;
    int ren_len = total_len;
    int offset=0;

    SeiData[0] = SeiData[1] = SeiData[2] = 0x00;
    SeiData[3] = 0x01;
    SeiData[4] = 0x4E;
    SeiData[5] = 0x01;
    SeiData[6] = 0x05; //user_data_unregistered type

    offset=7;
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

MS_S32 MHAL_MHE_CreateDevice(MS_U32 u32DevId, MHAL_VENC_DEV_HANDLE *phDev)
{
    mmhe_dev *mdev = NULL;
    mhve_ios* mios = NULL;
    mhve_reg mregs = {0};
    VOID* pBase[4];
    MS_S32 nSize[4];
    int i;

    *phDev = NULL;

    do
    {
        if(!(mdev = CamOsMemAlloc(sizeof(mmhe_dev))))
            break;

        CamOsMutexInit(&mdev->m_mutex);
        CamOsTsemInit(&mdev->tGetBitsSem, 1);
        CamOsTsemInit(&mdev->m_wqh, MMHE_DEV_STATE_IDLE);

        if(!(mdev->p_asicip = MheIosAcquire("mhe")))
            break;
        mios = mdev->p_asicip;

        MheDevGetResourceMem((u32)u32DevId, pBase, nSize);

#if FPGA_REG_DEBUG
        pBase[0] = (void *)RIU_BANK0_ADDR;
        pBase[1] = (void *)RIU_MARB_ADDR;
        pBase[2] = (void *)RIU_BANK1_ADDR;
        pBase[3] = (void *)RIU_BANK2_ADDR;
#endif

        mregs.i_id = 0;
        for(i = 0; i < MHVEREG_MAXBASE_NUM; i++)
        {
            mregs.base[i] = (VOID *)IO_ADDRESS(pBase[i]);
            mregs.size[i] = nSize[i];
            //CamOsPrintf( "[%s %d] MHE REG Bank%d = 0x%p, size = %d\n", __FUNCTION__, __LINE__, i, mregs.base[i] , mregs.size[i]);
        }
        mios->set_bank(mios, &mregs);
        mdev->p_reg_base[0] = pBase[0];
        mdev->p_reg_base[1] = pBase[2];

        if(0 != MheDevClkInit(mdev, u32DevId))
        {
            CamOsPrintf("MHAL_MHE_CreateDevice clock init fail\n");
            //break;    // fixme: not break for FPGA easy test
        }

        if(0 != MheDevGetResourceIrq((u32)u32DevId, &mdev->i_irq))
            break;

        mdev->i_rctidx = 1; //rate control index

        *phDev = (MHAL_VENC_DEV_HANDLE)mdev;

        return 0;
    }
    while(0);

    if(mdev)
    {
        mhve_ios* mios = mdev->p_asicip;
        if(mios)
            mios->release(mios);
        CamOsMemRelease(mdev);
    }

    return E_MHAL_ERR_ILLEGAL_PARAM;
}

MS_S32 MHAL_MHE_DestroyDevice(MHAL_VENC_DEV_HANDLE hDev)
{
    MHAL_ErrCode_e eError = 0;
    mmhe_dev *mdev = (mmhe_dev *)hDev;
    mhve_ios* mios = NULL;

    if(mdev)
    {
        mios = mdev->p_asicip;

        clk_put(mdev->p_clocks[0]);
        if(mios)
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

MS_S32 MHAL_MHE_CreateInstance(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_INST_HANDLE *phInst)
{
    mmhe_dev* mdev = (mmhe_dev *)hDev;
    mmhe_ctx* mctx;

    if(!mdev)
        return E_MHAL_ERR_INVALID_DEVID;

    if(!(mctx = MheCtxAcquire(mdev)))
    {
        return E_MHAL_ERR_BUSY;
    }

    if(0 <= MheDevRegister(mdev, mctx))
    {
        *phInst = mctx;

        return 0;
    }

    mctx->release(mctx);

    MheCtxActions(mctx, IOCTL_MMHE_STREAMOFF, NULL);

    return E_MHAL_ERR_ILLEGAL_PARAM;
}

MS_S32 MHAL_MHE_DestroyInstance(MHAL_VENC_INST_HANDLE hInst)
{
    MHAL_ErrCode_e eError = 0;
    mmhe_ctx* mctx = (mmhe_ctx*)hInst;

    if(mctx)
    {
        MheCtxActions(mctx, IOCTL_MMHE_STREAMOFF, NULL);
        MheDevUnregister(mctx->p_device, mctx);
        mctx->release(mctx);
        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }

    return eError;
}

MS_S32 MHAL_MHE_SetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    MHAL_ErrCode_e eError = 0;
    mmhe_ctx* mctx = (mmhe_ctx*)hInst;

    mmhe_parm mheparam;
    mmhe_ctrl vctrl;
    rqct_conf rqcnf;

    memset((void *)&mheparam, 0, sizeof(mheparam));
    memset((void *)&vctrl, 0, sizeof(vctrl));
    memset((void *)&rqcnf, 0, sizeof(rqcnf));

    if(NULL == pstParam)
    {
        eError = E_MHAL_ERR_NULL_PTR;
        CamOsPrintf("In %s parameter provided is null! err = %x\n", __func__, eError);
        return eError;
    }

    if(NULL == mctx)
    {
        eError = E_MHAL_ERR_NULL_PTR;
        CamOsPrintf("In %s parameter mctx is null! err = %x\n", __func__, eError);
        return eError;
    }

    switch(eType)
    {
        case E_MHAL_VENC_IDX_STREAM_ON:
        {

            MHAL_VENC_InternalBuf_t *pVEncBuf = (MHAL_VENC_InternalBuf_t *)pstParam;

            if (pstParam)
            {
                if ((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_InternalBuf_t))))
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

            MheCtxActions(mctx, IOCTL_MMHE_STREAMON, NULL);

            /*            CamOsPrintf("mctx->i_ialsz = %d, mctx->p_ialva = %p, mctx->p_ialma = %p, mctx->p_ircma = %p, mctx->i_ircms = %d\n",
                        mctx->i_ialsz,mctx->p_ialva,mctx->p_ialma, mctx->p_ircma, mctx->i_ircms);*/

            break;
        }
        case E_MHAL_VENC_IDX_STREAM_OFF:
        {
            MheCtxActions(mctx, IOCTL_MMHE_STREAMOFF, NULL);
            break;
        }
        //Cfg setting
        case E_MHAL_VENC_265_RESOLUTION:
        {

            MHAL_VENC_Resoluton_t *pVEncRes = (MHAL_VENC_Resoluton_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_Resoluton_t))))
            {
                break;
            }

            // resolution
            mheparam.type = MMHE_PARM_RES;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            mheparam.res.i_pict_w = pVEncRes->u32Width;
            mheparam.res.i_pict_h = pVEncRes->u32Height;
            mheparam.res.i_cropen = 0;
            mheparam.res.i_crop_offset_x = 0;
            mheparam.res.i_crop_offset_y = 0;
            mheparam.res.i_crop_w = 0;
            mheparam.res.i_crop_h = 0;
            mheparam.res.i_pixfmt = pVEncRes->eFmt;
            mheparam.res.i_outlen = -1;
            mheparam.res.i_flags = 0;
            //mheparam.res.i_ClkEn = 0; //pParam->sClock.nClkEn;     // Giggs Check
            //mheparam.res.i_ClkSor = 0; //pParam->sClock.nClkSor;   // Giggs Check
            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            break;
        }
        case E_MHAL_VENC_265_CROP:
        {

            MHAL_VENC_CropCfg_t* pVEncCrop = (MHAL_VENC_CropCfg_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_CropCfg_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_RES;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
            if((mheparam.res.i_pict_w == 0) || (mheparam.res.i_pict_h == 0))
            {
                eError = E_MHAL_ERR_ILLEGAL_PARAM;
                break;
            }

            mheparam.res.i_cropen =   pVEncCrop->bEnable;
            mheparam.res.i_crop_offset_x = pVEncCrop->stRect.u32X;
            mheparam.res.i_crop_offset_y = pVEncCrop->stRect.u32Y;
            mheparam.res.i_crop_w = pVEncCrop->stRect.u32W;
            mheparam.res.i_crop_h = pVEncCrop->stRect.u32H;
            mheparam.res.i_outlen = -1;    // rewrite again
            mheparam.res.i_flags = 0;

            if((mheparam.res.i_crop_w == 0) || (mheparam.res.i_crop_h == 0))
            {
                eError = E_MHAL_ERR_ILLEGAL_PARAM;
                break;
            }

            if(((mheparam.res.i_crop_offset_x + mheparam.res.i_crop_w) > mheparam.res.i_pict_w ) ||
                 (( mheparam.res.i_crop_offset_y +  mheparam.res.i_crop_h) > mheparam.res.i_pict_h))
            {
                eError = E_MHAL_ERR_ILLEGAL_PARAM;
                break;
            }

            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);
            break;
        }
        case E_MHAL_VENC_265_REF:
        {
            MHAL_VENC_ParamRef_t* pVEncRef = (MHAL_VENC_ParamRef_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamRef_t))))
            {
                break;
            }

            pVEncRef = pVEncRef;

            //TODO

            break;
        }
        case E_MHAL_VENC_265_VUI:
        {
            MHAL_VENC_ParamH265Vui_t* pVEncVui = (MHAL_VENC_ParamH265Vui_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Vui_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_VUI;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            mheparam.vui.i_sar_w = pVEncVui->stVuiAspectRatio.u16SarWidth;
            mheparam.vui.i_sar_h = pVEncVui->stVuiAspectRatio.u16SarHeight;

            mheparam.vui.b_timing_info_pres = pVEncVui->stVuiTimeInfo.u8TimingInfoPresentFlag;

            mheparam.vui.b_video_full_range = pVEncVui->stVuiVideoSignal.u8VideoFullRangeFlag;
            mheparam.vui.b_video_signal_pres = pVEncVui->stVuiVideoSignal.u8VideoSignalTypePresentFlag;
            mheparam.vui.i_video_format = pVEncVui->stVuiVideoSignal.u8VideoFormat;
            mheparam.vui.b_colour_desc_pres = pVEncVui->stVuiVideoSignal.u8ColourDescriptionPresentFlag;

            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            break;
        }
        case E_MHAL_VENC_265_DBLK:
        {
            MHAL_VENC_ParamH265Dblk_t* pVEncDblk = (MHAL_VENC_ParamH265Dblk_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Dblk_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            //disable_deblocking_filter_idc
            //0: enable deblocking and enable cross slice boundary
            //1: disable deblocking
            //2: enable deblocking and disable cross slice boundary
            if(pVEncDblk->disable_deblocking_filter_idc == 0)
            {
                mheparam.hevc.b_deblocking_disable = 0;
                mheparam.hevc.b_deblocking_cross_slice_enable = 1;
            }
            else if(pVEncDblk->disable_deblocking_filter_idc == 1)
            {
                mheparam.hevc.b_deblocking_disable = 1;
                mheparam.hevc.b_deblocking_cross_slice_enable = 0;
            }
            else if(pVEncDblk->disable_deblocking_filter_idc == 2)
            {
                mheparam.hevc.b_deblocking_disable = 0;
                mheparam.hevc.b_deblocking_cross_slice_enable = 0;
            }
            else
            {
                //not support mode
                return E_MHAL_ERR_ILLEGAL_PARAM;
            }

            mheparam.hevc.i_tc_offset_div2 = pVEncDblk->slice_tc_offset_div2;
            mheparam.hevc.i_beta_offset_div2 = pVEncDblk->slice_beta_offset_div2;

            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            break;
        }
        case E_MHAL_VENC_265_ENTROPY:
        {

            //H265 only support CABLC

            break;
        }
        case E_MHAL_VENC_265_TRANS:
        {
            MHAL_VENC_ParamH265Trans_t* pVEncTrans = (MHAL_VENC_ParamH265Trans_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Trans_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            mheparam.hevc.i_cqp_offset = pVEncTrans->s32ChromaQpIndexOffset;

            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            //pVEncTrans->u32InterTransMode
            //pVEncTrans->u32IntraTransMode

            break;
        }
        case E_MHAL_VENC_265_INTRA_PRED:
        {
            MHAL_VENC_ParamH265IntraPred_t* pVEncIntraPred = (MHAL_VENC_ParamH265IntraPred_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265IntraPred_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            //Intra prediction
            mheparam.hevc.b_constrained_intra_pred = pVEncIntraPred->constrained_intra_pred_flag;
            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            rqcnf.type = RQCT_CONF_PEN;
            MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);

//            rqcnf.pen.b_ia32lose = pVEncIntraPred->bIntra32x32PredEn? 0 : 1;
//            rqcnf.pen.b_ia16lose = pVEncIntraPred->bIntra16x16PredEn? 0 : 1;
//            rqcnf.pen.b_ia8xlose = pVEncIntraPred->bIntra8x8PredEn? 0 : 1;

            rqcnf.pen.u_ia32pen =  pVEncIntraPred->u32Intra32x32Penalty;
            rqcnf.pen.u_ia16pen =  pVEncIntraPred->u32Intra16x16Penalty;
            rqcnf.pen.u_ia8xpen =  pVEncIntraPred->u32Intra8x8Penalty;

            //pVEncIntraPred->bIpcmEn

            MheCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);

            break;
        }
        case E_MHAL_VENC_265_INTER_PRED:
        {
            MHAL_VENC_ParamH265InterPred_t* pVEncInterPred = (MHAL_VENC_ParamH265InterPred_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265InterPred_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_MOT;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            mheparam.mot.i_dmv_x = pVEncInterPred->u32HWSize;
            mheparam.mot.i_dmv_y = pVEncInterPred->u32VWSize;
            mheparam.mot.i_subpel = MMHE_SUBPEL_QUATER;

            if(pVEncInterPred->bInter8x8PredEn)
            {
                mheparam.mot.i_mvblks[0] |= MMHE_MVBLK_8x8;
            }
            else
            {
                mheparam.mot.i_mvblks[0] &= ~MMHE_MVBLK_8x8;
            }

            if(pVEncInterPred->bInter8x16PredEn)
            {
                mheparam.mot.i_mvblks[0] |= MMHE_MVBLK_8x16;
            }
            else
            {
                mheparam.mot.i_mvblks[0] &= ~MMHE_MVBLK_8x16;
            }

            if(pVEncInterPred->bInter16x8PredEn)
            {
                mheparam.mot.i_mvblks[0] |= MMHE_MVBLK_16x8;
            }
            else
            {
                mheparam.mot.i_mvblks[0] &= ~MMHE_MVBLK_16x8;
            }

            if(pVEncInterPred->bInter16x16PredEn)
            {
                mheparam.mot.i_mvblks[0] |= MMHE_MVBLK_16x16;
            }
            else
            {
                mheparam.mot.i_mvblks[0] &= ~MMHE_MVBLK_16x16;
            }


            MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

            break;
        }
        case E_MHAL_VENC_265_I_SPLIT_CTL:
        {
            MHAL_VENC_ParamSplit_t* pVEncSplit = (MHAL_VENC_ParamSplit_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamSplit_t))))
            {
                break;
            }

            vctrl.type = MMHE_CTRL_SPL;
            MheCtxActions(mctx, IOCTL_MMHE_G_CTRL, &vctrl);

            if(pVEncSplit->bSplitEnable)
            {
                vctrl.spl.i_rows =  pVEncSplit->u32SliceRowCount;
                vctrl.spl.i_bits = 0;   //derek check
            }

            MheCtxActions(mctx, IOCTL_MMHE_S_CTRL, &vctrl);

            break;
        }
        case E_MHAL_VENC_265_ROI:
        {
            MHAL_VENC_RoiCfg_t* pVEncRoiCfg = (MHAL_VENC_RoiCfg_t *)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RoiCfg_t))))
            {
                break;
            }

            vctrl.type = MMHE_CTRL_ROI;
            vctrl.roi.i_index = pVEncRoiCfg->u32Index;
            MheCtxActions(mctx, IOCTL_MMHE_G_CTRL, (VOID *)&vctrl);

            if(pVEncRoiCfg->bEnable)
            {
                vctrl.roi.i_absqp = pVEncRoiCfg->bAbsQp;
                vctrl.roi.i_roiqp = pVEncRoiCfg->s32Qp;
                vctrl.roi.i_cbx = (pVEncRoiCfg->stRect.u32X + 31) / 32;
                vctrl.roi.i_cby = (pVEncRoiCfg->stRect.u32Y + 31) / 32;
                vctrl.roi.i_cbw = (pVEncRoiCfg->stRect.u32W + 31) / 32;
                vctrl.roi.i_cbh = (pVEncRoiCfg->stRect.u32H + 31) / 32;
            }
            else
            {
                vctrl.roi.i_absqp = 0;
                vctrl.roi.i_roiqp = 0;
                vctrl.roi.i_cbx = 0;
                vctrl.roi.i_cby = 0;
                vctrl.roi.i_cbw = 0;
                vctrl.roi.i_cbh = 0;
            }

            MheCtxActions(mctx, IOCTL_MMHE_S_CTRL, (VOID *)&vctrl);

            break;
        }
        case E_MHAL_VENC_265_RC:
        {
            MHAL_VENC_RcInfo_t* pVEncRcInfo = (MHAL_VENC_RcInfo_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RcInfo_t))))
            {
                break;
            }


            if(E_MHAL_VENC_RC_MODE_H265CBR == pVEncRcInfo->eRcMode)
            {
                // Set BPS
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.bps.i_method = RQCT_METHOD_CBR;
                mheparam.bps.i_ref_qp = -1;        // Giggs Check
                mheparam.bps.i_bps = pVEncRcInfo->stAttrH265Cbr.u32BitRate;
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set FPS
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.fps.i_num = pVEncRcInfo->stAttrH265Cbr.u32SrcFrmRate;
                mheparam.fps.i_den = 1;
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set GOP
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.gop.i_pframes = pVEncRcInfo->stAttrH265Cbr.u32Gop;
                mheparam.gop.i_bframes = 0;
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set RQCT
                rqcnf.type = RQCT_CONF_QPR;
                MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                rqcnf.type = RQCT_CONF_QPR;
                rqcnf.qpr.i_iupperq = CBR_UPPER_QP;
                rqcnf.qpr.i_ilowerq = CBR_LOWER_QP;
                rqcnf.qpr.i_pupperq = CBR_UPPER_QP;
                rqcnf.qpr.i_plowerq = CBR_LOWER_QP;
                rqcnf.type = RQCT_CONF_QPR;
                MheCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
            }
            else if(E_MHAL_VENC_RC_MODE_H265VBR == pVEncRcInfo->eRcMode)
            {
                // Set BPS
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.bps.i_method = RQCT_METHOD_VBR;
                mheparam.bps.i_ref_qp = -1;
                mheparam.bps.i_bps = pVEncRcInfo->stAttrH265Vbr.u32MaxBitRate;
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set FPS
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.fps.i_num = pVEncRcInfo->stAttrH265Vbr.u32SrcFrmRate;
                mheparam.fps.i_den = 1;
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set GOP
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.gop.i_pframes = pVEncRcInfo->stAttrH265Vbr.u32Gop;
                mheparam.gop.i_bframes = 0;
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set RQCT
                rqcnf.type = RQCT_CONF_QPR;
                MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                rqcnf.type = RQCT_CONF_QPR;
                rqcnf.qpr.i_iupperq = pVEncRcInfo->stAttrH265Vbr.u32MaxQp;
                rqcnf.qpr.i_ilowerq = pVEncRcInfo->stAttrH265Vbr.u32MinQp;
                rqcnf.qpr.i_pupperq = pVEncRcInfo->stAttrH265Vbr.u32MaxQp;
                rqcnf.qpr.i_plowerq = pVEncRcInfo->stAttrH265Vbr.u32MinQp;
                rqcnf.type = RQCT_CONF_QPR;
                MheCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
            }
            else if(E_MHAL_VENC_RC_MODE_H265FIXQP == pVEncRcInfo->eRcMode)
            {
                // Set BPS
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.bps.i_method = RQCT_METHOD_CQP;
                mheparam.bps.i_ref_qp = pVEncRcInfo->stAttrH265FixQp.u32IQp;        // Giggs Check
                mheparam.bps.i_bps = 0;
                mheparam.type = MMHE_PARM_BPS ;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set FPS
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.fps.i_num = pVEncRcInfo->stAttrH265FixQp.u32SrcFrmRate;
                mheparam.fps.i_den = 1;
                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                // Set GOP
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.gop.i_pframes = pVEncRcInfo->stAttrH265FixQp.u32Gop;
                mheparam.gop.i_bframes = 0;
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);


                //rqcnf.type = RQCT_CONF_QPR;
                //MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                //rqcnf.type = RQCT_CONF_QPR;
                //MheCtxActions(mctx, IOCTL_RQCT_S_CONF, (VOID *)&rqcnf);
            }
            else
            {
                eError = E_MHAL_ERR_ILLEGAL_PARAM;
                break;
            }

            break;
        }
        case E_MHAL_VENC_265_USER_DATA:
            {
                MHAL_VENC_UserData_t* pVEncUserData = (MHAL_VENC_UserData_t *)pstParam;
                char SeiData[MMHE_SEI_MAX_LEN];
                int SeiLen = 0;
                mmhe_buff ibuff = {0};

                if ((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_UserData_t))))
                {
                    break;
                }

                if (mctx->i_usrcn >= 4 || pVEncUserData->u32Len > 1024)
                {
                    eError = E_MHAL_ERR_ILLEGAL_PARAM;
                    break;
                }

                SeiLen = _MheGenSeiNalu(pVEncUserData->pu8Data, pVEncUserData->u32Len, mctx->i_usrcn, SeiData);

                ibuff.i_memory = MMHE_MEMORY_USER;
                ibuff.i_planes = 1;
                ibuff.planes[0].i_size = MMHE_SEI_MAX_LEN;
                ibuff.planes[0].mem.uptr = SeiData;
                ibuff.planes[0].i_used = SeiLen;
                MheCtxActions(mctx, IOCTL_MMHE_S_DATA, (VOID *)&ibuff);

                break;
            }
        case E_MHAL_VENC_ENABLE_IDR:
            {
                MHAL_VENC_EnableIdr_t* pVEncEnableIdr = (MHAL_VENC_EnableIdr_t *)pstParam;

                if ((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_EnableIdr_t))))
                {
                    break;
                }

                // Set GOP
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                mheparam.gop.b_passiveI = ((pVEncEnableIdr->bEnable==0)?1:0);
                //CamOsPrintf("%s:%d pVEncEnableIdr->bEnable %d  b_passiveI %d\n", __FUNCTION__, __LINE__, pVEncEnableIdr->bEnable, mheparam.gop.b_passiveI);
                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_S_PARM, (VOID *)&mheparam);

                break;
            }
        case E_MHAL_VENC_265_FRAME_LOST:
        {
            MHAL_VENC_ParamFrameLost_t* pVencParamFrameLost = (MHAL_VENC_ParamFrameLost_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamFrameLost_t))))
            {
                break;
            }

            pVencParamFrameLost = pVencParamFrameLost;

            break;
        }
        case E_MHAL_VENC_265_FRAME_CFG:
        {
            MHAL_VENC_SuperFrameCfg_t* pVencSuperFrameCfg = (MHAL_VENC_SuperFrameCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_SuperFrameCfg_t))))
            {
                break;
            }

            pVencSuperFrameCfg = pVencSuperFrameCfg;

            break;
        }
        case E_MHAL_VENC_265_RC_PRIORITY:
        {
            MHAL_VENC_RcPriorityCfg_t* pVencRcPriorityCfg = (MHAL_VENC_RcPriorityCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RcPriorityCfg_t))))
            {
                break;
            }

            pVencRcPriorityCfg = pVencRcPriorityCfg;

            break;
        }
        default:
            CamOsPrintf("In %s command is not support! err = %x\n", __func__, eError);
            eError = E_MHAL_ERR_NOT_SUPPORT;
            break;

    }

    return eError;
}

MS_S32 MHAL_MHE_GetParam(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    MHAL_ErrCode_e eError;
    mmhe_ctx* mctx = (mmhe_ctx*)hInst;

    mmhe_parm mheparam;
    mmhe_ctrl vctrl;
    rqct_conf rqcnf;

    memset((void *)&mheparam, 0 , sizeof(mmhe_parm));
    memset((void *)&vctrl, 0, sizeof(mmhe_ctrl));
    memset((void *)&rqcnf, 0, sizeof(rqct_conf));

    if(mctx == NULL)
        return E_MHAL_ERR_NULL_PTR;
    if(pstParam == NULL)
        return E_MHAL_ERR_NULL_PTR;

    switch(eType)
    {

            //Cfg setting
        case E_MHAL_VENC_265_RESOLUTION:
        {
            MHAL_VENC_Resoluton_t* pVEncRes = (MHAL_VENC_Resoluton_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_Resoluton_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_RES;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncRes->u32Width = mheparam.res.i_pict_w;
            pVEncRes->u32Height = mheparam.res.i_pict_h;
            pVEncRes->eFmt = mheparam.res.i_pixfmt;

            break;
        }
        case E_MHAL_VENC_265_CROP:
        {
            MHAL_VENC_CropCfg_t* pVEncCropCfg = (MHAL_VENC_CropCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_CropCfg_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_RES;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncCropCfg->bEnable = mheparam.res.i_cropen;
            pVEncCropCfg->stRect.u32X = mheparam.res.i_crop_offset_x;
            pVEncCropCfg->stRect.u32Y = mheparam.res.i_crop_offset_y;
            pVEncCropCfg->stRect.u32W = mheparam.res.i_crop_w;
            pVEncCropCfg->stRect.u32H = mheparam.res.i_crop_h;
            break;
        }
        case E_MHAL_VENC_265_REF:
        {
            MHAL_VENC_ParamRef_t* pVEncRef = (MHAL_VENC_ParamRef_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamRef_t))))
            {
                break;
            }

            pVEncRef->u32RefLayerMode = 0;

            break;
        }
        case E_MHAL_VENC_265_VUI:
        {
            MHAL_VENC_ParamH265Vui_t* pVEncVui = (MHAL_VENC_ParamH265Vui_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Vui_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_VUI;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncVui->stVuiAspectRatio.u16SarWidth = mheparam.vui.i_sar_w;
            pVEncVui->stVuiAspectRatio.u16SarHeight = mheparam.vui.i_sar_h;

            pVEncVui->stVuiTimeInfo.u8TimingInfoPresentFlag = mheparam.vui.b_timing_info_pres;

            pVEncVui->stVuiVideoSignal.u8VideoFullRangeFlag = mheparam.vui.b_video_full_range;
            pVEncVui->stVuiVideoSignal.u8VideoSignalTypePresentFlag = mheparam.vui.b_video_signal_pres;
            pVEncVui->stVuiVideoSignal.u8VideoFormat = mheparam.vui.i_video_format;
            pVEncVui->stVuiVideoSignal.u8ColourDescriptionPresentFlag = mheparam.vui.b_colour_desc_pres;

            break;
        }
        case E_MHAL_VENC_265_DBLK:
        {
            MHAL_VENC_ParamH265Dblk_t* pVEncDblk = (MHAL_VENC_ParamH265Dblk_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Dblk_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            if(mheparam.hevc.b_deblocking_disable == 0 && mheparam.hevc.b_deblocking_cross_slice_enable == 1)
            {
                pVEncDblk->disable_deblocking_filter_idc = 0;
            }
            else if(mheparam.hevc.b_deblocking_disable == 1 && mheparam.hevc.b_deblocking_cross_slice_enable == 0)
            {
                pVEncDblk->disable_deblocking_filter_idc = 1;
            }
            else if(mheparam.hevc.b_deblocking_disable == 0 && mheparam.hevc.b_deblocking_cross_slice_enable == 0)
            {
                pVEncDblk->disable_deblocking_filter_idc = 2;
            }
            else
            {
                eError = -1;
                break;
            }

            pVEncDblk->slice_tc_offset_div2 = mheparam.hevc.i_tc_offset_div2;
            pVEncDblk->slice_beta_offset_div2 = mheparam.hevc.i_beta_offset_div2;

            break;
        }
        case E_MHAL_VENC_265_ENTROPY:
        {
            //H65 only support CABLV

            break;
        }
        case E_MHAL_VENC_265_TRANS:
        {
            MHAL_VENC_ParamH265Trans_t* pVEncTrans = (MHAL_VENC_ParamH265Trans_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265Trans_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncTrans->s32ChromaQpIndexOffset = mheparam.hevc.i_cqp_offset;
            pVEncTrans->u32IntraTransMode = 0;
            pVEncTrans->u32InterTransMode = 0;

            break;
        }
        case E_MHAL_VENC_265_INTRA_PRED:
        {
            MHAL_VENC_ParamH265IntraPred_t* pVEncIntraPred = (MHAL_VENC_ParamH265IntraPred_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265IntraPred_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_HEVC;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncIntraPred->constrained_intra_pred_flag = mheparam.avc.b_constrained_intra_pred;

            rqcnf.type = RQCT_CONF_PEN;
            MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);

//            pVEncIntraPred->bIntra32x32PredEn = rqcnf.pen.b_ia32lose? 0 : 1;
//            pVEncIntraPred->bIntra16x16PredEn = rqcnf.pen.b_ia16lose? 0 : 1;
//            pVEncIntraPred->bIntra8x8PredEn = rqcnf.pen.b_ia8xlose? 0 : 1;

            pVEncIntraPred->u32Intra32x32Penalty = rqcnf.pen.u_ia32pen;
            pVEncIntraPred->u32Intra16x16Penalty = rqcnf.pen.u_ia16pen;
            pVEncIntraPred->u32Intra8x8Penalty = rqcnf.pen.u_ia8xpen;

//            pVEncIntraPred->bIpcmEn = 1;    //derek check

            break;
        }
        case E_MHAL_VENC_265_INTER_PRED:
        {
            MHAL_VENC_ParamH265InterPred_t* pVEncInterPred = (MHAL_VENC_ParamH265InterPred_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamH265InterPred_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_MOT;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            pVEncInterPred->u32HWSize = mheparam.mot.i_dmv_x;
            pVEncInterPred->u32VWSize = mheparam.mot.i_dmv_y;

            pVEncInterPred->bInter8x8PredEn = (mheparam.mot.i_mvblks[0] | MMHE_MVBLK_8x8)? 1 : 0;
            pVEncInterPred->bInter8x16PredEn = (mheparam.mot.i_mvblks[0] | MMHE_MVBLK_8x16)? 1 : 0;
            pVEncInterPred->bInter16x8PredEn = (mheparam.mot.i_mvblks[0] | MMHE_MVBLK_16x8)? 1 : 0;
            pVEncInterPred->bInter16x16PredEn = (mheparam.mot.i_mvblks[0] | MMHE_MVBLK_16x16)? 1 : 0;

            pVEncInterPred->bExtedgeEn = 1; //always over boundary

            break;
        }
        case E_MHAL_VENC_265_I_SPLIT_CTL:
        {
            MHAL_VENC_ParamSplit_t* pVEncSplit = (MHAL_VENC_ParamSplit_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamSplit_t))))
            {
                break;
            }

            vctrl.type = MMHE_CTRL_SPL;
            MheCtxActions(mctx, IOCTL_MMHE_G_CTRL, &vctrl);

            pVEncSplit->u32SliceRowCount = vctrl.spl.i_rows;

            if(pVEncSplit->u32SliceRowCount != 0)
                pVEncSplit->bSplitEnable = 1;
            else
                pVEncSplit->bSplitEnable = 0;

            break;
        }
        case E_MHAL_VENC_265_ROI:
        {
            MHAL_VENC_RoiCfg_t* pVEncRoiCfg = (MHAL_VENC_RoiCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RoiCfg_t))))
            {
                break;
            }

            vctrl.type = MMHE_CTRL_ROI;
            vctrl.roi.i_index = pVEncRoiCfg->u32Index;
            MheCtxActions(mctx, IOCTL_MMHE_G_CTRL, (VOID *)&vctrl);

            if(pVEncRoiCfg->s32Qp == 0)
                pVEncRoiCfg->bEnable = 0;
            else
                pVEncRoiCfg->bEnable = 1;

            pVEncRoiCfg->bAbsQp = vctrl.roi.i_absqp;
            pVEncRoiCfg->s32Qp = vctrl.roi.i_roiqp;
            pVEncRoiCfg->stRect.u32X = vctrl.roi.i_cbx << 5;
            pVEncRoiCfg->stRect.u32Y = vctrl.roi.i_cby << 5;
            pVEncRoiCfg->stRect.u32W = vctrl.roi.i_cbw << 5;
            pVEncRoiCfg->stRect.u32H = vctrl.roi.i_cbh << 5;
            pVEncRoiCfg->RoiBgCtl.s32SrcFrmRate = 0;
            pVEncRoiCfg->RoiBgCtl.s32DstFrmRate = 0;
            pVEncRoiCfg->pDaQpMap = NULL;

            break;
        }
        case E_MHAL_VENC_265_RC:
        {
            MHAL_VENC_RcInfo_t* pVEncRcInfo = (MHAL_VENC_RcInfo_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RcInfo_t))))
            {
                break;
            }

            mheparam.type = MMHE_PARM_BPS ;
            MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);

            if(RQCT_METHOD_CBR == mheparam.bps.i_method)
            {
                pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H265CBR;
                pVEncRcInfo->stAttrH265Cbr.u32BitRate = mheparam.bps.i_bps;

                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                //mheparam.fps.i_num = 1;
                pVEncRcInfo->stAttrH265Cbr.u32SrcFrmRate = mheparam.fps.i_den;

                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                pVEncRcInfo->stAttrH265Cbr.u32Gop = mheparam.gop.i_pframes;

                //pVEncRcInfo->stAttrH265Cbr.FluctuateLevel
                //pVEncRcInfo->stAttrH265Cbr.StatTime
            }
            else if(RQCT_METHOD_VBR == mheparam.bps.i_method)
            {
                pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H265VBR;
                pVEncRcInfo->stAttrH265Vbr.u32MaxBitRate = mheparam.bps.i_bps;

                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                //mheparam.fps.i_num = 1;
                pVEncRcInfo->stAttrH265Vbr.u32SrcFrmRate = mheparam.fps.i_den;

                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                pVEncRcInfo->stAttrH265Vbr.u32Gop = mheparam.gop.i_pframes;

                rqcnf.type = RQCT_CONF_QPR;
                MheCtxActions(mctx, IOCTL_RQCT_G_CONF, (VOID *)&rqcnf);
                pVEncRcInfo->stAttrH265Vbr.u32MaxQp = rqcnf.qpr.i_iupperq;
                pVEncRcInfo->stAttrH265Vbr.u32MinQp = rqcnf.qpr.i_ilowerq;
                pVEncRcInfo->stAttrH265Vbr.u32MaxQp = rqcnf.qpr.i_pupperq;
                pVEncRcInfo->stAttrH265Vbr.u32MinQp = rqcnf.qpr.i_plowerq;

            }
            else if(RQCT_METHOD_CQP == mheparam.bps.i_method)
            {
                pVEncRcInfo->eRcMode = E_MHAL_VENC_RC_MODE_H265FIXQP;
                pVEncRcInfo->stAttrH265FixQp.u32IQp = mheparam.bps.i_ref_qp;
                pVEncRcInfo->stAttrH265FixQp.u32PQp = mheparam.bps.i_ref_qp;

                mheparam.type = MMHE_PARM_FPS;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                //mheparam.fps.i_num = 1;
                pVEncRcInfo->stAttrH265FixQp.u32SrcFrmRate = mheparam.fps.i_den;

                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                pVEncRcInfo->stAttrH265FixQp.u32Gop = mheparam.gop.i_pframes;
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

                if ((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_EnableIdr_t))))
                {
                    break;
                }

                mheparam.type = MMHE_PARM_GOP;
                MheCtxActions(mctx, IOCTL_MMHE_G_PARM, (VOID *)&mheparam);
                pVEncEnableIdr->bEnable = (mheparam.gop.b_passiveI)?0:1;

                break;
            }
        case E_MHAL_VENC_265_FRAME_LOST:
        {
            MHAL_VENC_ParamFrameLost_t* pVEncFrameLost = (MHAL_VENC_ParamFrameLost_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamFrameLost_t))))
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
        case E_MHAL_VENC_265_FRAME_CFG:
        {
            MHAL_VENC_SuperFrameCfg_t* pVEncFrameCfg = (MHAL_VENC_SuperFrameCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_SuperFrameCfg_t))))
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
        case E_MHAL_VENC_265_RC_PRIORITY:
        {
            MHAL_VENC_RcPriorityCfg_t* pVEncRCPriority = (MHAL_VENC_RcPriorityCfg_t*)pstParam;

            if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_RcPriorityCfg_t))))
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

/*static int _busywait2(int count)
{
    volatile MS_U32 i = count;
    while (--i > 0) ;
    return 0;
}*/

u64 _MheTimespecDiffNs(CamOsTimespec_t *start, CamOsTimespec_t *stop)
{
    return (stop->nSec - start->nSec)*1000000000 + stop->nNanoSec - start->nNanoSec;
}

MS_S32 MHAL_MHE_EncodeOneFrame(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InOutBuf_t* pstInOutBuf)
{
    MHAL_ErrCode_e eError = 0;
    mmhe_ctx* mctx = (mmhe_ctx*)hInst;
    MS_U32    cmd;
    mmhe_parm param;
    mmhe_buff  vbuff[2];
    mmhe_buff* ibuff = NULL;
    mmhe_buff* obuff = NULL;
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

    if(mctx && pstInOutBuf)
    {
        CamOsGetMonotonicTime(&mctx->p_device->utilization_calc_end);
        if ((timer_ns_diff = _MheTimespecDiffNs(&mctx->p_device->utilization_calc_start, &mctx->p_device->utilization_calc_end)) > 1000000000)
        {
            CamOsGetMonotonicTime(&mctx->p_device->utilization_calc_start);
            //CamOsPrintf("MHE utilization %d  %d\n", (u32)(mctx->p_device->encode_total_ns0 * 100 / timer_ns_diff), (u32)(mctx->p_device->encode_total_ns1 * 100 / timer_ns_diff));
            mctx->p_device->encode_total_ns0 = 0;
            mctx->p_device->encode_total_ns1 = 0;
        }

        CamOsGetMonotonicTime(&mctx->p_device->encode_start_time);

        // get resolution
        cmd = IOCTL_MMHE_G_PARM;
        param.type = MMHE_PARM_RES;
        MheCtxActions(mctx, cmd, (VOID *)&param);

        ibuff = &vbuff[0];
        obuff = &vbuff[1];

        memset(vbuff, 0, sizeof(vbuff));
        ibuff->i_timecode = 0;
        ibuff->i_width = param.res.i_pict_w;
        ibuff->i_height = param.res.i_pict_h;
        ibuff->i_memory = MMHE_MEMORY_MMAP;
        switch(param.res.i_pixfmt)
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
            ibuff->i_flags |= MMHE_FLAGS_IDR;
        }
        ibuff->planes[0].i_used = ibuff->planes[0].i_size;
        ibuff->planes[1].i_used = ibuff->planes[1].i_size;

        obuff->i_memory = MMHE_MEMORY_MMAP;
        obuff->i_planes = 1;
        obuff->planes[0].mem.phys = pstInOutBuf->phyOutputBuf;
        obuff->planes[0].mem.uptr = (void *)(u32)pstInOutBuf->virtOutputBuf;
        obuff->planes[0].i_size = pstInOutBuf->u32OutputBufSize;
        obuff->planes[0].i_used = 0;
        obuff->i_flags = 0;

#ifdef SUPPORT_CMDQ_SERVICE
        if(pstInOutBuf->pCmdQ)
        {
            // Trigger by CmdQ
            u32RegCmdLen = sizeof(mctx->p_regcmd) / sizeof(MS_U32) / 2;
            MheCtxGenCompressRegCmd(mctx, vbuff, mctx->p_regcmd, &u32RegCmdLen);

            //CamOsPrintf("Gen u32RegCmdLen: %d\n", u32RegCmdLen);

            stCmdQInf = (MHAL_CMDQ_CmdqInterface_t *)pstInOutBuf->pCmdQ;
            while(!stCmdQInf->MHAL_CMDQ_CheckBufAvailable(stCmdQInf, u32RegCmdLen))
            {
                stCmdQInf->MHAL_CMDQ_ReadStatusCmdq(stCmdQInf, &u32CmdQStat);
                if((u32CmdQStat & MHAL_CMDQ_ERROR_STATUS) != 0)
                    stCmdQInf->MHAL_CMDQ_CmdqResetEngine(stCmdQInf);

                CamOsMsSleep(2);

                u32CmdQInfRetryCnt++;
                if(u32CmdQInfRetryCnt > 1000)
                    BUG();
            }

            for(i = 0; i < u32RegCmdLen; i++)
            {
                reg_addr = *(MS_U32*)(mctx->p_regcmd + i * 2);
                reg_value = *(MS_U32*)(mctx->p_regcmd + i * 2 + 1);

                //CamOsPrintf("WriteReg(no.%d)    0x%08X  0x%04X\n", i, reg_addr, reg_value);

                // Fill CmdQ here
                nCmdqRet = stCmdQInf->MHAL_CMDQ_WriteRegCmdq(stCmdQInf, (reg_addr & 0x00FFFFFF) >> 1, reg_value);
            }

            if (((u32)mctx->p_device->p_reg_base[0]&0x00FFFF00) == 0x002E3A00)     /*Fixme: it need some info(bank addr) from hal*/
            {
                stCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(stCmdQInf, E_MHAL_CMDQEVE_CORE0_MHE_TRIG);
            }
            else if (((u32)mctx->p_device->p_reg_base[0]&0x00FFFF00) == 0x002E7800)
            {
                stCmdQInf->MHAL_CMDQ_CmdqAddWaitEventCmd(stCmdQInf, E_MHAL_CMDQEVE_CORE1_MHE_TRIG);
            }
        }
        else
#endif
        {
            if(MheCtxEncFireAndReturn(mctx, vbuff) != 0)
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

MS_S32 MHAL_MHE_EncodeFrameDone(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_EncResult_t* pstEncRet)
{
    MHAL_ErrCode_e eError = 0;
    mmhe_ctx* mctx = (mmhe_ctx*)hInst;
    MS_U32    cmd;
    mmhe_parm param;
    mmhe_buff  vbuff[2];
    mmhe_buff* ibuff = NULL;
    mmhe_buff* obuff = NULL;
    CamOsTimespec_t timer;

    if(mctx && pstEncRet)
    {
        // get resolution
        cmd = IOCTL_MMHE_G_PARM;
        param.type = MMHE_PARM_RES;
        MheCtxActions(mctx, cmd, (VOID *)&param);

        ibuff = &vbuff[0];
        obuff = &vbuff[1];

        memset(&vbuff, 0, sizeof(vbuff));
        ibuff->i_timecode = 0;
        ibuff->i_width = param.res.i_pict_w;
        ibuff->i_height = param.res.i_pict_h;
        ibuff->i_memory = MMHE_MEMORY_MMAP;
        switch(param.res.i_pixfmt)
        {
            case MMHE_PIXFMT_NV12:
            case MMHE_PIXFMT_NV21:
                ibuff->i_planes = 2;
                //ibuff->planes[0].mem.phys = (unsigned long long)(unsigned int)pstInOutBuf->InputYUVBuf;
                ibuff->planes[1].mem.phys = ibuff->planes[0].mem.phys + ibuff->i_width * ibuff->i_height;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height;
                ibuff->planes[1].i_size = ibuff->i_width * ibuff->i_height / 2;
                break;
            case MMHE_PIXFMT_YUYV:
                ibuff->i_planes = 1;
                //ibuff->planes[0].mem.phys = (unsigned long long)(unsigned int)pstInOutBuf->InputYUVBuf;
                ibuff->planes[1].mem.phys = 0;
                ibuff->planes[0].i_size = ibuff->i_width * ibuff->i_height * 2;
                ibuff->planes[1].i_size = 0;
                break;
            default:
                break;
        }
        ibuff->i_flags = 0;
        ibuff->planes[0].i_used = ibuff->planes[0].i_size;
        ibuff->planes[1].i_used = ibuff->planes[1].i_size;

        obuff->i_memory = MMHE_MEMORY_MMAP;
        obuff->i_planes = 1;
        //obuff->planes[0].mem.phys = (unsigned long long)(unsigned int)pstInOutBuf->OutputBuf;
        //obuff->planes[0].i_size = pstInOutBuf->OutputBufSize;
        obuff->planes[0].i_used = 0;
        obuff->i_flags = 0;

        //MheCtxActions(mctx, IOCTL_MMHE_ACQUIRE_NONBLOCKINGA, (VOID *)vbuff);
        if(MheCtxEncPostProc(mctx, vbuff) != 0)
            eError = E_MHAL_ERR_ILLEGAL_PARAM;

        pstEncRet->u32OutputBufUsed = obuff->planes[0].i_used;

        CamOsGetMonotonicTime(&timer);
        mctx->p_device->encode_total_ns1 += _MheTimespecDiffNs(&mctx->p_device->encode_start_time, &timer);

        eError = 0;
    }
    else
    {
        eError = E_MHAL_ERR_NULL_PTR;
    }
    return eError;
}

MS_S32 MHAL_MHE_QueryBufSize(MHAL_VENC_INST_HANDLE hInst, MHAL_VENC_InternalBuf_t *pstParam)
{

    mmhe_ctx* mctx = (mmhe_ctx*)hInst;
    mhve_ops* mops = mctx->p_handle;
    mhve_cfg mcfg;
    int size_mbr_lut, size_mbr_read_map, enc_frame_size;
    int size_mbr_gn, size_gn_mem;
    int size_ppu_int_b, size_ppu_int_a;
    int size_col, size_ppu_y, size_ppu_c;
    int size_dump_reg;
    int cbw, cbh, cbn;
    int size_lum, size_chr, size_out;
    int size_ppu_total;
    int size_total_al, size_total_rc;
    //int score;
    int size_value;
    int enc_w, enc_h;

    if(MMHE_CTX_STATE_NULL != mctx->i_state)
        return E_MHAL_ERR_NULL_PTR;

    mctx->i_dmems = 0;

    mcfg.type = MHVE_CFG_RES;
    mops->get_conf(mops, &mcfg);
    cbw = _ALIGN_(5, mcfg.res.i_pixw) / 32;
    cbh = _ALIGN_(5, mcfg.res.i_pixh) / 32;
    cbn = cbw * cbh;

    enc_w = _ALIGN_(5, mcfg.res.i_pixw);
    enc_h = _ALIGN_(5, mcfg.res.i_pixh);

    /* Calculate required buffer size */

    size_dump_reg = 256;    // Fixme, 25x64bit on I2 MHE

    // PMBR (delta-)QP LUT
    size_mbr_lut = 256;
    enc_frame_size = enc_w * enc_h;

    // CTB weight read (or write) buffer
    // Each CTB occupies 1 byte
    // Register: reg_mbr_gn_read_st_addr, reg_mbr_gn_write_st_addr
    size_mbr_gn = (((enc_frame_size >> 10) >> 5) + 1) << 5;

    // ROI map: including (delta-)QP-entry map, forced-Intra map, forced-ZeroMV map
    // Each CTB occupies 2 bytes.
    // Register: reg_mbr_gn_read_map_st_addr
    size_mbr_read_map = (((enc_frame_size >> 9) >> 5) + 1) << 5;
    //size_mbr_read_map += 256; //fixed QP value error in last line when ROI enabled

    // GN buffer
    // Each CTB occupies 32 * 8 bytes
    // Register: reg_hev_gn_mem_sadr
    size_gn_mem = enc_w * 8;
#if defined(MMHE_IMI_BUF_ADDR)
    size_gn_mem = enc_w * 4;
#endif

    // PPU intermedia A, B buffer
    // Register: reg_hev_ppu_fb_a_y_base, reg_hev_ppu_fb_b_y_base
    size_ppu_int_b = enc_w * 8;
    size_ppu_int_a = enc_h * 8;
#if defined(MMHE_IMI_BUF_ADDR)
    size_ppu_int_b = enc_w * 10;
#endif

    // Colocated buffer (read and write)
    // Each CTB occupies 64 bytes
    // Register: reg_hev_col_r_sadr0, reg_hev_col_w_sadr
    size_col = enc_frame_size / 16;

    // Reference frame and reconstruction frame
    // Register: reg_hev_ref_y_adr, reg_hev_ppu_fblut_luma_base
    //           reg_hev_ref_c_adr, reg_hev_ppu_fblut_chroma_base
    size_ppu_y = enc_frame_size;
    size_ppu_c = enc_frame_size / 2;

    // Low bandwidth - imi
#if defined(MMHE_IMI_BUF_ADDR)
    ctb_width = ((mcfg.res.i_pixw + 31) >> 5);
    size_imi_ref_y = (ctb_width << 5) * 96 + 4096; // (32*LumaCtbWidth)*96 + 4096  Bytes
    size_imi_ref_c = (ctb_width << 5) * 48 + 2048; // (32*ChromaCtbWidth)*48 + 2048 Bytes
#endif

    // 512 byte align
    size_value = size_ppu_int_b;
    size_ppu_int_b = _ALIGN_(9, size_value);
    size_value = size_ppu_int_a;
    size_ppu_int_a = _ALIGN_(9, size_value);
    size_value = size_ppu_y;
    size_ppu_y = _ALIGN_(9, size_value);
    size_value = size_ppu_c;
    size_ppu_c = _ALIGN_(9, size_value);

    size_value = size_mbr_gn;
    size_mbr_gn = _ALIGN_(9, size_value);
    size_value = size_mbr_read_map;
    size_mbr_read_map = _ALIGN_(9, size_value);
    size_value = size_mbr_lut;
    size_mbr_lut = _ALIGN_(9, size_value);

    size_ppu_total = size_ppu_int_b + size_ppu_int_a ;

    // output bitstream buffer
    size_lum = _ALIGN_(16, cbn * MBPIXELS_Y * 4);
    size_chr = _ALIGN_(16, cbn * MBPIXELS_C * 4);
    size_out = _ALIGN_(20, size_lum);
    if(mctx->i_omode == MMHE_OMODE_MMAP)
        size_out = 0;

    /* Calculate total size */
    size_total_rc = (size_ppu_y + size_ppu_c + size_col) * 2;
    size_total_al = size_dump_reg + size_gn_mem + size_mbr_lut + size_mbr_read_map + size_mbr_gn*2 + size_ppu_total;

    pstParam->u32IntrAlBufSize = size_total_al;
    pstParam->u32IntrRefBufSize = size_total_rc;

#if 0
    CamOsPrintf("MHE memory info: (QueryBufSize)\n");
    CamOsPrintf("-------------------\n");
    CamOsPrintf("size_total_rc: %d \n", size_total_rc);
    CamOsPrintf("size_total_al: %d \n", size_total_al);
    CamOsPrintf("   size_dump_reg    : %d \n", size_dump_reg);
    CamOsPrintf("   size_gn_mem      : %d \n", size_gn_mem);
    CamOsPrintf("   size_mbr_lut     : %d \n", size_mbr_lut);
    CamOsPrintf("   size_mbr_read_map: %d \n", size_mbr_read_map);
    CamOsPrintf("   size_mbr_gn      : %d \n", size_mbr_gn);
    CamOsPrintf("   size_ppu_total   : %d \n", size_ppu_total);
    CamOsPrintf("-------------------\n");
#endif

    return 0;
}


MS_S32 MHAL_MHE_GetDevConfig(MHAL_VENC_DEV_HANDLE hDev, MHAL_VENC_IDX eType, MHAL_VENC_Param_t* pstParam)
{
    mmhe_dev *mdev = (mmhe_dev *)hDev;
    MHAL_ErrCode_e eError;
    MHAL_VENC_ParamInt_t* pVEncInt = NULL;

    if(mdev)
    {
        switch(eType)
        {
            case E_MHAL_VENC_HW_IRQ_NUM:
                if(pstParam == NULL)
                    return E_MHAL_ERR_NULL_PTR;

                pVEncInt = (MHAL_VENC_ParamInt_t*)pstParam;

                if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamInt_t))))
                {
                    break;
                }

                pVEncInt->u32Val = mdev->i_irq;

                eError = 0;
                break;

            case E_MHAL_VENC_HW_CMDQ_BUF_LEN:
                if(pstParam == NULL)
                    return E_MHAL_ERR_NULL_PTR;

                pVEncInt = (MHAL_VENC_ParamInt_t*)pstParam;

                if((eError = _MheCheckHeader(pstParam, sizeof(MHAL_VENC_ParamInt_t))))
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

MS_S32 MHAL_MHE_IsrProc(MHAL_VENC_DEV_HANDLE hDev)
{
    mmhe_dev *mdev = (mmhe_dev *)hDev;
    CamOsTimespec_t timer;
    MS_S32 ret;

    if (mdev)
    {
        ret = MheDevIsrFnx(mdev);

        CamOsGetMonotonicTime(&timer);
        mdev->encode_total_ns0 += _MheTimespecDiffNs(&mdev->encode_start_time, &timer);
        return ret;
    }

    return 0;
}
