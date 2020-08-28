#include "DRAM.h"
#include "Cache.h"
#include "../tile/Core.h"
using namespace std;
#define UNUSED 0

string dram_accesses="dram_accesses";
string dram_reads_loads="dram_reads_loads";
string dram_reads_stores="dram_reads_stores";
string dram_writes_evictions="dram_writes_evictions";
string dram_bytes_accessed="dram_bytes_accessed";
string dram_total_read_latency="dram_total_read_latency";
string dram_total_write_latency="dram_total_write_latency";

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {

  //assert(outstanding_read_map.find(addr) != outstanding_read_map.end());
  if (outstanding_read_map.find(addr) == outstanding_read_map.end()) {
    return;
  }

  if(UNUSED)
    cout << id << clock_cycle;

  queue<pair<Transaction*, uint64_t>> &q = outstanding_read_map.at(addr);

  while(q.size() > 0) {

    pair<Transaction*, uint64_t> entry = q.front();

    stat.update(dram_total_read_latency, clock_cycle-entry.second);

    MemTransaction* t = static_cast<MemTransaction*>(entry.first);
    int nodeId = t->d->n->id;
    int dirtyEvict = -1;
    int64_t evictedAddr = -1;
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;

    Cache* c = t->cache_q->front();
    
    assert(c->isLLC);
    t->cache_q->pop_front();
    c->fc->insert(t->addr, nodeId, t->isLoad, &dirtyEvict, &evictedAddr, &evictedOffset, &evictedNodeId);
    if(evictedAddr!=-1) {

      assert(evictedAddr >= 0);
      stat.update("cache_evicts");
      if (c->useL2) {
        stat.update("l3_evicts");
      } else {
        stat.update("l2_evicts");
      }

      if (dirtyEvict) {
        c->to_evict.push_back(make_tuple(evictedAddr*c->size_of_cacheline + evictedOffset));
      
        if (c->useL2) {
          stat.update("l3_evicts_dirty");
        } else {
          stat.update("l2_evicts_dirty");
        }
      } else {
        if (c->useL2) {
          stat.update("l3_evicts_clean");
        } else {
          stat.update("l2_evicts_clean");
        }
      }
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

  queue<pair<Transaction*, uint64_t>> &q = outstanding_write_map.at(addr);

  pair<Transaction*, uint64_t> entry = q.front();
  stat.update(dram_total_write_latency, clock_cycle-entry.second);
  q.pop();

  if(q.size() == 0)
    outstanding_write_map.erase(addr);
}

void DRAMSimInterface::addTransaction(Transaction* t, uint64_t addr, bool isRead, int cacheline_size, uint64_t issueCycle) {
  if(t!=NULL) { // this is a LD or a ST (whereas a NULL transaction means it is really an eviction)
                // Note that caches are write-back ones, so STs also "read" from dram 
    free_read_ports--;

    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<pair<Transaction*, uint64_t>>()));      
    }

    if(cfg.SimpleDRAM) {       
      simpleDRAM->addTransaction(false,addr);
    } else {
      mem->addTransaction(false, addr);
    }
    
    outstanding_read_map.at(addr).push(make_pair(t, issueCycle));
      
    if(isRead) {
      stat.update(dram_reads_loads);
    } else {
      stat.update(dram_reads_stores);
    }
  } else { //eviction -> write iinto DRAM
    free_write_ports--;

    if(outstanding_write_map.find(addr) == outstanding_write_map.end()) {
      outstanding_write_map.insert(make_pair(addr, queue<pair<Transaction*, uint64_t>>()));
    }

    if(cfg.SimpleDRAM) {
      simpleDRAM->addTransaction(true, addr);
    } else {
      mem->addTransaction(true, addr);
    }
    outstanding_write_map.at(addr).push(make_pair(t, issueCycle));
    stat.update(dram_writes_evictions);
  }
  stat.update(dram_accesses);
  stat.update(dram_bytes_accessed,cacheline_size);
}

bool DRAMSimInterface::willAcceptTransaction(uint64_t addr, bool isRead) {
  if((free_read_ports == 0 && isRead && read_ports!=-1)  || (free_write_ports == 0 && !isRead && write_ports!=-1))
    return false;
  else if(cfg.SimpleDRAM)
    return simpleDRAM->willAcceptTransaction(addr); 
  else
    return mem->willAcceptTransaction(addr);
}
