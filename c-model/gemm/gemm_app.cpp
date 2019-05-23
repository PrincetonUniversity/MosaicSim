//
// Simulator API for invoking the gemm accelerator
//

#include "../accelerators.hpp"
#include "gemm_model.hpp"

int main(void)
{
    // prepare accelerator configuration.
    // in the real flow this comes from Python through the compiler
    config_gemm_t config_gemm;
    config_gemm.rowsA = 100;
    config_gemm.colsA = 100;
    config_gemm.colsB = 100;
    config_gemm.batch_size = 100;
    config_gemm.has_IS_tile = true;

    // accelerator invocations
    acc_perf_t gemm_perf = sim_gemm(config_gemm);

    return 0;
}
