//
// Test app to invoke the sdp accelerator
//

#include <stdlib.h>
#include "accelerators.hpp"
#include "sdp_model.hpp"

int main(int argc, char *argv[])
{
    config_sdp_t config_sdp;

    // prepare accelerator configuration.
    // in the real flow this comes from Python through the compiler
    if (argc == 1) {
	printf("No command line arguments. Use defaults\n");

	config_sdp.working_mode = 0;
	config_sdp.size = 256;

    } else if (argc == 3) {
	printf("Received arguments from command line.\n");

	config_sdp.working_mode = strtol(argv[1], NULL, 10);
	config_sdp.size = strtol(argv[2], NULL, 10);

	printf("config_sdp.working_mode: %d\n", config_sdp.working_mode);
	printf("config_sdp.size: %d\n", config_sdp.size);

    } else {
	printf("Wrong argument from command line.\n");
	return 1;
    }

    // accelerator invocations
    acc_perf_t perf = sim_sdp(config_sdp);

    // printout results
    printf("SDP ACCELERATOR: PERFORMANCE\n");
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
