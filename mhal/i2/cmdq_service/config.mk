include raw-impl-config.mk
#MACROS:=HAL_I2_SIMULATE
#DEPS:=simple raw_drv

DRV_PUB_INCS:=pub/mhal_cmdq.h
DRV_INC_PATH:= ./inc ./inc/linux ./pub ../../../include/common
DRV_SRCS:=src/drv_cmdq.c src/mhal_cmdq.c src/linux/drv_cmdq_os.c src/linux/drv_cmdq_irq.c \
          src/linux/cmdq_proc.c

HAL_PUB_PATH:= ./pub
HAL_INC_PATH:=../drv/inc/linux
HAL_SRCS:=src/hal_cmdq.c
include add-config.mk
