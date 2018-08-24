#include "../core/DynamicNode.h"
#include "Cache.h"
#include "DRAM.h"
bool Cache::process() {
  while(pq.size() > 0) {
    if(pq.top().second > cycles)
      break;    
    execute(pq.top().first);
    pq.pop();
  }
  for(auto it = to_send.begin(); it!= to_send.end();) {
    Transaction *t = *it;
    uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
    if(memInterface->willAcceptTransaction(dramaddr, true)) {
      memInterface->addTransaction(t, dramaddr, true);
      it = to_send.erase(it);
    }
    else
      it++;
  }
  for(auto it = to_evict.begin(); it!= to_evict.end();) {
    uint64_t eAddr = *it;
    if(memInterface->willAcceptTransaction(eAddr, false)) {
      memInterface->addTransaction(NULL, eAddr, false);
      it = to_evict.erase(it);
    }
    else
      it++;
  }
  cycles++;
  free_load_ports = load_ports;
  free_store_ports = store_ports;
  return (pq.size() > 0);  
}
void Cache::execute(Transaction* t) {
  uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
  bool res = true;
  if(!ideal)
    res = fc->access(dramaddr/size_of_cacheline, t->isLoad);
  if (res) {                
    //d->print("Cache Hit", 1);
    sim->accessComplete(t);
    stat.update("cache_hit");
  }
  else {
    to_send.push_back(t);
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
void Cache::TransactionComplete(Transaction *t) {
  sim->accessComplete(t);
}