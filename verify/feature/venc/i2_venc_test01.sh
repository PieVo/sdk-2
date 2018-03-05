#!/bin/sh

. ./_test.sh

script=$(basename "$0")
script="${script%.*}"

echo 3 > $dbg_level

log "\n\n${script}: 1 channel ${yuv}.\n"

${exe} 1 h264
ret=$?

test "The program returns ${ret}." `expr "${ret}" == "0"`

#device2.ch00.ElementaryStream
out_file0=enc_d2c00p00.es
result=`md5sum $out_path/${out_file0} | cut -c-32`
test "Check ${out_path}/${out_file0} ${out_file0}" `expr "${result}" == "${md5ch0}"`
#echo "result = ${result}, md5ch0 = ${md5ch0}"

print_result
