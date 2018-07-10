#include <random>

#define SIZE    128
#define SCALE   3
#define MAX_NUM 10000

void _kernel_triad(int a[], int b[], int c[], int s) {
  #pragma clang loop unroll(disable)
  for (int i = 0; i < SIZE; i++) {
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

  _kernel_triad(a, b, c, SCALE);

  return 0;
}
