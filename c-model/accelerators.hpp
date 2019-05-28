#ifndef _ACCELERATORS_HPP_
#define _ACCELERATORS_HPP_

#include <stdio.h>

// SoC model parameters
#define DRAM_LATENCY 300
#define DRAM_QUEUE_LATENCY 20
#define IS_LATENCY 4

// SoC accelerator tiles
#define N_ACC_MAX_PER_TYPE 4
#define N_IS 4
#define N_ACC_TYPES 3
#define N_ACC_NVDLA 4
#define N_ACC_GEMM 4
#define N_ACC_SDP 4

// IS tile model parameters
#define IS_MEM_SIZE 524288
#define IS_MIN_CHUNK 32
#define IS_BURST_LENGTH 4196
#define IS_MAX_DMA_REQS 15
// #define IS_AREA_32NM 200000
#define IS_AREA_14NM 38281
#define IS_AREA_5NM 4883
// #define IS_AVG_POWER 400
#define IS_AVG_POWER_14NM 76.56
#define IS_AVG_POWER_5NM 9.77

// contains performance estimates returned by an accelerator invocation
typedef struct acc_performance {
    long long unsigned int cycles;
    long long unsigned int bytes;
    float area_14nm; // in um^2
    float area_5nm; // in um^2
    float power_14nm; // in mW
    float power_5nm; // in mW
    float bandwidth; // bytes/cycles
} acc_perf_t; 

#endif // _ACCELERATORS_HPP_
