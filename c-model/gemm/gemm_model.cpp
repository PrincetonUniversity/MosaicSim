#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "../accelerators.hpp"
#include "../gemm_model.hpp"

#define DMA_CHUNK 64

void calculate_chunks(unsigned &matrix_chk, unsigned &matrix_rem, unsigned colsA)
{
     unsigned int matrix_mul;

     // calculating the number of chunks (ceil)
     matrix_chk = colsA / DMA_CHUNK;

     // calculating the number of cols (covered the by the chunks)
     matrix_mul = matrix_chk * DMA_CHUNK;

     // calculating the remaining cols (size of the last chunk)
     matrix_rem = colsA - matrix_mul;

     // adding the last chunk if it is necessary
     if (matrix_rem != 0) { ++matrix_chk; }
}

void load_model(long long unsigned &cycles, long long unsigned &bytes,
		unsigned length, int has_IS_tile)
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
	cycles += DRAM_LATENCY;

    // read dma channel		
    cycles += length;
    bytes += length;

    // 2. Load chunks of the second matrix

    // send dma read request: length
    cycles++;

    // add DMA latency
    if(has_IS_tile)
	cycles += IS_LATENCY;
    else
	cycles += DRAM_LATENCY;

    // read dma channel
    cycles += length;
    bytes += length;

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
    // TODO get real latency
    cycles += length * 2;
}

long long unsigned IS_model(unsigned length, int is_strided)
{
    bursts = length / IS_BURST_LENGTH;
    last_burst = length % IS_BURST_LENGTH;

    // cycles for the DMA read bursts
    if (!is_strided) {
	// (+2 is for the ping-pong overhead)
	cycles = bursts * (DRAM_LATENCY + IS_BURST_LENGTH + 1) +
	    (DRAM_LATENCY + last_burst + 1);
    } else {
	// DMA transactions of a single WORD
	// assuming up to IS_MAX_DMA_REQS outstanding DMA requests
	cycles = (DRAM_LATENCY / IS_MAX_DMA_REQS) * length;
    }

    return cycles;
}

void store_model(long long unsigned &cycles, long long unsigned &bytes, unsigned length)
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
    cycles += DRAM_LATENCY;

    // write dma channel		
    cycles += length;
    bytes += length;

    // handshake with compute
    cycles++;
}

