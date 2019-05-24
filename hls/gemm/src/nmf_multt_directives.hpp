/* Copyright 2018 Columbia University, SLD Group */

#ifndef __NMF_MULTT_DIRECTIVES_HPP__
#define __NMF_MULTT_DIRECTIVES_HPP__

// A memory

#define A_MEMORY_TYPE \
  GENERATE_PLM_TYPE(A_MEMORY, DMA_WIDTH, DMA_CHUNK, NUM_PORTS)

#define A_MEMORY_NAME \
  GENERATE_PLM_NAME(A_MEMORY, DMA_WIDTH, DMA_CHUNK, NUM_PORTS)

#define A_MEMORY_HEADER \
  GENERATE_PLM_HDR(A_MEMORY, DMA_WIDTH, DMA_CHUNK, NUM_PORTS)

// B memory

// Using the same memories of A

#define B_MEMORY_TYPE \
  GENERATE_PLM_TYPE(B_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

#define B_MEMORY_NAME \
  GENERATE_PLM_NAME(B_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

#define B_MEMORY_HEADER \
  GENERATE_PLM_HDR(B_MEMORY, DMA_WIDTH, DMA_CHUNK, 2)

//
// Floating/fixed point directives
//

// Conversions are necessary

#define INT2FP(x) int2fp<FPDATA, WORD_SIZE>(x)
#define FP2INT(x) fp2int<FPDATA, WORD_SIZE>(x)

#define MUL(x, y) (x * y)
#define ADD(x, y) (x + y)

//
// DMA WIDTH
//

#define LOAD_INPUT_RESET_PORTS \
    PLM2_A0.port1.reset(); \
    PLM2_A1.port1.reset(); \
    PLM2_A2.port1.reset(); \
    PLM2_A3.port1.reset(); \
    PLM2_A0.port2.reset(); \
    PLM2_A1.port2.reset(); \
    PLM2_A2.port2.reset(); \
    PLM2_A3.port2.reset()

#define LOAD_INPUT_WRITE_PLM2(A_MEMORY)				\
    A_MEMORY[i++] = data.range(31,0).to_uint();	\
    A_MEMORY[i++] = data.range(63,32).to_uint()

    // A_MEMORY.port1[0][i++] = data.range(31,0).to_uint();	\
    // A_MEMORY.port2[0][i++] = data.range(63,32).to_uint()

#define STORE_OUTPUT_RESET_PORTS \
    PLM2_B0.port2.reset(); \
    PLM2_B0.port3.reset()

#define STORE_OUTPUT_READ_PLM2(B_MEMORY)	\
    data.range(31,0) = B_MEMORY[i++];	\
    data.range(63,32) = B_MEMORY[i++]

    // data.range(31,0) = B_MEMORY.port2[0][i++];	\
    // data.range(63,32) = B_MEMORY.port3[0][i++]

// 1-port memory

#if NUM_PORTS == 1

#define COMPUTE_KERNEL_RESET_PORTS \
    PLM2_A0.port3.reset(); \
    PLM2_A1.port3.reset(); \
    PLM2_A2.port3.reset(); \
    PLM2_A3.port3.reset(); \
    PLM2_B0.port1.reset()

#define COMPUTE_KERNEL_MAIN_READ \
    FPDATA row_elem_1 = INT2FP(row[k]); \
    FPDATA col_elem_1 = INT2FP(col[k]);
    // FPDATA row_elem_1 = INT2FP(row.port3[0][k]); \
    // FPDATA col_elem_1 = INT2FP(col.port3[0][k]);

#if defined(FIXED_POINT)
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k < length) accumulator[0] = esp_f64_add(accumulator[0].to_uint64(), esp_f64_mul(row_elem_1.to_uint64(), col_elem_1.to_uint64())); else break
#else
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k < length) accumulator[0] = accumulator[0] + (row_elem_1 * col_elem_1); else break
#endif

#define COMPUTE_KERNEL_MAIN_INCR k += 1

// 2-ports memory

