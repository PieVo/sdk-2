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

#ifndef _MI_WARP_DATATYPE_H_
#define _MI_WARP_DATATYPE_H_
#include "mi_sys_datatype.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MI_WARP_MAX_DEVICE_NUM      1
#define MI_WARP_MAX_CHN_NUM         1
#define MI_WARP_MAX_INPUTPORT_NUM   1
#define MI_WARP_MAX_OUTPUTPORT_NUM  1


typedef enum
{
    E_MI_WARP_ERR_DEV_OPENED = MI_WARP_INITIAL_ERROR_CODE,
    E_MI_WARP_ERR_DEV_NOT_OPEN,
    E_MI_WARP_ERR_DEV_NOT_CLOSE,
    E_MI_WARP_ERR_CHN_OPENED,
    E_MI_WARP_ERR_CHN_NOT_OPEN,
    E_MI_WARP_ERR_CHN_NOT_STOPED,
    E_MI_WARP_ERR_CHN_STOPED,
    E_MI_WARP_ERR_CHN_NOT_CLOSE,
    E_MI_WARP_ERR_PORT_NOT_UNBIND,
}MI_WARP_ErrCode_e;

typedef void* MI_WARP_HANDLE;
typedef void* MI_WARP_INSTANCE;
typedef MI_U32 MI_WARP_DEV;
typedef MI_U32 MI_WARP_PORT;
typedef MI_U32 MI_WARP_CHN;

typedef struct MI_WARP_OutputPortAttr_s
{
    MI_U16 u16Width;                         // Width of target image
    MI_U16 u16Height;                        // Height of target image
    MI_SYS_PixelFormat_e  ePixelFormat;            // Pixel format of target image
    MI_SYS_CompressMode_e eCompressMode;   // Compression mode of the output
} MI_WARP_OutputPortAttr_t;

typedef struct MI_WARP_InputPortAttr_s
{
    MI_U16 u16Width;                         // Width of target image
    MI_U16 u16Height;                        // Height of target image
    MI_SYS_PixelFormat_e  ePixelFormat;            // Pixel format of target image
    MI_SYS_CompressMode_e eCompressMode;   // Compression mode of the output
} MI_WARP_InputPortAttr_t;

typedef enum
{
    MI_WARP_CALLBACK_EVENT_ONBINDINPUTPORT,
    MI_WARP_CALLBACK_EVENT_ONBINDOUTPUTPORT,
    MI_WARP_CALLBACK_EVENT_ONUNBINDINPUTPORT,
    MI_WARP_CALLBACK_EVENT_ONUNBINDOUTPUTPORT,
    MI_WARP_CALLBACK_EVENT_EXIT,
    MI_WARP_CALLBACK_EVENT_TIMEOUT,
} MI_WARP_CALLBACK_EVENT_e;

typedef struct MI_WARP_ModuleDevInfo_s
{
    MI_ModuleId_e eModuleId;
    MI_U32 u32DevId;
//    MI_SYS_DRV_HANDLE hWarpHandle;
    MI_U32 u32InputPortNum;
    MI_U32 u32OutputPortNum;
    MI_U32 u32DevChnNum;
}MI_WARP_ModuleDevInfo_t;

typedef MI_S32 (* MI_WARP_BindCallback)(MI_SYS_ChnPort_t *pstChnCurryPort, MI_SYS_ChnPort_t *pstChnPeerPort ,void *pUsrData);
typedef struct MI_WARP_RegisterDevParams_s
{
    MI_WARP_ModuleDevInfo_t stModDevInfo;
    MI_WARP_BindCallback OnBindInputPort;
    MI_WARP_BindCallback OnBindOutputPort;
    MI_WARP_BindCallback OnUnBindInputPort;
    MI_WARP_BindCallback OnUnBindOutputPort;
    void *pUsrData;
} MI_WARP_RegisterDevParams_t;


// config table
typedef enum
{
    MI_WARP_OP_MODE_PERSPECTIVE           = 0,
    MI_WARP_OP_MODE_MAP                   = 1
} MI_WARP_OP_MODE_E;

/// @brief MI WARP isr state
typedef enum
{
    MI_WARP_ISR_STATE_DONE                = 0,
}MI_WARP_ISR_STATE_E;

/// @brief MI WARP image formats enumeration
typedef enum
{
    MI_WARP_IMAGE_FROMAT_RGBA             = 0,
    MI_WARP_IMAGE_FROMAT_NV16             = 1,
    MI_WARP_IMAGE_FROMAT_NV12             = 2
} MI_WARP_IMAGE_FROMAT_E;

