#include "../sim.hpp"
#include "../misc/Memory_helpers.hpp"
#include "Tile.hpp"
#include "Accelerator.hpp"
#include <string> 
#include <sstream> 
#include "accelerator_models/c-model/gemm/gemm_model.hpp"
#include "accelerator_models/c-model/nvdla/nvdla.h"
#include "accelerator_models/c-model/sdp/sdp_model.hpp"

void Accelerator::RecieveTransactions(){
  int nb_trans = incoming_trans->get_comp_elems(cycles);
  pair<int, string> *new_tranasctions = incoming_trans->get_comp_buff(cycles);

  for(int i = 0; i < nb_trans; i++)  {
    string t = new_tranasctions[i].second;    
    int id = new_tranasctions[i].first;
    
    if (to_complete.find(id) != to_complete.end()) 
      to_complete[id].second++;
    else
      to_complete.emplace(id, make_pair(t, 1));
  }
}

void Accelerator::fastForward(uint64_t inc) {
  cycles+=inc;  
}

string Accelerator::getNextTransaction() {
  if (to_complete.empty())
    return "";

  auto it = to_complete.begin(); 
  auto elem = *it;
  if(elem.second.second == nb_cores) {
    string t = elem.second.first;
    to_complete.erase(it);
    return t;
  }
  
  return "";
}

bool Accelerator::process() {
  bool busy = true;

  RecieveTransactions();
  
  if(processing)  {
    busy = execute();
  } else {
    string t = getNextTransaction();
    if (t != "" ) { 
      busy = invoke(t);
    } else {
      busy = false;
    }
  }
      
  cycles++;
  return busy || !to_complete.empty(); 
}

bool Accelerator::execute() {
  int arrived = DRAM_responses->get_comp_elems(cycles);
  uint64_t *DRAM_timing = DRAM_responses->get_comp_buff(cycles);
  bool ret;
  
  if(!processing)
    return false;

  if(arrived) {
    final_cycle = max(final_cycle, *DRAM_timing);
    wait_on_DRAM = false;
  }
  
  ret = cycles <= final_cycle || wait_on_DRAM;

  /* finished, update processing */
  if(!ret) 
    processing = false;

  return ret;
}

