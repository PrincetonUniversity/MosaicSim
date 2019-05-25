#include "../sim.h"
#include "Tile.h"
#include "Accelerator.h"
#include <sstream> 
#include "accelerator_models/c-model/gemm/gemm_model.hpp"

vector<string> Accelerator::split(const string &s, char delim) {
  stringstream ss(s);
  string item;
  vector<string> tokens;
  while (getline(ss, item, delim)) {
    tokens.push_back(item);
  }
  return tokens;
}

bool Accelerator::process() {
  if(transaction_pending && cycles==final_cycle) {
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
    currentTransaction=t;
    //create a vector of the args for perf model
    vector<string> arg_vec=split(currentTransaction->d->acc_args, ',');
    //rowsa, colsa, colsb
    //offset+ 0, 1, 4 ; offset =2; 2, 3, 6
    //int rowsA, int colsA , int depA, int rowsB, int colsB, int depB
    config_gemm_t args;
    args.rowsA=stoi(arg_vec[2]);
    args.colsA=stoi(arg_vec[3]);
    args.colsB=stoi(arg_vec[6]);
    args.batch_size=1;
    args.has_IS_tile=0;
    transaction_pending=true;

    //get number of cycles for work, based on timing model for opcode
    if(arg_vec[1].find("decadesTF_matmul") != std::string::npos) {
      currentTransaction->perf=sim_gemm(args);
    }
    else {
      cout << "no matching acc model for invocation \n";
      assert(false);
    }
    //currentTransaction->cycle_count=num_cycles;
    final_cycle=cycles+currentTransaction->perf.cycles;
    return true;
  }
  return false;
}  

