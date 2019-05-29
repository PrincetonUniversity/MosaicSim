#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <math.h>

#include "../accelerators.hpp"
#include "gemm_model.hpp"

void calculate_chunks(unsigned &matrix_chk, unsigned &matrix_rem, unsigned colsA)
{
     unsigned int matrix_mul;

     // calculating the number of chunks (ceil)
     matrix_chk = colsA / GEMM_DMA_CHUNK;
     // printf("matrix_chk: %u\n", matrix_chk);

     // calculating the number of cols (covered the by the chunks)
     matrix_mul = matrix_chk * GEMM_DMA_CHUNK;
     // printf("matrix_mul: %u\n", matrix_mul);

     // calculating the remaining cols (size of the last chunk)
     matrix_rem = colsA - matrix_mul;
     // printf("matrix_rem: %u\n", matrix_rem);

     // adding the last chunk if it is necessary
     if (matrix_rem != 0) { ++matrix_chk; }
}

void load_model(long long unsigned &cycles, long long unsigned &bytes,
		unsigned length, int has_IS_tile, unsigned dram_latency)
{
    // iteration delay
    cycles++;

    // 2. Load chunks of the first matrix

    // send dma read request: length
    cycles++;

    // add DMA latency
    if(has_IS_tile)
	cycles += IS_LATENCY;
    else
	cycles += dram_latency;

    // read dma channel		
    cycles += length;
    bytes += length * 8;

    // 2. Load chunks of the second matrix

    // send dma read request: length
    cycles++;

    // add DMA latency
    if(has_IS_tile)
	cycles += IS_LATENCY;
    else
	cycles += dram_latency;

    // read dma channel
    cycles += length;
    bytes += length * 8;

    // handshake with compute
    cycles++;
}

void compute_model(long long unsigned &cycles, unsigned length)
{
    // iteration delay
    cycles++;

    // handshake with load
    cycles++;

    // main computation length times
    // read PLM + compute + increment
    cycles += length * 2;
}

void store_model(long long unsigned &cycles, long long unsigned &bytes,
		 unsigned length, unsigned dram_latency)
{
    // accumulate in compute
    cycles += 3;

    // iteration delay
    cycles++;

    // handshake with compute
    cycles++;

    // send dma write request: length
    cycles++;

    // add D latency
    cycles += dram_latency;

    // write dma channel		
    cycles += length;
    bytes += length * 8;

    // handshake with compute
    cycles++;
}

long long unsigned IS_model(unsigned length, int is_strided, unsigned dram_latency)
{
    unsigned bursts, last_burst;
    long long unsigned cycles;

    bursts = length / IS_BURST_LENGTH;
    last_burst = length % IS_BURST_LENGTH;

    cycles = dram_latency;

    // cycles for the DMA read bursts
    if (!is_strided) {
	// (+2 is for the ping-pong overhead)
	if (last_burst)
	    cycles += bursts * (dram_latency / IS_MAX_DMA_REQS + IS_BURST_LENGTH + 1) +
		(dram_latency / IS_MAX_DMA_REQS + last_burst + 1);
	else
	    cycles += bursts * (dram_latency / IS_MAX_DMA_REQS + IS_BURST_LENGTH + 1);

    } else {
	// DMA transactions of a single WORD
	// assuming up to IS_MAX_DMA_REQS outstanding DMA requests
	cycles += (dram_latency / IS_MAX_DMA_REQS) * length;
    }

    return cycles;
}

