#include "Statistics.hpp"


Statistics::Statistics() {
    registerStat("cycles", 0);
    //registerStat("energy", 0);
    registerStat("total_instructions", 0);
    registerStat("contexts", 0);
    //registerStat("accelerator_energy",0);

    // L1 Stats
    registerStat("l1_accesses", 0); // loads + stores
    registerStat("l1_hits", 0); // load_hits + store_hits
    registerStat("l1_misses", 0); // load_misses + store_misses
    registerStat("l1_misses_primary", 0); // primary_load_misses + primary_store_misses
    registerStat("l1_misses_secondary", 0); // secondary_load_misses + secondary_store_misses
    registerStat("l1_loads", 0); // load_hits + load_misses
    registerStat("l1_load_hits", 0);
    registerStat("l1_load_misses", 0);
    registerStat("l1_load_misses_primary", 0);
    registerStat("l1_load_misses_secondary", 0);
    registerStat("l1_stores", 0); // store_hits + store_misses
    registerStat("l1_store_hits", 0);
    registerStat("l1_store_misses", 0);
    registerStat("l1_store_misses_primary", 0);
    registerStat("l1_store_misses_secondary", 0);
    registerStat("l1_prefetches", 0); // prefetch_hits + prefetch_misses
    registerStat("l1_prefetch_hits", 0);
    registerStat("l1_prefetch_misses", 0);
    //registerStat("l1_total_accesses", 0); // l1_accesses + l1_prefetches
    registerStat("l1_evicts_dirty", 0);
    registerStat("l1_evicts_clean", 0);
    registerStat("l1_evicts", 0);
    registerStat("l1_forced_evicts", 0); // atomic evictions + TODO(evictions for accelerators) 
    registerStat("l1_forced_evicts_clean", 0); // clean atomic evictions + TODO(evictions for accelerators) 
    registerStat("l1_forced_evicts_dirty", 0); // dirty atomic evictions + TODO(evictions for accelerators) 
 
    // L2 Stats
    registerStat("l2_accesses", 0); // loads + stores
    registerStat("l2_hits", 0); // load_hits + store_hits
    registerStat("l2_misses", 0); // load_misses + store_misses
    registerStat("l2_loads", 0); // load_hits + load_misses
    registerStat("l2_load_hits", 0);
    registerStat("l2_load_misses", 0);
    registerStat("l2_stores", 0); // store_hits + store_misses
    registerStat("l2_store_hits", 0);
    registerStat("l2_store_misses", 0);
    registerStat("l2_prefetches", 0); // prefetch_hits + prefetch_misses
    registerStat("l2_prefetch_hits", 0);
    registerStat("l2_prefetch_misses", 0);
    //registerStat("l2_total_accesses", 0); // l2_accesses + l2_prefetches
    registerStat("l2_evicts_dirty", 0);
    registerStat("l2_evicts_clean", 0);
    registerStat("l2_evicts", 0);
    registerStat("l2_writebacks", 0); 
    registerStat("l2_forced_evicts", 0); // atomic evictions + TODO(evictions for accelerators) 
    registerStat("l2_forced_evicts_clean", 0); // clean atomic evictions + TODO(evictions for accelerators) 
    registerStat("l2_forced_evicts_dirty", 0); // dirty atomic evictions + TODO(evictions for accelerators) 

    // L3 Stats
    registerStat("l3_accesses", 0); // loads + stores
    registerStat("l3_hits", 0); // load_hits + store_hits
    registerStat("l3_misses", 0); // load_misses + store_misses
    registerStat("l3_loads", 0); // load_hits + load_misses
    registerStat("l3_load_hits", 0); 
    registerStat("l3_load_misses", 0);
    registerStat("l3_stores", 0); // store_hits + store_misses
    registerStat("l3_store_hits", 0);
    registerStat("l3_store_misses", 0);
    registerStat("l3_prefetches", 0); // prefetch_hits + prefetch_misses
    registerStat("l3_prefetch_hits", 0);
    registerStat("l3_prefetch_misses", 0);
    //registerStat("l3_total_accesses", 0); // l3_accesses + l3_prefetches
    registerStat("l3_evicts_dirty", 0);
    registerStat("l3_evicts_clean", 0);
    registerStat("l3_evicts", 0);
    registerStat("l3_writebacks", 0); 
 
    // other global cache Stats
    registerStat("cache_access", 1);
    registerStat("cache_pending", 1);
    registerStat("cache_evicts", 1);

    // DRAM Stats
    registerStat("dram_accesses", 1);
    registerStat("dram_acc_loads", 1);
    registerStat("dram_reads_loads", 1);
    registerStat("dram_reads_stores", 1);
    registerStat("dram_writes_evictions", 1);
    registerStat("dram_bytes_accessed", 1);
    registerStat("dram_total_read_latency", 1);
    registerStat("dram_total_write_latency", 1);
    registerStat("bytes_read",1);
    registerStat("bytes_write",1);
    
    // LSQ: store-to-load forwarding Stats
    registerStat("speculated", 2);
    registerStat("misspeculated", 2);
    registerStat("forwarded", 2);
    registerStat("speculatively_forwarded", 2);

    registerStat("lsq_insert_success", 4);
    registerStat("lsq_insert_fail", 4);

    // branch prediction Stats
    registerStat("bpred_uncond_branches", 0);
    registerStat("bpred_cond_branches", 0);
    registerStat("bpred_cond_correct_preds", 0);
    registerStat("bpred_cond_wrong_preds", 0);

    // other DeSC-related Stats
    registerStat("load_issue_try", 3);
    registerStat("load_issue_success", 3);
    registerStat("store_issue_try", 3);
    registerStat("store_issue_success", 3);
    registerStat("comp_issue_try", 3);
    registerStat("comp_issue_success", 3);
    
    registerStat("send_issue_try", 3);
    registerStat("send_issue_success", 3);
    registerStat("recv_issue_try", 3);
    registerStat("recv_issue_success", 3);

    registerStat("stval_issue_try", 3);
    registerStat("stval_issue_success", 3);
    registerStat("staddr_issue_try", 3);
    registerStat("staddr_issue_success", 3);

    registerStat("ld_prod_issue_try", 3);
    registerStat("ld_prod_issue_success", 3);
 
    checkpoint();
}

