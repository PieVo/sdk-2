#include "module_test_main.h"

#define RAW_WIDTH    1280
#define RAW_HEIGHT   720
#define INPUT_NAME   "Img1280x720_16bit.raw"
#define OUTPUT_NAME  "Output_16BitTo8Bit.raw"

MI_S32 ModuleTest_16BitTo8Bit()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
	MI_IVE_SrcImage_t src;
	MI_IVE_DstImage_t dst;
    MI_IVE_16bitTo8BitCtrl_t ctrl =
    {
        .eMode          = E_MI_IVE_16BIT_TO_8BIT_MODE_U16_TO_U8,
        .u16Denominator = 256,
        .u8Numerator    = 1,
        .s8Bias         = 0
    };

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    // Init IVE
    ret = MI_IVE_Create(handle);
	if (ret != MI_SUCCESS)
	{
		printf("Could not create IVE handle\n");
		return ret;
	}

    // Allocate input buffer
    ret = ModuleTest_AllocateImage(&src, E_MI_IVE_IMAGE_TYPE_U16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = ModuleTest_AllocateImage(&dst, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_IVE_16BitTo8Bit()
	ret = MI_IVE_16BitTo8Bit(handle, &src, &dst, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_16BitTo8Bit() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputImage(&dst, OUTPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save data to output file %s\n", OUTPUT_NAME);
        goto RETURN_1;
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME " " RESULT_PATH OUTPUT_NAME) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    ModuleTest_FreeImage(&dst);
RETURN_2:
    ModuleTest_FreeImage(&src);
RETURN_3:
    MI_IVE_Destroy(handle);

    return ret;
}
