include clear-config.mk
CFLAGS:=-O0 -W -Wall
SRCS:=venc.c venc_common.c env.c
LIBS:=mi_sys mi_venc
include add-config.mk