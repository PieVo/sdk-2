#ifndef VIF_DATATYPE_H
#define VIF_DATATYPE_H

typedef enum {
    VIF_CHANNEL_0 = 0,
    VIF_CHANNEL_1 = 1,
    VIF_CHANNEL_2 = 2,
    VIF_CHANNEL_3 = 3,
    VIF_CHANNEL_4 = 4,
    VIF_CHANNEL_5 = 5,
    VIF_CHANNEL_6 = 6,
    VIF_CHANNEL_7 = 7,
    VIF_CHANNEL_8 = 8,
    VIF_CHANNEL_9 = 9,
    VIF_CHANNEL_10= 10,
    VIF_CHANNEL_11= 11,
    VIF_CHANNEL_12= 12,
    VIF_CHANNEL_13= 13,
    VIF_CHANNEL_14= 14,
    VIF_CHANNEL_15= 15,
    VIF_CHANNEL_NUM,
} VIF_CHANNEL_e;

typedef enum {
    VIF_DISABLE = 0,
    VIF_ENABLE = 1,
} VIF_ONOFF_e;

typedef enum {
    VIF_CH_SRC_MIPI_0 = 0,
    VIF_CH_SRC_MIPI_1 = 1,
    VIF_CH_SRC_MIPI_2 = 2,
    VIF_CH_SRC_MIPI_3 = 3,
    VIF_CH_SRC_PARALLEL_SENSOR_0 = 4,
    VIF_CH_SRC_PARALLEL_SENSOR_1 = 5,
    VIF_CH_SRC_PARALLEL_SENSOR_2 = 6,
    VIF_CH_SRC_PARALLEL_SENSOR_3 = 7,
    VIF_CH_SRC_BT656 = 8,
    VIF_CH_SRC_MAX = 9,
} VIF_CHANNEL_SOURCE_e;

typedef enum {
    VIF_HDR_SRC_MIPI0,
    VIF_HDR_SRC_MIPI1,
    VIF_HDR_SRC_MIPI2,
    VIF_HDR_SRC_MIPI3,
    VIF_HDR_SRC_HISPI0,
    VIF_HDR_SRC_HISPI1,
    VIF_HDR_SRC_HISPI2,
    VIF_HDR_SRC_HISPI3,
    VIF_HDR_VC0 = 0,
    VIF_HDR_VC1 = 1,
    VIF_HDR_VC2 = 2,
    VIF_HDR_VC3 = 3,
} VIF_HDR_SOURCE_e;

typedef enum {
    VIF_STROBE_POLARITY_HIGH_ACTIVE,
    VIF_STROBE_POLARITY_LOW_ACTIVE,
} VIF_STROBE_POLARITY_e;

typedef enum {
    VIF_STROBE_VERTICAL_ACTIVE_START,
    VIF_STROBE_VERTICAL_BLANKING_START,
} VIF_STROBE_VERTICAL_START_e;

typedef enum {
    VIF_STROBE_SHORT,
    VIF_STROBE_LONG,
} VIF_STROBE_MODE_e;

typedef enum {
    VIF_SENSOR_POLARITY_HIGH_ACTIVE = 0,
    VIF_SENSOR_POLARITY_LOW_ACTIVE  = 1,
} VIF_SENSOR_POLARITY_e;

typedef enum {
    VIF_SENSOR_FORMAT_8BIT = 0,
    VIF_SENSOR_FORMAT_10BIT = 1,
    VIF_SENSOR_FORMAT_16BIT = 2,
    VIF_SENSOR_FORMAT_12BIT = 3,
} VIF_SENSOR_FORMAT_e;

typedef enum {
    VIF_SENSOR_INPUT_FORMAT_YUV422,
    VIF_SENSOR_INPUT_FORMAT_RGB,
} VIF_SENSOR_INPUT_FORMAT_e;

typedef enum {
    VIF_SENSOR_BIT_MODE_0,
    VIF_SENSOR_BIT_MODE_1,
    VIF_SENSOR_BIT_MODE_2,
    VIF_SENSOR_BIT_MODE_3,
} VIF_SENSOR_BIT_MODE_e;

typedef enum {
    VIF_SENSOR_YC_SEPARATE,
    VIF_SENSOR_YC_16BIT,
} VIF_SENSOR_YC_INPUT_FORMAT_e;

typedef enum {
    VIF_SENSOR_VS_FALLING_EDGE,
    VIF_SENSOR_VS_FALLING_EDGE_DELAY_2_LINE,
    VIF_SENSOR_VS_FALLING_EDGE_DELAY_1_LINE,
    VIF_SENSOR_VS_RISING_EDGE,
} VIF_SENSOR_VS_DELAY_e;

typedef enum {
    VIF_SENSOR_HS_RISING_EDGE,
    VIF_SENSOR_HS_FALLING_EDGE,
} VIF_SENSOR_HS_DELAY_e;

typedef enum {
    VIF_SENSOR_BT656_YC,
    VIF_SENSOR_BT656_CY,
} VIF_SENSOR_BT656_FORMAT_e;

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

