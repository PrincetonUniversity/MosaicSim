#include "../tile/DynamicNode.h"
#include "Cache.h"
#include "DRAM.h"
#include "../tile/Core.h"

string l1_hits = "l1_hits";
string l1_hits_non_prefetch = "l1_hits_non_prefetch";
string l1_misses="l1_misses";
string l1_misses_non_prefetch = "l1_misses_non_prefetch";
string l2_hits="l2_hits";
string l2_misses="l2_misses";
string l2_hits_non_prefetch= "l2_hits_non_prefetch";
string l2_mises="l2_misses";
string l2_misses_non_prefetch="l2_misses_non_prefetch"; 
string cache_evicts="cache_evicts";
string cache_access="cache_access";

void Cache::evict(uint64_t addr) {
  if(fc->evict(addr/size_of_cacheline)) { //evicts from the cache returns isDirty, in which case must write back to L2
    MemTransaction* t = new MemTransaction(-1, -1, -1, addr, false, -1);
    // if(parent_cache->willAcceptTransaction(t)) {
    t->cache_q->push_front(this); 
    parent_cache->addTransaction(t); //send eviction to parent cache
      //}
  }    
}

bool Cache::process() {

  next_to_execute.clear();
  for(auto t:to_execute) {
    execute(t);
  }
  
  while(pq.size() > 0) {
    if(pq.top().second > cycles)
      break;
    MemTransaction* t= static_cast<MemTransaction*>(pq.top().first);

    execute(t);
    
    pq.pop();
  }
  to_execute=next_to_execute;
  
  vector<MemTransaction*> next_to_send;
  
  for(auto it = to_send.begin(); it!= to_send.end();++it) {
    MemTransaction *t = *it;
    uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
   
    if(isLLC) {
      
      if((t->src_id!=-1) && memInterface->willAcceptTransaction(dramaddr,  t->isLoad)) {
        
        //assert(t->isLoad);
        t->cache_q->push_front(this);
              
        memInterface->addTransaction(t, dramaddr, t->isLoad);

        //collect mlp stats
        sim->curr_epoch_accesses++;
        
        //it = to_send.erase(it);        
      }
      else if ((t->src_id==-1) && memInterface->willAcceptTransaction(dramaddr, false)) { //forwarded evict1ion, will be treated as just a write, nothing to do
        memInterface->addTransaction(NULL, dramaddr, false);
        //collect mlp stats
        sim->curr_epoch_accesses++;
        //it = to_send.erase(it);
        delete t;           
      }
      else {
        next_to_send.push_back(t);
        //++it;
      }
    }
    else {
      if(parent_cache->willAcceptTransaction(t)) {
        t->cache_q->push_front(this);
        parent_cache->addTransaction(t);

        //it=to_send.erase(it);
      }
      else {
        next_to_send.push_back(t);
        //++it;
      }
    }
  }

  to_send=next_to_send;

  vector<uint64_t> next_to_evict;
  
  for(auto it = to_evict.begin(); it!= to_evict.end();++it) {
    uint64_t eAddr = *it;
    if(isLLC) {
      if(memInterface->willAcceptTransaction(eAddr, false)) {
        memInterface->addTransaction(NULL, eAddr, false);
        //it = to_evict.erase(it);
      }
      else {
        next_to_evict.push_back(eAddr);
        //++it;
      }
    }
    else {
      MemTransaction* t = new MemTransaction(-1, -1, -1, eAddr, false, -1);
      if(parent_cache->willAcceptTransaction(t)) {
        t->cache_q->push_front(this); 
        parent_cache->addTransaction(t);
        //it = to_evict.erase(it);
      }
      else {
        next_to_evict.push_back(eAddr);
        //++it;
      }
    }
  }
  
  to_evict=next_to_evict;

  
  cycles++;

  //reset mlp stats collection
  if(cycles % sim->mlp_epoch==0 && cycles!=0) {
    sim->accesses_per_epoch.push_back(sim->curr_epoch_accesses);
    sim->curr_epoch_accesses=0;
  }
    
  
  free_load_ports = load_ports;
  free_store_ports = store_ports;
  return (pq.size() > 0);  
}

