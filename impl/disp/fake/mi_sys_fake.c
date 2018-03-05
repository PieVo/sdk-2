#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/slab.h>
///#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/irqreturn.h>
#include <linux/list.h>

#include "mi_sys.h"
#include "mi_sys_internal.h"
#include "mi_print.h"
#include "mi_sys_datatype.h"
#define E_MI_ERR_BUSY (2)
#define MI_SUCCESS (0)
#define MI_SYS_CHECK_HANDLE_FAILED(miSysDrvHandle) (FALSE)
#define MI_SYS_CHECK_CHNN_FAILED(u32ChnId) (((miSysDrvHandle) != &_gstSysFake.stChannelInfo[0]) && ((miSysDrvHandle) != &_gstSysFake.stChannelInfo[1]))
#define MI_SYS_CHECK_PORT_FAILED(u32PortId) (FALSE)

typedef struct {
    MI_BOOL bEnable;
} mi_sys_fakeChannelInfo_t;

typedef struct {
    MI_U32 u32Count;
    mi_sys_fakeChannelInfo_t stChannelInfo[2];
} mi_sys_fake_test;

static mi_sys_fake_test _gstSysFake = { 0 };

MI_S32 mi_sys_EnableChannel(MI_SYS_DRV_HANDLE miSysDrvHandle, MI_U32 u32ChnId)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    mi_sys_fakeChannelInfo_t *pChnnInfo = (mi_sys_fakeChannelInfo_t *)miSysDrvHandle;

    if (MI_SYS_CHECK_HANDLE_FAILED(miSysDrvHandle) || MI_SYS_CHECK_CHNN_FAILED(u32ChnId))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    pChnnInfo->bEnable = TRUE;
    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 mi_sys_DisableChannel(MI_SYS_DRV_HANDLE miSysDrvHandle, MI_U32 u32ChnId)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    mi_sys_fakeChannelInfo_t *pChnnInfo = (mi_sys_fakeChannelInfo_t *)miSysDrvHandle;
    if (MI_SYS_CHECK_HANDLE_FAILED(miSysDrvHandle) || MI_SYS_CHECK_CHNN_FAILED(u32ChnId))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    pChnnInfo->bEnable = FALSE;

    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 mi_sys_MMA_Alloc(MI_U8 *u8MMAHeapName, MI_U32 u32Size ,MI_PHY *phyAddr)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    // MI_U8 *u8MMAHeapName, MI_U32 u32Size ,MI_PHY *phyAddr;
#if 0
    //if (NULL == MI_U8 *u8MMAHeapName, MI_U32 u32Size ,MI_PHY *phyAddr )
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
#endif
    DBG_INFO("%s()@line%d sucess u8MMAHeapName: %s u32Size: %d!!!\n", __func__, __LINE__, u8MMAHeapName,u32Size);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 mi_sys_MMA_Free(MI_PHY phyAddr)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
#if 0
    // MI_PHY phyAddr;
    //if (NULL == MI_PHY phyAddr )
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
#endif
    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

static MI_BOOL _gBufferUesd[3] = {FALSE, FALSE, FALSE};
static MI_SYS_BufInfo_t _gBuffInfo[3] = {
    {
        .eBufType = E_MI_SYS_BUFDATA_FRAME,
        .u64Pts   = 0x12345678,
        .bEndOfStream = TRUE,
        .bUsrBuf      = FALSE,
        .stFrameData  = {
            .eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE,
            .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
            .eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE,
            .eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE,
            .eFieldType = E_MI_SYS_FIELDTYPE_NONE,
            .u16Width = 1280,
            .u16Height = 720,
            .u32Stride[0] = 1280,
            .u32Stride[1] = 1280,
            .u32Stride[2] = 1280,
			.phyAddr[0] = 0xD800000,
			.phyAddr[1] = 0xD800000,
			.phyAddr[2] = 0xD800000,
        },
    },
    {
        .eBufType = E_MI_SYS_BUFDATA_FRAME,
        .u64Pts   = 0x12345700,
        .bEndOfStream = TRUE,
        .bUsrBuf      = FALSE,
        .stFrameData  = {
            .eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE,
            .ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
            .eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE,
            .eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE,
            .eFieldType = E_MI_SYS_FIELDTYPE_NONE,
			.u16Width = 1280,
			.u16Height = 720,
			.u32Stride[0] = 1280,
			.u32Stride[1] = 1280,
			.u32Stride[2] = 1280,
			.phyAddr[0] = 0xD9C2000,
			.phyAddr[1] = 0xD9C2000,
			.phyAddr[2] = 0xD9C2000,
        },
    },
    {
		.eBufType = E_MI_SYS_BUFDATA_FRAME,
		.u64Pts   = 0x12345702,
		.bEndOfStream = TRUE,
		.bUsrBuf	  = FALSE,
		.stFrameData  = {
			.eTileMode = E_MI_SYS_FRAME_TILE_MODE_NONE,
			.ePixelFormat = E_MI_SYS_PIXEL_FRAME_YUV422_YUYV,
			.eCompressMode = E_MI_SYS_COMPRESS_MODE_NONE,
			.eFrameScanMode = E_MI_SYS_FRAME_SCAN_MODE_PROGRESSIVE,
			.eFieldType = E_MI_SYS_FIELDTYPE_NONE,
			.u16Width = 1280,
			.u16Height = 720,
			.u32Stride[0] = 1280,
			.u32Stride[1] = 1280,
			.u32Stride[2] = 1280,
			.phyAddr[0] = 0xDB84000,
			.phyAddr[1] = 0xDB84000,
			.phyAddr[2] = 0xDB84000,
		},
    },

};

