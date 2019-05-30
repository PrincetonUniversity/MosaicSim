/* Copyright 2018 Columbia University, SLD Group */

#ifndef __SDP_DIRECTIVES_HPP__
#define __SDP_DIRECTIVES_HPP__

// A memory

// #define A_MEMORY_TYPE \
//   GENERATE_PLM_TYPE(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

// #define A_MEMORY_NAME \
//   GENERATE_PLM_NAME(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

// #define A_MEMORY_HEADER \
//   GENERATE_PLM_HDR(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

//
// Floating/fixed point directives
//

// Conversions are necessary

#define INT2FP(x) int2fp<FPDATA, WORD_SIZE>(x)
#define FP2INT(x) fp2int<FPDATA, WORD_SIZE>(x)

#define LOAD_INPUT_RESET_PORTS \
    PLM_A0.port1.reset(); \
    PLM_A1.port1.reset()

#define STORE_OUTPUT_RESET_PORTS \
    PLM_A0.port2.reset(); \
    PLM_A0.port3.reset(); \
    PLM_A1.port2.reset(); \
    PLM_A1.port3.reset()

#endif // __SDP_DIRECTIVES_HPP__
