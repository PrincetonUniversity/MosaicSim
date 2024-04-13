/*
 * sdp_model.hpp
 * Author: Davide Giri
 */

#ifndef _SDP_MODEL_HPP_
#define _SDP_MODEL_HPP_

#include "../accelerators.hpp"

#define SDP_DMA_CHUNK 128
#define SDP_STORE_CHUNK 32

// tech feature size (nm)
#define SDP_TECH 32

// area in um^2, IBM 32nm
#define SDP_AREA 84485.0

// average power consumption estimate in mW
#define SDP_AVG_POWER 26.19

// configuration parameters of Sdp accelerator
typedef struct config_sdp {
    int working_mode;
    int size;
} config_sdp_t;

// model API prototype
acc_perf_t sim_sdp(config_sys_t config_sys, config_sdp_t config);

#endif // _SDP_MODEL_HPP_
