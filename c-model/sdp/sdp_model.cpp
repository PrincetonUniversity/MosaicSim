#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "accelerators.hpp"
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

void compute_model(long long unsigned &cycles, unsigned length)
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

acc_perf_t dec_sdp_invoke(config_sdp_t config)
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

    chunks = config.size / chunk_size;
    chunks_rem = config.size - (chunks * chunk_size);
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
    compute_model(cycles_compute_chunk, SDP_STORE_CHUNK);

    printf("cycles_compute_chunk %llu\n", cycles_compute_chunk);

    // precompute performance of storing a chunk
    store_model(cycles_store_chunk, SDP_STORE_CHUNK);

    printf("cycles_store_chunk %llu,\n", cycles_store_chunk);

    cycles_load = DRAM_LATENCY;

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
	bytes_load = 2 * config.size * 4;
    else
	bytes_load = config.size * 4;

    bytes_store = config.size * 4;

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

    perf.area_14nm = SDP_AREA_14NM;
    perf.power_14nm = SDP_AVG_POWER_14NM;
    perf.area_5nm = SDP_AREA_5NM;
    perf.power_5nm = SDP_AVG_POWER_5NM;

    printf("cycles: %llu\n", perf.cycles);
    printf("bytes: %llu\n", perf.bytes);

    // evaluate bandwidth requirement
    perf.bandwidth = ((float) perf.bytes) / ((float) perf.cycles);

    return perf;
}

// simulator API to invoke Sdp accelerator
acc_perf_t sim_sdp(config_sdp_t config)
{
    int n_invocations = (int) config.batch_size;

    acc_perf_t perf = dec_sdp_invoke(config);

    perf.cycles = perf.cycles * n_invocations;
    perf.bytes = perf.bytes * n_invocations;

    return perf;
}