bool Accelerator::invoke(string t) {
  cout << "acc_arguments: " << t << endl;
  
  //assert(false);
  //create a vector of the args
  vector<string> arg_vec=split(t, ',');
  acc_perf_t perf;
  /*
    1) decadesTF_sdp
    2) decadesTF_matmul
    3) decadesTF_conv2d_layer
    4) decadesTF_dense_layer
  */
  int offset_size=2; //num args in dyn trace before acc args
  uint64_t total_size;
  
  if(arg_vec[1].find("decadesTF_matmul") != std::string::npos) {
    int arg_size=arg_vec.size();
    double memory_factor;
    uint64_t A, B, C;
    assert(arg_size > (3+offset_size));
    config_gemm_t args;
    args.rowsA=stoi(arg_vec[0+offset_size]);
    args.colsA=stoi(arg_vec[1+offset_size]);
    args.colsB=stoi(arg_vec[3+offset_size]);
    args.batch_size=stoi(arg_vec[4+offset_size]);
    args.has_IS_tile=0;
    A = stoll(arg_vec[5+offset_size]);
    B = stoll(arg_vec[6+offset_size]);
    C = stoll(arg_vec[7+offset_size]);

    perf = sim_gemm(sys_config, args);
    total_size = args.rowsA*args.colsA*4 + args.colsA*args.colsB*4 + args.rowsA*args.colsB*4;
    memory_factor = perf.bytes / total_size;
    DRAM_requests->insert(cycles, {A, args.rowsA*args.colsA*4, memory_factor, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {B, args.colsA*args.colsB*4, memory_factor, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {C, args.rowsA*args.colsB*4, memory_factor, cycles+perf.cycles});
  } else if(arg_vec[1].find("decadesTF_sdp") != std::string::npos) {
    int arg_size=arg_vec.size();
    double memory_factor;
    uint64_t A, B, C;
    config_sdp_t args;
    args.working_mode=stoi(arg_vec[0+offset_size]);
    args.size=stoi(arg_vec[1+offset_size]);
    A = stoll(arg_vec[2+offset_size]);
    B = stoll(arg_vec[3+offset_size]);
    C = stoll(arg_vec[4+offset_size]);
    total_size = 2 * args.size*4;
    if (args.working_mode < 4)
      total_size += args.size*4;

    perf=sim_sdp(sys_config, args);
    DRAM_requests->insert(cycles, {A, args.size*4, 1.0, cycles+perf.cycles});
    if (args.working_mode < 4)
      DRAM_requests->insert(cycles, {B, args.size*4, 1.0, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {C, args.size*4, 1.0, cycles+perf.cycles});
    
    cout << "working_mode " << args.working_mode << ", size " << args.size << endl;
  } else if(arg_vec[1].find("decadesTF_bias_add") != std::string::npos) {
    int arg_size=arg_vec.size(), size, batch;
    double memory_factor;
    uint64_t in, bias, out;
    config_sdp_t args;
    args.working_mode=9;
    batch = stoi(arg_vec[offset_size]);
    size = stoi(arg_vec[1+offset_size]);
    args.size= batch * size;
    in = stoll(arg_vec[2+offset_size]);
    bias = stoll(arg_vec[3+offset_size]);
    out = stoll(arg_vec[4+offset_size]);

    perf=sim_sdp(sys_config, args);
    DRAM_requests->insert(cycles, {in, args.size*4, 1.0, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {bias, size*4, size/batch, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {out, args.size*4, 1.0, cycles+perf.cycles});
  } else if(arg_vec[1].find("decadesTF_conv2d_layer") != std::string::npos) {
    int arg_size=arg_vec.size();
    uint64_t inC, outC, in, filter, out;
    assert(arg_size > (14+offset_size));
    config_nvdla_t args;
    
    args.num_of_inputs=stoi(arg_vec[1+offset_size]);
    args.input_height=stoi(arg_vec[2+offset_size]);;
    args.input_width=stoi(arg_vec[3+offset_size]);;
    args.num_of_outputs=stoi(arg_vec[4+offset_size]);
    args.filter_height=stoi(arg_vec[5+offset_size]);;
    args.filter_width=stoi(arg_vec[6+offset_size]);;
    args.zero_pad=stoi(arg_vec[7+offset_size]);;
    args.vertical_conv_dim=stoi(arg_vec[8+offset_size]);;
    args.horizontal_conv_dim=stoi(arg_vec[9+offset_size]);;
    args.pooling=stoi(arg_vec[10+offset_size]);;
    args.pool_height=stoi(arg_vec[11+offset_size]);;
    args.pool_width=stoi(arg_vec[12+offset_size]);;
    args.vertical_pool_dim=stoi(arg_vec[13+offset_size]);;
    args.horizontal_pool_dim=stoi(arg_vec[14+offset_size]);;
    args.type=conv;
    args.batch_size=stoi(arg_vec[0+offset_size]);
    perf=sim_nvdla(sys_config, args);

    in = stoll(arg_vec[15+offset_size]);
    filter = stoll(arg_vec[16+offset_size]);
    out = stoll(arg_vec[17+offset_size]);
    
    DRAM_requests->insert(cycles, {in, args.batch_size*args.num_of_inputs*args.input_height*args.input_width*4, 1.0, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {filter, args.batch_size*args.filter_height*args.filter_width*4, 1.0, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {out, args.batch_size*args.num_of_outputs*args.input_height*args.input_width*4, 1.0, cycles+perf.cycles});
    
    cout << "in_channels: " << args.num_of_inputs << ", in_height " << args.input_height << ", out_channels: " << args.num_of_outputs << ", filter_height " << args.filter_height << ", filter_width " << args.filter_width << ", zero_pad " << args.zero_pad << ", vert_conv_stride " << args.vertical_conv_dim << ", horiz_conv_stride " << args.horizontal_conv_dim << ", pooling " << args.pooling << ", pool_height " << args.pool_height << ", pool_width " << args.pool_width << ", vertical_pool_stride " << args.vertical_pool_dim << ",horizontal_pool_stride: "<< args.horizontal_pool_dim << ", batch " << args.batch_size << endl;
  } else if(arg_vec[1].find("decadesTF_dense_layer") != std::string::npos) {
    //f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << /*0*/ batch << ","<< /*1*/ in_channels << ","<< /*2*/ out_channels << "\n";*/
    int arg_size=arg_vec.size();
    double memory_factor;
    uint64_t  A, B, C;
    assert(arg_size > (3+offset_size));
    config_gemm_t args;
    args.rowsA=1;
    args.colsA=stoi(arg_vec[1+offset_size]);
    args.colsB=stoi(arg_vec[2+offset_size]);
    args.batch_size=stoi(arg_vec[0+offset_size]);
    args.has_IS_tile=0;
    A = stoll(arg_vec[3+offset_size]);
    B = stoll(arg_vec[4+offset_size]);
    C = stoll(arg_vec[5+offset_size]);
    
    perf = sim_gemm(sys_config, args);
    total_size = args.batch_size*args.rowsA*args.colsA*4+args.colsA*args.colsB*4+args.batch_size*args.rowsA*args.colsB*4;
    memory_factor = perf.bytes / total_size;
    cout << "MEMORY FACTOR !!! = " << memory_factor << endl;    
    DRAM_requests->insert(cycles, {A, args.batch_size*args.rowsA*args.colsA*4, memory_factor, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {B, args.colsA*args.colsB*4, memory_factor, cycles+perf.cycles});
    DRAM_requests->insert(cycles, {C, args.batch_size*args.rowsA*args.colsB*4, memory_factor, cycles+perf.cycles});
  } else {
    cout << "no matching acc model for invocation \n";
    return false;
  }
  wait_on_DRAM = true;
  processing=true;
  
  cout << "predicted_cycles: " << perf.cycles << endl;
    
  final_cycle=cycles+perf.cycles;
  cout << "final_cycle from transaction: " << final_cycle << endl;

  update_stats(perf, arg_vec[1], total_size);
  
  /* Update the stats */	       
  return true;
}  

void
Accelerator::update_stats(acc_perf_t &perf, string name, uint64_t total_size) {
  pair<string, uint64_t> accelerator = {name, total_size};
  double call_energy = (perf.power * perf.cycles * 1e-9)/chip_freq;

  dram_accesses += perf.bytes / size_of_cacheline;
  energy += call_energy;

  if (acc_bytes.find(accelerator) == acc_bytes.end()) 
    acc_bytes[accelerator] = perf.bytes; 
  else
    acc_bytes[accelerator] += perf.bytes;
  if (acc_cycles.find(accelerator) == acc_cycles.end()) 
    acc_cycles[accelerator] = perf.cycles; 
  else
    acc_cycles[accelerator] += perf.cycles;
  if (acc_energy.find(accelerator) == acc_energy.end()) 
    acc_energy[accelerator] = call_energy; 
  else
    acc_energy[accelerator] += call_energy;
}
