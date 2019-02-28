#include "DRAM.h"
#include "Cache.h"
#include "../core/Core.h"
using namespace std;
#define UNUSED 0
/*
void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());

  if(UNUSED)
    cout << id << clock_cycle;
  queue<Transaction*> &q = outstanding_read_map.at(addr);

  set<Cache*> coreIDSet;
  vector<Transaction*> transVec;
  
  while(q.size() > 0) {
    Transaction* t = q.front();    
        
    //gather all transactions from unique caches
    if(cacheSet.find(t->cache)==cacheSet.end()) {
      Transaction *nt = new Transaction(t->id, t->coreId, t->addr, t->isLoad);
      transVec.push_back(nt);
      cacheSet.insert(nt->coreId);
    }
    sim->TransactionComplete(t);
    q.pop();
  }
  sim->InsertCaches(transVec);
  if(q.size() == 0)
    outstanding_read_map.erase(addr);
}
*/
uint64_t stored_addr=-1;

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {

  //assert(addr!=140727892711936/64 * 64);
  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());

  //assert(addr!=140727892711936);
  //assert(addr!=140727892711952);
  if(UNUSED)
    cout << id << clock_cycle;
  queue<Transaction*> &q = outstanding_read_map.at(addr);
  while(q.size() > 0) {
    
    Transaction* t = q.front();
    DynamicNode *d = t->d;
   
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
 
  assert(addr!=140727892711936);
  if(UNUSED)
    cout << id << addr << clock_cycle;
}
void DRAMSimInterface::addTransaction(Transaction* t, uint64_t addr, bool isLoad) {

  
  //    assert(addr!=140727892711936);
  //assert(addr!=140727892711936);
  
  if(isLoad) 
    free_load_ports--;
  else
    free_store_ports--;
  if(t!= NULL) {
    //    assert(t->addr==addr);
    //assert(t->addr!=140727892711952);
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<Transaction*>()));
      mem->addTransaction(false, addr);
      stat.update("dram_access");     
    }
    outstanding_read_map.at(addr).push(t);
    /*
     if(t!=NULL && t->id!=-1) {
      DynamicNode *d = t->d;
     
      if(d->c->id==35682 && d->n->id==14 && d->core->id==0) {
        
        assert(t->addr==addr);
        assert(d->addr==addr);
        cout << addr << " is addr \n";
        stored_addr=addr;

         queue<Transaction*> &q = outstanding_read_map.at(addr);
         while(q.size() > 0) {
           
           Transaction* nt = q.front();
           assert(nt!=t);
           q.pop();
           
         }
      }
  }
    */   
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
  if(!mem->willAcceptTransaction(addr))
    return false;
  return true;
}
