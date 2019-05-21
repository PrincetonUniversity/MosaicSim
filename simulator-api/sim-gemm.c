//
// Simulator API for invoking the gemm accelerator
//

#include "sim_accelerators.h"

// simulator API to invoke GeMM accelerator
acc_perf_t sim_gemm(config_gemm_t config)
{
    int n_invocations = (int) config.batch_size / N_ACC_GEMM;

    // Call the accelerator only once, then project performance to
    // n_invocations of N_ACC_GEMM accelerator in parallel. The last
    // invocation might invoke less than N_ACC_GEMM accelerators.
    acc_perf_t perf = dec_gemm_invoke(config);

    perf.cycles = perf.cycles * n_invocations;
    perf.power = perf.power *
	((float) config.batch / (float) n_invocations);
    perf.bandwidth = perf.bandwidth *
	((float) config.batch / (float) n_invocations);
    perf.utilization = (float) config.batch / (float) n_invocations;

    return perf;
}

int main(void)
{
    // prepare accelerator configuration.
    // TODO this should come from Python through the compiler
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
