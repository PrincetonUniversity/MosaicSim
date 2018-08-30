#include <iostream>
#include <stdlib.h>

#define ARRAY_SIZE 500
void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
{
  #pragma clang loop unroll(disable)
  for(int i=0; i<ARRAY_SIZE; i++) {
      a[i] = b[i]+ c[i];
  } 
}
int main()
{
  srand(7);

  int a[ARRAY_SIZE], b[ARRAY_SIZE], c[ARRAY_SIZE];
  for(int i=0; i<ARRAY_SIZE; i++)
  {
    a[i] = rand() % 100;
    b[i] = rand() % 100;
    c[i] = rand() % 100;
  }
  _kernel_testbench(a,b,c);
}