acc_perf_t dec_gemm_invoke(config_sys_t config_sys, config_gemm_t config)
{
    acc_perf_t perf, perf_IS;
    long long unsigned cycles_load = 0, cycles_compute = 0;
    long long unsigned bytes_load = 0, bytes_store = 0;
    unsigned matrix_chk, matrix_chk_store;
    unsigned matrix_rem, matrix_rem_store;
    long long unsigned cycles_load_chunk = 0, cycles_load_chunk_rem = 0;
    long long unsigned cycles_compute_chunk = 0, cycles_compute_chunk_rem = 0;
    long long unsigned cycles_store_chunk = 0, cycles_store_chunk_rem = 0;
    long long unsigned bytes_load_chunk = 0, bytes_load_chunk_rem = 0;
    long long unsigned bytes_store_chunk = 0, bytes_store_chunk_rem = 0;
    unsigned count_store = 0, chk_store = 0;

    // pre-evaluate number of chunks and length of last chunk
    calculate_chunks(matrix_chk, matrix_rem, config.colsA);
    calculate_chunks(matrix_chk_store, matrix_rem_store,
		     config.rowsA * config.colsB);

    // precompute performance of loading a chunk
    load_model(cycles_load_chunk, bytes_load_chunk,
	       GEMM_DMA_CHUNK / 2, config.has_IS_tile, config_sys.dram_latency);
    load_model(cycles_load_chunk_rem, bytes_load_chunk_rem,
	       matrix_rem / 2, config.has_IS_tile, config_sys.dram_latency);
    // printf("cycles_load_chunk %llu, bytes_load_chunk %llu\n",
    // 	   cycles_load_chunk, bytes_load_chunk);
    // printf("cycles_load_chunk_rem %llu, bytes_load_chunk_rem %llu\n",
    // 	   cycles_load_chunk_rem, bytes_load_chunk_rem);

    // precompute performance of computing a chunk
    compute_model(cycles_compute_chunk, GEMM_DMA_CHUNK / 2);
    compute_model(cycles_compute_chunk_rem, matrix_rem / 2);
    // printf("cycles_compute_chunk %llu\n", cycles_compute_chunk);
    // printf("cycles_compute_chunk_rem %llu\n", cycles_compute_chunk_rem);

    // precompute performance of storing a chunk
    store_model(cycles_store_chunk, bytes_store_chunk,
		GEMM_DMA_CHUNK / 2, config_sys.dram_latency);
    store_model(cycles_store_chunk_rem, bytes_store_chunk_rem,
		matrix_rem_store / 2, config_sys.dram_latency);
    // printf("cycles_store_chunk %llu, bytes_store_chunk %llu\n",
    // 	   cycles_store_chunk, bytes_store_chunk);
    // printf("cycles_store_chunk_rem %llu, bytes_store_chunk_rem %llu\n",
    // 	   cycles_store_chunk_rem, bytes_store_chunk_rem);

    for (int rA = 0; rA < config.rowsA; ++rA) {
	for (int cB = 0; cB < config.colsB; ++cB) {
	    for (unsigned chk = 0; chk < matrix_chk; ++chk) {

		// LOAD PHASE
		if (chk == matrix_chk - 1 && matrix_rem != 0) {
		    // this is the last (smaller) chunk
		    cycles_load += cycles_load_chunk_rem;
		    bytes_load += bytes_load_chunk_rem;
		} else {
		    cycles_load += cycles_load_chunk;
		    bytes_load += bytes_load_chunk;
		}

		// COMPUTE PHASE
		if (!rA && !cB && !chk)
		    cycles_compute += cycles_load;

		if (chk == matrix_chk - 1 && matrix_rem != 0) {
		    // this is the last (smaller) chunk
		    cycles_compute += cycles_compute_chunk_rem;
		} else {
		    cycles_compute += cycles_compute_chunk;
		}
	    }

	    count_store++;

	    if (count_store == GEMM_DMA_CHUNK ||
		(rA == config.rowsA - 1 && cB == config.colsB - 1)) {
		count_store = 0;
		
		// STORE PHASE (embedded in compute)
		if (chk_store == matrix_chk_store - 1 && matrix_rem_store != 0) {
		    // this is the last (smaller) chunk
		    cycles_compute += cycles_store_chunk_rem;
		    bytes_store += bytes_store_chunk_rem;
		} else {
		    cycles_compute += cycles_store_chunk;
		    bytes_store += bytes_store_chunk;
		}

		chk_store++;
	    }
	}
    }

    // printf("cycles_load: %llu, cycles_compute: %llu\n",
    // 	   cycles_load, cycles_compute);
    // printf("bytes_load: %llu, bytes_store: %llu\n",
    // 	   bytes_load, bytes_store);

    // Execution time
    // the slowest of the three processes determines the execution time
    perf.cycles = std::max(cycles_load, cycles_compute);

    // Bandwidth requirement
    // sum load and store bytes accessed
    perf.bytes = bytes_load + bytes_store;

    perf.power = GEMM_AVG_POWER;

    // printf("cycles: %llu\n", perf.cycles);
    // printf("bytes: %llu\n", perf.bytes);

    if (config.has_IS_tile)
    {
	// TODO we are always pairing 1 IS tile with one GeMM accelerator.
	// Consider doing 2 IS tiles with one GeMM accelerator, but
	// we must keep the bandwidth below a certain threshold.

	long long unsigned int sizeA = config.rowsA * config.colsA;
	long long unsigned int sizeB = config.colsA * config.colsB;

	int is_readA_once = false;
	int is_readB_once = false;

	if (config.colsA + sizeB < IS_MEM_SIZE ||
	    sizeA + config.colsA < IS_MEM_SIZE) {

	    is_readA_once = true;
	    is_readB_once = true;

	} else if (config.colsA + IS_MIN_CHUNK < IS_MEM_SIZE) {
	    is_readB_once = true;

	} else if (config.colsA + IS_MIN_CHUNK < IS_MEM_SIZE) {
	    is_readA_once = true;
	}

	if (is_readA_once) {
	    perf_IS.cycles = IS_model(sizeA / 2, false,
				      config_sys.dram_latency);
	    // printf("perf_IS.cycles: %llu\n",  perf_IS.cycles);
	    perf_IS.bytes = sizeA * 4;
	} else {
	    perf_IS.cycles = IS_model(sizeA * config.colsB / 2, false,
				      config_sys.dram_latency);
	    perf_IS.bytes = sizeA * config.colsB * 4;
	}

	if (is_readB_once) {
	    perf_IS.cycles += IS_model(sizeB / 2, true,
				       config_sys.dram_latency);
	    // printf("perf_IS.cycles: %llu\n",  perf_IS.cycles);
	    perf_IS.bytes += sizeB * 4;
	} else {
	    perf_IS.cycles += IS_model(sizeB * config.rowsA / 2, true,
				       config_sys.dram_latency);
	    perf_IS.bytes += sizeB * config.rowsA * 4;
	}

	perf.bytes = perf_IS.bytes + bytes_store;
	perf.power += IS_AVG_POWER;

	if (perf_IS.cycles > perf.cycles)
	    perf.cycles = perf_IS.cycles;
    }

    // evaluate bandwidth requirement
    // perf.bandwidth = ((float) perf.bytes) / ((float) perf.cycles);

    return perf;
}

