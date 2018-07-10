#include <vector>
#include <iostream>
#include <set>
#include <bitset>
#include <math.h>

uint64_t extract(int max, int min, uint64_t address) // inclusive
{
    uint64_t maxmask = ((uint64_t)1 << (max+1))-1;
    uint64_t minmask = ((uint64_t)1 << (min))-1;
    uint64_t mask = maxmask - minmask;
    uint64_t val = address & mask;
    if(min > 0)
      val = val >> (min-1);
    cout << max << "/" << min << " -- " << bitset<64>(address) << "/" << bitset<64>(val)  << "\n";
    return val;
}

struct CacheLine
{
    uint64_t addr;
    CacheLine* prev;
    CacheLine* next;
};
class FunctionalCache
{
public:
  CacheLine *entries;
  CacheLine *head;
  CacheLine *tail;
  std::vector<CacheLine*> freeEntries;
  std::map<uint64_t, CacheLine*> addr_map;
  FunctionalCache(int size)
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
  }
  ~FunctionalCache()
  {
    delete head;
    delete tail;
    delete [] entries;
  }
  bool access(uint64_t address, int64_t* evict = NULL)
  {
    CacheLine *c = addr_map[address];
    if(c)
    {
      // Hit
      deleteNode(c);
      insertFront(c);
      return true;
    }
    else
    {      
      // Miss
      if(freeEntries.size() == 0) {
        if(evict)
          *evict = c->addr;        
      }
      return false;            
    }
  }

  //data has come back after DRAM miss, insert in cache set
  void set_insert(uint64_t address) {    
    CacheLine *c = addr_map[address];
    if(freeEntries.size() == 0) {
      // Evcit
      c = tail->prev;
      deleteNode(c);
      addr_map.erase(c->addr);
    }
    else {
      // Get from Free Entries
      c = freeEntries.front();
      freeEntries.erase(freeEntries.begin());
    }
    c->addr = address;
    addr_map[address] = c;
    insertFront(c);
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


class FunctionalSetCache
{
public:
  int line_count;
  int set_count;
  int log_set_count;
  std::vector<FunctionalCache*> sets;
  FunctionalSetCache(int size, int assoc)
  {
    line_count = size * 1024 * 1024 / 64;
    set_count = line_count / assoc;
    log_set_count = log2(set_count);
    for(int i=0; i<set_count; i++)
    {
      sets.push_back(new FunctionalCache(assoc));
    }
  }
  bool access(uint64_t address, int64_t *evicted = NULL)
  {
    uint64_t setid = extract(log_set_count-1, 0, address);
    uint64_t tag = extract(58, log_set_count, address);
    FunctionalCache *c = sets.at(setid);
    int64_t evictedTag = -1;
    bool res = c->access(tag, &evictedTag);
    if(evicted && evictedTag != -1)
    {
      *evicted = evictedTag * set_count + setid;
    }
    return res;
  }
  void cache_insert(uint64_t address) {
    uint64_t setid = extract(log_set_count-1, 0, address);
    FunctionalCache *c = sets.at(setid);
    c->set_insert(address);
  }
  
};
