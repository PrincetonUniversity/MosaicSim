#include <random>
#include <limits>

#define IMG_WIDTH     32
#define IMG_HEIGHT    32
#define FILTER_WIDTH  3
#define FILTER_HEIGHT 3

void _kernel_stencil(int* __restrict__ orig, int* __restrict__ sol, const int* __restrict__ filt) {
  #pragma clang loop unroll(disable)
  for (int i = 0; i < IMG_WIDTH-FILTER_WIDTH+1; i++) {
    for (int j = 0; j < IMG_HEIGHT-FILTER_HEIGHT+1; j++) {
      int conv = 0;

      for (int k = 0; k < FILTER_WIDTH; k++) {
        for (int l = 0; l < FILTER_HEIGHT; l++) {
          int mult = filt[k][l] * orig[i+k][j+l];
          conv = conv + mult;
        }
      }

      sol[i][j] = conv;
    }
  }
}

int main(int argc, char* argv[]) {
  int originalImage[IMG_WIDTH][IMG_HEIGHT];
  int solution[IMG_WIDTH][IMG_HEIGHT];
  int filter[3][3];

  std::random_device rd;
  std::mt19937 seed(rd());
  std::uniform_int_distribution<> gen(0, std::numeric_limits<int>::max());

  for (int i = 0; i < IMG_WIDTH; i++) {
    for (int j = 0; j < IMG_HEIGHT; j++) {
      originalImage[i][j] = gen(seed);
      solution[i][j] = 0;
    }
  }

  for (int i = 0; i < FILTER_WIDTH; i++) {
    for (int j = 0; j < FILTER_HEIGHT; j++) {
      filter[i][j] = gen(seed);
    }
  }

  __kernel_stencil(originalImage, solution, filter);

  return 0;
}
