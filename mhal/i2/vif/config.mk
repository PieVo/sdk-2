include raw-impl-config.mk

DEPS:=vif
DRV_PUB_INCS:=pub/mhal_vif.h pub/mhal_vif_datatype.h
DRV_INC_PATH:= ./inc ./pub  ../../cmdq_service/drv/pub ../../../include/common ../hal/infinity2/pub
DRV_SRCS:= src/linux/mhal_vif.c src/drv_vif.c  src/linux/vif_sys_linux.c src/common/vif_common.c

HAL_INC_PATH:=../drv/inc ../drv/pub  ../../cmdq_service/drv/pub ../../../include/common infinity2/inc infinity2/pub
HAL_SRCS:= infinity2/hal_dma.c  infinity2/hal_vif.c infinity2/src/hal_rawdma.c
include add-config.mk
