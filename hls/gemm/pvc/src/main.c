/* Copyright 2017 Columbia University, SLD Group */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multt.h"

int main(int argc, char *argv[])
{
    clock_t begin, end; 

    double_matrix_t *matrix_in1 = NULL;
    double_matrix_t *matrix_in2 = NULL;
    double_matrix_t *matrix_out = NULL;

    if (argc != 4)
    {
        fprintf(stderr, "Error: %s <input1> <input2> <output>\n", argv[0]);
        return 1;
    }

    printf("Info: input matrix 1 file %s\n", argv[1]);

    if (load_double_matrix_t(&matrix_in1, argv[1]) < 0)
    {
        fprintf(stderr, "Error: input matrix 1 not valid\n");
        return 1;
    }

    printf("Info: input matrix 2 file %s\n", argv[2]);

    if (load_double_matrix_t(&matrix_in2, argv[2]) < 0)
    {
        fprintf(stderr, "Error: input matrix 2 not valid\n");
        return 1;
    }

    begin = clock(); // start the execution timer

    multt(matrix_in1, matrix_in2, &matrix_out);

    end = clock(); // stop the execution timer

    printf("Info: output matrix file %s\n", argv[3]);

    if (store_double_matrix_t(matrix_out, argv[3]) < 0)
    {
        fprintf(stderr, "Error: output matrix not valid\n");
        return 1;
    }

    fprintf(stderr, "Info: execution time: %.2lf s\n", 
      ((double) (end - begin)) / CLOCKS_PER_SEC);

    free_double_matrix_t(&matrix_in1);
    free_double_matrix_t(&matrix_in2);

    free_double_matrix_t(&matrix_out);

    return 0;
}
