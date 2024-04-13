#pragma once 
#include "../common.hpp"
#include "../sim.hpp"
#include "accelerator_models/c-model/gemm/gemm_model.hpp"
//#include "accelerator_models/c-model/accelerators.hpp"
//#include "accelerator_models/c-model/gemm/gemm_model.cpp"

class Transaction;
class Simulator;
class L2Cache;

/** \brief Base class for core, accelerator, storage tiles etc.
    
    Note: don't change this base class or reinstantiate its member
    variables in derived tiles */    
class Tile {
 public:
  /** \brief   */
  int id;
  /** \brief   */
  Simulator* sim; 
  /** \brief clockspeed in MHertz */
  uint64_t clockspeed; //
  /** \brief The current cycle  */
  uint64_t cycles; 
  /** \brief   */
  string name="";
  /** \brief   */
  Tile(Simulator* sim, int clockspeed):
    sim(sim),clockspeed(clockspeed) {
    cycles=0;
  }
  /** \brief increment tile's cycle count, return true only if tile
      still has work; eventually return false; */
  virtual bool process()=0;  
  /** \brief   */
  virtual void fastForward(uint64_t inc)=0;  
};

/** \brief   */
typedef enum {DEFAULT, EXAMPLE1, TILE_COMPLETE, MEMORY, INTERRUPT} TransType;
/** \brief   */
typedef pair<Transaction*, uint64_t> TransactionOp;
class TransactionOpCompare {
public:
  /** \brief   */
  bool operator() (const TransactionOp &l, const TransactionOp &r) const {
    if(r.second < l.second)
      return true;
    else
      return false;
  }
};

/** \brief Base class for sending messages accross tiles

    Note: don't change this base class or reinstantiate its member
    variables in derived tiles */
class Transaction {
public:
  /** \brief  constructor for inter-tile communication  */
  Transaction(int id, int src_id, int dst_id) :
    id(id), src_id(src_id), dst_id(dst_id) {};
  /** \brief   */
  int id;
  /** \brief   */
  int src_id;
  /** \brief   */
  int dst_id;
  /** \brief   */
  bool complete=false;
  /** \brief   */
  TransType type=DEFAULT;
  /** \brief   */
  acc_perf_t perf;
  /** \brief Originating dynamic node, useful for core and cache model */
  DynamicNode* d = nullptr; //
  //void (*callBackFunc)(Transaction* t); 
};

class ExampleTransaction : public Transaction {
public:
  /** \brief   */
 ExampleTransaction(int id, int src_id, int dst_id) :
   Transaction(id,src_id,dst_id) {}
  /** \brief   */
  int data_width;
  /** \brief   */
  int data_height;
};

class GemmTransaction : public Transaction {
public:
  /** \brief   */
  string payload;
  /** \brief  dynamic node tied to transaction */
  DynamicNode* d;
  /** \brief   */
  GemmTransaction(int id, int src_id, int dst_id, string payLoad) :
    Transaction(id,src_id,dst_id) {
    payload=payLoad;
  }  
};

class MemTransaction : public Transaction {
public: 
  /** \brief   */
  MemTransaction(int id, int src_id, int dst_id, uint64_t addr, bool isLoad) :
    Transaction(id,src_id,dst_id), addr(addr), isLoad(isLoad) {}; 

  /** \brief   */
  uint64_t addr;
  /** \brief   */
  bool isLoad;
  /** \brief   */
  bool checkMSHR = false;
  /** \brief   */
  bool isPrefetch = false;
  /** \brief   */
  bool issuedPrefetch = false;
  /** \brief */
  bool atomicEviction = false;
  /** \brief Was this an eviction from lowe level dirty? */
  bool dirty = false;
  /** \brief is this an eviction due to the lock for the accelerators */
  bool acc_eviction = false;
  /** \breif The acknowledgement that needs to be updated atomically */
  int *acknw;
  
  /** \brief trail of originating caches */
  L2Cache *child_cache; //
};
