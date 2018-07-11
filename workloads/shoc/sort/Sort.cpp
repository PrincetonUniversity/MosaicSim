#include <random>

#define NUM_BLOCKS  512
#define BLOCK_SIZE  4
#define BUCKET_SIZE NUM_BLOCKS * BLOCK_SIZE
#define SCAN_SIZE   16
#define RADIX_SIZE  BUCKET_SIZE/SCAN_SIZE

#define EXP         1
#define MASK        0x3
#define ARR_FLAG    false

#define MAX_NUM     255

void _kernel_sort(int arr[][BLOCK_SIZE], int opp[][BLOCK_SIZE], int bucket[][SCAN_SIZE], int sum[]) {
  #pragma clang loop unroll(disable)
  for (int i = 0; i < NUM_BLOCKS; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      int block = (arr[i][j] >> EXP) & MASK;
      int index = block * NUM_BLOCKS + i + 1;
      int radix = index / SCAN_SIZE;
      int scan  = index - (radix * SCAN_SIZE);
      bucket[radix][scan]++;
    }
  }

  #pragma clang loop unroll(disable)
  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 1; j < SCAN_SIZE; j++) {
      bucket[i][j] = bucket[i][j] + bucket[i][j-1];
    }
  }

  #pragma clang loop unroll(disable)
  for (int i = 1; i < RADIX_SIZE; i++) {
    sum[i] = sum[i-1] + bucket[i-1][SCAN_SIZE-1];
  }

  #pragma clang loop unroll(disable)
  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      bucket[i][j] = bucket[i][j] + sum[i];
    }
  }

  #pragma clang loop unroll(disable)
  for (int i = 0; i < NUM_BLOCKS; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      int value           = arr[i][j];
      int block           = (value >> EXP) & MASK;
      int index           = block * NUM_BLOCKS + i;
      int radix           = index / SCAN_SIZE;
      int scan            = index - (radix * SCAN_SIZE);
      int bkt             = bucket[radix][scan];
      int k               = bkt / BLOCK_SIZE;
      int l               = bkt - (k * BLOCK_SIZE);
      opp[k][l]           = value;
      bucket[radix][scan] = bkt + 1;
    }
  }
}

int main(int argc, char* argv[]) {
  int a[NUM_BLOCKS][BLOCK_SIZE];
  int b[NUM_BLOCKS][BLOCK_SIZE];
  int bucket[RADIX_SIZE][SCAN_SIZE];
  int sum[RADIX_SIZE];

  std::random_device rd;
  std::mt19937 seed(rd());
  std::uniform_int_distribution<> gen(0, MAX_NUM);

  for (int i = 0; i < NUM_BLOCKS; i++) {
    for (int j = 0; j < BLOCK_SIZE; j++) {
      a[i][j] = gen(seed);
      b[i][j] = 0;
    }
  }

  for (int i = 0; i < RADIX_SIZE; i++) {
    for (int j = 0; j < SCAN_SIZE; j++) {
      bucket[i][j] = 0;
    }
    sum[i] = 0;
  }

  if (ARR_FLAG) {
    _kernel_sort(b, a, bucket, sum);
  } else {
    _kernel_sort(a, b, bucket, sum);
  }

  return 0;
}
