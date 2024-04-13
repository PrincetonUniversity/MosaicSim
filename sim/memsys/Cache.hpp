#pragma once

#include <vector>
#include <queue>
#include "../common.hpp"
#include "../sim.hpp"
#include "../misc/Memory_helpers.hpp"
#include "FunctionalCache.hpp"
#include <unordered_set>
#include "CommMec.hpp"

using namespace std;

class DRAMSimInterface;
class Simulator;
class L2Cache;
class Core;
class L1Cache;
class LLCache;

/** \brief   */
struct TransPointerLT {
    bool operator()(const MemTransaction* a, const MemTransaction* b) const {
      if(a->id >= 0 && b->id >= 0) { //not eviction or prefetch
        return *(a->d) < *(b->d); //compare their dynamic nodes
      } else {
        return a < b; //just use their pointers, so they're unique in the set
      }        
    }
};

/** \brief   */
class MSHR_entry {
 public:
  /** \brief   */
  set<MemTransaction*, TransPointerLT> opset;
  /** \brief   */
  bool hit=false;
  /** \brief   */
  int non_prefetch_size=0;
  /** \brief   */
  void insert(MemTransaction* t){
    opset.insert(t);
    //inc num of real mem ops
    
    if(!t->isPrefetch) 
      non_prefetch_size++; 
  }
};

/** \brief Abstract class for the different level of caches.
    
    It contains the common code amongs the different levels of caches
    (L1, L2 and LLC).
 */
class Cache {
public:
  /** \brief Cache type. L1, L2 or LLC  */
  string type;
  /** \brief transactions to be send to the upper level of cache.  */
  vector<MemTransaction *> to_send;
  /** \brief adresses to be evicted   */
  vector<uint64_t> to_evict;
  /** \brief The priority qeue with transaction to be executed once the cycle is reached  */
  priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare> pq;

  /** \brief Total size of the cache  */
  int size;
  /** \brief Cache's latency   */
  int latency;
  /** \brief Size of a cacheline  */
  int size_of_cacheline;
  /** \brief Cache's associativity
      
      Used only for the functional cache. 
   */
  int cache_associativity;
  /** \brief Number of load ports. 

      The maxmium number of load transaction. 
   */
  int load_ports;
  /** \brief  Number of store ports. 

      The maxmium number of store transaction. 
  */
  int store_ports;
  /** \brief  Current free load ports */
  int free_load_ports;
  /** \brief Current free store ports  */
  int free_store_ports;
  /** \brief  The current cycle */
  uint64_t cycles = 0;
  /** \brief Ideal cache. 

      Every memory access is a hit.
   */
  bool ideal;
  /** \brief The class that deals with memory addresses */ 
  FunctionalCache fc;
  /** \brief Statistics on the current run */
  Statistics *stats;
  /** \brief Transaction to be completed by the TransactionComplete */
  vector<MemTransaction *> to_complete;
  
  /** \brief   */
  Cache(int size, int latency, int size_of_cacheline, int cache_assoc, int load_ports, int store_ports,
	bool ideal, Statistics *global_stat, string type):
    size(size), latency(latency), size_of_cacheline(size_of_cacheline), cache_associativity(cache_assoc),
    load_ports(load_ports), store_ports(store_ports), free_load_ports(load_ports), free_store_ports(store_ports),
    ideal(ideal), stats(global_stat), type(type), fc(size, cache_assoc, size_of_cacheline)  { }

  /** \brief  Returns true if the cache will accept a new transaction, false otherwise. 

      It checks for available read/write ports. 
   */
  bool willAcceptTransaction(MemTransaction *t);

  /** \brief Executes all the ready transactions. 

      It execute all the memory transaction from pq for which the
      latency has passed. 
   */
  virtual bool process();
  
  /** \brief Executes a memory transactions. 

      Access the FunctionalCache and updates the stats. 
   */
  virtual bool execute(MemTransaction* t);

  /** \brief Adds a new transaction. 
      
      Inserts a new transaction in pq and adjusts the ports.
      willAcceptTransaction should be called before calling this
      method.
   */
  virtual void addTransaction(MemTransaction *t, uint64_t extra_lat);
  
