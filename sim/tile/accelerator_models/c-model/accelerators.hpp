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
#define IS_AVG_POWER 26.29

#define SCALING_FACTOR_32TO14 0.091
#define SCALING_FACTOR_16TO14 0.518
#define SCALING_FACTOR_32TO5 0.039
#define SCALING_FACTOR_16TO5 0.222

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
  
  // new projection
  if (tech == 32 && new_tech == 14)
    new_val = val * SCALING_FACTOR_32TO14;
  else if (tech == 16 && new_tech == 14)
    new_val = val * SCALING_FACTOR_16TO14;
  else if (tech == 32 && new_tech == 5)
      new_val = val * SCALING_FACTOR_32TO5;
  else if (tech == 16 && new_tech == 5)
    new_val = val * SCALING_FACTOR_16TO5;
  else
    new_val = val * pow(((float) new_tech / (float) tech), 2);
  
  return new_val;
}

#endif // _ACCELERATORS_HPP_