acc_perf_t dec_gemm_invoke(config_gemm_t config)
{
    acc_perf_t perf, perf_IS;
    long long unsigned cycles_load = 0, cycles_compute = 0, cycles_store = 0;
    long long unsigned bytes_load = 0, bytes_store = 0;
    unsigned matrix_chk, matrix_chk_store;
    unsigned matrix_rem, matrix_rem_store;
    unsigned cA;
    long long unsigned cycles_load_chunk = 0, cycles_load_chunk_rem = 0;
    long long unsigned cycles_compute_chunk = 0, cycles_compute_chunk_rem = 0;
    long long unsigned cycles_store_chunk = 0, cycles_store_chunk_rem = 0;
    long long unsigned bytes_load_chunk = 0, bytes_load_chunk_rem = 0;
    long long unsigned bytes_store_chunk = 0, bytes_store_chunk_rem = 0;

    // pre-evaluate number of chunks and length of last chunk
    calculate_chunks(matrix_chk, matrix_rem, config.colsA);
    calculate_chunks(matrix_chk_store, matrix_rem_store,
		     config.rowsA * config.colsB);

    // precompute performance of loading a chunk
    load_model(cycles_load_chunk, bytes_load_chunk,
	       DMA_CHUNK / 2, config.has_IS_tile);
    load_model(cycles_load_chunk_rem, bytes_load_chunk_rem,
	       matrix_rem / 2, config.has_IS_tile);

    // precompute performance of computing a chunk
    compute_model(cycles_compute_chunk, DMA_CHUNK / 2);
    compute_model(cycles_compute_chunk_rem, matrix_rem / 2);

    // precompute performance of storing a chunk
    store_model(cycles_store_chunk, bytes_store_chunk, DMA_CHUNK / 2);
    store_model(cycles_store_chunk_rem, bytes_store_chunk_rem, matrix_rem_store / 2);

    for (int rA = 0; rA < config.rowsA; ++rA) {
	for (int cB = 0; cB < config.colsB; ++cB) {
	    for (int chk = 0; chk < matrix_chk; ++chk)	{

		// LOAD PHASE
		if (chk == matrix_chk - 1 && matrix_rem != 0) {
		    // this is the last (smaller) chunk
		    cycles_load += cycles_load_chunk_rem;
		    cycles_load += bytes_load_chunk_rem;
		} else {
		    cycles_load += cycles_load_chunk;
		    cycles_load += bytes_load_chunk;
		}

		// COMPUTE PHASE
		if (!chk)
		    cycles_compute += cycles_load;

		if (chk == matrix_chk - 1 && matrix_rem != 0) {
		    // this is the last (smaller) chunk
		    cycles_compute += cycles_compute_chunk_rem;
		    cycles_compute += bytes_compute_chunk_rem;
		} else {
		    cycles_compute += cycles_compute_chunk;
		    cycles_compute += bytes_compute_chunk;
		}
	    }

	    // STORE PHASE (embedded in compute)
	    if (rA*cB == matrix_chk_store - 1 && matrix_rem_store != 0) {
		// this is the last (smaller) chunk
		cycles_compute += cycles_store_chunk_rem;
		cycles_compute += bytes_store_chunk_rem;
	    } else {
		cycles_compute += cycles_store_chunk;
		cycles_compute += bytes_store_chunk;
	    }
	}
    }

    // Execution time
    // the slowest of the three processes determines the execution time
    perf.cycles = std::max(cycles_load, cycles_compute);
    perf.cycles = std::max(perf.cycles, cycles_store);

    // Bandwidth requirement
    // sum load and store bytes accessed
    perf.bytes = bytes_load + bytes_store;

    perf.area = GEMM_AREA;
    perf.power = GEMM_AVG_POWER;

    if (config.has_IS_tile)
    {
	// TODO we are always pairing 1 IS tile with one GeMM accelerator.
	// Consider doing 2 IS tiles with one GeMM accelerator, but
	// we must keep the bandwidth below a certain threshold.

	long long unsigned int sizeA = config.rowsA * config.colsA;
	long long unsigned int sizeB = config.colsA * config.colsB;

	int is_readA_once = false;
	int is_readB_once = false;

	if (config.rowsA + sizeB < IS_MEM_SIZE ||
	    sizeA + config.rowsB < IS_MEM_SIZE) {
	    is_readA_once = true;
	    is_readB_once = true;

	} else if (config.rowsB + IS_MIN_CHUNK < IS_MEM_SIZE) {
	    is_readB_once = true;

	} else if (config.rowsA + IS_MIN_CHUNK < IS_MEM_SIZE) {
	    is_readA_once = true;
	}

	if (is_readA_once) {
	    perf_IS.cycles = IS_model(sizeA, false);
	    perf_IS.bytes = sizeA;
	} else {
	    perf_IS.cycles = IS_model(sizeA * config.colsB, false);
	    perf_IS.bytes = sizeA * config.colsB * 8;
	}

	if (is_readB_once) {
	    perf_IS.cycles += IS_model(sizeB, true);
	    perf_IS.bytes += sizeB * 8;
	} else {
	    perf_IS.cycles += IS_model(sizeB * config.rowsA, true);
	    perf_IS.bytes += sizeB * config.rowsA * 8;
	}

	perf.bytes = perf_IS.bytes + bytes_store;
	perf.area += IS_AREA;
	perf.power += IS_AVG_POWER;

	if (perf_IS.cycles > perf.cycles)
	    perf.cycles = perf_IS_cycles;
    }

    // evaluate bandwidth requirement
    perf.bandwidth = ((float) perf.bytes) / ((float) perf.cycles);

    return perf;
}

// simulator API to invoke GeMM accelerator
acc_perf_t sim_gemm(config_gemm_t config)
{
    int n_invocations = (int) config.batch_size;

    acc_perf_t perf = dec_gemm_invoke(config);

    perf.cycles = perf.cycles * n_invocations;
    perf.bytes = perf.bytes * n_invocations;

    // Commented out because we don't support multi-accelerator

    // int n_invocations = (int) config.batch_size / N_ACC_GEMM;

    // // Call the accelerator only once, then project performance to
    // // n_invocations of N_ACC_GEMM accelerator in parallel. The last
    // // invocation might invoke less than N_ACC_GEMM accelerators.
    // acc_perf_t perf = dec_gemm_invoke(config);

    // perf.cycles = perf.cycles * n_invocations;
    // perf.power = perf.power *
    // 	((float) config.batch / (float) n_invocations);
    // perf.bandwidth = perf.bandwidth *
    // 	((float) config.batch / (float) n_invocations);
    // perf.utilization = (float) config.batch / (float) n_invocations;

    return perf;
}
