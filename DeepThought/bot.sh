#!/bin/bash
while true
do
OS="`uname`"
case $OS in
	'Linux')
		git fetch origin
		git reset --hard origin/main
 		make 
		./bot
		;;
	'NetBSD')
		git fetch origin
                git reset --hard origin/main
		make -f Makefile.bsd 
		export LD_LIBRARY_PATH=/usr/pkg/lib
		make -f Makefile.bsd clean
		make -f Makefile.bsd 
		./bot
		;;
esac	
done
