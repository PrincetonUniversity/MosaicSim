#include <iostream>
#include <string>
#include "sim.h"
using namespace std;
  
Statistics stat;
Config cfg;

int main(int argc, char const *argv[]) {
  string wlpath;
  string cfgpath;
  string cfgname;
  int num_cores;
  bool test = false;
  if(argc == 1) {
    test = true;
    num_cores = 1;
    cfgname = "default";
  }
  else {
    string in(argv[1]);
    assert(in == "-n");
    num_cores = stoi(argv[2]);
    cfgname = argv[3];
  }
  
  cfgpath = "../sim/config/" + cfgname+".txt";
  cfg.read(cfgpath);
  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";
  string cname = wlpath + "/output/ctrl.txt";
  Simulator* simulator=new Simulator();
  
  if(test) {
    simulator->registerCore("../workloads/test", 0);
  }
  else {
    int arg_index=4;
    for (int i=0; i<num_cores; i++) {
      string wlpath(argv[arg_index]);
      simulator->registerCore(wlpath, i);
      arg_index++;
    }
    cfg.verbLevel = -1;
    if(argc > arg_index) {
      string verbosity(argv[arg_index]);
      if(verbosity == "-v")
        cfg.verbLevel = 2;
    }
  }  
  simulator->run();
  return 0;
} 
