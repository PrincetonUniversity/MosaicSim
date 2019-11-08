#ifndef SIM_H
#define SIM_H
#include <chrono>
#include "common.h"
#include "tile/DynamicNode.h"
#include "tile/Tile.h"
#include "tile/LoadStoreQ.h"
#include "memsys/SimpleDRAM.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

class Context;
class DRAMSimInterface;
class Core;
class Cache;
class Transaction;
class DESCQ;
class TransQueue;
class SimpleDRAM;

class DESCCompare {
public:
  bool operator() (const DynamicNode* l, const DynamicNode* r) const {
    //always order producing messages before consuming ones
    return l->desc_id < r->desc_id || (l->type==SEND || l->type==LD_PROD || l->type==STVAL) || l<r;
    /*    
    if (l->desc_id < r->desc_id)
      return true;
    else if(l->desc_id == r->desc_id) {
      return l->type==SEND || l->type==LD_PROD || l->type==STVAL; 
    }
    else {
      return l<r; //yes, the pointers, just to make sure no 2 entries are equal
      } */
  }
};

class Barrier {
public:
  int num_threads=0;
  int count=0;
  map<int, DynamicNode*> barrier_map; //map from tile_id to DN 
  bool register_barrier(DynamicNode* d); //return true if barrier successfully registered, free all barriers once thread count is reached
};

struct loadStat {
  uint64_t addr;
  TInstr type;
  long long issueCycle;
  long long completeCycle;
  bool hit;
  int nodeId;
  int graphNodeId;
  int graphNodeDeg;
};

struct runaheadStat {
  int runahead; 
  int coreId;
  int nodeId;
};

struct cacheStat {
  uint64_t cacheline;
  long long cycle;
  uint64_t offset;
  int nodeId;
  int graphNodeId;
  int graphNodeDeg;
  int unusedSpace;
};

class Simulator {
public:

  chrono::high_resolution_clock::time_point init_time;
  chrono::high_resolution_clock::time_point curr_time;
  chrono::high_resolution_clock::time_point last_time = chrono::high_resolution_clock::now();
  //MLP stats
  int mlp_epoch=1024; //cycles in which to collect mlp stats
  int curr_epoch_accesses=0; 
  vector<int> accesses_per_epoch; //outgoing dram accesses per epoch
  long long mem_chunk_size=1024; //granularity of reading from trace file 
  uint64_t last_instr_count = 0;
  uint64_t cycles=0;
  uint64_t total_instructions=0;
  uint64_t instruction_limit=2200000000; //250000000;
  map<int,Tile*> tiles;
  int tileCount=0;
  vector<uint64_t> clockspeedVec;
  //DESCQ* descq;
  bool decoupling_mode=false;
  bool debug_mode=false;
  bool mem_stats_mode=true;
  string outputDir="";
  ofstream epoch_stats_out;
  Statistics epoch_stats;
  vector<DESCQ*> descq_vec;
  Barrier* barrier = new Barrier();
  Cache* cache;
  Cache* llama_cache;
  string pythia_home;
  //every tile has a transaction priority queue
  unordered_map<int,priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare>> transq_map;
  int transq_latency=3;
  int clockspeed=2000; //default clockspeed in MHz
  uint64_t load_count=0;
  DRAMSimInterface* memInterface;
  //sum of all the runahead distances of all the decoupling pairs
  //runahead distance is cycles between issue of prod/load_prod and issue of consume
  uint64_t runahead_sum=0;
  vector<runaheadStat> runaheadVec;
  unordered_map<DynamicNode*, tuple<long long, long long, bool>> load_stats_map;
  unordered_map<uint64_t, int> graphNodeIdMap; //List of graph node IDs per address;
  unordered_map<int, int> graphNodeDegMap; //List of graph node degrees per ID
  vector<loadStat> load_stats_vector;
  unordered_map<DynamicNode*, uint64_t> recvLatencyMap;
  vector<cacheStat> evictStatsVec;
  int recordEvictions;
  uint64_t total_recv_latency=0;

  vector<int> commQSizes;
  vector<int> SABSizes;  
  vector<int> SVBSizes;
  vector<int> termBuffSizes;
  int commQMax = 0;
  int SABMax = 0;
  int SVBMax = 0; 
  int termBuffMax = 0;
 
