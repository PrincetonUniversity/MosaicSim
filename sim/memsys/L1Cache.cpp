#include "../tile/DynamicNode.hpp"
#include "Cache.hpp"
#include "DRAM.hpp"
#include "../tile/Core.hpp"

static string cache_evicts="cache_evicts";
static string evicts="l1_evicts";
static string l1_accesses="l1_accesses";
static string evicts_dirty="l1_evicts_dirty";
static string evicts_clean="l1_evicts_clean";

static string l1_misses_primary="l1_misses_primary"; // primary_load_misses + primary_store_misses
static string l1_misses_secondary="l1_misses_secondary"; // secondary_load_misses + secondary_store_misses
static string l1_load_misses_primary="l1_load_misses_primary";
static string l1_load_misses_secondary="l1_load_misses_secondary";
static string l1_store_misses_primary="l1_store_misses_primary";
static string l1_store_misses_secondary="l1_store_misses_secondary";
static string l1_forced_evicts="l1_forced_evicts";
static string l1_forced_evicts_clean="l1_forced_evicts_clean";
static string l1_forced_evicts_dirty="l1_forced_evicts_dirty";



bool L1Cache::willAcceptTransaction(bool isLoad, uint64_t addr)  {
  for (auto &block:  acc_memory)
    if (block.contains(addr))
      return false;
  if(isLoad)
    return free_load_ports > 0 || load_ports==-1;
  else
    return free_store_ports > 0 || store_ports==-1;
}

void L1Cache::evict(uint64_t addr, int *acknw) {
  uint64_t evictedOffset = 0;
  int evictedNodeId = -1;
  MemTransaction* t;
  bool present = fc.present(addr);
  bool dirty = fc.evict(addr, &evictedOffset, &evictedNodeId); 

  if (present) {
    stats->update(l1_forced_evicts);
    if (dirty) 
      stats->update(l1_forced_evicts_dirty);
    else 
      stats->update(l1_forced_evicts_clean);
  }
  
  t = new MemTransaction(-1, -1, -1, addr, false);
  t->atomicEviction = true;
  t->dirty = dirty;
  t->acknw = acknw;
  
  to_send.push_back(t);
}

