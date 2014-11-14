#!/usr/bin/python
# File Name: process.py

"""
	This file is used to process the data from the xentrace output
"""

import os
import string


FREQUENCY = 3.292624
CPU_MASK = "0x000000ff"
EVENT_MASK = "0x2f000"
TIME = "10"
DOMAIN_ID1 = "0x00000015,"
DOMAIN_ID2 = "0x00000005,"

##
# Command:
# $ xentrace -D -c 0 -e 0x2f000 -T 10 result
# $ cat result | xentrace_formats format > output
#
def execute_cmd(cpu_mask, event_mask, time):
	cmd0 = "rm -rf result output"
	cmd1 = "xentrace" + " -D" + " -c " + cpu_mask + " -e " + event_mask + " -T " + time + " result"
	cmd2 = "cat result | xentrace_format formats | grep switch_infprev > output"
	os.popen(cmd0)
	os.popen(cmd1)
	os.popen(cmd2)

def process(tid_list):

	data = open('output', 'r')
	activities = data.readlines()
	data.close()

	vm_list = [0]*len(activities)*2
	time_list = [0]*len(activities)*2
	
	sum_time = 0
	index = 0

	for activity in activities:
		activity_list = activity.split(' ')
		for i in range(len(activity_list)):
			if (activity_list[i] == "old_domid"):
				domid = activity_list[i+2]
				if (domid == DOMAIN_ID1):
					vm_list[index] = 1
					time_list[index] = sum_time + 1
					index += 1
					sum_time += (int)(activity_list[i+5])
					vm_list[index] = 1
					time_list[index] = sum_time
					index += 1
				if (domid == DOMAIN_ID2):
					vm_list[index] = 0
					time_list[index] = sum_time + 1
					index += 1
					sum_time += (int)(activity_list[i+5])
					vm_list[index] = 0
					time_list[index] = sum_time
					index += 1

		
	for i in range(len(vm_list)):
		print '{0}, {1}'.format(time_list[i], vm_list[i])


#execute_cmd(CPU_MASK, EVENT_MASK, TIME)
process([DOMAIN_ID1,DOMAIN_ID2])
