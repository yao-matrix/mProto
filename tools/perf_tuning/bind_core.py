import sys
import os
import re

# make sure all cores are on
for i in xrange(88):
	cmd = "echo 1 > /sys/devices/system/cpu/cpu%d/online" % (i)
	os.system(cmd)


regrex = re.compile('port=\d+')
i = 0
j = 4
for line in sys.stdin.readlines():
	fields = line.strip().split()
	# print fields[1]
	# print line
	candidates = regrex.findall(line)
	if len(candidates) == 1:
		process_name = None
		print "port ID: %s, process ID: %s" % (candidates[0][5:].strip(), fields[1])
		process_id = fields[1]
		if line.find('TranscodingTopology') != -1:
			process_name = 'transcoder'
			cmd = "echo 0 > /sys/devices/system/cpu/cpu%d/online" % (i + 44)
			os.system(cmd)
			print cmd
			cmd = "taskset -a -cp %d %s" % (i, process_id)
			# print cmd
			os.system(cmd)
			print "bind process %s[%s] to CPU[%d]" % (process_id, process_name, i)
			i += 1
		else:
			process_name = 'vis'
			cmd = "taskset -a -cp %d,%d %s" % (j, j + 44, process_id)
			# print cmd
			os.system(cmd)
			print "bind process %s[%s] to CPU[%d, %d]" % (process_id, process_name, j, j + 44)
			j += 1

