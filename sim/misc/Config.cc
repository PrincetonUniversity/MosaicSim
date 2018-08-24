#include "Config.h"
#include "../graph/Graph.h"
Config::Config() {    
  L1_latency = 1;
  L1_assoc = 8;
  L1_linesize = 64;
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
}