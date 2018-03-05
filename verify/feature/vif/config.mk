include clear-config.mk
CFLAGS:=-O0 -DCAM_OS_LINUX_USER -Wall -Werror -fno-builtin-log
SRCS:=mi_vif_test_util.c mi_vif_test_util_bind.c mi_vif_test_util_vif.c mi_vif_test_util_vpe.c mi_vif_test_util_venc.c mi_vif_test_util_divp.c mi_vif_test_util_disp.c mi_vif_test_main.c i2c.c memmap.c
LIBS:=mi_sys mi_vif mi_vpe mi_venc mi_divp mi_disp mi_hdmi
ifeq ($(CHIP),i2)
CFLAGS += -DTEST_VIF_ENABLE_LIBADDA
LIBS+=adda
endif
include add-config.mk
