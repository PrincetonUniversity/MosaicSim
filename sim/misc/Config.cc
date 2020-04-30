#include "Config.h"
#include "../graph/Graph.h"
#include <boost/algorithm/string.hpp>

using namespace std;

Config::Config() {

  // latencies for Haswell (our "cafe" machine: Xeon E5 v3) from https://uops.info/
//  instr_latency[I_ADDSUB] = 1;   // ADD (R64, I32)
//  instr_latency[I_MULT] = 3;     // IMUL (R64)
//  instr_latency[I_DIV] = 37;     // IDIV (R64) 
//  instr_latency[I_REM] = 37;
//  instr_latency[FP_ADDSUB] = 3;  // ADDSS (XMM, XMM)
//  instr_latency[FP_MULT] = 5;    // MULSS (XMM, XMM)
//  instr_latency[FP_DIV] = 10;    // DIVSS (XMM, XMM)
//  instr_latency[FP_REM] = 10;

  // base latencies from SniperSIM
  instr_latency[I_ADDSUB] = 1;   
  instr_latency[I_MULT] = 3;     
  instr_latency[I_DIV] = 18;    
  instr_latency[I_REM] = 18;
  instr_latency[FP_ADDSUB] = 3; 
  instr_latency[FP_MULT] = 5;    
  instr_latency[FP_DIV] = 6;   
  instr_latency[FP_REM] = 6;

  instr_latency[LOGICAL] = 1;
  instr_latency[CAST] = 0;
  instr_latency[GEP] = 1;
  instr_latency[LD] = -1;
  instr_latency[ST] = 1;
  instr_latency[TERMINATOR] = 1;
  instr_latency[PHI] = 0;     
  instr_latency[SEND] = 1;
  instr_latency[RECV] = 1;
  instr_latency[STADDR] = 1;
  instr_latency[STVAL] = 1;
  instr_latency[LD_PROD] = -1; //treat like an actual load
  instr_latency[INVALID] = 0; 
  instr_latency[BS_DONE] = 1;
  instr_latency[CORE_INTERRUPT] = 1;
  instr_latency[CALL_BS] = 1;
  instr_latency[BS_WAKE] = 1;
  instr_latency[BS_VECTOR_INC] = 1;
  instr_latency[BARRIER]=1;
  instr_latency[ACCELERATOR]=-1;
  instr_latency[ATOMIC_ADD] = 1;
  instr_latency[ATOMIC_FADD] = 1;
  instr_latency[ATOMIC_MIN] = 1;
  instr_latency[ATOMIC_CAS] = 1;
  instr_latency[TRM_ATOMIC_FADD] = 1;
  instr_latency[TRM_ATOMIC_MIN] = 1;
  instr_latency[TRM_ATOMIC_CAS] = 1;
  instr_latency[LLAMA] = -1;
  
  // # of FUs setting
  num_units[BS_DONE] = -1;
  num_units[CORE_INTERRUPT] = -1;
  num_units[CALL_BS] = -1;
  num_units[BS_WAKE] = -1;
  num_units[BS_VECTOR_INC] = -1;
  num_units[I_ADDSUB] = 4;        // to match Haswell's (https://en.wikichip.org/wiki/intel/microarchitectures/haswell_(client)#Execution_Units)
  num_units[I_MULT] =  2;         // to match Haswell's 
  num_units[I_DIV] = 1;           // to match Haswell's
  num_units[I_REM] = 1;
  num_units[FP_ADDSUB] = 2;       // to match Haswell's
  num_units[FP_MULT] = 2;         // to match Haswell's
  num_units[FP_DIV] = 1;          // to match Haswell's
  num_units[FP_REM] = 1;
  num_units[LOGICAL] = num_units[I_ADDSUB];  // the same # ALUs do arith & logicals
  num_units[CAST] = -1;
  num_units[GEP] = -1;
  num_units[LD] = -1;
  num_units[ST] = -1;
  num_units[TERMINATOR] = -1;
  num_units[PHI] = -1;
  num_units[SEND] = -1;
  num_units[RECV] = -1;
  num_units[STADDR] = -1;
  num_units[STVAL] = -1;
  num_units[LD_PROD] = -1;
  num_units[INVALID] = -1;
  num_units[BARRIER] = -1;
  num_units[ACCELERATOR] = -1;
  num_units[ATOMIC_ADD] = -1;
  num_units[ATOMIC_FADD] = -1;
  num_units[ATOMIC_MIN] = -1;
  num_units[ATOMIC_CAS] = -1;
  num_units[TRM_ATOMIC_FADD] = -1;
  num_units[TRM_ATOMIC_MIN] = -1;
  num_units[TRM_ATOMIC_CAS] = -1;
  num_units[LLAMA] = -1;
 
  // EPI: energy_per_instr (in Joules)
  //  - measured at a Nominal Core Volt (VDD) of 1.0V - 2GHz frequency
  technology_node = 22;
  energy_per_instr[technology_node][I_ADDSUB] = 12.13875*1e-12;
  energy_per_instr[technology_node][I_MULT]   = 37.16357758*1e-12;
  energy_per_instr[technology_node][I_DIV]    = 60.96258086*1e-12;  
  energy_per_instr[technology_node][I_REM] = energy_per_instr[technology_node][I_DIV];
  energy_per_instr[technology_node][FP_ADDSUB] = 49.88207567*1e-12;
  energy_per_instr[technology_node][FP_MULT]   = 52.75327756*1e-12;
  energy_per_instr[technology_node][FP_DIV]    = 58.39888425*1e-12; 
  energy_per_instr[technology_node][FP_REM] = energy_per_instr[technology_node][FP_DIV];
  energy_per_instr[technology_node][LOGICAL] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][CAST] = 0;
  energy_per_instr[technology_node][GEP] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][LD] = 14.180625*1e-12;          // NOTE it includes average L1 energy access !!
  energy_per_instr[technology_node][ST] = energy_per_instr[technology_node][LD]; 
  energy_per_instr[technology_node][TERMINATOR] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][PHI] = 0;     
  energy_per_instr[technology_node][SEND] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][RECV] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][STADDR] = energy_per_instr[technology_node][ST];      // let's assume this is the actual ST
  energy_per_instr[technology_node][STVAL] = energy_per_instr[technology_node][I_ADDSUB]; // note this is not storing anything
  energy_per_instr[technology_node][LD_PROD] = energy_per_instr[technology_node][LD];     // treat like an actual load
  energy_per_instr[technology_node][INVALID] = 0; 
  energy_per_instr[technology_node][BS_DONE] = 0;
  energy_per_instr[technology_node][CORE_INTERRUPT] = 0;
  energy_per_instr[technology_node][CALL_BS] = 0;
  energy_per_instr[technology_node][BS_WAKE] = 0;
  energy_per_instr[technology_node][BS_VECTOR_INC] = 0;
  energy_per_instr[technology_node][BARRIER] = 0;
  energy_per_instr[technology_node][ACCELERATOR] = 0;
  energy_per_instr[technology_node][LLAMA] = 14.180625*1e-12;

  technology_node = 14;
  energy_per_instr[technology_node][I_ADDSUB] = 8.828181818*1e-12;
  energy_per_instr[technology_node][I_MULT]   =  27.02805642*1e-12;
  energy_per_instr[technology_node][I_DIV]    = 44.33642245*1e-12;  
  energy_per_instr[technology_node][I_REM] = energy_per_instr[technology_node][I_DIV];
  energy_per_instr[technology_node][FP_ADDSUB] = 36.27787321*1e-12;
  energy_per_instr[technology_node][FP_MULT]   = 38.36602004*1e-12;
  energy_per_instr[technology_node][FP_DIV]    = 42.47191581*1e-12; 
  energy_per_instr[technology_node][FP_REM] = energy_per_instr[technology_node][FP_DIV];
  energy_per_instr[technology_node][LOGICAL] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][CAST] = 0;
  energy_per_instr[technology_node][GEP] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][LD] = 10.31318182*1e-12;          // NOTE it includes average L1 energy access !!
  energy_per_instr[technology_node][ST] = energy_per_instr[technology_node][LD]; 
  energy_per_instr[technology_node][TERMINATOR] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][PHI] = 0;     
  energy_per_instr[technology_node][SEND] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][RECV] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][STADDR] = energy_per_instr[technology_node][ST];      // let's assume this is the actual ST
  energy_per_instr[technology_node][STVAL] = energy_per_instr[technology_node][I_ADDSUB]; // note this is not storing anything
  energy_per_instr[technology_node][LD_PROD] = energy_per_instr[technology_node][LD];     // treat like an actual load
  energy_per_instr[technology_node][INVALID] = 0; 
  energy_per_instr[technology_node][BS_DONE] = 0;
  energy_per_instr[technology_node][CORE_INTERRUPT] = 0;
  energy_per_instr[technology_node][CALL_BS] = 0;
  energy_per_instr[technology_node][BS_WAKE] = 0;
  energy_per_instr[technology_node][BS_VECTOR_INC] = 0;
  energy_per_instr[technology_node][BARRIER] = 0;
  energy_per_instr[technology_node][ACCELERATOR] = 0;
  energy_per_instr[technology_node][LLAMA] = 10.31318182*1e-12;

  technology_node = 5;
  energy_per_instr[technology_node][I_ADDSUB] = 5.920399432*1e-12;
  energy_per_instr[technology_node][I_MULT] =  18.12569034*1e-12;
  energy_per_instr[technology_node][I_DIV] = 29.7331133*1e-12;  
  energy_per_instr[technology_node][I_REM] = energy_per_instr[technology_node][I_DIV];
  energy_per_instr[technology_node][FP_ADDSUB] = 24.32884872*1e-12;
  energy_per_instr[technology_node][FP_MULT] = 25.72921219*1e-12;
  energy_per_instr[technology_node][FP_DIV] = 28.48272854*1e-12; 
  energy_per_instr[technology_node][FP_REM] = energy_per_instr[technology_node][FP_DIV];
  energy_per_instr[technology_node][LOGICAL] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][CAST] = 0;
  energy_per_instr[technology_node][GEP] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][LD] = 6.916277557*1e-12;          // NOTE it includes average L1 energy access !!
  energy_per_instr[technology_node][ST] = energy_per_instr[technology_node][LD]; 
  energy_per_instr[technology_node][TERMINATOR] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][PHI] = 0;     
  energy_per_instr[technology_node][SEND] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][RECV] = energy_per_instr[technology_node][I_ADDSUB];
  energy_per_instr[technology_node][STADDR] = energy_per_instr[technology_node][ST];      // let's assume this is the actual ST
  energy_per_instr[technology_node][STVAL] = energy_per_instr[technology_node][I_ADDSUB]; // note this is not storing anything
  energy_per_instr[technology_node][LD_PROD] = energy_per_instr[technology_node][LD];     // treat like an actual load
  energy_per_instr[technology_node][INVALID] = 0; 
  energy_per_instr[technology_node][BS_DONE] = 0;
  energy_per_instr[technology_node][CORE_INTERRUPT] = 0;
  energy_per_instr[technology_node][CALL_BS] = 0;
  energy_per_instr[technology_node][BS_WAKE] = 0;
  energy_per_instr[technology_node][BS_VECTOR_INC] = 0;
  energy_per_instr[technology_node][BARRIER] = 0;
  energy_per_instr[technology_node][ACCELERATOR] = 0;
  energy_per_instr[technology_node][LLAMA] = 6.916277557*1e-12;

  technology_node = -1;

  // Average L3 access energy (in Joules)
  //  - measured at a Nominal SRAM Volt (VCS) of 1.05V
  //  - for the following L2 config: 64 KB size, 4 ways, 64 Byte lines
  energy_per_L3_access[22] = 7.254915576*1e-12; 
  energy_per_L3_access[14] = 5.276302237*1e-12;
  energy_per_L3_access[5]  = 3.538420188*1e-12;
  
  // Average DRAM access energy (in Joules)
  //  - DDR3 PHY at 800 MHz, 1600 MT/s
  //  - DDR3 controller at 200 MHz
  energy_per_DRAM_access[22] = 11050.14706*1e-12;
  energy_per_DRAM_access[14] = 8036.470588*1e-12;
  energy_per_DRAM_access[5]  = 5389.458088*1e-12;

  for(auto& tnode_darray:energy_per_instr) {
    tnode_darray.second[ATOMIC_ADD]=0;
    tnode_darray.second[ATOMIC_FADD]=0;
    tnode_darray.second[ATOMIC_MIN]=0;
    tnode_darray.second[ATOMIC_CAS]=0;
    tnode_darray.second[TRM_ATOMIC_FADD]=0;
    tnode_darray.second[TRM_ATOMIC_MIN]=0;
    tnode_darray.second[TRM_ATOMIC_CAS]=0;
  }
}

