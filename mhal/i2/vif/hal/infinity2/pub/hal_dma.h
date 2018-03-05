#ifndef HAL_DMA_H
#define HAL_DMA_H
#include <vif_datatype.h>

typedef struct
{
    u32 uX;
    u32 uY;
    u32 uW;
    u32 uH;
}WdmaCropParam_t;

typedef enum
{
    WDMA_TRIG_SINGLE = 0,
    WDMA_TRIG_CONTINUE = 1,
    WDMA_TRIG_STOP = 2,
}WdmaTrigMode_e;

typedef struct
{
    u32 uWidth;
    u32 uHeight;

}WdmaInfo_t;

void HalDma_Init(void);
void HalDma_Uninit(void);
int HalDma_GlobalEnable(void);
int HalDma_Config(u32 uCh,WdmaCropParam_t *ptCrop,u32 uMiuBaseY,u32 uMiuBaseC);
int HalDma_ConfigSub(u32 uCh,WdmaCropParam_t *ptCrop,u32 uMiuBaseY,u32 uMiuBaseC);
int HalDma_Trigger(u32 uCh,WdmaTrigMode_e eMode);
int HalDma_TriggerSub(u32 uCh,WdmaTrigMode_e eMode);
int HalDma_EnableIrq(u32 nCh);
int HalDma_DisableIrq(u32 nCh);
int HalDma_DmaMaskEnable(u32 nCh,u8 enable);
int HalDma_EnableGroup(u32 uGroup);
int HalDma_DisableGroup(u32 uGroup);
int HalDma_ConfigGroup(u32 uGroup,u32 uMaxChns);
int HalDma_MaskOutput(u32 nCh,u8 uMask);
int HalDma_MaskOutputSub(u32 uCh,u8 uMask);
int HalDma_SetOutputAddr(u32 uCh,u32 uOutAddrY,u32 uOutAddrC,u32 uPitch);
int HalDma_SetOutputAddrSub(u32 uCh,u32 uOutAddrY,u32 uOutAddrC,u32 uPitch);

unsigned int HalDma_DoneIrqFinalStatus(VIF_CHANNEL_e ch);
void HalDma_DoneIrqMask(VIF_CHANNEL_e ch, unsigned int mask);
void HalDma_DoneIrqUnMask(VIF_CHANNEL_e ch, unsigned int mask);
void HalDma_DoneIrqClr(VIF_CHANNEL_e ch, unsigned int mask);
void HalDma_SetDmagMLineCnt(VIF_CHANNEL_e ch, unsigned int cnt);
void HalDma_GetDmaInfo(VIF_CHANNEL_e ch,WdmaInfo_t *info);
void HalDma_GetSubDmaInfo(VIF_CHANNEL_e ch,WdmaInfo_t *info);

#endif
