#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <vector>
using namespace std;

#define NUM_INST_TYPES 17
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
  int L1_latency;  // cycles
  int L1_size;     // KB
  int L1_assoc; 
  int L1_linesize; // bytes

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
  int L1_latency;  // cycles
  int L1_size;     // KB
  int L1_assoc; 
  int L1_linesize; // bytes

};

#endif
