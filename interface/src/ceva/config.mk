include clear-config.mk
#KAPI_DISABLE:=TRUE
DEP_MODULE:=common sys cv gfx
API_FILE:=ceva_api.c
WRAPPER_FILE:=
LIBS:=mi_sys
IMPL_FILES:= 
include add-config.mk
