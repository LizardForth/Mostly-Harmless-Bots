#!/bin/bash
while true
do
OS="`uname`"
case $OS in
	'Linux')
		git checkout -- . 
 		make 
		./bot
		;;
	'NetBSD')
		git checkout -- .
		gmake -g Makefile.bsd 
		./bot
		;;
esac	
done
