////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

#include <linux/pfn.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/string.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/wait.h>


#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "ms_platform.h"
#include "ms_msys.h"

#include "drv_scl_os.h"
#include "drv_scl_dbg.h"
#include "hal_scl_util.h"
#include "drv_scl_hvsp_m.h"
#include "drv_scl_dma_m.h"

#include "drv_scl_ioctl.h"
#include "drv_scl_hvsp_io_st.h"
#include "drv_scl_hvsp_io_wrapper.h"
#include "drv_scl_dma_io_st.h"
#include "drv_scl_dma_io_wrapper.h"
#include "drv_scl_vip_ioctl.h"
#include "drv_scl_vip_io_st.h"
#include "drv_scl_vip_io_wrapper.h"
#include "drv_scl_verchk.h"
#include "mhal_vpe.h"
#include "drv_scl_vip_io.h"
//-------------------------------------------------------------------------------------------------
// Define & Macro
//-------------------------------------------------------------------------------------------------
#define DRV_SCL_DEVICE_COUNT    1
#define DRV_SCL_DEVICE_NAME     "mscl"
#define DRV_SCL_DEVICE_MINOR    0x10
#define DRV_SCLVIP_DEVICE_NAME     "msclvip"
#define DRV_SCLVIP_DEVICE_MINOR    0x11
#define DRV_SCLHVSP_DEVICE_NAME1     "msclhvsp1"
#define DRV_SCLHVSP_DEVICE_MINOR1    0x12
#define DRV_SCLHVSP_DEVICE_NAME2     "msclhvsp2"
#define DRV_SCLHVSP_DEVICE_MINOR2    0x13
#define DRV_SCLHVSP_DEVICE_NAME3     "msclhvsp3"
#define DRV_SCLHVSP_DEVICE_MINOR3    0x14
#define DRV_SCLHVSP_DEVICE_NAME4     "msclhvsp4"
#define DRV_SCLHVSP_DEVICE_MINOR4   0x15
#define DRV_SCLDMA_DEVICE_NAME1     "mscldma1"
#define DRV_SCLDMA_DEVICE_MINOR1    0x16
#define DRV_SCLDMA_DEVICE_NAME2     "mscldma2"
#define DRV_SCLDMA_DEVICE_MINOR2    0x17
#define DRV_SCLDMA_DEVICE_NAME3     "mscldma3"
#define DRV_SCLDMA_DEVICE_MINOR3    0x18
#define DRV_SCLDMA_DEVICE_NAME4     "mscldma4"
#define DRV_SCLDMA_DEVICE_MINOR4   0x19
#define DRV_SCLM2M_DEVICE_NAME     "msclm2m"
#define DRV_SCLM2M_DEVICE_MINOR    0x1a
#define DRV_SCL_DEVICE_MAJOR    0xea
#define TEST_INST_MAX 64
#define VPE_DEVICE_COUNT 8
#define M2M_DEVICE_COUNT 3
#define VPE_IQAPI_TEST 0
//-------------------------------------------------------------------------------------------------
// Prototype
//-------------------------------------------------------------------------------------------------

int DrvSclModuleOpen(struct inode *inode, struct file *filp);
int DrvSclModuleRelease(struct inode *inode, struct file *filp);
long DrvSclModuleIoctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
static int DrvSclModuleProbe(struct platform_device *pdev);
static int DrvSclModuleRemove(struct platform_device *pdev);
static int DrvSclModuleSuspend(struct platform_device *dev, pm_message_t state);
static int DrvSclModuleResume(struct platform_device *dev);
static unsigned int DrvSclModulePoll(struct file *filp, struct poll_table_struct *wait);
extern int DrvSclIoctlParse(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
extern int DrvSclM2MIoctlParse(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
extern long DrvSclVipIoctlParse(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg);
//-------------------------------------------------------------------------------------------------
// Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    int s32Major;
    int s32Minor;
    int refCnt;
    struct cdev cdev;
    struct file_operations fops;
    struct device *devicenode;
}DrvSclModuleDevice_t;

//-------------------------------------------------------------------------------------------------
// Variable
//-------------------------------------------------------------------------------------------------
extern u8  gbdbgmessage[EN_DBGMG_NUM_CONFIG];

static DrvSclModuleDevice_t _tSclDevice =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCL_DEVICE_MINOR,
    .refCnt = 0,
    .devicenode = NULL,
    .cdev =
    {
        .kobj = {.name= DRV_SCL_DEVICE_NAME, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclVipDevice =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLVIP_DEVICE_MINOR,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLVIP_DEVICE_NAME, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclM2MDevice =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLM2M_DEVICE_MINOR,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLM2M_DEVICE_NAME, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};

static DrvSclModuleDevice_t _tSclHvsp4Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLHVSP_DEVICE_MINOR4,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLHVSP_DEVICE_NAME4, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclHvsp3Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLHVSP_DEVICE_MINOR3,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLHVSP_DEVICE_NAME3, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclHvsp2Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLHVSP_DEVICE_MINOR2,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLHVSP_DEVICE_NAME2, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclHvsp1Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLHVSP_DEVICE_MINOR1,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLHVSP_DEVICE_NAME1, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclDma1Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLDMA_DEVICE_MINOR1,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLDMA_DEVICE_NAME1, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclDma2Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLDMA_DEVICE_MINOR2,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLDMA_DEVICE_NAME2, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclDma3Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLDMA_DEVICE_MINOR3,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLDMA_DEVICE_NAME3, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};
static DrvSclModuleDevice_t _tSclDma4Device =
{
    .s32Major = DRV_SCL_DEVICE_MAJOR,
    .s32Minor = DRV_SCLDMA_DEVICE_MINOR4,
    .refCnt = 0,
    .cdev =
    {
        .kobj = {.name= DRV_SCLDMA_DEVICE_NAME4, },
        .owner = THIS_MODULE,
    },
    .fops =
    {
        .open = DrvSclModuleOpen,
        .release = DrvSclModuleRelease,
        .unlocked_ioctl = DrvSclModuleIoctl,
        .poll = DrvSclModulePoll,
    },
};



static struct class * _tSclHvspClass = NULL;
static char * SclHvspClassName = "m_sclhvsp_1_class";


static const struct of_device_id _SclMatchTable[] =
{
    { .compatible = "mstar,sclhvsp1_i2" },
    {}
};

static struct platform_driver stDrvSclPlatformDriver =
{
    .probe      = DrvSclModuleProbe,
    .remove     = DrvSclModuleRemove,
    .suspend    = DrvSclModuleSuspend,
    .resume     = DrvSclModuleResume,
    .driver =
    {
        .name   = DRV_SCL_DEVICE_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(_SclMatchTable),
    },
};

static u64 u64SclHvsp_DmaMask = 0xffffffffUL;

static struct platform_device stDrvSclPlatformDevice =
{
    .name = DRV_SCL_DEVICE_NAME,
    .id = 0,
    .dev =
    {
        .of_node = NULL,
        .dma_mask = &u64SclHvsp_DmaMask,
        .coherent_dma_mask = 0xffffffffUL
    }
};
#if defined(SCLOS_TYPE_LINUX_TEST)
typedef struct
{
    bool bEn;
    u8 u8level;
    u32 u32FrameSize;
    s32 Id;
}UTestTask_t;
#define ISPINPUTWIDTH 736
#define ISPINPUTHEIGHT 240
#define MAXINPUTWIDTH 1920
#define MAXINPUTHEIGHT 1080
#define MAXCROPWIDTH(w) ((w)/3*2)
#define MAXCROPHEIGHT(h) ((h)/3*2)
#define MAXCROPX(w) ((w)/4)
#define MAXCROPY(h) ((h)/4)
#define MAXSTRIDEINPUTWIDTH 352
#define MAXSTRIDEINPUTHEIGHT 288
#define MAXOUTPUTDISPWIDTH 352
#define MAXOUTPUTDISPHEIGHT 288
#define FHDOUTPUTWIDTH 1920
#define FHDOUTPUTHEIGHT 1080
#define XGAOUTPUTWIDTH 1440
#define XGAOUTPUTHEIGHT 900
#define HDOUTPUTWIDTH 1280
#define HDOUTPUTHEIGHT 720
#define VGAOUTPUTWIDTH 640
#define VGAOUTPUTHEIGHT 360
#define CIFOUTPUTWIDTH 352
#define CIFOUTPUTHEIGHT 288
#define QXGAOUTPUTWIDTH 2048
#define QXGAOUTPUTHEIGHT 1536
#define WQHDOUTPUTWIDTH 2560
#define WQHDOUTPUTHEIGHT 1440
#define QSXGAOUTPUTWIDTH 2560
#define QSXGAOUTPUTHEIGHT 2048
#define QFHDOUTPUTWIDTH8 3840
#define QFHDOUTPUTHEIGHT8 2160
#define FKUOUTPUTWIDTH 4096
#define FKUOUTPUTHEIGHT 2160
#define MAXOUTPUTSTRIDEH 1920
#define MAXOUTPUTSTRIDEV 1080
#define MAXOUTPUTportH MAXOUTPUTSTRIDEH/4
#define MAXOUTPUTportV MAXOUTPUTSTRIDEV/4
#define CH0INWIDTH FHDOUTPUTWIDTH
#define CH1INWIDTH ISPINPUTWIDTH
#define CH2INWIDTH VGAOUTPUTWIDTH
#define CH3INWIDTH HDOUTPUTWIDTH
#define CH0INHEIGHT FHDOUTPUTHEIGHT
#define CH1INHEIGHT ISPINPUTHEIGHT
#define CH2INHEIGHT VGAOUTPUTHEIGHT
#define CH3INHEIGHT HDOUTPUTHEIGHT
#define CH0OUTWIDTH HDOUTPUTWIDTH
#define CH1OUTWIDTH VGAOUTPUTWIDTH
#define CH2OUTWIDTH CIFOUTPUTWIDTH
#define CH3OUTWIDTH XGAOUTPUTWIDTH
#define CH0OUTHEIGHT HDOUTPUTHEIGHT
#define CH1OUTHEIGHT VGAOUTPUTHEIGHT
#define CH2OUTHEIGHT CIFOUTPUTHEIGHT
#define CH3OUTHEIGHT XGAOUTPUTHEIGHT




static MHalVpeSclWinSize_t stMaxWin[TEST_INST_MAX];
static MHalVpeSclCropConfig_t stCropCfg[TEST_INST_MAX];
static MHalVpeSclInputSizeConfig_t stInputpCfg[TEST_INST_MAX];
static MHalVpeSclOutputSizeConfig_t stOutputCfg[TEST_INST_MAX];
static MHalVpeSclOutputDmaConfig_t stOutputDmaCfg[TEST_INST_MAX];
static MHalVpeSclOutputMDwinConfig_t stMDwinCfg[TEST_INST_MAX];
static MHalVpeSclOutputBufferConfig_t stpBuffer[TEST_INST_MAX];
static MHalVpeIqConfig_t stIqCfg[TEST_INST_MAX];
static MHalVpeIqOnOff_t stIqOnCfg[TEST_INST_MAX];
static MHalVpeIqWdrRoiReport_t stRoiReport[TEST_INST_MAX];
static MHalVpeIqWdrRoiHist_t stHistCfg[TEST_INST_MAX];
static u32 u32Ctx[TEST_INST_MAX];
static u32 IdNum[TEST_INST_MAX];
static u32 u32IqCtx[TEST_INST_MAX];
static u32 IdIqNum[TEST_INST_MAX];
static char sg_scl_yc_name[TEST_INST_MAX][16];
static char KEY_DMEM_SCL_VPE_YC[20] = "VPE_YC";
static u32 sg_scl_yc_addr[TEST_INST_MAX];
static u32 sg_scl_yc_size[TEST_INST_MAX];
static u32 *sg_scl_yc_viraddr[TEST_INST_MAX];
//isp
static char sg_Isp_yc_name[TEST_INST_MAX][16];
static char KEY_DMEM_Isp_VPE_YC[20] = "Isp_YC";
static u32 sg_Isp_yc_addr[TEST_INST_MAX];
static u32 sg_Isp_yc_size[TEST_INST_MAX];
static u32 *sg_Isp_yc_viraddr[TEST_INST_MAX];
static u32 u32IspCtx[TEST_INST_MAX];
static u32 IdIspNum[TEST_INST_MAX];
static MHalVpeIspInputConfig_t stIspInputCfg[TEST_INST_MAX];
static MHalVpeIspRotationConfig_t stRotCfg[TEST_INST_MAX];
static MHalVpeIspVideoInfo_t stVdinfo[TEST_INST_MAX];
static struct file *readfp = NULL;
static struct file *writefp = NULL;
static struct file *readfp2 = NULL;
static struct file *writefp2 = NULL;
static struct file *readfp3 = NULL;
static struct file *writefp3 = NULL;
static struct file *readfp4 = NULL;
static struct file *writefp4 = NULL;
static MHAL_CMDQ_CmdqInterface_t *pCmdqCfg=NULL;
UTestTask_t gstUtDispTaskCfg;
static u8 u8glevel[TEST_INST_MAX];
#endif
//-------------------------------------------------------------------------------------------------
// internal function
//-------------------------------------------------------------------------------------------------
static u8 _mdrv_Scl_Changebuf2hex(int u32num)
{
    u8 u8level;
    if(u32num==10)
    {
        u8level = 1;
    }
    else if(u32num==48)
    {
        u8level = 0;
    }
    else if(u32num==49)
    {
        u8level = 0x1;
    }
    else if(u32num==50)
    {
        u8level = 0x2;
    }
    else if(u32num==51)
    {
        u8level = 0x3;
    }
    else if(u32num==52)
    {
        u8level = 0x4;
    }
    else if(u32num==53)
    {
        u8level = 0x5;
    }
    else if(u32num==54)
    {
        u8level = 0x6;
    }
    else if(u32num==55)
    {
        u8level = 0x7;
    }
    else if(u32num==56)
    {
        u8level = 0x8;
    }
    else if(u32num==57)
    {
        u8level = 0x9;
    }
    else if(u32num==65)
    {
        u8level = 0xa;
    }
    else if(u32num==66)
    {
        u8level = 0xb;
    }
    else if(u32num==67)
    {
        u8level = 0xc;
    }
    else if(u32num==68)
    {
        u8level = 0xd;
    }
    else if(u32num==69)
    {
        u8level = 0xe;
    }
    else if(u32num==70)
    {
        u8level = 0xf;
    }
    else if(u32num==97)
    {
        u8level = 0xa;
    }
    else if(u32num==98)
    {
        u8level = 0xb;
    }
    else if(u32num==99)
    {
        u8level = 0xc;
    }
    else if(u32num==100)
    {
        u8level = 0xd;
    }
    else if(u32num==101)
    {
        u8level = 0xe;
    }
    else if(u32num==102)
    {
        u8level = 0xf;
    }
    return u8level;
}
void _mdrv_Scl_SetInputTestPatternAndTgen(u16 Width,u16 Height)
{
    MDrvSclHvspMiscConfig_t stHvspMiscCfg;
#if defined(SCLOS_TYPE_LINUX_TEST)
    u8 u8InputTgenSetBuf[200] =
    {
        0x25, 0x15, 0x80, 0x03, 0xFF,// 4
        0x25, 0x15, 0x81, 0x80, 0xFF,// 9
        0x25, 0x15, 0x82, 0x30, 0xFF,
        0x25, 0x15, 0x83, 0x30, 0xFF,
        0x25, 0x15, 0x84, 0x10, 0xFF,
        0x25, 0x15, 0x85, 0x10, 0xFF,
        0x25, 0x15, 0x86, 0x02, 0xFF,
        0x25, 0x15, 0x87, 0x00, 0xFF,
        0x25, 0x15, 0x88, 0x21, 0xFF,
        0x25, 0x15, 0x89, 0x0C, 0xFF,
        0x25, 0x15, 0xE0, 0x01, 0xFF,
        0x25, 0x15, 0xE1, 0x00, 0xFF,
        0x25, 0x15, 0xE2, 0x01, 0xFF,
        0x25, 0x15, 0xE3, 0x00, 0xFF,
        0x25, 0x15, 0xE4, 0x03, 0xFF,
        0x25, 0x15, 0xE5, 0x00, 0xFF,
        0x25, 0x15, 0xE6, 0x05, 0xFF,
        0x25, 0x15, 0xE7, 0x00, 0xFF,
        0x25, 0x15, 0xE8, 0x3C, 0xFF,//94
        0x25, 0x15, 0xE9, 0x04, 0xFF,//99
        0x25, 0x15, 0xEA, 0x05, 0xFF,
        0x25, 0x15, 0xEB, 0x00, 0xFF,
        0x25, 0x15, 0xEC, 0x3C, 0xFF,//114
        0x25, 0x15, 0xED, 0x04, 0xFF,//119
        0x25, 0x15, 0xEE, 0xFF, 0xFF,//124
        0x25, 0x15, 0xEF, 0x08, 0xFF,//129
        0x25, 0x15, 0xF2, 0x04, 0xFF,
        0x25, 0x15, 0xF3, 0x00, 0xFF,
        0x25, 0x15, 0xF4, 0x7F, 0xFF,
        0x25, 0x15, 0xF5, 0x00, 0xFF,
        0x25, 0x15, 0xF6, 0xA8, 0xFF,
        0x25, 0x15, 0xF7, 0x00, 0xFF,
        0x25, 0x15, 0xF8, 0x27, 0xFF,//164
        0x25, 0x15, 0xF9, 0x08, 0xFF,//169
        0x25, 0x15, 0xFA, 0xA8, 0xFF,
        0x25, 0x15, 0xFB, 0x00, 0xFF,
        0x25, 0x15, 0xFC, 0x27, 0xFF,//184
        0x25, 0x15, 0xFD, 0x08, 0xFF,//189
        0x25, 0x15, 0xFE, 0xFF, 0xFF,//194
        0x25, 0x15, 0xFF, 0x0B, 0xFF,//199
    };
#else
    unsigned char u8InputTgenSetBuf[300] =
    {
        0x18, 0x12, 0x80, 0x03, 0xFF,// 4
        0x18, 0x12, 0x81, 0x80, 0xFF,// 9
        0x18, 0x12, 0x82, 0x30, 0xFF,
        0x18, 0x12, 0x83, 0x30, 0xFF,
        0x18, 0x12, 0x84, 0x10, 0xFF,
        0x18, 0x12, 0x85, 0x10, 0xFF,
        0x18, 0x12, 0x86, 0x02, 0xFF,
        0x18, 0x12, 0x87, 0x00, 0xFF,
        0x18, 0x12, 0x88, 0x21, 0xFF,
        0x18, 0x12, 0x89, 0x0C, 0xFF,
        0x18, 0x12, 0xE0, 0x01, 0xFF,
        0x18, 0x12, 0xE1, 0x00, 0xFF,
        0x18, 0x12, 0xE2, 0x01, 0xFF,
        0x18, 0x12, 0xE3, 0x00, 0xFF,
        0x18, 0x12, 0xE4, 0x03, 0xFF,
        0x18, 0x12, 0xE5, 0x00, 0xFF,
        0x18, 0x12, 0xE6, 0x05, 0xFF,
        0x18, 0x12, 0xE7, 0x00, 0xFF,
        0x18, 0x12, 0xE8, 0x3C, 0xFF,//94
        0x18, 0x12, 0xE9, 0x04, 0xFF,//99
        0x18, 0x12, 0xEA, 0x05, 0xFF,
        0x18, 0x12, 0xEB, 0x00, 0xFF,
        0x18, 0x12, 0xEC, 0x3C, 0xFF,//114
        0x18, 0x12, 0xED, 0x04, 0xFF,//119
        0x18, 0x12, 0xEE, 0xFF, 0xFF,//124
        0x18, 0x12, 0xEF, 0x08, 0xFF,//129
        0x18, 0x12, 0xF2, 0x04, 0xFF,
        0x18, 0x12, 0xF3, 0x00, 0xFF,
        0x18, 0x12, 0xF4, 0x7F, 0xFF,
        0x18, 0x12, 0xF5, 0x00, 0xFF,
        0x18, 0x12, 0xF6, 0xA8, 0xFF,
        0x18, 0x12, 0xF7, 0x00, 0xFF,
        0x18, 0x12, 0xF8, 0x27, 0xFF,//164
        0x18, 0x12, 0xF9, 0x08, 0xFF,//169
        0x18, 0x12, 0xFA, 0xA8, 0xFF,
        0x18, 0x12, 0xFB, 0x00, 0xFF,
        0x18, 0x12, 0xFC, 0x27, 0xFF,//184
        0x18, 0x12, 0xFD, 0x08, 0xFF,//189
        0x18, 0x12, 0xFE, 0xFF, 0xFF,//194
        0x18, 0x12, 0xFF, 0x0B, 0xFF,//199
        0x21, 0x12, 0xE0, 0x01, 0x01,//vip
        0x1E, 0x12, 0x70, 0x00, 0xFF,
        0x1E, 0x12, 0x71, 0x04, 0x07,
        0x1E, 0x12, 0x72, 0x00, 0xFF,
        0x1E, 0x12, 0x73, 0x00, 0x01,
        0x1E, 0x12, 0x74, 0x00, 0xFF,
        0x1E, 0x12, 0x75, 0x04, 0x07,
        0x1E, 0x12, 0x76, 0x00, 0xFF,
        0x1E, 0x12, 0x77, 0x00, 0x01,
        0x1E, 0x12, 0x78, 0x00, 0xFF,
        0x1E, 0x12, 0x79, 0x04, 0x07,
        0x1E, 0x12, 0x7A, 0x00, 0xFF,
        0x1E, 0x12, 0x7B, 0x00, 0x01,
        0x1E, 0x12, 0x7C, 0x00, 0xFF,
        0x1E, 0x12, 0x7D, 0x04, 0x07,
        0x1E, 0x12, 0x7E, 0x00, 0xFF,
        0x1E, 0x12, 0x7F, 0x00, 0x01,
    };
#endif
    // Input tgen setting
    u8InputTgenSetBuf[93]  = (u8)((0x4+Height)&0x00FF);
    u8InputTgenSetBuf[98]  = (u8)(((0x4+Height)&0xFF00)>>8);
    u8InputTgenSetBuf[113] = (u8)((0x4+Height)&0x00FF);
    u8InputTgenSetBuf[118] = (u8)(((0x4+Height)&0xFF00)>>8);
    u8InputTgenSetBuf[123] = 0xFF;
    u8InputTgenSetBuf[128] = 0x08;
    u8InputTgenSetBuf[163] = (u8)((0xA7+Width)&0x00FF);
    u8InputTgenSetBuf[168] = (u8)(((0xA7+Width)&0xFF00)>>8);
    u8InputTgenSetBuf[183] = (u8)((0xA7+Width)&0x00FF);
    u8InputTgenSetBuf[188] = (u8)(((0xA7+Width)&0xFF00)>>8);
    u8InputTgenSetBuf[193] = 0xFF;
    u8InputTgenSetBuf[198] = 0x0B;

    stHvspMiscCfg.u8Cmd     = 0;
    stHvspMiscCfg.u32Size   = sizeof(u8InputTgenSetBuf);
    stHvspMiscCfg.u32Addr   = (u32)u8InputTgenSetBuf;
    MDrvSclHvspSetMiscConfigForKernel(&stHvspMiscCfg);
}

void _mdrv_Scl_OpenInputTestPatternAndTgen(bool ball)
{
    MDrvSclHvspSetPatTgenStatus(ball);
}

void _mdrv_Scl_OpenTestPatternByISPTgen(bool bDynamic, bool ball)
{
    MDrvSclHvspMiscConfig_t stHvspMiscCfg;
    u8 input_tgen_buf[] =
    {
        0x25, 0x15, 0x80, 0x03, 0xFF,// 4
        0x25, 0x15, 0x81, 0x80, 0xFF,// 9
        0x25, 0x15, 0x82, 0x30, 0xFF,
        0x25, 0x15, 0x83, 0x30, 0xFF,
        0x25, 0x15, 0x84, 0x10, 0xFF,
        0x25, 0x15, 0x85, 0x10, 0xFF,
        0x25, 0x15, 0x86, 0x02, 0xFF,
        0x25, 0x15, 0x87, 0x00, 0xFF,
        0x25, 0x15, 0x88, 0x21, 0xFF,
        0x25, 0x15, 0x89, 0x0C, 0xFF,
    };
    if(bDynamic)
    {
        input_tgen_buf[3]   = (u8)0x7;
    }
    stHvspMiscCfg.u8Cmd     = ball ? E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINSTALL : E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINST;
    stHvspMiscCfg.u32Size   = sizeof(input_tgen_buf);
    stHvspMiscCfg.u32Addr   = (u32)input_tgen_buf;
    MDrvSclHvspSetMiscConfigForKernel(&stHvspMiscCfg);

}
void _mdrv_Scl_CloseTestPatternByISPTgen(void)
{
    MDrvSclHvspMiscConfig_t stHvspMiscCfg;
    u8 input_tgen_buf[] =
    {
        0x25, 0x15, 0x80, 0x00, 0xFF,// 4
        0x25, 0x15, 0x81, 0x00, 0xFF,// 9
        0x25, 0x15, 0x86, 0x02, 0xFF,
        0x25, 0x15, 0x88, 0x20, 0xFF,
        0x25, 0x15, 0x89, 0x0C, 0xFF,
        0x25, 0x15, 0xE0, 0x00, 0xFF,
        0x25, 0x15, 0xE1, 0x00, 0xFF,
    };
    stHvspMiscCfg.u8Cmd     = E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINSTALL;
    stHvspMiscCfg.u32Size   = sizeof(input_tgen_buf);
    stHvspMiscCfg.u32Addr   = (u32)input_tgen_buf;
    MDrvSclHvspSetMiscConfigForKernel(&stHvspMiscCfg);

}


static ssize_t ptgen_call_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        if((int)*str == 49)    //input 1  echo 1 >ptgen_call
        {
            SCL_ERR( "[HVSP1]ptgen all static OK %d\n",(int)*str);
            _mdrv_Scl_OpenTestPatternByISPTgen(E_MDRV_SCLHVSP_CALLPATGEN_STATIC,1);
        }
        else if((int)*str == 48)  //input 0  echo 0 >ptgen_call
        {
            SCL_ERR( "[HVSP1]ptgen_call_close %d\n",(int)*str);
            _mdrv_Scl_CloseTestPatternByISPTgen();
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[HVSP1]ptgen all dynamic %d\n",(int)*str);
            _mdrv_Scl_OpenTestPatternByISPTgen(E_MDRV_SCLHVSP_CALLPATGEN_DYNAMIC,1);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[HVSP1]ptgen inst static %d\n",(int)*str);
            _mdrv_Scl_OpenTestPatternByISPTgen(E_MDRV_SCLHVSP_CALLPATGEN_STATIC,0);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[HVSP1]ptgen inst dynamic %d\n",(int)*str);
            _mdrv_Scl_OpenTestPatternByISPTgen(E_MDRV_SCLHVSP_CALLPATGEN_DYNAMIC,0);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[HVSP1]ptgen all dynamic by sclself%d\n",(int)*str);
            _mdrv_Scl_OpenInputTestPatternAndTgen(1);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[HVSP1]ptgen inst dynamic by sclself%d\n",(int)*str);
            _mdrv_Scl_OpenInputTestPatternAndTgen(0);
        }
        return n;
    }

    return 0;
}
static ssize_t ptgen_call_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf,"0:close\n1:open static ptgen\n2:open dynamic ptgen\n3:open inst dynamic ptgen\n4:open inst dynamic ptgen\n");
}

