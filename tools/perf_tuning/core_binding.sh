i=0
ps aux | grep 'java' | grep 'port=' | awk '{print $2}' | while read line ;
do
	j=$(($i+44))
	# echo $j
	taskset -a -cp $i,$j $line
	echo core binding with core = $i, process = $line
	((i++))
done
