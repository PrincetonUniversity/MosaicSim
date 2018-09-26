#include "stdio.h"
#include "decouple.h"

void desc_kernel_add(int *a, int *b, int *c) {

  //load A and B
  int reg_a = desc_consume_i32(); //int reg_a = *a;
  int reg_b = desc_consume_i32(); //int reg_b = *b;
  
  //Do the addition
  int val = reg_a + reg_b;
  
  //Store the result
  desc_store_val_i32(val); //*c = val;
  
}



int main() {

  int err = desc_init(COMPUTE);
  int a = 5, b = 6, c = 0;
  
  desc_kernel_add(&a, &b, &c);
  
  printf("got the value %d\n", c);
  
  return 0;
}
