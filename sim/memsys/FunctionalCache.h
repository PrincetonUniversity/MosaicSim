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
  CacheSet(int size)
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
  ~CacheSet()
  {
    delete head;
    delete tail;
    delete [] entries;
  }
  bool access(uint64_t address, bool isLoad)
  {
    CacheLine *c = addr_map[address];
    if(c)
    {
      // Hit

      deleteNode(c);
      insertFront(c);
      if(!isLoad)
        c->dirty = true;
      
      return true;
    }
    else
    {
      return false;
    }
  }
  void insert(uint64_t address, int64_t *evict = NULL)
  {
   
    CacheLine *c = addr_map[address];
     

    if(freeEntries.size() == 0)
    {
      
      // Evict the LRU
      
      c = tail->prev;
      assert(c!=head);
      //cout << "before res check \n";
      deleteNode(c);
      //cout << "after res check \n";
      addr_map.erase(c->addr);
     
      if(evict && c->dirty) {
        *evict = c->addr;
        
      }  
    }
    else
    {
      // Get from Free Entries
      c = freeEntries.front();
      freeEntries.erase(freeEntries.begin());
    }
    
    c->addr = address;
    c->dirty = false;
    addr_map[address] = c;
    insertFront(c);
 
  }

    //returns isDirty to determine if you should store back data up cache hierarchy
  //designing evict functionality, unfinished!!!
  bool evict(uint64_t address) {
    if(addr_map.find(address)!=addr_map.end()) {
      CacheLine *c = addr_map[address];        
      //c = tail->prev;
      
      if(c) {
        addr_map.erase(address);      
        deleteNode(c);
        if(c->dirty) { //you want to know if eviction actually took place
          return true;
        }
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
  std::vector<CacheSet*> sets;
  FunctionalCache(int size, int assoc, int line_size)
  {
    cache_line_size = line_size;
    line_count = size * 1024 / cache_line_size;
    set_count = line_count / assoc;
    log_set_count = log2(set_count);
    for(int i=0; i<set_count; i++)
    {
      sets.push_back(new CacheSet(assoc));
    }
  } 
  uint64_t extract(int max, int min, uint64_t address) // inclusive
  {
      uint64_t maxmask = ((uint64_t)1 << (max+1))-1;
      uint64_t minmask = ((uint64_t)1 << (min))-1;
      uint64_t mask = maxmask - minmask;
      uint64_t val = address & mask;
      if(min > 0)
        val = val >> (min-1);
      return val;
  }

  bool access(uint64_t address, bool isLoad)
  {
    uint64_t setid = extract(log_set_count-1, 0, address);
    uint64_t tag = extract(58, log_set_count, address);
    CacheSet *c = sets.at(setid);
    bool res = c->access(tag, isLoad);
    return res;
  }
  void insert(uint64_t address, int64_t *evicted = NULL)
  {

    uint64_t setid = extract(log_set_count-1, 0, address);
    uint64_t tag = extract(58, log_set_count, address);
    CacheSet *c = sets.at(setid);
    int64_t evictedTag = -1;

    c->insert(tag, &evictedTag);

    if(evicted && evictedTag != -1)
      *evicted = evictedTag * set_count + setid;

  }
  bool evict(uint64_t address) {
    
    uint64_t setid = extract(log_set_count-1, 0, address);
    
    uint64_t tag = extract(58, log_set_count, address);
    
    CacheSet *c = sets.at(setid);
    
    return c->evict(tag);
  }

  
};