vector<string> Config::split(const string &s, char delim) {
   stringstream ss(s);
   string item;
   vector<string> tokens;
   while (getline(ss, item, delim)) {
      tokens.push_back(item);
   }
   return tokens;
}

void Config::getCfg(int id, int val) {
  switch (id) { 
  case 0:
    lsq_size = val; 
    break;
  case 1:
    cf_mode = val;
    break;
  case 2:
    mem_speculate = val;
    break;
  case 3:
    mem_forward = val;
    break;
  case 4:
    max_active_contexts_BB = val;
    break;
  case 5:
    ideal_cache = val;
    break;
  case 6:
    cache_size = val;
    break;
  case 7:
    cache_load_ports = val;
    break;
  case 8:
    cache_store_ports = val;
    break;
  case 9:
    mem_read_ports = val;
    break;
  case 10:
    mem_write_ports = val;
    break;
  case 11:
    cache_latency = val;
    break;
  case 12:
    cache_assoc = val;
    break;
  case 13:
    cache_linesize = val;
    break;
  case 14:
    window_size = val;
    break;
  case 15:
    issueWidth = val;
    break;
  case 16:
    commBuff_size = val;
    break;
  case 17:
    commQ_size = val;
    break;
  case 18:
    term_buffer_size = val;
    break;
  case 19:
    SAB_size = val;
    break;
  case 20:
    desc_latency = val;
    break;
  case 21:
    SVB_size = val;  
    break;
  case 22:
    branch_predictor = val;  
    break;
  case 23:
    misprediction_penalty = val;  
    break;
  case 24:
    prefetch_distance = val;  
    break;
  case 25:
    if (cache_size > 0) {
      num_prefetched_lines = val;
    } else {
      num_prefetched_lines = 0;
    }
    break;
  case 26:
    SimpleDRAM = val;
    break;
  case 27:
    dram_bw = val;
    break;
  case 28:
    dram_latency = val;
    break;
  case 29:
    //we only support 5 and 14 nm
    if(!(val==5 || val==14 || val==22)) {
      cout << "currently only support 5, 14, and 22 nm \n";
      assert(false);
    }
    technology_node = val;
    break;
  case 30:
    chip_freq = val;
    break;
  case 31:
    num_accels = val;
    break;
  case 32:
    num_IS = val;
    break;
  case 33:
    mem_chunk_size = val; 
    break;
  case 34:
    llama_ideal_cache = val;
    break;
  case 35:
    llama_cache_size = val;
    break;
  case 36:
    llama_cache_assoc = val;
    break;
  case 37:
    llama_cache_linesize = val;
    break;
  case 38:
    llama_cache_load_ports = val;
    break;
  case 39:
    llama_cache_store_ports = val;
    break;
  case 40:
    llama_prefetch_distance = val;
    break;
  case 41:
    if (llama_cache_size > 0) {
      llama_num_prefetched_lines = val;
    } else {
      llama_num_prefetched_lines = 0;
    }
    break;
  case 42:
    eviction_policy = val;
    break;
  case 43:
    llama_eviction_policy = val;
    break;
  case 44:
    partition_L1 = val;
    break;
  case 45:
    partition_L2 = val;
    break;
  case 46:
    cache_by_temperature = val;
    break;
  case 47:
    node_degree_threshold = val;
    break;
  case 48:
    cache_by_signature = val;
    break;
  case 49:
    partition_ratio = val;
    break;
  case 50:
    perfect_llama = val;
    break;
  case 51:
    record_evictions = val;
    break;
  case 52:
    use_l2 = val;
    break;
  case 53:
    l2_ideal_cache = val;
    break;
  case 54:
    l2_cache_latency = val;
    break;
  case 55:
    l2_cache_size = val;
    break;
  case 56:
    l2_cache_assoc = val;
    break;
  case 57:
    l2_cache_linesize = val;
    break;
  case 58:
    l2_cache_load_ports = val;
    break;
  case 59:
    l2_cache_store_ports = val;
    break;
  case 60:
    l2_prefetch_distance = val;
    break;
  case 61:
    if (l2_cache_size > 0) {
      l2_num_prefetched_lines = val;
    } else {
      l2_num_prefetched_lines = 0;
    }
    break;
  case 62:
    l2_cache_by_temperature = val;
    break;
  case 63:
    l2_node_degree_threshold = val;
    break;
  case 64:
    llama_node_id = val;
    break;
  case 65:
    if (val == -1) {
      mshr_size = numeric_limits<int>::max();
    } else {
      mshr_size = val;
    }
    break;
  case 66:
    bht_size = val;
    break;
  case 67:
    gshare_global_hist_bits = val;
    break;
  case 68:
    openDCP_latency = val;
    break;
  default:
    break;
  }
}