static DEVICE_ATTR(ptgen,0644, ptgen_call_show, ptgen_call_store);


#if 0
static ssize_t check_clk_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    MDrvSclHvspClkConfig_t *stclk = NULL;
    DrvSclOsClkConfig_t stClkCfg;
    stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
    stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
    stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
    stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);
    stclk = (MDrvSclHvspClkConfig_t*)&(stClkCfg);
    return MDrvSclHvspClkFrameworkShow(buf,stclk);
}

static ssize_t check_clk_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    DrvSclOsClkConfig_t stClkCfg;
    stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
    stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
    stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
    stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);
    if(NULL!=buf)
    {
        const char *str = buf;
        if((int)*str == 49)    //input 1
        {
            SCL_ERR( "[CLK]open force mode %d\n",(int)*str);
            MDrvSclHvspSetClkForcemode(1);
        }
        else if((int)*str == 48)  //input 0
        {
            SCL_ERR( "[CLK]close force mode %d\n",(int)*str);
            MDrvSclHvspSetClkForcemode(0);
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[CLK]fclk1 max %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk1,0);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[CLK]fclk1 med %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk1,1);
        }
        else if((int)*str == 69)  //input E
        {
            SCL_ERR( "[CLK]fclk1 216 %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk1,2);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[CLK]fclk1 open %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptFclk1,1);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[CLK]fclk1 close %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptFclk1,0);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[CLK]fclk2 172 %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk2,0);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[CLK]fclk2 86 %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk2,1);
        }
        else if((int)*str == 70)  //input F
        {
            SCL_ERR( "[CLK]fclk2 216 %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptFclk2,2);
        }
        else if((int)*str == 56)  //input 8
        {
            SCL_ERR( "[CLK]fclk2 open %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptFclk2,1);
        }
        else if((int)*str == 57)  //input 9
        {
            SCL_ERR( "[CLK]fclk2 close %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptFclk2,0);
        }
        else if((int)*str == 58)  //input :
        {
            SCL_ERR( "[CLK]idclk ISP %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptIdclk,0x10);
        }
        else if((int)*str == 68)  //input D
        {
            SCL_ERR( "[CLK]idclk BT656 %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptIdclk,0x21);
        }
        else if((int)*str == 66)  //input B
        {
            SCL_ERR( "[CLK]idclk Open %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptIdclk,1);
        }
        else if((int)*str == 61)  //input =
        {
            SCL_ERR( "[CLK]idclk Close %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptIdclk,0);
        }
        else if((int)*str == 67)  //input C
        {
            SCL_ERR( "[CLK]odclk MAX %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptOdclk,0);
        }
        else if((int)*str == 63)  //input ?
        {
            SCL_ERR( "[CLK]odclk LPLL %d\n",(int)*str);
            MDrvSclHvspSetClkRate(stClkCfg.ptOdclk,3);
        }
        else if((int)*str == 64)  //input @
        {
            SCL_ERR( "[CLK]odclk open %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptOdclk,1);
        }
        else if((int)*str == 65)  //input A
        {
            SCL_ERR( "[CLK]odclk close %d\n",(int)*str);
            MDrvSclHvspSetClkOnOff(stClkCfg.ptOdclk,0);
        }
        return n;
    }

    return 0;
}


static DEVICE_ATTR(clk,0644, check_clk_show, check_clk_store);
static ssize_t check_osd_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        MDrvSclHvspOsdStore(buf,E_MDRV_SCLHVSP_ID_1);
        return n;
    }

    return 0;
}
static ssize_t check_osd_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspOsdShow(buf);
}
static DEVICE_ATTR(osd,0644, check_osd_show, check_osd_store);
static ssize_t check_fbmg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    DrvSclHvspIoSetFbManageConfig_t stFbMgCfg;
    const char *str = buf;
    if(NULL!=buf)
    {
        //if(!)
        if((int)*str == 49)    //input 1
        {
            SCL_ERR( "[FB]Set %d unuse\n",(int)*str);
        }
        else if((int)*str == 50)  //input 2
        {
            SCL_ERR( "[FB]Set %d unuse\n",(int)*str);
        }
        else if((int)*str == 51)  //input 3
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_DNR_Read_ON;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 52)  //input 4
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet= E_DRV_SCLHVSP_IO_FBMG_SET_DNR_Read_OFF;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 53)  //input 5
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_DNR_Write_ON;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 54)  //input 6
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_DNR_Write_OFF;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 55)  //input 7
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_DNR_BUFFER_1;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 56)  //input 8
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_DNR_BUFFER_2;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 57)  //input 9
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_UNLOCK;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 65)  //input A
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_PRVCROP_ON;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 66)  //input B
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_PRVCROP_OFF;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }
        else if((int)*str == 69)  //input E
        {
            SCL_ERR( "[FB]Set %d\n",(int)*str);
            stFbMgCfg.enSet = E_DRV_SCLHVSP_IO_FBMG_SET_LOCK;
            MDrvSclHvspSetFbManageConfig(stFbMgCfg.enSet);
        }


        return n;
    }

    return 0;
}
static ssize_t check_fbmg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspFBMGShow(buf);
}

static DEVICE_ATTR(fbmg,0644, check_fbmg_show, check_fbmg_store);

static ssize_t check_SCIQ_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    MDrvSclHvspScIqStore(buf,E_MDRV_SCLHVSP_ID_1);
    return n;
}
static ssize_t check_SCIQ_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspScIqShow(buf,E_MDRV_SCLHVSP_ID_1);
}

static DEVICE_ATTR(SCIQ,0644, check_SCIQ_show, check_SCIQ_store);
#endif

static ssize_t check_proc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspProcShow(buf);
}
static ssize_t check_proc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        u8 u8level;
        if(((int)*(str+1))>=48)//LF :line feed
        {
            u8level = _mdrv_Scl_Changebuf2hex((int)*(str+1));
            u8level |= (_mdrv_Scl_Changebuf2hex((int)*(str))<<4);
        }
        else
        {
            u8level = _mdrv_Scl_Changebuf2hex((int)*(str));
        }
        MDrvSclHvspSetProcInst(u8level);
        SCL_ERR( "[HVSP1]Set Inst %d\n",(int)u8level);
    }
    return n;
}
static DEVICE_ATTR(proc,0644, check_proc_show, check_proc_store);
void _UTest_FdRewind(struct file *fp)
{
    SCL_DBGERR("[%s]open fp:%lx %lx\n",__FUNCTION__,(u32)fp->f_op->llseek,(u32)fp->f_op);
    fp->f_op->llseek(fp,0,SEEK_SET);
}
struct file *_UTest_OpenFile(char *path,int flag,int mode)
{
    struct file *fp=NULL;

    fp = filp_open(path, flag, mode);
    SCL_ERR("[%s]open fp:%lx\n",__FUNCTION__,(u32)fp);
    if ((s32)fp != -1)
    {
        _UTest_FdRewind(fp);
        return fp;
    }
    else
    {
        SCL_ERR("[%s]open fail\n",__FUNCTION__);
        return NULL;
    }
}
int _UTest_WriteFile(struct file *fp,char *buf,int writelen)
{
    if (fp->f_op && fp->f_op->read)
    {
        return fp->f_op->write(fp,buf,writelen, &fp->f_pos);
    }
    else
    {
        return -1;
    }
}

int _UTest_CloseFile(struct file *fp)
{
    filp_close(fp,NULL);
    return 0;
}
#if defined(SCLOS_TYPE_LINUX_TEST)
int _UTest_ReadFile(struct file *fp,char *buf,int readlen)
{
    off_t u32current;
    off_t u32end;
    u32current = fp->f_op->llseek(fp,0,SEEK_CUR);
    u32end = fp->f_op->llseek(fp,0,SEEK_END);
    if ((u32end - u32current) < readlen)
    {
        SCL_DBGERR("[%s]length err\n",__FUNCTION__);
        return -1;
    }
    else
    {
        SCL_DBGERR("[%s]u32current :%lx u32end:%lx\n",__FUNCTION__,u32current,u32end);
    }
    fp->f_op->llseek(fp,u32current,SEEK_SET);
    if (fp->f_op && fp->f_op->read)
    {
        fp->f_op->read(fp,buf,readlen, &fp->f_pos);
        return 1;
    }
    else
    {
        SCL_ERR("[%s]read err\n",__FUNCTION__);
        return -1;
    }
}

void _check_sclproc_storeNaming(u32 u32Id)
{
    sg_scl_yc_name[u32Id][0] = 48+(((u32Id&0xFFF)%1000)/100);
    sg_scl_yc_name[u32Id][1] = 48+(((u32Id&0xFFF)%100)/10);
    sg_scl_yc_name[u32Id][2] = 48+(((u32Id&0xFFF)%10));
    sg_scl_yc_name[u32Id][3] = '_';
    sg_scl_yc_name[u32Id][4] = '\0';
    DrvSclOsStrcat(sg_scl_yc_name[u32Id],KEY_DMEM_SCL_VPE_YC);
}
void _check_Ispproc_storeNaming(u32 u32Id)
{
    sg_Isp_yc_name[u32Id][0] = 48+(((u32Id&0xFFF)%1000)/100);
    sg_Isp_yc_name[u32Id][1] = 48+(((u32Id&0xFFF)%100)/10);
    sg_Isp_yc_name[u32Id][2] = 48+(((u32Id&0xFFF)%10));
    sg_Isp_yc_name[u32Id][3] = '_';
    sg_Isp_yc_name[u32Id][4] = '\0';
    DrvSclOsStrcat(sg_Isp_yc_name[u32Id],KEY_DMEM_Isp_VPE_YC);
}
void _UTest_CleanInst(void)
{
    int i;
    for(i = 0;i<TEST_INST_MAX;i++)
    {
        if(IdNum[i])
        {
            DrvSclOsDirectMemFree(sg_scl_yc_name[i],sg_scl_yc_size[i],(void*)sg_scl_yc_viraddr[i],(DrvSclOsDmemBusType_t)sg_scl_yc_addr[i]);
            MHalVpeDestroySclInstance((void *)u32Ctx[i]);
            IdNum[i] = 0;
        }
        if(IdIqNum[i])
        {
            MHalVpeDestroyIqInstance((void *)u32IqCtx[i]);
            IdIqNum[i] = 0;
        }
        if(IdIspNum[i])
        {
            DrvSclOsDirectMemFree(sg_Isp_yc_name[i],sg_Isp_yc_size[i],(void*)sg_Isp_yc_viraddr[i],(DrvSclOsDmemBusType_t)sg_Isp_yc_addr[i]);
            MHalVpeDestroyIspInstance((void *)u32IspCtx[i]);
            IdIspNum[i] = 0;
        }
    }
    if(readfp)
    {
        _UTest_CloseFile(readfp);
        readfp = NULL;
    }
    if(writefp)
    {
        _UTest_CloseFile(writefp);
        writefp = NULL;
    }
    MHalVpeDeInit();
}
bool _UTest_PutFile2Buffer(int idx,char *path,u32 size)
{
    if(!readfp)
    readfp = _UTest_OpenFile(path,O_RDONLY,0);
    if(readfp)
    {
        if(_UTest_ReadFile(readfp,(char *)sg_Isp_yc_viraddr[idx],size)==1)
        {
            DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
            DrvSclOsWaitForCpuWriteToDMem();
        }
        else
        {
            _UTest_FdRewind(readfp);
            if(_UTest_ReadFile(readfp,(char *)sg_Isp_yc_viraddr[idx],size)==1)
            {
                DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
                DrvSclOsWaitForCpuWriteToDMem();
            }
            else
            {
                SCL_ERR("Test Fail\n");
                return 0;
            }
        }
    }
    else
    {
        SCL_ERR("Test Fail\n");
        return 0;
    }
    return 1;
}
bool _UTest_PutFile2Buffer2(int idx,char *path,u32 size)
{
    if(!readfp2)
    readfp2 = _UTest_OpenFile(path,O_RDONLY,0);
    if(readfp2)
    {
        if(_UTest_ReadFile(readfp2,(char *)sg_Isp_yc_viraddr[idx],size)==1)
        {
            DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
            DrvSclOsWaitForCpuWriteToDMem();
        }
        else
        {
            _UTest_FdRewind(readfp2);
            if(_UTest_ReadFile(readfp2,(char *)sg_Isp_yc_viraddr[idx],size)==1)
            {
                DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
                DrvSclOsWaitForCpuWriteToDMem();
            }
            else
            {
                SCL_ERR("Test Fail\n");
                return 0;
            }
        }
    }
    else
    {
        SCL_ERR("Test Fail\n");
        return 0;
    }
    return 1;
}
bool _UTest_PutFile2Buffer3(int idx,char *path,u32 size)
{
    if(!readfp3)
    readfp3 = _UTest_OpenFile(path,O_RDONLY,0);
    if(readfp3)
    {
        if(_UTest_ReadFile(readfp3,(char *)sg_Isp_yc_viraddr[idx],size)==1)
        {
            DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
            DrvSclOsWaitForCpuWriteToDMem();
        }
        else
        {
            _UTest_FdRewind(readfp3);
            if(_UTest_ReadFile(readfp3,(char *)sg_Isp_yc_viraddr[idx],size)==1)
            {
                DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
                DrvSclOsWaitForCpuWriteToDMem();
            }
            else
            {
                SCL_ERR("Test Fail\n");
                return 0;
            }
        }
    }
    else
    {
        SCL_ERR("Test Fail\n");
        return 0;
    }
    return 1;
}
bool _UTest_PutFile2Buffer4(int idx,char *path,u32 size)
{
    if(!readfp4)
    readfp4 = _UTest_OpenFile(path,O_RDONLY,0);
    if(readfp4)
    {
        if(_UTest_ReadFile(readfp4,(char *)sg_Isp_yc_viraddr[idx],size)==1)
        {
            DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
            DrvSclOsWaitForCpuWriteToDMem();
        }
        else
        {
            _UTest_FdRewind(readfp4);
            if(_UTest_ReadFile(readfp4,(char *)sg_Isp_yc_viraddr[idx],size)==1)
            {
                DrvSclOsDirectMemFlush((u32)sg_Isp_yc_viraddr[idx],size);
                DrvSclOsWaitForCpuWriteToDMem();
            }
            else
            {
                SCL_ERR("Test Fail\n");
                return 0;
            }
        }
    }
    else
    {
        SCL_ERR("Test Fail\n");
        return 0;
    }
    return 1;
}
MHAL_CMDQ_CmdqInterface_t* _UTest_AllocateCMDQ(void)
{
    MHAL_CMDQ_BufDescript_t stCfg;
    MHAL_CMDQ_CmdqInterface_t *pCfg=NULL;
    stCfg.u16MloadBufSizeAlign = 32;
    stCfg.u32CmdqBufSizeAlign = 32;
    stCfg.u32CmdqBufSize = 0x40000;
    stCfg.u32MloadBufSize = 0x10000;
#if defined(SCLOS_TYPE_LINUX_TESTCMDQ)
    pCfg = MHAL_CMDQ_GetSysCmdqService(E_MHAL_CMDQ_ID_VPE,&stCfg,0);
#endif
    return pCfg;
}
ssize_t  _UTest_CreateSclInstance(int idx, u16 u16Width, u16 u16Height)
{
    int a = 0xFF;
    stMaxWin[idx].u16Height = u16Height;
    stMaxWin[idx].u16Width = u16Width;
    MHalVpeInit(NULL,NULL);
    if(!MHalVpeCreateSclInstance(NULL, &stMaxWin[idx], (void *)&u32Ctx[idx]))
    {
        SCL_ERR( "[VPE]create Fail%s\n",sg_scl_yc_name[idx]);
        return 0;
    }
    MHalVpeSclDbgLevel((void *)&a);
    MHalVpeIspDbgLevel((void *)&a);
    return 1;
}
void _UTest_AllocateMem(int idx,u32 size)
{
    _check_sclproc_storeNaming(idx);
    sg_scl_yc_size[idx] = size;
    sg_scl_yc_viraddr[idx] = DrvSclOsDirectMemAlloc(sg_scl_yc_name[idx], sg_scl_yc_size[idx],
        (DrvSclOsDmemBusType_t *)&sg_scl_yc_addr[idx]);
    DrvSclOsMemset(sg_scl_yc_viraddr[idx],0,size);
}
void _UTest_AllocateIspMem(int idx,u32 size)
{
    _check_Ispproc_storeNaming(idx);
    sg_Isp_yc_size[idx] = size;
    sg_Isp_yc_viraddr[idx] = DrvSclOsDirectMemAlloc(sg_Isp_yc_name[idx], sg_Isp_yc_size[idx],
        (DrvSclOsDmemBusType_t *)&sg_Isp_yc_addr[idx]);
    DrvSclOsMemset(sg_Isp_yc_viraddr[idx],0,size);
    SCL_ERR("[%s]vir:%lx ,phy:%lx",__FUNCTION__,(u32)sg_Isp_yc_viraddr[idx],sg_Isp_yc_addr[idx]);
}
void _UTest_SetInputCfgISPMode(int idx, u16 u16Width, u16 u16Height)
{
    stIspInputCfg[idx].eCompressMode = E_MHAL_COMPRESS_MODE_NONE;
    stIspInputCfg[idx].enInType = E_MHAL_ISP_INPUT_YUV420;
    stIspInputCfg[idx].ePixelFormat = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stIspInputCfg[idx].u32Height = u16Height;
    stIspInputCfg[idx].u32Width = u16Width;
    MHalVpeIspInputConfig((void *)u32IspCtx[idx], &stIspInputCfg[idx]);
    MHalVpeIspRotationConfig((void *)u32IspCtx[idx],&stRotCfg[idx]);
    stInputpCfg[idx].eCompressMode = E_MHAL_COMPRESS_MODE_NONE;
    stInputpCfg[idx].ePixelFormat = E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420;
    stInputpCfg[idx].u16Height = u16Height;
    stInputpCfg[idx].u16Width = u16Width;
    MHalVpeSclInputConfig((void *)u32Ctx[idx], &stInputpCfg[idx]);
}
void _UTest_SetInputCfgPatMode(int idx, u16 u16Width, u16 u16Height)
{
    stInputpCfg[idx].eCompressMode = E_MHAL_COMPRESS_MODE_NONE;
    stInputpCfg[idx].ePixelFormat = E_MHAL_PIXEL_FRAME_ARGB8888;
    stInputpCfg[idx].u16Height = u16Height;
    stInputpCfg[idx].u16Width = u16Width;
    MHalVpeSclInputConfig((void *)u32Ctx[idx], &stInputpCfg[idx]);
}
void _UTest_SetCropCfg(int idx, u16 u16Width, u16 u16Height)
{
    stCropCfg[idx].bCropEn = (idx%2) ? 1 :0;
    stCropCfg[idx].stCropWin.u16Height = (idx%2) ? MAXCROPHEIGHT(u16Height) : u16Height;
    stCropCfg[idx].stCropWin.u16Width = (idx%2) ? MAXCROPWIDTH(u16Width) : u16Width;
    stCropCfg[idx].stCropWin.u16X = (idx%2) ? MAXCROPX(u16Width) : 0;
    stCropCfg[idx].stCropWin.u16Y = (idx%2) ? MAXCROPY(u16Height) : 0;
    MHalVpeSclCropConfig((void *)u32Ctx[idx], &stCropCfg[idx]);
}
void _UTest_SetDmaCfg(int idx, MHalPixelFormat_e enColor, MHalVpeSclOutputPort_e enPort)
{
    stOutputDmaCfg[idx].enCompress = E_MHAL_COMPRESS_MODE_NONE;
    stOutputDmaCfg[idx].enOutFormat = enColor;
    stOutputDmaCfg[idx].enOutPort = enPort;
    MHalVpeSclOutputDmaConfig((void *)u32Ctx[idx], &stOutputDmaCfg[idx]);
}
void _UTest_SetOutputSizeCfg(int idx, MHalVpeSclOutputPort_e enPort, u16 u16Width, u16 u16Height)
{
    stOutputCfg[idx].enOutPort = enPort;
    stOutputCfg[idx].u16Height = u16Height;
    stOutputCfg[idx].u16Width = u16Width;
    MHalVpeSclOutputSizeConfig((void *)u32Ctx[idx], &stOutputCfg[idx]);
}
int _UTest_GetEmptyIdx(void)
{
    int idx,i;
    for(i = 0;i<TEST_INST_MAX;i++)
    {
        if(IdNum[i]==0)
        {
            IdNum[i] = 1;
            idx = i;
            break;
        }
    }
    return idx;
}
int _UTest_GetIqEmptyIdx(void)
{
    int idx,i;
    for(i = 0;i<TEST_INST_MAX;i++)
    {
        if(IdIqNum[i]==0)
        {
            IdIqNum[i] = 1;
            idx = i;
            break;
        }
    }
    return idx;
}
void _UTest_SetProcessCfg(int idx, MHalVpeSclOutputPort_e enPort, u32 u32AddrY,bool bEn,u32 u32Stride)
{
    stpBuffer[idx].stCfg[enPort].bEn = bEn;
    stpBuffer[idx].stCfg[enPort].stBufferInfo.u32Stride[0] = u32Stride;
    stpBuffer[idx].stCfg[enPort].stBufferInfo.u64PhyAddr[0] = u32AddrY;
    stpBuffer[idx].stCfg[enPort].stBufferInfo.u64PhyAddr[1] = u32AddrY+(stOutputCfg[idx].u16Width*stOutputCfg[idx].u16Height);
}
void _UTest_SetRoiCfg(int idx, MHAL_CMDQ_CmdqInterface_t *pCfg )
{
    int i;
    DrvSclOsMemset(&stHistCfg[idx],0,sizeof(MHalVpeIqWdrRoiHist_t));
    stHistCfg[idx].bEn = 1;
    stHistCfg[idx].enPipeSrc = E_MHAL_IQ_ROISRC_BEFORE_WDR;
    stHistCfg[idx].u8WinCount = 4;
    for(i=0;i<4;i++)
    {
        stHistCfg[idx].stRoiCfg[i].bEnSkip = 0;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccX[0] = 100+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccY[0] = 100+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccX[1] = 200+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccY[1] = 100+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccX[2] = 200+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccY[2] = 200+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccX[3] = 100+i*100;
        stHistCfg[idx].stRoiCfg[i].u16RoiAccY[3] = 200+i*100;
    }
    MHalVpeIqSetWdrRoiHist((void *)u32IqCtx[idx], &stHistCfg[idx]);
    MHalVpeIqSetWdrRoiMask((void *)u32IqCtx[idx],0, pCfg);
}
typedef enum
{
    SET_DEF = 0,
    SET_EG,      ///< ID_1
    SET_ES,      ///< ID_2
    SET_ES2,      ///< ID_2
    SET_CON,      ///< ID_2
    SET_UV,      ///< ID_2
    SET_NR,      ///< ID_2
    SET_all,      ///< ID_2
    SET_TYPE,      ///< ID_4
} UTest_SetIqCfg_e;
void _UTest_SetIqCfg(int idx,UTest_SetIqCfg_e enType)
{
    if(enType==SET_DEF)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
    }
    else if(enType==SET_NR)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8NRY_SF_STR = 150;
        stIqCfg[idx].u8NRY_TF_STR = 150;
        stIqCfg[idx].u8NRC_SF_STR = 150;
        stIqCfg[idx].u8NRC_TF_STR = 150;
        stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
        stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
        stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
    }
    else if(enType==SET_ES)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8NRY_SF_STR = 255;
        stIqCfg[idx].u8NRY_TF_STR = 0;
        stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
        stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
        stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
    }
    else if(enType==SET_ES2)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8NRY_SF_STR = 255;
        stIqCfg[idx].u8NRY_TF_STR = 0;
        stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
        stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
        stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 16;
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
    }
    else if(enType==SET_CON)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
        stIqCfg[idx].u8Contrast= 128;
    }
    else if(enType==SET_all)
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8NRY_SF_STR = 192;
        stIqCfg[idx].u8NRY_TF_STR = 192;
        stIqCfg[idx].u8NRC_SF_STR = 192;
        stIqCfg[idx].u8NRC_TF_STR = 192;
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
        stIqCfg[idx].u8Contrast= 128;
        stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
        stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
        stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 16;
        stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;
    }
    else
    {
        DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
        stIqCfg[idx].u8EdgeGain[0] = 128;
        stIqCfg[idx].u8EdgeGain[1] = 192;
        stIqCfg[idx].u8EdgeGain[2] = 255;
        stIqCfg[idx].u8EdgeGain[3] = 255;
        stIqCfg[idx].u8EdgeGain[4] = 255;
        stIqCfg[idx].u8EdgeGain[5] = 255;
    }
    MHalVpeIqConfig((void *)u32IqCtx[idx], &stIqCfg[idx]);
}
void _UTest_SetIqCfgYEE(int idx, u8 *data)
{

    DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
    stIqCfg[idx].u8NRY_SF_STR = 0;
    stIqCfg[idx].u8NRY_TF_STR = 0;
    stIqCfg[idx].u8NRC_SF_STR = 0;
    stIqCfg[idx].u8NRC_TF_STR = 0;
    stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
    stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
    stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 0;
    stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 0;
    stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;/**/
    stIqCfg[idx].u8EdgeGain[0] = data[0];
    stIqCfg[idx].u8EdgeGain[1] = data[1];
    stIqCfg[idx].u8EdgeGain[2] = data[2];
    stIqCfg[idx].u8EdgeGain[3] = data[3];
    stIqCfg[idx].u8EdgeGain[4] = data[4];
    stIqCfg[idx].u8EdgeGain[5] = data[5];
    SCL_ERR( "[VPE]Val %d, %d, %d, %d, %d, %d\n",data[0],data[1],data[2],data[3],data[4],data[5]);
    MHalVpeIqConfig((void *)u32IqCtx[idx], &stIqCfg[idx]);
}

