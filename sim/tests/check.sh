#!/bin/bash
mode=base
nb_cores=2
diff_args=
accelerators=
#shift through the args, until they are empty
while [[ $# > 0 ]]
do
    case "$1" in
	-d|--decoupled) mode=decoupled; shift;;
	-n|--nb_cores) nb_cores=$2; shift 2;;
	-i|--ignore_cycles) diff_args=--ignore-matching-lines=cycles; shift;;
	-A|--accelerators) accelerators=1; shift;;
    esac
done


[[ "$mode" == "decoupled" ]] && nb_cores=$(($nb_cores*2))

#reinit the diff file, if it already exists 
diff $diff_args Global_sim outputs/$mode/Global_sim > diff_$mode
for i in $(seq 0 $(($nb_cores-1)))
do
    #append all the diffs in the diff file
    diff $diff_args core_$i outputs/$mode/core_$i >> diff_$mode
done
[[ $accelerators -eq 1 ]] &&  diff $diff_args Acc_sim outputs/$mode/Acc_sim >> diff_$mode

# if the diff file is not empyt
if [ -s diff_$mode ]
then
    # print the diff and exit with code 1 (fail)
    cat diff_$mode
    exit 1
else
    # success
    exit 0
fi