typedef enum {
    VIF_RGRG_GBGB,
    VIF_GRGR_BGBG,
    VIF_BGBG_GRGR,
    VIF_GBGB_RGRG,
} VIF_BAYER_MODE_e;

typedef enum {
    VIF_CYCY,
    VIF_YCYC,
} VIF_INPUT_PRIORITY_YUV_e;

typedef enum {
    VIF_SUCCESS,
    VIF_FAIL,
    VIF_ISR_CREATE_FAIL,
} VIF_STATUS_e;

typedef enum
{
    VIF_CH0_FRAME_START_INTS = 0x01,
    VIF_CH0_FRAME_END_INTS = 0x02,
    VIF_CH1_FRAME_START_INTS = 0x04,
    VIF_CH1_FRAME_END_INTS = 0x08,
    VIF_CH2_FRAME_START_INTS = 0x10,
    VIF_CH2_FRAME_END_INTS = 0x20,
} VIF_INTS_EVENT_TYPE_e;

typedef enum
{
    VIF_CLK_CSI2_MAC_0 = 0,//000: clk_csi2_mac_toblk0_buf0_p
    VIF_CLK_CSI2_MAC_1 = 1,//001: clk_csi2_mac_toblk0_4_buf0_p
    VIF_CLK_BT656_P0_0_P = 2,//010: clk_bt656_p0_0_p
    VIF_CLK_BT656_P0_1_P = 3,//011: clk_bt656_p0_1_p
    VIF_CLK_BT656_P1_0_P = 4,//100: clk_bt656_p1_0_p
    VIF_CLK_BT656_P1_1_P = 5,//101: clk_bt656_p1_1_p*/
    VIF_CLK_PARALLEL = 6,//110: clk_parllel*/
    VIF_CLK_FPGA_BT656 = 7, //111: TBD.
}  VIF_CLK_TYPE_e;

typedef enum
{
    VIF_BT656_EAV_DETECT = 0,
    VIF_BT656_SAV_DETECT = 1,
} VIF_BT656_CHANNEL_SELECT_e;

typedef enum
{
    VIF_BT656_VSYNC_DELAY_1LINE = 0,
    VIF_BT656_VSYNC_DELAY_2LINE = 1,
    VIF_BT656_VSYNC_DELAY_0LINE = 2,
    VIF_BT656_VSYNC_DELAY_BT656 = 3,
} VIF_BT656_VSYNC_DELAY_e;

typedef enum
{
    SENSOR_PAD_GROUP_A = 0,
    SENSOR_PAD_GROUP_B = 1,
} SENSOR_PAD_GROUP_e;

typedef struct VifCfg_s
{
    VIF_CHANNEL_SOURCE_e eSrc;
    VIF_HDR_SOURCE_e eHdrSrc;
    VIF_SENSOR_POLARITY_e HsyncPol;
    VIF_SENSOR_POLARITY_e VsyncPol;
    VIF_SENSOR_POLARITY_e ResetPinPol;
    VIF_SENSOR_POLARITY_e PdwnPinPol;
    VIF_SENSOR_FORMAT_e ePixelDepth;
    VIF_BAYER_MODE_e eBayerID;
    uint32_t FrameStartLineCount; // lineCount0 for FS ints
    uint32_t FrameEndLineCount;   // lineCount1 for FE ints
} VifSensorCfg_t;

typedef struct VifBT656Cfg_s
{
    uint32_t bt656_ch_det_en;
    VIF_BT656_CHANNEL_SELECT_e bt656_ch_det_sel;
    uint32_t bt656_bit_swap;
    uint32_t bt656_8bit_mode;
    VIF_BT656_VSYNC_DELAY_e bt656_vsync_delay;
    uint32_t bt656_hsync_inv;
    uint32_t bt656_vsync_inv;
    uint32_t bt656_clamp_en;
} VifBT656Cfg_t;

typedef struct VifPatGenCfg_s
{
    uint32_t nWidth;
    uint32_t nHeight;
    uint32_t nFps;
} VifPatGenCfg_t;

typedef enum
{
    VIF_CLK_POL_POS = 0,
    VIF_CLK_POL_NEG
} VIF_CLK_POL;

typedef enum
{
    PIN_POL_POS = 0,
    PIN_POL_NEG = 1
}PIN_POL;

typedef enum
{
    PCLK_MIPI_0 = 0x0,
    PCLK_MIPI_1 = 0x1,
    PCLK_MIPI_2 = 0x2,
    PCLK_MIPI_3 = 0x3,
    PLCK_BT656_P0=0x4,
    PLCK_BT656_P1=0x5,
    PLCK_SENSOR =0x6,
}VifPclk_e;

typedef enum
{
    MCLK_6M = 0x0,
    MCLK_12M = 0x1,
    MCLK_24M = 0x2,
    MCLK_27M = 0x3,
    MCLK_36M =0x4,
    MCLK_37P6M =0x5,
    MCLK_54M =0x6,
    MCLK_75M =0x7,
}VifMclk_e;

typedef struct
{
	uint32_t uReceiveWidth;
	uint32_t uReceiveHeight;
}VifInfo_t;

#endif