MI_S32 mi_sys_FinishBuf(MI_SYS_BufInfo_t*buf)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    int i = 0;
    if ((NULL == buf) && (buf != &_gBuffInfo[0]) && (buf != &_gBuffInfo[1]) && (buf != &_gBuffInfo[2]))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    for (i = 0; i < sizeof(_gBufferUesd)/sizeof(_gBufferUesd[0]); i++)
    {
        if (&_gBuffInfo[i] == buf)
        {
            _gBufferUesd[i] = FALSE;
            s32Ret = MI_SUCCESS;
            break;
        }
    }

    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_S32 mi_sys_WaitOnInputTaskAvailable(MI_SYS_DRV_HANDLE miSysDrvHandle , MI_S32 u32TimeOutMs)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    static MI_U32  u32Cnt = 0;
    if (MI_SYS_CHECK_HANDLE_FAILED(miSysDrvHandle))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    schedule_timeout_interruptible(msecs_to_jiffies(u32TimeOutMs));//sleep 10ms
    if (u32Cnt++ > 1000*20)
    {
        DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
        u32Cnt = 0;
    }
    s32Ret = MI_SUCCESS;
    return s32Ret;
}

MI_SYS_BufInfo_t *mi_sys_GetInputPortBuf(MI_SYS_DRV_HANDLE miSysDrvHandle, MI_U32 u32ChnId, MI_U32 u32PortId, MI_U32 u32Flags)
{
    MI_SYS_BufInfo_t *pBuffer = NULL;
    mi_sys_fakeChannelInfo_t *pChnnInfo = (mi_sys_fakeChannelInfo_t *)miSysDrvHandle;
    int i = 0;
    if (MI_SYS_CHECK_HANDLE_FAILED(miSysDrvHandle)
        || MI_SYS_CHECK_CHNN_FAILED(u32ChnId)
        || MI_SYS_CHECK_PORT_FAILED(u32PortId)
        )
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return NULL;

    }

    if (pChnnInfo->bEnable == FALSE)
    {
        return NULL;
    }

    for (i = 0; i < sizeof(_gBufferUesd)/sizeof(_gBufferUesd[0]); i++)
    {
        if (_gBufferUesd[i] == FALSE)
        {
            _gBufferUesd[i] = TRUE;
            pBuffer = &_gBuffInfo[i];
            break;
        }
    }
    DBG_INFO("%s()@line%d sucess pBuffer: %p!!!\n", __func__, __LINE__, pBuffer);

    return pBuffer;
}

MI_S32 mi_sys_RewindBuf(MI_SYS_BufInfo_t*buf)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    int i = 0;
    if ((NULL == buf) && (buf != &_gBuffInfo[0]) && (buf != &_gBuffInfo[1]) && (buf != &_gBuffInfo[2]))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    for (i = 0; i < sizeof(_gBufferUesd)/sizeof(_gBufferUesd[0]); i++)
    {
        if (&_gBuffInfo[i] == buf)
        {
            _gBufferUesd[i] = FALSE;
            s32Ret = MI_SUCCESS;
            break;
        }
    }

    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);

    return s32Ret;
}
#define SYS_TEST_MAX (2)
MI_SYS_DRV_HANDLE  mi_sys_RegisterDev(mi_sys_ModuleDevInfo_t *pstMouleInfo, mi_sys_ModuleDevBindOps_t *pstModuleBindOps , void *pUsrData)
{
    MI_SYS_DRV_HANDLE handle = NULL;
    if (_gstSysFake.u32Count++ > SYS_TEST_MAX)
    {
        return handle;
    }

    if (NULL == pstMouleInfo)
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return handle;
    }

    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    return &_gstSysFake.stChannelInfo[pstMouleInfo->u32DevId];
}

MI_S32 mi_sys_UnRegisterDev(MI_SYS_DRV_HANDLE miSysDrvHandle)
{
    MI_S32 s32Ret = E_MI_ERR_BUSY;
    if (((miSysDrvHandle) != &_gstSysFake.stChannelInfo[0]) && ((miSysDrvHandle) != &_gstSysFake.stChannelInfo[1]))
    {
        DBG_ERR("%s()@line%d pointer Failed!!!\n", __func__, __LINE__);
        return E_MI_ERR_BUSY;

    }
    _gstSysFake.u32Count--;

    DBG_INFO("%s()@line%d sucess!!!\n", __func__, __LINE__);
    s32Ret = MI_SUCCESS;
    return s32Ret;
}
