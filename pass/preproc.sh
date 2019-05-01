#!/bin/bash

# First arg: llvm file to be modified (and changed in place)
# Second arg: tile name (e.g. compute, supply, biscuit)
# third arg: a flag saying if we should link or not
# fourth - end : ids for the tiles

CC=clang++
CURR_PASS=$1; shift 
TILE_NAME=$1; shift
LINK_FLAG=$1; shift
TILE_IDS=( "$@" );

PYTHIA_HOME=$(dirname $(realpath -s ${0}../))/..
DIR_NAME=$(dirname ${CURR_PASS})
LLVM_OUT=$(basename ${CURR_PASS})


echo ${PYTHIA_HOME}" is printing pythia home"

echo ${DIR_NAME}

for i in "${TILE_IDS[@]}"
do
    echo "Executing: mkdir -p ${DIR_NAME}/output_${TILE_NAME}_${i}"
    mkdir -p ${DIR_NAME}/output_${TILE_NAME}_${i}
    touch ${DIR_NAME}/output_${TILE_NAME}_${i}/ctrl.txt
    touch ${DIR_NAME}/output_${TILE_NAME}_${i}/mem.txt
done
    
echo "Executing: mkdir -p ${DIR_NAME}/output_${TILE_NAME}" &&
mkdir -p ${DIR_NAME}/output

echo "Executing: cd ${DIR_NAME}; opt -S -instnamer -load ${PYTHIA_HOME}/lib/libGraphGen.so -graphgen ${LLVM_OUT} > /dev/null"
cd ${DIR_NAME}; opt -S -instnamer -load ${PYTHIA_HOME}/lib/libGraphGen.so -graphgen ${LLVM_OUT} > /dev/null

cd -;

echo "Executing: opt -S -instnamer -load ${PYTHIA_HOME}/lib/libRecordDynamicInfo.so -recorddynamicinfo ${CURR_PASS} -o  ${CURR_PASS}"
opt -S -instnamer -load ${PYTHIA_HOME}/lib/libRecordDynamicInfo.so -recorddynamicinfo ${CURR_PASS} -o  ${CURR_PASS}




for i in "${TILE_IDS[@]}"
do
    cp ${DIR_NAME}/output/graphOutput.txt ${DIR_NAME}/output_${TILE_NAME}_${i}/
done

rm -r ${DIR_NAME}/output


if [ "$LINK_FLAG" -eq 1 ];
then
    echo "Executing: llvm-link -S ${PYTHIA_HOME}/tools/tracer.llvm ${CURR_PASS} -o ${CURR_PASS}"
    llvm-link -S ${PYTHIA_HOME}/tools/tracer.llvm ${CURR_PASS} -o ${CURR_PASS}
fi