bool L1Cache::process() {
  vector<pair<uint64_t, int *>> &atomic_evict = atomic_evictons.get_comp_buff(cycles); 
  vector<int> &unlock_acknw = unlock_memory.get_comp_buff(cycles);
  for(auto t: to_prefetch)
    if (free_store_ports > 0)
      addPrefetch(t, -1);
    else
      break;
  
  /* accelerator has finished */
  for(int acknw: unlock_acknw)  {
    /* only one accelerator finished at a given time */
    assert(unlock_acknw.size() == 1);
    /* take out the memory blocks */
    for(int i =0; i < acknw; i++)
      if (!acc_memory.empty())
	acc_memory.pop_front();
  }
  unlock_acknw.clear();

  /* Process accelerator memory blocks */
  for (auto &block: acc_memory) {
    if (block.completed_requesting())  
      continue;
    uint64_t addr;
    while(addr = block.next_to_send()) {
      /* Add transaction if present  */
      if (fc.present(addr)) {
	if (free_store_ports > 0 || store_ports==-1) {
	  MemTransaction *t = new MemTransaction(-1, -1, -1, addr, false);
	  t->acc_eviction = true;
	  addTransaction(t, -2);
	  block.sent();
	  free_store_ports--;
	} else {
	  break;
	}
      } else {
	block.sent();
	block.transaction_complete();
      }
    }
  }  
    
  /* send data to the core */
  for(MemTransaction *t: to_core)
    core->accessComplete(t);
  to_core.clear();

  /* Complete transaction incoming from L2 */
  for(MemTransaction *t: to_complete) 
    TransactionComplete(t);
  to_complete.clear();

  /* Perform the evictions issued from the atomic operation */
  for(auto addr: atomic_evict)
    evict(addr.first, addr.second);
  atomic_evict.clear();

  Cache::process();
  
  /* Transactions to send up to L2 and handle the MSHR */
  for(auto it = to_send.begin(); it!= to_send.end(); ) { 
    MemTransaction *t = *it;
    if (t->src_id!=-1 && t->checkMSHR) {
      uint64_t cacheline = t->addr/size_of_cacheline;
      bool mshrMiss = mshr.find(cacheline)==mshr.end();
      if(mshrMiss && mshr.size() < mshr_size) {  // not present in MSHRs and there is space for 1 more entry
        if (size > 0 || ideal) {
          if (t->isLoad) {
            stats->update(l1_load_misses_primary);
          } else {
            stats->update(l1_store_misses_primary);
          }
          stats->update(l1_misses_primary);
          mshr[cacheline]=MSHR_entry();
          mshr[cacheline].insert(t);
          mshr[cacheline].hit=0; 
          t->checkMSHR=false;
        }
      } else if (!mshrMiss) {   // it is already present in MSHRs
        if (t->isLoad) {
          stats->update(l1_load_misses_secondary);
        } else {
          stats->update(l1_store_misses_secondary);
        }
        stats->update(l1_misses_secondary);
        mshr[cacheline].insert(t);
        t->checkMSHR=false;
	it = to_send.erase(it);
        continue;
      }  else if (mshr_size == mshr.size()) {  // not present in MSHRs but they are FULL
        t->checkMSHR=true;   
	++it;
        continue;
      }
    }
    if(parent_cache->willAcceptTransaction(t)) {
      parent_cache->addTransaction(t,-1);
      it = to_send.erase(it);
    } else {
      ++it;
    }
  }
  
  for(auto it = to_evict.begin(); it!= to_evict.end(); ) {
    uint64_t eAddr = *it;
    MemTransaction* t = new MemTransaction(-1, -1, -1, eAddr, false);
    if(size == 0 || parent_cache->willAcceptTransaction(t)) {
      parent_cache->addTransaction(t,-1);
      it = to_evict.erase(it);
    } else {
      ++it;
    }
  }
  
  cycles++;
  
  //reset mlp stats collection
  /* if(cycles % sim->mlp_epoch==0 && cycles!=0) { */
  /*   // sim->accesses_per_epoch.push_back(sim->curr_epoch_accesses); */
  /*   sim->curr_epoch_accesses=0; */
  /* } */
    
  /* reinit the ports */ 
  free_load_ports = load_ports;
  free_store_ports = store_ports;
  free_prefetch_ports = cache_prefetch_ports;

  return (pq.size() > 0);  
}

bool L1Cache::execute(MemTransaction* t) {
  /* Handle the atomic evictions */
  if (t->acc_eviction) {
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;
    bool dirty = fc.evict(t->addr, &evictedOffset, &evictedNodeId); 
    
    stats->update(l1_forced_evicts);
    if (dirty) 
      stats->update(l1_forced_evicts_dirty);
    else 
      stats->update(l1_forced_evicts_clean);
    /* Send it to the L2 */
    if(parent_cache->willAcceptTransaction(t)) 
      parent_cache->addTransaction(t,-2);
    else
      to_send.push_back(t);
    return true;
  }
  
  bool res = Cache::execute(t);
  
  // Update the MSHR
  if (res) {
    if(t->src_id!=-1) { //just normal hit, not an eviction from lower cache
      uint64_t cacheline=t->addr/size_of_cacheline;
      mshr[cacheline]=MSHR_entry();
      mshr[cacheline].insert(t);
      mshr[cacheline].hit=true;
      
      transComplete_MSHR_update(t);
    } else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      delete t; 
    }
    /* misses */
  } else {
    if (t->src_id !=-1)
      t->checkMSHR=true;
    to_send.push_back(t); //send higher up in hierarchy
  }
  return res;
}

void L1Cache::addPrefetch(MemTransaction *t, uint64_t extra_lat) {
  pq.push(make_pair(t, cycles+latency+extra_lat));
  stats->update("cache_access");
  free_prefetch_ports--;
}

