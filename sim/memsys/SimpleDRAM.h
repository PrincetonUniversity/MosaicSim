#ifndef SIMPLEDRAM_H
#define SIMPLEDRAM_H
//
#include "../sim.h"
#include <unordered_map>
//#include <priority_queue>

using namespace std;

class Simulator;
class DRAMSimInterface;

struct MemOperator {
  uint64_t addr;
  uint64_t final_cycle;
  uint64_t trans_id;
  uint64_t isWrite;
};

class MemOpCompare {
public:
  bool operator() (const MemOperator &l, const MemOperator &r) const {
    if(r.final_cycle < l.final_cycle) 
      return true;
    else if (r.final_cycle == l.final_cycle) 
      return (r.trans_id < l.trans_id);
    else
      return false;
  }
};

class SimpleDRAM {
 public:
  Simulator* sim;
  int latency=200; //cycles
 
  int core_clockspeed=2000; //MHz
  int Peak_BW=12; //GB/s
  priority_queue<MemOperator, vector<MemOperator>, MemOpCompare> pq;
  uint64_t trans_id=0;
  int epoch_length=16; //cycles
  int bytes_per_req=64;
  long max_req_per_epoch;
  long request_count=0;
  uint64_t cycles=0;
  int read_ports;
  int write_ports;
  int free_read_ports;
  int free_write_ports;
 
  DRAMSimInterface* memInterface;
    
  unordered_map<uint64_t, queue<Transaction*>> outstanding_read_map;
  unordered_map<uint64_t, queue<Transaction*>> outstanding_write_map;
  SimpleDRAM(Simulator* simulator, DRAMSimInterface* dramInterface, Config dram_config); 
  void initialize(int coreClockspeed);
  bool process();
  void addTransaction(bool isWrite, uint64_t addr);
  bool willAcceptTransaction(uint64_t addr);
};

#endif
