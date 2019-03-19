#include "../sim.h"
#include "Tile.h"

//An example tile showing how to implement Tile abstract class and how communication across tiles can be accomplished. This tile simply receives messages from a core to perform some fixed function accelerator operation on some data. Based on the Transaction Type and data sizes indicated in the transaction (i.e., some performance model), a response is sent back to the source after x cycles.

class ExampleTile: public Tile {  
public:
  ExampleTile(Simulator* sim, uint64_t clockspeed) : Tile(sim, clockspeed) {}
  ExampleTransaction* currentTransaction;
  uint64_t last_received;
  int cycle_const=1;
  bool tile_complete=false;
  bool transaction_pending=false;
  deque<Transaction*> transq;

  priority_queue<TransactionOp, vector<TransactionOp>, TransactionOpCompare> pq;

  //performance model: #cycles taken for a task based on data size
  uint64_t perf_model(int data_width, int data_height) {
    return cycle_const+data_width+data_height;
  }
  
  bool process() {
    
    while(!transq.empty()) {
      ExampleTransaction* currentTransaction=static_cast<ExampleTransaction*>(transq.front());      
      uint64_t num_cycles=perf_model(currentTransaction->data_width, currentTransaction->data_height);      
      pq.push({currentTransaction, cycles+num_cycles});
      transq.pop_front(); 
    }
    
    while(pq.size() > 0) {
      if(pq.top().second > cycles) {
        break;
      }
      Transaction* currentTransaction = pq.top().first;
      uint64_t final_cycle=pq.top().second;
      currentTransaction->dst_id=currentTransaction->src_id;
      currentTransaction->src_id=id;
      sim->InsertTransaction(currentTransaction, final_cycle);      
      pq.pop();
    }
    cycles++;    
    return !tile_complete;
  }
  
  bool ReceiveTransaction(Transaction* t) {
   
    if(t->type==TILE_COMPLETE) {
      tile_complete=true;
      return true;
    }    
    transq.push_back(t);
    return true;
  }  
};
