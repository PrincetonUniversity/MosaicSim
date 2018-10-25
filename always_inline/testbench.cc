#include <iostream>
#include <stdlib.h>
#include <queue>
#include <vector>
using namespace std;

#define ARRAY_SIZE 100
// void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
// {
//   #pragma clang loop unroll(disable)
//   for(int i=0; i<ARRAY_SIZE; i++) {
//       a[i] = b[i]+ c[i];
//   } 
// }
// std::queue<int> comm_buffer;

// void __attribute__ ((noinline)) send(int val) {
//   comm_buffer.push(val);
// }

// int __attribute__ ((noinline)) recv() {
//   int val = comm_buffer.front();
//   comm_buffer.pop();
//   return val;
// }
// void _kernel_testbench(int*  __restrict__ a, int* __restrict__ b, int * __restrict__ c)
// {
//   #pragma clang loop unroll(disable)
//   for(int i=0; i<ARRAY_SIZE; i++) {
//       send(b[i]);
//       send(c[i]);
//   }
//   for(int i=0; i<ARRAY_SIZE; i++) {
//       int val1 = recv();
//       int val2 = recv();
//       a[i] = val1 + val2;
//   } 
// }


/*int square_mod(int entry, int max) {
  int less=9; 
  max--;less--;
  vector<int> myvec;
  myvec.push_back(12345);
  return (entry*entry*max*less*myvec.size())%max;
}
*/

void _kernel_testbench(int* a) {
  vector<int> myvec;
#pragma clang loop unroll(disable)  
  for(int i=0; i<10; i++) {
    myvec.push_back(4321234);
    //a[square_mod(a[i], ARRAY_SIZE)] = 2;
  }
  a[myvec.size()%3]=20;  
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
  _kernel_testbench(a);
}
