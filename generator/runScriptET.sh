#!/bin/bash
mainDir=$1		#main directory where all folder parameter setting are
resultDir=$2
rm -rf $2
mkdir $2
DLVHEX="dlvhex --mlp --num=100 --verbose=128"
for dir in $mainDir/*; do 
	if [ -d $dir ]; then
		#execute 10 instances here
		shortDir=${dir#$mainDir/}
		mkdir $2/$shortDir
		echo "process $shortDir"
		for i in {1..10}
		do
			(ulimit -v 1048576 ; /usr/bin/time --verbose -o $2/$shortDir/time-$shortDir-i$i.log $DLVHEX $mainDir/$shortDir/$shortDir-i$i-*.mlp) 2>$2/$shortDir/stats-$shortDir-i$i.log 1>/dev/null
			echo "$i instances(s) processed"
		done
		echo $curdir
		
	fi
done
