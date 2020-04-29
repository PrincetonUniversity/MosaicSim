#include "../tile/DynamicNode.h"
#include "Cache.h"
#include "DRAM.h"
#include "../tile/Core.h"

string l1_accesses="l1_accesses"; // loads + stores
string l1_hits="l1_hits"; // load_hits + store_hits
string l1_misses="l1_misses"; // load_misses + store_misses
string l1_primary_misses="l1_misses_primary"; // primary_load_misses + primary_store_misses
string l1_secondary_misses="l1_misses_secondary"; // secondary_load_misses + secondary_store_misses
string l1_loads="l1_loads"; // load_hits + load_misses
string l1_load_hits="l1_load_hits";
string l1_load_misses="l1_load_misses";
string l1_primary_load_misses="l1_load_misses_primary";
string l1_secondary_load_misses="l1_load_misses_secondary";
string l1_stores="l1_stores"; // store_hits + store_misses
string l1_store_hits="l1_store_hits";
string l1_store_misses="l1_store_misses";
string l1_primary_store_misses="l1_store_misses_primary";
string l1_secondary_store_misses="l1_store_misses_secondary";
string l1_prefetches="l1_prefetches"; // prefetch_hits + prefetch_misses
string l1_prefetch_hits="l1_prefetch_hits";
string l1_prefetch_misses="l1_prefetch_misses";
string l1_total_accesses="l1_total_accesses"; // l1_accesses + l1_prefetches
string l1_dirty_evicts="l1_evicts_dirty";
string l1_clean_evicts="l1_evicts_clean";
string l1_evicts="l1_evicts";

string l2_accesses="l2_accesses"; // loads + stores
string l2_hits="l2_hits"; // load_hits + store_hits
string l2_misses="l2_misses"; // load_misses + store_misses
string l2_loads="l2_loads"; // load_hits + load_misses
string l2_load_hits="l2_load_hits";
string l2_load_misses="l2_load_misses";
string l2_stores="l2_stores"; // store_hits + store_misses
string l2_store_hits="l2_store_hits";
string l2_store_misses="l2_store_misses";
string l2_prefetches="l2_prefetches"; // prefetch_hits + prefetch_misses
string l2_prefetch_hits="l2_prefetch_hits";
string l2_prefetch_misses="l2_prefetch_misses";
string l2_total_accesses="l2_total_accesses"; // l2_accesses + l2_prefetches
string l2_dirty_evicts="l2_evicts_dirty";
string l2_clean_evicts="l2_evicts_clean";
string l2_evicts="l2_evicts";
string l2_writebacks="l2_writebacks"; 

string l3_accesses="l3_accesses"; // loads + stores
string l3_hits="l3_hits"; // load_hits + store_hits
string l3_misses="l3_misses"; // load_misses + store_misses
string l3_loads="l3_loads"; // load_hits + load_misses
string l3_load_hits="l3_load_hits";
string l3_load_misses="l3_load_misses";
string l3_stores="l3_stores"; // store_hits + store_misses
string l3_store_hits="l3_store_hits";
string l3_store_misses="l3_store_misses";
string l3_prefetches="l3_prefetches"; // prefetch_hits + prefetch_misses
string l3_prefetch_hits="l3_prefetch_hits";
string l3_prefetch_misses="l3_prefetch_misses";
string l3_total_accesses="l3_total_accesses"; // l3_accesses + l3_prefetches
string l3_dirty_evicts="l3_evicts_dirty";
string l3_clean_evicts="l3_evicts_clean";
string l3_evicts="l3_evicts";
string l3_writebacks="l3_writebacks"; 

string cache_evicts="cache_evicts";
string cache_access="cache_access";

