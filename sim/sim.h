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

class DESCQ {
public:
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  deque<pair<DynamicNode*,uint64_t>> supply_q;
  deque<pair<DynamicNode*,uint64_t>> consume_q;
  uint64_t cycles=0;
  int latency=2;
  uint64_t supply_count=0;
  Config config; 
  void process();
  void execute(DynamicNode* d);
  void insert(DynamicNode* d);
};

class Simulator {
public:  
  vector<Core*> cores;
  DESCQ* descq;
  Cache* cache;
  DRAMSimInterface* memInterface;
  
  Simulator();
  void issueDESC(DynamicNode *d);
  void run();
  bool canAccess(bool isLoad);
  void access(Transaction *t);
  void accessComplete(Transaction *t);
  void registerCore(string wlpath, string cfgname, int id);
};

#endif
