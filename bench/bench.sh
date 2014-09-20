#!/usr/bin/bash

ipaddr=127.0.0.1
port=8714

function test_gz2_chunk() {
    ./main -i $ipaddr -p $port -u /lua_gz2 > out
    diff gz2.1.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi

    ./main -i $ipaddr -p $port -u /lua_gz2 -v 0 > out
    diff gz2.0.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi
}

function test_gz_chunk() {
    ./main -i $ipaddr -p $port -u /lua_gz > out
    diff gz.1.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi

    ./main -i $ipaddr -p $port -u /lua_gz -v 0 > out
    diff gz.0.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi
}


function test_fgz_chunk() {
    ./main -i $ipaddr -p $port -u /lua_fgz > out
    diff fgz.1.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi

    ./main -i $ipaddr -p $port -u /lua_fgz -v 0 > out
    diff fgz.0.out out
    if [ $? -ne 0 ] ; then
        echo "found diff"
        exit -1
    fi
}

rm -rf out
for ((i=0;i<100000;i++))
do
    test_gz2_chunk
    test_gz_chunk
    test_fgz_chunk
    echo -e -n "i=$i\r"
done