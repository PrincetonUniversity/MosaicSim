#include <iostream>
#include <stdlib.h>

#define SIZE 8000
#define inliner __attribute__((always_inline))
using namespace std;
//#define MODE2
#define MODE3
#ifdef CTRLSPEC_VARIANT
void _kernel_test(int (&arr)[SIZE]) {
  int loc = 0;
  #pragma clang loop unroll(disable)
  while(loc < SIZE) {
    if(arr[loc] == 1) {
      arr[loc] = 2;
      loc = loc + 2;
    }
    else {
      loc = loc + 1;
    }
  }
}
#endif
#ifdef CTRLSPEC
void _kernel_test(int (&arr)[SIZE]) {
  #pragma clang loop unroll(disable)
  for(int i=0; i<SIZE; i++) {
    if(arr[i] == 1)
      arr[i] = 2;
  }
}
#endif

#ifdef MEMSPEC
void _kernel_test(int (&arr)[SIZE]) {
  #pragma clang loop unroll(disable)
  for(int i=0; i<SIZE; i++) {
    arr[arr[i]] = 2;
  }
}
#endif
int main()
{
  int arr[SIZE];
  srand(5);
  for(int i=0; i<SIZE; i++)
  {
    arr[i] = rand() % 2;
  }
  _kernel_test(arr);
  for(int i=0; i<SIZE; i++)
  {
    cout << arr[i] << endl;
  }
  return 0;
}