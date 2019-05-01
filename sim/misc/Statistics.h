#ifndef STAT_H
#define STAT_H
#include <iostream>
#include <map>
#include "../common.h"

using namespace std;

class Statistics {
public:
     
  map<string, pair<int, int>> stats;
  int num_types = 4;
   
  void registerStat(string str, int type) {
    stats.insert(make_pair(str, make_pair(0, type)));
  }
  Statistics() {
    registerStat("cycles", 0);
    registerStat("total_instructions", 0);
    registerStat("contexts", 0);
    
    registerStat("cache_hit", 1);
    registerStat("cache_miss", 1);
    registerStat("cache_access", 1);
    registerStat("cache_pending", 1);
    registerStat("cache_evict", 1);
    registerStat("dram_access", 1);
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
  }
  int get(string str) {
    return stats.at(str).first;
  }
  void set(string str, int set) {
    stats.at(str).first = set;
  }
  void update(string str, int inc=1) {
    stats.at(str).first += inc;
  }
  void reset() {
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      it->second.first = 0;
    }
  }
  void print() {
    cout << "IPC : " << (double) get("total_instructions") / get("cycles") << "\n";
    cout << "BW : " << (double) get("dram_access") * 64 * 2 / get ("cycles") << " GB/s \n";
    for (auto it = stats.begin(); it != stats.end(); ++it) {
      cout << it->first << " : " << it->second.first << "\n";
    }
  }
};
extern Statistics stats;

#endif