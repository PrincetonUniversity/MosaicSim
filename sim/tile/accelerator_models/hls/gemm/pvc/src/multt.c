/* Copyright 2017 Columbia University, SLD Group */

#include <assert.h>

#if defined(USE_CBLAS)
#include "cblas.h"
#endif // USE_CBLAS

#include "multt.h"

void _multt(double_matrix_t *matrix_in1, double_matrix_t*
  matrix_in2, double_matrix_t *matrix_out)
{
    assert(matrix_in1->dim == 2);
    assert(matrix_in2->dim == 2);

    assert(matrix_in1->dims[1] ==
           matrix_in2->dims[1]);

#if defined(USE_CBLAS)
    // Perform the following operation: C := alpha * A *  B^T + beta * C
    cblas_dgemm(CblasRowMajor,        // row-major order representation
                CblasNoTrans,         // matrix_in1 no transposed
                CblasTrans,           // matrix_in2 is transposed 
                matrix_in1->dims[0],  // M -> matrix_in1 rows
                matrix_in2->dims[0],  // N -> matrix_in2 rows
                matrix_in2->dims[1],  // K -> matrix_in2 cols
                (double) 1.0,         // alpha coeff -> 1.0
                matrix_in1->data,     // matrix_in1 val  
                matrix_in1->dims[1],  // matrix_in1 ld             
                matrix_in2->data,     // matrix_in2 val
                matrix_in2->dims[1],  // matrix_in2 ld
                (double) 0.0,         // beta coeff -> 0.0
                matrix_out->data,     // matrix_out val
                matrix_in2->dims[0]); // matrix_out ld
#else //  Not-efficient implementation 

    unsigned d1, d2, k; 
    double accumulator;

    for (d1 = 0; d1 < matrix_in1->dims[0]; ++d1)
    {
        for (d2 = 0; d2 < matrix_in2->dims[0]; ++d2)
        {
            accumulator = 0.0;

            for (k = 0; k < matrix_in1->dims[1]; ++k)
            {
                accumulator +=
                    matrix_in1->data[d1 * matrix_in1->dims[1] + k] *
                    matrix_in2->data[d2 * matrix_in2->dims[1] + k];
            }

            matrix_out->data[d1 * matrix_in2->dims[0] + d2] = accumulator;
        }
    }
#endif
}

void multt(double_matrix_t *matrix_in1, double_matrix_t*
  matrix_in2, double_matrix_t **matrix_out)
{
    size_t sizes[2] = {matrix_in1->dims[0], matrix_in2->dims[0]};

    create_double_matrix_t(matrix_out, sizes, 2);

    _multt(matrix_in1, matrix_in2, (*matrix_out));
}

void multt_no_allocation(double_matrix_t *matrix_in1, double_matrix_t 
   *matrix_in2, double_matrix_t *matrix_out)
{
    assert(matrix_out->dim == 2);

    assert(matrix_out->dims != NULL);
    assert(matrix_out->data != NULL);

    assert(matrix_out->dims[0] == matrix_in1->dims[0]);
    assert(matrix_out->dims[1] == matrix_in2->dims[0]);

    _multt(matrix_in1, matrix_in2, matrix_out);
}
