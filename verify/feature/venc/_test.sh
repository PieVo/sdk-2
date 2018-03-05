#!/bin/sh

. ./_config.sh

#out_path=/tmp
#dbg_level=/sys/module/mi_venc/parameters/debug_level

PASS="\e[32mPass\e[0m"
FAIL="\e[31mFail\e[0m"

total=0
pass=0
fail=0

test() {
	#echo ts $1, $2 ${LINENO}}
	total=`expr $total + 1`
	if [ "$2" == "0" ];then (>&2 echo -e $1 "==>" $FAIL); fail=`expr $fail + 1`;
	else (>&2 echo -e $1 "==>" $PASS); pass=`expr $pass + 1`; fi
}

print_result() {
	(>&2 printf "--------------------------------\nTotal $total tests, ")
	#if [ "$fail" == "0" ]; then (>&2 echo -e "\e[32m${fail} Failed\e[0m");
	if [ "$fail" == "0" ]; then (>&2 echo -e "\e[32mAll passed.\e[0m");
	else (>&2 echo -e "\e[31m${fail} Failed\e[0m"); fi
}

log() {
	echo -e "\n\n""\e[33m""$1""\e[0m""\n\n"
	(>&2 echo -e "\e[36m""$1""\e[0m")
	#(>&2 printf $1)
	#(>&2 echo -e "1\neee\n" "$1")
}