  /** \brief Completes a Transaction. 

      Inserts a new entry in the FunctionalCache and updates stats. If
      an eviction occures, that address is added to the to_evict
      vector.
   */
  virtual void TransactionComplete(MemTransaction *t);
};

class L1Cache : public Cache {
public:
  /** \brief The core that the cache is attached to  */
  Core *core;
  /** \brief The L2 cache  */
  L2Cache* parent_cache = NULL;
  /** \brief Memory transaction to be sent to the core  */
  vector<MemTransaction *> to_core;  
  /** \brief Memory transaction to prefetch  */
  vector<MemTransaction *> to_prefetch;  
  /** \brief Number of bytes to be prefeteched */
  int prefetch_set_size=128;
  /** \brief order of loads. Not vert clear. The MSHR mechanisim needs to be reviced.   */
  queue<uint64_t> prefetch_queue;
  /** \brief Not vert clear. The MSHR mechanisim needs to be reviced. */
  unordered_set<uint64_t> prefetch_set;
  /** \brief how many neighboring addresses to check to determine spatially local accesses */
  int pattern_threshold=4;
  /** \brief bytes of strided access */
  int min_stride=4;
  /** \brief Number of ports dedicated to the prefetch */
  int cache_prefetch_ports=4;
  /** \brief Live nb of ports dedicated to the prefetch */
  int free_prefetch_ports;
  
  /** \brief map of cacheline to set to mshr_entry4  */
  unordered_map<uint64_t,MSHR_entry> mshr;
  /** \brief Addresses that need to be evicted due to atomic conflict.  */
  PP_Buff<pair<uint64_t, int *>> atomic_evictons;
  
  /** \brief   */
  int mshr_size;
  /** \brief   */
  int prefetch_distance=0;
  /** \brief   */
  int num_prefetched_lines=1;
  /** \brief Buffer used for communication with LLC for the accelerator's memory */
  PP_Buff<int> unlock_memory;
  /** \brief Blocks of memory that are locked due to  accelerator usage  */
  deque<AccBlock> acc_memory;

  
  /** \brief   */
  L1Cache(Config cache_cfg, Statistics *global_stat): Cache(cache_cfg.cache_size, cache_cfg.cache_latency, cache_cfg.cache_linesize, cache_cfg.cache_assoc, cache_cfg.cache_load_ports, cache_cfg.cache_store_ports, cache_cfg.ideal_cache, global_stat, "l1"),
						      mshr_size(cache_cfg.mshr_size),
						      prefetch_distance(cache_cfg.prefetch_distance),
						      num_prefetched_lines(cache_cfg.num_prefetched_lines),
						      cache_prefetch_ports(cache_cfg.cache_prefetch_ports),
						      free_prefetch_ports(cache_cfg.cache_prefetch_ports){}
						      
  /** \brief Evicts for atomic operations.
      
      Evicts the address from the local chache. It sends
      an eviction transaction to the L2. This transaction is flaged as
      atomic eviction, so that the L2 will always send a notification
      to the LLC.
   */
  void evict(uint64_t addr, int *acknw);
  /** \brief  Process all the in/out going transactions. 

      Additionally it deals with releasing the accelerators locked
      memory.
   */
  bool process() override;
  /** \brief Executes the transaction. 

      If it is an eviction due to the accelerators, the address is
      evicted from the FunctionalCache.
   */
  bool execute(MemTransaction* t) override;
  /** \brief  Adds the tranasaction to be executed. 

      It also detects memory strides. If one is detected, a prefetch
      is issued.
   */
  void addTransaction(MemTransaction *t, uint64_t extra_lat) override;
  /** \brief  Adds a prefetch  tranasaction to be executed. 

      It also consumes a prefetch port.
   */
  void addPrefetch(MemTransaction *t, uint64_t extra_lat); 
  /** \brief Handles a termination of a transaction. 

      It notifies the core that the data has arrived. 
   */
  void TransactionComplete(MemTransaction *t) override;
  /** \brief Additionally  to checking the port availability, it handles locked memory for the accelerators */
  bool willAcceptTransaction(bool isLoad, uint64_t addr);
private:
  /** \brief   */
  void transComplete_MSHR_update(MemTransaction *t);
};