/// @brief MI WARP perspective coefficients enumeration
typedef enum
{
    MI_WARP_PERSECTIVE_COEFFS_C00         = 0,
    MI_WARP_PERSECTIVE_COEFFS_C01         = 1,
    MI_WARP_PERSECTIVE_COEFFS_C02         = 2,
    MI_WARP_PERSECTIVE_COEFFS_C10         = 3,
    MI_WARP_PERSECTIVE_COEFFS_C11         = 4,
    MI_WARP_PERSECTIVE_COEFFS_C12         = 5,
    MI_WARP_PERSECTIVE_COEFFS_C20         = 6,
    MI_WARP_PERSECTIVE_COEFFS_C21         = 7,
    MI_WARP_PERSECTIVE_COEFFS_C22         = 8,
    MI_WARP_PERSECTIVE_COEFFS_NUM_COEFFS  = 9
} MI_WARP_PERSECTIVE_COEFFS_E;

/// @brief MI WARP displacement map resolution enumeration
typedef enum
{
    MI_WARP_MAP_RESLOUTION_8X8            = 0,
    MI_WARP_MAP_RESLOUTION_16X16          = 1
} MI_WARP_MAP_RESLOUTION_E;

/// @brief MI WARP displacement map format enumeration
typedef enum
{
    MI_WARP_MAP_FORMAT_ABSOLUTE           = 0,
    MI_WARP_MAP_FORMAT_RELATIVE           = 1
} MI_WARP_MAP_FORMAT_E;

/// @brief MIimage plane index enumeration
typedef enum
{
    MI_WARP_IMAGE_PLANE_RGBA = 0,
    MI_WARP_IMAGE_PLANE_Y = 0,
    MI_WARP_IMAGE_PLANE_UV = 1
} MI_WARP_IMAGE_PLANE_E;

/// @brief MI WARP displacement map entry for absolute coordinates format
typedef struct
{
    MI_S32 y;
    MI_S32 x;
} MI_WARP_DISPPLAY_ABSOLUTE_ENTRY_T;

/// @brief MI WARP displacement map entry for coordinates' relative offset format
typedef struct
{
    MI_S16 y;
    MI_S16 x;
} MI_WARP_DISPPLAY_RELATIVE_ENTRY_T;

/// @brief MI WARP displacement table descriptor
typedef struct
{
    MI_WARP_MAP_RESLOUTION_E resolution;              // map resolution
    MI_WARP_MAP_FORMAT_E     format;                  // map format (absolute, relative)
    union
    {
        const MI_WARP_DISPPLAY_ABSOLUTE_ENTRY_T* absolute_table;
        const MI_WARP_DISPPLAY_RELATIVE_ENTRY_T* relative_table;
        MI_U32* overlay;
    };
} MI_WARP_DISPLAY_TABLE_T;

typedef struct
{
    MI_S16 height;                                      // input tile height
    MI_S16 width;                                       // input tile width
    MI_S16 y;                                           // input tile top left y coordinate
    MI_S16 x;                                           // input tile top left x coordinate
}  MI_WARP_BOUND_BOX_ENTRY_T;

typedef struct
{
    MI_S32 size;                                        // number of bound boxes
    const MI_WARP_BOUND_BOX_ENTRY_T *table;           // list of all bound boxes
}MI_WARP_BOUND_BOX_TABLE_T;

/// @brief MI WARP image descriptor
typedef struct
{
    MI_WARP_IMAGE_FROMAT_E format;                    // image format
    MI_U32 width;                                       // image width  (for YUV - Y plane width)
    MI_U32 height;                                      // image height (for YUV - Y plane width)
} MI_WARP_IMAGE_DESC_T;

/// @brief MI WARP image data structure
typedef struct
{
    MI_U32 num_planes;                                  // number of image planes
    MI_U8* data[2];                                     // pointers to the image planes' data
} MI_WARP_IMAGE_DATA_T;

