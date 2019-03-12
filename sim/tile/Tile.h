#ifndef TILE_H
#define TILE_H
#include "../common.h"
#include "../sim.h"


class Transaction;
class Simulator;

typedef enum {MEMORY, INTERRUPT, CONVOLUTION} TransType;
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

//Transaction class for sending messages across tiles
class Transaction {
public:
  Transaction(int trans_id, int src_id, int dst_id) : trans_id(trans_id), src_id(src_id), dst_id(dst_id) {}; //constructor for inter-tile communication
  
  Transaction(int trans_id, int src_id, uint64_t addr, bool isLoad) : trans_id(trans_id), src_id(src_id), addr(addr), isLoad(isLoad) {}; //special constructor for mem transactions  
  int trans_id;
  int src_id;
  int dst_id;
  TransType type;
  void (*callBackFunc)(Transaction* t);

  //below are for memory transactions
  uint64_t addr;
  bool isLoad;
  DynamicNode* d; //originating dynamic node, useful for core and cache model
  deque<Cache*>* cache_q = new deque<Cache*>(); //trail of originating caches
  ~Transaction(){
    delete cache_q;
  }
};

//Abstract base class for core, accelerator, storage tiles
//Note: don't reinstantiate id, sim, clockspeed, cycles in derived tiles

class Tile {
 public:
  int id;
  Simulator* sim; 
  int clockspeed; //clockspeed in Hertz
  uint64_t cycles; 

  Tile(Simulator* sim, int clockspeed): sim(sim),clockspeed(clockspeed) {
    cycles=0;
  }  
  //increment tile's cycle count, return true only if tile still has work; eventually return false;
  virtual bool process()=0;  
  //call back function to process a completed transaction
  virtual bool ReceiveTransaction(Transaction* t)=0;
};

#endif
