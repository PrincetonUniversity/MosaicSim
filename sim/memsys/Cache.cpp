#include "Cache.hpp"

static string cache_evicts="cache_evicts";
static string cache_access="cache_access";
static string evicts="_evicts";
static string evicts_dirty="_evicts_dirty";
static string evicts_clean="_evicts_clean";

static string prefetch_hits="_prefetch_hits";
static string prefetch_misses="_prefetch_misses";
static string prefetches="_prefetches"; // prefetch_hits + prefetch_misses
static string hits="_hits"; // load_hits + store_hits
static string load_hits="_load_hits";
static string loads="_loads"; // load_hits + load_misses
static string stores="_stores"; // store_hits + store_misses
static string store_hits="_store_hits";
static string accesses="_accesses"; // loads + stores
static string misses="_misses"; // load_misses + store_misses
static string load_misses="_load_misses";
static string store_misses="_store_misses";

bool Cache::willAcceptTransaction(MemTransaction *t)  {
  if(!t || t->id==-1)  //eviction or null transaction
    return free_store_ports > 0 || store_ports==-1;
  else if(t->isLoad)
    return free_load_ports > 0 || load_ports==-1;
  else
    return free_store_ports > 0 || store_ports==-1;
}

bool Cache::process() {
  while(pq.size() > 0) {
    if (pq.top().second > cycles)
      break;
    MemTransaction* t = static_cast<MemTransaction*>(pq.top().first);
    execute(t);
    pq.pop();
  }
  return true;
}

bool Cache::execute(MemTransaction* t)   {
  uint64_t dramaddr = t->addr;
  bool res = true;  
  int nodeId = -1;
  
  if(t->src_id!=-1)
    nodeId = t->d->n->id;
  if(!ideal)
    res = fc.access(dramaddr, nodeId, t->isLoad);
  
  if (res) {
    /* just normal hit, not an eviction from lower cache  */
    if(t->src_id!=-1) {
      if(t->isPrefetch) {
	stats->update(type+prefetch_hits);
	stats->update(type+prefetches);
      } else {
	stats->update(type+hits);
	if(t->isLoad) {
	  stats->update(type+load_hits);
	  stats->update(type+loads);
	} else {
	  stats->update(type+store_hits);
	  stats->update(type+stores);
	}
	stats->update(type+accesses);
      }
    }
  /* misses */
  } else {
    if (t->src_id!=-1) {
      if(t->isPrefetch) {
	stats->update(type+prefetch_misses);
	stats->update(type+prefetches);
      } else {
	stats->update(type+misses);
	if(t->isLoad) {
	  stats->update(type+load_misses);
	  stats->update(type+loads);
	} else {
	  stats->update(type+store_misses);
	  stats->update(type+stores);
	}
	stats->update(type+accesses);
      }
    }
  }
  return res;
}
 
void Cache::addTransaction(MemTransaction *t, uint64_t extra_lat) {
  pq.push(make_pair(t, cycles+latency+extra_lat));
  
  stats->update(cache_access);
  if(t->isLoad) {
    free_load_ports--;
    assert(free_load_ports >= 0);
  } else {
    free_store_ports--;
    assert(free_load_ports >= 0);
  }
}

void Cache::TransactionComplete(MemTransaction *t) {
  int nodeId = t->d->n->id;
  int dirtyEvict = -1;
  int64_t evictedAddr = -1;
  uint64_t evictedOffset = 0;
  int evictedNodeId = -1;
  
  fc.insert(t->addr, nodeId, t->isLoad, &dirtyEvict, &evictedAddr, &evictedOffset, &evictedNodeId);
  
  /* we have eveicted an entry in fc */
  if(evictedAddr!=-1) {
    assert(evictedAddr >= 0);
    stats->update(cache_evicts);
    stats->update(type+evicts);
    
    if (dirtyEvict) {
      to_evict.push_back(evictedAddr*size_of_cacheline + evictedOffset);
      stats->update(type+evicts_dirty);
    } else {
      stats->update(type+evicts_clean);
    }
  }
}

