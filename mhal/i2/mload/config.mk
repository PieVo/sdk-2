include raw-impl-config.mk
DEPS:=mload

DRV_PUB_INCS:=pub/mdrv_mload.h
DRV_INC_PATH:=inc pub ../../cmdq_service/drv/pub ../../../include/common ../hal/infinity2/inc
DRV_SRCS:=src/common/mload_common.c \
src/linux/drv_mload.c \
src/linux/mdrv_mload.c

HAL_INC_PATH:=infinity2/inc ../drv/inc ../drv/pub ../../cmdq_service/drv/pub ../../../include/common
HAL_SRCS:=infinity2/src/hal_mload.c
include add-config.mk
