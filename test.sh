#!/bin/bash

files=$(find -type f)

function getFileSize {
    ls -l $1 | awk '{print $5}'
}

for file in $files ; do
    ans=`getFileSize $file`
    ./fileclient $file 2> ../log
    ret=`cat ../log`
    echo "ret:$ret  ans:$ans"
    if [ $ret -ne $ans ] ; then
        echo "test $file failed ret:$ret  ans:$ans"
        exit 1
    fi
    echo "test $file success"
done

