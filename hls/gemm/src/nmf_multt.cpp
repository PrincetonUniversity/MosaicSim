/* Copyright 2018 Columbia University SLD Group */

#include "nmf_multt.hpp"

#define DMA_CHUNK_LOG (slog_2<DMA_CHUNK>::value)

// -- Functions

#include "nmf_multt_functions.hpp"

// -- Processes

void nmf_multt::load_input()
{
    bool pingpong;

    uint32_t length;
    uint32_t index_d1;
    uint32_t index_d2;
    uint32_t matrix_d1;
    uint32_t matrix_d2;
    uint32_t matrix_d3;
    uint32_t matrix_chk;
    uint32_t matrix_rem;
    uint32_t ld_offset1;
    uint32_t ld_offset2;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-load");

        this->reset_load_input();
        // LOAD_INPUT_RESET_PORTS;

        pingpong = false;

        length = 0;
        index_d1 = 0;
        index_d2 = 0;
        matrix_d1 = 0;
        matrix_d2 = 0;
        matrix_d3 = 0;
        matrix_chk = 0;
        matrix_rem = 0;
        ld_offset1 = 0;
        ld_offset2 = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-load");

        cfg.wait_for_config();

        matrix_d1 = this->conf_info.read().d1;
        matrix_d2 = this->conf_info.read().d2;
        matrix_d3 = this->conf_info.read().d3;

        ld_offset1 = this->conf_info.read().ld_offset1;
        ld_offset2 = this->conf_info.read().ld_offset2;
   }

    // Compute

    {
        calculate_chunks(matrix_chk, matrix_rem, matrix_d2);

        for (uint32_t d1 = 0; d1 < matrix_d1; ++d1)
        {
            HLS_UNROLL_LOOP(OFF); // disabled

            for (uint32_t d2 = 0; d2 < matrix_d3; ++d2)
            {
                HLS_UNROLL_LOOP(OFF); // disabled

                index_d1 = ld_offset1 + ((d1 * matrix_d2) >> 1);
                index_d2 = ld_offset2 + ((d2 * matrix_d2) >> 1);
                length = DMA_CHUNK >> 1;

                for (uint32_t chk = 0; chk < matrix_chk; ++chk)
                {
                    HLS_UNROLL_LOOP(OFF); // disabled

                    //
                    // 1. Load chunks of the first matrix in PLM2_A0 or PLM2_A1
                    //

                    if (chk == matrix_chk - 1 && matrix_rem != 0)
                       // the next is the last (smaller) chunk
		    { length = matrix_rem >> 1; }

                    {
                        HLS_DEFINE_PROTOCOL("load-matrix1-info");
                        dma_info_t dma_info(index_d1, length);
                        this->dma_read_ctrl.put(dma_info);
                    }

                    uint32_t i = 0;

                    bool high = false;
                    sc_dt::sc_bv<DMA_WIDTH> data_high;

                    for (uint32_t k = 0; k < length; ++k)
                    {
                        HLS_UNROLL_LOOP(OFF); // disabled

                        sc_dt::sc_bv<DMA_WIDTH> data;

                        data = this->dma_read_chnl.get();

                        {
                            // This ensures the maximum throughput

                            HLS_CONSTRAIN_LATENCY(0, 1, "load-matrix1");

                            if (pingpong)
                            {
                                LOAD_INPUT_WRITE_PLM2(PLM2_A0);
                            }
                            else
                            {
                                LOAD_INPUT_WRITE_PLM2(PLM2_A1);
                            }

                            wait(); // Only considered in behavioral simulation
                        }
                    }

                    //
                    // 2. Load chunks of the second matrix in PLM2_A2 or PLM2_A3
                    //

                    {
                        HLS_DEFINE_PROTOCOL("load-matrix2-info");
                        dma_info_t dma_info(index_d2, length);
                        this->dma_read_ctrl.put(dma_info);
                    }

                    i = 0;

                    high = false;

                    for (uint32_t k = 0; k < length; ++k)
                    {
                        HLS_UNROLL_LOOP(OFF); // disabled

                        sc_dt::sc_bv<DMA_WIDTH> data;

                        data = this->dma_read_chnl.get();

                        {
                            // This ensures the maximum throughput

                            HLS_CONSTRAIN_LATENCY(0, 1, "load-matrix2");

                            if (pingpong)
                            {
                                LOAD_INPUT_WRITE_PLM2(PLM2_A2);
                            }
                            else
                            {
                                LOAD_INPUT_WRITE_PLM2(PLM2_A3);
                            }

                            wait(); // Only considered in behavioral simulation
                        }
                    }

                    // Call the compute_kernel process
                    load_compute_handshake();

                    // Change pingpong buffer
                    pingpong = !pingpong;

                    // Update the indices
                    index_d1 += length;
                    index_d2 += length;
                }
            }
        }
    }

    // Conclude

    {
        this->process_done();
    }
}

