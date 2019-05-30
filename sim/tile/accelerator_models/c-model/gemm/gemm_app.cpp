//
// Test app to invoke the gemm accelerator
//

#include <stdlib.h>
#include "accelerators.hpp"
#include "gemm_model.hpp"

int main(int argc, char *argv[])
{
    config_sys_t config_sys;
    config_gemm_t config_gemm;
    config_gemm.has_IS_tile = true;

    // prepare accelerator configuration.
    // in the real flow this comes from Python through the compiler
    if (argc == 1) {
	printf("No command line arguments. Use defaults\n");

	config_sys.tech = 14;
	config_sys.mem_bandwidth = 48;
	config_sys.dram_latency = 300;
	config_sys.n_acc_tiles = 4;
	config_sys.n_IS_tiles = 4;
	config_gemm.rowsA = 10;
	config_gemm.colsA = 10;
	config_gemm.colsB = 10;
	config_gemm.batch_size = 1;

    } else if (argc == 10) {
	printf("Received arguments from command line.\n");

	config_sys.tech = strtol(argv[1], NULL, 10);
	config_sys.mem_bandwidth = strtol(argv[2], NULL, 10);
	config_sys.dram_latency = strtol(argv[3], NULL, 10);
	config_sys.n_acc_tiles = strtol(argv[4], NULL, 10);
	config_sys.n_IS_tiles = strtol(argv[5], NULL, 10);
	config_gemm.rowsA = strtol(argv[6], NULL, 10);
	config_gemm.colsA = strtol(argv[7], NULL, 10);
	config_gemm.colsB = strtol(argv[8], NULL, 10);
	config_gemm.batch_size = strtol(argv[9], NULL, 10);

    } else {
	printf("Wrong argument from command line.\n");
	return 1;
    }

    printf("config_sys.tech: %d\n", config_sys.tech);
    printf("config_sys.mem_bandwidth: %d\n", config_sys.mem_bandwidth);
    printf("config_sys.dram_latency: %d\n", config_sys.dram_latency);
    printf("config_sys.n_acc_tiles: %d\n", config_sys.n_acc_tiles);
    printf("config_sys.n_IS_tiles: %d\n", config_sys.n_IS_tiles);
    printf("config_gemm.rowsA: %d\n", config_gemm.rowsA);
    printf("config_gemm.colsA: %d\n", config_gemm.colsA);
    printf("config_gemm.colsB: %d\n", config_gemm.colsB);
    printf("config_gemm.batch_size: %d\n", config_gemm.batch_size);

    // accelerator invocations
    acc_perf_t perf = sim_gemm(config_sys, config_gemm);

    // printout results
    printf("GEMM ACCELERATOR: PERFORMANCE\n");
    printf(" - cycles: %llu\n", perf.cycles);
    printf(" - bytes: %llu\n", perf.bytes);
    printf(" - power: %f\n", perf.power);
    printf("\n");

    return 0;
}
