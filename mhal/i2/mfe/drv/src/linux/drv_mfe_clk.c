
#include "drv_mfe_dev.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

int MfeDevClkRate(mmfe_dev* mdev, int rate)
{
    struct clk* clock = mdev->p_clocks[0];
    struct clk* clk;
    int i = 0, best = 0;
    while (!(clk = clk_get_parent_by_index(clock, i++)))
    {
        int r = clk_get_rate(clk);

        if (rate > best && best < r)
            best = r;
        if (rate < best && rate < r && r < best)
            best = r;
    }
    return best;
}

int MfeDevClkOn(mmfe_dev* mdev, int on)
{
    int i = on!=0;
    struct clk* clk;
    mhve_ios* mios = mdev->p_asicip;
    mios->irq_mask(mios, 0xFF);
    if (i == 0)
    {
        clk = mdev->p_clocks[0];
        clk_set_parent(clk, clk_get_parent_by_index(clk, 0));
        while (i < MMFE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
            clk_disable_unprepare(clk);
        return 0;
    }
    while (i < MMFE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
        clk_prepare_enable(clk);
    clk = mdev->p_clocks[0];
    clk_set_rate(clk, mdev->i_ratehz);
    clk_prepare_enable(clk);
    return 0;
}

int MfeDevClkInit(mmfe_dev* mdev, u32 nDevId)
{
    struct device_node *np;
    char compatible[16];
    struct clk* clock;
    int i;

    // fixme, direct enable mfe clock
    u16 nRegVal;
    nRegVal = *(volatile u16 *)(IO_ADDRESS((0x1F000000 + (0x100a00 + 0x10 * 2) * 2)));
    //nRegVal = (nRegVal & 0xFF00) | 0x0004;      // 480MHz
    nRegVal = (nRegVal & 0xFF00) | 0x0018;      // 576MHz
    *(volatile u16 *)(IO_ADDRESS((0x1F000000 + (0x100a00 + 0x10 * 2) * 2))) = nRegVal;


    do
    {
        if (nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe");
        np = of_find_compatible_node(NULL, NULL, compatible);
        clock = of_clk_get_by_name(np, "CKG_mfe");
        if (IS_ERR(clock))
            break;
        mdev->p_clocks[0] = clock;
        mdev->i_clkidx = 0;
        mdev->i_ratehz = clk_get_rate(clk_get_parent_by_index(clock, mdev->i_clkidx));

        for (i = 1; i < MMFE_CLOCKS_NR; i++)
        {
            clock = of_clk_get(np, i);
            if (IS_ERR(clock))
                break;
            mdev->p_clocks[i] = clock;
        }

        return 0;
    }
    while (0);

    return -1;
}

int MfeDevGetResourceMem(u32 nDevId, void** ppBase, int* pSize)
{
    struct device_node *np;
    struct resource res;
    char compatible[16];

    if (ppBase && pSize)
    {
        if (nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe");
        np = of_find_compatible_node(NULL, NULL, compatible);
        if (!np || of_address_to_resource(np, 0, &res))
        {
            return -1;
        }

        //CamOsPrintf("res.start: %p    res.end: %p\n", res.start, res.end);

        *ppBase = (void *)IO_ADDRESS(res.start);
        *pSize = res.end - res.start;
        return 0;
    }

    return -1;
}

int MfeDevGetResourceIrq(u32 nDevId, int* pIrq)
{
    struct device_node *np;
    char compatible[16];

    if (pIrq)
    {
        if (nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mfe");
        np = of_find_compatible_node(NULL, NULL, compatible);
        if (!np)
        {
            return -1;
        }
        *pIrq = irq_of_parse_and_map(np, 0);
        return 0;
    }

    return -1;
}
