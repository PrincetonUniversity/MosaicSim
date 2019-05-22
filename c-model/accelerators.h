#ifndef _SIM_ACCELERATORS_H_
#define _SIM_ACCELERATORS_H_

// amounts of accelerator and IS tiles
#define N_IS 4
#define N_ACC_TYPES 4
#define N_ACC_NVDLA 4
#define N_ACC_GEMM 4
#define N_ACC_SDP 4
#define N_ACC_CONV 4
#define N_ACC_MAX_PER_TYPE 4

#define IS_MEM_SIZE 262144
#define IS_MIN_CHUNK 32
#define IS_AREA
#define IS_AVG_POWER
#define IS_BURST_LENGTH 4196
#define IS_MAX_DMA_REQS 16

// area in # of cells, IBM 32nm
#define GEMM_AREA 54288
// average power consumption estimate in mW
#define GEMM_AVG_POWER 34

#define DRAM_LATENCY 300
#define IS_LATENCY 4

// TODO probably should be protected by a lock
// counters for each accelerator type to keep track of how many are in use
int acc_status[N_ACC_TYPES + 1]; // +1 is the IS tile

// contains performance estimates returned by an accelerator invocation
typedef struct acc_performance {
    long long unsigned int cycles;
    long long unsigned int bytes;
    float area; // in um^2
    float power; // in mW
    float bandwidth; // bytes/cycles
    float utilization;
} acc_perf_t; 

// configuration parameters of GeMM accelerator
typedef struct config_gemm {
    int rowsA;
    int colsA;
    int colsB;
    int batch_size;
    int has_IS_tile;
} config_gemm_t; 


#endif // _SIM_ACCELERATORS_H_
