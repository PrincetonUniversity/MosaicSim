#pragma once

#include <string>
#include <vector>
#include <map>
#include <assert.h>
#include "../tile/Bpred.hpp"

using namespace std;

#define NUM_INST_TYPES 36
/** \bref Type of the instruction */
typedef enum { 
  /** integer addition */ 
  I_ADDSUB,
  /** integer multiplication */ 
  I_MULT,
  /** integer divison */ 
  I_DIV,
  /** integer ?? */ 
  I_REM,
  /** floating point addition */ 
  FP_ADDSUB,
  /** floating point multiplication */ 
  FP_MULT,
  /** floating point division */ 
  FP_DIV,
  /** floating point ?? */ 
  FP_REM,
  /** logical instruction */ 
  LOGICAL,
  /** cast instruction */
  CAST,
  /** Get element pointer (llvm part) */
  GEP,
  /** Load from memory hierarchy */
  LD,
  /** store into memory hierarchy */
  ST,
  /** Last instruction in a Basic Block? */
  TERMINATOR,
  /** ?? */
  PHI,
  /** Send data to queue */ 
  SEND,
  /** recieve data from queue */
  RECV,
  /** store addres from compute core */ 
  STADDR,
  /** store value from compute core */ 
  STVAL,
  /** Load data directly to compute node */
  LD_PROD,
  /** Invalid instruction */
  INVALID,
  /** Biscuit -> nibbler, probably not used. TODO: check if it is used.  */
  BS_DONE,  CORE_INTERRUPT, CALL_BS, BS_WAKE, BS_VECTOR_INC, 
  /** Barrier */ 
  BARRIER,
  /** Partial Barrier, where not all threads are involved */ 
  PARTIAL_BARRIER,
  /** Accelerator instruction */
  ACCELERATOR,
  /** atomic integer add */
  ATOMIC_ADD,
  /** atomic floating point add */ 
  ATOMIC_FADD,
  /** atomic  min */
  ATOMIC_MIN,
  /** atomic compare and swap */
  ATOMIC_CAS, 
  /** LD_PRODUCE + floating point add */
  TRM_ATOMIC_FADD,
  /** LD_PRODUCE + atomic terminal min */
  TRM_ATOMIC_MIN,
  /** LD_PRODUCE + atomic terminal compare and swap */
  TRM_ATOMIC_CAS, 
} TInstr;

/** \bref  
    
    TODO: allow different sizes. Now, word_size is a constant
*/
#define word_size_bytes  4  // 

/** \bref   */
class Config {
public:
  Config();

  /** \bref   */
  void read(string name);
  /** \bref   */
  vector<string> split(const string &s, char delim);
  /** \bref  */
  void set_config(int id, int val);
  void set_config(string parm, int val);

  // Config parameters
  /** \bref  verbosity level */
  int  verbLevel; // 
  /** \bref  0: one at a time / 1: all together+ prefect prediction */
  bool cf_mode;   // 
  /** \bref   */
  bool mem_speculate;
  /** \bref   */
  bool mem_forward;

  // branch predictor
  /** \bref   */
  int branch_predictor; 
  /** \bref number of cycles to insert before creation of next context to model misprediction  */
  int misprediction_penalty=0; 
  /** \bref   */
  int bht_size=1024; 
  /** \bref   */
  int gshare_global_hist_bits=10; 

  // Resources
  /** \bref   */
  int lsq_size;
  /** \bref   */
  int cache_load_ports;
  /** \bref   */
  int cache_store_ports;
  /** \bref   */
  int mem_read_ports=65536;
  /** \bref   */
  int mem_write_ports=65536;
  /** \bref   */
  int max_active_contexts_BB;
  /** \bref   */
  int cache_prefetch_ports=4;
  
  // FUs
  /** \bref   */
  int instr_latency[NUM_INST_TYPES];
  /** \bref   */
  int num_units[NUM_INST_TYPES];
  
  // Energy variables, all in Joules
  /** \bref   */
  map <int, double [NUM_INST_TYPES]> energy_per_instr;
  /** \bref   */
  map <int, double> energy_per_L3_access;
  /** \bref   */
  map <int, double> energy_per_DRAM_access;
  /** \bref   */
  double base_freq_for_EPI = 2000;  // in Mhz
  
