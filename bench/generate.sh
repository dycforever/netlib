#!/usr/bin/bash

host=m.sm.cn
ip=127.0.0.1
port=8714

# ./main -i $ip -p $port -u /lua_fgz -v 0 > fgz.0.out
# ./main -i $ip -p $port -u /lua_fgz > fgz.1.out
# 
 ./main -i $ip -p $port -u /lua_gz2 -v 0 > gz2.0.out
 ./main -i $ip -p $port -u /lua_gz2 > gz2.1.out
# 
# ./main -i $ip -p $port -u /lua_gz -v 0 > gz.0.out
# ./main -i $ip -p $port -u /lua_gz > gz.1.out

for f in *.out ; do echo "========== $f:   "; cat $f; done | less