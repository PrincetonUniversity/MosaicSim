#ifndef STAT_H
#define STAT_H
#include <iostream>
#include <map>
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
    registerStat("l1_hits", 1);
    registerStat("l1_misses", 1);
    registerStat("l1_hits_non_prefetch", 1);
    registerStat("l1_misses_non_prefetch", 1);
    registerStat("l1_load_hits", 1);
    registerStat("l1_load_misses", 1);
    registerStat("l2_hits", 0);
    registerStat("l2_misses", 0);
    registerStat("l2_hits_non_prefetch", 0);
    registerStat("l2_misses_non_prefetch", 0);
    registerStat("l2_load_hits", 1);
    registerStat("l2_load_misses", 1);
 
    registerStat("cache_access", 1);
    registerStat("cache_pending", 1);
    registerStat("cache_evicts", 1);
    registerStat("dram_accesses", 1);
    registerStat("dram_loads", 1);
    registerStat("bytes_read",1);
    registerStat("bytes_write",1);
    
    registerStat("speculated", 2);
    registerStat("misspeculated", 2);
    registerStat("forwarded", 2);
    registerStat("speculatively_forwarded", 2);

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
    
    registerStat("lsq_insert_success", 4);
    registerStat("lsq_insert_fail", 4);
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
    ofile << "Average BW : " << (double) get("dram_accesses") / (get ("cycles") / (64 * 2)) << " GB/s \n";
    
    if(get("l1_misses")!=0)
      {
        ofile << "L1 Miss Rate: " <<  ((100.0 * get("l1_misses"))/ (get("l1_misses")+get("l1_hits"))) << "%"<< endl;
      }

    if(get("l2_misses")!=0)
      {
        ofile << "L2 Miss Rate: " << ( (100.0 * get("l2_misses"))/(get("l2_misses")+get("l2_hits"))) << "%"<< endl;
      }
    
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      ofile << it->first << " : " << it->second.first << "\n";
    }
  }
  void print_epoch(ostream& ofile) {
    //ofile << "PRINTING EPOCH \n";
    //return;
    ofile << "IPC : " << (double) get_epoch("total_instructions") / get_epoch("cycles") << "\n";
    ofile << "Average BW : " << (double) get_epoch("dram_accesses") / (get_epoch ("cycles") / (64 * 2)) << " GB/s \n";
    
    if(get_epoch("l1_misses")!=0)
      {
        ofile << "L1 Miss Rate: " <<  ((100.0 * get_epoch("l1_misses"))/ (get_epoch("l1_misses")+get_epoch("l1_hits"))) << "%"<< endl;
      }

    if(get_epoch("l2_misses")!=0)
      {
        ofile << "L2 Miss Rate: " << ( (100.0 * get_epoch("l2_misses"))/(get_epoch("l2_misses")+get_epoch("l2_hits"))) << "%"<< endl;
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
