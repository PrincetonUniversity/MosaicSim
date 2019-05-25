//
// Test app to invoke the gemm accelerator
//

#include <stdlib.h>
#include "accelerators.hpp"
#include "gemm_model.hpp"

int main(int argc, char *argv[])
{
    config_gemm_t config_gemm;
    config_gemm.has_IS_tile = true;

    // prepare accelerator configuration.
    // in the real flow this comes from Python through the compiler
    if (argc == 1) {
	printf("No command line arguments. Use defaults\n");

	config_gemm.rowsA = 10;
	config_gemm.colsA = 10;
	config_gemm.colsB = 10;
	config_gemm.batch_size = 1;

    } else if (argc == 5) {
	printf("Received arguments from command line.\n");

	config_gemm.rowsA = strtol(argv[1], NULL, 10);
	config_gemm.colsA = strtol(argv[2], NULL, 10);
	config_gemm.colsB = strtol(argv[3], NULL, 10);
	config_gemm.batch_size = strtol(argv[4], NULL, 10);

	printf("config_gemm.rowsA: %d\n", config_gemm.rowsA);
	printf("config_gemm.colsA: %d\n", config_gemm.colsA);
	printf("config_gemm.colsB: %d\n", config_gemm.colsB);
	printf("config_gemm.batch_size: %d\n", config_gemm.batch_size);

    } else {
	printf("Wrong argument from command line.\n");
	return 1;
    }

    // accelerator invocations
    acc_perf_t perf = sim_gemm(config_gemm);

    // printout results
    printf("GEMM ACCELERATOR: PERFORMANCE\n");
    printf(" - cycles: %llu\n", perf.cycles);
    printf(" - bytes: %llu\n", perf.bytes);
    printf(" - area 14nm: %f\n", perf.area_14nm);
    printf(" - area 5nm: %f\n", perf.area_5nm);
    printf(" - power 14nm: %f\n", perf.power_14nm);
    printf(" - power 5nm: %f\n", perf.power_5nm);
    printf(" - bandwidth: %f\n", perf.bandwidth);
    printf("\n");

    return 0;
}
