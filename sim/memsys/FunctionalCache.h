#include <map>
#include <vector>
#include <set>
#include <bitset>
#include <math.h>
#include <iostream>
using namespace std;

struct CacheLine
{
  uint64_t addr;
  uint64_t offset;
  CacheLine* prev;
  CacheLine* next;
  int *used;
  int nodeId;
  int graphNodeId;
  int graphNodeDeg;
  bool dirty=false;
};

class CacheSet
{
public:
  CacheLine *entries;
  CacheLine *head;
  CacheLine *tail;
  std::vector<CacheLine*> freeEntries;
  std::unordered_map<uint64_t, CacheLine*> addr_map;
  int num_addresses;
  int eviction_policy;

  CacheSet(int size, int num_blocks, int eviction_policy_input)
  {
    num_addresses = num_blocks;
    head = new CacheLine;
    tail = new CacheLine;
    entries = new CacheLine[size];
    for(int i=0; i<size;i++)
    {
      CacheLine *c = &entries[i];
      c->used = new int[num_addresses];
      for(int i=0; i<num_addresses;i++)
      {
        c->used[i] = 0;
      }
      freeEntries.push_back(c);
    }
    head->prev = NULL;
    head->next = tail;
    tail->next = NULL;
    tail->prev = head;
    eviction_policy = eviction_policy_input;
  }
  ~CacheSet()
  {
    delete head;
    delete tail;
    delete [] entries;
  }

  bool access(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, int graphNodeDeg, bool isLoad)
  {
    CacheLine *c = addr_map[address];
    if(c)
    {
      // Hit

      deleteNode(c);
      
      if (eviction_policy == 0) {
        insertFront(c); 
      } else if (eviction_policy == 1) {
        insertByDegree(c);
      }

      if(!isLoad)
        c->dirty = true;
      c->offset = offset;
      c->nodeId = nodeId;
      c->graphNodeId = graphNodeId;
      c->graphNodeDeg = graphNodeDeg;
      c->used[offset/4] = 1;

      return true;
    }
    else
    {
      return false;
    }
  }

  void insert(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, int graphNodeDeg, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace)
  {
   
    CacheLine *c = addr_map[address];
    
    if(freeEntries.size() == 0)
    {
      // Evict the LRU
      c = tail->prev;
      assert(c!=head);
  
      // Measured unused space before deleting
      for(int i=0; i<num_addresses; i++) 
      {
        *unusedSpace+= c->used[i];
        c->used[i] = 0;
      }
      *unusedSpace = 4*(num_addresses-*unusedSpace); // unused space in bytes

      deleteNode(c);
      addr_map.erase(c->addr);
      *evictedAddr = c->addr;
      *evictedOffset = c->offset;
      *evictedNodeId = c->nodeId;
      *evictedGraphNodeId = c->graphNodeId;
      *evictedGraphNodeDeg = c->graphNodeDeg;
     
      if(c->dirty) {
        *dirtyEvict = 1;
      } else {
        *dirtyEvict = 0;
      }
    }
    else
    {
      // Get from Free Entries
      c = freeEntries.back();
      freeEntries.pop_back();
    }
   
    c->addr = address;
    c->offset = offset;
    c->nodeId = nodeId;
    c->graphNodeId = graphNodeId;
    c->graphNodeDeg = graphNodeDeg;
    c->dirty = false;
    addr_map[address] = c;
    c->used[offset/4] = 1;
 
    if (eviction_policy == 0) {
      insertFront(c); 
    } else if (eviction_policy == 1) {
      insertByDegree(c);
    }
  }

  //returns isDirty to determine if you should store back data up cache hierarchy
  //designing evict functionality, unfinished!!!
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace) {
    if(addr_map.find(address)!=addr_map.end()) {
      CacheLine *c = addr_map[address];        
      //c = tail->prev;
      
      if(c) {

        // Measure unused space before deleting 
        for(int i=0; i<num_addresses; i++)
        {
          *unusedSpace+=c->used[i];
          c->used[i] = 0;
        }
        *unusedSpace = 4*(num_addresses-*unusedSpace); // unused space in bytes

        addr_map.erase(address);
        deleteNode(c);
        freeEntries.push_back(c);
        *evictedOffset = c->offset;
        *evictedNodeId = c->nodeId;
        *evictedGraphNodeId = c->graphNodeId;
        *evictedGraphNodeDeg = c->graphNodeDeg;
        if(c->dirty) { //you want to know if eviction actually took place
          return true;
        }
      }
    }
    return false;
  }
  
  // Insert such that LRU is first
  void insertFront(CacheLine *c)
  {
    c->next = head->next;
    c->prev = head;
    head->next = c;
    c->next->prev = c;
  }
  
  // Insert such that lowest degree is first
  void insertByDegree(CacheLine *c)
  {
    CacheLine *curr = head->next;   
    while (curr != tail && curr->graphNodeDeg > c->graphNodeDeg) {
      curr = curr->next;
    }
    c->next = curr;
    c->prev = curr->prev;
    curr->prev->next = c;
    curr->prev = c;
  }

  void deleteNode(CacheLine *c)
  {
    c->prev->next = c->next;
    c->next->prev = c->prev;
  }
};

class FunctionalCache
{
public:
  int line_count;
  int set_count;
  int log_set_count;
  int cache_line_size;
  int log_line_size;
  std::vector<CacheSet*> sets;
  FunctionalCache(int size, int assoc, int line_size, int eviction_policy)
  {
    cache_line_size = line_size;
    line_count = size / cache_line_size;
    set_count = line_count / assoc;
    log_set_count = log2(set_count);
    log_line_size = log2(cache_line_size);
    for(int i=0; i<set_count; i++)
    {
      sets.push_back(new CacheSet(assoc, cache_line_size/4, eviction_policy));
    }
  } 
  uint64_t extract(int max, int min, uint64_t address) // inclusive
  {
      uint64_t maxmask = ((uint64_t)1 << (max+1))-1;
      uint64_t minmask = ((uint64_t)1 << (min))-1;
      uint64_t mask = maxmask - minmask;
      uint64_t val = address & mask;
      val = val >> min;
      return val;
  }

  bool access(uint64_t address, int nodeId, int graphNodeId, int graphNodeDeg, bool isLoad)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count+log_line_size-1, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    bool res = c->access(tag, offset, nodeId, graphNodeId, graphNodeDeg, isLoad);
    return res;
  }

  void insert(uint64_t address, int nodeId, int graphNodeId, int graphNodeDeg, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    int64_t evictedTag = -1;

    c->insert(tag, offset, nodeId, graphNodeId, graphNodeDeg, dirtyEvict, &evictedTag, evictedOffset, evictedNodeId, evictedGraphNodeId, evictedGraphNodeDeg, unusedSpace);
  
    if(evictedAddr && evictedTag != -1)
      *evictedAddr = evictedTag * set_count + setid;
  }

  // reserved for decoupling
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace) { 
    //uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address); 
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    
    return c->evict(tag, evictedOffset, evictedNodeId, evictedGraphNodeId, evictedGraphNodeDeg, unusedSpace);
  }
 
};