  // cache
  /** \bref   */
  bool ideal_cache;
  /** \bref cache latency in cycles  */
  int cache_latency = 1;  // 
  /** \bref cache size in KB  */
  int cache_size;         // 
  /** \bref   */
  int cache_assoc = 8; 
  /** \bref  cache line size in bytes */
  int cache_linesize = 64; // 
  /** \bref  instruction window size */
  int window_size = 128; 
  /** \bref total number issues per cycle  */
  int issueWidth = 8; //
  /** \bref  number of cachelines ahead to prefetch */
  int prefetch_distance=0; 
  /** \bref   */
  int mshr_size = 32;

  // l2 cache
  /** \bref   */
  bool use_l2=true;
  /** \bref   */
  bool l2_ideal_cache;
  /** \bref   */
  int l2_cache_latency = 6;
  /** \bref   */
  int l2_cache_size = 0;
  /** \bref   */
  int l2_cache_assoc = 8;
  /** \bref   */
  int l2_cache_linesize = 64;
  /** \bref   */
  int l2_cache_load_ports;
  /** \bref   */
  int l2_cache_store_ports;
  /** \bref   */
  int l2_prefetch_distance = 0;
  /** \bref   */
  int l2_num_prefetched_lines = 1;

  /** \bref comm buff size  */
  int commBuff_size=64; 
  /** \bref comm queue size  */
  int commQ_size=512; 
  /** \bref store address buffer size  */
  int SAB_size=128;
  /** \bref store value buffer size  */
  int SVB_size=128; 
  /** \bref max size of terminal load buffer  */
  int term_buffer_size=32; 
  /** \bref desc queue latency  */
  int desc_latency=3; 
  /** \bref   */
  int num_prefetched_lines=1;
  /** \bref   */
  bool SimpleDRAM=0;
  /** \bref DRAM's bandiwth in GB/s  */
  int dram_bw=12; 
  /** \bref DRAM latency in cycles  */
  int dram_latency=300; 
  /** \bref Chip frquency in MHz  */
  int chip_freq=2000; 
  /** \bref  in nm  -- for now we support 22nm, 14nm, 5nm */
  int technology_node = 5; 
  /** \bref   */
  int num_accels = 8;
  /** \bref   */
  int num_IS = 8;
  /** \bref   */
  long long mem_chunk_size=1024; 
  
  /** \bref this converts the text in the config file to the variable using the getCfg function  */
  map<string, int> param_map = {{"lsq_size",0},{"cf_mode",1},{"mem_speculate",2},{"mem_forward",3},{"max_active_contexts_BB",4},
				{"ideal_cache",5},{"cache_size",6},{"cache_load_ports",7},{"cache_store_ports",8},{"mem_read_ports",9},
				{"mem_write_ports",10}, {"cache_latency",11}, {"cache_assoc",12}, {"cache_linesize",13}, {"window_size",14}, 
				{"issueWidth",15}, {"commBuff_size", 16}, {"commQ_size",17}, {"term_buffer_size",18}, {"SAB_size",19}, 
				{"desc_latency",20}, {"SVB_size",21}, {"branch_predictor", 22}, {"misprediction_penalty", 23}, 
				{"prefetch_distance", 24}, {"num_prefetched_lines",25}, {"SimpleDRAM",26}, {"dram_bw",27}, {"dram_latency",28}, 
				{"technology_node",29}, {"chip_freq",30}, {"num_accels",31}, {"num_IS",32}, {"mem_chunk_size",33},
				{"use_l2", 34}, {"l2_ideal_cache", 35}, {"l2_cache_latency", 36}, {"l2_cache_size", 37}, {"l2_cache_assoc", 38}, {"l2_cache_linesize", 39}, 
				{"l2_cache_load_ports", 40}, {"l2_cache_store_ports", 41}, {"l2_prefetch_distance", 42}, {"l2_num_prefetched_lines", 43}, 
				{"mshr_size", 44}, {"bht_size", 45}, {"gshare_global_hist", 46}, {"cache_prefetch_ports", 47} };
  
};

