#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
#include <map>
using namespace std;

#define NUM_INST_TYPES 26
#define word_size_bytes  4  // TODO: allow different sizes. Now, word_size is a constant

class Config {
public:
  Config();
  void read(string name);
  vector<string> split(const string &s, char delim);
  void getCfg(int id, int val);

  // Config parameters
  int  verbLevel; // verbosity level
  bool cf_mode;   // 0: one at a time / 1: all together+ prefect prediction
  bool mem_speculate;
  bool mem_forward;
  bool branch_prediction=false; // one at a time + prediction
  int misprediction_penalty=1; //number of cycles to insert before creation of next context..model misprediction
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
  // cache
  bool ideal_cache;
  int cache_latency = 1;  // cycles
  int cache_size;     // KB
  int cache_assoc = 8; 
  int cache_linesize = 64; // bytes
  int window_size = 128; //instruction window size
  int issueWidth = 8; //total # issues per cycle
  int prefetch_distance=0; // number of cachelines ahead to prefetch

  int commBuff_size=64; //comm buff size
  int commQ_size=512; //comm queue size
  int SAB_size=128; //store address buffer size
  int SVB_size=128; //store value buffer size
  int term_buffer_size=32; //max size of terminal load buffer
  int desc_latency=5; //desc queue latency
  
  map<string, int> param_map = {{"lsq_size",0},{"cf_mode",1},{"mem_speculate",2},{"mem_forward",3},{"max_active_contexts_BB",4},{"ideal_cache",5},{"cache_size_in_kb",6},{"cache_load_ports",7},{"cache_store_ports",8},{"mem_load_ports",9},{"mem_store_ports",10}, {"cache_latency",11}, {"cache_assoc",12}, {"cache_linesize",13}, {"window_size",14}, {"issueWidth",15}, {"commBuff_size", 16}, {"commQ_size",17}, {"term_buffer_size",18}, {"SAB_size",19},  {"desc_latency",20}, {"SVB_size",21}, {"branch_prediction", 22}, {"misprediction_penalty", 23}, {"prefetch_distance", 24}}; 
  //this converts the text in the config file to the variable using the getCfg function above
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
  // cache
  bool ideal_cache;
  int cache_latency = 1;  // cycles
  int cache_size;     // KB
  int cache_assoc = 8; 
  int cache_linesize = 64; // bytes
};

#endif