#elif NUM_PORTS == 2

#define COMPUTE_KERNEL_RESET_PORTS \
    PLM2_A0.port4.reset(); \
    PLM2_A0.port3.reset(); \
    PLM2_A1.port4.reset(); \
    PLM2_A1.port3.reset(); \
    PLM2_A2.port4.reset(); \
    PLM2_A2.port3.reset(); \
    PLM2_A3.port4.reset(); \
    PLM2_A3.port3.reset(); \
    PLM2_B0.port1.reset()

#define COMPUTE_KERNEL_MAIN_READ \
    FPDATA row_elem_1 = INT2FP(row[k + 0]); \
    FPDATA row_elem_2 = INT2FP(row[k + 1]); \
    FPDATA col_elem_1 = INT2FP(col[k + 0]); \
    FPDATA col_elem_2 = INT2FP(col[k + 1])
    // FPDATA row_elem_1 = INT2FP(row.port3[0][k + 0]); \
    // FPDATA row_elem_2 = INT2FP(row.port4[0][k + 1]); \
    // FPDATA col_elem_1 = INT2FP(col.port3[0][k + 0]); \
    // FPDATA col_elem_2 = INT2FP(col.port4[0][k + 1])

#if defined(FIXED_POINT)
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = ADD(accumulator[0], MUL(row_elem_1, col_elem_1)); else break; \
    if (k + 1 < length) accumulator[1] = ADD(accumulator[1], MUL(row_elem_2, col_elem_2)); else break
#else
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = accumulator[0] + (row_elem_1 * col_elem_1); else break; \
    if (k + 1 < length) accumulator[1] = accumulator[1] + (row_elem_2 * col_elem_2); else break
#endif

#define COMPUTE_KERNEL_MAIN_INCR k += 2

// 4-ports memory

#elif NUM_PORTS == 4

#define COMPUTE_KERNEL_RESET_PORTS \
    PLM2_A0.port6.reset(); \
    PLM2_A0.port3.reset(); \
    PLM2_A0.port4.reset(); \
    PLM2_A0.port5.reset(); \
    PLM2_A1.port6.reset(); \
    PLM2_A1.port3.reset(); \
    PLM2_A1.port4.reset(); \
    PLM2_A1.port5.reset(); \
    PLM2_A2.port6.reset(); \
    PLM2_A2.port3.reset(); \
    PLM2_A2.port4.reset(); \
    PLM2_A2.port5.reset(); \
    PLM2_A3.port6.reset(); \
    PLM2_A3.port3.reset(); \
    PLM2_A3.port4.reset(); \
    PLM2_A3.port5.reset(); \
    PLM2_B0.port1.reset()

#define COMPUTE_KERNEL_MAIN_READ \
    FPDATA row_elem_1 = INT2FP(row[k + 0]); \
    FPDATA row_elem_2 = INT2FP(row[k + 1]); \
    FPDATA row_elem_3 = INT2FP(row[k + 2]); \
    FPDATA row_elem_4 = INT2FP(row[k + 3]); \
    FPDATA col_elem_1 = INT2FP(col[k + 0]); \
    FPDATA col_elem_2 = INT2FP(col[k + 1]); \
    FPDATA col_elem_3 = INT2FP(col[k + 2]); \
    FPDATA col_elem_4 = INT2FP(col[k + 3])

    // FPDATA row_elem_1 = INT2FP(row.port3[0][k + 0]); \
    // FPDATA row_elem_2 = INT2FP(row.port4[0][k + 1]); \
    // FPDATA row_elem_3 = INT2FP(row.port5[0][k + 2]); \
    // FPDATA row_elem_4 = INT2FP(row.port6[0][k + 3]); \
    // FPDATA col_elem_1 = INT2FP(col.port3[0][k + 0]); \
    // FPDATA col_elem_2 = INT2FP(col.port4[0][k + 1]); \
    // FPDATA col_elem_3 = INT2FP(col.port5[0][k + 2]); \
    // FPDATA col_elem_4 = INT2FP(col.port6[0][k + 3])

