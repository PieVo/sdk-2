#!/bin/sh

#config the variable to sync with Linux environment
out_path=/tmp
dbg_level=/proc/mi_modules/mi_venc/debug_level
exe=./feature_venc #the name of the executables, which are executable without path.
result=result

#set config
export VENC_GLOB_AUTO_STOP=1

#Set this value align with venc include files
#yuv="352x288 5 frames"
yuv="352x288 10 frames"
#yuv="320x240 10 frames"


#append your answers if needed.
if [ "${yuv}" == "320x240 10 frames" ]; then
md5ch0=4c31debc017d23b2ec80e4868ec9f4b4
md5ch1=06146b0e937279e05fdfc4e80c16d3de
elif [ "${yuv}" = "352x288 5 frames" ]; then
md5ch0=bfbdad3193faea0f7ab4c576e3383e0c
md5ch1=dc27ba616075ad6cf63e0662ec926535
elif [ "${yuv}" = "352x288 10 frames" ]; then
md5ch0=bfbdad3193faea0f7ab4c576e3383e0c
md5ch1=dc27ba616075ad6cf63e0662ec926535
fi