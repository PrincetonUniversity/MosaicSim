#pragma once

#include <unordered_map>
#include <queue>
#include "assert.h"
#include "config.hpp"
#include "../common.hpp"
#include "../include/DRAMSim.hpp"
#include "../sim.hpp"

class Cache;
class Simulator;
class SimpleDRAM;
using namespace std;

class DRAMSimInterface {
public:
  LLCache *cache;
  bool ideal;
  uint64_t cycles = 0;
  int read_ports;
  int write_ports;
  int free_read_ports;
  int free_write_ports;
  int free_acc_read_ports;
  int free_acc_write_ports;
  SimpleDRAM *simpleDRAM;
  bool use_SimpleDRAM;
  Statistics *global_stat;
  vector<AccBlock> acc_memory;
  unordered_map<uint64_t, queue<pair<Transaction *, uint64_t>>> outstanding_read_map;
  unordered_map<uint64_t, queue<pair<Transaction *, uint64_t>>> outstanding_write_map;
  DRAMSim::MultiChannelMemorySystem *mem;
  /* requests coming from the accelerator */
  PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>> *acc_requests;  
  /* responses to the accelerator */
  PP_static_Buff<uint64_t> *acc_responses;  
  int cacheline_size;
  bool finished_acc = false;
  int LLC_evicted = false;
  uint64_t acc_final_cycle = 0;
  DRAMSimInterface(Simulator *sim, LLCache *cache, bool use_SimpleDRAM,
		   bool ideal, int read_ports, int write_ports,
		   string DRAM_system, string DRAM_device,
		   PP_static_Buff<tuple<uint64_t, uint64_t, double, uint64_t>> *acc_requests,
		   PP_static_Buff<uint64_t> *acc_responses  ) :
    global_stat(sim->global_stat), cache(cache), use_SimpleDRAM(use_SimpleDRAM),
    ideal(ideal), read_ports(read_ports), write_ports(write_ports),
    acc_requests(acc_requests), acc_responses(acc_responses)
  {

    // create DRAMsim2 object
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance(DRAM_device, DRAM_system, "", "MosaicSim", 65536); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);

    // create simple DRAM object
    simpleDRAM = new SimpleDRAM(this, sim->cfg->dram_latency, sim->cfg->dram_bw);
    
    free_read_ports = read_ports;
    free_write_ports = write_ports;
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(Transaction* t, uint64_t addr, bool isRead, uint64_t issueCycle);
  bool willAcceptTransaction(uint64_t addr, bool isRead);
  bool willAcceptAccTransaction(uint64_t addr, bool isRead);
  bool process();
  
  void initialize(int clockspeed, int cacheline_size) {
    mem->setCPUClockSpeed(clockspeed * 1000000);
    simpleDRAM->initialize(clockspeed, cacheline_size);
    this->cacheline_size = cacheline_size;
  }

  /**
     \brief Forwards the cycle counter 
   */
  void fastForward(uint64_t inc) {
    if(use_SimpleDRAM) 
      simpleDRAM->cycles+=inc;
    else
      for(uint64_t i=0; i<inc; i++) 
        mem->update();
  }
};
