#!/bin/bash

if [ c$1 = cc ]
then
#    scons -c && scons -j8
    scons -j8
fi

rm -rf chunk_* log/* cc*
./build/example/trival_client >cc 2>cc2
#valgrind --leak-check=full --log-file=./val.log ./build/example/trival_client >cc 2>cc2
#valgrind --leak-check=full --log-file=./val.log ./build/example/http_client >cc 2>cc2

