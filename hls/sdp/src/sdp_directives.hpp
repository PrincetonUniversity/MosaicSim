/* Copyright 2018 Columbia University, SLD Group */

#ifndef __SDP_DIRECTIVES_HPP__
#define __SDP_DIRECTIVES_HPP__

// A memory

#define A_MEMORY_TYPE \
  GENERATE_PLM_TYPE(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

#define A_MEMORY_NAME \
  GENERATE_PLM_NAME(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

#define A_MEMORY_HEADER \
  GENERATE_PLM_HDR(A_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

//
// Floating/fixed point directives
//

// Conversions are necessary

#define INT2FP(x) int2fp<FPDATA, WORD_SIZE>(x)
#define FP2INT(x) fp2int<FPDATA, WORD_SIZE>(x)

#define MUL(x, y) (x * y)
#define ADD(x, y) (x + y)

#define LOAD_INPUT_RESET_PORTS \
    PLM_A0.port1.reset(); \
    PLM_A1.port1.reset()

#define STORE_OUTPUT_RESET_PORTS \
    PLM_A0.port2.reset(); \
    PLM_A0.port3.reset(); \
    PLM_A1.port2.reset(); \
    PLM_A1.port3.reset()

#define STORE_OUTPUT_READ_PLM2(B_MEMORY)	\
    data.range(31,0) = B_MEMORY.port2[0][i++];	\
    data.range(63,32) = B_MEMORY.port3[0][i++]

#define COMPUTE_KERNEL_MAIN_READ \
    FPDATA row_elem_1 = INT2FP(row.port3[0][k + 0]); \
    FPDATA row_elem_2 = INT2FP(row.port4[0][k + 1]); \
    FPDATA col_elem_1 = INT2FP(col.port3[0][k + 0]); \
    FPDATA col_elem_2 = INT2FP(col.port4[0][k + 1])

#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = accumulator[0] + (row_elem_1 * col_elem_1); else break; \
    if (k + 1 < length) accumulator[1] = accumulator[1] + (row_elem_2 * col_elem_2); else break

#endif // __SDP_DIRECTIVES_HPP__