void _UTest_SetIqCfgMCNR(int idx, u8 *data)
{
    DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
    stIqCfg[idx].u8NRY_SF_STR = data[0];
    stIqCfg[idx].u8NRY_TF_STR = data[1];
    stIqCfg[idx].u8NRC_SF_STR = data[2];
    stIqCfg[idx].u8NRC_TF_STR = data[3];
    stIqCfg[idx].u8NRY_BLEND_MOTION_TH = data[4]/25;
    stIqCfg[idx].u8NRY_BLEND_STILL_TH = data[5]/25;
    stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = data[6]/25;
    stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = data[7]/25;
    stIqCfg[idx].u8NRY_BLEND_STILL_WEI = data[8]/25;
    stIqCfg[idx].u8EdgeGain[0] = 0;
    stIqCfg[idx].u8EdgeGain[1] = 20;
    stIqCfg[idx].u8EdgeGain[2] = 80;
    stIqCfg[idx].u8EdgeGain[3] = 120;
    stIqCfg[idx].u8EdgeGain[4] = 160;
    stIqCfg[idx].u8EdgeGain[5] = 160;
   SCL_ERR( "[VPE]Val %d, %d, %d, %d, %d, %d, %d, %d, %d\n",data[0]/25,data[1]/25,data[2]/25,data[3]/25,data[4]/25,data[5]/25,data[6]/25,data[7]/25,data[8]/25);

    MHalVpeIqConfig((void *)u32IqCtx[idx], &stIqCfg[idx]);
}
void _UTest_SetIqCfgWDR(int idx, u8 *data)
{
    DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
    stIqCfg[idx].u8NRY_SF_STR = 0;
    stIqCfg[idx].u8NRY_TF_STR = 0;
    stIqCfg[idx].u8NRC_SF_STR = 0;
    stIqCfg[idx].u8NRC_TF_STR = 0;
    stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
    stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
    stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 0;
    stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 0;
    stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;/**/
    stIqCfg[idx].u8EdgeGain[0] = 0;
    stIqCfg[idx].u8EdgeGain[1] = 20;
    stIqCfg[idx].u8EdgeGain[2] = 80;
    stIqCfg[idx].u8EdgeGain[3] = 120;
    stIqCfg[idx].u8EdgeGain[4] = 160;
    stIqCfg[idx].u8EdgeGain[5] = 160;
    stIqCfg[idx].u8Contrast= data[0];
    SCL_ERR( "[VPE]Val %d,\n",data[0]);

    MHalVpeIqConfig((void *)u32IqCtx[idx], &stIqCfg[idx]);
}
void _UTest_SetIqCfgES(int idx, u8 *data)
{
    DrvSclOsMemset(&stIqCfg[idx],0,sizeof(MHalVpeIqConfig_t));
    stIqCfg[idx].u8NRY_SF_STR = 255;
    stIqCfg[idx].u8NRY_TF_STR = 0;
    stIqCfg[idx].u8NRY_BLEND_MOTION_TH = 0;
    stIqCfg[idx].u8NRY_BLEND_STILL_TH = 1;
    stIqCfg[idx].u8NRY_BLEND_MOTION_WEI = 16;
    stIqCfg[idx].u8NRY_BLEND_OTHER_WEI = 16;
    stIqCfg[idx].u8NRY_BLEND_STILL_WEI = 0;
    stIqCfg[idx].u8EdgeGain[0] = 128;
    stIqCfg[idx].u8EdgeGain[1] = 192;
    stIqCfg[idx].u8EdgeGain[2] = 255;
    stIqCfg[idx].u8EdgeGain[3] = 255;
    stIqCfg[idx].u8EdgeGain[4] = 255;
    stIqCfg[idx].u8EdgeGain[5] = 255;
    MHalVpeIqConfig((void *)u32IqCtx[idx], &stIqCfg[idx]);
}

void _UTest_SetIqOnOff(int idx,UTest_SetIqCfg_e enType)
{
    if(enType==SET_DEF)
    {
        DrvSclOsMemset(&stIqOnCfg[idx],0,sizeof(MHalVpeIqOnOff_t));
    }
    else if(enType==SET_EG)
    {
        stIqOnCfg[idx].bEdgeEn = 1;
    }
    else if(enType==SET_ES)
    {
        stIqOnCfg[idx].bESEn = 1;
    }
    else if(enType==SET_CON)
    {
        stIqOnCfg[idx].bContrastEn= 1;
    }
    else if(enType==SET_UV)
    {
        stIqOnCfg[idx].bUVInvert= 1;
    }
    else if(enType==SET_NR)
    {
        stIqOnCfg[idx].bNREn= 1;
    }
    else if(enType==SET_all)
    {
        stIqOnCfg[idx].bNREn= 1;
        stIqOnCfg[idx].bContrastEn= 1;
        stIqOnCfg[idx].bESEn = 1;
    }
    MHalVpeIqOnOff((void *)u32IqCtx[idx], &stIqOnCfg[idx]);
}
void _UTest_SetIqOnOff1(int idx,UTest_SetIqCfg_e enType, int en)
{
    if(enType==SET_DEF)
    {
        DrvSclOsMemset(&stIqOnCfg[idx],0,sizeof(MHalVpeIqOnOff_t));
    }
    else if(enType==SET_EG)
    {
        stIqOnCfg[idx].bEdgeEn = en;
    }
    else if(enType==SET_ES)
    {
        stIqOnCfg[idx].bESEn = en;
    }
    else if(enType==SET_CON)
    {
        stIqOnCfg[idx].bContrastEn= en;
    }
    else if(enType==SET_UV)
    {
        stIqOnCfg[idx].bUVInvert= en;
    }
    else if(enType==SET_NR)
    {
        stIqOnCfg[idx].bNREn= en;
    }
    else if(enType==SET_all)
    {
        stIqOnCfg[idx].bNREn= en;
        stIqOnCfg[idx].bContrastEn= en;
        stIqOnCfg[idx].bESEn = en;
    }
    MHalVpeIqOnOff((void *)u32IqCtx[idx], &stIqOnCfg[idx]);
}

