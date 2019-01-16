#include <random>

#define ROW_SIZE   32
#define BLOCK_SIZE 8
#define NUM_BLOCKS (ROW_SIZE * ROW_SIZE)/(BLOCK_SIZE * BLOCK_SIZE)

#define MAX_NUM    128

__attribute__((noinline)) void _kernel_gemm(int x[][BLOCK_SIZE], int y[][BLOCK_SIZE], int z[][BLOCK_SIZE]) {
  #pragma clang loop unroll(disable)
  for (int i = 0; i < ROW_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      for (int k = 0; k < BLOCK_SIZE; k++) {
        z[i][j] = z[i][j] + x[i][j] * y[j][k];
      }
    }
  }
}

int main(int argc, char* argv[]) {
  int x[ROW_SIZE][BLOCK_SIZE];
  int y[BLOCK_SIZE][BLOCK_SIZE];
  int z[ROW_SIZE][BLOCK_SIZE];

  std::random_device rd;
  std::mt19937 seed(rd());
  std::uniform_int_distribution<> gen(0, MAX_NUM);

  for (int i = 0; i < ROW_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      x[i][j] = gen(seed);
      z[i][j] = 0;
    }
  }

  for (int i = 0; i < BLOCK_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      y[i][j] = gen(seed);
    }
  }

  _kernel_gemm(x, y, z);

  return 0;
}
