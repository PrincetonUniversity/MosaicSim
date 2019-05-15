/* Copyright 2018 Columbia University, SLD Group */

#ifndef __PVC_DOUBLE_MATRIX_T_H__
#define __PVC_DOUBLE_MATRIX_T_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct
{
    double *data;

    size_t *dims;

    size_t dim;

} double_matrix_t;

__attribute__((unused))
static void create_double_matrix_t(double_matrix_t
  **matrix, size_t *dims, size_t dim)
{
    unsigned i, size = 1;

    for (i = 0; i < dim; ++i)
        size *= dims[i];

    (*matrix) = (double_matrix_t*)
      malloc(sizeof(double_matrix_t));

    (*matrix)->data = (double*)
       malloc(sizeof(double) * size);

    (*matrix)->dims = (size_t*)
       malloc(sizeof(size_t) * dim);

    (*matrix)->dim = dim;
    for (i = 0; i < dim; ++i)
       (*matrix)->dims[i] = dims[i];
}

__attribute__((unused))
static void reshape_double_matrix_t(double_matrix_t
  **matrix, size_t *dims, size_t dim)
{
    unsigned i, old_size = 1, new_size = 1;

    for (i = 0; i < (*matrix)->dim; ++i)
        old_size *= (*matrix)->dims[i];

    for (i = 0; i < dim; ++i)
        new_size *= dims[i];

    if (old_size != new_size)
    {
        (*matrix)->data = (double*) realloc(
          (*matrix)->data, sizeof(double) * new_size);
    }

    if ((*matrix)->dim != dim)
    {
        (*matrix)->dims = (size_t*) realloc(
          (*matrix)->dims, sizeof(size_t) * dim);
    }

    (*matrix)->dim = dim;

    for (i = 0; i < dim; ++i)
        (*matrix)->dims[i] = dims[i];
}

__attribute__((unused))
static void print_double_matrix_t(double_matrix_t *matrix)
{
    unsigned int i, size = 1;

    fprintf(stderr, "%zu ", matrix->dim);

    for (i = 0; i < matrix->dim; ++i)
    {
        fprintf(stderr, "%zu ", matrix->dims[i]);

        size *= matrix->dims[i];
    }

    fprintf(stderr, "\n");

    for (i = 0; i < size; ++i)
       fprintf(stderr, "%lf ", matrix->data[i]);

    fprintf(stderr, "\n");
}

__attribute__((unused))
static int store_double_matrix_t(double_matrix_t *matrix, const char *file)
{
    unsigned int i, size = 1;

    FILE *fp = fopen(file, "w");

    if (!fp) { return -1; }

    fprintf(fp, "%zu ", matrix->dim);

    for (i = 0; i < matrix->dim; ++i)
    {
        fprintf(fp, "%zu ", matrix->dims[i]);

        size *= matrix->dims[i];
    }

    fprintf(fp, "\n");

    for (i = 0; i < size; ++i)
        fprintf(fp, "%lf ", matrix->data[i]);

    fprintf(fp, "\n");

    fclose(fp);

    return 0;
}

__attribute__((unused))
static int load_double_matrix_t(double_matrix_t **matrix, const char *file)
{
    unsigned int i, size = 1;

    FILE *fp = fopen(file, "r");

    if (!fp) { return -1; }

    *matrix = (double_matrix_t*)
      malloc(sizeof(double_matrix_t));

    fscanf(fp, "%zu\n", &((*matrix)->dim));

    (*matrix)->dims = (size_t*) malloc(
      sizeof(size_t) * (*matrix)->dim);

    for (i = 0; i < (*matrix)->dim; ++i)
    {
        fscanf(fp, "%zu\n", &((*matrix)->dims[i]));

        size *= (*matrix)->dims[i];
    }

    (*matrix)->data = (double*)
      malloc(sizeof(double) * size);

    for (i = 0; i < size; ++i)
        fscanf(fp, "%lf", &((*matrix)->data[i]));

    fclose(fp);

    return 0;
}

__attribute__((unused))
static void free_double_matrix_t(double_matrix_t **matrix)
{
    free((*matrix)->data);
    free((*matrix)->dims);
    free(*matrix);
}

#endif // __PVC_DOUBLE_MATRIX_T_H__

