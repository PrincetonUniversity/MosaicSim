#include "stdio.h"
#include "../decouple.h"

void desc_kernel_add(int *a, int *b, int *c) {

  //load A and B  
  desc_produce_i32(a); //int reg_a = *a;
  desc_produce_i32(b); //int reg_b = *b;

  //Do the addition
  //int val = reg_a + reg_b;

  //Store the result
  desc_store_addr_i32(c); //*c = val;
}


int main() {
  desc_init(SUPPLY);
  int a = 5, b = 6, c = 0;
  
  desc_kernel_add(&a, &b, &c);
  
  printf("got the value %d\n", c);
  
  return 0;
}
