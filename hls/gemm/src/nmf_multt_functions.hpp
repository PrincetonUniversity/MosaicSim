/* Copyright 2017 Columbia University, SLD Group */

#ifndef __NMF_MULTT_FUNCTIONS_HPP__
#define __NMF_MULTT_FUNCTIONS_HPP__

// Computational kernels

void nmf_multt::multt_main(uint32_t length,
  A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> &row,
  A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> &col)
{
    for (uint32_t k = 0; k < DMA_CHUNK;)
    {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "main-loop");

        COMPUTE_KERNEL_MAIN_READ;
        COMPUTE_KERNEL_MAIN_COMP;
        COMPUTE_KERNEL_MAIN_INCR;
    }
}

// Utility functions

inline void nmf_multt::calculate_chunks(uint32_t
  &matrix_chk, uint32_t &matrix_rem, uint32_t matrix_d2)
{
     uint32_t matrix_mul;

     {
        HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "calc-chunks");

        // calculating the number of chunks (ceil)
        matrix_chk = matrix_d2 >> DMA_CHUNK_LOG;

        // calculating the number of cols (covered the by the chunks)
        matrix_mul = matrix_chk << DMA_CHUNK_LOG;

        // calculating the remaining cols (size of the last chunk)
        matrix_rem = matrix_d2 - matrix_mul;

        // adding the last chunk if it is necessary
        if (matrix_rem != 0) { ++matrix_chk; }
    }
}

inline void nmf_multt::sync_compute_store(uint32_t &count)
{
    {
        ++count;

        if (count >= DMA_CHUNK)
        {
            count = 0;

            // Call the store_output process
            compute_store_handshake();

            // Wait for the store_output process
            compute_store_2_handshake();
        }
    }
}

inline void nmf_multt::compute_store_2_handshake()
{
    HLS_DEFINE_PROTOCOL("compute-store-2-handshake");

    output_done.ack.ack();
}

inline void nmf_multt::store_compute_2_handshake()
{
    HLS_DEFINE_PROTOCOL("store-compute-2-handshake");

    output_done.req.req();
}

#endif // __NMF_MULTT_FUNCTIONS_HPP__
