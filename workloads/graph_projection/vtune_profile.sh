#!/bin/bash

#  Navigate  to  directory  containing  source  code  and  perform  any
#  necessary  compilation
#  Perform  any  necessary  setup  for  running  the  application  (i.e.
#  setting  up  data
#  Navigate  to  directory  where  you  run  the  script  that  runs  the
#  application  (and  corresponding  arguments)

RESULT_DIR=/home/luwa/DECADES/workflows/v0_graphx/cpp_version/vtune_results
#ANALYSIS_TYPES=( hotspots general-exploration )
ANALYSIS_TYPES=( memory-access )
#INPUTS=( _generated_data/graph500-scale18-ef16_adj_sorted.tsv )
#INPUTS=( _generated_data/rmat200k _generated_data/rmat500k _generated_data/rmat1M )

INPUTS=( _generated_data/rmat200k _generated_data/rmat500k _generated_data/rmat1M )

for AT in ${ANALYSIS_TYPES[@]}; do
    echo $AT
    for I in ${INPUTS[@]}; do
	echo $I
	input_name=$(basename ${I})
	input_no_ext="${input_name%.*}"
	#echo $input_no_ext
	RESULTS=${RESULT_DIR}/${AT}_$input_no_ext
	rm -rf $RESULTS
	
	#echo $extra_args
	echo "Storing results to: $RESULTS"
	echo "running: "
	echo amplxe-cl  –r  ${RESULTS}  -collect  ${AT} ./graph_projection_outlined --edgelistfile ${I} 
	amplxe-cl  –r  ${RESULTS}  -collect  ${AT} ./graph_projection_outlined --edgelistfile ${I} 
    done
done
#
