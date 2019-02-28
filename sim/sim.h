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
struct Transaction;

class DESCQ {
public:
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  deque<DynamicNode*> supply_q;
  deque<DynamicNode*> consume_q;
  map<uint64_t,DynamicNode*> send_map;
  map<uint64_t,DynamicNode*> stval_map;
  
  map<uint64_t, uint64_t> send_runahead_map;
  //map of runahead distance in cycles between when a send (or ld_produce) completes and a receive completes
  
  map<uint64_t, uint64_t> stval_runahead_map;
  map<uint64_t, uint64_t> recv_delay_map; 
  
  int consume_size=64; //comm buff size
  int supply_size=512; //comm queue size
  int consume_count=0;
  int supply_count=0;

  map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> staddr_map; //store address buffer (SAB)
  //map of address to ordered set of staddr dynamic nodes storing to that address
  
  
  map<uint64_t, int> stval_svb_map; //store value buffer (SVB)
  //map of desc id (of stval, also staddr) to #forwards to be expected

  map<uint64_t, uint64_t> recv_map;
  //map of desc id (same for ld_prod and recv) to desc id (stval)
  
  
  map<DynamicNode*, uint64_t> final_cycle; 
  uint64_t cycles=0;
  uint64_t last_send_id;
  uint64_t last_stval_id;
  uint64_t last_recv_id;
  uint64_t last_staddr_id;
  int latency=5;
  deque<DynamicNode*> term_ld_buffer;
  int term_buffer_size=32; //max size of terminal load buffer
  Config config; 
  void process();
  bool execute(DynamicNode* d);
  bool insert(DynamicNode* d);
  bool updateSAB(DynamicNode* lpd); //returns true if ld_prod can be done via forwarding, avoiding mem access
  void updateSVB(DynamicNode* lpd, DynamicNode* staddr_d);
  bool stvalFwdPending(DynamicNode* stval_d);
  void insert_staddr_map(DynamicNode* d);
  bool decrementFwdCounter(DynamicNode* recv_d);
  set<uint64_t> debug_send_set;
  set<uint64_t> debug_stval_set;
};

class Simulator {
public:
  chrono::high_resolution_clock::time_point init_time;
  chrono::high_resolution_clock::time_point curr_time;
  chrono::high_resolution_clock::time_point last_time;
  uint64_t last_instr_count;
  uint64_t total_instructions=0;
  vector<Core*> cores;
  DESCQ* descq;
  Cache* cache;
  DRAMSimInterface* memInterface;
  vector<Cache*> Caches;
  
  Simulator();
  bool communicate(DynamicNode* d);
  void orderDESC(DynamicNode* d);
  void run();
  bool canAccess(Core* core, bool isLoad);
  void access(Transaction *t);
  void accessComplete(Transaction *t);
  void registerCore(string wlpath, string cfgname, int id);
  void TransactionComplete(Transaction* t);
  void InsertCaches(vector<Transaction*>& transVec);
};

#endif
