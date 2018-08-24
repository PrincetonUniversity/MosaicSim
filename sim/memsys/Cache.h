#ifndef CACHE_H
#define CACHE_H
#include <vector>
#include <queue>
#include "../common.h"
#include "../sim.h"
#include "FunctionalCache.h"
using namespace std;

class DRAMSimInterface;
class Cache {
public:
  Simulator *sim;
  DRAMSimInterface *memInterface;
  vector<Transaction*> to_send;
  vector<uint64_t> to_evict;
  priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare> pq;
  int latency;
  int size_of_cacheline;
  int load_ports;
  int store_ports;
  int free_load_ports;
  int free_store_ports;
  uint64_t cycles = 0;
  bool ideal;
  
  FunctionalCache *fc;
  Cache(int latency, int size, int assoc, int linesize, int load_ports, int store_ports, bool ideal): 
    latency(latency), size_of_cacheline(linesize), load_ports(load_ports), store_ports(store_ports), ideal(ideal), fc(new FunctionalCache(size, assoc)) {
      free_load_ports = load_ports;
      free_store_ports = store_ports;
    }
  bool process();
  void execute(Transaction* t);
  void addTransaction(Transaction *t);
  void TransactionComplete(Transaction *t);
};

#endif