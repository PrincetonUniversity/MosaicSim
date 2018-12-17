#include "Config.h"
#include "../graph/Graph.h"
#include <iostream>
#include <fstream>
#include <sstream> 
#include <boost/algorithm/string.hpp>
using namespace std;
Config::Config() {    
  //L1_latency = 1;
  //L1_assoc = 8;
  //L1_linesize = 64;
  instr_latency[I_ADDSUB] = 1;
  instr_latency[I_MULT] = 3;
  instr_latency[I_DIV] = 26;
  instr_latency[I_REM] = 1;
  instr_latency[FP_ADDSUB] = 1;
  instr_latency[FP_MULT] = 3;
  instr_latency[FP_DIV] = 26;
  instr_latency[FP_REM] = 1;
  instr_latency[LOGICAL] = 1;
  instr_latency[CAST] = 1;
  instr_latency[GEP] = 1;
  instr_latency[LD] = -1;
  instr_latency[ST] = 1;
  instr_latency[TERMINATOR] = 1;
  instr_latency[PHI] = 1;     // JLA: should it be 0 ?
  instr_latency[SEND] = 1;
  instr_latency[RECV] = 1;
  instr_latency[STADDR] = 1;
  instr_latency[STVAL] = 1;
  num_units[I_ADDSUB] = -1;
  num_units[I_MULT] =  -1;
  num_units[I_DIV] = -1;
  num_units[I_REM] = -1;
  num_units[FP_ADDSUB] = -1;
  num_units[FP_MULT] = -1;
  num_units[FP_DIV] = -1;
  num_units[FP_REM] = -1;
  num_units[LOGICAL] = -1;
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
  num_units[INVALID] = -1;
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
    L1_size = val;
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
    L1_latency = val;
  case 12:
    L1_assoc = val;
  case 13:
    L1_linesize = val;
  case 14:
    window_size = val;
  case 15:
    issueWidth = val;
  default:
    break;
  }
}
void Config::read(std::string name) {
  string line;
  string last_line;
  ifstream cfile(name);
  int id = 0;
  if (cfile.is_open()) {
    while (getline (cfile,line)) {
      vector<string> s = split(line, ',');
      //getCfg(id, stoi(s.at(0)));
      string param = s.at(1);
      boost::trim(param);
      if(param_map.find(param)==param_map.end()) {
        cout <<"[ERROR] Can't find config mapping of: "  << s.at(1) << endl;
        assert(false);
      }
      id=param_map.at(param);
      getCfg(id, stoi(s.at(0)));
      id++;
    }
  }
  else {
    cout << "[ERROR] Cannot open Config file\n";
    assert(false);
  }
  cfile.close();
  cout << "[INFO] Finished Reading Config File (" << name << ") \n";
}
