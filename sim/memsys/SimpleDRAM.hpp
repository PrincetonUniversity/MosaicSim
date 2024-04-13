#ifndef SIMPLEDRAM_H
#define SIMPLEDRAM_H

#include "../sim.hpp"
#include <unordered_map>

using namespace std;

class Simulator;
class DRAMSimInterface;

/** \brief   */
struct MemOperator {
  /** \brief   */
  uint64_t addr;
  /** \brief   */
  uint64_t final_cycle;
  /** \brief   */
  uint64_t trans_id;
  /** \brief   */
  uint64_t isWrite;
};

/** \brief   */
class MemOpCompare {
public:
  /** \brief   */
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
  /** \brief   */
  int latency=200; //cycles
 
  /** \brief   */
  int core_clockspeed=2000; //MHz
  /** \brief   */
  int Peak_BW=12; //GB/s
  /** \brief   */
  priority_queue<MemOperator, vector<MemOperator>, MemOpCompare> pq;
  /** \brief   */
  uint64_t trans_id=0;
  /** \brief   */
  int epoch_length=16; //cycles
  /** \brief   */
  int bytes_per_req=64;
  /** \brief   */
  long max_req_per_epoch;
  /** \brief   */
  long request_count=0;
  /** \brief   */
  uint64_t cycles=0;
  /** \brief   */
  int read_ports;
  /** \brief   */
  int write_ports;
  /** \brief   */
  int free_read_ports;
  /** \brief   */
  int free_write_ports;
 
  /** \brief   */
  DRAMSimInterface* memInterface;
    
  /** \brief   */
  unordered_map<uint64_t, queue<Transaction*>> outstanding_read_map;
  /** \brief   */
  unordered_map<uint64_t, queue<Transaction*>> outstanding_write_map;
  /** \brief   */
  SimpleDRAM(DRAMSimInterface* dramInterface, int latency, int bandwith) :
    memInterface(dramInterface), latency(latency), Peak_BW(bandwith) {} 
  /** \brief   */
  void initialize(int coreClockspeed, int bytes_per_req);
  /** \brief   */
  bool process();
  /** \brief   */
  void addTransaction(bool isWrite, uint64_t addr);
  /** \brief   */
  bool willAcceptTransaction(uint64_t addr);
};

#endif
