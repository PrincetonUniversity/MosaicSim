#ifndef ACCELERATOR_H
#define ACCELERATOR_H

#include "../common.h"
#include "../sim.h"
#include "Tile.h"
#include <string>
#include "accelerator_models/c-model/accelerators.hpp"

using namespace std;

typedef enum{CONVOLUTION,MATRIX_MULT} OPCODE;

class LATransaction : public Transaction {
public:
  LATransaction(int id, int src_id, int dst_id) : Transaction(id,src_id,dst_id) {}
  OPCODE opcode;
  int numColumnsA=50;
  int numRowsA;

  int numColumnsB;
  int numRowsB;

  int cycle_count;
  int avg_power=30; //in milliWatts  
};

class Accelerator: public Tile {  
public:
 
  Transaction* currentTransaction; 
  uint64_t final_cycle=0;
  bool tile_complete=false;
  config_sys_t sys_config;
  bool transaction_pending=false;
  Accelerator(Simulator* sim, uint64_t clockspeed) : Tile(sim, clockspeed) {
    sys_config.tech=cfg.technology_node;
    // mem BW in bytes per cycle
    sys_config.mem_bandwidth=(1e3 * cfg.dram_bw) / cfg.chip_freq;
    sys_config.dram_latency=cfg.dram_latency;
    sys_config.n_IS_tiles=cfg.num_IS;
    sys_config.n_acc_tiles=cfg.num_accels/3; //effectively splitting the e main accelerators eveny
  }
  vector<string> split(const string &s, char delim);
  
  bool process();
  bool ReceiveTransaction(Transaction* t);
};
#endif
