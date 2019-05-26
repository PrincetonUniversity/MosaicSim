#ifndef _SDP_MODEL_HPP_
#define _SDP_MODEL_HPP_

#include "accelerators.hpp"

#define SDP_DMA_CHUNK 128
#define SDP_STORE_CHUNK 32

// #define SDP_AREA 54288 // area in um^2, IBM 32nm
#define SDP_AREA_14NM 10391
#define SDP_AREA_5NM 1325 

//#define SDP_AVG_POWER 34.0 // average power consumption estimate in mW
#define SDP_AVG_POWER_14NM 6.5
#define SDP_AVG_POWER_5NM 0.83

// configuration parameters of Sdp accelerator
typedef struct config_sdp {
    int working_mode;
    int size;
} config_sdp_t;

// model API prototype
acc_perf_t sim_sdp(config_sdp_t config);

#endif // _SDP_MODEL_HPP_
