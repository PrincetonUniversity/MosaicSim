#!/bin/bash
CC=clang++
PYTHIA_HOME=/home/jlaragon/pythia
CURR_PASS=$1
DIR_NAME=$(dirname ${CURR_PASS})
LLVM_OUT=$(basename ${CURR_PASS})

echo ${DIR_NAME} &&
    
echo "Executing: mkdir -p ${DIR_NAME}/output" &&
mkdir -p ${DIR_NAME}/output &&
touch ${DIR_NAME}/output/ctrl.txt &&
touch ${DIR_NAME}/output/mem.txt &&
echo "Executing: llvm-link -S ${PYTHIA_HOME}/tools/tracer.llvm ${CURR_PASS} -o ${DIR_NAME}/integrated.llvm" &&
llvm-link -S ${PYTHIA_HOME}/tools/tracer.llvm ${CURR_PASS} -o ${DIR_NAME}/integrated.llvm &&

echo "Executing: opt -S -instnamer -load ${PYTHIA_HOME}/lib/libRecordDynamicInfo.so -recorddynamicinfo ${DIR_NAME}/integrated.llvm -o  ${CURR_PASS}" &&
opt -S -instnamer -load ${PYTHIA_HOME}/lib/libRecordDynamicInfo.so -recorddynamicinfo ${DIR_NAME}/integrated.llvm -o  ${CURR_PASS} &&

echo "Executing: cd ${DIR_NAME}; opt -S -instnamer -load ${PYTHIA_HOME}/lib/libGraphGen.so -graphgen integrated.llvm > /dev/null" && 
cd ${DIR_NAME}; opt -S -instnamer -load ${PYTHIA_HOME}/lib/libGraphGen.so -graphgen integrated.llvm > /dev/null
