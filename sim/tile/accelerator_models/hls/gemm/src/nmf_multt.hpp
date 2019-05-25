/* Copyright 2017 Columbia University, SLD Group */

#ifndef __NMF_MULTT_HPP__
#define __NMF_MULTT_HPP__

#include "nmf_data.hpp"

#include "nmf_multt_config.hpp"

#include "nmf_multt_directives.hpp"

#include "esp_templates.hpp"

//#include A_MEMORY_HEADER
//#include B_MEMORY_HEADER

class nmf_multt: public esp_accelerator_3P<DMA_WIDTH>
{
    public:

        // -- Instances

        // Config process
        esp_config_proc cfg;

        // -- Handshakes

        // Additional handshake
        handshake_t output_done;

        // Declaration of the accelerator PLMs
	// A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> PLM2_A0;
        // A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> PLM2_A1;
        // A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> PLM2_A2;
        // A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> PLM2_A3;
        // B_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> PLM2_B0;
	FPDATA_WORD PLM2_A0[DMA_CHUNK];
        FPDATA_WORD PLM2_A1[DMA_CHUNK];
        FPDATA_WORD PLM2_A2[DMA_CHUNK];
        FPDATA_WORD PLM2_A3[DMA_CHUNK];
        FPDATA_WORD PLM2_B0[DMA_CHUNK];

        FPDATA accumulator[NUM_PORTS]; // flatten

        // -- Module constructor

        SC_HAS_PROCESS(nmf_multt);
        nmf_multt(const sc_module_name &name)
            : esp_accelerator_3P<DMA_WIDTH>(name)
            , cfg("configuration_process")
            , output_done("output_done")
        {
            // Signal binding with config
            cfg.bind_with<DMA_WIDTH>(*this);

            // Signal binding with the handshake
            output_done.bind_with<DMA_WIDTH>(*this);

            // Always flatten the registers
            HLS_FLATTEN_ARRAY(accumulator);

            // Binding explicit memories
            // PLM2_A0.clk(this->clk);
            // PLM2_A1.clk(this->clk);
            // PLM2_A2.clk(this->clk);
            // PLM2_A3.clk(this->clk);
            // PLM2_B0.clk(this->clk);
            HLS_FLATTEN_ARRAY(PLM2_A0);
            HLS_FLATTEN_ARRAY(PLM2_A1);
            HLS_FLATTEN_ARRAY(PLM2_A2);
            HLS_FLATTEN_ARRAY(PLM2_A3);
            HLS_FLATTEN_ARRAY(PLM2_B0);
        }

        // -- Processes

        // Load input from memory
        void load_input();

        // Perform the computation
        void compute_kernel();

        // Store output in memory
        void store_output();

        // -- Functions

        // Calculate the number of chunks and remaining cols
        inline void calculate_chunks(uint32_t &matrix_chk,
          uint32_t &matrix_rem, uint32_t matrix_d2);

        // Synchronize compute_kernel and store_output processes
        inline void sync_compute_store(uint32_t &count);

        // Handshake callable from compute_kernel
        inline void compute_store_2_handshake();

        // Handshake callable from store_output
        inline void store_compute_2_handshake();

        // -- Kernels

        // Matrix multiplication kernel
        // void multt_main(uint32_t length,
        //   A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> &row,
        //   A_MEMORY_TYPE<FPDATA_WORD, DMA_CHUNK> &col);
        void multt_main(uint32_t length,
			FPDATA_WORD *row,
			FPDATA_WORD *col);
};

#endif // __NMF_MULTT_HPP__