void L1Cache::addTransaction(MemTransaction *t, uint64_t extra_lat) {
  Cache::addTransaction(t, extra_lat);
  
  /* Detect and perform prefetching if a stride is detected */
  /* for prefetching, don't issue prefetch for evict or for access
     with outstanding prefetches or for access that is prefetch or
     eviction */
  if(t->src_id>=0 && num_prefetched_lines>0) {
    bool pattern_detected=false;
    bool single_stride=true;
    bool double_stride=true;
    int stride=min_stride;
    
    //detect single stride
    for(int i=1; i<(pattern_threshold+1); i++) {
      if(prefetch_set.find(t->d->addr-stride*i)==prefetch_set.end()) {
        single_stride=false;
        break;
      }
    }
    //detect double stride
    if(!single_stride) {
      stride=2*min_stride;
      for(int i=1; i<(pattern_threshold+1); i++) {
        if(prefetch_set.find(t->d->addr-stride*i)==prefetch_set.end()) {
          double_stride=false;
          break;
        }
      }      
    }
    pattern_detected=single_stride || double_stride;
    //don't prefetch or insert prefectch instrs into prefetch set
    if(!t->issuedPrefetch && !t->isPrefetch) {
      if(pattern_detected) { 
        for (int i=0; i<num_prefetched_lines; i++) {
          MemTransaction* prefetch_t = new MemTransaction(-2, -2, -2, t->addr + size_of_cacheline*(prefetch_distance+i), true); //prefetch some distance ahead
          prefetch_t->d=t->d;
          prefetch_t->isPrefetch=true;
	  if (free_prefetch_ports > 0) { 
	    addPrefetch(prefetch_t,-1);	    
	  } else {
	    to_prefetch.push_back(prefetch_t);
	  }
        }
        prefetch_set.insert(t->d->addr);
        prefetch_set.erase(t->d->addr-stride*pattern_threshold); 
      } else { //keep prefetch set size capped
        int current_size = prefetch_queue.size();
        if(current_size >= prefetch_set_size) {
          prefetch_set.erase(prefetch_queue.front());
          prefetch_queue.pop();
          prefetch_set_size--;
        }
        prefetch_set.insert(t->d->addr);
        prefetch_queue.push(t->d->addr);
        prefetch_set_size++; 
      }      
    }   
  }
}

void L1Cache::transComplete_MSHR_update(MemTransaction *t) {
  uint64_t cacheline=t->addr/size_of_cacheline;
  if (mshr.find(cacheline) == mshr.end()) {
    if(core->debug_mode || core->mem_stats_mode) {
      DynamicNode* d=t->d;
      assert(d!=NULL);
      /* REINTRODUCE STATS */
      /* assert(core->sim->load_stats_map.find(d)!=core->sim->load_stats_map.end()); */
      /* get<2>(core->sim->load_stats_map[d])=1; */
    } 
  } else if (size > 0 || ideal) {
    MSHR_entry mshr_entry = mshr[cacheline];
    auto trans_set=mshr_entry.opset;
    
    //process callback for each individual transaction in batch
    for (auto it=trans_set.begin(); it!=trans_set.end(); ++it) {
      MemTransaction* curr_t=*it;
      if(!curr_t->isPrefetch) {
	//record statistics on non-prefetch loads/stores
	if(core->sim->debug_mode || core->sim->mem_stats_mode) {
	  DynamicNode* d=curr_t->d;
	  assert(d!=NULL);
	  /* REINTRODUCE STATS */
	  /* assert(core->sim->load_stats_map.find(d)!=core->sim->load_stats_map.end()); */
	  /* get<2>(core->sim->load_stats_map[d])=mshr_entry.hit; */
	}
	to_core.push_back(curr_t);
      } else { //prefetches get no callback, tied to no dynamic node
	delete curr_t;
      }   
    }
    mshr.erase(cacheline); //clear mshr for that cacheline
  }
}

void L1Cache::TransactionComplete(MemTransaction *t) { 
  Cache::TransactionComplete(t);
  /* Handle the MSHR update */
  transComplete_MSHR_update(t);
}

