#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include "../tile/Bpred.h"

using namespace std;

#define NUM_INST_TYPES 35
typedef enum { 
  I_ADDSUB, I_MULT, I_DIV, I_REM, FP_ADDSUB, FP_MULT, FP_DIV, FP_REM, LOGICAL, 
  CAST, GEP, LD, ST, TERMINATOR, PHI, SEND, RECV, STADDR, STVAL, LD_PROD, 
  INVALID, BS_DONE, CORE_INTERRUPT, CALL_BS, BS_WAKE, BS_VECTOR_INC, 
  BARRIER, ACCELERATOR, ATOMIC_ADD, ATOMIC_FADD, ATOMIC_MIN, ATOMIC_CAS, 
  TRM_ATOMIC_FADD, TRM_ATOMIC_MIN, TRM_ATOMIC_CAS
} TInstr;
#define word_size_bytes  4  // TODO: allow different sizes. Now, word_size is a constant

class Config {
public:

  void read(string name);
  vector<string> split(const string &s, char delim);
  void getCfg(int id, int val);

  // Config parameters
  int  verbLevel; // verbosity level
  bool cf_mode;   // 0: one at a time / 1: all together+ prefect prediction
  bool mem_speculate;
  bool mem_forward;

  // branch predictor
  int branch_predictor; 
  int misprediction_penalty=0; // number of cycles to insert before creation of next context to model misprediction
  int bht_size=1024; 
  int gshare_global_hist_bits=10; 

  // Resources
  int lsq_size;
  int cache_load_ports;
  int cache_store_ports;
  int mem_read_ports=65536;
  int mem_write_ports=65536;
  int outstanding_load_requests;
  int outstanding_store_requests;
  int max_active_contexts_BB;
  
  // FUs
  int instr_latency[NUM_INST_TYPES];
  int num_units[NUM_INST_TYPES];
  
  // Energy variables, all in Joules
  map <int, double [NUM_INST_TYPES]> energy_per_instr;
  map <int, double> energy_per_L3_access;
  map <int, double> energy_per_DRAM_access;
  double base_freq_for_EPI = 2000;  // in Mhz
  
  // cache
  bool ideal_cache;
  int cache_latency = 1;  // cycles
  int cache_size;         // in KB
  int cache_assoc = 8; 
  int cache_linesize = 64; // bytes
  int window_size = 128; //instruction window size
  int issueWidth = 8; //total # issues per cycle
  int prefetch_distance=0; // number of cachelines ahead to prefetch
  int mshr_size = 32;

  // l2 cache
  bool use_l2=true;
  bool l2_ideal_cache;
  int l2_cache_latency = 6;
  int l2_cache_size = 0;
  int l2_cache_assoc = 8;
  int l2_cache_linesize = 64;
  int l2_cache_load_ports;
  int l2_cache_store_ports;
  int l2_prefetch_distance = 0;
  int l2_num_prefetched_lines = 1;

  int commBuff_size=64; //comm buff size
  int commQ_size=512; //comm queue size
  int SAB_size=128; //store address buffer size
  int SVB_size=128; //store value buffer size
  int term_buffer_size=32; //max size of terminal load buffer
  int desc_latency=3; //desc queue latency
  int num_prefetched_lines=1;
  bool SimpleDRAM=0;
  int dram_bw=12; //GB/s
  int dram_latency=300; //cycles
  int chip_freq=2000; //MHz
  int technology_node = 5;  // in nm  -- for now we support 22nm, 14nm, 5nm
  int num_accels = 8;
  int num_IS = 8;
  long long mem_chunk_size=1024; 
  
  map<string, int> param_map = {{"lsq_size",0},{"cf_mode",1},{"mem_speculate",2},{"mem_forward",3},{"max_active_contexts_BB",4},
                {"ideal_cache",5},{"cache_size",6},{"cache_load_ports",7},{"cache_store_ports",8},{"mem_read_ports",9},
                {"mem_write_ports",10}, {"cache_latency",11}, {"cache_assoc",12}, {"cache_linesize",13}, {"window_size",14}, 
                {"issueWidth",15}, {"commBuff_size", 16}, {"commQ_size",17}, {"term_buffer_size",18}, {"SAB_size",19}, 
                {"desc_latency",20}, {"SVB_size",21}, {"branch_predictor", 22}, {"misprediction_penalty", 23}, 
                {"prefetch_distance", 24}, {"num_prefetched_lines",25}, {"SimpleDRAM",26}, {"dram_bw",27}, {"dram_latency",28}, 
                {"technology_node",29}, {"chip_freq",30}, {"num_accels",31}, {"num_IS",32}, {"mem_chunk_size",33},
                {"use_l2", 34}, {"l2_ideal_cache", 35}, {"l2_cache_latency", 36}, {"l2_cache_size", 37}, {"l2_cache_assoc", 38}, {"l2_cache_linesize", 39}, 
                {"l2_cache_load_ports", 40}, {"l2_cache_store_ports", 41}, {"l2_prefetch_distance", 42}, {"l2_num_prefetched_lines", 43}, 
                {"mshr_size", 44}, {"bht_size", 45}, {"gshare_global_hist", 46}};
  //this converts the text in the config file to the variable using the getCfg function above
  
  Config();
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
  int mshr_size = 10;
  // l2 cache
  bool use_l2 = 1;
  bool l2_ideal_cache;
  int l2_cache_latency = 9;
  int l2_cache_size = 0;
  int l2_cache_assoc = 8;
  int l2_cache_linesize = 64;
  int l2_cache_load_ports;
  int l2_cache_store_ports;
};

#endif
