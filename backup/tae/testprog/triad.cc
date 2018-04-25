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
void _kernel_triad(int*  __restrict__ a, int* __restrict__ b, int *c)
{
  for(int i=0; i<ARRAY_SIZE; i++) {
    if(a[i] > ARRAY_SIZE / 2) {
      c[i] = b[i] +1;
    }
    else
      c[i] = b[i] -1;
    switch(a[i])
    {
      case 0:
        c[i] = b[i] *2;
      case 1: 
        c[i] = b[i] *3;
      default:
        c[i] = b[i] *4;
    }
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
    //std::cout << i << " / " << a[i] << " / " << b[i] << " / " << c[i] << "\n";
  }
  _kernel_triad(a,b,c);
  //for (int i=0; i<ARRAY_SIZE; i++)
  //  std::cout << a[i] << " / " << b[i] << " / " << c[i] << "\n";
}
