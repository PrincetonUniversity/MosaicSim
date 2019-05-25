/* Copyright 2017 Columbia University, SLD Group */

#ifndef __PVC_MULTT_H__
#define __PVC_MULTT_H__

#include "double_matrix_t.h"

void multt(
  double_matrix_t *matrix_in1,
  double_matrix_t *matrix_in2,
  double_matrix_t **matrix_out);

void multt_no_allocation(
  double_matrix_t *matrix_in1,
  double_matrix_t *matrix_in2,
  double_matrix_t *matrix_out);

#endif // __PVC_MULTT_H__