typedef struct MI_WARP_Config_s
{
    MI_U32 u32Mode;

    MI_WARP_OP_MODE_E op_mode;                        // Operation mode
    MI_WARP_DISPLAY_TABLE_T disp_table;               // Displacement table descriptor
    MI_WARP_BOUND_BOX_TABLE_T bb_table;               // Bounding box table descriptor

    MI_S32 u32Coeff[MI_WARP_PERSECTIVE_COEFFS_NUM_COEFFS];  // Perspective transform coefficients

    MI_WARP_IMAGE_DESC_T input_image;                 // Input image
    MI_WARP_IMAGE_DATA_T input_data;

    MI_WARP_IMAGE_DESC_T output_image;                // Output image
    MI_WARP_IMAGE_DATA_T output_data;

    MI_U32 afbc_en;                                     // Enable AFBC mode
    MI_U8 fill_value[2];                                // Fill value for out of range pixels

    MI_U32 axi_max_read_burst_size;                     // Maximum read burst size of the AXI master port
    MI_U32 axi_max_write_burst_size;                    // Maximum write burst size of the AXI master port
    MI_U32 axi_max_read_outstanding;                    // Maximum outstanding read accesses of the AXI master port
    MI_U32 axi_max_write_outstanding;                   // Maximum outstanding write accesses of the AXI master port

    // Debug features
    MI_U32 debug_bypass_displacement_en;
    MI_U32 debug_bypass_interp_en;

    MI_U32 output_tiles_width;                          // calculated internally based on the bounding boxes
    MI_U32 output_tiles_height;                         // calculated internally based on the bounding boxes
    MI_U32 num_output_tiles;                            // calculated internally based on the bounding boxes
}MI_WARP_Config_t;

typedef enum
{
    MI_WARP_IMAGE_FORMAT_RGBA = 0,
    MI_WARP_IMAGE_FORMAT_NV16 = 1,
    MI_WARP_IMAGE_FORMAT_NV12 = 2,
    MI_WARP_IMAGE_FORMAT_YONLY = 3
} MI_WARP_ImageFormat_e;

typedef struct
{
    MI_U32 u32Width;
    MI_U32 u32Height;
    MI_WARP_ImageFormat_e eFormat;
    MI_U8*  pu8Buffer[2];
} MI_WARP_ImageBuf_t;

typedef MI_U32 (*MI_WARP_AppCallback)(MI_U32 u32CvCmd, MI_U32 u32InBufNo, MI_WARP_ImageBuf_t **ppInBuf, MI_U32 u32OutBufNo, MI_WARP_ImageBuf_t **ppOutBuf);

#define MI_WARP_OK                      MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_INFO, MI_SUCCESS)
#define MI_ERR_WARP_ILLEGAL_PARAM       MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_ILLEGAL_PARAM)
#define MI_ERR_WARP_NULL_PTR            MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_NULL_PTR)
#define MI_ERR_WARP_BUSY                MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_BUSY)
#define MI_ERR_WARP_FAIL                MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_FAILED)
#define MI_ERR_WARP_INVALID_DEVID       MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_INVALID_DEVID)
#define MI_ERR_WARP_NOT_SUPPORT         MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_NOT_SUPPORT)
#define MI_ERR_WARP_MOD_INITED          MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_INITED)
#define MI_ERR_WARP_MOD_NOT_INIT        MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_NOT_INIT)
#define MI_ERR_WARP_DEV_OPENED          MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_DEV_OPENED)
#define MI_ERR_WARP_DEV_NOT_OPEN        MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_DEV_NOT_OPEN)
#define MI_ERR_WARP_DEV_NOT_STOP        MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_DEV_NOT_STOPED)
#define MI_ERR_WARP_DEV_NOT_CLOSE       MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_DEV_NOT_CLOSE)
#define MI_ERR_WARP_NOT_CONFIG          MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_NOT_CONFIG)
#define MI_ERR_WARP_CHN_OPENED          MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_CHN_OPENED)
#define MI_ERR_WARP_CHN_NOT_OPEN        MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_CHN_NOT_OPEN)
#define MI_ERR_WARP_CHN_NOT_STOP        MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_CHN_NOT_STOPED)
#define MI_ERR_WARP_CHN_STOPED         MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_CHN_STOPED)
#define MI_ERR_WARP_CHN_NOT_CLOSE       MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_CHN_NOT_CLOSE)
#define MI_ERR_WARP_PORT_NOT_DISABLE    MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_ERR_NOT_DISABLE)
#define MI_ERR_WARP_PORT_NOT_UNBIND     MI_DEF_ERR(E_MI_MODULE_ID_WARP, E_MI_ERR_LEVEL_ERROR, E_MI_WARP_ERR_PORT_NOT_UNBIND)


#ifdef __cplusplus
}
#endif

#endif///_MI_VPE_DATATYPE_H_