ssize_t  _UTest_CreateIqInstance(int idx)
{
    int a = 0;
    DrvSclOsMemset(&stRoiReport[idx],0,sizeof(MHalVpeIqWdrRoiReport_t));
    MHalVpeCreateIqInstance(NULL ,(void *)&u32IqCtx[idx]);
    MHalVpeIqDbgLevel((void *)&a);
    _UTest_SetIqCfg(idx,SET_DEF);
    _UTest_SetIqOnOff(idx,SET_DEF);
    return 1;
}
void _UTest_SetLevelSize(u8 u8level,u16 *u16wsize ,u16 *u16hsize)
{
    if(u8level == 0)
    {
        *u16wsize = FHDOUTPUTWIDTH;
        *u16hsize = FHDOUTPUTHEIGHT;
    }
    else if(u8level == 1)
    {
        *u16wsize = XGAOUTPUTWIDTH;
        *u16hsize = XGAOUTPUTHEIGHT;
    }
    else if(u8level == 2)
    {
        *u16wsize = HDOUTPUTWIDTH;
        *u16hsize = HDOUTPUTHEIGHT;
    }
    else if(u8level == 3)
    {
        *u16wsize = VGAOUTPUTWIDTH;
        *u16hsize = VGAOUTPUTHEIGHT;
    }
    else if(u8level == 4)
    {
        *u16wsize = CIFOUTPUTWIDTH;
        *u16hsize = CIFOUTPUTHEIGHT;
    }
    else if(u8level == 5)
    {
        *u16wsize = QXGAOUTPUTWIDTH;
        *u16hsize = QXGAOUTPUTHEIGHT;
    }
    else if(u8level == 6)
    {
        *u16wsize = WQHDOUTPUTWIDTH;
        *u16hsize = WQHDOUTPUTHEIGHT;
    }
    else if(u8level == 7)
    {
        *u16wsize = QSXGAOUTPUTWIDTH;
        *u16hsize = QSXGAOUTPUTHEIGHT;
    }
    else if(u8level == 8)
    {
        *u16wsize = QFHDOUTPUTWIDTH8;
        *u16hsize = QFHDOUTPUTHEIGHT8;
    }
    else if(u8level == 9)
    {
        *u16wsize = FKUOUTPUTWIDTH;
        *u16hsize = FKUOUTPUTHEIGHT;
    }
    else
    {
        *u16wsize = FHDOUTPUTWIDTH;
        *u16hsize = FHDOUTPUTHEIGHT;
    }
}
void _UTest_SetLevelInSize(u8 u8level,u16 *u16wsize ,u16 *u16hsize)
{
    if(u8level == 0)
    {
        *u16wsize = FHDOUTPUTWIDTH;
        *u16hsize = FHDOUTPUTHEIGHT;
    }
    else if(u8level == 1)
    {
        *u16wsize = ISPINPUTWIDTH;
        *u16hsize = ISPINPUTHEIGHT;
    }
    else if(u8level == 2)
    {
        *u16wsize = HDOUTPUTWIDTH;
        *u16hsize = HDOUTPUTHEIGHT;
    }
    else if(u8level == 3)
    {
        *u16wsize = VGAOUTPUTWIDTH;
        *u16hsize = VGAOUTPUTHEIGHT;
    }
    else if(u8level == 4)
    {
        *u16wsize = CIFOUTPUTWIDTH;
        *u16hsize = CIFOUTPUTHEIGHT;
    }
    else if(u8level == 5)
    {
        *u16wsize = QXGAOUTPUTWIDTH;
        *u16hsize = QXGAOUTPUTHEIGHT;
    }
    else if(u8level == 6)
    {
        *u16wsize = WQHDOUTPUTWIDTH;
        *u16hsize = WQHDOUTPUTHEIGHT;
    }
    else if(u8level == 7)
    {
        *u16wsize = QSXGAOUTPUTWIDTH;
        *u16hsize = QSXGAOUTPUTHEIGHT;
    }
    else if(u8level == 8)
    {
        *u16wsize = QFHDOUTPUTWIDTH8;
        *u16hsize = QFHDOUTPUTHEIGHT8;
    }
    else if(u8level == 9)
    {
        *u16wsize = FKUOUTPUTWIDTH;
        *u16hsize = FKUOUTPUTHEIGHT;
    }
    else
    {
        *u16wsize = FHDOUTPUTWIDTH;
        *u16hsize = FHDOUTPUTHEIGHT;
    }
}
MHalVpeWaitDoneType_e _UTest_GetWaitFlag(u8 u8level)
{
    MHalVpeWaitDoneType_e enType;
    if(u8level ==3)
    {
        enType = E_MHAL_VPE_WAITDONE_MDWINONLY;
    }
    else if(u8level ==8||u8level ==9)
    {
        enType = E_MHAL_VPE_WAITDONE_DMAANDMDWIN;
    }
    else
    {
        enType = E_MHAL_VPE_WAITDONE_DMAONLY;
    }
    return enType;
}
void UT_multiport_multisize(u8 u8level,u8 u8Count)
{
    u32 u32Addr;
    u32 u32IspinH;
    u32 u32IspinV;
    u32 u32ScOutH;
    u32 u32ScOutV;
    int idx;
    int i;
    u32 u32CMDQIrq;
    _UTest_CleanInst();
    if(pCmdqCfg==NULL)
    {
        pCmdqCfg = _UTest_AllocateCMDQ();
    }
    writefp = _UTest_OpenFile("/tmp/Scout_ch0.bin",O_WRONLY|O_CREAT,0777);
    writefp2 = _UTest_OpenFile("/tmp/Scout_ch1.bin",O_WRONLY|O_CREAT,0777);
    if(u8level>2)
    {
        writefp3 = _UTest_OpenFile("/tmp/Scout_ch2.bin",O_WRONLY|O_CREAT,0777);
    }
    if(u8level>3)
    {
        writefp4 = _UTest_OpenFile("/tmp/Scout_ch3.bin",O_WRONLY|O_CREAT,0777);
    }
    for(idx=0;idx<u8level;idx++)
    {
        u32IspinH = (idx ==0) ? CH0INWIDTH : (idx ==1) ? CH1INWIDTH : (idx ==2) ? CH2INWIDTH : CH3INWIDTH;
        u32IspinV = (idx ==0) ? CH0INHEIGHT : (idx ==1) ? CH1INHEIGHT: (idx ==2) ? CH2INHEIGHT : CH3INHEIGHT;
        u32ScOutH = (idx ==0) ? CH0OUTWIDTH : (idx ==1) ? CH1OUTWIDTH :(idx ==2) ? CH2OUTWIDTH : CH3OUTWIDTH;
        u32ScOutV = (idx ==0) ? CH0OUTHEIGHT: (idx ==1) ? CH1OUTHEIGHT: (idx ==2) ? CH2OUTHEIGHT : CH3OUTHEIGHT;
        _UTest_CreateSclInstance(idx,u32IspinH,u32IspinV);
        IdNum[idx] = 1;
        MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
        IdIspNum[idx] = 1;
        _UTest_CreateIqInstance(idx);
        IdIqNum[idx] = 1;
        _UTest_SetIqCfg(idx,SET_all);
        _UTest_SetIqOnOff(idx,SET_all);
        _UTest_AllocateMem(idx,(u32)u32ScOutH*u32ScOutV*2);
        _UTest_AllocateIspMem(idx,(u32)u32IspinH*u32IspinV*3/2);
        u32Addr = sg_scl_yc_addr[idx];
        if(idx ==0)
        {
            if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin_ch0.yuv",(u32)(u32IspinH*u32IspinV*3/2)))
            {
                return ;
            }
        }
        else if(idx ==1)
        {
            if(!_UTest_PutFile2Buffer2(idx,"/tmp/Ispin_ch1.yuv",(u32)(u32IspinH*u32IspinV*3/2)))
            {
                return ;
            }
        }
        else if(idx ==2)
        {
            if(!_UTest_PutFile2Buffer3(idx,"/tmp/Ispin_ch2.yuv",(u32)(u32IspinH*u32IspinV*3/2)))
            {
                return ;
            }
        }
        else if(idx ==3)
        {
            if(!_UTest_PutFile2Buffer4(idx,"/tmp/Ispin_ch3.yuv",(u32)(u32IspinH*u32IspinV*3/2)))
            {
                return ;
            }
        }
        if(sg_scl_yc_viraddr[idx])
        {
            _UTest_SetInputCfgISPMode(idx, u32IspinH, u32IspinV);
            _UTest_SetCropCfg(idx, u32IspinH, u32IspinV);
            _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
            _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
            _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
            _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
            _UTest_SetOutputSizeCfg(idx,idx,u32ScOutH, u32ScOutV);
            MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
            //  1,2,3,4,5,6,  8,9,10,11,12,13,  , 15    =13
            _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,
                u32Addr,(idx==0) ? 1 : 0,u32ScOutH*2);
            // 0, 2,  4,   6,  8    10,    12,    14        = 8
            _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,
                u32Addr,(idx==1) ? 1 : 0,u32ScOutH*2);
            //  1,2  ,4,5,  7,8,   10,11,    13,14,15   =11
            _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,
                u32Addr,(idx==2) ? 1 : 0,u32ScOutH*2);
            _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,
                u32Addr,(idx==3) ? 1 : 0,u32ScOutH*2);
            MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
            stVdinfo[idx].u32Stride[0] = u32IspinH;
            stVdinfo[idx].u32Stride[1] = u32IspinH;
            stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[idx];
            stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[idx]+((u32)(u32IspinH*u32IspinV));
            MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
            MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,
                (idx==3) ? E_MHAL_VPE_WAITDONE_MDWINONLY : E_MHAL_VPE_WAITDONE_DMAONLY);
            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
        }
    }
    if(pCmdqCfg)
    {
        do
        {
            DrvSclOsDelayTask(5);
            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
        }
        while(!(u32CMDQIrq&0x800));
    }
    DrvSclOsDelayTask(1000);
    _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],CH0OUTWIDTH*CH0OUTHEIGHT*2);
    _UTest_WriteFile(writefp2,(char *)sg_scl_yc_viraddr[1],CH1OUTWIDTH*CH1OUTHEIGHT*2);
    if(writefp3 && sg_scl_yc_viraddr[2])
    {
        _UTest_WriteFile(writefp3,(char *)sg_scl_yc_viraddr[2],CH2OUTWIDTH*CH2OUTHEIGHT*2);
    }
    if(writefp4 && sg_scl_yc_viraddr[3])
    {
        _UTest_WriteFile(writefp4,(char *)sg_scl_yc_viraddr[3],CH3OUTWIDTH*CH3OUTHEIGHT*2);
    }
    if(u8Count>1)
    {
        for(i=1;i<u8Count;i++)
        {
            for(idx=0;idx<4;idx++)
            {
                if(idx ==0)
                {
                    if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin_ch0.yuv",(u32)(CH0INWIDTH*CH0INHEIGHT*3/2)))
                    {
                        return ;
                    }
                }
                else if(idx ==1)
                {
                    if(!_UTest_PutFile2Buffer2(idx,"/tmp/Ispin_ch1.yuv",(u32)(CH1INWIDTH*CH1INHEIGHT*3/2)))
                    {
                        return ;
                    }
                }
                else if(idx ==2)
                {
                    if(!_UTest_PutFile2Buffer3(idx,"/tmp/Ispin_ch2.yuv",(u32)(CH2INWIDTH*CH2INHEIGHT*3/2)))
                    {
                        return ;
                    }
                }
                else if(idx ==3)
                {
                    if(!_UTest_PutFile2Buffer4(idx,"/tmp/Ispin_ch3.yuv",(u32)(CH3INWIDTH*CH3INHEIGHT*3/2)))
                    {
                        return ;
                    }
                }
                MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,
                    (idx==3) ? E_MHAL_VPE_WAITDONE_MDWINONLY : E_MHAL_VPE_WAITDONE_DMAONLY);
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                if(pCmdqCfg)
                {
                    do
                    {
                        DrvSclOsDelayTask(5);
                        pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                    }
                    while(!(u32CMDQIrq&0x800));
                }
            }
            DrvSclOsDelayTask(1000);
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],CH0OUTWIDTH*CH0OUTHEIGHT*2);
            _UTest_WriteFile(writefp2,(char *)sg_scl_yc_viraddr[1],CH1OUTWIDTH*CH1OUTHEIGHT*2);
            if(writefp3 && sg_scl_yc_viraddr[2])
            {
                _UTest_WriteFile(writefp3,(char *)sg_scl_yc_viraddr[2],CH2OUTWIDTH*CH2OUTHEIGHT*2);
            }
            if(writefp4 && sg_scl_yc_viraddr[3])
            {
                _UTest_WriteFile(writefp4,(char *)sg_scl_yc_viraddr[3],CH3OUTWIDTH*CH3OUTHEIGHT*2);
            }
        }
    }
}
void UT_DispTask(void *arg)
{
    u32 u32CMDQIrq;
    while(gstUtDispTaskCfg.bEn)
    {
        _UTest_PutFile2Buffer(gstUtDispTaskCfg.u8level,"/tmp/Ispin.yuv",(u32)gstUtDispTaskCfg.u32FrameSize);
        MHalVpeIqRead3DNRTbl((void *)u32IqCtx[gstUtDispTaskCfg.u8level]);
        MHalVpeIqProcess((void *)u32IqCtx[gstUtDispTaskCfg.u8level], pCmdqCfg);
        MHalVpeSclProcess((void *)u32Ctx[gstUtDispTaskCfg.u8level],pCmdqCfg, &stpBuffer[gstUtDispTaskCfg.u8level]);
        MHalVpeIspProcess((void *)u32IspCtx[gstUtDispTaskCfg.u8level],pCmdqCfg, &stVdinfo[gstUtDispTaskCfg.u8level]);
        MHalVpeSclSetWaitDone((void *)u32Ctx[gstUtDispTaskCfg.u8level],pCmdqCfg,
            _UTest_GetWaitFlag(u8glevel[gstUtDispTaskCfg.u8level]));
        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
        if(pCmdqCfg)
        {
            do
            {
                DrvSclOsDelayTask(5);
                pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
            }
            while(!(u32CMDQIrq&0x800));
        }
        DrvSclOsDelayTask(1000);
    }
}
void _UTest_Create_Task(u8 u8level,u32 u32Framesize,TaskEntry pf)
{
    gstUtDispTaskCfg.bEn = 1;
    gstUtDispTaskCfg.u8level = u8level;
    gstUtDispTaskCfg.u32FrameSize = u32Framesize;
    gstUtDispTaskCfg.Id  = DrvSclOsCreateTask(pf,(MS_U32)NULL,TRUE,(char*)"UTDISPTask");
    if (gstUtDispTaskCfg.Id == -1)
    {
        SCL_ERR("[UT]%s create task fail\n", __FUNCTION__);
        return;
    }
}
void _UTest_Destroy_Task(void)
{
    if (gstUtDispTaskCfg.bEn)
    {
        gstUtDispTaskCfg.bEn = 0;
        DrvSclOsDeleteTask(gstUtDispTaskCfg.Id);
    }
}
void _UTestClosePatternGen(MHAL_CMDQ_CmdqInterface_t *pCmdqinf)
{
    pCmdqinf->MHAL_CMDQ_WriteRegCmdqMask(pCmdqinf,0x152580,0,0x8000);
    pCmdqinf->MHAL_CMDQ_WriteRegCmdqMask(pCmdqinf,0x1525E0,0,0x1);
}
static ssize_t check_test_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    const char *str = buf;
    int i;
    int idx;
    u8 u8level;
    u32 u32Time;
    u32 u32CMDQIrq;
    static u8 u8Count = 1;
    static u8 u8Rot = 0;
    static u16 u16swsize = FHDOUTPUTWIDTH;
    static u16 u16shsize = FHDOUTPUTHEIGHT;
    static u16 u16IspInwsize = ISPINPUTWIDTH;
    static u16 u16IspInhsize = ISPINPUTHEIGHT;
    static u16 u16PatInwsize = MAXINPUTWIDTH;
    static u16 u16PatInhsize = MAXINPUTHEIGHT;
    static u8 u8Str[10];

    if(NULL!=buf)
    {
        u8level = _mdrv_Scl_Changebuf2hex((int)*(str+1));
        u32Time = ((u32)DrvSclOsGetSystemTimeStamp());
        u8Str[0] = u8Str[0];

#if VPE_IQAPI_TEST
        i=0;
        u8Rot=0;
        u32CMDQIrq=0;
        u16PatInwsize = MAXINPUTWIDTH;
        u16PatInhsize = MAXINPUTHEIGHT;

        if((int)*str >= 48 && (int)*str <= 57)    //Test 0~9
        {
            u8Str[u8Count-1] = ((int)*str - 48)*25;
            SCL_ERR( "[VPE]Set [%d]:%d\n",u8Count-1, u8Str[u8Count-1]);
            u8Count++;
        }
        else if((int)*str == 'C')  //Test
        {
            u8Count=1;
        }
        else if((int)*str == 'E')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgYEE(idx,u8Str);
                _UTest_SetIqOnOff(idx,SET_EG);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'S')  //Test S
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgES(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_ES,1);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 's')  //Test S
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgES(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_ES,0);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'N')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgMCNR(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_NR,1);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'n')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgMCNR(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_NR,0);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'W')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgWDR(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_CON,1);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
         else if((int)*str == 'w')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfgWDR(idx,u8Str);
                _UTest_SetIqOnOff1(idx,SET_CON,0);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'U')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_UV);
                _UTest_SetIqOnOff(idx,SET_UV);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'A')  //Test
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_all);
                _UTest_SetIqOnOff(idx,SET_all);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }

        else if((int)*str == 'D')  //Test D
        {
            idx = _UTest_GetEmptyIdx();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
            MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
            IdIspNum[idx] = 1;
            _UTest_CreateIqInstance(idx);
            IdIqNum[idx] = 1;
            MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
//            _UTest_SetIqCfg(idx,SET_all);
//            _UTest_SetIqOnOff(idx,SET_all);
            _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2);
            _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);
            if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
            {
                return 0;
            }
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                MHalVpeIqProcess((void *)u32IqCtx[idx],pCmdqCfg);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8) ? 1 :0,u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                stVdinfo[idx].u32Stride[1] = u16IspInwsize;
                stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[idx];
                stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[idx]+((u32)(u16IspInwsize*u16IspInhsize));
                MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                u8glevel[idx] = u8level;
            }
            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestD_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }
        else if((int)*str == 'd')  //Test d
        {
            if(IdIspNum[u8level])
            {
                if(!_UTest_PutFile2Buffer(u8level,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                {
                    return 0;
                }
                MHalVpeIqRead3DNRTbl((void *)u32IqCtx[u8level]);
                MHalVpeIqProcess((void *)u32IqCtx[u8level], pCmdqCfg);
                MHalVpeSclProcess((void *)u32Ctx[u8level],pCmdqCfg, &stpBuffer[u8level]);
                MHalVpeIspProcess((void *)u32IspCtx[u8level],pCmdqCfg, &stVdinfo[u8level]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[u8level],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[u8level]));
                SCL_ERR( "[VPE]Set MHAL_CMDQ_KickOffCmdq\n");
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                if(pCmdqCfg)
                {
                    do
                    {
                        DrvSclOsDelayTask(5);
                        pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                    }
                    while(!(u32CMDQIrq&0x800));
                }
                SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Testd_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                    stOutputCfg[u8level].u16Width,stOutputCfg[u8level].u16Height,u8level,(u16)(u32Time&0xFFFF)
                    ,sg_scl_yc_addr[u8level]+MIU0_BASE,(u32)(stOutputCfg[u8level].u16Width*stOutputCfg[u8level].u16Height*2));
            }
        }

