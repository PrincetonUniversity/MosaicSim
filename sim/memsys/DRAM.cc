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

void DRAMSimInterface::read_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {

  assert(outstanding_read_map.find(addr) != outstanding_read_map.end());

  if(UNUSED)
    cout << id << clock_cycle;

  queue<Transaction*> &q = outstanding_read_map.at(addr);

  while(q.size() > 0) {
    
    MemTransaction* t = static_cast<MemTransaction*>(q.front());
    int nodeId = t->d->n->id;
    int graphNodeId = t->graphNodeId;
    int graphNodeDeg = t->graphNodeDeg;
    int dirtyEvict = -1;
    int64_t evictedAddr = -1;
    uint64_t evictedOffset = 0;
    int evictedNodeId = -1;
    int evictedGraphNodeId = -1;
    int evictedGraphNodeDeg = -1;
    int unusedSpace = 0;

    Cache* c = t->cache_q->front();
    
    assert(c->isLLC);
    t->cache_q->pop_front();
    c->fc->insert(t->addr, nodeId, graphNodeId, graphNodeDeg, &dirtyEvict, &evictedAddr, &evictedOffset, &evictedNodeId, &evictedGraphNodeId, &evictedGraphNodeDeg, &unusedSpace);
    if(evictedAddr!=-1) {

      int cacheline_size;
      if (c->cacheBySignature == 0 || evictedGraphNodeDeg == -1) {
        cacheline_size = c->size_of_cacheline;
      } else {
        cacheline_size = 4;
      }

      assert(evictedAddr >= 0);
      stat.update("cache_evicts");
      c->to_evict.push_back(make_tuple(evictedAddr*c->size_of_cacheline, evictedGraphNodeId, evictedGraphNodeDeg, cacheline_size));
      if (c->useL2) {
        stat.update("l3_evicts");
      } else {
        stat.update("l2_evicts");
      }

      if (sim->recordEvictions) {    
        cacheStat cache_stat;
        cache_stat.cacheline = t->addr/c->size_of_cacheline*c->size_of_cacheline;
        cache_stat.cycle = c->cycles;
        cache_stat.offset = evictedOffset;
        cache_stat.nodeId = evictedNodeId;
        cache_stat.graphNodeId = evictedGraphNodeId;
        cache_stat.graphNodeDeg = evictedGraphNodeDeg;
        cache_stat.unusedSpace = unusedSpace;
        cache_stat.cacheLevel = 3;
        sim->evictStatsVec.push_back(cache_stat); 
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
}

void DRAMSimInterface::addTransaction(Transaction* t, uint64_t addr, bool isRead, int cacheline_size) {
  if(t!=NULL) { // this is a LD or a ST (whereas a NULL transaction means it is really an eviction)
                // Note that caches are write-back ones, so STs also "read" from dram 
    free_read_ports--;
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<Transaction*>()));
      if(cfg.SimpleDRAM) {
        simpleDRAM->addTransaction(false,addr);
      }
      else {
        mem->addTransaction(false, addr);
      }
    }
    outstanding_read_map.at(addr).push(t);
    if (isRead) {
      stat.update(dram_reads_loads);
    } else {
      stat.update(dram_reads_stores);
    }
  }
  else { // this is an eviction -> it writes into dram
    free_write_ports--;
    if(cfg.SimpleDRAM) {
      simpleDRAM->addTransaction(true, addr);
    }
    else {
      mem->addTransaction(true, addr);
    }
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
