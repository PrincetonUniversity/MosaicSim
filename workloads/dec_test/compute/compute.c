#include "stdio.h"
#include "../decouple.h"

void desc_kernel_add(int *a, int *b, int *c) {
  for(int i=0; i<100; i++) {
    //load A and B
    int reg_a = desc_consume_i32(); //int reg_a = *a;
    int reg_b = desc_consume_i32(); //int reg_b = *b;
    
    //Do the addition
    int val = reg_a + reg_b;
    
    //Store the result
    desc_store_val_i32(val); //*c = val;
  }
}



int main() {

  int err = desc_init(COMPUTE);
  int a[100], b[100], c[100];

  for(int i=0; i<100; i++) {
    a[i] = i;
    b[i] = i*2;
    c[i] = 0;
  }
  
  desc_kernel_add(a, b, c);
  
  printf("got the value %d\n", c[99]);
  
  return 0;
}
