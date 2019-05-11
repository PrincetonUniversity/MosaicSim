#ifndef CACHE_H
#define CACHE_H
#include <vector>
#include <queue>
#include "../common.h"
#include "../sim.h"
#include "FunctionalCache.h"
#include <unordered_set>
using namespace std;

class DRAMSimInterface;
class Simulator;
class Cache;


class Cache {
public:
  Simulator *sim;
  DRAMSimInterface *memInterface;
  Cache* parent_cache;
  Cache* child_cache;
  bool isLLC=false;
  bool isL1=false;

  int prefetch_set_size=128;
  queue<uint64_t> prefetch_queue; //order of loads
  unordered_set<uint64_t> prefetch_set;
  int pattern_threshold=4; //how many close addresses to check to determine spatially local accesses
  int min_stride=4; //bytes of strided access
  
  vector<MemTransaction*> to_send;
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
  int prefetch_distance=0;
  int num_prefetched_lines=1;
  FunctionalCache *fc;
  
  Cache(Config cache_cfg): 
    latency(cache_cfg.cache_latency), size_of_cacheline(cache_cfg.cache_linesize), load_ports(cache_cfg.cache_load_ports), store_ports(cache_cfg.cache_store_ports), ideal(cache_cfg.ideal_cache),  prefetch_distance(cache_cfg.prefetch_distance), num_prefetched_lines(cache_cfg.num_prefetched_lines), fc(new FunctionalCache(cache_cfg.cache_size, cache_cfg.cache_assoc, cache_cfg.cache_linesize)) {
    free_load_ports = load_ports;
    free_store_ports = store_ports;
  }
  bool process();
  void execute(MemTransaction* t);
  void addTransaction(MemTransaction *t);
  void TransactionComplete(MemTransaction *t);
  bool willAcceptTransaction(MemTransaction *t);
};

#endif
