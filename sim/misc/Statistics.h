#ifndef STAT_H
#define STAT_H

#include "../common.h"

using namespace std;

class Statistics {
public:
     
  map<string, pair<uint64_t, int>> stats;
  map<string, pair<uint64_t, int>> old_stats;
  int num_types = 4;
  int printInterval = 5000000; 
  double global_energy = 0.0;
  double global_avg_power = 0.0;
  double acc_energy = 0.0;
  void registerStat(string str, int type) {
    stats.insert(make_pair(str, make_pair(0, type)));
  }
  void checkpoint() {
    for(auto stat_entry:stats) {
      old_stats.insert(stat_entry);
    }
  }
  Statistics() {
    registerStat("cycles", 0);
    //registerStat("energy", 0);
    registerStat("total_instructions", 0);
    registerStat("contexts", 0);
    //registerStat("accelerator_energy",0);

    // L1 Stats
    registerStat("l1_accesses", 0); // loads + stores
    registerStat("l1_hits", 0); // load_hits + store_hits
    registerStat("l1_misses", 0); // load_misses + store_misses
    registerStat("l1_primary_misses", 0); // primary_load_misses + primary_store_misses
    registerStat("l1_secondary_misses", 0); // secondary_load_misses + secondary_store_misses
    registerStat("l1_loads", 0); // load_hits + load_misses
    registerStat("l1_load_hits", 0);
    registerStat("l1_load_misses", 0);
    registerStat("l1_primary_load_misses", 0);
    registerStat("l1_secondary_load_misses", 0);
    registerStat("l1_stores", 0); // store_hits + store_misses
    registerStat("l1_store_hits", 0);
    registerStat("l1_store_misses", 0);
    registerStat("l1_primary_store_misses", 0);
    registerStat("l1_secondary_store_misses", 0);
    registerStat("l1_prefetches", 0); // prefetch_hits + prefetch_misses
    registerStat("l1_prefetch_hits", 0);
    registerStat("l1_prefetch_misses", 0);
    registerStat("l1_total_accesses", 0); // l1_accesses + l1_prefetches
    registerStat("l1_dirty_evicts", 0);
    registerStat("l1_clean_evicts", 0);
    registerStat("l1_evicts", 0);
    
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
    registerStat("l2_total_accesses", 0); // l2_accesses + l2_prefetches
    registerStat("l2_dirty_evicts", 0);
    registerStat("l2_clean_evicts", 0);
    registerStat("l2_evicts", 0);

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
    registerStat("l3_total_accesses", 0); // l3_accesses + l3_prefetches
    registerStat("l3_dirty_evicts", 0);
    registerStat("l3_clean_evicts", 0);
    registerStat("l3_evicts", 0);
 
    // other global cache Stats
    registerStat("cache_access", 1);
    registerStat("cache_pending", 1);
    registerStat("cache_evicts", 1);

    // DRAM Stats
    registerStat("dram_accesses", 1);
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
    registerStat("bpred_correct_preds", 0);
    registerStat("bpred_mispredictions", 0);

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
  uint64_t get(string str) {
    return stats.at(str).first;
  }
  uint64_t get_epoch(string str) {
    return stats.at(str).first-old_stats.at(str).first;
  }
  void set(string str, uint64_t val) {
    stats.at(str).first = val;
  }
  void update(string str, uint64_t inc=1) {
    stats.at(str).first += inc;
  }
  void reset() {
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      it->second.first = 0;
    }
  }
  void print(ostream& ofile) {
    ofile << "IPC : " << (double) get("total_instructions") / get("cycles") << "\n";
    ofile << "Average BW: " << (double) get("dram_bytes_accessed") / (get("cycles") / 2) << " GB/s \n";
    ofile << "Average Bandwidth (PBC): "  << (double) get("dram_bytes_accessed") / (get("cycles")/2) << " GB/s \n";
    ofile << "Average DRAM Read Latency (cycles): " << (double) get("dram_total_read_latency") / (get("dram_reads_loads") + get("dram_reads_stores")) << "\n";
    ofile << "Average DRAM Write Latency (cycles): " << (double) get("dram_total_write_latency") / get("dram_writes_evictions") << "\n";

    if(get("l1_misses")!=0)
      ofile << "L1 Miss Rate: " <<  ((100.0 * get("l1_primary_misses"))/ (get("l1_misses")+get("l1_hits"))) << "%"<< endl;
    if(get("l2_misses")!=0)
      ofile << "L2 Miss Rate: " << ( (100.0 * get("l2_misses"))/(get("l2_misses")+get("l2_hits"))) << "%"<< endl;
    if(get("l3_misses")!=0)
      ofile << "L3 Miss Rate: " << ( (100.0 * get("l3_misses"))/(get("l3_misses")+get("l3_hits"))) << "%"<< endl;

    ofile << "Branch misprediction rate: " <<  ((100.0 * get("bpred_mispredictions"))/ (get("bpred_mispredictions")+get("bpred_correct_preds"))) << "%"<< endl;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
      ofile << it->first << " : " << it->second.first << "\n";
    }
  }
  void print_epoch(ostream& ofile) {
    //ofile << "PRINTING EPOCH \n";
    //return;
    ofile << "IPC : " << (double) get_epoch("total_instructions") / get_epoch("cycles") << "\n";
    ofile << "Average BW : " << (double) get_epoch("dram_bytes_accessed") / (get_epoch ("cycles") / 2) << " GB/s \n";
    ofile << "Average Bandwidth (PBC) : "  << (double) get_epoch("dram_bytes_accessed") / (get_epoch("cycles")/2) << " GB/s \n";
    ofile << "Average DRAM Read Latency (cycles): " << (double) get_epoch("dram_total_read_latency") / (get_epoch("dram_reads_loads") + get_epoch("dram_reads_stores")) << "\n";
    ofile << "Average DRAM Write Latency (cycles): " << (double) get_epoch("dram_total_write_latency") / get_epoch("dram_writes_evictions") << "\n";
 
    if(get_epoch("l1_misses")!=0)
      {
        ofile << "L1 Miss Rate: " <<  ((100.0 * get_epoch("l1_primary_misses"))/ (get_epoch("l1_misses")+get_epoch("l1_hits"))) << "%"<< endl;
      }

    if(get_epoch("l2_misses")!=0)
      {
        ofile << "L2 Miss Rate: " << ( (100.0 * get_epoch("l2_misses"))/(get_epoch("l2_misses")+get_epoch("l2_hits"))) << "%"<< endl;
      }
    
    if(get_epoch("l3_misses")!=0)
      {
        ofile << "L3 Miss Rate: " << ( (100.0 * get_epoch("l3_misses"))/(get_epoch("l3_misses")+get_epoch("l3_hits"))) << "%"<< endl;
      }
    
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      //cout << "failing stat" << it->first << endl;
      if(old_stats.find(it->first)!=old_stats.end())
        ofile << it->first << " : " << it->second.first - old_stats.at(it->first).first << "\n";
    }
    checkpoint();
  }
};
extern Statistics stats;
#endif
