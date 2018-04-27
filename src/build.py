#!/usr/bin/env python

#doesn't work properly yet

#usage:
#python3 make.py -n "netid"

import os
import subprocess
import argparse

def main():
	parser = argparse.ArgumentParser(description = 'get args')
	parser.add_argument('-n', '--netid', type=str, help='a target IP address', required=True)
	
	args = vars(parser.parse_args())
	netid = args['netid']
	
	go_back_cd = 'cd ..'
	make_clean = 'make clean'
	make = 'make'
	go_to_src = 'cd src'
	unmount = 'fusermount -u /tmp/' +netid+ '/mountdir'
	mount = './sfs /.freespace/'+netid+'/testfsfile /tmp/'+netid+'/mountdir'
	check = 'mount | grep sfs'

	os.chdir('./..')
	#os.system('ls')
	os.system(make_clean)
	os.system(make)
	os.chdir('./src')
	os.system(unmount)
	os.system(mount)
	os.system(check)


if __name__ == "__main__":
	main()
	