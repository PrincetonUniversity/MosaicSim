#include <random>

#define BUCKET_SIZE 2048
#define BLOCK_SIZE  16
#define RADIX_SIZE  BUCKET_SIZE/BLOCK_SIZE
#define MAX_NUM     1024

__attribute__((noinline)) void _kernel_scan(int bucketA[][BLOCK_SIZE], int bucketB[][BLOCK_SIZE], int sum[]) {
  #pragma clang loop unroll(disable)
  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 1; j < BLOCK_SIZE; j++) {
      bucketA[i][j] = bucketA[i][j-1];
    }
  }

  #pragma clang loop unroll(disable)
  for (int i = 1; i < RADIX_SIZE; i++) {
    sum[i] = sum[i-1] + bucketA[i-1][BLOCK_SIZE-1];
  }

  #pragma clang loop unroll(disable)
  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      bucketB[i][j] = bucketA[i][j] + sum[i];
    }
  }
}

int main(int argc, char* argv[]) {
  int bucketA[RADIX_SIZE][BLOCK_SIZE];
  int bucketB[RADIX_SIZE][BLOCK_SIZE];
  int sum[RADIX_SIZE];

  std::random_device rd;
  std::mt19937 seed(rd());
  std::uniform_int_distribution<> gen(0, MAX_NUM);

  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      bucketA[i][j] = gen(seed);
      bucketB[i][j] = gen(seed);
    }
    sum[i] = 0;
  }

  _kernel_scan(bucketA, bucketB, sum);

  return 0;
}
