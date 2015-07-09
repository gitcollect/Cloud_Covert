#!/usr/bin/python
# File Name: process.py

"""
	This file is used to process the data from the xentrace output
"""

import os
import string
import math

FREQUENCY = 3.292624
CPU_MASK = "0x000000ff"
EVENT_MASK = "0x2f000"
TIME = "10"
DOMAIN_ID = "0x0000002d,"

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

def process():

	data = open('output', 'r')
	activities = data.readlines()
	data.close()

	time_list = [0]*len(activities)
	prob_list = [0]*30
	
	index = 0

	for activity in activities:
		activity_list = activity.split(' ')
		for i in range(len(activity_list)):
			if (activity_list[i] == "old_domid"):
				domid = activity_list[i+2]
				if (domid == DOMAIN_ID):
					time_list[index] = (int)(activity_list[i+5])
					index += 1

	num = 0
	for i in range(len(time_list)):
		if (time_list[i]>0):
			index = time_list[i]/1000000
			prob_list[index] += 1
			num += 1

	for i in range(30):
		prob_list[i] = prob_list[i] * 1.0/num
		print '{0}'.format(prob_list[i])
	
	entropy = 0
	for i in range(28):
		if (prob_list[i] > 0):
			entropy -= prob_list[i] * math.log(prob_list[i], 2)

	print 'entropy is {0}'.format(entropy)

#execute_cmd(CPU_MASK, EVENT_MASK, TIME)
process()