void Cache::execute(MemTransaction* t) {
  if(isL1 && t->d) { //testing, remove false!!!
    DynamicNode* d=t->d;
    
    if(!t->isPrefetch && d->atomic) { //don't acquire locks based on accesses spurred by prefetch 
      if(!sim->lockCacheline(d)) {
         next_to_execute.push_back(t);
         return;
      }
    }    
    else if(sim->isLocked(d)) {
      next_to_execute.push_back(t);
      return;
    }

  }
  uint64_t dramaddr = t->addr; // /size_of_cacheline * size_of_cacheline;
  bool res = true;  

  
  int nodeId = -1;
  if(t->src_id!=-1) {
    nodeId = t->d->n->id;
  }
  int graphNodeId = -1;
  if (core->graphNodeIdMap.find(dramaddr) != core->graphNodeIdMap.end()) {
    graphNodeId = core->graphNodeIdMap[dramaddr];
  }
  if(!ideal) {
    res = fc->access(dramaddr/size_of_cacheline, nodeId, graphNodeId, t->isLoad);
  }
  cout << "RES = " << res << endl;

  //luwa change, just testing!!!
  //go to dram
  /*
  if(t->src_id!=-1 && t->d->type==LD) {
    res=true;
  }
  else {
    res=false;
  }
  */

  if (res) {

    //d->print("Cache Hit", 1);
    if(t->src_id!=-1) { //just normal hit, not an eviction from lower cache
         
      if(isL1) {

        //don't do anything with prefetch instructions
        /*if(!t->isPrefetch) {
          sim->accessComplete(t);         
          }*/
                
        uint64_t cacheline=t->addr/size_of_cacheline;
        assert(mshr.find(cacheline)!=mshr.end());
        mshr[cacheline].hit=true;
        
        TransactionComplete(t);

      }
      //for l2 cache
      else {
        int nodeId = t->d->n->id;
        int graphNodeId = -1;
        if (core->graphNodeIdMap.find(dramaddr) != core->graphNodeIdMap.end()) {
          graphNodeId = core->graphNodeIdMap[dramaddr];
        }
        int dirtyEvict = -1;
        int64_t evictedAddr = -1;
        int evictedNodeId = -1;
        int evictedGraphNodeId = -1;
        Cache* child_cache=t->cache_q->front();
        t->cache_q->pop_front();
        
        child_cache->fc->insert(dramaddr/child_cache->size_of_cacheline, nodeId, graphNodeId, &dirtyEvict, &evictedAddr, &evictedNodeId, &evictedGraphNodeId); 

        if(evictedAddr != -1) {
          
          assert(evictedAddr >= 0);
          stat.update(cache_evicts);
          
          if (dirtyEvict) {
            child_cache->to_evict.push_back(evictedAddr*child_cache->size_of_cacheline);
          }
        }
        child_cache->TransactionComplete(t);
        stat.update(l2_hits);
        if(!t->isPrefetch) {
          stat.update(l2_hits_non_prefetch);
        }

      }
          
      
    }
    else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      delete t; 
    }

  } //misses
  else {
    if(isL1) {
      //do nothing, MSHR entry is set to miss by default   
    }
    else {
      stat.update(l2_misses);
      if(!t->isPrefetch) {
        stat.update(l2_misses_non_prefetch);
      }
    }
 
    to_send.push_back(t); //send higher up in hierarchy
    //d->print(Cache Miss, 1);
    //if(!t->isPrefetch) {
    
    //}
  }

}

