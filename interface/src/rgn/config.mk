include clear-config.mk
DEP_MODULE:=common sys gfx
DEP_HAL:=common cmdq_service rgn
API_FILE:=rgn_api.c
WRAPPER_FILE:=rgn_ioctl.c
IMPL_FILES:= mi_rgn_impl.c mi_rgn_drv.c
include add-config.mk