  void fastForward(int tid, uint64_t inc);
  bool communicate(DynamicNode* d);
  void orderDESC(DynamicNode* d);
  void run();
  void initDRAM(int clockspeed); //clockspeed in MHz
  bool canAccess(Core* core, bool isLoad);
  void access(Transaction *t);
  void accessComplete(MemTransaction *t);
  void registerCore(string wlpath, string cfgpath, string cfgname, int id);
  void registerTile(Tile* tile, int tid);
  void registerTile(Tile* tile);
  void InsertCaches(vector<Transaction*>& transVec);
  bool InsertTransaction(Transaction* t, uint64_t cycle);
  DESCQ* get_descq(DynamicNode* d);
  DESCQ* get_descq(Tile* tile);
  int getAccelerator();
  void calculateGlobalEnergyPower();
  
  /*atomic operations implementation*/
  bool lockCacheline(DynamicNode* d);
  void evictAllCaches(uint64_t addr);
  bool isLocked(DynamicNode* d);
  bool hasLock(DynamicNode* d);
  void releaseLock(DynamicNode* d);
  unordered_map<uint64_t, DynamicNode*> lockedLineMap; //map from cacheline to dn holding the lock
  unordered_map<uint64_t, queue<DynamicNode*>> lockedLineQ; //map from cacheline to queue of lock requestors

  //loop through all requestors. if lock not held, add to map
  //loop thru map. execute all instructions in map
   
  Simulator(string home);
};

class DESCQ {
public:
  priority_queue<Operator, vector<Operator>, OpCompare> pq;
  deque<DynamicNode*> supply_q;
  deque<DynamicNode*> consume_q;
  //unordered_map<uint64_t,DynamicNode*> send_map;
  unordered_map<uint64_t,DynamicNode*> stval_map;
  //unordered_map<uint64_t,DynamicNode*> recv_map;
  set<DynamicNode*> execution_set; //sorted by desc_id

  map<uint64_t, DynamicNode*> SAB; //for stores (stval)
  unordered_map<uint64_t, DynamicNode*>commQ; //for holding SEND and LD_PROD instructions 

  map<uint64_t, DynamicNode*> commBuff; //for OoO data consumption by compute
  unordered_map<uint64_t, uint64_t> STLMap; //map from recv desc_id to desc_id of send doing stl fwd
  
  map<uint64_t, set<uint64_t>> SVB; //mapping from desc id of STVAL to STVAL instrn and desc id of RECVs awaiting forwards
  map<uint64_t, DynamicNode*> TLBuffer; //terminal load buffer, just to hold DNs waiting for mem response

  // Decoupling-related stuff
  int term_ld_count=0;
  int commBuff_size=64; //comm buff size
  int commQ_size=512; //comm queue size
  int SAB_size=128; //store address buffer size
  int term_buffer_size=32; //max size of terminal load buffer
  int commBuff_count=0;
  int SAB_count=0;
  uint64_t SAB_issue_pointer=0; //DESC_ID of instruction next to issue 
  uint64_t SAB_back=127; //DESC ID of youngest STADDR instruction allowed to issue
  int commQ_count=0;
  int SVB_count=0;
  int SVB_size=128;
  uint64_t SVB_back=127;
  uint64_t SVB_issue_pointer=0;
  
  unordered_map<uint64_t, int64_t> send_runahead_map;

  //map of runahead distance in cycles between when a send (or ld_produce) completes and a receive completes
  map<uint64_t, int64_t> stval_runahead_map;
  map<uint64_t, int64_t> recv_delay_map; 

  //map of address to ordered set of staddr dynamic nodes storing to that address
  map<uint64_t, set<DynamicNode*, DynamicNodePointerCompare>> staddr_map; //store address buffer (SAB)
  
  //map of desc id (of stval, also staddr) to #forwards to be expected
  map<uint64_t, int> stval_svb_map; //store value buffer (SVB)

  
  //map<uint64_t, uint64_t> recv_map;
  //map of desc id (same for ld_prod and recv) to desc id (stval)
  map<DynamicNode*, uint64_t> final_cycle; 
  uint64_t cycles=0;
  uint64_t last_send_id=0;
  uint64_t last_stval_id=0;
  uint64_t last_recv_id=0;
  uint64_t last_staddr_id=0;
  int latency=5;

  Config config;
  DESCQ(Config cfg) {
    commBuff_size=cfg.commBuff_size;
    commQ_size=cfg.commQ_size;
    term_buffer_size=cfg.term_buffer_size;
    latency=cfg.desc_latency;  
    SVB_size=cfg.SVB_size;
    SAB_size=cfg.SAB_size;
    SAB_back=SAB_size-1;
    SVB_back=SVB_size-1;
  }
  
  void process();
  bool execute(DynamicNode* d);
  bool insert(DynamicNode* d, DynamicNode* forwarding_staddr, Simulator* sim);

  DynamicNode* sab_has_dependency(DynamicNode* d);
  
  set<uint64_t> debug_send_set;
  set<uint64_t> debug_stval_set;
};

#endif
