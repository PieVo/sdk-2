#!/bin/sh

. ./_test.sh

script=$(basename "$0")
script="${script%.*}"

#debug_level 3 would cause much delay and causing MI gets every interrupts.
#use debug_level 0 to make
echo 0 > $dbg_level #/sys/module/mi_venc/parameters/debug_level
cat $dbg_level

log "\n\n${script}: 2 channels ${yuv}.\n"

${exe} 2 h264 h264
ret=$?

test "The program returns ${ret}." `expr "${ret}" == "0"`

#device2.ch00.ElementaryStream
out_file0=enc_d2c00p00.es
result=`md5sum $out_path/${out_file0} | cut -c-32`
test "Check ${out_path}/${out_file0} ${out_file0}" `expr "${result}" == "${md5ch0}"`

out_file1=enc_d2c01p00.es
result=`md5sum $out_path/${out_file1} | cut -c-32`
test "Check ${out_path}/${out_file1}" `expr "${result}" == "${md5ch1}"`
#echo result=${result}

print_result
