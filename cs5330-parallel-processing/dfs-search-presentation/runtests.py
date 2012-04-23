#!/usr/bin/python
#
# David Ells 
# Script for generating test results using dfs-search program

import os, sys
thread_nums = (1,2,3,4,8,16,32,64,128)

if len(sys.argv) != 4:
    print sys.argv[0], '[file] [search_val] [b|r]'
    sys.exit(1)

fname = sys.argv[1]
val = int(sys.argv[2])
mode = sys.argv[3]

if mode[0] == 'r':
    print './dfs-search', fname, val, thread_nums
    for i in thread_nums:
        os.system("./dfs-search %s %d %d" % (fname, val, i))

elif mode[0] == 'b':
    print './dfs-search -b', fname, val, thread_nums
    for i in thread_nums:
        os.system("./dfs-search -b %s %d %d" % (fname, val, i))
