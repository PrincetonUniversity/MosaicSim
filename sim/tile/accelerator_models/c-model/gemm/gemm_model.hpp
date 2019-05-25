#ifndef _GEMM_MODEL_HPP_
#define _GEMM_MODEL_HPP_

#include "../accelerators.hpp"

#define GEMM_DMA_CHUNK 64

// #define GEMM_AREA 54288 // area in um^2, IBM 32nm
#define GEMM_AREA_14NM 10391
#define GEMM_AREA_5NM 1325 

//#define GEMM_AVG_POWER 34.0 // average power consumption estimate in mW
#define GEMM_AVG_POWER_14NM 6.5
#define GEMM_AVG_POWER_5NM 0.83

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
