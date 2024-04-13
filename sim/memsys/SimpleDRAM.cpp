#include "SimpleDRAM.hpp"
#include "DRAM.hpp"
#include "Cache.hpp"
#include "../tile/Core.hpp"

void SimpleDRAM::initialize(int coreClockspeed, int bytes_per_req) {
  //GB/s rate / (bytes/req*clockspeed)
  core_clockspeed=coreClockspeed;
  this->bytes_per_req=bytes_per_req;
  long long num=(1000*Peak_BW*epoch_length);
  long long denom=(bytes_per_req*core_clockspeed);
  if (num/denom < 1 ){
    max_req_per_epoch=1;
  } else {
    max_req_per_epoch = num/denom;
  }
}

bool SimpleDRAM::process() {
  //handle all ready dram requests for this cycle
  while(pq.size()>0) {
    MemOperator memop=pq.top();
    if(memop.final_cycle > cycles || request_count>=max_req_per_epoch) {
      break;
    }
    if(memop.isWrite) {
      memInterface->write_complete(memop.trans_id,memop.addr,cycles);
    } else {
      memInterface->read_complete(memop.trans_id,memop.addr,cycles);
    }
    request_count++;  
    pq.pop();
  }
  cycles++;
  //reset request count every epoch
  if(cycles % epoch_length==0) {
    request_count=0;
  }
  return pq.size() > 0;
}

bool SimpleDRAM::willAcceptTransaction(uint64_t addr) {
  return true;  // always returns true
}

void SimpleDRAM::addTransaction(bool isWrite, uint64_t addr) {  
  MemOperator memop;
  memop.addr=addr; 
  memop.isWrite=isWrite; 
  memop.trans_id=trans_id++; 
  memop.final_cycle=cycles+latency-1;
  pq.push(memop); //push to priority queue     
}
