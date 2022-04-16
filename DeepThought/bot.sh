#!/bin/bash
while true
do
OS="`uname`"
case $OS in
	'Linux')
		git fetch origin
		git reset --hard origin/main
 		make clean
 		make make -j$(nproc)
		./bot
		;;
	'NetBSD')
		git fetch origin
                git reset --hard origin/main
		make -f Makefile.bsd clean 
		make -f Makefile.bsd -j4
		export LD_LIBRARY_PATH=/usr/pkg/lib
		./bot
		;;
esac	
done
