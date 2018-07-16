#include "header.h"
using namespace std;
  
Statistics stat;
Config cfg;

int main(int argc, char const *argv[]) {
  Simulator sim;
  string wlpath;
  string cfgpath;
  // set workload path
  if (argc >=2) {
    string in(argv[1]);
    wlpath = in;
  }
  else {
    wlpath = "../workloads/test/";
  }
  // set config file path
  if(argc >= 3) {
    string cfgname(argv[2]);
    cfgpath = "../sim/config/" + cfgname  +".txt";
  }
  else 
    cfgpath = "../sim/config/default.txt";

  string gname = wlpath + "/output/graphOutput.txt";
  string mname = wlpath + "/output/mem.txt";
  string cname = wlpath + "/output/ctrl.txt";
  
  // enable verbosity level: check for -v in command line
  cfg.verbLevel = -1;
  if (argc == 4) {
    string v(argv[3]);
    if (v == "-v")
       cfg.verbLevel = 2;
  }

  Reader r; 
  r.readCfg(cfgpath);
  r.readGraph(gname, sim.g);
  r.readProfMemory(mname , sim.memory);
  r.readProfCF(cname, sim.cf);
  
  GraphOpt opt(sim.g);
  opt.inductionOptimization();
  cout << "[5] Initialization Complete \n";
  sim.initialize();
  sim.run();
  return 0;
} 
