How to run it
=============

* compile the thing. on my mac, it goes "make clean all"
* create some searching input
run ./randints <random seed> <number of ints> > <output file>
example of creating 10M random ints:

    ./randints 2765 10000000 > 10M.txt

* see dfs-search help to see what it does

    ./dfs-search -h

* run dfs-search on the input

    ./dfs-search 10M.txt -1 4

the above runs on 10M.txt, looking for value -1 (which will never be found), using 4 threads
