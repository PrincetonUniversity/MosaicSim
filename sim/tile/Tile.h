#ifndef TILE_H
#define TILE_H
#include "../common.h"
#include "../sim.h"
#include "accelerator_models/c-model/gemm/gemm_model.hpp"
//#include "accelerator_models/c-model/accelerators.hpp"
//#include "accelerator_models/c-model/gemm/gemm_model.cpp"

class Transaction;
class Simulator;
//struct config_gemm;
//Abstract base class for core, accelerator, storage tiles
//Note: don't change this base class or reinstantiate its member variables in derived tiles

class Tile {
 public:
  int id;
  Simulator* sim;
  uint64_t clockspeed; //clockspeed in MHertz
  uint64_t cycles;
  string name="";
  Tile(Simulator* sim, int clockspeed): sim(sim),clockspeed(clockspeed) {
    cycles=0;
  }
  //increment tile's cycle count, return true only if tile still has work; eventually return false;
  virtual bool process()=0;
  //call back function to process a completed transaction
  virtual bool ReceiveTransaction(Transaction* t)=0;
  virtual void fastForward(uint64_t inc)=0;
};

typedef enum {DEFAULT, EXAMPLE1, TILE_COMPLETE, MEMORY, INTERRUPT} TransType;
typedef pair<Transaction*, uint64_t> TransactionOp;
class TransactionOpCompare {
public:
  bool operator() (const TransactionOp &l, const TransactionOp &r) const {
    if(r.second < l.second)
      return true;
    else
      return false;
  }
};

//Abstract base class for sending messages accross tiles
//Note: don't change this base class or reinstantiate its member variables in derived tiles

class Transaction {
public:
  Transaction(int id, int src_id, int dst_id) : id(id), src_id(src_id), dst_id(dst_id) {}; //constructor for inter-tile communication
  int id;
  int src_id;
  int dst_id;
  bool complete=false;
  TransType type=DEFAULT;
  acc_perf_t perf;
  DynamicNode* d; //originating dynamic node, useful for core and cache model
  //void (*callBackFunc)(Transaction* t);
};

class ExampleTransaction : public Transaction {
public:
  ExampleTransaction(int id, int src_id, int dst_id) : Transaction(id,src_id,dst_id) {}
  int data_width;
  int data_height;
};

class GemmTransaction : public Transaction {
public:
  string payload;
  DynamicNode* d; //dynamic node tied to transaction
  GemmTransaction(int id, int src_id, int dst_id, string payLoad) : Transaction(id,src_id,dst_id) {
    payload=payLoad;
  }
};

class MemTransaction : public Transaction {
public:
  MemTransaction(int id, int src_id, int dst_id, uint64_t addr, bool isLoad) : Transaction(id,src_id,dst_id), addr(addr), isLoad(isLoad) {};

  uint64_t addr;
  bool isLoad;
  bool checkMSHR = false;
  bool isPrefetch = false;
  bool issuedPrefetch = false;

  deque<Cache*>* cache_q = new deque<Cache*>(); //trail of originating caches
  ~MemTransaction(){
    delete cache_q;
  }
};



#endif