void Config::read(std::string name) {
  string line;
  string last_line;
  ifstream cfile(name);
  
  int id = 0;
  
  cout << "\n[SIM] ----Reading CONFIGURATION file---------\n";
  cout << "File: " << name << endl;
  if (cfile.is_open()) {
    while (getline (cfile,line)) {
      
      boost::trim(line);
      
      if(line[0]=='#' || line=="")
        continue;
      
      vector<string> s = split(line, ',');
      //getCfg(id, stoi(s.at(0)));
      string param = split(s.at(1),'#').at(0); //cut out trailing comments
      boost::trim(param);
      if(param_map.find(param)==param_map.end()) {
        cout <<"[ERROR] Can't find config mapping of: "  << s.at(1) << endl;
        assert(false);
      }
      id=param_map.at(param); //convert text variable "param" to id corresponding to cfg variable
      getCfg(id, stoi(s.at(0))); //set cfg variables to value in config file
      cout << "("<<id<<")"<< " " << param << ": " << stoi(s.at(0)) << endl;
      //id++;
    }
  }
  else {
    cout << "[ERROR] Cannot open Config file.\n";
    assert(false);
  }
  cfile.close();
//  cout << "[SIM] Finished reading configuration file\n";
  cout << "------------------------------------\n\n";
}
