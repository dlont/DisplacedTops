#!/bin/bash 
if [[ -n $1 ]] #check if variable is not empty
then
    if [[ $1 == "test" ]]
    then
	cd test
	for f in ./submit*.sh
	do
	    qsub $f
	    sleep 0.2
	done
	cd -
    else
	cd output
        for f in ../submit_$1*.sh
        do
            qsub $f
	    sleep 0.2
        done
        cd -
    fi

else
    cd output
    for f in ../submit*.sh
    do
	qsub $f
	sleep 0.2
    done
    cd -

fi
