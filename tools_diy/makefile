ping:myping.c ping_common.c ping_common.h
	gcc -O3 -fno-strict-aliasing -Wstrict-prototypes -Wall -g -D_GNU_SOURCE ping_common.c myping.c -o ping

.PHONY:clean
clean:
	rm -rf ping
