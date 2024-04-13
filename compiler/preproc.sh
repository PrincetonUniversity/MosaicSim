#!/bin/bash

#Bash script to pass to the DEC++ compiler in order for MosaicSim to run LLVM passes
#generating static data dependency graphs and dynamic trace instrumentation

# First arg: llvm file to be modified (and changed in place)
# Second arg: tile name (e.g. compute, supply, biscuit)
# third arg: a flag saying if we should link or not
# fourth - end : ids for the tiles

CC=clang++-9
CURR_PASS=$1; shift 
TILE_NAME=$1; shift
LINK_FLAG=$1; shift
TILE_IDS=( "$@" );

DIR_NAME=$(dirname ${CURR_PASS})
LLVM_OUT=$(basename ${CURR_PASS})


echo "MosaicSim Home: "${MOSAIC_HOME}

echo ${DIR_NAME}

for i in "${TILE_IDS[@]}"
do
    echo "Executing: mkdir -p ${DIR_NAME}/output_${TILE_NAME}_${i}"
    mkdir -p ${DIR_NAME}/output_${TILE_NAME}_${i}
done
    
echo "Executing: mkdir -p ${DIR_NAME}/output_${TILE_NAME}" &&
mkdir -p ${DIR_NAME}/output

echo "Executing: cd ${DIR_NAME}; @LLVM_TOOLS_BINARY_DIR@/opt -S -instnamer -load @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/libGraphGen.so -graphgen ${LLVM_OUT} > /dev/null"
cd ${DIR_NAME};  @LLVM_TOOLS_BINARY_DIR@/opt -S -instnamer -load @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/libGraphGen.so -graphgen ${LLVM_OUT} > /dev/null

cd -;

echo "Executing:  @LLVM_TOOLS_BINARY_DIR@/opt -S -instnamer -load @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/libRecordDynamicInfo.so -recorddynamicinfo ${CURR_PASS} -o  ${CURR_PASS}"
@LLVM_TOOLS_BINARY_DIR@/opt -S -instnamer -load @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/libRecordDynamicInfo.so -recorddynamicinfo ${CURR_PASS} -o  ${CURR_PASS}




for i in "${TILE_IDS[@]}"
do
    cp ${DIR_NAME}/output/graphOutput.txt ${DIR_NAME}/output_${TILE_NAME}_${i}/
done

rm -r ${DIR_NAME}/output


if [ "$LINK_FLAG" -eq 1 ];
then
    echo "Executing:  @LLVM_TOOLS_BINARY_DIR@/llvm-link -S @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/tracer.llvm ${CURR_PASS} -o ${CURR_PASS}"
    @LLVM_TOOLS_BINARY_DIR@/llvm-link -S @CMAKE_LIBRARY_OUTPUT_DIRECTORY@/@TRACER@  ${CURR_PASS} -o ${CURR_PASS}
fi
