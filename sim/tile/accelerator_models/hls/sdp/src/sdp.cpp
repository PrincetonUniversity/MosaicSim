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
        //LOAD_INPUT_RESET_PORTS;

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
			// PLM_A0.port1[0][j] = data.to_uint();
			PLM_A0[j] = data.to_uint();
		    } else {
			// PLM_A1.port1[0][j] = data.to_uint();
			PLM_A1[j] = data.to_uint();
		    }
		} else {
		    if (pingpong) {
			PLM_A0[j << 1] = data.to_uint();
			// PLM_A0.port1[0][j << 1] = data.to_uint();
		    } else {
			// PLM_A1.port1[0][j << 1] = data.to_uint();
			PLM_A1[j << 1] = data.to_uint();
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
			// PLM_A0.port1[0][(j << 1) + 1] = data.to_uint();
			PLM_A0[(j << 1) + 1] = data.to_uint();
		    } else {
			// PLM_A1.port1[0][(j << 1) + 1] = data.to_uint();
			PLM_A1[(j << 1) + 1] = data.to_uint();
		    }

		    wait(); // Only considered in behavioral simulation
		}
	    }

	    indexB += length;
	}

	// Call the compute_kernel process
	load_compute_handshake();

	pingpong = !pingpong;

    }

    // Conclude
    {
        this->process_done();
    }
}

void sdp::compute_kernel()
{
    bool pingpong, pingpong2;

    uint32_t working_mode;
    uint32_t sizeA;
    FPDATA scalar;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-compute");

        this->reset_compute_kernel();

        pingpong = false;
        pingpong2 = false;

	working_mode = 0;
	sizeA = 0;
	scalar = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-compute");

        cfg.wait_for_config();

        working_mode = this->conf_info.read().working_mode;
        sizeA = this->conf_info.read().sizeA;
        scalar = int2fp<FPDATA, WORD_SIZE>(this->conf_info.read().scalar);
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


    for (uint32_t i = 0; i < chunks; ++i) {
	HLS_UNROLL_LOOP(OFF); // disabled

	compute_load_handshake();

	if (i == chunks - 1 && chunks_rem)
	    length = chunks_rem >> 1;



	uint32_t k = 0;
	for (uint32_t j = 0; j < length; j+=4) {

	    HLS_UNROLL_LOOP(OFF); // disabled
	    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "compute-constrain-latency");
		    
	    sc_dt::sc_bv<DMA_WIDTH> data[4];
	    sc_dt::sc_bv<DMA_WIDTH> dataA[4], dataB[4];
	    FPDATA dataA0[4], dataA1[4], dataB0[4], dataB1[4];

	    HLS_FLATTEN_ARRAY(data);
	    HLS_FLATTEN_ARRAY(dataA);
	    HLS_FLATTEN_ARRAY(dataB);
	    HLS_FLATTEN_ARRAY(dataA0);
	    HLS_FLATTEN_ARRAY(dataA1);
	    HLS_FLATTEN_ARRAY(dataB0);
	    HLS_FLATTEN_ARRAY(dataB1);

	    for (uint32_t l = 0; l < 4; l++) {
		HLS_UNROLL_LOOP(ON);

		if (working_mode >= 4) {

		    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "compute-constrain-latency");

		    if (pingpong) {
			// dataA = PLM_A0.port2[0][j];
			dataA[l] = PLM_A0[j + l];
			
		    } else {
			// dataA = PLM_A1.port2[0][j];
			dataA[l] = PLM_A1[j + l];
		    }

		    dataA0[l] = INT2FP(dataA[l].range(31, 0).to_uint());
		    dataA1[l] = INT2FP(dataA[l].range(63, 32).to_uint());
		    
		    if (working_mode == 4) {
			dataA0[l] += scalar;
			dataA1[l] += scalar;
		    } else if (working_mode == 5) {
			dataA0[l] -= scalar;
			dataA1[l] -= scalar;
		    } else if (working_mode == 6) {
			dataA0[l] *= scalar;
			dataA1[l] *= scalar;
		    } else if (working_mode == 7) {
			dataA0[l] /= scalar;
			dataA1[l] /= scalar;
		    } else { // 8
			if (dataA0[l] < 0) dataA0[l] = 0;
			if (dataA1[l] < 0) dataA1[l] = 0;
		    }
		    data[l].range(31, 0) = FP2INT(dataA0[l]);
		    data[l].range(63, 32) = FP2INT(dataA1[l]);

		} else {

		    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "compute-constrain-latency");

		    if (pingpong) {
			// dataA = PLM_A0.port2[0][j << 1];
			// dataB = PLM_A0.port3[0][(j << 1) + 1];
			dataA[l] = PLM_A0[(j + l) << 1];
			dataB[l] = PLM_A0[((j + l) << 1) + 1];
		    } else {
			// dataA = PLM_A1.port2[0][j << 1];
			// dataB = PLM_A1.port3[0][(j << 1) + 1];
			dataA[l] = PLM_A1[(j + l) << 1];
			dataB[l] = PLM_A1[((j + l) << 1) + 1];
		    }

		    dataA0[l] = INT2FP(dataA[l].range(31, 0).to_uint());
		    dataA1[l] = INT2FP(dataA[l].range(63, 32).to_uint());
		    dataB0[l] = INT2FP(dataB[l].range(31, 0).to_uint());
		    dataB1[l] = INT2FP(dataB[l].range(63, 32).to_uint());

		    if (working_mode == 0) {
			dataA0[l] = dataA0[l] + dataB0[l];
			dataA1[l] = dataA1[l] + dataB1[l];
		    } else if (working_mode == 1) {
			dataA0[l] = dataA0[l] - dataB0[l];
			dataA1[l] = dataA1[l] - dataB1[l];
		    } else if (working_mode == 2) {
			dataA0[l] = dataA0[l] * dataB0[l];
			dataA1[l] = dataA1[l] * dataB1[l];
		    } else { // 3
			dataA0[l] = dataA0[l] / dataB0[l];
			dataA1[l] = dataA1[l] / dataB1[l];
		    }

		    data[l].range(31, 0) = FP2INT(dataA0[l]);
		    data[l].range(63, 32) = FP2INT(dataA1[l]);
		}

		if (!pingpong2)
		    PLM_B0[k] = data[l];
		else
		    PLM_B1[k] = data[l];

		++k;
	    }

	    if (k == STORE_CHUNK || j >= length - 1) {
		k = 0;
		compute_store_handshake();
		pingpong2 = !pingpong2;
	    }
	}

	pingpong = !pingpong;

    }

    // Conclude

    {
        this->process_done();
    }
}

