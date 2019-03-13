#include "../sim.h"
#include "Tile.h"

//An example tile showing how to implement Tile abstract class and how communication across tiles can be accomplished. This tile simply receives messages from a core to perform some fixed function accelerator operation on some data. Based on the Transaction Type and data sizes indicated in the transaction (i.e., some performance model), a response is sent back to the source after x cycles.

class ExampleTile: public Tile {  
public:
  ExampleTile(Simulator* sim, int clockspeed) : Tile(sim, clockspeed) {}
  ExampleTransaction* currentTransaction;
  uint64_t last_received;
  int cycle_const=0;
  bool transaction_pending=false;

  //performance model: #cycles taken for a task based on data size
  uint64_t count_cycles(int data_width, int data_height) {
    return cycle_const+data_width*data_height;
  }
  
  bool process() {    
    if(transaction_pending) {
      bool send_response=false;
      if(currentTransaction->type==EXAMPLE1) {
        send_response=(cycles-last_received)==1;
        cout << "Processing transaction in cycle " << cycles << endl;
      }
      else if(currentTransaction->type==EXAMPLE2) {
        uint64_t num_cycles=count_cycles(currentTransaction->data_width, currentTransaction->data_height);
        send_response=(cycles-last_received)==num_cycles;
        
      }
      else {
        cout << "Wrong Transaction Type!" << endl;
        assert(false);
      }
      
      if(send_response) {
        currentTransaction->dst_id=currentTransaction->src_id;
        currentTransaction->src_id=id;
        sim->InsertTransaction(currentTransaction);
        transaction_pending=false;
      }      
    }
    cycles++;
    return false;//cycles<1000000000;
  }

  bool ReceiveTransaction(Transaction* t) {
    
    
    if(transaction_pending) {
      
      return false;
    }
    cout << "Received Transaction: " << cycles << endl;
    currentTransaction=static_cast<ExampleTransaction*>(t);
    last_received=cycles;
    transaction_pending=true;
    return true;
  }  
};
