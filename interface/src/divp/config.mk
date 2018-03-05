include clear-config.mk
DEP_MODULE:=common sys vdec rgn gfx
DEP_HAL:=divp cmdq_service common
API_FILE:=divp_api.c
WRAPPER_FILE:=divp_ioctl.c
IMPL_FILES:=mi_divp_impl.c
include add-config.mk
