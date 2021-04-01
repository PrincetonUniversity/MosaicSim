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
  int nodeId;
  int numAccesses;
  int rrpv;
  bool dirty=false;
};

class CacheSet
{
public:
  CacheLine *head;
  CacheLine *tail;
  CacheLine *entries;
  std::vector<CacheLine*> freeEntries;
  std::unordered_map<uint64_t, CacheLine*> addr_map;

  int associativity;
  int log_linesize;

  std::unordered_map<int, int> access_map;

  CacheSet(int size, int cache_line_size)
  {
    associativity = size;
    log_linesize = log2(cache_line_size);

    CacheLine *c;
    entries = new CacheLine[associativity];
    for(int i=0; i<associativity; i++) {
      c = &entries[i];
      freeEntries.push_back(c);
    }

    head = new CacheLine;
    tail = new CacheLine;
    head->prev = NULL;
    head->next = tail;
    tail->next = NULL;
    tail->prev = head;
  }

  ~CacheSet()
  {
    delete head;
    delete tail;
    delete [] entries;
  }

  bool access(uint64_t address, uint64_t offset, int nodeId, bool isLoad)
  {
    CacheLine *c;
    c = addr_map[address];

    if(c) { // Hit
      deleteNode(c);
      insertFront(c, head);

      if(!isLoad)
        c->dirty = true;
      c->offset = offset;
      c->nodeId = nodeId;
      c->numAccesses++;
      c->rrpv = 0;

      return true;
    } else {
      return false;
    }
  }

  void insert(uint64_t address, uint64_t offset, int nodeId, bool isLoad, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId)
  {
    CacheLine *c;
    int eviction = 0;
    c = addr_map[address];
    if(freeEntries.size() == 0) {
      eviction = 1;
    } else {
      c = freeEntries.back(); // Get from Free Entries
      freeEntries.pop_back();
    }

    if (eviction == 1) {
      c = tail->prev; // Evict the last of the list
      assert(c!=head);
      addr_map.erase(c->addr);

      deleteNode(c);
      *evictedAddr = c->addr;
      *evictedOffset = c->offset;
      *evictedNodeId = c->nodeId;
      if(c->dirty) {
        *dirtyEvict = 1;
      } else {
        *dirtyEvict = 0;
      }
    }

    c->addr = address;
    c->offset = offset;
    c->nodeId = nodeId;
    c->numAccesses = 1;
    c->rrpv = 2;
    if (!isLoad) {
      c->dirty = true;
    } else {
      c->dirty = false;
    }
    addr_map[address] = c;
    insertFront(c, head);
  }

  //returns isDirty to determine if you should store back data up cache hierarchy
  //designing evict functionality, unfinished!!!
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId) {
    if(addr_map.find(address)!=addr_map.end()) {
      CacheLine *c = addr_map[address];
      //c = tail->prev;

      if(c) {
        addr_map.erase(address);
        deleteNode(c);
        freeEntries.push_back(c);
        *evictedOffset = c->offset;
        *evictedNodeId = c->nodeId;
        if(c->dirty) { //you want to know if eviction actually took place
          return true;
        }
      }
    }
    return false;
  }

  // Insert such that MRU is first
  void insertFront(CacheLine *c, CacheLine *currHead)
  {
    c->next = currHead->next;
    c->prev = currHead;
    currHead->next = c;
    c->next->prev = c;
  }

  // Insert such that highest number of accesses is first
  void insertByNumAccesses(CacheLine *c, CacheLine *currHead, CacheLine *currTail)
  {
    CacheLine *curr = currHead->next;
    while (curr != currTail && curr->numAccesses > c->numAccesses) {
      curr = curr->next;
    }
    c->next = curr;
    c->prev = curr->prev;
    curr->prev->next = c;
    curr->prev = c;
  }

  CacheLine* RRIP(CacheLine *currHead, CacheLine *currTail, int dist) {
    CacheLine* c = currTail->prev;
    while (c != currHead && c->rrpv != dist) {
      c = c->prev;
    }
    if (c == currHead) {
      c = currTail->prev;
      while (c != currHead) {
        c->rrpv++;
        c = c->prev;
      }
      return RRIP(currHead, currTail, dist);
    } else {
      return c;
    }
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
      sets.push_back(new CacheSet(assoc, cache_line_size));
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

  bool access(uint64_t address, int nodeId, bool isLoad)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count+log_line_size-1, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    bool res = c->access(tag, offset, nodeId, isLoad);

    return res;
  }

  void insert(uint64_t address, int nodeId, bool isLoad, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    int64_t evictedTag = -1;

    c->insert(tag, offset, nodeId, isLoad, dirtyEvict, &evictedTag, evictedOffset, evictedNodeId);

    if(evictedAddr && evictedTag != -1)
      *evictedAddr = evictedTag * set_count + setid;
  }

  // reserved for decoupling
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId) {
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    return c->evict(tag, evictedOffset, evictedNodeId);
  }

};
