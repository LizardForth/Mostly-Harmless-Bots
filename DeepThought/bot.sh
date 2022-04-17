#!/bin/bash
while true
do
OS="`uname`"
case $OS in
	'Linux')
		git fetch origin
		git reset --hard origin/main
 		make clean
 		make -j$(nproc)
 		clear
		./bot
		;;
	'NetBSD')
		git fetch origin
                git reset --hard origin/main
		make -f Makefile.bsd clean 
		make -f Makefile.bsd -j4
		export LD_LIBRARY_PATH=/usr/pkg/lib
		clear
		./bot
		;;
esac	
done
