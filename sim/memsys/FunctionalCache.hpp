#pragma once
#include <map>
#include <vector>
#include <set>
#include <bitset>
#include <unordered_map>
#include <iostream>
#include <math.h>

using namespace std;

/** \brief   */
struct CacheLine
{
  /** \brief   */
  uint64_t addr;
  /** \brief   */
  uint64_t offset;
  /** \brief   */
  CacheLine* prev;
  /** \brief   */
  CacheLine* next;
  /** \brief   */
  int nodeId;
  /** \brief   */
  int numAccesses;
  /** \brief   */
  int rrpv;
  /** \brief   */
  bool dirty=false;
};

/** \brief   */
class CacheSet
{
public:
  /** \brief   */
  CacheLine *head;
  /** \brief   */
  CacheLine *tail;
  /** \brief   */
  CacheLine *entries;
  /** \brief   */
  vector<CacheLine*> freeEntries;
  /** \brief   */
  map<uint64_t, CacheLine*> addr_map;
  
  /** \brief   */
  int associativity; 
  /** \brief   */
  int log_linesize;
  
  /** \brief   */
  map<int, int> access_map;
  
  /** \brief  */
  CacheSet(int size, int cache_line_size);

  /** \brief   */
  ~CacheSet();

  /** \brief   */
  bool access(uint64_t address, uint64_t offset, int nodeId, bool isLoad);

  /** \brief   */
  void insert(uint64_t address, uint64_t offset,
	      int nodeId, bool isLoad,
	      int *dirtyEvict, int64_t *evictedAddr,
	      uint64_t *evictedOffset, int *evictedNodeId);

  /** \brief  evict the cachline that contains the address, if present

      returns isDirty to determine if you should store back data up cache hierarchy
      designing evict functionality.
  */
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId);  

  /** checks if the cacheline associated with address is present */
  bool present(uint64_t address);

  /** \brief   
      
      Insert such that MRU is first
  */
  void insertFront(CacheLine *c, CacheLine *currHead);
  
  /** \brief  Insert such that highest number of accesses is first */
  void insertByNumAccesses(CacheLine *c, CacheLine *currHead, CacheLine *currTail);

  /** \brief   */
  CacheLine* RRIP(CacheLine *currHead, CacheLine *currTail, int dist);

  /** \brief   */
  void deleteNode(CacheLine *c);
};

/** \brief   */
class FunctionalCache
{
public:
  /** \brief   */
  int line_count;
  /** \brief   */
  int set_count;
  /** \brief   */
  int log_set_count;
  /** \brief   */
  int cache_line_size;
  /** \brief   */
  int log_line_size;
  /** \brief   */
  vector<CacheSet*> sets;

  /** \brief   */
  FunctionalCache(int size, int assoc, int line_size);

  /** \brief inclusive   */
  uint64_t extract(int max, int min, uint64_t address);

  /** \brief   */
  bool  access(uint64_t address, int nodeId, bool isLoad);

  /** \brief   */
  void insert(uint64_t address, int nodeId, bool isLoad, int *dirtyEvict,
	      int64_t *evictedAddr, uint64_t *evictedOffset, int *evictedNodeId);

  /** \brief  evicts the cacheline containing the address  */
  bool evict(uint64_t address, uint64_t *evictedOffset, int *evictedNodeId);

  /** \brief  checks if the address is present  */
  bool present(uint64_t address);
};
