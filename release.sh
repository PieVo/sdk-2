#!/bin/bash


dir_array=("impl" "interface" "mhal" "misc" );

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


    if [ "$v"x = "interface"x ]; then
        #rm ./interface/module.mk
        #mv ./interface/module_relase.mk ./interface/module.mk
        sed -i "s/@ln -sf \$(CURDIR)\/modules.mk/@ln -sf \$(CURDIR)\/modules_release.mk/" ./interface/makefile
    elif [ "$v"x = "misc"x ]; then
        sed -i "/\$(CC) -o config_tool \$\^/s/^/#/"  ./misc/config_tool/makefile
    else
        echo "no need modify makefile."
    fi

done

sed -i "s/^clean_all:\s.*/clean_all:/" ./sdk.mk
sed -i "/\$(MAKE) -C \$(PROJ_ROOT)\/\.\.\/sdk\/misc clean/s/^/#/"   ./sdk.mk

# remove testmodule no need relase.
detele_array=("module_test" "rgn" "venc" "vif" "vpe" );

path="./verify/feature"

for v in ${detele_array[@]}; do
    echo "remove ${path}/$v"
    rm -rf ${path}/$v
done


