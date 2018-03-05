include clear-config.mk
#KAPI_DISABLE:=TRUE
DEP_MODULE:=common sys warp gfx
API_FILE:=cv_api.c
WRAPPER_FILE:=cv_ioctl.c
LIBS:=mi_sys
IMPL_FILES:= mi_cv_impl.c
include add-config.mk
