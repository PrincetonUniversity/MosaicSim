#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
#include <map>
using namespace std;

#define NUM_INST_TYPES 21
#define word_size_bytes  4  // TODO: allow different sizes. Now, word_size is a constant

class Config {
public:
  Config();
  void read(string name);
  vector<string> split(const string &s, char delim);
  void getCfg(int id, int val);

  // Config parameters
  int  verbLevel; // verbosity level
  bool cf_mode; // 0: one at a time / 1: all together
  bool mem_speculate;
  bool mem_forward;
  // Resources
  int lsq_size;
  int cache_load_ports;
  int cache_store_ports;
  int mem_load_ports=65536;
  int mem_store_ports=65536;
  int outstanding_load_requests;
  int outstanding_store_requests;
  int max_active_contexts_BB;
  // FUs
  int instr_latency[NUM_INST_TYPES];
  int num_units[NUM_INST_TYPES];
  // L1 cache
  bool ideal_cache;
  int L1_latency = 1;  // cycles
  int L1_size;     // KB
  int L1_assoc = 8; 
  int L1_linesize = 64; // bytes
  int window_size = 128; //instruction window size
  int issueWidth = 8; //total # issues per cycle
  
  map<string, int> param_map = {{"lsq_size",0},{"cf_mode",1},{"mem_speculate",2},{"mem_forward",3},{"max_active_contexts_BB",4},{"ideal_cache",5},{"l1_size_in_kb",6},{"cache_load_ports",7},{"cache_store_ports",8},{"mem_load_ports",9},{"mem_store_ports",10}, {"L1_latency",11}, {"L1_assoc",12}, {"L1_linesize",13}, {"window_size",14}, {"issueWidth",15}}; 
  //this converts the text in the config file to the variable using the getCfg function above
  //it allows us reorder and group variables at will in the config file or even omit them

};

class CacheConfig {
public:
  CacheConfig();
  void read(string name);
  vector<string> split(const string &s, char delim);
  void getCfg(int id, int val);

  // Config parameters
  int  verbLevel; // verbosity level
  bool cf_mode; // 0: one at a time / 1: all together
  bool mem_speculate;
  bool mem_forward;
  bool perfect_mem_spec;
  // Resources
  int lsq_size;
  int cache_load_ports;
  int cache_store_ports;
  int mem_load_ports=65536;
  int mem_store_ports=65536;
  int outstanding_load_requests;
  int outstanding_store_requests;
  int max_active_contexts_BB;
  // FUs
  int instr_latency[NUM_INST_TYPES];
  int num_units[NUM_INST_TYPES];
  // L1 cache
  bool ideal_cache;
  int L1_latency = 1;  // cycles
  int L1_size;     // KB
  int L1_assoc = 8; 
  int L1_linesize = 64; // bytes
};

#endif
