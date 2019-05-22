#ifndef _ACCELERATORS_HPP_
#define _ACCELERATORS_HPP_

// SoC model parameters
#define DRAM_LATENCY 300
#define IS_LATENCY 4

// SoC accelerator tiles
#define N_ACC_MAX_PER_TYPE 4
#define N_IS 4
#define N_ACC_TYPES 4
#define N_ACC_NVDLA 4
#define N_ACC_GEMM 4
#define N_ACC_SDP 4
#define N_ACC_CONV 4

// IS tile model parameters
#define IS_MEM_SIZE 262144
#define IS_MIN_CHUNK 32
#define IS_AREA
#define IS_AVG_POWER
#define IS_BURST_LENGTH 4196
#define IS_MAX_DMA_REQS 16

// contains performance estimates returned by an accelerator invocation
typedef struct acc_performance {
    long long unsigned int cycles;
    long long unsigned int bytes;
    float area; // in um^2
    float power; // in mW
    float bandwidth; // bytes/cycles
    float utilization;
} acc_perf_t; 

#endif // _ACCELERATORS_HPP_
