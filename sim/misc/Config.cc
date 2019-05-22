#include "Config.h"
#include "../graph/Graph.h"
#include <iostream>
#include <fstream>
#include <sstream> 
#include <boost/algorithm/string.hpp>

using namespace std;

Config::Config() {    
  instr_latency[I_ADDSUB] = 1;
  instr_latency[I_MULT] = 3;
  instr_latency[I_DIV] = 8;  
  instr_latency[I_REM] = 8;
  instr_latency[FP_ADDSUB] = 1;
  instr_latency[FP_MULT] = 3;
  instr_latency[FP_DIV] = 8; 
  instr_latency[FP_REM] = 8;
  instr_latency[LOGICAL] = 1;
  instr_latency[CAST] = 1;
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

  num_units[BS_DONE] = -1;
  num_units[CORE_INTERRUPT] = -1;
  num_units[CALL_BS] = -1;
  num_units[BS_WAKE] = -1;
  num_units[BS_VECTOR_INC] = -1;
  num_units[I_ADDSUB] = 8;
  num_units[I_MULT] =  2;
  num_units[I_DIV] = 2;
  num_units[I_REM] = 2;
  num_units[FP_ADDSUB] = 8;
  num_units[FP_MULT] = 2;
  num_units[FP_DIV] = 2;
  num_units[FP_REM] = 2;
  num_units[LOGICAL] = 4;
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
    mem_load_ports = val;
    break;
  case 10:
    mem_store_ports = val;
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
    branch_prediction = val;  
    break;
  case 23:
    misprediction_penalty = val;  
    break;
  case 24:
    prefetch_distance = val;  
    break;
  case 25:
    num_prefetched_lines = val;
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
