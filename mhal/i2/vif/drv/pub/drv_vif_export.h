#ifndef DRV_VIF_EXPORT_H
#define DRV_VIF_EXPORT_H

typedef enum {
    VIF_CHANNEL_0,
    VIF_CHANNEL_1,
    VIF_CHANNEL_2,
    VIF_CHANNEL_3,
    VIF_CHANNEL_4,
    VIF_CHANNEL_5,
    VIF_CHANNEL_6,
    VIF_CHANNEL_7,
    VIF_CHANNEL_8,
    VIF_CHANNEL_9,
    VIF_CHANNEL_10,
    VIF_CHANNEL_11,
    VIF_CHANNEL_12,
    VIF_CHANNEL_13,
    VIF_CHANNEL_14,
    VIF_CHANNEL_15,
    VIF_CHANNEL_NUM,
} VIF_CHANNEL_e;

typedef enum {
    VIF_INTERRUPT_VREG_RISING_EDGE,
    VIF_INTERRUPT_VREG_FALLING_EDGE,
    VIF_INTERRUPT_HW_FLASH_STROBE_DONE,
    VIF_INTERRUPT_PAD2VIF_VSYNC_RISING_EDGE,
    VIF_INTERRUPT_PAD2VIF_VSYNC_FALLING_EDGE,
    VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT0,
    VIF_INTERRUPT_VIF_TO_ISP_LINE_COUNT_HIT1,
    VIF_INTERRUPT_BT656_CHANNEL_DETECT_DONE,
    VIF_INTERRUPT_FINAL_STATUS_MAX
} VIF_INTERRUPT_e;


int DrvVif_InputMask(int ch,int OnOff);
int DrvVif_IntStatusCbReg(VIF_CHANNEL_e ch, VIF_INTERRUPT_e st, void (*cb)(void));
int DrvVif_EnableInterrupt(VIF_CHANNEL_e ch, unsigned char en);
int DrvVif_LineCountCfg(VIF_CHANNEL_e ch, unsigned int linecnt0, unsigned int linecnt1);

int DrvVif_SetDmagMLineCntInt(VIF_CHANNEL_e ch, unsigned char en);
int DrvVif_SetDmagMLineCnt(VIF_CHANNEL_e ch, unsigned int cnt);
int DrvVif_RegDmagMCb(void (*cb)(void));

#endif
