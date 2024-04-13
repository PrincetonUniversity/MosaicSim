#include "../tile/DynamicNode.hpp"
#include "Cache.hpp"
#include "DRAM.hpp"
#include "../tile/Core.hpp"

static string cache_evicts="cache_evicts";
static string evicts="l2_evicts";
static string evicts_dirty="l2_evicts_dirty";
static string evicts_clean="l2_evicts_clean";
static string l2_writebacks="l2_writebacks"; 
static string l2_accesses="l2_accesses";
static string l2_forced_evicts="l2_forced_evicts";
static string l2_forced_evicts_clean="l2_forced_evicts_clean";
static string l2_forced_evicts_dirty="l2_forced_evicts_dirty";

bool L2Cache::process() {
  /* Treat the tranasactions to complete coming from the LLC */
  vector<MemTransaction *> &_to_complete = to_complete.get_comp_buff(cycles);
  for(MemTransaction *t: _to_complete) {
    TransactionComplete(t);
  }
  _to_complete.clear();

  Cache::process();
  
  for(auto it = to_send.begin(); it!= to_send.end();) { 
    MemTransaction *t = *it;
    bool sent; 

    t->child_cache = this;
    if(t->isLoad)
      sent = parent_cache->incoming_loads.insert(cycles, t);
    else
      sent = parent_cache->incoming_stores.insert(cycles, t);

    if(sent) 
      it = to_send.erase(it);
    else 
      ++it;
  }
  
  for(auto it = to_evict.begin(); it!= to_evict.end();) {
    uint64_t eAddr = *it;
    MemTransaction* t = new MemTransaction(-1, -1, -1, eAddr, false);

    if (parent_cache->incoming_stores.insert(cycles, t)) 
      it = to_evict.erase(it);
    else 
      ++it;
  }
  
  cycles++;

  //reset mlp stats collection
  /* if(cycles % sim->mlp_epoch==0 && cycles!=0) { */
  /*   // sim->accesses_per_epoch.push_back(sim->curr_epoch_accesses); */
  /*   sim->curr_epoch_accesses=0; */
  /* } */
    
  free_load_ports = load_ports;
  free_store_ports = store_ports;
  return (pq.size() > 0);  
}

bool L2Cache::execute(MemTransaction* t) {
  bool res;
  if (t->acc_eviction) {
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;
    bool dirty = fc.evict(t->addr, &evictedOffset, &evictedNodeId); 
    
    stats->update(l2_forced_evicts);
    if (dirty) 
      stats->update(l2_forced_evicts_dirty);
    else 
      stats->update(l2_forced_evicts_clean);
    delete t;
    return true;
  }

  /* Handle atomic evictions */
  if (t->atomicEviction) {
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;
    bool res, present = fc.present(t->addr);
    int *acknw = t->acknw;
    
    if (t->dirty)
      stats->update(l2_writebacks);
    t->dirty = fc.evict(t->addr, &evictedOffset, &evictedNodeId);

   if (present) {
      stats->update(l2_forced_evicts);
      if (t->dirty) 
	stats->update(l2_forced_evicts_dirty);
      else 
	stats->update(l2_forced_evicts_clean);
   }

#pragma omp atomic update
    *acknw = *acknw + 1;
    
    delete t;

    return res;
  }

  res = Cache::execute(t);  

  if (res) {
    if(t->src_id!=-1) { //just normal hit, not an eviction from lower cache
      child_cache->to_complete.push_back(t);
    } else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      delete t;
      stats->update(l2_writebacks);
    }
    /* misses */
  } else {
    if (t->src_id == -1){
      stats->update(l2_writebacks);
    } else { 
      to_send.push_back(t);
    }
  }
  
  return res;
}
  
void L2Cache::TransactionComplete(MemTransaction *t) { 
  Cache::TransactionComplete(t);
  child_cache->to_complete.push_back(t);      
}