#else
        if((int)*str == 48)    //Test 0
        {
            _UTest_CleanInst();
            SCL_ERR( "[VPE]Set %d disable all test\n",(int)*str);
        }
        if((int)*str == 49)    //Test 1
        {
            for(i = 0;i<TEST_INST_MAX;i++)
            {
                idx = _UTest_GetEmptyIdx();
                _UTest_CreateSclInstance(idx,MAXOUTPUTportH,MAXOUTPUTportV);
            }
            //_UTest_AllocateMem(idx,MAXINPUTWIDTH*MAXINPUTHEIGHT*2);
            SCL_ERR( "[VPE]Set %d create 1 inst Debug level on %s\n",(int)*str,sg_scl_yc_name[idx]);
        }
        else if((int)*str == 50)  //Test 2
        {
            for(i = 0;i<TEST_INST_MAX;i++)
            {
                if(IdNum[i])
                {
                    _UTest_SetInputCfgPatMode(i, u16PatInwsize, u16PatInhsize);
                    _UTest_SetCropCfg(i, u16PatInwsize, u16PatInhsize);
                }
            }
            SCL_ERR( "[VPE]Set %d Set all input/crop\n",(int)*str);
        }
        else if((int)*str == 51)  //Test 3
        {
            MHalPixelFormat_e enType;
            for(i = 0;i<TEST_INST_MAX;i++)
            {
                if(IdNum[i])
                {
                    enType = (u8level == 1) ? E_MHAL_PIXEL_FRAME_YUV_SEMIPLANAR_420: E_MHAL_PIXEL_FRAME_YUV422_YUYV;
                    _UTest_SetDmaCfg(i,enType,E_MHAL_SCL_OUTPUT_PORT0);
                    _UTest_SetDmaCfg(i,enType,E_MHAL_SCL_OUTPUT_PORT1);
                    _UTest_SetDmaCfg(i,enType,E_MHAL_SCL_OUTPUT_PORT2);
                    _UTest_SetDmaCfg(i,enType,E_MHAL_SCL_OUTPUT_PORT3);
                }
            }
            SCL_ERR( "[VPE]Set %d Set all Dma config\n",(int)*str);
        }
        else if((int)*str == 52)  //Test 4
        {
            _UTest_SetLevelSize(u8level,&u16swsize,&u16shsize);
            for(i = 0;i<TEST_INST_MAX;i++)
            {
                if(IdNum[i])
                {
                    _UTest_SetOutputSizeCfg(i,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(i,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(i,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(i,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                }
            }
            SCL_ERR( "[VPE]Set %d Set all Dma output size %hu/%hu\n",(int)*str,u16swsize,u16shsize);

        }
        else if((int)*str == 53)  //Test 5
        {
            for(i = 0;i<TEST_INST_MAX;i++)
            {
                if(IdNum[i])
                {
                    _UTest_SetProcessCfg(i,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[i],
                        (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7) ? 1 :0,0);
                    _UTest_SetProcessCfg(i,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[i],
                        (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,0);
                    _UTest_SetProcessCfg(i,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[i],
                        (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,0);
                    _UTest_SetProcessCfg(i,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[i],0,stOutputCfg[i].u16Width*2);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[i],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[i]));
                    MHalVpeSclProcess((void *)u32Ctx[i],NULL, &stpBuffer[i]);
                    SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Test5_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                        stOutputCfg[i].u16Width,stOutputCfg[i].u16Height,i,(u16)(u32Time&0xFFFF)
                        ,sg_scl_yc_addr[i]+MIU0_BASE,(u32)(stOutputCfg[i].u16Width*stOutputCfg[i].u16Height*2));
                }
            }
            SCL_ERR( "[VPE]Set %d Set all Dma Process\n",(int)*str);
        }
        else if((int)*str == 54)  //Test 6
        {
            if(IdNum[u8level])
            {
                _UTest_SetProcessCfg(u8level,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[u8level],(u8level%3 ==2) ? 1 :0,0);
                _UTest_SetProcessCfg(u8level,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[u8level],(u8level%3 ==1) ? 1 :0,0);
                _UTest_SetProcessCfg(u8level,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[u8level],(u8level%3 ==0) ? 1 :0,0);
                _UTest_SetProcessCfg(u8level,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[u8level],0,stOutputCfg[u8level].u16Width*2);
                MHalVpeSclSetWaitDone((void *)u32Ctx[u8level],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[u8level]));
                MHalVpeSclProcess((void *)u32Ctx[u8level],NULL, &stpBuffer[u8level]);
                SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Test6_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                    stOutputCfg[u8level].u16Width,stOutputCfg[u8level].u16Height,u8level,(u16)(u32Time&0xFFFF)
                    ,sg_scl_yc_addr[u8level]+MIU0_BASE,(u32)(stOutputCfg[u8level].u16Width*stOutputCfg[u8level].u16Height*2));
            }
            SCL_ERR( "[VPE]Set %d Set for level:%hhu Dma Process\n",(int)*str,u8level);
        }
        else if((int)*str == 55)//Test 7
        {
            _UTest_SetLevelSize(u8level,&u16swsize,&u16shsize);
            SCL_ERR("[VPE]Set OutPutSize:(%hu,%hu) \n",u16swsize,u16shsize);
        }
        else if((int)*str == 56)//Test 8
        {
            _UTest_SetLevelInSize(u8level,&u16PatInwsize,&u16PatInhsize);
            SCL_ERR("[VPE]Set Pat InPutSize:(%hu,%hu) \n",u16PatInwsize,u16PatInhsize);
        }
        else if((int)*str == 57)//Test9
        {
            _UTest_SetLevelInSize(u8level,&u16IspInwsize,&u16IspInhsize);
            SCL_ERR("[VPE]Set ISP InPutSize:(%hu,%hu) \n",u16IspInwsize,u16IspInhsize);
        }
        else if((int)*str == 65)  //Test A
        {
            idx = _UTest_GetEmptyIdx();
            //MHAL_CMDQ_GetSysCmdqService()
            _UTest_CreateSclInstance(idx,u16PatInwsize,u16PatInhsize);
            _UTest_AllocateMem(idx,(u32)(u16swsize*u16shsize*2));
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgPatMode(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetCropCfg(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8),u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],NULL, &stpBuffer[idx]);
                u8glevel[idx] = u8level;
            }
            MHalVpeSclSetWaitDone((void *)u32Ctx[idx],NULL,_UTest_GetWaitFlag(u8level));
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[idx],u16swsize*u16shsize*2);
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestA_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }
        else if((int)*str == 'a')  //Test a
        {
            idx = _UTest_GetEmptyIdx();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            _UTest_CreateSclInstance(idx,u16PatInwsize,u16PatInhsize);
            _UTest_AllocateMem(idx,(u32)(u16swsize*u16shsize*2));
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgPatMode(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetCropCfg(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8),u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                u8glevel[idx] = u8level;
            }
            if(pCmdqCfg)
            {
                _UTestClosePatternGen(pCmdqCfg);
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                do
                {
                    DrvSclOsDelayTask(5);
                    pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                }
                while(!(u32CMDQIrq&0x800));
            }
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[idx],u16swsize*u16shsize*2);
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Testa_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }
        else if((int)*str == 66)  //Test B
        {
            idx = _UTest_GetEmptyIdx();
            _UTest_CreateSclInstance(idx,u16PatInwsize,u16PatInhsize);
            _UTest_AllocateMem(idx,(u32)(u16swsize*u16shsize*2*4));
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgPatMode(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetCropCfg(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],1,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx]+(u32)u16swsize*u16shsize*2,1,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx]+(u32)u16swsize*u16shsize*2*2,1,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx]+(u32)u16swsize*u16shsize*2*3,1,u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                u8glevel[idx] = 9;
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
            }
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestB_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2*4));
        }
        else if((int)*str == 'b')  //Test b
        {
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            idx = _UTest_GetEmptyIdx();
            _UTest_CreateSclInstance(idx,u16PatInwsize,u16PatInhsize);
            _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2);
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgPatMode(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetCropCfg(idx, u16PatInwsize, u16PatInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8|| u8level ==9) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7|| u8level ==9) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7|| u8level ==9) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8|| u8level ==9),u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],0,u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                u8glevel[idx] = u8level;
                if(pCmdqCfg)
                {
                    _UTestClosePatternGen(pCmdqCfg);
                    pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                    do
                    {
                        DrvSclOsDelayTask(5);
                        pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                    }
                    while(!(u32CMDQIrq&0x800));
                }
                writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
                _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[idx],u16swsize*u16shsize*2);
            }
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Testb_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }
        else if((int)*str == 67)  //Test C
        {
            idx = _UTest_GetIqEmptyIdx();
            _UTest_CreateIqInstance(idx);
            _UTest_SetRoiCfg(idx,pCmdqCfg);
            MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
            MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
            MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
            MHalVpeIqGetWdrRoiHist((void *)u32IqCtx[idx], &stRoiReport[idx]);
            SCL_ERR("[VPE]stRoiReport %u %u %u %u\n",
                stRoiReport[idx].u32Y[0],stRoiReport[idx].u32Y[1],stRoiReport[idx].u32Y[2],stRoiReport[idx].u32Y[3]);

            SCL_ERR( "[VPE]Set IQ inst Direct%d\n",(int)*str);
        }
        else if((int)*str == 'D')  //Test D
        {
            idx = _UTest_GetEmptyIdx();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
            MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
            IdIspNum[idx] = 1;
            _UTest_CreateIqInstance(idx);
            IdIqNum[idx] = 1;
            MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
            _UTest_SetIqCfg(idx,SET_all);
            _UTest_SetIqOnOff(idx,SET_all);
            _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2);
            _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);
            if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
            {
                return 0;
            }
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                MHalVpeIqProcess((void *)u32IqCtx[idx],pCmdqCfg);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8) ? 1 :0,u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                stVdinfo[idx].u32Stride[1] = u16IspInwsize;
                stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[idx];
                stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[idx]+((u32)(u16IspInwsize*u16IspInhsize));
                MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                u8glevel[idx] = u8level;
            }
            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestD_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }
        else if((int)*str == 'd')  //Test d
        {
            if(IdIspNum[u8level])
            {
                if(!_UTest_PutFile2Buffer(u8level,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                {
                    return 0;
                }
                MHalVpeIqRead3DNRTbl((void *)u32IqCtx[u8level]);
                MHalVpeIqProcess((void *)u32IqCtx[u8level], pCmdqCfg);
                MHalVpeSclProcess((void *)u32Ctx[u8level],pCmdqCfg, &stpBuffer[u8level]);
                MHalVpeIspProcess((void *)u32IspCtx[u8level],pCmdqCfg, &stVdinfo[u8level]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[u8level],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[u8level]));
                SCL_ERR( "[VPE]Set MHAL_CMDQ_KickOffCmdq\n");
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                if(pCmdqCfg)
                {
                    do
                    {
                        DrvSclOsDelayTask(5);
                        pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                    }
                    while(!(u32CMDQIrq&0x800));
                }
                SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Testd_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                    stOutputCfg[u8level].u16Width,stOutputCfg[u8level].u16Height,u8level,(u16)(u32Time&0xFFFF)
                    ,sg_scl_yc_addr[u8level]+MIU0_BASE,(u32)(stOutputCfg[u8level].u16Width*stOutputCfg[u8level].u16Height*2));
            }
        }
        else if((int)*str == 69)  //Test E
        {
            if(IdIqNum[u8level])
            {
                MHalVpeIqRead3DNRTbl((void *)u32IqCtx[u8level]);
                SCL_ERR("[VPE]get DNR report \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'e')  //Test e
        {
            if(IdIqNum[u8level])
            {
                MHalVpeIqGetWdrRoiHist((void *)u32IqCtx[u8level], &stRoiReport[u8level]);
                SCL_ERR("[VPE]stRoiReport %u %u %u %u\n",
                    stRoiReport[u8level].u32Y[0],stRoiReport[u8level].u32Y[1],stRoiReport[u8level].u32Y[2],stRoiReport[u8level].u32Y[3]);
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'F')  //Test F
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_DEF);
                _UTest_SetIqOnOff(idx,SET_DEF);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Default \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'G')  //Test G
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_EG);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Edge \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'g')  //Test g
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqOnOff(idx,SET_EG);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set ONOff Edge \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'H')  //Test H
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_ES);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Es \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'f')  //Test f
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_ES2);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config Es2 \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'h')  //Test h
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqOnOff(idx,SET_ES);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set ONOff Es \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'I')  //Test I
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_CON);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config contrast \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'i')  //Test i
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqOnOff(idx,SET_CON);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set ONOff contrast \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'J')  //Test J
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_UV);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config uv \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'j')  //Test j
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqOnOff(idx,SET_UV);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set ONOff uv \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'N')  //Test N
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqCfg(idx,SET_NR);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set Config uv \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'n')  //Test n
        {
            if(IdIqNum[u8level])
            {
                idx = u8level;
                _UTest_SetIqOnOff(idx,SET_NR);
                MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                SCL_ERR("[VPE]Set ONOff uv \n");
            }
            SCL_ERR( "[VPE]Set %d\n",(int)*str);
        }
        else if((int)*str == 'P')  //Test P
        {
            u8Count = u8level;
            SCL_ERR("[VPE]Set Write count:%hhu \n",u8Count);
        }
        else if((int)*str == 'p')  //Test p
        {
            idx = _UTest_GetEmptyIdx();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
            MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
            IdIspNum[idx] = 1;
            _UTest_CreateIqInstance(idx);
            IdIqNum[idx] = 1;
            MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
            _UTest_SetIqCfg(idx,SET_NR);
            _UTest_SetIqOnOff(idx,SET_NR);
            _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2*3);
            _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            if(sg_scl_yc_viraddr[idx])
            {
                _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx]+(u32)u16swsize*u16shsize*2,
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx]+(u32)u16swsize*u16shsize*4,
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    0,u16swsize*2);
                u8glevel[idx] = u8level;
                for(i=0;i<u8Count;i++)
                {
                    if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                    {
                        return 0;
                    }
                    MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                    MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                    MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                    stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                    stVdinfo[idx].u32Stride[1] = u16IspInhsize;
                    stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[idx];
                    stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[idx]+((u32)(u16IspInwsize*u16IspInhsize));
                    MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                    if(pCmdqCfg)
                    {
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                    }
                    _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[idx],(u32)u16swsize*u16shsize*2*3);
                }
                SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\Testp_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                    u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                    ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2*3));
            }
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
        }
        else if((int)*str == 'R')  //Test R
        {
            if(IdIspNum[u8level])
            {
                stRotCfg[u8level].enRotType = u8Rot;
                MHalVpeIspRotationConfig((void *)u32IspCtx[u8level],&stRotCfg[u8level]);
            }
            SCL_ERR("[VPE]Set Write level Rot:%hhu \n",u8Rot);
        }
        else if((int)*str == 'r')  //Test r
        {
            u8Rot = u8level;
            SCL_ERR("[VPE]Set Write Rot:%hhu \n",u8Rot);
        }
        else if((int)*str == 'S')  //Test S
        {
            //disable all
            _UTest_CleanInst();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            for(idx=0;idx<16;idx++)
            {
                _UTest_CreateSclInstance(idx,MAXSTRIDEINPUTWIDTH,MAXSTRIDEINPUTHEIGHT);
                IdNum[idx] = 1;
                _UTest_CreateIqInstance(idx);
                IdIqNum[idx] = 1;
                MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
                if(idx==0)
                {
                    _UTest_AllocateMem(idx,(u32)MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152580,0,0x8000);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x1525E0,0,0x1);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,1,0x1);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,0,0x1);
                }
                else
                {
                    if(pCmdqCfg)
                    {
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                    }
                }
                if(sg_scl_yc_viraddr[0])
                {
                    _UTest_SetInputCfgPatMode(idx, MAXSTRIDEINPUTWIDTH, MAXSTRIDEINPUTHEIGHT);
                    _UTest_SetCropCfg(idx, MAXSTRIDEINPUTWIDTH, MAXSTRIDEINPUTHEIGHT);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetIqCfg(idx,SET_NR);
                    _UTest_SetIqOnOff(idx,SET_NR);
                    MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==0)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==1)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==2)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==3)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                    u8glevel[idx] = u8level;
                }
            }
            if(pCmdqCfg)
            {
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                do
                {
                    DrvSclOsDelayTask(5);
                    pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                }
                while(!(u32CMDQIrq&0x800));
            }
            DrvSclOsDelayTask(1000);
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
            if(u8Count>1)
            {
                for(i=1;i<u8Count;i++)
                {
                    for(idx=0;idx<16;idx++)
                    {
                        MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                        MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                        MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                    }
                    if(pCmdqCfg)
                    {
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                    }
                    _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
                }
            }
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestS_%dx%d_16chin1_%hx.bin a:0x%lx++0x%lx \n",
                MAXOUTPUTSTRIDEH,MAXOUTPUTSTRIDEV,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[0]+MIU0_BASE,(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2));
        }
        else if((int)*str == 's')  //Test s
        {
            //disable all
            _UTest_CleanInst();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            for(idx=0;idx<16;idx++)
            {
                _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
                IdNum[idx] = 1;
                MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
                IdIspNum[idx] = 1;
                _UTest_CreateIqInstance(idx);
                IdIqNum[idx] = 1;
                MHalVpeIqSetDnrTblMask((void *)u32IqCtx[idx],0, pCmdqCfg);
                if(idx==0)
                {
                    _UTest_AllocateMem(idx,(u32)MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
                    _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);
                }
                else
                {
                    if(pCmdqCfg)
                    {
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                    }
                }
                if(!_UTest_PutFile2Buffer(0,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                {
                    return 0;
                }
                if(sg_scl_yc_viraddr[0])
                {
                    _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                    _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,MAXOUTPUTportH, MAXOUTPUTportV);
                    _UTest_SetIqCfg(idx,SET_NR);
                    _UTest_SetIqOnOff(idx,SET_NR);
                    MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==0)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==1)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==2)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,
                        sg_scl_yc_addr[0]+(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTportV*2*(idx/4))+(u32)((idx%4)*MAXOUTPUTportH*2),
                        (u8level ==3)? 1 : 0,MAXOUTPUTSTRIDEH*2);
                    MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                    stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                    stVdinfo[idx].u32Stride[1] = u16IspInwsize;
                    stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[0];
                    stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[0]+((u32)(u16IspInwsize*u16IspInhsize));
                    MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                    u8glevel[idx] = u8level;
                }
            }
            if(pCmdqCfg)
            {
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                do
                {
                    DrvSclOsDelayTask(5);
                    pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                }
                while(!(u32CMDQIrq&0x800));
            }
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
            if(u8Count>1)
            {
                for(i=1;i<u8Count;i++)
                {
                    for(idx=0;idx<16;idx++)
                    {
                        if(!_UTest_PutFile2Buffer(0,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                        {
                            return 0;
                        }
                        MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                        MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                        MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                        MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        if(pCmdqCfg)
                        {
                            do
                            {
                                DrvSclOsDelayTask(5);
                                pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                            }
                            while(!(u32CMDQIrq&0x800));
                        }
                    }
                    _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2);
                }
            }
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestS_%dx%d_16chin1_%hx.bin a:0x%lx++0x%lx \n",
                MAXOUTPUTSTRIDEH,MAXOUTPUTSTRIDEV,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[0]+MIU0_BASE,(u32)(MAXOUTPUTSTRIDEH*MAXOUTPUTSTRIDEV*2));
        }
        else if((int)*str == 'M') //Test M
        {
            u16 inputhsize[16] = {352,480,176,192,416,640,256,360,288,360,244,256,240,480,256,192};
            u16 inputvsize[16] = {288,360,244,256,240,480,256,192,352,480,176,192,416,640,256,360};
            u32 u32Addr;
            //disable all
            _UTest_CleanInst();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            for(idx=0;idx<16;idx++)
            {
                _UTest_CreateSclInstance(idx,inputhsize[idx],inputvsize[idx]);
                IdNum[idx] = 1;
                _UTest_CreateIqInstance(idx);
                IdIqNum[idx] = 1;
                u16swsize = MAXOUTPUTportH;
                u16shsize = MAXOUTPUTportV;
                if(idx==0)
                {
                    _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2*31);
                    u32Addr = sg_scl_yc_addr[0];
                }
                else
                {
                    pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                }
                if(sg_scl_yc_viraddr[0])
                {
                    _UTest_SetInputCfgPatMode(idx, inputhsize[idx], inputvsize[idx]);
                    _UTest_SetCropCfg(idx, inputhsize[idx], inputvsize[idx]);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                    _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                    _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                    MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                    //  1,2,3,4,5,6,  8,9,10,11,12,13,  , 15    =13
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,
                        u32Addr,(idx%7) ? 1 : 0,u16swsize*2);
                    if((idx%7))
                    {
                        u32Addr = u32Addr+(u32)(u16swsize*u16shsize*2);
                    }
                    // 0, 2,  4,   6,  8    10,    12,    14        = 8
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,
                        u32Addr,(idx%2) ? 0 : 1,u16swsize*2);
                    if(!(idx%2))
                    {
                        u32Addr = u32Addr+(u32)(u16swsize*u16shsize*2);
                    }
                    //  1,2  ,4,5,  7,8,   10,11,    13,14,15   =11
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,
                        u32Addr,(idx%3) ? 1 : 0,u16swsize*2);
                    if((idx%3))
                    {
                        u32Addr = u32Addr+(u32)(u16swsize*u16shsize*2);
                    }
                    _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,
                        u32Addr,0,u16swsize*2);
                    MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,E_MHAL_VPE_WAITDONE_DMAONLY);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152580,0,0x8000);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x1525E0,0,0x1);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,1,0x1);
                    pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,0,0x1);
                }
            }
            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
            if(pCmdqCfg)
            {
                do
                {
                    DrvSclOsDelayTask(5);
                    pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                }
                while(!(u32CMDQIrq&0x800));
            }
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTportH*MAXOUTPUTportV*2*31);
            if(u8Count>1)
            {
                for(i=1;i<u8Count;i++)
                {
                    for(idx=0;idx<16;idx++)
                    {
                        MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                        MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                        MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,E_MHAL_VPE_WAITDONE_DMAONLY);
                        pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152580,0,0x8000);
                        pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x1525E0,0,0x1);
                        pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,1,0x1);
                        pCmdqCfg->MHAL_CMDQ_WriteRegCmdqMask(pCmdqCfg,0x152502,0,0x1);
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                    }
                    if(pCmdqCfg)
                    {
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                    }
                    _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTportH*MAXOUTPUTportV*2*31);
                }
            }
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestM_%dx%d_multiport_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[0]+MIU0_BASE,(u32)(u16swsize*u16shsize*2*31));
        }
        else if((int)*str == 'm') //Test m
        {
            //u16 inputhsize[16] = {352,480,176,192,416,640,256,360,288,360,244,256,240,480,256,192};
            //u16 inputvsize[16] = {288,360,244,256,240,480,256,192,352,480,176,192,416,640,256,360};
            u32 u32Addr;
            //disable all
            if(u8level>=2)
            {
                UT_multiport_multisize(u8level,u8Count);
            }
            else
            {
                _UTest_CleanInst();
                if(pCmdqCfg==NULL)
                {
                    pCmdqCfg = _UTest_AllocateCMDQ();
                }
                writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
                u16swsize = MAXOUTPUTportH;
                u16shsize = MAXOUTPUTportV;
                //idx=0;
                for(idx=0;idx<16;idx++)
                {
                    _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
                    IdNum[idx] = 1;
                    MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
                    IdIspNum[idx] = 1;
                    _UTest_CreateIqInstance(idx);
                    IdIqNum[idx] = 1;
                    if(idx==0)
                    {
                        _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2*31);
                        _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);
                        u32Addr = sg_scl_yc_addr[0];
                    }
                    else
                    {
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                    }
                    if(!_UTest_PutFile2Buffer(0,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                    {
                        return 0;
                    }
                    if(sg_scl_yc_viraddr[0])
                    {
                        _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                        _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                        _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                        _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                        _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                        _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                        _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                        _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                        _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                        _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
                        MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                        //  1,2,3,4,5,6,  8,9,10,11,12,13,  , 15    =13
                        _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,
                            u32Addr,(idx%7) ? 1 : 0,u16swsize*2);
                        if((idx%7))
                        {
                            u32Addr = u32Addr+(u16swsize*u16shsize*2);
                        }
                        // 0, 2,  4,   6,  8    10,    12,    14        = 8
                        _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,
                            u32Addr,(idx%2) ? 0 : 1,u16swsize*2);
                        if(!(idx%2))
                        {
                            u32Addr = u32Addr+(u16swsize*u16shsize*2);
                        }
                        //  1,2  ,4,5,  7,8,   10,11,    13,14,15   =11
                        _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,
                            u32Addr,(idx%3) ? 1 : 0,u16swsize*2);
                        if((idx%3))
                        {
                            u32Addr = u32Addr+(u32)(u16swsize*u16shsize*2);
                        }
                        _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,
                            u32Addr,0,u16swsize*2);
                        MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                        stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                        stVdinfo[idx].u32Stride[1] = u16IspInwsize;
                        stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[0];
                        stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[0]+((u32)(u16IspInwsize*u16IspInhsize));
                        MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,E_MHAL_VPE_WAITDONE_DMAONLY);
                    }
                }
                pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                DrvSclOsDelayTask(1000);
                _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTportH*MAXOUTPUTportV*2*31);
                if(u8Count>1)
                {
                    for(i=1;i<u8Count;i++)
                    {
                        for(idx=0;idx<16;idx++)
                        {
                            if(!_UTest_PutFile2Buffer(0,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                            {
                                return 0;
                            }
                            MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                            MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                            MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                            MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                            MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,E_MHAL_VPE_WAITDONE_DMAONLY);
                            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        }
                        if(pCmdqCfg)
                        {
                            do
                            {
                                DrvSclOsDelayTask(5);
                                pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                            }
                            while(!(u32CMDQIrq&0x800));
                        }
                        _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[0],MAXOUTPUTportH*MAXOUTPUTportV*2*31);
                    }
                }
                SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestM_%dx%d_multiport_%hx.bin a:0x%lx++0x%lx \n",
                    u16swsize,u16shsize,(u16)(u32Time&0xFFFF)
                    ,sg_scl_yc_addr[0]+MIU0_BASE,(u32)(u16swsize*u16shsize*2*31));
            }
        }
        else if((int)*str == 'W')  //Test W
        {
            if(IdIspNum[u8level])
            {
                idx=0;
                MHalVpeSclDbgLevel((void *)&idx);
                while(1)
                {
                    if(!_UTest_PutFile2Buffer(u8level,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                    {
                        return 0;
                    }
                    MHalVpeIqRead3DNRTbl((void *)u32IqCtx[u8level]);
                    MHalVpeIqProcess((void *)u32IqCtx[u8level], pCmdqCfg);
                    MHalVpeSclProcess((void *)u32Ctx[u8level],pCmdqCfg, &stpBuffer[u8level]);
                    MHalVpeIspProcess((void *)u32IspCtx[u8level],pCmdqCfg, &stVdinfo[u8level]);
                    MHalVpeSclSetWaitDone((void *)u32Ctx[u8level],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[u8level]));
                    pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                    if(pCmdqCfg)
                    {
                        do
                        {
                            DrvSclOsDelayTask(5);
                            pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                        }
                        while(!(u32CMDQIrq&0x800));
                    }
                }
            }
            SCL_ERR( "[VPE]Set %d Set all Dma Process\n",(int)*str);
        }
        else if((int)*str == 'w')  //Test w
        {
            idx=0;
            MHalVpeSclDbgLevel((void *)&idx);
            while(1)
            {
                for(idx=0;idx<=u8level;idx++)
                {
                    if(IdIspNum[idx])
                    {
                        if(!_UTest_PutFile2Buffer(0,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
                        {
                            return 0;
                        }
                        MHalVpeIqRead3DNRTbl((void *)u32IqCtx[idx]);
                        MHalVpeIqProcess((void *)u32IqCtx[idx], pCmdqCfg);
                        MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                        MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                        MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8glevel[idx]));
                        pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
                        if(pCmdqCfg)
                        {
                            do
                            {
                                DrvSclOsDelayTask(5);
                                pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                            }
                            while(!(u32CMDQIrq&0x800));
                        }
                    }
                }
            }
        }
        else if((int)*str == 'T')  //Test T
        {
            if(IdIspNum[u8level])
            {
                _UTest_Create_Task(u8level,(u32)(u16IspInwsize*u16IspInhsize*3/2),(TaskEntry)UT_DispTask);
            }
            SCL_ERR( "[VPE]Create_Task\n");
        }
        else if((int)*str == 't')  //Test T
        {
            if(IdIspNum[u8level])
            {
                _UTest_Destroy_Task();
            }
            SCL_ERR( "[VPE]Destroy_Task\n");
        }
        //if(pCmdqCfg)
        //pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
#endif

        return n;
    }

    return 0;
}

static ssize_t check_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    end = end;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL UNIT TEST======================\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 0: Close All Inst\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 1: Create Scl Inst Max Input Size(%d,%d)\n",MAXOUTPUTportH,MAXOUTPUTportV);
    str += DrvSclOsScnprintf(str, end - str, "Test 2: Set All Input/Crop Config bEn:by idx size(,,,)\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 3: Set All Dma config by level 0:422 1:420\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 4: Set all Dma output size by level 0:FHD 1:XGA 2:HD 3:VGA 4:CIF 5:3M 6:4M 7:5M 8:s4K2K 9:4K2K\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 5: Set for All Dma Process\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 6: Set for level Dma Process\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 7: Set level Dma output size level 0:FHD 1:XGA 2:HD 3:VGA 4:CIF 5:3M 6:4M 7:5M 8:s4K2K 9:4K2K\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 8: Set level Pat input size level 0:FHD 1:736x240 2:HD 3:VGA 4:CIF 5:3M 6:4M 7:5M 8:s4K2K 9:4K2K\n");
    str += DrvSclOsScnprintf(str, end - str, "Test 9: Set level Isp input size level 0:FHD 1:736x240 2:HD 3:VGA 4:CIF 5:3M 6:4M 7:5M 8:s4K2K 9:4K2K\n");
    str += DrvSclOsScnprintf(str, end - str, "Test A: PT All Flow level port\n");
    str += DrvSclOsScnprintf(str, end - str, "Test a: PT All Flow CMDQ level port\n");
    str += DrvSclOsScnprintf(str, end - str, "Test B: PT All Flow all port\n");
    str += DrvSclOsScnprintf(str, end - str, "Test b: PT All Flow level port write\n");
    str += DrvSclOsScnprintf(str, end - str, "Test C: Create IQ Inst\n");
    str += DrvSclOsScnprintf(str, end - str, "Test c: Reserve\n");
    str += DrvSclOsScnprintf(str, end - str, "Test D: ISP All Flow level port //tmp//Ispin.yuv\n");
    str += DrvSclOsScnprintf(str, end - str, "Test d: ISP All Flow level port Process\n");
    str += DrvSclOsScnprintf(str, end - str, "Test E: Get Nr by Level\n");
    str += DrvSclOsScnprintf(str, end - str, "Test e: Get Roi by Level\n");
    str += DrvSclOsScnprintf(str, end - str, "Test F: Set Config Default\n");
    str += DrvSclOsScnprintf(str, end - str, "Test f: Set Config Es2\n");
    str += DrvSclOsScnprintf(str, end - str, "Test G: Set Config Edge\n");
    str += DrvSclOsScnprintf(str, end - str, "Test g: Set ON Edge\n");
    str += DrvSclOsScnprintf(str, end - str, "Test H: Set Config Es\n");
    str += DrvSclOsScnprintf(str, end - str, "Test h: Set ON Es\n");
    str += DrvSclOsScnprintf(str, end - str, "Test I: Set Config contrast\n");
    str += DrvSclOsScnprintf(str, end - str, "Test i: Set ON contrast\n");
    str += DrvSclOsScnprintf(str, end - str, "Test J: Set Config uv\n");
    str += DrvSclOsScnprintf(str, end - str, "Test j: Set ON uv\n");
    str += DrvSclOsScnprintf(str, end - str, "Test M: PT  16inst multiport random input/crop 256x256 output\n");
    str += DrvSclOsScnprintf(str, end - str, "Test m: ISP 16inst multiport random input/crop 256x256 output\n");
    str += DrvSclOsScnprintf(str, end - str, "Test N: Set Config Nr\n");
    str += DrvSclOsScnprintf(str, end - str, "Test n: Set ON Nr\n");
    str += DrvSclOsScnprintf(str, end - str, "Test P: ISP All Flow level port write file count\n");
    str += DrvSclOsScnprintf(str, end - str, "Test p: ISP All Flow level port write file \n");
    str += DrvSclOsScnprintf(str, end - str, "Test R: ISP Rotate by level inst\n");
    str += DrvSclOsScnprintf(str, end - str, "Test r: ISP Set Rotate angle by level\n");
    str += DrvSclOsScnprintf(str, end - str, "Test S: PT  16inst port 0 Stride 1 frame 256x256x16\n");
    str += DrvSclOsScnprintf(str, end - str, "Test s: ISP 16inst port 0 Stride 1 frame 256x256x16\n");
    str += DrvSclOsScnprintf(str, end - str, "========================SCL UNIT TEST======================\n");
    return (str - buf);
}

