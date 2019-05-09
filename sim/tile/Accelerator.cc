#include "../sim.h"
#include "Tile.h"
#include "Accelerator.h"
    
bool Accelerator::process() {
  if(transaction_pending && cycles==final_cycle) {
    transaction_pending=false;
    currentTransaction->dst_id=currentTransaction->src_id;
    currentTransaction->src_id=id;
    sim->InsertTransaction(currentTransaction, final_cycle);    
  }
  cycles++;
  //always return false, since it's dependent on core invocation
  return false; 
}

bool Accelerator::ReceiveTransaction(Transaction* t) {
  if(transaction_pending) {
    return false;
  }
  else {
    currentTransaction=static_cast<LATransaction*>(t);
    transaction_pending=true;

    //get number of cycles for work, based on timing model for opcode
    int num_cycles=timing_model(currentTransaction); 
    currentTransaction->cycle_count=num_cycles;
    final_cycle=cycles+num_cycles;
    return true;
  } 
}  

//return num cycles based on opcode and data size
int Accelerator::timing_model(LATransaction* t) {
  if(t->opcode==MATRIX_MULT) {
    return t->numColumnsA/5;
  }
  return t->numColumnsA/10;  
}
