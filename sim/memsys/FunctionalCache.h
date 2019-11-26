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
  int numAccesses;
  int count;
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
  
  int llama_size;
  CacheLine **llamaHeads;
  CacheLine **llamaTails;
  CacheLine **llamaEntries;
  std::vector<CacheLine*> llamaFreeEntries[16];
  std::unordered_map<uint64_t, CacheLine*> llama_addr_map;
 
  int associativity; 
  int num_addresses;
  int llama_num_addresses;
  int cache_by_signature;
  int eviction_policy;
  int cache_by_temperature;
  int node_degree_threshold;
  int perfect_llama;
  
  int TIME;
  std::unordered_map<int, int> access_map;

  CacheSet(int size, int cache_line_size, int llama_cache_line_size, int cache_by_signature_input, int partition_ratio, int eviction_policy_input, int cache_by_temperature_input, int node_degree_threshold_input, int perfect_llama_input)
  {
    associativity = size;
    num_addresses = cache_line_size/4;
    llama_num_addresses = llama_cache_line_size/4;
    int num_blocks = cache_line_size/llama_cache_line_size;
    cache_by_signature = cache_by_signature_input;
    eviction_policy = eviction_policy_input;
    cache_by_temperature = cache_by_temperature_input;
    node_degree_threshold = node_degree_threshold_input;
    perfect_llama = perfect_llama_input;

    int normal_size; // number of entries in normal list
    CacheLine *c;
    if (cache_by_signature == 1) {
      normal_size = size/(partition_ratio + 1);
      llama_size = (size - normal_size);
      llamaEntries = new CacheLine*[num_blocks];
      llamaHeads = new CacheLine*[num_blocks];
      llamaTails = new CacheLine*[num_blocks];
      for(int n=0; n<num_blocks; n++) {
        llamaEntries[n] = new CacheLine[llama_size];
        llamaHeads[n] = new CacheLine;
        llamaTails[n] = new CacheLine;
        for(int i=0; i<llama_size; i++) {
          c = &llamaEntries[n][i];
          c->used = new int[llama_num_addresses];
          for(int j=0; j<llama_num_addresses; j++) {
            c->used[j] = 0;
          }
          llamaFreeEntries[n].push_back(c); 
        }
        llamaHeads[n]->prev = NULL;
        llamaHeads[n]->next = llamaTails[n];
        llamaTails[n]->next = NULL;
        llamaTails[n]->prev = llamaHeads[n];
      } 
    } else {
      normal_size = size;
    }

    entries = new CacheLine[normal_size];
    for(int i=0; i<normal_size; i++) {
      c = &entries[i];
      c->used = new int[num_addresses];
      for(int j=0; j<num_addresses; j++) {
        c->used[j] = 0;
      }
      freeEntries.push_back(c);
    }
    head = new CacheLine;
    tail = new CacheLine;
    head->prev = NULL;
    head->next = tail;
    tail->next = NULL;
    tail->prev = head;
    TIME = 4;
  }
  ~CacheSet()
  {
    delete head;
    delete tail;
    delete [] entries;
    //delete [] llamaEntries;
  }

  bool access(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, int graphNodeDeg, bool isLoad)
  {
    if (cache_by_temperature == 1 && graphNodeId != -1 && graphNodeDeg < node_degree_threshold) {
      return false;
    }
    if (cache_by_signature == 1 && graphNodeId != -1 && llama_size == 0) {
      return false;
    }

    CacheLine *c;
    if (cache_by_signature == 1 && graphNodeId != -1) { 
      c = llama_addr_map[address];
    } else {
      c = addr_map[address];
    } 

    int index = offset >> 2;
    if(c) { // Hit
      deleteNode(c);

      if (cache_by_signature == 1) { // use evict by degree for llamas, LRU for non-llamas
        if (graphNodeDeg != -1) { // llama access
          if (eviction_policy == 0 || eviction_policy == 2) {
            insertFront(c, llamaHeads[index]);
          } else if (eviction_policy == 1) {
            insertByNumAccesses(c, llamaHeads[index], llamaTails[index]);
          } else if (eviction_policy == 3) {
            insertByWeightedAccesses(c, llamaHeads[index], llamaTails[index]);
          } else if (eviction_policy == 4) {
            insertByDegree(c, llamaHeads[index], llamaTails[index]);
          }
        } else {
          if (perfect_llama == 1) {
            if (eviction_policy == 0 || eviction_policy == 2) {
              insertFront(c, head);
            } else if (eviction_policy == 1) {
              insertByNumAccesses(c, head, tail);
            } else if (eviction_policy == 3) {
              insertByWeightedAccesses(c, head, tail);
            }
          } else { 
            insertFront(c, head);
          }
        }
      } else {
        if (eviction_policy == 0 || eviction_policy == 2) {
          insertFront(c, head); 
        } else if (eviction_policy == 1) {
          insertByNumAccesses(c, head, tail);
        } else if (eviction_policy == 3) {
          insertByWeightedAccesses(c, head, tail);
        }
      }

      if(!isLoad)
        c->dirty = true;
      c->offset = offset;
      c->nodeId = nodeId;
      c->graphNodeId = graphNodeId;
      c->graphNodeDeg = graphNodeDeg;
      c->numAccesses++;
      if (graphNodeId != -1) {
        access_map[graphNodeId]++;
      }
      c->count = TIME;
      c->rrpv = 0;
      if (cache_by_signature == 1 && graphNodeDeg != -1) {
        c->used[0] = 1;
      } else {
        c->used[offset/4] = 1;
      }

      return true;
    } else {
      return false;
    }
  }

  void insert(uint64_t address, uint64_t offset, int nodeId, int graphNodeId, int graphNodeDeg, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace)
  {
    if (cache_by_temperature == 1 && graphNodeId != -1 && graphNodeDeg < node_degree_threshold) {
      return;
    }
    if (cache_by_signature == 1 && graphNodeId != -1 && llama_size == 0) {
      return;
    }

    CacheLine *c;
    int eviction = 0;
    int index = offset >> 2;
    if (cache_by_signature == 1 && graphNodeId != -1) { 
      c = llama_addr_map[address];
      if (llamaFreeEntries[index].size() == 0) {   
        eviction = 1;
      } else {
        c = llamaFreeEntries[index].back();
        llamaFreeEntries[index].pop_back();
        cout << index << " " << llamaFreeEntries[index].size() << endl;
      }
    } else {
      c = addr_map[address];
      if(freeEntries.size() == 0) {
        eviction = 1;
      } else {
        c = freeEntries.back(); // Get from Free Entries
        freeEntries.pop_back();
      }
    }
    
    if (eviction == 1) {
      int size;
      if (cache_by_signature == 1 && graphNodeId != -1) {
        if (eviction_policy == 2) {
          c = RRIP(llamaHeads[index], llamaTails[index], 3);
        } else {
          CacheLine *curr = llamaHeads[index]->next;
          cout << llamaFreeEntries[index].size() << endl;
          cout << index << ": ";
          while (curr != llamaTails[index]) {
            cout << curr->graphNodeId << " ";
            curr = curr->next;
          }
          cout << endl;
          c = llamaTails[index]->prev;
        }
        assert(c!=llamaHeads[index]);
        llama_addr_map.erase(c->addr);
        size = llama_num_addresses;
      } else {
        c = tail->prev; // Evict the last of the list
        if ((cache_by_signature == 0 || perfect_llama == 1) && eviction_policy == 2) {
          c = RRIP(head, tail, 3);
        }
        assert(c!=head);
        addr_map.erase(c->addr);
        size = num_addresses;
      }
      
      // Measured unused space
      for(int i=0; i<size; i++) {
        *unusedSpace+= c->used[i];
        c->used[i] = 0;
      }
      *unusedSpace = 4*(num_addresses-*unusedSpace); // unused space in bytes

      deleteNode(c);
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

    c->addr = address;
    c->offset = offset;
    c->nodeId = nodeId;
    c->graphNodeId = graphNodeId;
    c->graphNodeDeg = graphNodeDeg;
    c->numAccesses = 1;
    if (graphNodeId != -1) {
      if (access_map.find(graphNodeId) == access_map.end()) { // never seen node before
        access_map[graphNodeId] = c->numAccesses;
      } else {
        access_map[graphNodeId] += c->numAccesses;
      }
    }
    c->count = TIME;
    c->rrpv = 2;
    c->dirty = false;
    if (cache_by_signature == 1 && graphNodeId != -1) {
      llama_addr_map[address] = c;
    } else {
      addr_map[address] = c;
    }
    if (cache_by_signature == 1 && graphNodeDeg != -1) {
      c->used[0] = 1;
    } else {
      c->used[offset/4] = 1;
    }

    if (cache_by_signature == 1) { // use evict by degree for llamas, LRU for non-llamas
      if (graphNodeDeg != -1) { // llama access
        if (eviction_policy == 0 || eviction_policy == 2) {
          cout << "INSERTING" << endl;
          CacheLine *curr = llamaHeads[index]->next;
          cout << "size = " << llamaFreeEntries[index].size() << endl;
          cout << index << ": ";
          while (curr != llamaTails[index]) {
            cout << curr->graphNodeId << " ";
            curr = curr->next;
          }
          cout << endl;
          insertFront(c, llamaHeads[index]);
        } else if (eviction_policy == 1) {
          insertByNumAccesses(c, llamaHeads[index], llamaTails[index]);
        } else if (eviction_policy == 3) {
          insertByWeightedAccesses(c, llamaHeads[index], llamaTails[index]);
        } else if (eviction_policy == 4) {
          insertByDegree(c, llamaHeads[index], llamaTails[index]);
        }
      } else {
        if (perfect_llama == 1) {
          if (eviction_policy == 0 || eviction_policy == 2) {
            insertFront(c, head);
          } else if (eviction_policy == 1) {
            insertByNumAccesses(c, head, tail);
          } else if (eviction_policy == 3) {
            insertByWeightedAccesses(c, head, tail);
          }
        } else { 
          insertFront(c, head);
        }
      }
    } else {
      if (eviction_policy == 0 || eviction_policy == 2) {
        insertFront(c, head);
      } else if (eviction_policy == 1) {
        insertByNumAccesses(c, head, tail);
      } else if (eviction_policy == 3) {
        insertByWeightedAccesses(c, head, tail);
      } 
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
  
  // Insert such that MRU is first
  void insertFront(CacheLine *c, CacheLine *currHead)
  {
    c->next = currHead->next;
    c->prev = currHead;
    currHead->next = c;
    c->next->prev = c;
  }
  
  // Insert such that highest degree is first
  void insertByDegree(CacheLine *c, CacheLine *currHead, CacheLine *currTail)
  {
    CacheLine *curr = currHead->next;   
    while (curr != currTail && curr->graphNodeDeg > c->graphNodeDeg) {
      curr = curr->next;
    }
    c->next = curr;
    c->prev = curr->prev;
    curr->prev->next = c;
    curr->prev = c;
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

  // Insert such that highest number of accesses is first
  void insertByWeightedAccesses(CacheLine *c, CacheLine *currHead, CacheLine *currTail)
  {
    CacheLine *curr = currHead->next;   
    while (curr != currTail && access_map[curr->graphNodeId] > access_map[c->graphNodeId]) {
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
  int cache_by_signature;
  int perfect_llama;
  int llama_cache_line_size;
  int log_llama_line_size;

  FunctionalCache(int size, int assoc, int line_size, int llama_line_size, int cache_by_signature_input, int partition_ratio, int perfect_llama_input, int eviction_policy, int cache_by_temperature, int node_degree_threshold)
  {
    cache_by_signature = cache_by_signature_input;
    perfect_llama = perfect_llama_input;
    cache_line_size = line_size;
    llama_cache_line_size = llama_line_size;
    line_count = size / cache_line_size;
    set_count = line_count / assoc;
    log_set_count = log2(set_count);
    log_line_size = log2(cache_line_size);
    log_llama_line_size = log2(llama_cache_line_size);
    for(int i=0; i<set_count; i++)
    {
      sets.push_back(new CacheSet(assoc, cache_line_size, llama_cache_line_size, cache_by_signature, partition_ratio, eviction_policy, cache_by_temperature, node_degree_threshold, perfect_llama));
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
    //cout << "accessing" << endl;
    //fflush(stdout);
    if (perfect_llama == 1 && graphNodeId != -1) {
      return true;
    }

    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count+log_line_size-1, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    bool res = c->access(tag, offset, nodeId, graphNodeId, graphNodeDeg, isLoad);

    //cout << "done accessing" << endl;
    //fflush(stdout);
    return res;
  }

  void insert(uint64_t address, int nodeId, int graphNodeId, int graphNodeDeg, int *dirtyEvict, int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace)
  {
    uint64_t offset = extract(log_line_size-1, 0, address);
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address);
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    int64_t evictedTag = -1;

    if (graphNodeId!= -1) {
      cout << "inserting into " << setid << " " << set_count << endl;
    }
    c->insert(tag, offset, nodeId, graphNodeId, graphNodeDeg, dirtyEvict, &evictedTag, evictedOffset, evictedNodeId, evictedGraphNodeId, evictedGraphNodeDeg, unusedSpace);

    if(evictedAddr && evictedTag != -1)
      *evictedAddr = evictedTag * set_count + setid;
    //cout << "done inserting" << endl;
    //fflush(stdout);
  }

  // reserved for decoupling
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId, int *evictedGraphNodeId, int *evictedGraphNodeDeg, int *unusedSpace) { 
    uint64_t setid = extract(log_set_count-1+log_line_size, log_line_size, address); 
    uint64_t tag = extract(63, log_set_count+log_line_size, address);
    CacheSet *c = sets.at(setid);
    return c->evict(tag, evictedOffset, evictedNodeId, evictedGraphNodeId, evictedGraphNodeDeg, unusedSpace);
  }
 
};