static DEVICE_ATTR(test,0644, check_test_show, check_test_store);
#if defined(USE_USBCAM)
static ssize_t check_iqtest_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    const char *str = buf;
    int idx,i;
    u8 u8level;
    u32 u32Time,u32CMDQIrq;
    static u16 u16swsize = FHDOUTPUTWIDTH;
    static u16 u16shsize = FHDOUTPUTHEIGHT;
    static u16 u16IspInwsize = ISPINPUTWIDTH;
    static u16 u16IspInhsize = ISPINPUTHEIGHT;
    static u8 u8Str[10];

    if(NULL!=buf)
    {
        u8level = _mdrv_Scl_Changebuf2hex((int)*(str+1));
        u32Time = ((u32)DrvSclOsGetSystemTimeStamp());
        u8Str[0] = u8Str[0];

        if((int)*str == 'A')  //Test A
        {
#if 1
            DrvSclVipIoPeakingConfig_t tTmpcfg;
            DrvSclVipIoDlcHistogramConfig_t t1Tmpcfg;
            DrvSclVipIoDlcConfig_t t2Tmpcfg;
            DrvSclVipIoLceConfig_t t3Tmpcfg;
            DrvSclVipIoUvcConfig_t t4Tmpcfg;
            DrvSclVipIoIhcConfig_t t5Tmpcfg;
            DrvSclVipIoIccConfig_t t6Tmpcfg;
            DrvSclVipIoIhcIccConfig_t t7Tmpcfg;
            DrvSclVipIoIbcConfig_t    t8Tmpcfg;
            DrvSclVipIoFccConfig_t    t9Tmpcfg;
            DrvSclVipIoNlmConfig_t t10Tmpcfg;
            DrvSclVipIoAckConfig_t t11Tmpcfg;
            DrvSclVipIoSetMaskOnOff_t t12Tmpcfg;
            DrvSclVipIoWdrRoiHist_t   t13Tmpcfg;
            DrvSclVipIoConfig_t         t14Tmpcfg;
            DrvSclVipIoAipConfig_t       t15Tmpcfg;
            DrvSclVipIoAipSramConfig_t   t16Tmpcfg;
            void                         *tmpmem;
            DrvSclVipIoMcnrConfig_t      t17Tmpcfg;
            DrvSclVipIoReqMemConfig_t    stReqMemCfg;
#endif
            idx = _UTest_GetEmptyIdx();
            if(pCmdqCfg==NULL)
            {
                pCmdqCfg = _UTest_AllocateCMDQ();
            }
            _UTest_CreateSclInstance(idx,u16IspInwsize,u16IspInhsize);
            MHalVpeCreateIspInstance(NULL ,(void *)&u32IspCtx[idx]);
            IdIspNum[idx] = 1;
            u32IqCtx[idx] = DrvSclVipIoOpen(E_DRV_SCLVIP_IO_ID_1);
            IdIqNum[idx] = 1;
            _UTest_AllocateMem(idx,(u32)u16swsize*u16shsize*2);
            _UTest_AllocateIspMem(idx,(u32)u16IspInwsize*u16IspInhsize*3/2);//YUV420
            if(!_UTest_PutFile2Buffer(idx,"/tmp/Ispin.yuv",(u32)(u16IspInwsize*u16IspInhsize*3/2)))
            {
                return 0;
            }
            writefp = _UTest_OpenFile("/tmp/Scout.bin",O_WRONLY|O_CREAT,0777);
            if(sg_scl_yc_viraddr[idx])
            {
                //MHalVpeSetIrqCallback(NULL);
                //MHalVpeSclEnableIrq(1);
                _DrvSclVipIoKeepCmdqFunction(pCmdqCfg);
                _UTest_SetInputCfgISPMode(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetCropCfg(idx, u16IspInwsize, u16IspInhsize);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT0);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT1);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT2);
                _UTest_SetDmaCfg(idx,E_MHAL_PIXEL_FRAME_YUV422_YUYV,E_MHAL_SCL_OUTPUT_PORT3);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,u16swsize, u16shsize);
                _UTest_SetOutputSizeCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,u16swsize, u16shsize);
#if 1
                memset(&tTmpcfg,0xaa,sizeof(DrvSclVipIoPeakingConfig_t));
                FILL_VERCHK_TYPE(tTmpcfg,tTmpcfg.VerChk_Version,tTmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                //printk("[CMDQ]IQ peak bnum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                DrvSclVipIoSetPeakingConfig(u32IqCtx[idx],&tTmpcfg);
                //printk("[CMDQ]IQ peak mnum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ peak anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                //MHalVpeIqProcess((void *)u32IqCtx[idx],pCmdqCfg);

                memset(&t1Tmpcfg,0x11,sizeof(DrvSclVipIoDlcHistogramConfig_t));
                FILL_VERCHK_TYPE(t1Tmpcfg,t1Tmpcfg.VerChk_Version,t1Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetDlcHistogramConfig(u32IqCtx[idx],&t1Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 111 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));



                //set dlc
                memset(&t2Tmpcfg,0x22,sizeof(DrvSclVipIoDlcConfig_t));
                FILL_VERCHK_TYPE(t2Tmpcfg,t2Tmpcfg.VerChk_Version,t2Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetDlcConfig(u32IqCtx[idx],&t2Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 222 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //set lce
                memset(&t3Tmpcfg,0x33,sizeof(DrvSclVipIoLceConfig_t));
                FILL_VERCHK_TYPE(t3Tmpcfg,t3Tmpcfg.VerChk_Version,t3Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetLceConfig(u32IqCtx[idx],&t3Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 333 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //Uvc
                memset(&t4Tmpcfg,0x44,sizeof(DrvSclVipIoUvcConfig_t));
                FILL_VERCHK_TYPE(t4Tmpcfg,t4Tmpcfg.VerChk_Version,t4Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetUvcConfig(u32IqCtx[idx],&t4Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 444 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //IHC
                memset(&t5Tmpcfg,0x55,sizeof(DrvSclVipIoIhcConfig_t));
                FILL_VERCHK_TYPE(t5Tmpcfg,t5Tmpcfg.VerChk_Version,t5Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetIhcConfig(u32IqCtx[idx],&t5Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 555 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //ICC
                memset(&t6Tmpcfg,0x66,sizeof(DrvSclVipIoIccConfig_t));
                FILL_VERCHK_TYPE(t6Tmpcfg,t6Tmpcfg.VerChk_Version,t6Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetIccConfig(u32IqCtx[idx],&t6Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 666 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));

                //IhcIceAdpYConfig
                memset(&t7Tmpcfg,0x77,sizeof(DrvSclVipIoIhcIccConfig_t));
                FILL_VERCHK_TYPE(t7Tmpcfg,t7Tmpcfg.VerChk_Version,t7Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetIhcIceAdpYConfig(u32IqCtx[idx],&t7Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 777 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //Ibc
                memset(&t8Tmpcfg,0x88,sizeof(DrvSclVipIoIbcConfig_t));
                FILL_VERCHK_TYPE(t8Tmpcfg,t8Tmpcfg.VerChk_Version,t8Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetIbcConfig(u32IqCtx[idx],&t8Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 888 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));

                //FCC
                memset(&t9Tmpcfg,0x99,sizeof(DrvSclVipIoFccConfig_t));
                FILL_VERCHK_TYPE(t9Tmpcfg,t9Tmpcfg.VerChk_Version,t9Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetFccConfig(u32IqCtx[idx],&t9Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ 999 anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //nlm

                memset(&t10Tmpcfg,0xaa,sizeof(DrvSclVipIoNlmConfig_t));
                FILL_VERCHK_TYPE(t10Tmpcfg,t10Tmpcfg.VerChk_Version,t10Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetNlmConfig(u32IqCtx[idx],&t10Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ aa anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));

                //ack
                memset(&t11Tmpcfg,0xbb,sizeof(DrvSclVipIoAckConfig_t));
                FILL_VERCHK_TYPE(t11Tmpcfg,t11Tmpcfg.VerChk_Version,t11Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetAckConfig(u32IqCtx[idx],&t11Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ bb anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));


                //wdr mask
                memset(&t12Tmpcfg,0xCC,sizeof(DrvSclVipIoSetMaskOnOff_t));
                //FILL_VERCHK_TYPE(t12Tmpcfg,t12Tmpcfg.VerChk_Version,t12Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                t12Tmpcfg.enMaskType = E_DRV_SCLVIP_IO_MASK_WDR;
                DrvSclVipIoSetWdrRoiMask(u32IqCtx[idx],&t12Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ cc anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));

                //WDR ROI
                memset(&t13Tmpcfg,0xdd,sizeof(DrvSclVipIoWdrRoiHist_t));
                //FILL_VERCHK_TYPE(t13Tmpcfg,t13Tmpcfg.VerChk_Version,t13Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetWdrRoiHistConfig(u32IqCtx[idx],&t13Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ dd anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));

                //VIP
                memset(&t14Tmpcfg,0xcc,sizeof(DrvSclVipIoConfig_t));
                FILL_VERCHK_TYPE(t14Tmpcfg,t14Tmpcfg.VerChk_Version,t14Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                DrvSclVipIoSetVipConfig(u32IqCtx[idx],&t14Tmpcfg);
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ cc anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                //DrvSclVipIoSetVipConfig(s32 s32Handler,DrvSclVipIoConfig_t *pstCfg)

                //AIP
                tmpmem = DrvSclOsMemalloc(16*1024,GFP_KERNEL);
                memset(tmpmem,0xff,sizeof(tmpmem));
                for(i = E_DRV_SCLVIP_IO_AIP_YEE ; i < E_DRV_SCLVIP_IO_AIP_NUM ; i++)
                {

                    memset(&t15Tmpcfg,0xdd,sizeof(DrvSclVipIoAipConfig_t));
                    FILL_VERCHK_TYPE(t15Tmpcfg,t15Tmpcfg.VerChk_Version,t15Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                    t15Tmpcfg.enAIPType = i;
                    t15Tmpcfg.u32Viraddr = (u32)tmpmem;
                    DrvSclVipIoSetAipConfig(u32IqCtx[idx],&t15Tmpcfg);
                    //DrvSclVipIoSetAipConfig(s32 s32Handler, DrvSclVipIoAipConfig_t *pstIoConfig)
                }
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ dd anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                DrvSclOsMemFree(tmpmem);

                //AIP sram
                tmpmem = DrvSclOsMemalloc(16*1024,GFP_KERNEL);
                memset(tmpmem,0xaa,sizeof(tmpmem));
                for(i = E_DRV_SCLVIP_IO_AIP_SRAM_WDR_TBL ; i <= E_DRV_SCLVIP_IO_AIP_SRAM_WDR_TBL ; i++)
                {
                    memset(&t16Tmpcfg,0xee,sizeof(DrvSclVipIoAipSramConfig_t));
                    FILL_VERCHK_TYPE(t16Tmpcfg,t16Tmpcfg.VerChk_Version,t16Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                    t16Tmpcfg.enAIPType = i;
                    t16Tmpcfg.u32Viraddr = (u32)tmpmem;
                    DrvSclVipIoSetAipSramConfig(u32IqCtx[idx],&t16Tmpcfg);
                    //DrvSclVipIoSetAipSramConfig(s32 s32Handler, DrvSclVipIoAipSramConfig_t *pstIoCfg)
                }
                //DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                //printk("[CMDQ]IQ ee anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                DrvSclOsMemFree(tmpmem);

                tmpmem = DrvSclOsMemalloc(16*1024,GFP_KERNEL);
                memset(&t17Tmpcfg,0xee,sizeof(DrvSclVipIoAipSramConfig_t));
                FILL_VERCHK_TYPE(t17Tmpcfg,t17Tmpcfg.VerChk_Version,t17Tmpcfg.VerChk_Size,DRV_SCLVIP_VERSION);
                FILL_VERCHK_TYPE(stReqMemCfg,stReqMemCfg.VerChk_Version,stReqMemCfg.VerChk_Size,DRV_SCLVIP_VERSION);
                stReqMemCfg.u16Pitch = 1920;
                stReqMemCfg.u16Vsize = 1080;
                stReqMemCfg.enCeType = E_DRV_SCLVIP_IO_UCMCE_8;
                stReqMemCfg.u32MemSize = 1920*1080*2;
                t17Tmpcfg.u32Viraddr = (u32)tmpmem;
                _DrvSclVipIoReqmemConfig(u32IqCtx[idx],&stReqMemCfg);
                DrvSclVipIoSetMcnrConfig(u32IqCtx[idx],&t17Tmpcfg);
                    //DrvSclVipIoSetAipSramConfig(s32 s32Handler, DrvSclVipIoAipSramConfig_t *pstIoCfg)
                DrvSclVipIoctlInstFlip(u32IqCtx[idx]);
                DrvSclOsMemFree(tmpmem);
#endif
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT0,sg_scl_yc_addr[idx],
                    (u8level ==0 || u8level ==4 || u8level ==5 || u8level ==7 || u8level ==8) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT1,sg_scl_yc_addr[idx],
                    (u8level ==1 || u8level ==4 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT2,sg_scl_yc_addr[idx],
                    (u8level ==2 || u8level ==5 || u8level ==6 || u8level ==7) ? 1 :0,u16swsize*2);
                _UTest_SetProcessCfg(idx,E_MHAL_SCL_OUTPUT_PORT3,sg_scl_yc_addr[idx],
                    (u8level ==3 || u8level ==8) ? 1 :0,u16swsize*2);
                MHalVpeSclProcess((void *)u32Ctx[idx],pCmdqCfg, &stpBuffer[idx]);
                //printk("[CMDQ]IQ sclprocess anum=%d\n",pCmdqCfg->MHAL_CMDQ_GetCurrentCmdqNumber(pCmdqCfg));
                stVdinfo[idx].u32Stride[0] = u16IspInwsize;
                stVdinfo[idx].u32Stride[1] = u16IspInwsize;
                stVdinfo[idx].u64PhyAddr[0] = sg_Isp_yc_addr[idx];
                stVdinfo[idx].u64PhyAddr[1] = sg_Isp_yc_addr[idx]+((u32)(u16IspInwsize*u16IspInhsize));
                MHalVpeIspProcess((void *)u32IspCtx[idx],pCmdqCfg, &stVdinfo[idx]);
                MHalVpeSclSetWaitDone((void *)u32Ctx[idx],pCmdqCfg,_UTest_GetWaitFlag(u8level));
                u8glevel[idx] = u8level;

            }
            pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);
            if(pCmdqCfg)
            {
                do
                {
                    DrvSclOsDelayTask(5);
                    pCmdqCfg->MHAL_CMDQ_ReadStatusCmdq(pCmdqCfg,(unsigned int*)&u32CMDQIrq);
                }
                while(!(u32CMDQIrq&0x800));
            }
            _UTest_WriteFile(writefp,(char *)sg_scl_yc_viraddr[idx],(u32)u16swsize*u16shsize*2);
            SCL_ERR( "[VPE]Set inst Direct%d\n",(int)*str);
            SCL_ERR( "data.save.binary \\\\hcnas01\\garbage\\paul-pc.wang\\frame\\TestD_%dx%d_%d_%hx.bin a:0x%lx++0x%lx \n",
                u16swsize,u16shsize,idx,(u16)(u32Time&0xFFFF)
                ,sg_scl_yc_addr[idx]+MIU0_BASE,(u32)(u16swsize*u16shsize*2));
        }

        //if(pCmdqCfg)
        //pCmdqCfg->MHAL_CMDQ_KickOffCmdq(pCmdqCfg);


        return n;
    }

    return 0;
}
static ssize_t check_iqtest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    end = end;
    str += DrvSclOsScnprintf(str, end - str, "========================SCL UNIT TEST======================\n");

    str += DrvSclOsScnprintf(str, end - str, "========================SCL UNIT TEST======================\n");
    return (str - buf);
}

static DEVICE_ATTR(iqtest,0644, check_iqtest_show, check_iqtest_store);

#endif// for Iq
#endif// for Test
static ssize_t check_ints_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspIntsShow(buf);
}
static DEVICE_ATTR(ints,0444, check_ints_show, NULL);


static ssize_t check_dbgmg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspDbgmgFlagShow(buf);
}

static ssize_t check_dbgmg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        u8 u8level;
        SCL_ERR( "[HVSP1]check_dbgmg_store OK %d\n",(int)*str);
        SCL_ERR( "[HVSP1]check_dbgmg_store level %d\n",(int)*(str+1));
        SCL_ERR( "[HVSP1]check_dbgmg_store level2 %d\n",(int)*(str+2));
        if(((int)*(str+2))>=48)//LF :line feed
        {
            u8level = _mdrv_Scl_Changebuf2hex((int)*(str+2));
            u8level |= (_mdrv_Scl_Changebuf2hex((int)*(str+1))<<4);
        }
        else
        {
            u8level = _mdrv_Scl_Changebuf2hex((int)*(str+1));
        }

        if((int)*str == 48)    //input 1  echo 0 >
        {
            Reset_DBGMG_FLAG();
            MHalVpeIqDbgLevel(&gbdbgmessage[EN_DBGMG_VPEIQ_CONFIG]);
            MHalVpeSclDbgLevel(&gbdbgmessage[EN_DBGMG_VPESCL_CONFIG]);
            MHalVpeIspDbgLevel(&gbdbgmessage[EN_DBGMG_VPEISP_CONFIG]);
        }
        else if((int)*str == 49)    //input 1  echo 1 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_MDRV_CONFIG,u8level);
        }
        else if((int)*str == 50)    //input 1  echo 2 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_IOCTL_CONFIG,u8level);
        }
        else if((int)*str == 51)    //input 1  echo 3 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_HVSP_CONFIG,u8level);
        }
        else if((int)*str == 52)    //input 1  echo 4 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_SCLDMA_CONFIG,u8level);
        }
        else if((int)*str == 53)    //input 1  echo 5 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_PNL_CONFIG,u8level);
        }
        else if((int)*str == 54)    //input 1  echo 6 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_VIP_CONFIG,u8level);
        }
        else if((int)*str == 55)    //input 1  echo 7 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVPQ_CONFIG,u8level);
        }
        else if((int)*str == 56)    //input 1  echo 8 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_CTX_CONFIG,u8level);
        }
        else if((int)*str == 57)    //input 1  echo 9 >
        {
            Set_DBGMG_FLAG(EN_DBGMG_VPESCL_CONFIG,u8level);
            MHalVpeSclDbgLevel(&gbdbgmessage[EN_DBGMG_VPESCL_CONFIG]);
        }
        else if((int)*str == 65)    //input 1  echo A >
        {
            Set_DBGMG_FLAG(EN_DBGMG_VPEIQ_CONFIG,u8level);
            MHalVpeIqDbgLevel(&gbdbgmessage[EN_DBGMG_VPEIQ_CONFIG]);
        }
        else if((int)*str == 66)    //input 1  echo B >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVHVSP_CONFIG,u8level);
        }
        else if((int)*str == 67)    //input 1  echo C >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVSCLDMA_CONFIG,u8level);
        }
        else if((int)*str == 68)    //input 1  echo D >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVSCLIRQ_CONFIG,u8level);
        }
        else if((int)*str == 69)    //input 1  echo E >
        {
            Set_DBGMG_FLAG(EN_DBGMG_VPEISP_CONFIG,u8level);
            MHalVpeIspDbgLevel(&gbdbgmessage[EN_DBGMG_VPEISP_CONFIG]);
        }
        else if((int)*str == 70)    //input 1  echo F >
        {
            Set_DBGMG_FLAG(EN_DBGMG_DRVVIP_CONFIG,u8level);
        }
        else if((int)*str == 71)    //input 1  echo G >
        {
            Set_DBGMG_FLAG(EN_DBGMG_PRIORITY_CONFIG,1);
        }
        else if((int)*str == 72)    //input 1  echo H >
        {
            Set_DBGMG_FLAG(EN_DBGMG_UTILITY_CONFIG,u8level);
        }
        else if((int)*str == 73) // input 1 echo I >
        {
            MDrvSclHvspDbgmgDumpShow(u8level);
        }
        return n;
    }

    return 0;
}

static DEVICE_ATTR(dbgmg,0644, check_dbgmg_show, check_dbgmg_store);
static ssize_t check_lock_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return MDrvSclHvspLockShow(buf);
}
static ssize_t check_lock_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        if((int)*str == 76)    //input 1  echo L >
        {
            if(!DrvSclOsReleaseMutexAll())
            {
                SCL_DBGERR("[HVSP]!!!!!!!!!!!!!!!!!!! HVSP Release Mutex fail\n");
            }
        }
        return n;
    }
    return 0;
}

