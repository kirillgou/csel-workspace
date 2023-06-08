#!/bin/sh
for i in $(seq 1 200)
do
	echo "Init $i"
	modprobe mod_ex_noyau_4
	echo "Rmmod"
	modprobe -r mod_ex_noyau_4
done