void sdp::store_output()
{
    bool pingpong, pingpong2;

    uint32_t working_mode;
    uint32_t sizeA;
    uint32_t st_offset;

    // Reset

    {
        HLS_DEFINE_PROTOCOL("reset-store");

        this->reset_store_output();
        //STORE_OUTPUT_RESET_PORTS;

        pingpong = false;
        pingpong2 = false;

	working_mode = 0;
	sizeA = 0;
	st_offset = 0;

        wait();
    }

    // Config

    {
        HLS_DEFINE_PROTOCOL("config-store");

        cfg.wait_for_config();

        working_mode = this->conf_info.read().working_mode;
        sizeA = this->conf_info.read().sizeA;
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

	if (i == chunks - 1 && chunks_rem)
	    length = chunks_rem >> 1;

	uint32_t k = 0;
	uint32_t stored = 0;
	for (uint32_t j = 0; j < length; j++) {

	    HLS_UNROLL_LOOP(OFF); // disabled
	    HLS_CONSTRAIN_LATENCY(0, HLS_ACHIEVABLE, "store-constrain-latency");
	
	    if (k == 0) {
		store_compute_handshake();

		uint32_t store_length = STORE_CHUNK;
		if (length - stored < STORE_CHUNK)
		    store_length = length - stored;

		stored += store_length;

		{
		    HLS_DEFINE_PROTOCOL("load-tensorA-info");
		    dma_info_t dma_info(index, store_length);
		    this->dma_write_ctrl.put(dma_info);
		}
	    }

	    sc_dt::sc_bv<DMA_WIDTH> data = 0;

	    if (!pingpong2) {
		// data = PLM_B0.port2[0][j];
		data = PLM_B0[k];
			
	    } else {
		// data = PLM_A1.port2[0][j];
		data = PLM_B1[k];
	    }

	    this->dma_write_chnl.put(data);

	    ++k;
	    if (k == STORE_CHUNK || j == length - 1) {
		k = 0;
		pingpong2 = !pingpong2;
	    }
	}

	index += length;
	pingpong = !pingpong;
    }

    // Conclude

    {
        this->accelerator_done();

        this->process_done();
    }
}
