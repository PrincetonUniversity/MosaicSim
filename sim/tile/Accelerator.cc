#include "../sim.h"
#include "Tile.h"
#include "Accelerator.h"
#include <sstream> 
#include "accelerator_models/c-model/gemm/gemm_model.hpp"
#include "accelerator_models/c-model/nvdla/nvdla.h"
#include "accelerator_models/c-model/sdp/sdp_model.hpp"

vector<string> Accelerator::split(const string &s, char delim) {
  stringstream ss(s);
  string item;
  vector<string> tokens;
  while (getline(ss, item, delim)) {
    tokens.push_back(item);
  }
  return tokens;
}

void Accelerator::fastForward(uint64_t inc) {
  cycles+=inc;  
}

bool Accelerator::process() {
  if(transaction_pending && cycles>=final_cycle) {
  
    transaction_pending=false;
    currentTransaction->dst_id=currentTransaction->src_id;
    currentTransaction->src_id=id;
    sim->InsertTransaction(currentTransaction, final_cycle);    
  }

  cycles++;
  //always return false, since it's dependent on core invocation
  return false; 
}

bool Accelerator::ReceiveTransaction(Transaction* t) {
  
  
  if(transaction_pending) {
    return false;
  }
  else {
    //cout << "Tech: " << sys_config.tech << "nm, BW: " << sys_config.mem_bandwidth <<"Bytes/cycle, Latency: " << sys_config.dram_latency << "cycles, # Acc Tiles: " << sys_config.n_acc_tiles << ", # IS Tiles: " << sys_config.n_IS_tiles << endl;
    currentTransaction=t;
    cout << "acc_arguments: " << t->d->acc_args << endl;
    
    //assert(false);
    //create a vector of the args for perf model
    vector<string> arg_vec=split(currentTransaction->d->acc_args, ',');
    //rowsa, colsa, colsb
    //offset+ 0, 1, 4 ; offset =2; 2, 3, 6
    //int rowsA, int colsA , int depA, int rowsB, int colsB, int depB
    
    transaction_pending=true;

    /*
      1) decadesTF_sdp
      2) decadesTF_matmul
      3) decadesTF_conv2d_layer
      4) decadesTF_dense_layer
     */
    int offset_size=2; //num args in dyn trace before acc args
    
    if(arg_vec[1].find("decadesTF_matmul") != std::string::npos) {
      //cout << "acc received matmul \n";
      int arg_size=arg_vec.size();
      assert(arg_size > (3+offset_size));
      config_gemm_t args;
      args.rowsA=stoi(arg_vec[0+offset_size]);
      args.colsA=stoi(arg_vec[1+offset_size]);
      args.colsB=stoi(arg_vec[3+offset_size]);
      args.batch_size=stoi(arg_vec[4+offset_size]);
      args.has_IS_tile=1;
      currentTransaction->perf=sim_gemm(sys_config, args);
      cout << "rows A " << args.rowsA << ", col A " << args.colsA << ", cols b " << args.colsB << ", batch size " << args.batch_size << endl;
      //assert(false);
    }
    else if(arg_vec[1].find("decadesTF_sdp") != std::string::npos) {
      int arg_size=arg_vec.size();
      assert(arg_size > (1+offset_size));
      config_sdp_t args;
      args.working_mode=stoi(arg_vec[0+offset_size]);
      args.size=stoi(arg_vec[1+offset_size]);
      currentTransaction->perf=sim_sdp(sys_config, args);
      cout << "working_mode " << args.working_mode << ", size " << args.size << endl;
      //assert(false);
    }
    else if(arg_vec[1].find("decadesTF_conv2d_layer") != std::string::npos) {
      int arg_size=arg_vec.size();
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
      currentTransaction->perf=sim_nvdla(sys_config, args);
      cout << "in_channels: " << args.num_of_inputs << ", in_height " << args.input_height << ", out_channels: " << args.num_of_outputs << ", filter_height " << args.filter_height << ", filter_width " << args.filter_width << ", zero_pad " << args.zero_pad << ", vert_conv_stride " << args.vertical_conv_dim << ", horiz_conv_stride " << args.horizontal_conv_dim << ", pooling " << args.pooling << ", pool_height " << args.pool_height << ", pool_width " << args.pool_width << ", vertical_pool_stride " << args.vertical_pool_dim << ",horizontal_pool_stride: "<< args.horizontal_pool_dim << ", batch " << args.batch_size << endl;
      //assert(false);
    }
    else if(arg_vec[1].find("decadesTF_dense_layer") != std::string::npos) {
      //f3[omp_get_thread_num()] << acc_kernel_name << "," << node_id << "," << /*0*/ batch << ","<< /*1*/ in_channels << ","<< /*2*/ out_channels << "\n";*/
      int arg_size=arg_vec.size();
      assert(arg_size > (2+offset_size));
      config_nvdla_t args;
      args.num_of_inputs=stoi(arg_vec[1+offset_size]);
      args.input_height=1;
      args.input_width=1;
      args.num_of_outputs=stoi(arg_vec[2+offset_size]);
      args.filter_height=1;
      args.filter_width=1;
      args.zero_pad=0;
      args.vertical_conv_dim=1;
      args.horizontal_conv_dim=1;
      args.pooling=0;
      args.pool_height=1;
      args.pool_width=1;
      args.vertical_pool_dim=1;
      args.horizontal_pool_dim=1;
      args.type=fc;
      args.batch_size=stoi(arg_vec[0+offset_size]);
      currentTransaction->perf=sim_nvdla(sys_config, args);
      cout << "in_channels: " << args.num_of_inputs << ", out_channels: " << args.num_of_outputs << ", batch: " << args.batch_size << endl;
      //assert(false);
    }
    else {
      cout << "no matching acc model for invocation \n";
      assert(false);
    }
    
    cout << "predicted_cycles: " << currentTransaction->perf.cycles << endl;
    
    final_cycle=cycles+currentTransaction->perf.cycles;
    cout << "final_cycle: " << currentTransaction->perf.cycles + cycles << endl;
    sim->fastForward(id,currentTransaction->perf.cycles);
    //cout << "bytes " << currentTransaction->perf.bytes << endl;
    //assert(false);
    return true;
  }
  return false;
}  

