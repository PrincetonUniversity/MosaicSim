#ifndef DRAM_H
#define DRAM_H
#include <unordered_map>
#include <queue>
#include "assert.h"
#include "../common.h"
#include "../include/DRAMSim.h"
#include "../sim.h"

class Cache;
class Simulator;
class SimpleDRAM;
using namespace std;

class DRAMSimInterface {
public:
  //Cache *c;
  Simulator* sim;
  bool ideal;
  int load_ports;
  int store_ports;
  int free_load_ports;
  int free_store_ports;
  SimpleDRAM* simpleDRAM;
  unordered_map<uint64_t, queue<Transaction*>> outstanding_read_map;
  DRAMSim::MultiChannelMemorySystem *mem;

  DRAMSimInterface(Simulator* sim, bool ideal, int load_ports, int store_ports) : sim(sim),ideal(ideal), load_ports(load_ports), store_ports(store_ports) {
    
 
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance(sim->pythia_home+"/sim/config/DDR3_micron_16M_8B_x8_sg15.ini", sim->pythia_home+"/sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    
    simpleDRAM = new SimpleDRAM(sim, this, cfg);
    free_load_ports = load_ports;
    free_store_ports = store_ports;
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(Transaction* t, uint64_t addr, bool isLoad);
  bool willAcceptTransaction(uint64_t addr, bool isLoad);
  void initialize(int clockspeed) {
    mem->setCPUClockSpeed(clockspeed * 1000000);
    simpleDRAM->initialize(clockspeed);
  }
  void process() {
    free_load_ports = load_ports;
    free_store_ports = store_ports;
    if(cfg.SimpleDRAM) {
      simpleDRAM->process();
    }
    else {
      mem->update();
    }
  }
};
#endif
