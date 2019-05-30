//
// Test app to invoke the sdp accelerator
//

#include <stdlib.h>
#include "accelerators.hpp"
#include "sdp_model.hpp"

int main(int argc, char *argv[])
{
    config_sys_t config_sys;
    config_sdp_t config_sdp;

    // prepare accelerator configuration.
    // in the real flow this comes from Python through the compiler
    if (argc == 1) {
	printf("No command line arguments. Use defaults\n");

	config_sys.tech = 14;
	config_sys.mem_bandwidth = 96;
	config_sys.dram_latency = 300;
	config_sys.n_acc_tiles = 4;
	config_sys.n_IS_tiles = 4;
	config_sdp.working_mode = 0;
	config_sdp.size = 256000;

    } else if (argc == 8) {
	printf("Received arguments from command line.\n");

	config_sys.tech = strtol(argv[1], NULL, 10);
	config_sys.mem_bandwidth = strtol(argv[2], NULL, 10);
	config_sys.dram_latency = strtol(argv[3], NULL, 10);
	config_sys.n_acc_tiles = strtol(argv[4], NULL, 10);
	config_sys.n_IS_tiles = strtol(argv[5], NULL, 10);
	config_sdp.working_mode = strtol(argv[6], NULL, 10);
	config_sdp.size = strtol(argv[7], NULL, 10);

    } else {
	printf("Wrong argument from command line.\n");
	return 1;
    }

    printf("config_sys.tech: %d\n", config_sys.tech);
    printf("config_sys.mem_bandwidth: %d\n", config_sys.mem_bandwidth);
    printf("config_sys.dram_latency: %d\n", config_sys.dram_latency);
    printf("config_sys.n_acc_tiles: %d\n", config_sys.n_acc_tiles);
    printf("config_sys.n_IS_tiles: %d\n", config_sys.n_IS_tiles);
    printf("config_sdp.working_mode: %d\n", config_sdp.working_mode);
    printf("config_sdp.size: %d\n", config_sdp.size);

    // accelerator invocations
    acc_perf_t perf = sim_sdp(config_sys, config_sdp);

    // printout results
    printf("SDP ACCELERATOR: PERFORMANCE\n");
    printf(" - cycles: %llu\n", perf.cycles);
    printf(" - bytes: %llu\n", perf.bytes);
    printf(" - power: %f\n", perf.power);
    printf("\n");

    return 0;
}
