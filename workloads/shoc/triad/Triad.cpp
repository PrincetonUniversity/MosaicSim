#include <random>
#include <iostream>
#define SIZE    2048
#define SCALE   3
#define MAX_NUM 10000
#include "DECADES/DECADES.h"

__attribute__((noinline))
void _kernel_(int a[], int b[], int c[], int s, int tile_id, int num_tiles) {
  //#pragma clang loop unroll(disable)
  for (int i = tile_id; i < SIZE; i+=num_tiles) {
    c[i] = a[i] + s * b[i];
  }
}

int main(int argc, char* argv[]) {
  int a[SIZE];
  int b[SIZE];
  int c[SIZE];

  std::random_device rd;
  std::mt19937 seed(rd());
  std::uniform_int_distribution<> gen(0, MAX_NUM);

  for (int i = 0; i < SIZE; i++) {
    a[i] = gen(seed);
    b[i] = gen(seed);
    c[i] = 0;
  }

  _kernel_(a, b, c, SCALE, 0, 1);
  std::cout << "finished with no errors \n";
  return 0;
}
