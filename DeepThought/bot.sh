#!/bin/bash
while true
do 	
	git checkout -- .
	make clean
	make 
	./bot
done
