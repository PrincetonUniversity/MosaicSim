#include <iostream>
#include <string>
#include "sim.h"
using namespace std;
  
Statistics stat;
Config cfg;

int main(int argc, char const *argv[]) {
  string wlpath;
  string cfgpath;
  string in(argv[1]);
  assert(argc >= 4);
  assert(in == "-n");
  int num_cores = stoi(argv[2]);
  string cfgname(argv[3]);
  cfgpath = "../sim/config/" + cfgname+".txt";
  cfg.read(cfgpath);
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";
  string cname = wlpath + "/output/ctrl.txt";
  Simulator* simulator=new Simulator();
  
  int arg_index=4;
  for (int i=0; i<num_cores; i++) {
    string wlpath(argv[arg_index]);
    simulator->registerCore(wlpath);
    arg_index++;
  }
  cfg.verbLevel = -1;
  if(argc > arg_index) {
    string verbosity(argv[arg_index]);
    cfg.verbLevel = 2;
  }
  simulator->run();
  return 0;
} 
