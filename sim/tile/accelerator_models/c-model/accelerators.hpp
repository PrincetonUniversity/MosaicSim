/*
 * accelerators.hpp
 * Author: Davide Giri
 */

#ifndef _ACCELERATORS_HPP_
#define _ACCELERATORS_HPP_

#include <stdio.h>
#include <math.h>

// SoC model parameters
#define DRAM_QUEUE_LATENCY 20
#define IS_LATENCY 4

// IS tile model parameters
#define IS_MEM_SIZE 262144
#define IS_MIN_CHUNK 32
#define IS_BURST_LENGTH 4196
#define IS_MAX_DMA_REQS 15
#define IS_AREA 1917472
#define IS_AVG_POWER 52

#define ACC_INVOKE_LATENCY 10000

// contains performance estimates returned by an accelerator invocation
typedef struct acc_performance {
    long long unsigned int cycles;
    long long unsigned int bytes;
    float power; // mW
} acc_perf_t; 

typedef struct config_sys {
    unsigned int tech; // nm
    unsigned int mem_bandwidth; // bytes per cycle
    unsigned int dram_latency;
    unsigned int n_acc_tiles; 
    unsigned int n_IS_tiles;
} config_sys_t;

inline float tech_projection(float val, unsigned int tech, unsigned int new_tech)
{
    float new_val;

    new_val = val * pow(((float) new_tech / (float) tech), 2);

    return new_val;
}

#endif // _ACCELERATORS_HPP_
