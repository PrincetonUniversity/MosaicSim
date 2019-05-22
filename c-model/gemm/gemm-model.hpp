#ifndef _GEMM_MODEL_HPP_
#define _GEMM_MODEL_HPP_

#include "../accelerator.hpp"

// area in # of cells, IBM 32nm
#define GEMM_AREA 54288
// average power consumption estimate in mW
#define GEMM_AVG_POWER 34

// configuration parameters of GeMM accelerator
typedef struct config_gemm {
    int rowsA;
    int colsA;
    int colsB;
    int batch_size;
    int has_IS_tile;
} config_gemm_t; 

// model API prototype
acc_perf_t sim_gemm(config_gemm_t config);

#endif // _GEMM_MODEL_HPP_