void Cache::evict(uint64_t addr) {
  uint64_t evictedOffset = 0;
  int evictedNodeId = -1;
  int evictedGraphNodeId = -1;
  int evictedGraphNodeDeg = -1;
  int unusedSpace = 0;

  if(fc->evict(addr, &evictedOffset, &evictedNodeId, &evictedGraphNodeId, &evictedGraphNodeDeg, &unusedSpace)) { //evicts from the cache returns isDirty, in which case must write back to L2
    MemTransaction* t = new MemTransaction(-1, -1, -1, addr, false, evictedGraphNodeId, evictedGraphNodeDeg);
    // if(parent_cache->willAcceptTransaction(t)) {
    t->cache_q->push_front(this); 
    parent_cache->addTransaction(t); //send eviction to parent cache
      //}
  }    

  if (sim->recordEvictions) {         
    cacheStat cache_stat;
    cache_stat.cacheline = addr/size_of_cacheline*size_of_cacheline;
    cache_stat.cycle = cycles;
    cache_stat.offset = evictedOffset;
    cache_stat.nodeId = evictedNodeId;
    cache_stat.graphNodeId = evictedGraphNodeId;
    cache_stat.graphNodeDeg = evictedGraphNodeDeg;
    cache_stat.unusedSpace = unusedSpace;
    if (isL1) {
      cache_stat.cacheLevel = 1;
    } else {
      cache_stat.cacheLevel = 2;
    }
    sim->evictStatsVec.push_back(cache_stat);  
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
  
  for(auto it = to_send.begin(); it!= to_send.end(); ++it) { 
    MemTransaction *t = *it;

    if (isL1 && t->src_id!=-1 && t->checkMSHR) {
      uint64_t cacheline = t->addr/size_of_cacheline;
      bool mshrMiss = mshr.find(cacheline)==mshr.end();
      if(mshrMiss && num_mshr_entries < mshr_size) {  // not present in MSHRs and there is space for 1 more entry
        if (size > 0 || ideal) {
          if (t->isLoad) {
            stat.update(l1_primary_load_misses);
            core->local_stat.update(l1_primary_load_misses);
          } else {
            stat.update(l1_primary_store_misses);
            core->local_stat.update(l1_primary_store_misses);
          }
          stat.update(l1_primary_misses);
          core->local_stat.update(l1_primary_misses);
          mshr[cacheline]=MSHR_entry();
          mshr[cacheline].insert(t);
          mshr[cacheline].hit=0; 
          num_mshr_entries++;
          t->checkMSHR=false;
        }
      } else if (!mshrMiss) {   // it is already present in MSHRs
        if (t->isLoad) {
          stat.update(l1_secondary_load_misses);
          core->local_stat.update(l1_secondary_load_misses);
        } else {
          stat.update(l1_secondary_store_misses);
          core->local_stat.update(l1_secondary_store_misses);
        }
        stat.update(l1_secondary_misses);
        core->local_stat.update(l1_secondary_misses);
        mshr[cacheline].insert(t);
        t->checkMSHR=false;
        continue;
      } 
      else if (mshr_size == num_mshr_entries) {  // not present in MSHRs but they are FULL
        t->checkMSHR=true;   
        next_to_send.push_back(t);
        continue;
      }
    }
      
    uint64_t dramaddr = t->addr/size_of_cacheline * size_of_cacheline;
    int cacheline_size;
    if (cacheBySignature == 0 || t->graphNodeId == -1) {
      cacheline_size = size_of_cacheline;
    } else {
      cacheline_size = 4;
    }

    if(isLLC) {
      
      if((t->src_id!=-1) && memInterface->willAcceptTransaction(dramaddr,  t->isLoad)) {
        
        //assert(t->isLoad);
        t->cache_q->push_front(this);
              
        memInterface->addTransaction(t, dramaddr, t->isLoad, cacheline_size, cycles);

        //collect mlp stats
        sim->curr_epoch_accesses++;
        
        //it = to_send.erase(it);        
      } else if ((t->src_id==-1) && memInterface->willAcceptTransaction(dramaddr, false)) { //forwarded eviction, will be treated as just a write, nothing to do
        memInterface->addTransaction(NULL, dramaddr, false, cacheline_size, cycles);
        //collect mlp stats
        sim->curr_epoch_accesses++;
        //it = to_send.erase(it);
        delete t;           
      } else {
        next_to_send.push_back(t);
        //++it;
      }
    } else {
      if(size == 0 || parent_cache->willAcceptTransaction(t)) {
        t->cache_q->push_front(this);
        parent_cache->addTransaction(t);

        //it=to_send.erase(it);
      } else {
        next_to_send.push_back(t);
        //++it;
      }
    }
  }

  to_send=next_to_send;

  vector<tuple<uint64_t, int, int, int>> next_to_evict;
  
  for(auto it = to_evict.begin(); it!= to_evict.end();++it) {
    uint64_t eAddr = get<0>(*it);
    int graphNodeId = get<1>(*it);
    int graphNodeDeg = get<2>(*it);
    int cacheline_size = get<3>(*it);
    if(isLLC) {
      if(memInterface->willAcceptTransaction(eAddr, false)) {
        
        memInterface->addTransaction(NULL, eAddr, false, cacheline_size, cycles);
        //it = to_evict.erase(it);
      }
      else {
        next_to_evict.push_back(make_tuple(eAddr, graphNodeId, graphNodeDeg, cacheline_size));
        //++it;
      }
    }
    else {
      MemTransaction* t = new MemTransaction(-1, -1, -1, eAddr, false, graphNodeId, graphNodeDeg);
      if(size == 0 || parent_cache->willAcceptTransaction(t)) {
        t->cache_q->push_front(this); 
        parent_cache->addTransaction(t);
        //it = to_evict.erase(it);
      }
      else {
        next_to_evict.push_back(make_tuple(eAddr, graphNodeId, graphNodeDeg, cacheline_size));
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
  int graphNodeId = -1;
  int graphNodeDeg = -1;
  if(t->src_id!=-1) {
    nodeId = t->d->n->id;
  }
  graphNodeId = t->graphNodeId;
  graphNodeDeg = t->graphNodeDeg;
 
  bool bypassCache = (size == 0); //|| (cache_by_temperature == 1 && isL1 && graphNodeDeg < node_degree_threshold); //set as false to turn this off

  if(!ideal) {
    if (!bypassCache) {
      res = fc->access(dramaddr, nodeId, graphNodeId, graphNodeDeg, t->isLoad);   
    } else {
      res = false;
    }
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
      if(isL1) {

        //don't do anything with prefetch instructions
        /*if(!t->isPrefetch) {
          sim->accessComplete(t);         
          }*/
                
        uint64_t cacheline=t->addr/size_of_cacheline;
        mshr[cacheline]=MSHR_entry();
        mshr[cacheline].insert(t);
        mshr[cacheline].hit=1;
        
        //assert(mshr.find(cacheline)!=mshr.end());
        TransactionComplete(t);
      
        //stat.update(l1_total_accesses);
        //core->local_stat.update(l1_total_accesses);
        if(t->isPrefetch) {
          stat.update(l1_prefetch_hits);
          core->local_stat.update(l1_prefetch_hits);
          stat.update(l1_prefetches);
          core->local_stat.update(l1_prefetches);  
        } else {    
          stat.update(l1_hits);
          core->local_stat.update(l1_hits);
          if(t->isLoad) {
            stat.update(l1_load_hits);
            core->local_stat.update(l1_load_hits);
            stat.update(l1_loads);
            core->local_stat.update(l1_loads);
          } else {
            stat.update(l1_store_hits);
            core->local_stat.update(l1_store_hits);
            stat.update(l1_stores);
            core->local_stat.update(l1_stores);
          }
          stat.update(l1_accesses);
          core->local_stat.update(l1_accesses);
        }
      }
      //for l2 and l3 cache
      else {
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
        Cache* child_cache=t->cache_q->front();
        t->cache_q->pop_front();
        
        bool bypassCache = (child_cache->size == 0); // || (child_cache->cache_by_temperature == 1 && child_cache->isL1 && graphNodeDeg < child_cache->node_degree_threshold);
        if (!bypassCache) { // hit in L2 or L3, so insert in L1 
          child_cache->fc->insert(dramaddr, nodeId, graphNodeId, graphNodeDeg, t->isLoad, &dirtyEvict, &evictedAddr, &evictedOffset, &evictedNodeId, &evictedGraphNodeId, &evictedGraphNodeDeg, &unusedSpace); 
        }
        
        if(evictedAddr != -1) {
          
          assert(evictedAddr >= 0);
          stat.update(cache_evicts);
          if (child_cache->isL1) {
            stat.update(l1_evicts);
          } else {
            stat.update(l2_evicts);
          }
          
          int cacheline_size;
          if (cacheBySignature == 0 || evictedGraphNodeDeg == -1) {
            cacheline_size = child_cache->size_of_cacheline;
          } else {
            cacheline_size = 4;
          }

          if (dirtyEvict) {
            child_cache->to_evict.push_back(make_tuple(evictedAddr*child_cache->size_of_cacheline + evictedOffset, evictedGraphNodeId, evictedGraphNodeDeg, cacheline_size));
          
            if (child_cache->isL1) {
              stat.update(l1_dirty_evicts);
            } else {
              stat.update(l2_dirty_evicts);
            }
          } else {
            if (child_cache->isL1) {
              stat.update(l1_clean_evicts);
            } else {
              stat.update(l2_clean_evicts);
            }
          }

          if (sim->recordEvictions) {
            cacheStat cache_stat;
            cache_stat.cacheline = evictedAddr*child_cache->size_of_cacheline + evictedOffset;
            cache_stat.cycle = cycles;
            cache_stat.offset = evictedOffset;
            cache_stat.nodeId = evictedNodeId;
            cache_stat.graphNodeId = evictedGraphNodeId;
            cache_stat.graphNodeDeg = evictedGraphNodeDeg;
            cache_stat.unusedSpace = unusedSpace;
            if (child_cache->isL1) {
              cache_stat.cacheLevel = 1;
            } else {
              cache_stat.cacheLevel = 2;
            }
            sim->evictStatsVec.push_back(cache_stat);  
          }        
        }
        
        child_cache->TransactionComplete(t);

        if (isLLC && useL2) {
          //stat.update(l3_total_accesses);
          if(t->isPrefetch) {
            stat.update(l3_prefetch_hits);
            stat.update(l3_prefetches);
          } else {
            stat.update(l3_hits);
            if(t->isLoad) {
              stat.update(l3_load_hits);
              stat.update(l3_loads);
            } else {
              stat.update(l3_store_hits);
              stat.update(l3_stores);
            }
            stat.update(l3_accesses);
          }
        } else {
          //stat.update(l2_total_accesses);
          if(t->isPrefetch) {
            stat.update(l2_prefetch_hits);
            stat.update(l2_prefetches);
          } else {
            stat.update(l2_hits);
            if(t->isLoad) {
              stat.update(l2_load_hits);
              stat.update(l2_loads);
            } else {
              stat.update(l2_store_hits);
              stat.update(l2_stores);
            }
            stat.update(l2_accesses);
          }
        }
      }       
    } else { // eviction from lower cache, no need to do anything, since it's a hit, involves no DN
      if(isL1) {
        //stat.update(l1_total_accesses);
        //core->local_stat.update(l1_total_accesses);
        stat.update(l1_accesses);
        core->local_stat.update(l1_accesses);
      } else { //for l2 and l3 cache
        if (isLLC && useL2) {
          //stat.update(l3_total_accesses);
          stat.update(l3_accesses);
          stat.update(l3_writebacks);
        } else {
          //stat.update(l2_total_accesses);
          stat.update(l2_accesses);
          stat.update(l2_writebacks);
        }
      }       
      delete t; 
    }
  } //misses
  else {
    if (t->src_id!=-1) { // not a dirty eviction
      if(isL1) {
        //stat.update(l1_total_accesses);
        //core->local_stat.update(l1_total_accesses);
        if(t->isPrefetch) {
          stat.update(l1_prefetch_misses);
          core->local_stat.update(l1_prefetch_misses);
          stat.update(l1_prefetches);
          core->local_stat.update(l1_prefetches);  
        } else {    
          stat.update(l1_misses);
          core->local_stat.update(l1_misses);
          if(t->isLoad) {
            stat.update(l1_load_misses);
            core->local_stat.update(l1_load_misses);
            stat.update(l1_loads);
            core->local_stat.update(l1_loads);
          } else {
            stat.update(l1_store_misses);
            core->local_stat.update(l1_store_misses);
            stat.update(l1_stores);
            core->local_stat.update(l1_stores);
          }
          stat.update(l1_accesses);
          core->local_stat.update(l1_accesses);
        }
        t->checkMSHR=true;
      } else if (isLLC && useL2) {
        //stat.update(l3_total_accesses);
        if(t->isPrefetch) {
          stat.update(l3_prefetch_misses);
          stat.update(l3_prefetches);
        } else {
          stat.update(l3_misses);
          if(t->isLoad) {
            stat.update(l3_load_misses);
            stat.update(l3_loads);
          } else {
            stat.update(l3_store_misses);
            stat.update(l3_stores);
          }
          stat.update(l3_accesses);
        }
      } else {
        //stat.update(l2_total_accesses);
        if(t->isPrefetch) {
          stat.update(l2_prefetch_misses);
          stat.update(l2_prefetches);
        } else {
          stat.update(l2_misses);
          if(t->isLoad) {
            stat.update(l2_load_misses);
            stat.update(l2_loads);
          } else {
            stat.update(l2_store_misses);
            stat.update(l2_stores);
          }
          stat.update(l2_accesses);
        }
      }
    } else {
      if(isL1) {
        //stat.update(l1_total_accesses);
        //core->local_stat.update(l1_total_accesses);
        stat.update(l1_accesses);
        core->local_stat.update(l1_accesses);
      } else { //for l2 and l3 cache
        if (isLLC && useL2) {
          //stat.update(l3_total_accesses);
          stat.update(l3_accesses);
          stat.update(l3_writebacks);
        } else {
          //stat.update(l2_total_accesses);
          stat.update(l2_accesses);
          stat.update(l2_writebacks);
        }
      }       
    }
    to_send.push_back(t); //send higher up in hierarchy
    //d->print(Cache Miss, 1);
    //if(!t->isPrefetch) {    
    //}
  }
}

void Cache::addTransaction(MemTransaction *t) {
  /*if(isL1 && t->src_id!=-1) { //not eviction
    uint64_t cacheline = t->addr/size_of_cacheline;
    if(mshr.find(cacheline)==mshr.end()) {
      if (size > 0 || ideal) {
        mshr[cacheline]=MSHR_entry();
      }
      pq.push(make_pair(t, cycles+latency)); //add transaction only if it's the 1st
    }
    if (size > 0 || ideal) {
      mshr[cacheline].insert(t); 
    }
  } else {
     pq.push(make_pair(t, cycles+latency));
  }*/

  pq.push(make_pair(t, cycles+latency));
  stat.update(cache_access);
  if(t->isLoad)
    free_load_ports--;
  else
    free_store_ports--;

  //for prefetching, don't issue prefetch for evict or for access with outstanding prefetches or for access that IS  prefetch or eviction
  if(isL1 && t->src_id>=0 && num_prefetched_lines>0 && t->graphNodeId == -1) {
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
          MemTransaction* prefetch_t = new MemTransaction(-2, -2, -2, t->addr + size_of_cacheline*(prefetch_distance+i), true, -1, -1); //prefetch some distance ahead
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
    // ANINDA COMMENT
    /*if((size > 0 || ideal) && mshr.find(cacheline)==mshr.end()) {
      assert(false);
    }*/
    
    /*MSHR_entry mshr_entry;
    if (size > 0 || ideal) {
      mshr_entry=mshr[cacheline];
    } 
    }*/

    //update hit/miss stats, account for each individual access

    /*stat.update(l1_total_accesses,batch_size);
    
    if(mshr_entry.hit) {
      if(t->isprefetch) {
        stat.update(l1_prefetch_hits,batch_size-non_prefetch_size);
        stat.update(l1_prefetches,batch_size-non_prefetch_size);  
      } else {
        stat.update(l1_hits,non_prefetch_size);
        core->local_stat.update(l1_hits,non_prefetch_size);
        stat.update(l1_accesses,non_prefetch_size);
        core->local_stat.update(l1_accesses,non_prefetch_size);
      }
    } else {
      if(t->isprefetch) {
        stat.update(l1_prefetch_misses,batch_size-non_prefetch_size);
        stat.update(l1_prefetches,batch_size-non_prefetch_size);  
      } else {
        stat.update(l1_misses,non_prefetch_size);
        core->local_stat.update(l1_misses,non_prefetch_size);
        stat.update(l1_accesses,non_prefetch_size);
        core->local_stat.update(l1_accesses,non_prefetch_size);
      }
    }*/
    
    if (mshr.find(cacheline) == mshr.end()) {
      if(core->sim->debug_mode || core->sim->mem_stats_mode) {
        DynamicNode* d=t->d;
        assert(d!=NULL);
        assert(core->sim->load_stats_map.find(d)!=core->sim->load_stats_map.end());
        get<2>(core->sim->load_stats_map[d])=1;
      } 
    } else if (size > 0 || ideal) {
      MSHR_entry mshr_entry = mshr[cacheline];
      auto trans_set=mshr_entry.opset;
      //int batch_size=trans_set.size();
      //int non_prefetch_size=mshr_entry.non_prefetch_size;
    
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
           
        /*if(mshr_entry.hit) {     
          if(curr_t->isLoad) {
            stat.update(l1_load_hits);
            core->local_stat.update(l1_load_hits);
            stat.update(l1_loads);
            core->local_stat.update(l1_loads);
          } else {
            stat.update(l1_store_hits);
            core->local_stat.update(l1_store_hits);
            stat.update(l1_stores);
            core->local_stat.update(l1_stores);
          }
        } else {
          if(curr_t->isLoad) {
            stat.update(l1_load_misses);
            core->local_stat.update(l1_load_misses);
            stat.update(l1_loads);
            core->local_stat.update(l1_loads);
          } else {
            stat.update(l1_store_misses);
            core->local_stat.update(l1_store_misses);
            stat.update(l1_stores);
            core->local_stat.update(l1_stores);
          }
        }*/

          sim->accessComplete(curr_t);
        } else { //prefetches get no callback, tied to no dynamic node
          delete curr_t;
        }   
      }
      num_mshr_entries--;
      mshr.erase(cacheline); //clear mshr for that cacheline
    }
  } else {
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
    
    t->cache_q->pop_front();

    bool bypassCache = (c->size == 0); // || (c->cache_by_temperature == 1 && c->isL1 && graphNodeDeg < c->node_degree_threshold);
    if (!bypassCache) { // retrieved from DRAM, need to send up to L1 
      c->fc->insert(t->addr, nodeId, graphNodeId, graphNodeDeg, t->isLoad, &dirtyEvict, &evictedAddr, &evictedOffset, &evictedNodeId, &evictedGraphNodeId, &evictedGraphNodeDeg, &unusedSpace);
    }

    if(evictedAddr!=-1) {
      assert(evictedAddr >= 0);
      stat.update(cache_evicts);
      if (c->isL1) {
        stat.update(l1_evicts);
      } else {
        stat.update(l2_evicts);
      }

      int cacheline_size;
      if (cacheBySignature == 0 || evictedGraphNodeDeg == -1) {
        cacheline_size = c->size_of_cacheline;
      } else {
        cacheline_size = 4;
      }

      if (dirtyEvict) {
        c->to_evict.push_back(make_tuple(evictedAddr*c->size_of_cacheline + evictedOffset, evictedGraphNodeId, evictedGraphNodeDeg, cacheline_size));
        if (c->isL1) {
          stat.update(l1_dirty_evicts);
        } else {
          stat.update(l2_dirty_evicts);
        }
      } else {
        if (c->isL1) {
          stat.update(l1_clean_evicts);
        } else {
          stat.update(l2_clean_evicts);
        }
      }

      if (sim->recordEvictions) {
        cacheStat cache_stat;
        cache_stat.cacheline = evictedAddr*c->size_of_cacheline + evictedOffset;
        cache_stat.cycle = cycles;
        cache_stat.offset = evictedOffset;
        cache_stat.nodeId = evictedNodeId;
        cache_stat.graphNodeId = evictedGraphNodeId;
        cache_stat.graphNodeDeg = evictedGraphNodeDeg;
        cache_stat.unusedSpace = unusedSpace;
        if (c->isL1) {
          cache_stat.cacheLevel = 1;
        } else {
          cache_stat.cacheLevel = 2;
        }
        sim->evictStatsVec.push_back(cache_stat);         
      } 
    }
    c->TransactionComplete(t);      
  }
}