#if defined(FIXED_POINT)
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = ADD(accumulator[0], MUL(row_elem_1, col_elem_1)); else break; \
    if (k + 1 < length) accumulator[1] = ADD(accumulator[1], MUL(row_elem_2, col_elem_2)); else break; \
    if (k + 2 < length) accumulator[2] = ADD(accumulator[2], MUL(row_elem_3, col_elem_3)); else break; \
    if (k + 3 < length) accumulator[3] = ADD(accumulator[3], MUL(row_elem_4, col_elem_4)); else break
#else
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = accumulator[0] + (row_elem_1 * col_elem_1); else break; \
    if (k + 1 < length) accumulator[1] = accumulator[1] + (row_elem_2 * col_elem_2); else break; \
    if (k + 2 < length) accumulator[2] = accumulator[2] + (row_elem_3 * col_elem_3); else break; \
    if (k + 3 < length) accumulator[3] = accumulator[3] + (row_elem_4 * col_elem_4); else break
#endif

#define COMPUTE_KERNEL_MAIN_INCR k += 4

// 8-ports memory

#elif NUM_PORTS == 8

#define COMPUTE_KERNEL_RESET_PORTS \
    PLM2_A0.port10.reset(); \
    PLM2_A0.port3.reset(); \
    PLM2_A0.port4.reset(); \
    PLM2_A0.port5.reset(); \
    PLM2_A0.port6.reset(); \
    PLM2_A0.port7.reset(); \
    PLM2_A0.port8.reset(); \
    PLM2_A0.port9.reset(); \
    PLM2_A1.port10.reset(); \
    PLM2_A1.port3.reset(); \
    PLM2_A1.port4.reset(); \
    PLM2_A1.port5.reset(); \
    PLM2_A1.port6.reset(); \
    PLM2_A1.port7.reset(); \
    PLM2_A1.port8.reset(); \
    PLM2_A1.port9.reset(); \
    PLM2_A2.port10.reset(); \
    PLM2_A2.port3.reset(); \
    PLM2_A2.port4.reset(); \
    PLM2_A2.port5.reset(); \
    PLM2_A2.port6.reset(); \
    PLM2_A2.port7.reset(); \
    PLM2_A2.port8.reset(); \
    PLM2_A2.port9.reset(); \
    PLM2_A3.port10.reset(); \
    PLM2_A3.port3.reset(); \
    PLM2_A3.port4.reset(); \
    PLM2_A3.port5.reset(); \
    PLM2_A3.port6.reset(); \
    PLM2_A3.port7.reset(); \
    PLM2_A3.port8.reset(); \
    PLM2_A3.port9.reset(); \
    PLM2_B0.port1.reset()

