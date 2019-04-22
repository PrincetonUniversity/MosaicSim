#include "../tile/DynamicNode.h"
#include "Cache.h"
#include "DRAM.h"
#include "../tile/Core.h"
bool Cache::process() {


  while(pq.size() > 0) {
    if(pq.top().second > cycles)
      break;
    MemTransaction* t= static_cast<MemTransaction*>(pq.top().first);

    execute(t);
    
    pq.pop();
  }

  for(auto it = to_send.begin(); it!= to_send.end();) {
    MemTransaction *t = *it;
    uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
   
    if(isLLC) {
      
      if((t->src_id!=-1) && memInterface->willAcceptTransaction(dramaddr,  t->isLoad)) {
        
        //assert(t->isLoad);
        t->cache_q->push_front(this);
              
        memInterface->addTransaction(t, dramaddr, t->isLoad);
        it = to_send.erase(it);        
      }
      else if ((t->src_id==-1) && memInterface->willAcceptTransaction(dramaddr, false)) { //forwarded eviction, will be treated as just a write, nothing to do
        memInterface->addTransaction(NULL, dramaddr, false);
        it = to_send.erase(it);
        delete t;           
      }
      else
        ++it;
    }
  
    else {

      if(parent_cache->willAcceptTransaction(t)) {
        
        t->cache_q->push_front(this);
        
        parent_cache->addTransaction(t);
        it=to_send.erase(it);
      }
      else
        ++it;
    }
  }
  
  for(auto it = to_evict.begin(); it!= to_evict.end();) {
    uint64_t eAddr = *it;
    if(isLLC) {
      if(memInterface->willAcceptTransaction(eAddr, false)) {
        memInterface->addTransaction(NULL, eAddr, false);
        it = to_evict.erase(it);
      }
      else
        ++it;
    }
    else {
      MemTransaction* t = new MemTransaction(-1, -1, -1, eAddr, false);
      if(parent_cache->willAcceptTransaction(t)) {
        
        
        t->cache_q->push_front(this); 
        parent_cache->addTransaction(t);
        it = to_evict.erase(it);
      }
      else
        ++it;
    }
  }
  cycles++;
  free_load_ports = load_ports;
  free_store_ports = store_ports;
  return (pq.size() > 0);  
}
void Cache::execute(MemTransaction* t) {
  uint64_t dramaddr = t->addr; // /size_of_cacheline * size_of_cacheline;
  bool res = true;  
 
  if(!ideal) {
    res = fc->access(dramaddr/size_of_cacheline, t->isLoad);
  }
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
      DynamicNode* d = t->d;
      if(isL1) {
        //enter into load stats map
        
        if(!t->isPrefetch && (d->type==LD || d->type==LD_PROD)) {
          assert(d->core->sim->load_stats_map.find(d)!=d->core->sim->load_stats_map.end());
          auto& entry_tuple = d->core->sim->load_stats_map[d];
          get<2>(entry_tuple)=true;
        }

        //don't do anything with prefetch instructions
        if(!t->isPrefetch) {
          sim->accessComplete(t);
          stat.update("cache_hit"); //luwa todo l1 hit
        }
      }
      else {
        
        int64_t evictedAddr = -1;

        Cache* child_cache=t->cache_q->front();

 
        t->cache_q->pop_front(); 

         
        child_cache->fc->insert(dramaddr/child_cache->size_of_cacheline, &evictedAddr); 
        
        if(evictedAddr!=-1) {
          
          assert(evictedAddr >= 0);
          stat.update("cache_evict");

          child_cache->to_evict.push_back(evictedAddr*child_cache->size_of_cacheline);
        }
        child_cache->TransactionComplete(t);
        //stat.update("cache_hit"); //luwa todo l2 hit
      }

    }
    else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      
      delete t; 
    }
     
  }
  else {

    to_send.push_back(t); //send higher up in hierarchy
    //d->print("Cache Miss", 1);
    if(!t->isPrefetch) {
      stat.update("cache_miss");
    }
  }
}
void Cache::addTransaction(MemTransaction *t) {

  //inf BW
  //luwa change this!!! Testing!!!
  
  //if(t->src_id!=-1 && (t->d->type==LD))  
    //if(t->src_id!=-1 && t->d->type==LD && t->d->n->id==8) 
    //pq.push(make_pair(t, cycles+1)); 
  //else  
  pq.push(make_pair(t, cycles+latency));
  
  stat.update("cache_access");
  if(t->isLoad)
    free_load_ports--;
  else
    free_store_ports--;

  //for prefetching, don't issue prefetch for evict or for access with outstanding prefetches or for access that IS  prefetch
  if(isL1 && t->src_id!=-1 && !t->issuedPrefetch && !t->isPrefetch) {
    for(int i=0; i<prefetch_distance; i++) {
      MemTransaction* prefetch_t = new MemTransaction(-2, -2, -2, t->addr+(i+1)*size_of_cacheline, true);
      prefetch_t->d=t->d;
      prefetch_t->isPrefetch=true;
      pq.push(make_pair(prefetch_t, cycles+latency));      
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
    if(!t->isPrefetch) {
      sim->accessComplete(t);
    }
    else{
      delete t;
    }
  }
  else {

    int64_t evictedAddr = -1;
    Cache* c = t->cache_q->front();
    
    t->cache_q->pop_front();    
    c->fc->insert(t->addr/c->size_of_cacheline, &evictedAddr);
    if(evictedAddr!=-1) {
      assert(evictedAddr >= 0);
      stat.update("cache_evict");
      c->to_evict.push_back(evictedAddr*c->size_of_cacheline);
    }
    c->TransactionComplete(t);      
  }
}
