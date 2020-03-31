#include "SimpleDRAM.h"
#include "DRAM.h"
#include "Cache.h"
#include "../tile/Core.h"

SimpleDRAM::SimpleDRAM(Simulator* simulator, DRAMSimInterface* dramInterface, Config dram_config) {
  latency=dram_config.dram_latency; Peak_BW=dram_config.dram_bw; sim=simulator; memInterface=dramInterface; 
}

void SimpleDRAM::initialize(int coreClockspeed) {
  //GB/s rate / (bytes/req*clockspeed)
  core_clockspeed=coreClockspeed;
  bytes_per_req=sim->cache->size_of_cacheline;
  long long num=(1000*Peak_BW*epoch_length);
  long long denom=(bytes_per_req*core_clockspeed);
  max_req_per_epoch=num/denom;
}

bool SimpleDRAM::process() {
  //handle all ready dram requests for this cycle
  //cout << "max req epoch " << max_req_per_epoch << endl;
  while(pq.size()>0) {
    MemOperator memop=pq.top();
    if(memop.final_cycle > cycles || request_count>=max_req_per_epoch) {
      break;
    }
    if(memop.isStore) {
      memInterface->write_complete(memop.trans_id,memop.addr,cycles);
    }
    else {
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
  return true;
}

bool SimpleDRAM::willAcceptTransaction(uint64_t addr) {
  return addr==addr;
}

void SimpleDRAM::addTransaction(bool isStore, uint64_t addr) {  
  MemOperator memop;
  memop.addr=addr; memop.isStore=isStore; memop.trans_id=trans_id++; memop.final_cycle=cycles+latency;
  pq.push(memop); //push to priority queue     
}
