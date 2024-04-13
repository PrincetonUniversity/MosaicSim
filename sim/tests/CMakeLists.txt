set(MOSAIC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mosaic")
set(SIM "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/sim_driver")
configure_file(check.sh check.sh)

file(COPY graph_projections_fast_llamas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY bfs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY bfs_fast_llamas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY spmm DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY sssp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY sssp_fast_llamas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY pagerank_fast_llamas DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY common DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Kron16 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY acc_dense_layer DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY acc_matmul DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY acc_sdp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

## MOSAIC TESTS
add_test(NAME mosaic_base_graph_projections_fast_llamas COMMAND bash -c
	      "cd graph_projections_fast_llamas && ${MOSAIC} main.cpp --exe_args data.txt -n 2 > mosaic_base 2>&1")
add_test(NAME mosaic_decoupled_graph_projections_fast_llamas COMMAND bash -c
	      "cd  graph_projections_fast_llamas && ${MOSAIC} main.cpp --DeSC --exe_args data.txt -n 2  > base 2>&1")

add_test(NAME mosaic_base_bfs COMMAND bash -c
	      "cd bfs && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2   > mosaic_base.out 2>&1")
add_test(NAME mosaic_decoupled_bfs COMMAND bash -c
	      "cd bfs && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2 --decoupled  > mosaic_decoupled.out 2>&1")

add_test(NAME mosaic_base_bfs_fast_llamas COMMAND bash -c
	      "cd bfs_fast_llamas && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2  > mosaic_base.out 2>&1")
add_test(NAME mosaic_decoupled_bfs_fast_llamas COMMAND bash -c
	      "cd bfs_fast_llamas && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2 --DeSC  > mosaic_decoupled.out 2>&1")

add_test(NAME mosaic_base_spmm COMMAND bash -c
	      "cd spmm && ${MOSAIC} main.cpp --exe_args \"data/Amat.txt  data/Bmat.txt\" -n 2   > mosaic_base.out 2>&1")
add_test(NAME mosaic_decoupled_spmm COMMAND bash -c
	      "cd spmm && ${MOSAIC} main.cpp --exe_args \"data/Amat.txt  data/Bmat.txt\" -n 2 --decoupled   > mosaic_decoupled.out 2>&1")

add_test(NAME mosaic_base_sssp COMMAND bash -c
	      "cd sssp && ${MOSAIC} main.cpp --exe_args Kron5/ -n 2  > mosaic_base.out 2>&1")
# DECOUPLED MISSING! add_test(NAME mosaic_base_sssp COMMAND bash -c 
# 	      "cd sssp && ${MOSAIC} main.cpp --exe_args Kron5/ -n 2  > mosaic_base.out 2>&1")

add_test(NAME mosaic_base_sssp_fast_llamas COMMAND bash -c
	      "cd sssp_fast_llamas && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2  > mosaic_base.out 2>&1")
# DECOUPLED MISSING! add_test(NAME mosaic_decoupled_sssp_fast_llamas COMMAND bash -c
# 	      "cd sssp_fast_llamas && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2  -d > mosaic_base.out 2>&1")

add_test(NAME mosaic_base_pagerank_fast_llamas COMMAND bash -c
	      "cd pagerank_fast_llamas && ${MOSAIC} main.cpp --exe_args \"../Kron16/ 4 30175\" -n 2  > mosaic_base.out 2>&1")
# DECOUPLED MISSING!  add_test(NAME mosaic_decoupled_pagerank_fast_llamas COMMAND bash -c
# 	      "cd pagerank_fast_llamas && ${MOSAIC} main.cpp --DeSC --exe_args \"../Kron16/ 4 30175\" -n 2  > mosaic_base.out 2>&1")

## SIMULATOR TESTS
add_test(NAME simulate_base_graph_projections_fast_llamas COMMAND bash -c
	      "cd graph_projections_fast_llamas && ${SIM} -n 2 input  > sim_base 2>&1 && ../check.sh")
add_test(NAME simulate_decoupled_graph_projections_fast_llamas COMMAND bash -c
	      "cd graph_projections_fast_llamas && ${SIM} -n 2 input --DeSC  > sim_decoupled 2>&1 && ../check.sh -d")
add_test(NAME simulate_base_bfs COMMAND bash -c
	      "cd bfs && ${SIM} -n 2 input > sim_base 2>&1 && ../check.sh -i")
add_test(NAME simulate_decoupled_bfs COMMAND bash -c
	      "cd bfs && ${SIM} -n 2 input --decoupled  > sim_decoupled 2>&1 && ../check.sh -d")
add_test(NAME simulate_base_bfs_fast_llamas COMMAND bash -c
	      "cd bfs && ${SIM} -n 2 input  > sim_base 2>&1 && ../check.sh -i")
add_test(NAME simulate_decoupled_bfs_fast_llamas COMMAND bash -c
	      "cd bfs && ${SIM} -n 2 input --DeSC  > sim_decoupled 2>&1 && ../check.sh -d")
add_test(NAME simulate_base_spmm COMMAND bash -c
	      "cd spmm && ${SIM} -n 2 input  > sim_base 2>&1 && ../check.sh")
add_test(NAME simulate_decoupled_spmm COMMAND bash -c
	      "cd spmm && ${SIM} -n 2 input --decoupled  > sim_decoupled 2>&1 && ../check.sh -d")
add_test(NAME simulate_base_sssp COMMAND bash -c
	      "cd sssp && ${SIM} -n 2 input  > sim_base 2>&1 && ../check.sh -i")
# add_test(NAME simulate_decoupled_sssp COMMAND bash -c
# 	      "cd sssp && ${SIM} -n 2 input --decoupled  > sim_decoupled 2>&1 && ../check.sh -d")
add_test(NAME simulate_base_sssp_fast_llamas COMMAND bash -c
	      "cd sssp_fast_llamas && ${SIM} -n 2 input  > sim_base 2>&1 && ../check.sh")
# add_test(NAME simulate_decoupled_sssp_fast_llamas COMMAND bash -c
# 	      "cd sssp_fast_llamas && ${SIM} -n 2 input --DeSC > sim_decoupled  2>&1 && ../check.sh -d")
add_test(NAME simulate_base_pagerank_fast_llamas COMMAND bash -c
	      "cd pagerank_fast_llamas && ${SIM} -n 1 input  > sim_base 2>&1 && ../check.sh -n 1 -i")
# add_test(NAME simulate_decoupled_pagerank_fast_llamas COMMAND bash -c
# 	      "cd pagerank_fast_llamas && ${SIM} -n 2 input --DeSC  > sim_decoupled 2>&1 && ../check.sh -d")

## Accelerators
add_test(NAME accelerators_dense_layer COMMAND bash -c
	      "cd acc_dense_layer && ${SIM} -n 2 > sim_base 2>&1 && ../check.sh -i -A")
set_tests_properties(accelerators_dense_layer PROPERTIES TIMEOUT 5) 
add_test(NAME accelerators_matmul COMMAND bash -c
	      "cd acc_matmul && ${SIM} -n 2 > sim_base 2>&1 && ../check.sh -i -A")
set_tests_properties(accelerators_matmul PROPERTIES TIMEOUT 5) 
add_test(NAME accelerators_sdp COMMAND bash -c
	      "cd acc_sdp && ${SIM} -n 2 > sim_base 2>&1 && ../check.sh -i -A")
set_tests_properties(accelerators_sdp PROPERTIES TIMEOUT 5) 
