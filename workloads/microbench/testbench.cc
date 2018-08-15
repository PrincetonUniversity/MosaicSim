#include <iostream>
#include <stdlib.h>

#define SIZE 2000
#define inliner __attribute__((always_inline))
using namespace std;
void _kernel_test(int (&arr)[SIZE]) {
  int loc = 0;
  int dummy  = 0;
  while(loc < SIZE) {
    /*if(arr[loc] == 1) {
      arr[loc] = 2;
      loc = loc + 2;
    }
    else {
      loc = loc + 1;
    }*/
    if(arr[loc] == 1)
      arr[loc] = 2;
    loc++;
  }
}

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