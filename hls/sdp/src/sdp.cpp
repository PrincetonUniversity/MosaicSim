/* Copyright 2018 Columbia University SLD Group */

#include "sdp.hpp"

#define DMA_CHUNK_LOG (slog_2<DMA_CHUNK>::value)

// -- Functions

#include "sdp_functions.hpp"

// -- Processes

void sdp::load_input()
{
    bool pingpong;

    uint32_t working_mode;
    uint32_t sizeA;
    uint32_t ld_offsetA;
    uint32_t ld_offsetB;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-load");

        this->reset_load_input();
        LOAD_INPUT_RESET_PORTS;

        pingpong = false;

	working_mode = 0;
	sizeA = 0;
	ld_offsetA = 0;
	ld_offsetB = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-load");

        cfg.wait_for_config();

        working_mode = this->conf_info.read().working_mode;
        sizeA = this->conf_info.read().sizeA;
        ld_offsetA = this->conf_info.read().ld_offsetA;
        ld_offsetB = this->conf_info.read().ld_offsetB;
   }

    // Compute

    uint32_t chunk_size = DMA_CHUNK;
    if (working_mode >= 4)
	chunk_size = (DMA_CHUNK << 1);

    uint32_t chunks = sizeA / chunk_size;
    uint32_t chunks_rem = sizeA - (chunks * chunk_size);
    if (chunks_rem)
	++chunks;

    uint32_t length = chunk_size >> 1;
    uint32_t indexA = ld_offsetA;
    uint32_t indexB = ld_offsetB;

    for (uint32_t i = 0; i < chunks; ++i) {
	HLS_UNROLL_LOOP(OFF); // disabled

	if (i == chunks - 1 && chunks_rem)
	    length = chunks_rem >> 1;

	{
	    HLS_DEFINE_PROTOCOL("load-tensorA-info");
	    dma_info_t dma_info(indexA, length);
	    this->dma_read_ctrl.put(dma_info);
	}

	for (uint32_t j = 0; j < length; j++) {
	    HLS_UNROLL_LOOP(OFF); // disabled

	    sc_dt::sc_bv<DMA_WIDTH> data;
	    data = this->dma_read_chnl.get();

	    {
		HLS_CONSTRAIN_LATENCY(0, 1, "load-tensorA");

		if (working_mode >= 4) {
		    if (pingpong) {
			PLM_A0.port1[0][j] = data.to_uint();
		    } else {
			PLM_A1.port1[0][j] = data.to_uint();
		    }
		} else {
		    if (pingpong) {
			PLM_A0.port1[0][j << 1] = data.to_uint();
		    } else {
			PLM_A1.port1[0][j << 1] = data.to_uint();
		    }
		}

		wait(); // Only considered in behavioral simulation
	    }
	}

	indexA += length;

	if (working_mode < 4) {

	    {
		HLS_DEFINE_PROTOCOL("load-tensorA-info");
		dma_info_t dma_info(indexB, length);
		this->dma_read_ctrl.put(dma_info);
	    }


	    for (uint32_t j = 0; j < length; j++) {
		HLS_UNROLL_LOOP(OFF); // disabled

		sc_dt::sc_bv<DMA_WIDTH> data;
		data = this->dma_read_chnl.get();

		{
		    HLS_CONSTRAIN_LATENCY(0, 1, "load-tensorA");

		    if (pingpong) {
			PLM_A0.port1[0][(j << 1) + 1] = data.to_uint();
		    } else {
			PLM_A1.port1[0][(j << 1) + 1] = data.to_uint();
		    }

		    wait(); // Only considered in behavioral simulation
		}
	    }

	    indexB += length;
	}

	// Call the compute_kernel process
	load_store_handshake();

	pingpong = !pingpong;

    }

    // Conclude
    {
        this->process_done();
    }
}

void sdp::compute_kernel()
{
    // Conclude

    {
        this->process_done();
    }
}

