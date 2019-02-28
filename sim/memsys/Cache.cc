#include "../core/DynamicNode.h"
#include "Cache.h"
#include "DRAM.h"
#include "../core/Core.h"
bool Cache::process() {


  while(pq.size() > 0) {
    if(pq.top().second > cycles)
      break;
    Transaction* t= pq.top().first;
    /*if(t->id != -1) {
      
      DynamicNode *d = t->d;
      assert(!(d->c->id==12 && d->n->id==4 && d->core->id==0));
      
    }*/
    execute(t);
    
    pq.pop();
  }

  for(auto it = to_send.begin(); it!= to_send.end();) {
    Transaction *t = *it;
    uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
   
    if(isLLC) {
      
      if((t->coreId!=-1) && memInterface->willAcceptTransaction(dramaddr,  t->isLoad)) {

        if(t->id!=-1) {
          DynamicNode *d = t->d;
          if((d->c->id==35683 && d->n->id==14 && d->core->id==0)) {
            cout << t->addr << " weird addr"<< endl;
          }
        }
        
        
        //assert(t->isLoad);
        t->cache_q->push_front(this);
        
        
        DynamicNode *d = t->d;
        if(d->c->id==35682 && d->n->id==14 && d->core->id==0) {
          assert(t->isLoad);
          cout << t->addr << " is address \n";
          
        }
        /*
        if(t->addr!=dramaddr)
          {
            cout << "t->addr: " << t->addr << "\n" << "dramaddr: " << dramaddr << endl;
          }
        */
          //apparently this fails!!
        memInterface->addTransaction(t, dramaddr, t->isLoad);
        it = to_send.erase(it);        
      }
      else if ((t->coreId==-1) && memInterface->willAcceptTransaction(dramaddr, false)) { //forwarded eviction, will be treated as just a write, nothing to do
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
      Transaction* t = new Transaction(-1, -1, eAddr, false);
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
void Cache::execute(Transaction* t) {
  uint64_t dramaddr = t->addr; // /size_of_cacheline * size_of_cacheline;
  bool res = true;  
 
  if(!ideal) {
    res = fc->access(dramaddr/size_of_cacheline, t->isLoad);

  }
  if (res) {                
    //d->print("Cache Hit", 1);
    if(t->coreId!=-1) { //just normal hit, not an eviction from lower cache      
      if(isL1) {
        
        sim->accessComplete(t); 
        stat.update("cache_hit"); //luwa todo l1 hit
      }
      else {

        int64_t evictedAddr = -1;

        Cache* child_cache=t->cache_q->front();

 
        t->cache_q->pop_front(); 

         
        child_cache->fc->insert(dramaddr/size_of_cacheline, &evictedAddr); 
        
        if(evictedAddr!=-1) {
          
          assert(evictedAddr >= 0);
          stat.update("cache_evict");

          child_cache->to_evict.push_back(evictedAddr*size_of_cacheline);
        }
        child_cache->TransactionComplete(t);
        stat.update("cache_hit"); //luwa todo l2 hit
      }

    }
    else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      assert(false);
      delete t; 
    }
     
  }
  else {

    to_send.push_back(t); //send higher up in hierarchy
    //d->print("Cache Miss", 1);
    
    stat.update("cache_miss");
  }
}
void Cache::addTransaction(Transaction *t) {
  //d->print("Cache Transaction Added", 1);
  
  stat.update("cache_access");
  if(t->isLoad)
    free_load_ports--;
  else
    free_store_ports--;  
  pq.push(make_pair(t, cycles+latency));
}

bool Cache::willAcceptTransaction(Transaction *t) {  
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

void Cache::TransactionComplete(Transaction *t) {
  
  if(isL1) {
    sim->accessComplete(t);
  }
  else {

    int64_t evictedAddr = -1;
    Cache* c = t->cache_q->front();
    assert(isLLC);
    t->cache_q->pop_front();
    //luwa todo: change 64 to cacheline size
    c->fc->insert(t->addr/c->size_of_cacheline, &evictedAddr);
    if(evictedAddr!=-1) {
      assert(evictedAddr >= 0);
      stat.update("cache_evict");
      c->to_evict.push_back(evictedAddr*c->size_of_cacheline);
    }
    c->TransactionComplete(t);      
  }
  //  else {    
  //  child_cache->TransactionComplete(t);
  //}
}
