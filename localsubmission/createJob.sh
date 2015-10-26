#!/bin/bash 

printf "#!/bin/bash\n\n" > submit.sh



# getting the right queu
if [[ -n $1 ]]
then
    var=1
    echo "argument found!"
    if [[ $1 == "test" ]]
    then
	echo "launching with q express"
	printf "#PBS -q express\n\n" >> submit.sh
	var=2
    else
	echo "argument should be 'test'. Exiting... "
	exit
    fi
else
    echo "no argument found. => q = localgrid"
    var=3
    printf "#PBS -q localgrid\n" >> submit.sh
    printf "#PBS -l walltime=02:00:00\n\n" >> submit.sh

fi

echo "var is"
echo $var

printf "# setting up your code and your env\n" >> submit.sh 
echo "cd /user/qpython/TopBrussels7X/CMSSW_7_4_14/src/TopBrussels/DisplacedTops" >> submit.sh
#echo "eval `scramv1 runtime -sh` # cmsenv" >> submit.sh
printf "cmsenv\n\n" >> submit.sh

echo "# want you really want to do!!" >> submit.sh
printf "python /user/qpython/TopBrussels7X/CMSSW_7_4_14/src/TopBrussels/DisplacedTops/LaunchParallelMacro.py\n\n" >> submit.sh


# executing the script created
qsub submit.sh




