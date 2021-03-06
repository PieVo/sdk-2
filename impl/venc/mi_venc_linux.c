#include "mi_common.h"
#include "mhal_venc.h"
#include "mi_venc_internal.h"
#include <linux/interrupt.h>

MI_S32 MI_VENC_RequestIrq(MI_U32 u32IrqNum, MI_VENC_PFN_IRQ pfnIsr, char *szName, void* pUserData)
{
    int ret;
    ret = request_irq(u32IrqNum, pfnIsr, IRQF_SHARED, szName, pUserData);
    if (ret == 0)
        return 0;
    return -1;
}

MI_S32 MI_VENC_FreeIrq(MI_U32 u32IrqNum, void* pUserData)
{
    free_irq(u32IrqNum, pUserData);
    return 0;
}
