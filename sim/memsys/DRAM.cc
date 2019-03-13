#include "DRAM.h"
#include "Cache.h"
#include "../tile/Core.h"
using namespace std;
#define UNUSED 0

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
 
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());

  
  if(UNUSED)
    cout << id << clock_cycle;
  queue<Transaction*> &q = outstanding_read_map.at(addr);
  while(q.size() > 0) {
    
    MemTransaction* t = static_cast<MemTransaction*>(q.front());
   
    int64_t evictedAddr = -1;
    Cache* c = t->cache_q->front();
    
    assert(c->isLLC);
    t->cache_q->pop_front();
    
    c->fc->insert(addr/64, &evictedAddr);
    if(evictedAddr!=-1) {

      assert(evictedAddr >= 0);
      stat.update("cache_evict");
      c->to_evict.push_back(evictedAddr*64);
    }
    
    c->TransactionComplete(t);
    q.pop();
        
  }
  if(q.size() == 0)
    outstanding_read_map.erase(addr);
}

void DRAMSimInterface::write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
 
  if(UNUSED)
    cout << id << addr << clock_cycle;
}
void DRAMSimInterface::addTransaction(Transaction* t, uint64_t addr, bool isLoad) {
  
  if(isLoad) 
    free_load_ports--;
  else
    free_store_ports--;
  if(t!= NULL) {
     if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<Transaction*>()));
      mem->addTransaction(false, addr);
      stat.update("dram_access");     
    }
    outstanding_read_map.at(addr).push(t);
 
  }
  else { //write
    
    assert(isLoad == false);
    mem->addTransaction(true, addr);
    stat.update("dram_access");
  }
}
bool DRAMSimInterface::willAcceptTransaction(uint64_t addr, bool isLoad) {
  if((free_load_ports == 0 && isLoad && load_ports!=-1)  || (free_store_ports == 0 && !isLoad && store_ports!=-1))
    return false;
  
  return mem->willAcceptTransaction(addr);
}