void
Statistics::registerStat(string str, int type) {
    stats.insert(make_pair(str, make_pair(0, type)));
  }

void
Statistics::checkpoint() {
  for(auto stat_entry:stats)
    old_stats.insert(stat_entry);
}

uint64_t
Statistics::get(string str) {
  return stats.at(str).first;
}

uint64_t
Statistics::get_epoch(string str) {
  return stats.at(str).first-old_stats.at(str).first;
}

void
Statistics::set(string str, uint64_t val) {
  stats.at(str).first = val;
}

void
Statistics::update(string str, uint64_t inc) {
  stats.at(str).first += inc;
}

void
Statistics::reset() {
  for (auto it = stats.begin(); it != stats.end(); ++it) 
    it->second.first = 0;
}

void
Statistics::print(ostream& ofile) {
  ofile << "IPC : " << (double) get("total_instructions") / get("cycles") << "\n";
  ofile << "Average BW: " << (double) get("dram_bytes_accessed") / (get("cycles") / 2) << " GB/s \n";
  ofile << "Average Bandwidth (PBC): "  << (double) get("dram_bytes_accessed") / (get("cycles")/2) << " GB/s \n";
  ofile << "Average DRAM Read Latency (cycles): " << (double) get("dram_total_read_latency") / (get("dram_reads_loads") + get("dram_reads_stores")) << "\n";
  ofile << "Average DRAM Write Latency (cycles): " << (double) get("dram_total_write_latency") / get("dram_writes_evictions") << "\n";
  ofile << "global_energy: " << global_energy << endl;
  ofile << "DRAM_energy: " << DRAM_energy << endl;
  ofile << "LLC_energy: " << LLC_energy << endl;
  ofile << "cores_energy: " << cores_energy << endl;
  ofile << "accelerators_energy: " << acc_energy << endl;
  
  print_raw(ofile);
}

void
Statistics::print_raw(ostream& ofile) {
  
  for (auto it = stats.begin(); it != stats.end(); ++it) 
    ofile << it->first << " : " << it->second.first << "\n";
}


void
Statistics::print_epoch(ostream& ofile) {
  ofile << "IPC : " << (double) get_epoch("total_instructions") / get_epoch("cycles") << "\n";
  ofile << "Average BW : " << (double) get_epoch("dram_bytes_accessed") / (get_epoch ("cycles") / 2) << " GB/s \n";
  ofile << "Average Bandwidth (PBC) : "  << (double) get_epoch("dram_bytes_accessed") / (get_epoch("cycles")/2) << " GB/s \n";
  ofile << "Average DRAM Read Latency (cycles): " << (double) get_epoch("dram_total_read_latency") / (get_epoch("dram_reads_loads") + get_epoch("dram_reads_stores")) << "\n";
  ofile << "Average DRAM Write Latency (cycles): " << (double) get_epoch("dram_total_write_latency") / get_epoch("dram_writes_evictions") << "\n";
  
  if(get_epoch("l1_accesses")!=0)
    ofile << "L1 Miss Rate: " <<  (100.0 * get_epoch("l1_misses"))/(get_epoch("l1_accesses")) << "%"<< endl;
  
  if(get_epoch("l2_accesses")!=0)
    ofile << "L2 Miss Rate: " << (100.0 * get_epoch("l2_misses"))/(get_epoch("l2_accesses")) << "%"<< endl;
  
  for (auto it = stats.begin(); it != stats.end(); ++it) {
    if(old_stats.find(it->first)!=old_stats.end())
      ofile << it->first << " : " << it->second.first - old_stats.at(it->first).first << "\n";
  }
  checkpoint();
}

Statistics& Statistics::operator+=(const Statistics& rhs){
  for(auto const& entry: rhs.stats) {
    string const &parm = entry.first;
    if (this->stats.find(parm) == this->stats.end())
      this->registerStat(parm, 0);
    this->stats.at(parm).first += rhs.stats.at(parm).first;
  }
  return *this;
}
