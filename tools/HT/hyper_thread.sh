#!/bin/sh

# Examples:
#  turn on HT:
#    ./hyper_thread.sh on
#  turn off HT:
#    ./hyper_thread.sh off
#  query HT status
#    ./hyper_thread.sh

action=$1  # actrion should only be one of 'on', 'off', will do check anyway

# check HT status
core_num=`ls -l /sys/devices/system/cpu/ | grep "cpu[0-9]*$" | wc -l`
online_cores=`cat /sys/devices/system/cpu/cpu*/online | grep -o '1' | wc -l`
offline_cores=`cat /sys/devices/system/cpu/cpu*/online | grep -o '0' | wc -l`
# echo ${offline_cores}
ht=0
if [ ${offline_cores} == ${online_cores} ]
then
	echo "Hyper Threading is OFF"
else
	echo "Hyper Threading is ON"
	ht=1
fi

# echo ${ht}

# enable HT
if [ ${action} == "on" ]
then
	if [ ${ht} == 0 ]
	then
		num=`expr ${core_num} - 1`
		for cpu_id in $(seq 0 1 ${num})
		do
			echo "enabling: "$cpu_id
			echo 1 > /sys/devices/system/cpu/cpu${cpu_id}/online
		done
	else
		echo "Hyper Threading has been enabled, nothing to do"
	fi
fi

# disable HT
if [ ${action} == "off" ]
then
	if [ ${ht} == 1 ]
	then
		CPUFILE=./THREAD_SIBLINGS_LIST
		cat /sys/devices/system/cpu/cpu*/topology/thread_siblings_list >$CPUFILE

		for cpu_id in $(cat $CPUFILE | cut -s -d, -f2- | tr ',' '\n' | sort -un)
		do
			echo "disabling: "$cpu_id
			echo 0 > /sys/devices/system/cpu/cpu$cpu_id/online
		done
		rm -f ${CPUFILE}
	else
		echo "Hyper Threading has been disabled, nothing to do"
	fi
fi

echo "DONE"

