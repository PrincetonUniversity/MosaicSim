#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

// average power consumption estimate in mW
#define GEMM_AREA 300000
#define GEMM_AVG_POWER 20
#define DMA_CHUNK 1024
#define DRAM_LATENCY 300

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

void load_model(long long unsigned &cycles, long long unsigned &bytes, unsigned length)
{
    // iteration delay
    cycles++;

    // send dma read request: length
    cycles++;

    // add DRAM latency
    cycles += DRAM_LATENCY;

    // read dma channel		
    cycles += length;
    bytes += length;

    // 2. Load chunks of the second matrix

    // send dma read request: length
    cycles++;

    // add DRAM latency
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
    cycles += (length * (2 + 4 + 1));
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

    // add DRAM latency
    cycles += DRAM_LATENCY;

    // write dma channel		
    cycles += length;
    bytes += length;

    // handshake with compute
    cycles++;
}

acc_perf_t dec_gemm_invoke(config_gemm_t config)
{
    acc_perf_t perf;
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
    load_model(cycles_load_chunk, bytes_load_chunk, DMA_CHUNK / 2);
    load_model(cycles_load_chunk_rem, bytes_load_chunk_rem, matrix_rem / 2);

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

    // evaluate bandwidth requirement
    perf.bandwidth = ((float) perf.bytes) / ((float) perf.cycles);

    perf.area = GEMM_AREA;
    perf.power = GEMM_AVG_POWER;

    return perf;
}