class L2Cache: public Cache{
 public:
  /** \brief  The LLC */
  LLCache* parent_cache;
  /** \brief  The L1 cache  */
  L1Cache* child_cache;
  /** \brief transaction to complete

      Tranasaction that have been completed by the LLC. 
   */
  PP_Buff<MemTransaction *> to_complete;
  /** \brief   */
  L2Cache(Config cache_cfg, Statistics *global_stat):
    Cache(cache_cfg.l2_cache_size, cache_cfg.l2_cache_latency,
	  cache_cfg.l2_cache_linesize, cache_cfg.l2_cache_assoc,
	  cache_cfg.l2_cache_load_ports, cache_cfg.l2_cache_store_ports,
	  cache_cfg.ideal_cache, global_stat, "l2") {}
  /** \brief  Process all the in/out going transactions. */
  bool process() override;
  /** \brief  Execute the transaction. 

      Atomic evictions and accelerator evictions are handled here.
  */
  bool execute(MemTransaction* t) override;
  /** \brief Completes the transaction and notifies the L1 cache.  */
  void TransactionComplete(MemTransaction *t)  override;
};

class LLCache: public Cache {
public:
  /** \brief deals with locked atomic transactions  */
  vector<MemTransaction *> to_execute;
  /** \brief deals with locked atomic transactions  */
  vector<MemTransaction *> next_to_execute;
  /** \brief Incoming  transaction from the LLC
   */
  vector<MemTransaction *> to_add;
  /** \brief The DRAM mememory interface   */
  DRAMSimInterface *memInterface;
  /** \brief Transaction for witch the DRAM has completed  */
  vector<MemTransaction *> to_complete;
  /** \brief Incoming loads from all the L2 caches */
  PP_static_Buff<MemTransaction *> incoming_loads;
  /** \brief Incoming stores from all the L2 caches */
  PP_static_Buff<MemTransaction *> incoming_stores;
  /** \brief Release the locked data for atomic operations (the data
     has arrived to the Core) */
  PP_Buff<DynamicNode *> to_release;

  /** \brief map from cacheline to dn holding the lock  */
  unordered_map<uint64_t, DynamicNode*> lockedLineMap;
  /** \brief  map from cacheline to queue of lock requestors  */
  unordered_map<uint64_t, queue<DynamicNode*>> lockedLineQ;
  /** \brief map from cacheline to an int that holds the number of
      acknowledgments */
  unordered_map<uint64_t, int *> acknowledgements;

  /** \brief Pointer to tiles. Used for the forced (atomic/accelerator) eviction */
  map<int,Tile*> *tiles;
  /** \brief How many lower caches are there

      TODO: for now we assume #core == #l2 caches
 */
  int nb_cores;
  /** \brief Memory blocks locked due to accelerator usage */
  vector<AccBlock> accelertor_mem;
  bool doing_acc = false;
  bool acc_finished = false;
  int nb_acc_blocks = 0;
  
  LLCache(Config &cache_cfg, Statistics *global_stat, int load_ports, int store_ports):
    Cache(cache_cfg.cache_size, cache_cfg.cache_latency, cache_cfg.cache_linesize,
	  cache_cfg.cache_assoc, load_ports, store_ports,
	  cache_cfg.ideal_cache, global_stat, "l3"),
    incoming_loads(load_ports),
    incoming_stores(store_ports) { }
  
  /** \brief Locks a cacheline, and it evicts it from all the lower
      levels of cache */
  bool lockCacheline(DynamicNode* d);
  /** \brief Evicts the cacheline from all the lower level caches  */
  void evictAllCaches(uint64_t addr, int *ackwn);
  /** \brief Returns true if the cacheline is locked  */
  bool isLocked(DynamicNode* d);
  /** \brief Returns true if the cacheline has lock */
  bool hasLock(DynamicNode* d);
  /** \brief Release the lock and pass the lock to the next
      transaction, if present */
  void releaseLock(DynamicNode* d);
  
  /** \brief   */
  bool process() override;
  /** \brief   */
  bool execute(MemTransaction* t) override;
  /** \brief Completes the transaction and sends the transaction to
      the L2 */
  void TransactionComplete(MemTransaction *t)  override;
};
