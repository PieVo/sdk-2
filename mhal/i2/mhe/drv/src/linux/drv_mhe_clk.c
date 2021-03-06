
#include "drv_mhe_dev.h"

#include "infinity2/irqs.h"

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#define I2_ASIC_CLOCK


int MheDevClkRate(mmhe_dev* mdev, int rate)
{

    struct clk* clock = mdev->p_clocks[0];
    struct clk* clk;
    int i = 0, best = 0;
    while(!(clk = clk_get_parent_by_index(clock, i++)))
    {
        int r = clk_get_rate(clk);

        if(rate > best && best < r)
            best = r;
        if(rate < best && rate < r && r < best)
            best = r;
    }
    return best;

}

int MheDevClkOn(mmhe_dev* mdev, int on)
{

    int i = on != 0;
    struct clk* clk;
    mhve_ios* mios = mdev->p_asicip;
    mios->irq_mask(mios, 0xFF);
    if(i == 0)
    {
        clk = mdev->p_clocks[0];
        clk_set_parent(clk, clk_get_parent_by_index(clk, 0));
        while(i < MMHE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
            clk_disable_unprepare(clk);
        return 0;
    }
    while(i < MMHE_CLOCKS_NR && (clk = mdev->p_clocks[i++]))
        clk_prepare_enable(clk);
    clk = mdev->p_clocks[0];
    clk_set_rate(clk, mdev->i_ratehz);
    clk_prepare_enable(clk);
    return 0;
}

int MheDevClkInit(mmhe_dev* mdev, u32 nDevId)
{

    int i;
    struct device_node *np;
    char compatible[16];
    struct clk* clock;

#ifdef I2_ASIC_CLOCK


    // fixme, direct enable mfe clock
     u16 nRegVal;
     nRegVal = *(volatile u16 *)(IO_ADDRESS((0x1F000000 + (0x100a00 + 0x10 * 2) * 2)));
     //nRegVal = (nRegVal & 0x00FF) | 0x0400;      // 480MHz
     nRegVal = (nRegVal & 0x00FF) | 0x1800;      // 576MHz
     *(volatile u16 *)(IO_ADDRESS((0x1F000000 + (0x100a00 + 0x10 * 2) * 2))) = nRegVal;

    return 0;
#endif

    do
    {
        if(nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe");
        np = of_find_compatible_node(NULL, NULL, compatible);
        clock = of_clk_get_by_name(np, "CKG_mhe");
        if(IS_ERR(clock))
            break;
        mdev->p_clocks[0] = clock;
        mdev->i_clkidx = 0;
        mdev->i_ratehz = clk_get_rate(clk_get_parent_by_index(clock, mdev->i_clkidx));

        for(i = 1; i < MMHE_CLOCKS_NR; i++)
        {
            clock = of_clk_get(np, i);
            if(IS_ERR(clock))
                break;

            mdev->p_clocks[i] = clock;
        }

        return 0;
    }
    while(0);

    return -1;
}


int MheDevGetResourceMem(u32 nDevId, void** ppBase, int* pSize)
{
    int i;
    struct device_node *np;
    struct resource res;
    char compatible[16];

    if(ppBase && pSize)
    {
        if(nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe");

        for(i = 0; i < MHVEREG_MAXBASE_NUM; i++)
        {
            np = of_find_compatible_node(NULL, NULL, compatible);
            if(!np || of_address_to_resource(np, i, &res))
            {
                return -1;
            }

            *ppBase = (void *)res.start;
            *pSize = res.end - res.start;

            ppBase++;
            pSize++;
        }
        /*

                for(i = 0; i < MHVEREG_MAXBASE_NUM; i++)
                {
                    res = platform_get_resource(pOsDev, IORESOURCE_MEM, i);
                    *ppBase =  (void *)res->start;
                    *pSize = res->end - res->start;

                    ppBase++;
                    pSize++;
                }
        */

        return 0;
    }

    return -1;

}

int MheDevGetResourceIrq(u32 nDevId, int* pIrq)
{


    struct device_node *np;
    char compatible[16];

    if(pIrq)
    {
        if(nDevId)
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe%d", nDevId);
        else
            CamOsSnprintf(compatible, sizeof(compatible), "mstar,mhe");
        np = of_find_compatible_node(NULL, NULL, compatible);
        if(!np)
        {
            return -1;
        }
        *pIrq = irq_of_parse_and_map(np, 0);

//        res = platform_get_resource(pOsDev, IORESOURCE_IRQ, 0);
//        *pIrq = res->start;

        return 0;
    }

    return -1;

}
