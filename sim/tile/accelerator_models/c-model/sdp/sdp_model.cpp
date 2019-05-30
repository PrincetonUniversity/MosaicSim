#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "../accelerators.hpp"
#include "sdp_model.hpp"

void load_model(long long unsigned &cycles,
		unsigned length, unsigned working_mode)
{
    // iteration delay
    cycles++;

    // 1. Load chunks of the first matrix

    // send dma read request: length
    cycles++;

    // add DMA latency
    cycles += DRAM_QUEUE_LATENCY;

    // read dma channel		
    cycles += length;

    if (working_mode < 4) {
	// 2. Load chunks of the second matrix

	// send dma read request: length
	cycles++;

	// add DMA latency
	cycles += DRAM_QUEUE_LATENCY;

	// read dma channel
	cycles += length;
    }
}

void sdp_compute_model(long long unsigned &cycles, unsigned length)
{
    long long unsigned local_cycles = 0;

    // iteration delay
    local_cycles += 1;

    // multiplexing
    local_cycles += 1;

    // read 4 PLM entries
    local_cycles += 1;

    // perform operation in parallel on 4 PLM entries
    local_cycles += 7;

    // write 4 PLM entries
    local_cycles += 1;

    // repeat until reached length
    cycles += local_cycles * (length / 4);
}

void store_model(long long unsigned &cycles, unsigned length)
{
    // iteration delay
    cycles += 1;

    // send dma write request: length
    cycles += 1;

    // send dma data
    cycles += length * 2;
}

acc_perf_t dec_sdp_invoke(config_sys_t config_sys, config_sdp_t config, unsigned size)
{
    acc_perf_t perf;
    unsigned chunk_size, chunks, chunks_rem, subchunks, subchunks_rem, _subchunks;
    unsigned length;
    long long unsigned cycles_load = 0, cycles_compute = 0, cycles_store = 0;
    long long unsigned bytes_load = 0, bytes_store = 0;
    long long unsigned cycles_load_chunk = 0, cycles_load_chunk_rem = 0;
    long long unsigned cycles_compute_chunk = 0;
    long long unsigned cycles_store_chunk = 0;

    // pre-evaluate number of chunks and length of last chunk
    chunk_size = SDP_DMA_CHUNK;
    if (config.working_mode >= 4)
	chunk_size = (SDP_DMA_CHUNK << 1);

    chunks = size / chunk_size;
    chunks_rem = size - (chunks * chunk_size);
    if (chunks_rem)
	++chunks;

    length = chunk_size >> 1;
    subchunks = length / SDP_STORE_CHUNK;
    subchunks_rem = chunks_rem / SDP_STORE_CHUNK;

    // precompute performance of loading a chunk
    load_model(cycles_load_chunk, length, config.working_mode);
    load_model(cycles_load_chunk_rem, chunks_rem / 2, config.working_mode);

    printf("cycles_load_chunk %llu\n", cycles_load_chunk);
    printf("cycles_load_chunk_rem %llu\n", cycles_load_chunk_rem);

    // precompute performance of computing a chunk
    sdp_compute_model(cycles_compute_chunk, SDP_STORE_CHUNK);

    printf("cycles_compute_chunk %llu\n", cycles_compute_chunk);

    // precompute performance of storing a chunk
    store_model(cycles_store_chunk, SDP_STORE_CHUNK);

    printf("cycles_store_chunk %llu,\n", cycles_store_chunk);

    cycles_load = config_sys.dram_latency;

    for (unsigned i = 0; i < chunks; ++i) {

	// LOAD PHASE
	if (i == chunks - 1 && chunks_rem) {
	    cycles_load += cycles_load_chunk_rem;
	    _subchunks = subchunks_rem;
	} else {
	    cycles_load += cycles_load_chunk;
	    _subchunks = subchunks;
	}

	++cycles_load; // handshake
	++cycles_compute; // handshake

	for (unsigned j = 0; j < _subchunks; ++j) {

	    // COMPUTE PHASE
	    if (!i && !j)
		cycles_compute += cycles_load;

	    cycles_compute += cycles_compute_chunk;

	    ++cycles_compute; // handshake
	    ++cycles_store; // handshake

	    // STORE PHASE
	    if (!i && !j)
		cycles_store += cycles_compute;


	    cycles_store += cycles_store_chunk;
	}
    }

    // TRASFERRED DATA SIZE

    if (config.working_mode < 4)
	bytes_load = 2 * size * 4;
    else
	bytes_load = size * 4;

    bytes_store = size * 4;

    // PERFORMANCE ESTIMATES

    printf("cycles_load: %llu, cycles_compute: %llu, cycles_store: %llu\n",
    	   cycles_load, cycles_compute, cycles_store);
    printf("bytes_load: %llu, bytes_store: %llu\n",
    	   bytes_load, bytes_store);

    // Execution time
    // the slowest of the three processes determines the execution time
    perf.cycles = std::max(cycles_load, cycles_compute);
    perf.cycles = std::max(perf.cycles, cycles_store);

    // Bandwidth requirement
    // sum load and store bytes accessed
    perf.bytes = bytes_load + bytes_store;

    perf.power = SDP_AVG_POWER;

    printf("cycles: %llu\n", perf.cycles);
    printf("bytes: %llu\n", perf.bytes);

    return perf;
}

// simulator API to invoke Sdp accelerator
acc_perf_t sim_sdp(config_sys_t config_sys, config_sdp_t config)
{
    // max # accelerator parallelism based on system specs
    unsigned int n_acc_bandwidth_bound = config_sys.mem_bandwidth / 8; // 8 bytes = 1 word
    unsigned int n_acc_max;
    if (n_acc_bandwidth_bound < config_sys.n_acc_tiles)
	n_acc_max = n_acc_bandwidth_bound;
    else
	n_acc_max = config_sys.n_acc_tiles;

    acc_perf_t perf = dec_sdp_invoke(config_sys, config, config.size / n_acc_max);

    perf.cycles = perf.cycles;
    perf.bytes = perf.bytes * n_acc_max;
    perf.power = perf.power * n_acc_max;

    // project to required technology
    perf.power = tech_projection(perf.power, SDP_TECH, config_sys.tech);

    // add invocation latency of accelerator
    perf.cycles += ACC_INVOKE_LATENCY;

    return perf;
}
