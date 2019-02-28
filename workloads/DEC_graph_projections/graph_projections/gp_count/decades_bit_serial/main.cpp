#include "DECADES/BitSerial/BitSerial.h"
#include "../../common/common.h"
#include <fstream>
#include <iostream>

using namespace std;
ofstream outfile;
vector<int> index_vec;
uint64_t load_count=0;
typedef uint weight_type;


void preprocess(bgraph g, weight_type *output) { }

void dec_bs_batch (weight_type* output, ulong k) {
  decades_bs_incr_uint(output,k);
  index_vec.push_back(k);
  load_count++;
}

void dec_bs_prefix(bgraph& g) {
  outfile.open("../bs_core/bitserial.cpp");
  string outstring=string("#include <stdint.h>\n #include <cstdlib>\n");
    outstring+=string("using namespace std;\n");
  string allocate_array=string("uint* weight_matrix=(uint*) calloc(") + to_string(get_projection_size(g))+string(",") + to_string(sizeof(uint)) + "); \n";  
  outstring+=allocate_array;
  outstring+=string("uint c[1]={1}; \n \
__attribute__((noinline)) void dec_invisible_load(int x) { \n \
c[0]=x; \n \
} \n \
__attribute__((noinline)) void dec_bs_wait() {c[0]=0;} \n \
__attribute__((noinline)) void dec_bs_supply() {c[0]=0;} \n \
__attribute__((noinline)) void dec_bs_vector_inc(int x) {c[0]=x;} \n \
__attribute__((noinline)) void _kernel_() { \n \
for(int i=0; i<c[0]; i++) { \n ");
  outfile << outstring;
}

void dec_bs_postfix() {
  outfile << "} \n \
} \n \
int main() { \n      \
_kernel_(); \n     \
return 1;\n } \n";
}

__attribute__((noinline))
void dec_call_bs(vector<int>& index_vec) {
  

  string bs_string="";  
  for(int j=0; j<index_vec.size(); j++) {
    if(j==0) {
      bs_string+="dec_bs_wait(); \n";
    }
    bs_string+=string("dec_invisible_load(") + string("weight_matrix[") +to_string(index_vec[j])+string("]);\n");
    if(j==index_vec.size()-1) {
      bs_string+=string("dec_bs_vector_inc(") + to_string(index_vec.size())+string("); \n");
      bs_string+=string("dec_bs_supply(); \n");
    } 
  }
  outfile << bs_string;
  index_vec.clear();
  
  decades_bs_flush(); //we'll treat this is core_consume()
}

__attribute__((noinline))
void _kernel_(bgraph g, weight_type *output) {
  dec_bs_prefix(g);
  
  int total = 0;
  for (unsigned int i = 0; i < g.x_nodes; i++) {   
    for (unsigned int e1 = g.node_array[i]; e1 < g.node_array[i+1] - 1; e1++) {
      int y_node_1 = g.edge_array[e1];
      ulong cached_second = i_j_to_k_get_second(g.y_nodes, y_node_1);
      for (unsigned int e2 = e1+1; e2 < g.node_array[i+1]; e2++) {
        //total++;
        
        int y_node_2 = g.edge_array[e2];
#ifdef KERNEL_ASSERTS
        assert(y_node_1 < y_node_2);
#endif
        

	ulong k = i_j_to_k_cached(y_node_1, y_node_2, g.y_nodes, g.first_val, cached_second);

#ifdef ASSERTS
	assert(k >= 0 && k < get_projection_size_nodes(g.y_nodes));
#endif
        //output[k] += 1;
        dec_bs_batch(output,k);
      }
    }
    dec_call_bs(index_vec);
    
  }
  dec_bs_postfix();
  //printf("total examined %d edges\n", total);
  
  return;
}

#include "../../common/main_decades.h"
