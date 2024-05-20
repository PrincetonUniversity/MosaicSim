#include "DRAM.hpp"
#include "Cache.hpp"
#include "../misc/Memory_helpers.hpp"
#include "../tile/Core.hpp"

using namespace std;

string dram_accesses="dram_accesses";
string dram_acc_loads="dram_acc_loads";
string dram_reads_loads="dram_reads_loads";
string dram_reads_stores="dram_reads_stores";
string dram_writes_evictions="dram_writes_evictions";
string dram_bytes_accessed="dram_bytes_accessed";
string dram_total_read_latency="dram_total_read_latency";
string dram_total_write_latency="dram_total_write_latency";

void DRAMSimInterface::read_complete(unsigned id,
				     uint64_t addr,
				     uint64_t clock_cycle) {
  for (auto &block:  acc_memory) 
    if (block.contains(addr) && !block.completed()) {
	block.transaction_complete();
	return;
      }

  if (outstanding_read_map.find(addr) == outstanding_read_map.end()) 
    return;
  queue<pair<Transaction*, uint64_t>> &q = outstanding_read_map.at(addr);

  while(q.size() > 0) {
    pair<Transaction*, uint64_t> entry = q.front();

    global_stat->update(dram_total_read_latency, clock_cycle-entry.second);

    MemTransaction* t = static_cast<MemTransaction*>(entry.first);

    cache->to_complete.push_back(t);

    q.pop();      
  }
  if(q.size() == 0)
    outstanding_read_map.erase(addr);
}

void DRAMSimInterface::write_complete(unsigned id, uint64_t addr, uint64_t clock_cycle) {
  queue<pair<Transaction*, uint64_t>> &q = outstanding_write_map.at(addr);

  pair<Transaction*, uint64_t> entry = q.front();
  global_stat->update(dram_total_write_latency, clock_cycle-entry.second);
  q.pop();

  if(q.size() == 0)
    outstanding_write_map.erase(addr);
}

void DRAMSimInterface::addTransaction(Transaction* t, uint64_t addr, bool isRead, uint64_t issueCycle) {
  if(t!=NULL) { // this is a LD or a ST (whereas a NULL transaction means it is really an eviction)
                // Note that caches are write-back ones, so STs also "read" from dram 
    free_read_ports--;
    
    if(outstanding_read_map.find(addr) == outstanding_read_map.end()) {
      outstanding_read_map.insert(make_pair(addr, queue<pair<Transaction*, uint64_t>>()));      
    }

    if(use_SimpleDRAM) {       
      simpleDRAM->addTransaction(false,addr);
    } else {
      mem->addTransaction(false, addr);
    }
    
    outstanding_read_map.at(addr).push(make_pair(t, issueCycle));
      
    if(isRead) {
      global_stat->update(dram_reads_loads);
    } else {
      global_stat->update(dram_reads_stores);
    }

  } else { //eviction -> write into DRAM
    free_write_ports--;

    if(outstanding_write_map.find(addr) == outstanding_write_map.end()) {
      outstanding_write_map.insert(make_pair(addr, queue<pair<Transaction*, uint64_t>>()));
    }

    if(use_SimpleDRAM) {
      simpleDRAM->addTransaction(true, addr);
    } else {
      mem->addTransaction(true, addr);
    }
    outstanding_write_map.at(addr).push(make_pair(t, issueCycle));
    global_stat->update(dram_writes_evictions);
  }
  global_stat->update(dram_accesses);
  global_stat->update(dram_bytes_accessed,cacheline_size);
}

bool DRAMSimInterface::willAcceptTransaction(uint64_t addr, bool isRead) {
  for (auto &block:  acc_memory)
    if (block.contains(addr))
      return false;
  
  if((free_read_ports <= 0 && isRead && read_ports!=-1)  || (free_write_ports <= 0 && !isRead && write_ports!=-1))
    return false;
  else if(use_SimpleDRAM)
    return simpleDRAM->willAcceptTransaction(addr); 
  else
    return mem->willAcceptTransaction(addr);
}

bool DRAMSimInterface::willAcceptAccTransaction(uint64_t addr, bool isRead) {
  if((free_acc_read_ports <= 0 && isRead && read_ports!=-1)  || (free_acc_write_ports <= 0 && !isRead && write_ports!=-1))
    return false;
  else if(use_SimpleDRAM)
    return simpleDRAM->willAcceptTransaction(addr); 
  else
    return mem->willAcceptTransaction(addr);
}

bool DRAMSimInterface::process() {
  int finished_blocks = 0;
  int nb_trans = acc_requests->get_comp_elems(cycles);
  tuple<uint64_t, uint64_t, double, uint64_t> *new_tranasctions = acc_requests->get_comp_buff(cycles);

  free_read_ports = read_ports;
  free_write_ports = write_ports;
  free_acc_read_ports = read_ports;
  free_acc_write_ports = write_ports;

  /* check if there is new accelerator memory blocks */
  if (nb_trans) {
    cache->acc_started = true;
    for(int i = 0; i < nb_trans; i++) {
      uint64_t addr; uint64_t size;  double factor; uint64_t acc_end;
      tie(addr, size, factor, acc_end) = new_tranasctions[i];
      acc_memory.push_back(AccBlock(addr, size, factor, cacheline_size, cycles));
      cache->accelertor_mem.push_back(AccBlock(addr, size, 1.0, cacheline_size, cycles));
      cache->nb_acc_blocks = nb_trans;
      acc_final_cycle = acc_end;
      finished_acc = false;
      LLC_evicted = false;
    }
  }
  if(use_SimpleDRAM) 
    simpleDRAM->process();
  else 
    mem->update();
  
  if (LLC_evicted) {
    /* Process accelerator memory blocks */
    for (auto &block:  acc_memory) {
      if (block.completed_requesting())  
	continue;
      uint64_t acc_addr;
      while(acc_addr = block.next_to_send()) {
	if (willAcceptAccTransaction(acc_addr, true)) {
	  free_acc_read_ports--;
	  if(use_SimpleDRAM) {       
	    simpleDRAM->addTransaction(false, acc_addr);
	  } else {
	    mem->addTransaction(false, acc_addr);
	  }
	  global_stat->update(dram_accesses);
	  global_stat->update(dram_bytes_accessed,cacheline_size);
	  global_stat->update(dram_acc_loads);
	  block.sent();
	} else {
	  break;
	}
      }
    }
  }

  /* Check if the transaction for the accelerator memory blocks has
     been completed */
  if(!finished_acc) { 
    for (auto &block:  acc_memory) 
      if (block.completed())
	finished_blocks++;
    if (finished_blocks == acc_memory.size() && finished_blocks) {
      finished_acc = true;
      acc_responses->insert(cycles, cycles);
      // cout << "DONE MEMORY FOR ACCELERATORS!!! on cycle " << cycles << endl;
    }
  }
  
  /* The memory trnasaction are finished and/or the estimiation for the accelerator has finished. 
   time to reset everything */
  if (finished_acc && cycles >= acc_final_cycle) {
    acc_memory.clear();
    cache->acc_finished = true;
    finished_acc = false;
    LLC_evicted = false;
  }
  
  cycles++;

  return acc_memory.size() > 0;
}
