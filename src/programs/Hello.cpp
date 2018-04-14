#include <iostream>
#include <vector>

int max(std::vector<int> lst) {
  int max = std::numeric_limits<int>::min();
  for (auto& elem : lst) {
    if (elem > max) {
      max = elem;
    }
  }
  return max;
}

int main() {
  int arr[] = {1, -10, 3, 29, 42, -69, 289, -371, 4, 8, 240};
  std::vector<int> lst (std::begin(arr), std::end(arr));
  int hi = max(lst);
  std::cout << hi << "\n";
  return 0;
}