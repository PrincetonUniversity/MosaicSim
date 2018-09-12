#include <iostream>
#include <stdlib.h>
#include <queue>

#define ARRAY_SIZE 500
// void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
// {
//   #pragma clang loop unroll(disable)
//   for(int i=0; i<ARRAY_SIZE; i++) {
//       a[i] = b[i]+ c[i];
//   } 
// }
std::queue<int> comm_buffer;

void __attribute__ ((noinline)) send(int val) {
  comm_buffer.push(val);
}

int __attribute__ ((noinline)) recv() {
  int val = comm_buffer.front();
  comm_buffer.pop();
  return val;
}
void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
{
  #pragma clang loop unroll(disable)
  for(int i=0; i<ARRAY_SIZE; i++) {
      send(b[i]);
      send(c[i]);
  }
  for(int i=0; i<ARRAY_SIZE; i++) {
      int val1 = recv();
      int val2 = recv();
      a[i] = val1 + val2;
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
