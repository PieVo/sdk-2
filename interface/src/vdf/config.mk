include clear-config.mk
#KAPI_DISABLE:=TRUE
DEP_MODULE:=common sys shadow
API_FILE:=vdf_api.c
WRAPPER_FILE:=
IMPL_FILES:=
include add-config.mk
