/*
 * gemm_model.hpp
 * Author: Davide Giri
 */

#ifndef _GEMM_MODEL_HPP_
#define _GEMM_MODEL_HPP_

#include "../accelerators.hpp"

#define GEMM_DMA_CHUNK 64

// tech feature size: IBM 32nm
#define GEMM_TECH 32

// area in um^2, IBM 32nm
#define GEMM_AREA 63295.0

// average power consumption estimate in mW: IBM 32nm
#define GEMM_AVG_POWER 26.34

// configuration parameters of GeMM accelerator
typedef struct config_gemm {
  long rowsA;
  long colsA;
  long colsB;
  long batch_size;
  long has_IS_tile;
} config_gemm_t; 

// model API prototype
acc_perf_t sim_gemm(config_sys_t config_sys, config_gemm_t config_gemm);

#endif // _GEMM_MODEL_HPP_
