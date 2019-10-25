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
  CacheLine* prev;
  CacheLine* next;
  int nodeId;
  int graphNodeId;
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
  int *used;

  CacheSet(int size, int num_blocks)
  {
    head = new CacheLine;
    tail = new CacheLine;
    entries = new CacheLine[size];
    for(int i=0; i<size;i++)
    {
      freeEntries.push_back(&entries[i]);
    }
    head->prev = NULL;
    head->next = tail;
    tail->next = NULL;
    tail->prev = head;
    num_addresses = num_blocks;
    used = new int[num_addresses];
    for(int i=0; i<num_addresses;i++)
    {
      used[i] = 0;
    }
  }
  ~CacheSet()
  {
    delete head;
    delete tail;
    delete [] entries;
    delete used;
  }
  bool access(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, bool isLoad)
  {
    CacheLine *c = addr_map[address];
    if(c)
    {
      // Hit

      deleteNode(c);
      insertFront(c);
      if(!isLoad)
        c->dirty = true;
      c->nodeId = nodeId;
      c->graphNodeId = graphNodeId;
      used[offset] = 1;

      return true;
    }
    else
    {
      return false;
    }
  }

  void insert(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, int *dirtyEvict, int64_t *evictedAddr, int *evictedNodeId, int *evictedGraphNodeId, int *unusedSpace)
  {
   
    CacheLine *c = addr_map[address];
    
    if(freeEntries.size() == 0)
    {
      
      // Evict the LRU
      c = tail->prev;
      assert(c!=head);
      deleteNode(c);
      addr_map.erase(c->addr);
      *evictedAddr = c->addr;
      *evictedNodeId = c->nodeId;
      *evictedGraphNodeId = c->graphNodeId;
     
      if(c->dirty) {
        *dirtyEvict = 1;
      } else {
        *dirtyEvict = 0;
      } 

      for(int i=0; i<num_addresses; i++) 
      {
        *unusedSpace+= used[i];
        used[i] = 0;
      }
      *unusedSpace = 4*(num_addresses-*unusedSpace); // unused space in bytes
    }
    else
    {
      // Get from Free Entries
      c = freeEntries.back();
      freeEntries.pop_back();
    }
   
    used[offset] = 1;
    c->addr = address;
    c->nodeId = nodeId;
    c->graphNodeId = graphNodeId;
    c->dirty = false;
    addr_map[address] = c;
    insertFront(c);
 
  }

    //returns isDirty to determine if you should store back data up cache hierarchy
  //designing evict functionality, unfinished!!!
  bool evict(uint64_t address, uint64_t offset, int *evictedNodeId, int *evictedGraphNodeId, int *unusedSpace) {
    if(addr_map.find(address)!=addr_map.end()) {
      CacheLine *c = addr_map[address];        
      //c = tail->prev;
      
      if(c) {
        addr_map.erase(address);
        deleteNode(c);
        freeEntries.push_back(c);
        *evictedNodeId = c->nodeId;
        *evictedGraphNodeId = c->graphNodeId;
        if(c->dirty) { //you want to know if eviction actually took place
          return true;
        }

        for(int i=0; i<num_addresses; i++)
        {
          *unusedSpace+=used[i];
          used[i] = 0;
        }
        *unusedSpace = 4*(num_addresses-*unusedSpace); // unused space in bytes
        used[offset] = 1;
      }
    }
    return false;
  }
  
  void insertFront(CacheLine *c)
  {
    c->next = head->next;
    c->prev = head;
    head->next = c;
    c->next->prev = c;
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
  FunctionalCache(int size, int assoc, int line_size)
  {
    cache_line_size = line_size;
    line_count = size / cache_line_size;
    set_count = line_count / assoc;
    log_set_count = log2(set_count);
    log_line_size = log2(cache_line_size);
    for(int i=0; i<set_count; i++)
    {
      sets.push_back(new CacheSet(assoc, cache_line_size/4));
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

  bool access(uint64_t address, int nodeId, int graphNodeId, bool isLoad)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count+log_line_size-1, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    bool res = c->access(tag, offset/4, nodeId, graphNodeId, isLoad);
    return res;
  }

  void insert(uint64_t address, int nodeId, int graphNodeId, int *dirtyEvict, int64_t *evictedAddr, int *evictedNodeId, int *evictedGraphNodeId, int *unusedSpace)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    int64_t evictedTag = -1;

    c->insert(tag, offset/4, nodeId, graphNodeId, dirtyEvict, &evictedTag, evictedNodeId, evictedGraphNodeId, unusedSpace);

    if(evictedAddr && evictedTag != -1)
      *evictedAddr = evictedTag * set_count + setid;

  }

  // reserved for decoupling
  bool evict(uint64_t address, int *evictedNodeId, int *evictedGraphNodeId, int *unusedSpace) { 
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address); 
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    
    return c->evict(tag, offset/4, evictedNodeId, evictedGraphNodeId, unusedSpace);
  }
 
};
