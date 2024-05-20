#include "../tile/DynamicNode.hpp"
#include "Cache.hpp"
#include "DRAM.hpp"
#include "../tile/Core.hpp"

static string l3_writebacks="l3_writebacks"; 

bool LLCache::isLocked(DynamicNode* d) {
  uint64_t cacheline=d->addr/d->core->cache->size_of_cacheline;
  return lockedLineMap.find(cacheline) != lockedLineMap.end();
}

bool LLCache::hasLock(DynamicNode* d) {
  uint64_t cacheline=d->addr/size_of_cacheline;
  return lockedLineMap.find(cacheline)!=lockedLineMap.end() && lockedLineMap[cacheline]==d;
}

void LLCache::releaseLock(DynamicNode* d) {
  uint64_t cacheline=d->addr/size_of_cacheline;

  /* assign lock to next in queue */
  if(lockedLineQ.find(cacheline)!=lockedLineQ.end() && !lockedLineQ[cacheline].empty()) {
    lockedLineMap[cacheline]=lockedLineQ[cacheline].front();
    lockedLineQ[cacheline].pop();
    int *new_entry = new int(0);
    acknowledgements[cacheline] = new_entry;
    /* upon assignment of lock, must evict all cachelines and reset acknowledgements */
    evictAllCaches(d->addr, new_entry);
  } else {
    lockedLineMap.erase(cacheline);
    lockedLineQ.erase(cacheline);
  }
}

bool LLCache::lockCacheline(DynamicNode* d) {
  uint64_t cacheline=d->addr/size_of_cacheline;
  if(isLocked(d) && !hasLock(d)) { //if someone else holds lock
    if(!d->requestedLock) {//havent requested before
      lockedLineQ[cacheline].push(d); //enqueue
    }
    d->requestedLock=true;
    return false;
  }
  d->requestedLock=true;
  /*If no one holds the lock, must evict cachelines.
    If you hold the lock, that was done right when you were given the lock
    (i.e., in the code line above or when the last owner released the lock)*/
  if(!isLocked(d)) {
    int *new_entry = new int(0);
    acknowledgements[cacheline] = new_entry;
    /* pass in address, not cacheline */
    evictAllCaches(d->addr, new_entry);
  }
  /* get the lock, idempotent if you already have it */
  lockedLineMap[cacheline]=d;
  return true;
}

void LLCache::evictAllCaches(uint64_t addr, int *ackwn) {
  for(auto id_tile: *tiles)
    if(Core* core=dynamic_cast<Core*>(id_tile.second)) {
      vector<pair<uint64_t, int *>> &evictions =  core->cache->atomic_evictons.get_comm_buff(cycles);
      evictions.push_back({addr, ackwn});
    }
}

