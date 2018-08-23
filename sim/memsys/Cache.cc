#include "Cache.h"
#include "DRAM.h"
bool Cache::process_cache() {
  while(pq.size() > 0) {
    if(pq.top().second > cycles)
      break;    
    execute(pq.top().first);
    pq.pop();
  }
  for(auto it = to_send.begin(); it!= to_send.end();) {
    DynamicNode *d = *it;
    uint64_t dramaddr = d->addr/size_of_cacheline * size_of_cacheline;
    if(memInterface->willAcceptTransaction(d,dramaddr)) {
      memInterface->addTransaction(d, dramaddr, true);
      it = to_send.erase(it);
    }
    else
      it++;
  }
  for(auto it = to_evict.begin(); it!= to_evict.end();) {
    uint64_t eAddr = *it;
    if(memInterface->mem->willAcceptTransaction(eAddr)) {
      memInterface->addTransaction(NULL, eAddr, false);
      it = to_evict.erase(it);
    }
    else
      it++;
  }
  cycles++;
  ports[0] = cfg.cache_load_ports;
  ports[1] = cfg.cache_store_ports;
  ports[2] = cfg.mem_load_ports;
  ports[3] = cfg.mem_store_ports;
  return (pq.size() > 0);  
}
void Cache::execute(DynamicNode* d) {
  uint64_t dramaddr = d->addr/size_of_cacheline * size_of_cacheline;
  bool res = true;
  if(!ideal)
    res = fc->access(dramaddr/size_of_cacheline, d->type==LD);
  if (res) {                
    d->print("Cache Hit", 1);
    d->handleMemoryReturn();
    stat.update("cache_hit");
  }
  else {
    to_send.push_back(d);
    d->print("Cache Miss", 1);
    stat.update("cache_miss");
  }
}
void Cache::addTransaction(DynamicNode *d) {
  d->print("Cache Transaction Added", 1);
  stat.update("cache_access");
  pq.push(make_pair(d, cycles+latency));
}   
