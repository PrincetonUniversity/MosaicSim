#include <vector>
#include <utility>

class compressed_sparse {
public:
  uint64_t nVertices;
  uint64_t nEdges;
  compressed_sparse(size_t d0, size_t d1 = 0) : 
    nVertices(d0), 
    nEdges(d1)/*, 
                ptrs(nVertices + 1)*/ {}

  void push_back(size_t i, size_t j, double weight) {
    ++ptrs[i];
    indices.push_back(std::make_pair(j, weight));
}

  void accumulate() {
    for (uint64_t i = 0; i < nVertices; i++) {
      ptrs[i + 1] += ptrs[i]; 
    }
    for (uint64_t i = nVertices; i > 0; --i) {
      ptrs[i] = ptrs[i - 1];
    }
    ptrs[0] = 0;
  }

  // private:
  std::vector<size_t> ptrs;
  std::vector<std::pair<uint64_t, double>> indices;
};
