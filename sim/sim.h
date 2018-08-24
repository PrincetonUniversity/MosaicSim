#ifndef SIM_H
#define SIM_H
#include <chrono>
#include "core/DynamicNode.h"
using namespace std;

class Context;
class DRAMSimInterface;
class Core;
class Cache;

class Interconnect {
public:
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  uint64_t cycles=0;
  int latency=2;
  
  void process();
  void execute(DynamicNode* d);
  void insert(DynamicNode* d);
};

class Digestor {
public:  
  vector<Core*> all_sims;
  Interconnect* intercon;
  Cache* cache;
  DRAMSimInterface* memInterface;
 
  Digestor();
  void run();
};

#endif