#define COMPUTE_KERNEL_MAIN_READ \
    FPDATA row_elem_1 = INT2FP(row[k + 0]); \
    FPDATA row_elem_2 = INT2FP(row[k + 1]); \
    FPDATA row_elem_3 = INT2FP(row[k + 2]); \
    FPDATA row_elem_4 = INT2FP(row[k + 3]); \
    FPDATA row_elem_5 = INT2FP(row[k + 4]); \
    FPDATA row_elem_6 = INT2FP(row[k + 5]); \
    FPDATA row_elem_7 = INT2FP(row[k + 6]); \
    FPDATA row_elem_8 = INT2FP(row[k + 7]); \
    FPDATA col_elem_1 = INT2FP(col[k + 0]); \
    FPDATA col_elem_2 = INT2FP(col[k + 1]); \
    FPDATA col_elem_3 = INT2FP(col[k + 2]); \
    FPDATA col_elem_4 = INT2FP(col[k + 3]); \
    FPDATA col_elem_5 = INT2FP(col[k + 4]); \
    FPDATA col_elem_6 = INT2FP(col[k + 5]); \
    FPDATA col_elem_7 = INT2FP(col[k + 6]); \
    FPDATA col_elem_8 = INT2FP(col[k + 7])
    // FPDATA row_elem_1 = INT2FP(row.port3[0][k + 0]); \
    // FPDATA row_elem_2 = INT2FP(row.port4[0][k + 1]); \
    // FPDATA row_elem_3 = INT2FP(row.port5[0][k + 2]); \
    // FPDATA row_elem_4 = INT2FP(row.port6[0][k + 3]); \
    // FPDATA row_elem_5 = INT2FP(row.port7[0][k + 4]); \
    // FPDATA row_elem_6 = INT2FP(row.port8[0][k + 5]); \
    // FPDATA row_elem_7 = INT2FP(row.port9[0][k + 6]); \
    // FPDATA row_elem_8 = INT2FP(row.port10[0][k + 7]); \
    // FPDATA col_elem_1 = INT2FP(col.port3[0][k + 0]); \
    // FPDATA col_elem_2 = INT2FP(col.port4[0][k + 1]); \
    // FPDATA col_elem_3 = INT2FP(col.port5[0][k + 2]); \
    // FPDATA col_elem_4 = INT2FP(col.port6[0][k + 3]); \
    // FPDATA col_elem_5 = INT2FP(col.port7[0][k + 4]); \
    // FPDATA col_elem_6 = INT2FP(col.port8[0][k + 5]); \
    // FPDATA col_elem_7 = INT2FP(col.port9[0][k + 6]); \
    // FPDATA col_elem_8 = INT2FP(col.port10[0][k + 7])

#if defined(FIXED_POINT)
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = ADD(accumulator[0], MUL(row_elem_1, col_elem_1)); else break; \
    if (k + 1 < length) accumulator[1] = ADD(accumulator[1], MUL(row_elem_2, col_elem_2)); else break; \
    if (k + 2 < length) accumulator[2] = ADD(accumulator[2], MUL(row_elem_3, col_elem_3)); else break; \
    if (k + 3 < length) accumulator[3] = ADD(accumulator[3], MUL(row_elem_4, col_elem_4)); else break; \
    if (k + 4 < length) accumulator[4] = ADD(accumulator[4], MUL(row_elem_5, col_elem_5)); else break; \
    if (k + 5 < length) accumulator[5] = ADD(accumulator[5], MUL(row_elem_6, col_elem_6)); else break; \
    if (k + 6 < length) accumulator[6] = ADD(accumulator[6], MUL(row_elem_7, col_elem_7)); else break; \
    if (k + 7 < length) accumulator[7] = ADD(accumulator[7], MUL(row_elem_8, col_elem_8)); else break
#else
#define COMPUTE_KERNEL_MAIN_COMP \
    if (k + 0 < length) accumulator[0] = accumulator[0] + (row_elem_1 * col_elem_1); else break; \
    if (k + 1 < length) accumulator[1] = accumulator[1] + (row_elem_2 * col_elem_2); else break; \
    if (k + 2 < length) accumulator[2] = accumulator[2] + (row_elem_3 * col_elem_3); else break; \
    if (k + 3 < length) accumulator[3] = accumulator[3] + (row_elem_4 * col_elem_4); else break; \
    if (k + 4 < length) accumulator[4] = accumulator[4] + (row_elem_5 * col_elem_5); else break; \
    if (k + 5 < length) accumulator[5] = accumulator[5] + (row_elem_6 * col_elem_6); else break; \
    if (k + 6 < length) accumulator[6] = accumulator[6] + (row_elem_7 * col_elem_7); else break; \
    if (k + 7 < length) accumulator[7] = accumulator[7] + (row_elem_8 * col_elem_8); else break
#endif

#define COMPUTE_KERNEL_MAIN_INCR k += 8

#else // NUM_PORTS not defined

#define COMPUTE_KERNEL_RESET_PORTS
#define COMPUTE_KERNEL_MAIN_READ
#define COMPUTE_KERNEL_MAIN_COMP
#define COMPUTE_KERNEL_MAIN_INCR

#endif // NUM_PORTS

#endif // __NMF_MULTT_DIRECTIVES_HPP__