void nmf_multt::compute_kernel()
{
    bool pingpong;

    uint32_t length;
    uint32_t matrix_d1;
    uint32_t matrix_d2;
    uint32_t matrix_d3;
    uint32_t matrix_chk;
    uint32_t matrix_rem;
    uint32_t store_count;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-compute");

        this->reset_compute_kernel();
        output_done.ack.reset_ack();
        // COMPUTE_KERNEL_RESET_PORTS;

        pingpong = false;

        matrix_d1 = 0;
        matrix_d2 = 0;
        matrix_d3 = 0;
        matrix_chk = 0;
        matrix_rem = 0;
        store_count = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-compute");

        cfg.wait_for_config();

        matrix_d1 = this->conf_info.read().d1;
        matrix_d2 = this->conf_info.read().d2;
        matrix_d3 = this->conf_info.read().d3;
    }

    // Compute

    {
        calculate_chunks(matrix_chk, matrix_rem, matrix_d2);

        for (uint32_t d1 = 0; d1 < matrix_d1; ++d1)
        {
            HLS_UNROLL_LOOP(OFF); // disabled

            for (uint32_t d2 = 0; d2 < matrix_d3; ++d2)
            {
                HLS_UNROLL_LOOP(OFF); // disabled

                {
                    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "reset-acc");

                    for (uint8_t i = 0; i < NUM_PORTS; ++i)
                    {
                        HLS_UNROLL_LOOP(ON, "reduce");
                        accumulator[i] = FPDATA(0.0);
                    }
                }

                length = DMA_CHUNK;

                for (uint32_t chk = 0; chk < matrix_chk; ++chk)
                {
                    HLS_UNROLL_LOOP(OFF); // disabled

                    if (chk == matrix_chk - 1 && matrix_rem != 0)
                       // the next is the last (smaller) chunk
                       { length = matrix_rem; }

                    // Wait the load_input process
                    compute_load_handshake();

                    if (pingpong)
                        multt_main(length, PLM2_A0, PLM2_A2);
                    else
                        multt_main(length, PLM2_A1, PLM2_A3);

                    pingpong = !pingpong;
                }

                {
                    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "reduce-acc");

                    for (uint8_t i = 1; i < NUM_PORTS; ++i)
                    {
                        HLS_UNROLL_LOOP(ON, "reduce");
                        accumulator[0] += accumulator[i];
                    }
                }

                // PLM2_B0.port1[0][store_count] =
                //   FP2INT(accumulator[0]);
                PLM2_B0[store_count] =
                  FP2INT(accumulator[0]);

                // Call the store_output process and
                // wait for the store_output process
                // -> PLM2_B0 is not in pingpong
                sync_compute_store(store_count);
            }
        }

        // Force to store the last chunk
        store_count = DMA_CHUNK - 1;
        sync_compute_store(store_count);
    }

    // Conclude

    {
        this->process_done();
    }
}

void nmf_multt::store_output()
{
    uint32_t index;
    uint32_t length;
    uint32_t matrix_d1;
    uint32_t matrix_d3;
    uint32_t matrix_chk;
    uint32_t matrix_rem;
    uint32_t matrix_out;
    uint32_t st_offset;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-store");

        this->reset_store_output();
        output_done.req.reset_req();
        // STORE_OUTPUT_RESET_PORTS;

        index = 0;
        length = 0;
        matrix_d1 = 0;
        matrix_d3 = 0;
        matrix_chk = 0;
        matrix_rem = 0;
        matrix_out = 0;
        st_offset = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-store");

        cfg.wait_for_config();

        matrix_d1 = this->conf_info.read().d1;
        matrix_d3 = this->conf_info.read().d3;
        st_offset = this->conf_info.read().st_offset;
   }

    // Compute

    {
        // Calculating number of outputs to generate
        matrix_out = matrix_d1 * matrix_d3;
        calculate_chunks(matrix_chk, matrix_rem, matrix_out);

        index = st_offset;

        length = DMA_CHUNK >> 1;

        for (uint32_t chk = 0; chk < matrix_chk; ++chk)
        {
            HLS_UNROLL_LOOP(OFF); // disabled

            if (chk == matrix_chk - 1 && matrix_rem != 0)
               // the next is the last (smaller) chunk
	    { length = (matrix_rem >> 1); }

            // Wait the compute_process
            store_compute_handshake();

            {
                HLS_DEFINE_PROTOCOL("store-matrix-info");

                dma_info_t dma_info(index, length);
                this->dma_write_ctrl.put(dma_info);
            }

            uint32_t i = 0;

            bool high = false;

            for (uint32_t k = 0; k < length; ++k)
            {
                HLS_UNROLL_LOOP(OFF); // disabled

                sc_dt::sc_bv<DMA_WIDTH> data = 0;

                {
                    // This ensures the maximum throughput

                    HLS_CONSTRAIN_LATENCY(0, 1, "store-matrix");

                    STORE_OUTPUT_READ_PLM2(PLM2_B0);

                    wait(); // Only considered in behavioral simulation
                }

                this->dma_write_chnl.put(data);
            }

            // release compute_process
            store_compute_2_handshake();

            // update the index
            index += length;
       }
    }

    // Conclude

    {
        this->accelerator_done();

        this->process_done();
    }
}
