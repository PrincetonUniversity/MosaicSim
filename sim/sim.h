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
  deque<DynamicNode*> supply_q;
  deque<DynamicNode*> consume_q;
  map<int,DynamicNode*> supply_map;
  map<DynamicNode*, uint64_t> final_cycle; 
  uint64_t cycles=0;
  int last_supply_id;
  int last_consume_id;
  int latency=2;
  uint64_t supply_count=0;
  Config config; 
  void process();
  bool execute(DynamicNode* d);
  void insert(DynamicNode* d);
};

class Simulator {
public:  
  vector<Core*> cores;
  DESCQ* descq;
  Cache* cache;
  DRAMSimInterface* memInterface;
  
  Simulator();
  void issueDESC(DynamicNode* d);
  void orderDESC(DynamicNode* d);
  void run();
  bool canAccess(bool isLoad);
  void access(Transaction *t);
  void accessComplete(Transaction *t);
  void registerCore(string wlpath, string cfgname, int id);
};

#endif
