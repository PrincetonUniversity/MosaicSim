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
  instr_latency[LD_PROD] = -1; //treat like an actual load
  num_units[I_ADDSUB] = 8;
  num_units[I_MULT] =  1;
  num_units[I_DIV] = 1;
  num_units[I_REM] = 2;
  num_units[FP_ADDSUB] = 1;
  num_units[FP_MULT] = 1;
  num_units[FP_DIV] = 1;
  num_units[FP_REM] = 1;
  num_units[LOGICAL] = 2;
  num_units[CAST] = 2;
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
    break;
  case 12:
    L1_assoc = val;
    break;
  case 13:
    L1_linesize = val;
    break;
  case 14:
    window_size = val;
    break;
  case 15:
    issueWidth = val;
    break;
  case 16:
    consume_size = val;
    break;
  case 17:
    supply_size = val;
    break;
  case 18:
    term_buffer_size = val;
    break;
  case 19:
    desc_latency = val;
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
  cout << "\n---------CONFIGS---------\n";
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
      id=param_map.at(param);
      getCfg(id, stoi(s.at(0)));
      cout << "("<<id<<")"<< " " << param << ": " << stoi(s.at(0)) << endl;
      //id++;
    }
    cout << "----------------------------\n\n";
  }
  else {
    cout << "[ERROR] Cannot open Config file\n";
    assert(false);
  }
  cfile.close();
  cout << "[INFO] Finished Reading Config File (" << name << ") \n";
}
