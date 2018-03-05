#!/bin/bash


dir_array=("mhal" );

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "@start relase source code !!!@"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

#clean source file and temp file, modify makefile.
for v in ${dir_array[@]}; do
    echo "clean source file in $v"

    find ./$v -name "*.ko.cmd" -exec rm -rf {} \;

    find ./$v -name "*.o.cmd" -exec rm -rf {} \;

    find ./$v -name "modules.order" -exec rm -rf {} \;

    find ./$v -name "Module.symvers" -exec rm -rf {} \;

    find ./$v -name "*.c" -exec rm -rf {} \;

    find ./$v -name ".tmp_versions" -exec rm -rf {} \;


    echo "no need modify makefile."

done

sed -i "/^clean_all:\s.*/s/\sclean_mhal//" ./sdk.mk

echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
echo "@relase script run finished!  @"
echo "@pelase ===make clean===.     @"
echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"