bool LLCache::process() {
  vector<MemTransaction*> next_to_send;
  vector<DynamicNode *> &cachelines_to_release = to_release.get_comp_buff(cycles);
  int finished_blocks = 0;
  /* incoming transaction from L2. */ 
  int nb_loads  = incoming_loads.get_comp_elems(cycles);
  int nb_stores = incoming_stores.get_comp_elems(cycles);
  MemTransaction **loads  = incoming_loads.get_comp_buff(cycles);
  MemTransaction **stores = incoming_stores.get_comp_buff(cycles);
  
  /** accelerator started. Notify all the L1 */
  if(acc_started) {
    for(auto id_tile: *tiles)
      if(Core* core=dynamic_cast<Core*>(id_tile.second)) {
    	vector<AccBlock> &L1_acc_memory =  core->cache->acc_comm.get_comm_buff(cycles);
	for (auto block: accelertor_mem) {
	  L1_acc_memory.push_back(block);
	}
      }
    acc_started = false;
  }

  /* Process accelerator memory blocks */
  /* TODO: need to send notification to L1 and somehow wait for a notification back */
  for (auto &block: accelertor_mem) {
    if (block.completed_requesting())  
      continue;
    uint64_t addr;
    while(addr = block.next_to_send()) {
      /* Add accelerator's eviction transaction transaction if present  */
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

  /* Release the locked data for atomic operations (the data has
     arrived to the Core) */
  for(auto dynNode: cachelines_to_release)
    releaseLock(dynNode);
  cachelines_to_release.clear();

  // add the old transaction from L2
  for(auto t = to_add.begin(); t!= to_add.end(); ) { 
    if(willAcceptTransaction(*t)){
      addTransaction(*t, -2);
      t = to_add.erase(t); 
    } else {
      t++;
    }
  }

  // add the old transaction from L2
  for(int i = 0; i < nb_loads; ++i) {
    MemTransaction *t = loads[i];
    if(willAcceptTransaction(t))
      addTransaction(t, -2);
    else
      to_add.push_back(t);
  }
  for(int i = 0; i < nb_stores; ++i) {
    MemTransaction *t = stores[i];
    if(willAcceptTransaction(t))
      addTransaction(t, -2);
    else
      to_add.push_back(t);
  }
  
  for(MemTransaction *t: to_complete)
    TransactionComplete(t);
  to_complete.clear();

  next_to_execute.clear();
  /* Process previously blocked transactions */
  for(auto t: to_execute)
    execute(t);
  
  Cache::process();
  to_execute=next_to_execute;
  
  for(auto it = to_send.begin(); it!= to_send.end(); ) { 
    MemTransaction *t = *it;
    uint64_t dramaddr = (t->addr/size_of_cacheline) * size_of_cacheline;
    if((t->src_id!=-1) && memInterface->willAcceptTransaction(dramaddr,  t->isLoad)) {
      memInterface->addTransaction(t, dramaddr, t->isLoad, cycles);
    // /* REINTRODUCE STATS */
    //   //collect mlp stats
    //   sim->curr_epoch_accesses++;
      it = to_send.erase(it);
    } else if ((t->src_id==-1) && memInterface->willAcceptTransaction(dramaddr, false)) { //forwarded eviction, will be treated as just a write, nothing to do
      memInterface->addTransaction(NULL, dramaddr, false, cycles);
      // /* REINTRODUCE STATS */
      //   //collect mlp stats
      //   sim->curr_epoch_accesses++;
      delete t;        
      it = to_send.erase(it);
    } else {
      it++;
    }
  }
  
  for(auto it = to_evict.begin(); it!= to_evict.end();) {
    uint64_t eAddr = *it;
    if(memInterface->willAcceptTransaction(eAddr, false)) {
      memInterface->addTransaction(NULL, eAddr, false, cycles);
      it = to_evict.erase(it);
    } else {
      ++it;
    }
  }
  
  cycles++;
  
  /* check if the transaction for the accelerator memory blocks has
       been completed */
  for (auto &block:  accelertor_mem) 
    if (block.completed())
      finished_blocks++;
  if (finished_blocks == accelertor_mem.size() && finished_blocks > 0){ 
    accelertor_mem.clear();
    memInterface->LLC_evicted = true;
  }
  
  free_load_ports = load_ports;
  free_store_ports = store_ports;
    
  return (pq.size() > 0);  
}

bool LLCache::execute(MemTransaction* t) {
  bool res;
  if (t->acc_eviction) {
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;
    fc.evict(t->addr, &evictedOffset,  &evictedNodeId); 
    for (auto &block: accelertor_mem) 
      if (block.contains(t->addr) && !block.completed()) {
	block.transaction_complete();
	return true;
      }
    delete t;
    return true;
  }
  
  /* Handle atomic operation's cacheline locks */
  if(t->d) { 
    DynamicNode* d=t->d;
    /* If we have the lock, wait for the acknowledgements */
    if(hasLock(d)) {
      uint64_t cacheline = d->addr / size_of_cacheline;
      int *my_ackns = acknowledgements[cacheline];
      int nb_finished;
#pragma omp atomic read
      nb_finished = my_ackns[0];
      if (nb_finished == nb_cores)  {
	int *entry = acknowledgements[cacheline];
	delete(entry);
	acknowledgements.erase(cacheline);
      } else {
	next_to_execute.push_back(t);
	return false;
      }
    } else { 
      /* Acquire lock for atomic transactions. Don't acquire locks
	 based on accesses spurred by prefetch*/
      if(!t->isPrefetch && d->atomic) { 
	lockCacheline(d);
	/* If we get the lock, we wait for the ackonlegemetns. If we
	   dont get the lock, wait until we get the lock. */
	next_to_execute.push_back(t);
	return false;
	/* Not an atomic operation. Check if the cacheline is locked/ */
      } else {
	bool IsLocked;
	IsLocked = isLocked(d);
	if(IsLocked) { 
	  next_to_execute.push_back(t);
	  return false;
	}
      }
    }
  }
  
  res = Cache::execute(t);

  if (res) {
    /* just normal hit, not an eviction from lower cache */
    if(t->src_id!=-1) { 
      L2Cache* child_cache=t->child_cache;
      vector<MemTransaction *> &to_complete = child_cache->to_complete.get_comm_buff(cycles);
      to_complete.push_back(t);
      /* eviction from lower cache, no need to do anything, since it's a hit, involves no DN */
    } else { 
      stats->update(l3_writebacks);
      delete t; 
    }
    /* it is a miss */
  } else { 
    if (t->src_id!=-1) {
      to_send.push_back(t); //send higher up in hierarchy
    } else { 
      stats->update(l3_writebacks);
      delete t; 
    }
  }
  
  return res;
}

void LLCache::TransactionComplete(MemTransaction *t) { 
  L2Cache* child_cache = t->child_cache;
  vector<MemTransaction *> &to_complete = child_cache->to_complete.get_comm_buff(cycles);

  Cache::TransactionComplete(t);
  to_complete.push_back(t);
}
