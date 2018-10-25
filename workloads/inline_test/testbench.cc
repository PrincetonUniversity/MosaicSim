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

// bool helper(int max, int less, int* a) {
//   //vector<int> myvec;
  
//   for (int i=0; i<a[max%ARRAY_SIZE]; i++) {
//     //myvec.push_back(a[i]);
//     max+=a[i];
//   }
  
//   /*
//   for (int i=0; i<myvec.size(); i++) {
//     max+=myvec.at(i);
//     }*/
//   return (max*2)%less<a[(4321*max)%ARRAY_SIZE];
// }

// int square_mod(int entry, int max, int* a) {
//   int less=9; 
//   max--;less--;
//   if (helper(max, a[less%ARRAY_SIZE], a)) {
//     return 5;
//   }
//   return 7;
          
//   //vector<int> myvec;
//   //myvec.push_back(12345);
//   //return (entry*entry*max*less*myvec.size())%max;
  
// }

int helper(int myint, int* a) {
  vector<int> myvec;
  for (int i=1; i<ARRAY_SIZE; i++) {
    a[i]=a[i-1]*2;
    myvec.push_back(a[i]);
  }  
  return myvec.at(a[ARRAY_SIZE-1]);
}

int randfunc(int myint, int* a) {
  for (int i=1; i<ARRAY_SIZE; i++) {
    a[i]=a[i-1];
  }
  helper(myint, a);
  return a[ARRAY_SIZE-1];

}

void _kernel_testbench(int* a) {
  //vector<int> myvec;
#pragma clang loop unroll(disable)  
  for(int i=0; i<10; i++) {
    //myvec.push_back(43211234);
    //a[square_mod(a[i], ARRAY_SIZE,a)] = 2;
    a[i+1]=2*a[i-1];
  }
  randfunc(2, a);
  cout<<a[ARRAY_SIZE-1] << endl;
  //a[myvec.size()%3]=20;  
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