void sdp::store_output()
{
    bool pingpong;

    uint32_t working_mode;
    uint32_t sizeA;
    FPDATA scalar;
    uint32_t st_offset;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-load");

        this->reset_load_input();
        LOAD_INPUT_RESET_PORTS;

        pingpong = false;

	working_mode = 0;
	sizeA = 0;
	scalar = 0;
	st_offset = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-load");

        cfg.wait_for_config();

        working_mode = this->conf_info.read().working_mode;
        sizeA = this->conf_info.read().sizeA;
        scalar = (FPDATA) this->conf_info.read().scalar;
        st_offset = this->conf_info.read().st_offset;
   }

   // Compute

    uint32_t chunk_size = DMA_CHUNK;
    if (working_mode >= 4)
	chunk_size = (DMA_CHUNK << 1);

    uint32_t chunks = sizeA / chunk_size;
    uint32_t chunks_rem = sizeA - (chunks * chunk_size);
    if (chunks_rem)
	++chunks;

    uint32_t length = chunk_size >> 1;
    uint32_t index = st_offset;

    for (uint32_t i = 0; i < chunks; ++i) {
	HLS_UNROLL_LOOP(OFF); // disabled

	store_load_handshake();

	if (i == chunks - 1 && chunks_rem)
	    length = chunks_rem >> 1;

	{
	    HLS_DEFINE_PROTOCOL("load-tensorA-info");
	    dma_info_t dma_info(index, length);
	    this->dma_write_ctrl.put(dma_info);
	}

	for (uint32_t j = 0; j < length; j++) {
	    HLS_UNROLL_LOOP(OFF); // disabled

	    sc_dt::sc_bv<DMA_WIDTH> data = 0;
	    uint64_t dataA, dataB;
	    FPDATA dataA0, dataA1, dataB0, dataB1;

	    {
		HLS_CONSTRAIN_LATENCY(0, 1, "store-tensor");

		if (working_mode >= 4) {
		    if (pingpong) {
			dataA = PLM_A0.port2[0][j];
		    } else {
			dataA = PLM_A1.port2[0][j];
		    }
		    dataA0 = int2fp<FPDATA, WORD_SIZE>(dataA.range(31, 0));
		    dataA1 = int2fp<FPDATA, WORD_SIZE>(dataA.range(63, 32));
		    
		    if (working_mode == 4) {
			dataA0 += scalar;
			dataA1 += scalar;
		    } else if (working_mode == 5) {
			dataA0 -= scalar;
			dataA1 -= scalar;
		    } else if (working_mode == 6) {
			dataA0 *= scalar;
			dataA1 *= scalar;
		    } else if (working_mode == 7) {
			dataA0 /= scalar;
			dataA1 /= scalar;
		    } else {
			if (dataA0 < 0) dataA0 = 0;
			if (dataA1 < 0) dataA1 = 0;
		    }
		    data.range(31, 0) = fp2int<FPDATA, WORD_SIZE>(dataA0);
		    data.range(63, 32) = fp2int<FPDATA, WORD_SIZE>(dataA1);

		} else {
		    if (pingpong) {
			dataA = PLM_A0.port2[0][j << 1];
			dataB = PLM_A0.port3[0][(j << 1) + 1];
		    } else {
			dataA = PLM_A1.port2[0][j << 1];
			dataB = PLM_A1.port3[0][(j << 1) + 1];
		    }

		    dataA0 = int2fp<FPDATA, WORD_SIZE>(dataA.range(31, 0));
		    dataA1 = int2fp<FPDATA, WORD_SIZE>(dataA.range(63, 32));
		    dataB0 = int2fp<FPDATA, WORD_SIZE>(dataB.range(31, 0));
		    dataB1 = int2fp<FPDATA, WORD_SIZE>(dataB.range(63, 32));

		    if (working_mode == 4) {
			dataA0 = dataA0 + dataB0;
			dataA1 = dataA1 + dataB1;
		    } else if (working_mode == 5) {
			dataA0 = dataA0 - dataB0;
			dataA1 = dataA1 - dataB1;
		    } else if (working_mode == 6) {
			dataA0 = dataA0 * dataB0;
			dataA1 = dataA1 * dataB1;
		    } else (working_mode == 7) {
			dataA0 = dataA0 / dataB0;
			dataA1 = dataA1 / dataB1;
		    }

		    data.range(31, 0) = fp2int<FPDATA, WORD_SIZE>(dataA0);
		    data.range(63, 32) = fp2int<FPDATA, WORD_SIZE>(dataA1);
		}
	    }

	    this->dma_write_chnl.put(data);
	}

	indexA += length;
	pingpong = !pingpong;

    }

    // Conclude

    {
        this->accelerator_done();

        this->process_done();
    }
}
