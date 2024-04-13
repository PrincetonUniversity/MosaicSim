#include "common.h"

#include "DECADES.h"

typedef uint weight_type;

void preprocess(bgraph g, weight_type *output) { }

__attribute__((noinline))
void _kernel_(bgraph g, weight_type *output, int tid, int num_threads) {
  int total = 0;
  for (unsigned int i = 0; i < g.x_nodes; i++) {   
    for (unsigned int e1 = g.node_array[i] + tid; e1 < g.node_array[i+1]; e1+=num_threads) {
      int y_node_1 = g.edge_array[e1];
      ulong cached_second = i_j_to_k_get_second(g.y_nodes, y_node_1);
      for (unsigned int e2 = e1+1; e2 < g.node_array[i+1]; e2++) {
        int y_node_2 = g.edge_array[e2];

#ifdef KERNEL_ASSERTS
        assert(y_node_1 < y_node_2);
#endif
        
	ulong k = i_j_to_k_cached(y_node_1, y_node_2, g.y_nodes, g.first_val, cached_second);

#ifdef KERNEL_ASSERTS
	assert(k >= 0 && k < get_projection_size_nodes(g.y_nodes));
#endif

        output[k] += 1;
      }
    }
    DECADES_BARRIER();
  }
  
  return;
}

#include "../common/main_decades.h"