void Cache::addTransaction(MemTransaction *t) {
  if(isL1 && t->src_id!=-1) { //not eviction
    uint64_t cacheline = t->addr/size_of_cacheline;
    if(mshr.find(cacheline)==mshr.end()) {
      mshr[cacheline]=MSHR_entry();
      pq.push(make_pair(t, cycles+latency));
      //add transaction only if it's the 1st
    }
    mshr[cacheline].insert(t); 
  }
  else {
     pq.push(make_pair(t, cycles+latency));
  }
  stat.update(cache_access);
  if(t->isLoad)
    free_load_ports--;
  else
    free_store_ports--;

  //for prefetching, don't issue prefetch for evict or for access with outstanding prefetches or for access that IS  prefetch or eviction
  if(isL1 && t->src_id>=0 && num_prefetched_lines>0) {
    //int cache_line = t->d->addr/size_of_cacheline;
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
          MemTransaction* prefetch_t = new MemTransaction(-2, -2, -2, t->addr + size_of_cacheline*(prefetch_distance+i), true, -1); //prefetch some distance ahead
          prefetch_t->d=t->d;
          prefetch_t->isPrefetch=true;
          //pq.push(make_pair(prefetch_t, cycles+latency));
          this->addTransaction(prefetch_t);
        }
        
        if(t->d->type==LD_PROD) {
          //cout << "PREFETCHING LD_PROD addr: " << t->d->addr << endl;
        }
        
        prefetch_set.insert(t->d->addr);
        prefetch_set.erase(t->d->addr-stride*pattern_threshold); 
       
      }
      else { //keep prefetch set size capped
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

bool Cache::willAcceptTransaction(MemTransaction *t) {  
  if(!t || t->id==-1) { //eviction or null transaction
    return free_store_ports > 0 || store_ports==-1;
  }
  else if(t->isLoad) {
    return free_load_ports > 0 || load_ports==-1;
  }
  else {
    return free_store_ports > 0 || store_ports==-1;
  }
}

void Cache::TransactionComplete(MemTransaction *t) { 
  
  if(isL1) {
    
    uint64_t cacheline=t->addr/size_of_cacheline;

    //should be part of an mshr entry
    if(mshr.find(cacheline)==mshr.end()) {
      assert(false);
    }
    MSHR_entry mshr_entry=mshr[cacheline];
    
    auto trans_set=mshr_entry.opset;
    int batch_size=trans_set.size();

    int non_prefetch_size=mshr_entry.non_prefetch_size;
    
    //update hit/miss stats, account for each individual access

    if(mshr_entry.hit) {
      stat.update(l1_hits,batch_size);
      core->local_stat.update(l1_hits,batch_size);
      stat.update(l1_hits_non_prefetch,non_prefetch_size);
      core->local_stat.update(l1_hits_non_prefetch,non_prefetch_size);
    }
    else {
      stat.update(l1_misses,batch_size);
      core->local_stat.update(l1_misses,batch_size);
      stat.update(l1_misses_non_prefetch,non_prefetch_size);
      core->local_stat.update(l1_misses_non_prefetch,non_prefetch_size);
    }
    
    //process callback for each individual transaction in batch
    for (auto it=trans_set.begin(); it!=trans_set.end(); ++it) {
      MemTransaction* curr_t=*it;
      if(!curr_t->isPrefetch) {

        //record statistics on non-prefetch loads/stores
        if(core->sim->debug_mode || core->sim->mem_stats_mode) {
          DynamicNode* d=curr_t->d;
          assert(d!=NULL);
          assert(core->sim->load_stats_map.find(d)!=core->sim->load_stats_map.end());
          get<2>(core->sim->load_stats_map[d])=mshr_entry.hit;
          //get<2>(entry_tuple)=mshr_entry.hit;
                    
        }
                
        sim->accessComplete(curr_t);
      }
      else { //prefetches get no callback, tied to no dynamic node
        delete curr_t;
      }   
    }
    mshr.erase(cacheline); //clear mshr for that cacheline
  }
  else {
    int nodeId = t->d->n->id;
    int graphNodeId = -1;
    if (core->graphNodeIdMap.find(t->addr) != core->graphNodeIdMap.end()) {
      graphNodeId = core->graphNodeIdMap[t->addr];
    }
    int dirtyEvict = -1;
    int64_t evictedAddr = -1;
    int evictedNodeId = -1;
    int evictedGraphNodeId = -1;

    Cache* c = t->cache_q->front();
    
    t->cache_q->pop_front();    
    c->fc->insert(t->addr/c->size_of_cacheline, nodeId, graphNodeId, &dirtyEvict, &evictedAddr, &evictedNodeId, &evictedGraphNodeId);

    if(evictedAddr!=-1) {
      assert(evictedAddr >= 0);
      stat.update(cache_evicts);

      if (dirtyEvict) {
        c->to_evict.push_back(evictedAddr*c->size_of_cacheline);
      }
    }
    c->TransactionComplete(t);      
  }
}
