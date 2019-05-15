//
// Simulator API for invoking the gemm accelerator
//

#define N_ACC_GEMM 4

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
    int datatype; // 0 = int32, 1 = float32
    int rowsA;
    int colsA;
    int colsB;
    int batch_size;
    int has_IS_tile;
} config_gemm_t; 


// simulator API to invoke GeMM accelerator
acc_perf_t sim_gemm(config_gemm_t config)
{
    // TODO: make sure the bandwidth required by the N_ACC_GEMM accelerators
    // doesn't exceed the available bandwidth

    int n_invocations = (int) config.batch_size / N_ACC_GEMM;

    // Call the accelerator only once, then project performance to
    // n_invocations of N_ACC_GEMM accelerator in parallel. The last
    // invocation might invoke less than N_ACC_GEMM accelerators.
    acc_perf_t perf = dec_gemm_invoke(config);

    if (!config.has_IS_tile) {

	perf.cycles = perf.cycles * n_invocations;
	perf.power = perf.power *
	    ((float) config.batch / (float) n_invocations);
	perf.bandwidth = perf.bandwidth *
	    ((float) config.batch / (float) n_invocations);
	perf.utilization = (float) config.batch / (float) n_invocations;
    } else {

	// TODO this part is still TBD
	// acc_perf_t perf_IS = dec_IS_gemm_invoke(config);
    }

    return perf;
}

int main(void)
{
    // prepare accelerator configuration.
    // TODO this should come from Python through the compiler
    config_gemm_t config_gemm;
    config_gemm.datatype = 1;
    config_gemm.rowsA = 100;
    config_gemm.colsA = 100;
    config_gemm.colsB = 100;
    config_gemm.batch_size = 100;
    config_gemm.has_IS_tile = false;

    // accelerator invocations
    acc_perf_t gemm_perf = sim_gemm(config_gemm);

    return 0;
}
