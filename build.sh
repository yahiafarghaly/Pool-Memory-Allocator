#!/bin/sh

rm *.out *~ 2> /dev/zero

echo "Standard C++ new/delete"
g++ -O3 -Wall -DNDEBUG -std=c++11 -s main.cc complex.cc -o STD_MEM.out -w
./STD_MEM.out
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
echo "Pool Memory Technique" 
g++ -O3 -Wall -DNDEBUG -std=c++11 -s main.cc complex.cc -o POOL_MEM.out -DPOOL_MEMORY -DOVERLOADED_MEMORY -w
./POOL_MEM.out
