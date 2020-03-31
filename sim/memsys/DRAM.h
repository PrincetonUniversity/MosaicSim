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
  int read_ports;
  int write_ports;
  int free_read_ports;
  int free_write_ports;
  SimpleDRAM* simpleDRAM;
  unordered_map<uint64_t, queue<pair<Transaction*, uint64_t>>> outstanding_read_map;
  unordered_map<uint64_t, queue<pair<Transaction*, uint64_t>>> outstanding_write_map;
  DRAMSim::MultiChannelMemorySystem *mem;

  DRAMSimInterface(Simulator* sim, bool ideal, int read_ports, int write_ports) : sim(sim),ideal(ideal), read_ports(read_ports), write_ports(write_ports) {

    // create DRAMsim2 object
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
//    string ini_file="/sim/config/DDR3_micron_16M_8B_x8_sg15.ini";
    string ini_file="/sim/config/DDR3_micron_16M_8B_x8_sg3.ini";
//    string ini_file="/sim/config/DDR3_micron_32M_8B_x8_sg25E.ini";
//    string ini_file="/sim/config/DDR3_micron_64M_8B_x4_sg15.ini";
//    string ini_file="/sim/config/DDR3_micron_64M_8B_x8_sg15.ini";
//    string ini_file="/sim/config/DDR3_micron_64M_8B_x8_sg3.ini";
    
    mem = DRAMSim::getMemorySystemInstance(sim->mosaic_home+ini_file, sim->mosaic_home+"/sim/config/dramsys.ini", "..", "MosaicSim", 65536); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);

    // create simple DRAM object
    simpleDRAM = new SimpleDRAM(sim, this, cfg);
    
    free_read_ports = read_ports;
    free_write_ports = write_ports;
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(Transaction* t, uint64_t addr, bool isLoad, int cacheline_size, uint64_t issueCycle);
  bool willAcceptTransaction(uint64_t addr, bool isLoad);
  void initialize(int clockspeed) {
    mem->setCPUClockSpeed(clockspeed * 1000000);
    simpleDRAM->initialize(clockspeed);
  }
  void fastForward(uint64_t inc) {
    if(cfg.SimpleDRAM) {
      simpleDRAM->cycles+=inc;
    }
    else {
      for(uint64_t i=0; i<inc; i++) {
        mem->update();
      }
    }
  }
  void process() {
    free_read_ports = read_ports;
    free_write_ports = write_ports;
    if(cfg.SimpleDRAM) {
      simpleDRAM->process();
    }
    else {
      mem->update();
    }
  }
};
#endif
