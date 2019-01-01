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
  map<uint64_t,DynamicNode*> send_map;
  map<uint64_t,DynamicNode*> stval_map;
  int consume_size=128; //comm buff size
  int supply_size=128; //comm queue size
  int consume_count=0;
  int supply_count=0;
  map<DynamicNode*, uint64_t> final_cycle; 
  uint64_t cycles=0;
  uint64_t last_send_id;
  uint64_t last_stval_id;
  uint64_t last_recv_id;
  uint64_t last_staddr_id;
  int latency=2;  
  Config config; 
  void process();
  bool execute(DynamicNode* d);
  bool insert(DynamicNode* d);
  set<uint64_t> debug_send_set;
  set<uint64_t> debug_stval_set;
};

class Simulator {
public:  
  vector<Core*> cores;
  DESCQ* descq;
  Cache* cache;
  DRAMSimInterface* memInterface;
  
  Simulator();
  bool communicate(DynamicNode* d);
  void orderDESC(DynamicNode* d);
  void run();
  bool canAccess(Core* core, bool isLoad);
  void access(Transaction *t);
  void accessComplete(Transaction *t);
  void registerCore(string wlpath, string cfgname, int id);
  void TransactionComplete(Transaction* t);
  void InsertCaches(const vector<Transaction*>& transVec);
};

#endif