// simulator API to invoke GeMM accelerator
acc_perf_t sim_gemm(config_sys_t config_sys, config_gemm_t config_gemm)
{
    // max # accelerator parallelism based on system specs
    unsigned int n_acc_bandwidth_bound = config_sys.mem_bandwidth / 8; // 8 bytes = 1 word
    unsigned int n_acc_max;
    if (n_acc_bandwidth_bound < config_sys.n_acc_tiles &&
	n_acc_bandwidth_bound < config_sys.n_IS_tiles)
	n_acc_max = n_acc_bandwidth_bound;
    else if (config_sys.n_acc_tiles < config_sys.n_IS_tiles)
	n_acc_max = config_sys.n_acc_tiles;
    else
	n_acc_max = config_sys.n_IS_tiles;

    // invoke accelerator
    acc_perf_t perf = dec_gemm_invoke(config_sys, config_gemm);

    // project performance based on accelerator parallelism
    float n_invoke = ((float) config_gemm.batch_size) / ((float) n_acc_max);
    float n_invoke_ceil = ceil(((float) config_gemm.batch_size) / ((float) n_acc_max));
    float utilization = n_invoke / n_invoke_ceil;

    perf.cycles = perf.cycles * n_invoke_ceil;
    perf.bytes = perf.bytes * config_gemm.batch_size;
    perf.power = perf.power * config_sys.n_acc_tiles * utilization;

    // project to required technology
    perf.power = tech_projection(perf.power, GEMM_TECH, config_sys.tech);

    return perf;
}