static DEVICE_ATTR(mutex,0644, check_lock_show, check_lock_store);
u32 gu32SaveAddr = 0;
void *gpSaveVirAddr = NULL;
u32 gu32SaveSize = 0;
bool gbioremap = 0;
bool gSavepath[20] = "/tmp/Scout.bin";
static ssize_t savebin_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct file *wfp = NULL;
    SCL_ERR("[SCL]Size :%lx ,VirAddr:%lx ,Addr:%lx path:%s\n",gu32SaveSize,(u32)gpSaveVirAddr,(u32)gu32SaveAddr,gSavepath);
    wfp = _UTest_OpenFile(gSavepath,O_WRONLY|O_CREAT,0777);
    if(wfp && gpSaveVirAddr && gu32SaveSize)
    {
        _UTest_WriteFile(wfp,(char *)gpSaveVirAddr,gu32SaveSize);
        _UTest_CloseFile(wfp);
    }
    return 0;
}
static ssize_t savebin_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        int idx = 0;
        u32 u32Level;
        u8 u8off = 7;
        if((int)*str == 'a')
        {
            str++;
            idx++;
            u8off = 7;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu32SaveAddr |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]gu32SaveAddr OK %lx\n",gu32SaveAddr);
            str++;
        }
        if((int)*str == 's')
        {
            str++;
            idx = 0;
            u8off = 0;
            while(1)
            {
                if((((int)*(str+idx))<48) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            u8off--;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu32SaveSize |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]Size OK %lx\n",gu32SaveSize);
            str++;
        }
        if((int)*str == 'r')
        {
            str++;
            //gpSaveVirAddr = ioremap((gu32SaveAddr+0x20000000),gu32SaveSize);
            if(gu32SaveAddr < 0x2F800000+0x1000000)
            {
                gpSaveVirAddr = (void*)(gu32SaveAddr+(0xC0000000-0x1000000));
                SCL_ERR("[SCL]Save bin VirAddr: %lx\n",(u32)gpSaveVirAddr);
            }
            else
            {
                gpSaveVirAddr = ioremap((gu32SaveAddr+0x20000000),gu32SaveSize);
                gbioremap = 1;
            }
            str++;
        }
        if((int)*str == 'p')
        {
            str++;
            u8off = 0;
            idx = 0;
            while(1)
            {
                if((((int)*(str+idx))<33) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            for(idx=0;idx<u8off;idx++)
            {
                gSavepath[idx] = *(str+idx);
            }
            SCL_ERR("[SCL]idx OK %s\n",gSavepath);
        }
        if((int)*str == 'u')
        {
            //gpSaveVirAddr = ioremap((gu32SaveAddr+0x20000000),gu32SaveSize);
            if(gbioremap)
            {
                iounmap(gpSaveVirAddr);
                gbioremap = 0;
                gpSaveVirAddr = NULL;
                gu32SaveAddr = 0;
                gu32SaveSize = 0;
            }
            else
            {
                gpSaveVirAddr = NULL;
                gu32SaveAddr = 0;
                gu32SaveSize = 0;
            }
        }
        return n;
    }
    return 0;
}

static DEVICE_ATTR(savebin,0644, savebin_show, savebin_store);
u32 gu32RegAddr = 0x151e;
u16 gu16RegVal = 0;
u16 gu16RegMsk = 0xFFFF;
u16 gu16RegOffset = 0;
bool gbAllBank = 1;
static ssize_t Reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 u32RegAddr = 0;
    u32RegAddr = ((gu16RegOffset<<1) | (gu32RegAddr<<8));
    if(u32RegAddr)
    {
        return MDrvSclHvspRegShow(buf,u32RegAddr,&gu16RegVal,gbAllBank);
    }
    else
    {
        return 0;
    }
}
static ssize_t Reg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        const char *str = buf;
        int idx = 0;
        u32 u32Level;
        static u32 u32Inst = 0xFF;
        u8 u8off = 0;
        MDrvSclHvspMiscConfig_t stHvspMiscCfg;
        u8 input_tgen_buf[10];
        if((int)*str == 'a')//addr
        {
            str++;
            idx++;
            u8off = 3;
            gu32RegAddr = 0;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu32RegAddr |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]gu32SaveAddr OK %lx\n",gu32RegAddr);
            str++;

        }
        if((int)*str == 'o')//offset
        {
            str++;
            idx = 0;
            u8off = 0;
            gu16RegOffset = 0;
            while(1)
            {
                if((((int)*(str+idx))<48) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            u8off--;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu16RegOffset |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]RegOffset OK %hx\n",gu16RegOffset);
            str++;
        }
        if((int)*str == 'v')//value
        {
            str++;
            idx = 0;
            u8off = 0;
            gu16RegVal = 0;
            while(1)
            {
                if((((int)*(str+idx))<48) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            u8off--;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu16RegVal |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]RegVal OK %hx\n",gu16RegVal);
            str++;
        }
        if((int)*str == 'm')//mask
        {
            str++;
            idx = 0;
            u8off = 0;
            gu16RegMsk = 0;
            while(1)
            {
                if((((int)*(str+idx))<48) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            u8off--;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    gu16RegMsk |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]RegMsk OK %hx\n",gu16RegMsk);
            str++;
        }
        if((int)*str == 'b')//b all = 1
        {
            str++;
            gbAllBank = 1;
            str++;
        }
        if((int)*str == 's')//b all = 0
        {
            str++;
            gbAllBank = 0;
            str++;
        }
        if((int)*str == 'i')//write reg
        {
            str++;
            idx = 0;
            u8off = 0;
            u32Inst = 0;
            while(1)
            {
                if((((int)*(str+idx))<48) || idx >100  )//LF :line feed
                {
                    break;
                }
                else
                {
                    u8off++;
                    idx++;
                }
            }
            u8off--;
            while(1)
            {
                if((((int)*(str))<48) || idx >100  )//LF :line feed
                {
                    str++;
                    break;
                }
                else
                {
                    u32Level = (u32)_mdrv_Scl_Changebuf2hex((int)*(str));
                    u32Inst |= (u32Level<< (u8off*4));
                    if(u8off)
                    {
                        u8off--;
                        str++;
                        idx++;
                    }
                    else
                    {
                        str++;
                        idx++;
                        break;
                    }
                }
            }
            SCL_ERR("[SCL]u32Inst OK %lx\n",u32Inst);
            MDrvSclHvspSetProcInst(u32Inst);
            str++;
        }
        if((int)*str == 'w')//write reg
        {
            str++;
            stHvspMiscCfg.u8Cmd     = (u32Inst==0xFF) ? E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINSTALL : E_MDRV_SCLHVSP_MISC_CMD_SET_REG_BYINST;
            stHvspMiscCfg.u32Size   = sizeof(input_tgen_buf);
            stHvspMiscCfg.u32Addr   = (u32)input_tgen_buf;
            input_tgen_buf[0] = gu32RegAddr&0xFF;
            input_tgen_buf[1] = (gu32RegAddr>>8)&0xFF;
            input_tgen_buf[2] = (gu16RegOffset&0xFF)*2;
            input_tgen_buf[3] = gu16RegVal&0xFF;
            input_tgen_buf[4] = gu16RegMsk&0xFF;
            input_tgen_buf[5] = gu32RegAddr&0xFF;
            input_tgen_buf[6] = (gu32RegAddr>>8)&0xFF;
            input_tgen_buf[7] = ((gu16RegOffset&0xFF)*2)+1;
            input_tgen_buf[8] = (gu16RegVal>>8)&0xFF;
            input_tgen_buf[9] = (gu16RegMsk>>8)&0xFF;
            MDrvSclHvspSetMiscConfigForKernel(&stHvspMiscCfg);
            SCL_ERR("[SCL]Write inst:%lx OK\n",u32Inst);
            str++;
        }
        return n;
    }
    return 0;
}

static DEVICE_ATTR(reg,0644, Reg_show, Reg_store);

//==============================================================================

long DrvSclModuleIoctl(struct file *filp, unsigned int u32Cmd, unsigned long u32Arg)
{
    int err = 0;
    int retval = 0;
    if(filp->private_data == NULL)
    {
        SCL_ERR( "[SCL] IO_IOCTL private_data =NULL!!! \n");
        return -EFAULT;
    }
    /* check u32Cmd valid */
    if(IOCTL_SCL_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_SCL_MAX_NR)
        {
            SCL_ERR( "[SCL] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else if(IOCTL_VIP_MAGIC == _IOC_TYPE(u32Cmd))
    {
        if(_IOC_NR(u32Cmd) >= IOCTL_SCL_MAX_NR)
        {
            SCL_ERR( "[SCL] IOCtl NR Error!!! (Cmd=%x)\n",u32Cmd);
            return -ENOTTY;
        }
    }
    else
    {
        SCL_ERR( "[SCL] IOCtl MAGIC Error!!! (Cmd=%x)\n",u32Cmd);
        return -ENOTTY;
    }

    /* verify Access */
    if (_IOC_DIR(u32Cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    else if (_IOC_DIR(u32Cmd) & _IOC_WRITE)
    {
        err =  !access_ok(VERIFY_READ, (void __user *)u32Arg, _IOC_SIZE(u32Cmd));
    }
    if (err)
    {
        return -EFAULT;
    }
    /* not allow query or command once driver suspend */

    retval = DrvSclIoctlParse(filp, u32Cmd, u32Arg);
    return retval;
}


static unsigned int DrvSclModulePoll(struct file *filp, struct poll_table_struct *wait)
{
    wait_queue_head_t *pWaitQueueHead = NULL;
    DrvSclHvspIoWrapperPollConfig_t stPollCfg;
    s32 s32Handler = *((s32 *)filp->private_data);

    pWaitQueueHead = (wait_queue_head_t *)MDrvSclHvspGetWaitQueueHead();
    stPollCfg.stPollWaitCfg.filp = filp;
    stPollCfg.stPollWaitCfg.pWaitQueueHead = pWaitQueueHead;
    stPollCfg.stPollWaitCfg.pstPollQueue = wait;
    stPollCfg.bWaitQueue = 1;
    stPollCfg.pCBFunc = NULL;
    stPollCfg.u8pollval = 0;
    stPollCfg.u8retval = 0;

    _DrvSclHvspIoPoll(s32Handler, &stPollCfg);

    return (unsigned int )stPollCfg.u8retval;
}


static int DrvSclModuleSuspend(struct platform_device *dev, pm_message_t state)
{
    MDrvSclHvspSuspendResumeConfig_t stHvspSuspendResumeCfg;
    int ret = 0;
    DrvSclOsClkConfig_t stClkCfg;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP_1] %s\n",__FUNCTION__);
    stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
    stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
    stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
    stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);

    stHvspSuspendResumeCfg.u32IRQNum = DrvSclOsGetIrqIDSCL(E_DRV_SCLOS_SCLIRQ_SC0);
    stHvspSuspendResumeCfg.u32CMDQIRQNum = DrvSclOsGetIrqIDCMDQ(E_DRV_SCLOS_CMDQIRQ_CMDQ0);
    if(MDrvSclHvspSuspend(E_MDRV_SCLHVSP_ID_1, &stHvspSuspendResumeCfg))
    {
        MDrvSclHvspIdclkRelease((MDrvSclHvspClkConfig_t *)&stClkCfg);
        ret = 0;
    }
    else
    {
        ret = -EFAULT;
    }

    return ret;
}

static int DrvSclModuleResume(struct platform_device *dev)
{
    MDrvSclHvspSuspendResumeConfig_t stHvspSuspendResumeCfg;
    int ret = 0;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[HVSP1] %s\n",__FUNCTION__);

    stHvspSuspendResumeCfg.u32IRQNum     = DrvSclOsGetIrqIDSCL(E_DRV_SCLOS_SCLIRQ_SC0);;
    stHvspSuspendResumeCfg.u32CMDQIRQNum = DrvSclOsGetIrqIDCMDQ(E_DRV_SCLOS_CMDQIRQ_CMDQ0);;
    MDrvSclHvspResume(E_MDRV_SCLHVSP_ID_1, &stHvspSuspendResumeCfg);

    return ret;
}
void _DrvSclModuleSetRefCnt(u32 u32DevNum, bool bAdd)
{
    switch(u32DevNum)
    {
        case DRV_SCL_DEVICE_MINOR:
            if(bAdd)
            {
                _tSclDevice.refCnt++;
            }
            else
            {
                if(_tSclDevice.refCnt)
                {
                    _tSclDevice.refCnt--;
                }
            }
        break;
        case DRV_SCLM2M_DEVICE_MINOR:
            if(bAdd)
            {
                _tSclM2MDevice.refCnt++;
            }
            else
            {
                if(_tSclM2MDevice.refCnt)
                {
                    _tSclM2MDevice.refCnt--;
                }
            }
        break;
        case DRV_SCLVIP_DEVICE_MINOR:
            if(bAdd)
            {
                _tSclVipDevice.refCnt++;
            }
            else
            {
                if(_tSclVipDevice.refCnt)
                {
                    _tSclVipDevice.refCnt--;
                }
            }
        break;
        case DRV_SCLHVSP_DEVICE_MINOR1:
            if(bAdd)
            {
                _tSclHvsp1Device.refCnt++;
            }
            else
            {
                if(_tSclHvsp1Device.refCnt)
                {
                    _tSclHvsp1Device.refCnt--;
                }
            }
        break;
        case DRV_SCLHVSP_DEVICE_MINOR2:
            if(bAdd)
            {
                _tSclHvsp2Device.refCnt++;
            }
            else
            {
                if(_tSclHvsp2Device.refCnt)
                {
                    _tSclHvsp2Device.refCnt--;
                }
            }
        break;
        case DRV_SCLHVSP_DEVICE_MINOR3:
            if(bAdd)
            {
                _tSclHvsp3Device.refCnt++;
            }
            else
            {
                if(_tSclHvsp3Device.refCnt)
                {
                    _tSclHvsp3Device.refCnt--;
                }
            }
        break;
        case DRV_SCLHVSP_DEVICE_MINOR4:
            if(bAdd)
            {
                _tSclHvsp4Device.refCnt++;
            }
            else
            {
                if(_tSclHvsp4Device.refCnt)
                {
                    _tSclHvsp4Device.refCnt--;
                }
            }
        break;
        case DRV_SCLDMA_DEVICE_MINOR1:
            if(bAdd)
            {
                _tSclDma1Device.refCnt++;
            }
            else
            {
                if(_tSclDma1Device.refCnt)
                {
                    _tSclDma1Device.refCnt--;
                }
            }
        break;
        case DRV_SCLDMA_DEVICE_MINOR2:
            if(bAdd)
            {
                _tSclDma2Device.refCnt++;
            }
            else
            {
                if(_tSclDma2Device.refCnt)
                {
                    _tSclDma2Device.refCnt--;
                }
            }
        break;
        case DRV_SCLDMA_DEVICE_MINOR3:
            if(bAdd)
            {
                _tSclDma3Device.refCnt++;
            }
            else
            {
                if(_tSclDma3Device.refCnt)
                {
                    _tSclDma3Device.refCnt--;
                }
            }
        break;
        case DRV_SCLDMA_DEVICE_MINOR4:
            if(bAdd)
            {
                _tSclDma4Device.refCnt++;
            }
            else
            {
                if(_tSclDma4Device.refCnt)
                {
                    _tSclDma4Device.refCnt--;
                }
            }
        break;
    }
}
bool _DrvSclVipCreateIqInst(s32 *ps32Handle)
{
    DrvSclVipIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclVipIoLockConfig_t));
    stIoInCfg.ps32IdBuf = ps32Handle;
    stIoInCfg.u8BufSize = 1;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclVipIoCreateInstConfig(*ps32Handle,&stIoInCfg))
    {
        SCL_ERR("[SCLMOD]%s Create Inst Fail\n",__FUNCTION__);
        return 0;
    }
    _DrvSclVipIoReqWdrMloadBuffer(*ps32Handle);
    return 1;
}
bool _DrvSclVipDestroyInst(s32 *ps32Handle)
{
    DrvSclVipIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclVipIoLockConfig_t));
    stIoInCfg.ps32IdBuf = ps32Handle;
    stIoInCfg.u8BufSize = 1;
    //free wdr mload
    _DrvSclVipIoFreeWdrMloadBuffer(*ps32Handle);
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclVipIoDestroyInstConfig(*ps32Handle,&stIoInCfg))
    {
        SCL_ERR("[VPE]%s Destroy Inst Fail\n",__FUNCTION__);
        return 0;
    }
    return 1;
}
s32 _DrvSclCreateInst(s32 *s32Handle,u32 u32Count)
{
    DrvSclDmaIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclDmaIoLockConfig_t));
    stIoInCfg.ps32IdBuf = s32Handle;
    stIoInCfg.u8BufSize = u32Count;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclDmaIoCreateInstConfig(s32Handle[0],&stIoInCfg))
    {
        SCL_ERR("[VPE]%s Create Inst Fail\n",__FUNCTION__);
        return -1;
    }
    return (s32Handle[0]&0xFFFF);
}
bool _DrvSclDestroyInst(s32 *s32Handle,u32 u32Count)
{
    DrvSclDmaIoLockConfig_t stIoInCfg;
    DrvSclOsMemset(&stIoInCfg,0,sizeof(DrvSclDmaIoLockConfig_t));
    stIoInCfg.ps32IdBuf = s32Handle;
    stIoInCfg.u8BufSize = u32Count;
    FILL_VERCHK_TYPE(stIoInCfg, stIoInCfg.VerChk_Version, stIoInCfg.VerChk_Size, DRV_SCLDMA_VERSION);
    if(_DrvSclDmaIoDestroyInstConfig(s32Handle[0],&stIoInCfg))
    {
        SCL_ERR("[VPE]%s Destroy Inst Fail\n",__FUNCTION__);
        return 0;
    }
    return 1;
}

s32 _DrvSclModuleOpenDevice(u32 u32DevNum)
{
    s32 s32Handler = -1;
    s32 *ps32MHandler;
    switch(u32DevNum)
    {
        case DRV_SCL_DEVICE_MINOR:
            /*
            ps32MHandler = DrvSclOsVirMemalloc(sizeof(s32)*VPE_DEVICE_COUNT);
            if(ps32MHandler ==NULL)
            {
                return s32Handler;
            }
            ps32MHandler[0] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_1);
            ps32MHandler[1] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_2);
            ps32MHandler[2] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_3);
            ps32MHandler[3] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_4);
            ps32MHandler[4] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_1);
            ps32MHandler[5] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_2);
            ps32MHandler[6] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_3);
            ps32MHandler[7] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_4);
            s32Handler = _DrvSclCreateInst(ps32MHandler,VPE_DEVICE_COUNT);
            s32Handler = s32Handler | SCLVPE_HANDLER_PRE_FIX;
            DrvSclOsVirMemFree(ps32MHandler);
            */
                s32Handler = SCLVPE_HANDLER_PRE_FIX;
        break;
        case DRV_SCLM2M_DEVICE_MINOR:
            ps32MHandler = DrvSclOsVirMemalloc(sizeof(s32)*M2M_DEVICE_COUNT);
            if(ps32MHandler ==NULL)
            {
                return s32Handler;
            }
            ps32MHandler[0] = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_3);
            ps32MHandler[1] = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_3);
            ps32MHandler[2] = (ps32MHandler[0]&0xFFFF)| SCLM2M_HANDLER_PRE_FIX;
            _DrvSclCreateInst(ps32MHandler,M2M_DEVICE_COUNT);
            s32Handler = ps32MHandler[2];
            DrvSclOsVirMemFree(ps32MHandler);
        break;
        case DRV_SCLVIP_DEVICE_MINOR:
            s32Handler = _DrvSclVipIoOpen(E_DRV_SCLVIP_IO_ID_1);
            _DrvSclVipCreateIqInst(&s32Handler);
        break;
        case DRV_SCLHVSP_DEVICE_MINOR1:
            s32Handler = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_1);
        break;
        case DRV_SCLHVSP_DEVICE_MINOR2:
            s32Handler = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_2);
        break;
        case DRV_SCLHVSP_DEVICE_MINOR3:
            s32Handler = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_3);
        break;
        case DRV_SCLHVSP_DEVICE_MINOR4:
            s32Handler = _DrvSclHvspIoOpen(E_DRV_SCLHVSP_IO_ID_4);
        break;
        case DRV_SCLDMA_DEVICE_MINOR1:
            s32Handler = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_1);
        break;
        case DRV_SCLDMA_DEVICE_MINOR2:
            s32Handler = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_2);
        break;
        case DRV_SCLDMA_DEVICE_MINOR3:
            s32Handler = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_3);
        break;
        case DRV_SCLDMA_DEVICE_MINOR4:
            s32Handler = _DrvSclDmaIoOpen(E_DRV_SCLDMA_IO_ID_4);
        break;
    }
    return s32Handler;
}
u32 _DrvSclModuleCloseDevice(u32 u32DevNum, s32 s32Handler)
{
    u32 u32Ret = 0;
    s32 *ps32MHandler;
    switch(u32DevNum)
    {
        case DRV_SCL_DEVICE_MINOR:
            /*
            ps32MHandler = DrvSclOsVirMemalloc(sizeof(s32)*VPE_DEVICE_COUNT);
            if(ps32MHandler ==NULL)
            {
                return s32Handler;
            }
            ps32MHandler[0] = (s32Handler&0xFFFF)|SCLDMA_HANDLER_PRE_FIX;
            _DrvSclDestroyInst(ps32MHandler,VPE_DEVICE_COUNT);
            u32Ret = _DrvSclDmaIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_DMA_4]);
            u32Ret = _DrvSclDmaIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_DMA_3]);
            u32Ret = _DrvSclDmaIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_DMA_2]);
            u32Ret = _DrvSclDmaIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_DMA_1]);
            u32Ret = _DrvSclHvspIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_HVSP_4]);
            u32Ret = _DrvSclHvspIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_HVSP_3]);
            u32Ret = _DrvSclHvspIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_HVSP_2]);
            u32Ret = _DrvSclHvspIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_HVSP_1]);
            DrvSclOsVirMemFree(ps32MHandler);
            */

        break;
        case DRV_SCLM2M_DEVICE_MINOR:
            ps32MHandler = DrvSclOsVirMemalloc(sizeof(s32)*M2M_DEVICE_COUNT);
            if(ps32MHandler ==NULL)
            {
                return s32Handler;
            }
            ps32MHandler[0] = s32Handler;
            _DrvSclDestroyInst(ps32MHandler,M2M_DEVICE_COUNT);
            u32Ret = _DrvSclDmaIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_DMA_3]);
            u32Ret = _DrvSclHvspIoRelease(ps32MHandler[E_DRV_SCLOS_DEV_HVSP_3]);
            DrvSclOsVirMemFree(ps32MHandler);
        break;
        case DRV_SCLVIP_DEVICE_MINOR:
            _DrvSclVipDestroyInst(&s32Handler);
            u32Ret = _DrvSclVipIoRelease(s32Handler);
        break;
        case DRV_SCLHVSP_DEVICE_MINOR1:
        case DRV_SCLHVSP_DEVICE_MINOR2:
        case DRV_SCLHVSP_DEVICE_MINOR3:
        case DRV_SCLHVSP_DEVICE_MINOR4:
            u32Ret = _DrvSclHvspIoRelease(s32Handler);
            break;
        case DRV_SCLDMA_DEVICE_MINOR1:
        case DRV_SCLDMA_DEVICE_MINOR2:
        case DRV_SCLDMA_DEVICE_MINOR3:
        case DRV_SCLDMA_DEVICE_MINOR4:
            u32Ret = _DrvSclDmaIoRelease(s32Handler);
            break;
    }
    return u32Ret;
}

int DrvSclModuleOpen(struct inode *inode, struct file *filp)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s %lx\n",__FUNCTION__,(u32)inode->i_rdev);

    SCL_ASSERT(_tSclDevice.refCnt>=0);

    if(filp->private_data == NULL)
    {

        filp->private_data = DrvSclOsVirMemalloc(sizeof(s32));

        if(filp->private_data == NULL)
        {
            SCL_ERR("[SCLHVSP_1] %s %d, allocate memory fail\n", __FUNCTION__, __LINE__);
            ret = -EFAULT;
        }
        else
        {
            s32 s32Handler = -1;
            s32Handler = _DrvSclModuleOpenDevice((u32)(inode->i_rdev&0xFF));
            if(s32Handler != -1)
            {
                *((s32 *)filp->private_data) = s32Handler;
            }
            else
            {
                SCL_ERR("[HVSP1] %s %d, handler error fail\n", __FUNCTION__, __LINE__);
                ret = -EFAULT;
                DrvSclOsVirMemFree(filp->private_data);
            }
        }
    }
    if(!ret)
    {
        _DrvSclModuleSetRefCnt((u32)(inode->i_rdev&0xFF),1);
    }

    return ret;
}


