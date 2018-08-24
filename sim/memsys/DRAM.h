#ifndef DRAM_H
#define DRAM_H
#include <unordered_map>
#include <queue>
#include "assert.h"
#include "../include/DRAMSim.h"
class Cache;
class DynamicNode;

using namespace std;

class DRAMSimInterface {
public:
  Cache *c;
  bool ideal;
  int mem_load_ports;
  int mem_store_ports;
  unordered_map< uint64_t, queue<DynamicNode*> > outstanding_read_map;
  DRAMSim::MultiChannelMemorySystem *mem;

  DRAMSimInterface(Cache *c, bool ideal, int mem_load_ports, int mem_store_ports) : c(c),ideal(ideal), mem_load_ports(mem_load_ports), mem_store_ports(mem_store_ports) {
    DRAMSim::TransactionCompleteCB *read_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::read_complete);
    DRAMSim::TransactionCompleteCB *write_cb = new DRAMSim::Callback<DRAMSimInterface, void, unsigned, uint64_t, uint64_t>(this, &DRAMSimInterface::write_complete);
    mem = DRAMSim::getMemorySystemInstance("sim/config/DDR3_micron_16M_8B_x8_sg15.ini", "sim/config/dramsys.ini", "..", "Apollo", 16384); 
    mem->RegisterCallbacks(read_cb, write_cb, NULL);
    mem->setCPUClockSpeed(2000000000);
  }
  void read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle);
  void addTransaction(DynamicNode* d, uint64_t addr, bool isLoad);
  bool willAcceptTransaction(DynamicNode* d, uint64_t addr);\
  void process() {
    mem->update();
  }
};
#endif