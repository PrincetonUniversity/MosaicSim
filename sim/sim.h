#ifndef SIM_H
#define SIM_H
#include <chrono>
#include "common.h"
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

class Simulator {
public:  
  vector<Core*> cores;
  Interconnect* intercon;
  Cache* cache;
  DRAMSimInterface* memInterface;
 
  Simulator();
  void run();
  void registerCore(string wlpath);
};

#endif