int DrvSclModuleRelease(struct inode *inode, struct file *filp)
{
    int ret = 0;
    s32 s32Handler;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);

    s32Handler = *((s32 *)filp->private_data);

    if( _DrvSclModuleCloseDevice((u32)(inode->i_rdev&0xFF),s32Handler) == E_DRV_SCLHVSP_IO_ERR_OK)
    {
        DrvSclOsVirMemFree(filp->private_data);
        filp->private_data = NULL;
    }
    else
    {
        ret = -EFAULT;
        SCL_ERR("[SCLHVSP_1] Release Fail\n");
    }

    _DrvSclModuleSetRefCnt((u32)(inode->i_rdev&0xFF),0);
    SCL_ASSERT(_tSclDevice.refCnt>=0);
    return ret;
}
void _DrvSclVpeModuleDeInit(void)
{
    DrvSclOsClkConfig_t stClkCfg;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLDMA_1] %s\n",__FUNCTION__);
    stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
    stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
    stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
    stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);
    MDrvSclHvspIdclkRelease((MDrvSclHvspClkConfig_t *)&stClkCfg);
    MDrvSclDmaClkClose((MDrvSclDmaClkConfig_t *)&stClkCfg);
    if(_tSclDevice.cdev.count)
    {
        cdev_del(&_tSclDevice.cdev);
    }
    stDrvSclPlatformDevice.dev.of_node=NULL;
    _tSclHvspClass = NULL;
    //ToDo
    //device_unregister(_tSclDevice.devicenode);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor));
    //class_destroy(_tSclHvspClass);
    //unregister_chrdev_region(MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor), DRV_SCL_DEVICE_COUNT);
}
void _DrvSclVpeModuleInit(void)
{
    int s32Ret;
    dev_t  dev;
    DrvSclOsClkConfig_t stClkCfg;
    if(_tSclDevice.s32Major)
    {
        dev     = MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor);
        //s32Ret  = register_chrdev_region(dev, DRV_SCL_DEVICE_COUNT, DRV_SCL_DEVICE_NAME);
        if(!_tSclHvspClass)
        {
            _tSclHvspClass = msys_get_sysfs_class();
            if(!_tSclHvspClass)
            {
                _tSclHvspClass = class_create(THIS_MODULE, SclHvspClassName);
            }

        }
        else
        {
            cdev_init(&_tSclDevice.cdev, &_tSclDevice.fops);
            if (0 != (s32Ret= cdev_add(&_tSclDevice.cdev, dev, DRV_SCL_DEVICE_COUNT)))
            {
                SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
            }
        }
        //ToDo
        if(_tSclDevice.devicenode==NULL)
        {
            _tSclDevice.devicenode = device_create(_tSclHvspClass, NULL, dev,NULL, DRV_SCL_DEVICE_NAME);
            //create device
            device_create_file(_tSclDevice.devicenode, &dev_attr_proc);
            device_create_file(_tSclDevice.devicenode, &dev_attr_ints);
            device_create_file(_tSclDevice.devicenode, &dev_attr_dbgmg);
            //device_create_file(_tSclDevice.devicenode, &dev_attr_clk);
            device_create_file(_tSclDevice.devicenode, &dev_attr_mutex);
            device_create_file(_tSclDevice.devicenode, &dev_attr_savebin);
            device_create_file(_tSclDevice.devicenode, &dev_attr_reg);
            device_create_file(_tSclDevice.devicenode, &dev_attr_ptgen);
#if defined(SCLOS_TYPE_LINUX_TEST)
            device_create_file(_tSclDevice.devicenode, &dev_attr_test);
#if defined(USE_USBCAM)
            device_create_file(_tSclDevice.devicenode, &dev_attr_iqtest);
#endif
#endif
        }

        if(stDrvSclPlatformDevice.dev.of_node==NULL)
        {
            stDrvSclPlatformDevice.dev.of_node = of_find_compatible_node(NULL, NULL, "mstar,sclhvsp1_i2");
        }
        if(stDrvSclPlatformDevice.dev.of_node==NULL)
        {
            SCL_ERR("[VPE INIT] Get Device mode Fail!!\n");
        }
        DrvSclOsSetSclIrqIDFormSys(&stDrvSclPlatformDevice,0,E_DRV_SCLOS_SCLIRQ_SC0);
        DrvSclOsSetCmdqIrqIDFormSys(&stDrvSclPlatformDevice,1,E_DRV_SCLOS_CMDQIRQ_CMDQ0);
        //clk enable
        stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
        stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
        stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
        stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);
        DrvSclOsClkSetConfig(E_DRV_SCLOS_CLK_ID_HVSP1, &stClkCfg);
    }
}

#if CONFIG_OF
static int DrvSclModuleProbe(struct platform_device *pdev)
{
    //unsigned char ret;
    int s32Ret;
    dev_t  dev[E_DRV_SCLOS_DEV_MAX];
    //struct resource *res_irq;
    //struct device_node *np;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s:%d\n",__FUNCTION__,__LINE__);
    if(_tSclDevice.s32Major)
    {
        dev[E_DRV_SCLOS_DEV_VIP]     = MKDEV(_tSclVipDevice.s32Major, _tSclVipDevice.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_VIP], DRV_SCL_DEVICE_COUNT, DRV_SCLVIP_DEVICE_NAME);
        dev[E_DRV_SCLOS_DEV_HVSP_1]     = MKDEV(_tSclHvsp1Device.s32Major, _tSclHvsp1Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_1], DRV_SCL_DEVICE_COUNT, DRV_SCLHVSP_DEVICE_NAME1);
        dev[E_DRV_SCLOS_DEV_HVSP_2]     = MKDEV(_tSclHvsp2Device.s32Major, _tSclHvsp2Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_2], DRV_SCL_DEVICE_COUNT, DRV_SCLHVSP_DEVICE_NAME2);
        dev[E_DRV_SCLOS_DEV_HVSP_3]     = MKDEV(_tSclHvsp3Device.s32Major, _tSclHvsp3Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_3], DRV_SCL_DEVICE_COUNT, DRV_SCLHVSP_DEVICE_NAME3);
        dev[E_DRV_SCLOS_DEV_HVSP_4]     = MKDEV(_tSclHvsp4Device.s32Major, _tSclHvsp4Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_4], DRV_SCL_DEVICE_COUNT, DRV_SCLHVSP_DEVICE_NAME4);
        dev[E_DRV_SCLOS_DEV_DMA_1]     = MKDEV(_tSclDma1Device.s32Major, _tSclDma1Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_1], DRV_SCL_DEVICE_COUNT, DRV_SCLDMA_DEVICE_NAME1);
        dev[E_DRV_SCLOS_DEV_DMA_2]     = MKDEV(_tSclDma2Device.s32Major, _tSclDma2Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_2], DRV_SCL_DEVICE_COUNT, DRV_SCLDMA_DEVICE_NAME2);
        dev[E_DRV_SCLOS_DEV_DMA_3]     = MKDEV(_tSclDma3Device.s32Major, _tSclDma3Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_3], DRV_SCL_DEVICE_COUNT, DRV_SCLDMA_DEVICE_NAME3);
        dev[E_DRV_SCLOS_DEV_DMA_4]     = MKDEV(_tSclDma4Device.s32Major, _tSclDma4Device.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_4], DRV_SCL_DEVICE_COUNT, DRV_SCLDMA_DEVICE_NAME4);
        dev[E_DRV_SCLOS_DEV_M2M]     = MKDEV(_tSclM2MDevice.s32Major, _tSclM2MDevice.s32Minor);
        //s32Ret  = register_chrdev_region(dev[E_DRV_SCLOS_DEV_M2M], DRV_SCL_DEVICE_COUNT, DRV_SCLM2M_DEVICE_NAME);
    }
    else
    {
        s32Ret                  = alloc_chrdev_region(&dev[E_DRV_SCLOS_DEV_HVSP_1], _tSclDevice.s32Minor, DRV_SCL_DEVICE_COUNT, DRV_SCL_DEVICE_NAME);
        _tSclDevice.s32Major  = MAJOR(dev[E_DRV_SCLOS_DEV_HVSP_1]);
    }
    //if (0 > s32Ret)
    //{
    //    SCL_ERR( "[SCLHVSP_1] Unable to get major %d\n", _tSclDevice.s32Major);
    //    return s32Ret;
    //}

    cdev_init(&_tSclVipDevice.cdev, &_tSclVipDevice.fops);
    cdev_init(&_tSclM2MDevice.cdev, &_tSclM2MDevice.fops);
    cdev_init(&_tSclHvsp1Device.cdev, &_tSclHvsp1Device.fops);
    cdev_init(&_tSclHvsp2Device.cdev, &_tSclHvsp2Device.fops);
    cdev_init(&_tSclHvsp3Device.cdev, &_tSclHvsp3Device.fops);
    cdev_init(&_tSclHvsp4Device.cdev, &_tSclHvsp4Device.fops);
    cdev_init(&_tSclDma1Device.cdev, &_tSclDma1Device.fops);
    cdev_init(&_tSclDma2Device.cdev, &_tSclDma2Device.fops);
    cdev_init(&_tSclDma3Device.cdev, &_tSclDma3Device.fops);
    cdev_init(&_tSclDma4Device.cdev, &_tSclDma4Device.fops);
    if (0 != (s32Ret= cdev_add(&_tSclVipDevice.cdev, dev[E_DRV_SCLOS_DEV_VIP], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_VIP], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclM2MDevice.cdev, dev[E_DRV_SCLOS_DEV_M2M], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_M2M], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclHvsp1Device.cdev, dev[E_DRV_SCLOS_DEV_HVSP_1], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_1], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclHvsp2Device.cdev, dev[E_DRV_SCLOS_DEV_HVSP_2], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_2], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclHvsp3Device.cdev, dev[E_DRV_SCLOS_DEV_HVSP_3], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_3], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclHvsp4Device.cdev, dev[E_DRV_SCLOS_DEV_HVSP_4], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_HVSP_4], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclDma1Device.cdev, dev[E_DRV_SCLOS_DEV_DMA_1], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_1], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclDma2Device.cdev, dev[E_DRV_SCLOS_DEV_DMA_2], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_2], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclDma3Device.cdev, dev[E_DRV_SCLOS_DEV_DMA_3], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_3], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    if (0 != (s32Ret= cdev_add(&_tSclDma4Device.cdev, dev[E_DRV_SCLOS_DEV_DMA_4], DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev[E_DRV_SCLOS_DEV_DMA_4], DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }
    _tSclHvspClass = msys_get_sysfs_class();
    if(!_tSclHvspClass)
    {
        _tSclHvspClass = class_create(THIS_MODULE, SclHvspClassName);
    }
    if(IS_ERR(_tSclHvspClass))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        _tSclVipDevice.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_VIP],NULL, DRV_SCLVIP_DEVICE_NAME);
        _tSclM2MDevice.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_M2M],NULL, DRV_SCLM2M_DEVICE_NAME);
        _tSclHvsp1Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_HVSP_1],NULL, DRV_SCLHVSP_DEVICE_NAME1);
        _tSclHvsp2Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_HVSP_2],NULL, DRV_SCLHVSP_DEVICE_NAME2);
        _tSclHvsp3Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_HVSP_3],NULL, DRV_SCLHVSP_DEVICE_NAME3);
        _tSclHvsp4Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_HVSP_4],NULL, DRV_SCLHVSP_DEVICE_NAME4);
        _tSclDma1Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_DMA_1],NULL, DRV_SCLDMA_DEVICE_NAME1);
        _tSclDma2Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_DMA_2],NULL, DRV_SCLDMA_DEVICE_NAME2);
        _tSclDma3Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_DMA_3],NULL, DRV_SCLDMA_DEVICE_NAME3);
        _tSclDma4Device.devicenode = device_create(_tSclHvspClass, NULL, dev[E_DRV_SCLOS_DEV_DMA_4],NULL, DRV_SCLDMA_DEVICE_NAME4);

        //_tSclDevice.devicenode->dma_mask=&u64SclHvsp_DmaMask;
        //_tSclDevice.devicenode->coherent_dma_mask=u64SclHvsp_DmaMask;
    }
    //probe
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);
    stDrvSclPlatformDevice.dev.of_node = pdev->dev.of_node;

    //create device
    //ret = device_create_file(_tSclHvsp1Device.devicenode, &dev_attr_SCIQ);
    //ret = device_create_file(_tSclHvsp1Device.devicenode, &dev_attr_osd);
    //ret = device_create_file(_tSclHvsp2Device.devicenode, &dev_attr_SCIQ);
    //ret = device_create_file(_tSclHvsp2Device.devicenode, &dev_attr_osd);
    //ret = device_create_file(_tSclHvsp3Device.devicenode, &dev_attr_SCIQ);
    //ret = device_create_file(_tSclHvsp3Device.devicenode, &dev_attr_osd);
    //ret = device_create_file(_tSclHvsp4Device.devicenode, &dev_attr_SCIQ);
    //ret = device_create_file(_tSclHvsp4Device.devicenode, &dev_attr_osd);
    //ret = device_create_file(_tSclHvsp1Device.devicenode, &dev_attr_fbmg);
    Reset_DBGMG_FLAG();
    _DrvSclVpeModuleInit();
    _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_1);
    _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_2);
    _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_3);
    _DrvSclHvspIoInit(E_DRV_SCLHVSP_IO_ID_4);
    _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_1);
    _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_2);
    _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_3);
    _DrvSclDmaIoInit(E_DRV_SCLDMA_IO_ID_4);
    _DrvSclVipIoInit();
    DrvSclOsSetProbeInformation(E_DRV_SCLOS_INIT_ALL);
#if defined(SCLOS_TYPE_LINUX_TEST)
    DrvSclOsMemset(stMaxWin,0,sizeof(MHalVpeSclWinSize_t)*TEST_INST_MAX);
    DrvSclOsMemset(stCropCfg,0,sizeof(MHalVpeSclCropConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stInputpCfg,0,sizeof(MHalVpeSclInputSizeConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stOutputCfg,0,sizeof(MHalVpeSclOutputSizeConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stOutputDmaCfg,0,sizeof(MHalVpeSclOutputDmaConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stMDwinCfg,0,sizeof(MHalVpeSclOutputMDwinConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stpBuffer,0,sizeof(MHalVpeSclOutputBufferConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(u32Ctx,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(u32IqCtx,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(u32IspCtx,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(IdIqNum,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(IdIspNum,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(IdNum,0,sizeof(u32)*TEST_INST_MAX);
    DrvSclOsMemset(stIspInputCfg,0,sizeof(MHalVpeIspInputConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stRotCfg,0,sizeof(MHalVpeIspRotationConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stVdinfo,0,sizeof(MHalVpeIspVideoInfo_t)*TEST_INST_MAX);
    DrvSclOsMemset(stIqCfg,0,sizeof(MHalVpeIqConfig_t)*TEST_INST_MAX);
    DrvSclOsMemset(stIqOnCfg,0,sizeof(MHalVpeIqOnOff_t)*TEST_INST_MAX);
    DrvSclOsMemset(stRoiReport,0,sizeof(MHalVpeIqWdrRoiReport_t)*TEST_INST_MAX);
    DrvSclOsMemset(stHistCfg,0,sizeof(MHalVpeIqWdrRoiHist_t)*TEST_INST_MAX);
#endif
    return 0;
}

static int DrvSclModuleRemove(struct platform_device *pdev)
{
    DrvSclOsClkConfig_t stClkCfg;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLDMA_1] %s\n",__FUNCTION__);
    stClkCfg.ptIdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,0);
    stClkCfg.ptFclk1 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,1);
    stClkCfg.ptFclk2 = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,2);
    stClkCfg.ptOdclk = DrvSclOsClkGetClk((void *)stDrvSclPlatformDevice.dev.of_node,3);
    _DrvSclHvspIoMemFree();
    MDrvSclHvspIdclkRelease((MDrvSclHvspClkConfig_t *)&stClkCfg);
    MDrvSclDmaClkClose((MDrvSclDmaClkConfig_t *)&stClkCfg);
    _DrvSclVipIoDeInit();
    _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_4);
    _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_3);
    _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_2);
    _DrvSclDmaIoDeInit(E_DRV_SCLDMA_IO_ID_1);
    _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_4);
    _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_3);
    _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_2);
    _DrvSclHvspIoDeInit(E_DRV_SCLHVSP_IO_ID_1);
    _DrvSclVpeModuleDeInit();
    //ToDo
    device_unregister(_tSclDevice.devicenode);
    cdev_del(&_tSclHvsp1Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclHvsp1Device.s32Major, _tSclHvsp1Device.s32Minor));
    device_unregister(_tSclHvsp1Device.devicenode);
    cdev_del(&_tSclHvsp2Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclHvsp2Device.s32Major, _tSclHvsp2Device.s32Minor));
    device_unregister(_tSclHvsp2Device.devicenode);
    cdev_del(&_tSclHvsp3Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclHvsp3Device.s32Major, _tSclHvsp3Device.s32Minor));
    device_unregister(_tSclHvsp3Device.devicenode);
    cdev_del(&_tSclHvsp4Device.cdev);
    device_unregister(_tSclHvsp4Device.devicenode);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclHvsp4Device.s32Major, _tSclHvsp4Device.s32Minor));
    cdev_del(&_tSclDma1Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDma1Device.s32Major, _tSclDma1Device.s32Minor));
    device_unregister(_tSclDma1Device.devicenode);
    cdev_del(&_tSclDma2Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDma2Device.s32Major, _tSclDma2Device.s32Minor));
    device_unregister(_tSclDma2Device.devicenode);
    cdev_del(&_tSclDma3Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDma3Device.s32Major, _tSclDma3Device.s32Minor));
    device_unregister(_tSclDma3Device.devicenode);
    cdev_del(&_tSclDma4Device.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDma4Device.s32Major, _tSclDma4Device.s32Minor));
    device_unregister(_tSclDma4Device.devicenode);
    cdev_del(&_tSclVipDevice.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclVipDevice.s32Major, _tSclVipDevice.s32Minor));
    device_unregister(_tSclVipDevice.devicenode);
    cdev_del(&_tSclM2MDevice.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclM2MDevice.s32Major, _tSclM2MDevice.s32Minor));
    device_unregister(_tSclM2MDevice.devicenode);
    //cdev_del(&_tSclDevice.cdev);
    //device_destroy(_tSclHvspClass, MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor));
    //class_destroy(_tSclHvspClass);
    //unregister_chrdev_region(MKDEV(_tSclHvsp1Device.s32Major, _tSclHvsp1Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclHvsp2Device.s32Major, _tSclHvsp2Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclHvsp3Device.s32Major, _tSclHvsp3Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclHvsp4Device.s32Major, _tSclHvsp4Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclDma1Device.s32Major, _tSclDma1Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclDma2Device.s32Major, _tSclDma2Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclDma3Device.s32Major, _tSclDma3Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclDma4Device.s32Major, _tSclDma4Device.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclVipDevice.s32Major, _tSclVipDevice.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclM2MDevice.s32Major, _tSclM2MDevice.s32Minor), DRV_SCL_DEVICE_COUNT);
    //unregister_chrdev_region(MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor), DRV_SCL_DEVICE_COUNT);
    return 0;
}
#else
static int DrvSclModuleProbe(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);

    return 0;
}
static int DrvSclModuleRemove(struct platform_device *pdev)
{
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);

    return 0;
}

#endif


//-------------------------------------------------------------------------------------------------
// Module functions
//-------------------------------------------------------------------------------------------------
#if CONFIG_OF
int _DrvSclModuleInit(void)
{
    int ret = 0;
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s:%d\n",__FUNCTION__,__LINE__);

    ret = platform_driver_register(&stDrvSclPlatformDriver);
    if (!ret)
    {
        SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] platform_driver_register success\n");
    }
    else
    {
        SCL_ERR( "[SCLHVSP_1] platform_driver_register failed\n");
        platform_driver_unregister(&stDrvSclPlatformDriver);
    }


    return ret;
}
void _DrvSclModuleExit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);
    platform_driver_unregister(&stDrvSclPlatformDriver);
}
#else

int _DrvSclModuleInit(void)
{
    int ret = 0;
    int s32Ret;
    dev_t  dev;
    //struct device_node *np;

    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);
    //np = of_find_compatible_node(NULL, NULL, "mstar,hvsp1");
    //if (np)
    //{
    //  SCL_DBG(SCL_DBG_LV_MDRV_IO(), "Find scl dts node\n");
    //  stDrvSclPlatformDevice.dev.of_node = of_node_get(np);
    //  of_node_put(np);
    //}
    //else
    //{
    //    return -ENODEV;
    //}

    if(_tSclDevice.s32Major)
    {
        dev     = MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor);
        s32Ret  = register_chrdev_region(dev, DRV_SCL_DEVICE_COUNT, DRV_SCL_DEVICE_NAME);
    }
    else
    {
        s32Ret                  = alloc_chrdev_region(&dev, _tSclDevice.s32Minor, DRV_SCL_DEVICE_COUNT, DRV_SCL_DEVICE_NAME);
        _tSclDevice.s32Major  = MAJOR(dev);
    }

    if (0 > s32Ret)
    {
        SCL_ERR( "[SCLHVSP_1] Unable to get major %d\n", _tSclDevice.s32Major);
        return s32Ret;
    }

    cdev_init(&_tSclDevice.cdev, &_tSclDevice.fops);
    if (0 != (s32Ret= cdev_add(&_tSclDevice.cdev, dev, DRV_SCL_DEVICE_COUNT)))
    {
        SCL_ERR( "[SCLHVSP_1] Unable add a character device\n");
        unregister_chrdev_region(dev, DRV_SCL_DEVICE_COUNT);
        return s32Ret;
    }

    _tSclHvspClass = class_create(THIS_MODULE, SclHvspClassName);
    if(IS_ERR(_tSclHvspClass))
    {
        printk(KERN_WARNING"Failed at class_create().Please exec [mknod] before operate the device/n");
    }
    else
    {
        device_create(_tSclHvspClass, NULL, dev,NULL, DRV_SCL_DEVICE_NAME);
    }

    ret = platform_driver_register(&stDrvSclPlatformDriver);

    if (!ret)
    {
        ret = platform_device_register(&stDrvSclPlatformDevice);
        if (ret)    // if register device fail, then unregister the driver.
        {
            platform_driver_unregister(&stDrvSclPlatformDriver);
            SCL_ERR( "[SCLHVSP_1] platform_driver_register failed\n");

        }
        else
        {
            SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] platform_driver_register success\n");
        }
    }


    return ret;
}
void _DrvSclModuleExit(void)
{
    /*de-initial the who GFLIPDriver */
    SCL_DBG(SCL_DBG_LV_MDRV_IO(), "[SCLHVSP_1] %s\n",__FUNCTION__);

    cdev_del(&_tSclDevice.cdev);
    device_destroy(_tSclHvspClass, MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor));
    class_destroy(_tSclHvspClass);
    unregister_chrdev_region(MKDEV(_tSclDevice.s32Major, _tSclDevice.s32Minor), DRV_SCL_DEVICE_COUNT);
    platform_driver_unregister(&stDrvSclPlatformDriver);
}

#endif
EXPORT_SYMBOL(_DrvSclVpeModuleInit);
EXPORT_SYMBOL(_DrvSclVpeModuleDeInit);

module_init(_DrvSclModuleInit);
module_exit(_DrvSclModuleExit);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("mstar sclhvsp ioctrl driver");
MODULE_LICENSE("GPL");
