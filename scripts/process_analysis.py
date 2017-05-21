import sys
import os
import re

# import pandas as pd

regrex = re.compile('port=\d+')

process_info = {}

for line in sys.stdin.readlines():
	fields = line.strip().split()
	# print fields[1]
	# print line
	candidates = regrex.findall(line)
	if len(candidates) == 1:
		# print "port ID: %s, process ID: %s" % (candidates[0][5:].strip(), fields[1])
		process_info[fields[1]] = [candidates[0][5:].strip()]
	
for line in os.popen("top -bi -n 2").readlines():
	records = line.strip().split()
	# print records
	if len(records) > 0 and records[0].isdigit():
		if records[0] in process_info:
			process_info[records[0]].append(records[8].strip())
			process_info[records[0]].append(records[5].strip())


print "process_id  port_id  cpu_occupance physical_memory"
print "=================================================="
for k, v in process_info.iteritems():
	print "%s      %s       %s%%         %s" % (k, v[0], v[1], v[2])
# print process_info
# df = pd.DataFrame(process_info)
# print df 
