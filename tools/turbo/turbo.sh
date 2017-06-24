#!/bin/sh

# Prerequisites:
# 1. download and install MSR(Model-Specific Register) from:
#     https://01.org/msr-tools/downloads

# Examples:
#  turn on Turbo:
#    ./turbo.sh on
#  turn off Turbo:
#    ./turbo.sh off
#  query Turbo status:
#    ./turbo.sh

cores=$(cat /proc/cpuinfo | grep processor | awk '{print $3}')

# echo ${cores}
if [[ $1 == "off" ]]; then
	echo "Turning off all core's Turbo"
	for core in $cores
	do
		wrmsr -p${core} 0x1a0 0x4000850089
	done
elif [[ $1 == "on" ]]; then
	echo "Turning on all core's Turbo"
	for core in $cores
	do
		wrmsr -p${core} 0x1a0 0x850089
	done
fi

echo "Check all core's Turbo status"
for core in ${cores}
do
	status=`rdmsr -p${core} 0x1a0 -f 38:38`
	if [[ ${status} == 1 ]]; then
		echo "Core[${core}] status: OFF"
	else
		echo "Core[${core}] status: ON"
	fi
done

echo "DONE"
