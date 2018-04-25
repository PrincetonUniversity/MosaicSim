#include <iostream>
#include <stdlib.h>

#define ARRAY_SIZE 100
// void _kernel_triad(int a[], int b[], int c[])
// {
//   #pragma clang loop vectorize_width(64) interleave_count(1) unroll_count(1)
//   for(int i=0; i<ARRAY_SIZE; i++)
//   {
//     c[i] = a[i] * 3 + b[i];
//   }
// }
/*void printInt(int cond)
{
  std::cout << "Print : " << cond << "\n";
}*/
void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
{
  #pragma clang loop unroll(disable)
  for(int i=0; i<ARRAY_SIZE; i++) {
      c[i] = a[i] * 3 + b[i];
      b[i] = c[i-1] + 2;
  }
}
int main()
{
  srand(7);

  int a[ARRAY_SIZE], b[ARRAY_SIZE], c[ARRAY_SIZE];
  for(int i=0; i<ARRAY_SIZE; i++)
  {
  	a[i] = rand() % ARRAY_SIZE;
  	b[i] = rand() % ARRAY_SIZE;
  	c[i] = rand() % ARRAY_SIZE;
  }
  _kernel_testbench(a,b,c);
}
