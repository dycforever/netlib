#!/bin/bash

rm -rf chunk_* log/* cc
strace -f ./build/example/http_client >cc 2>cc2

