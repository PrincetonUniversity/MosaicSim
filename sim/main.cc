#include <iostream>
#include <string>
#include "sim.h"
#include "misc/reader.h"
#include "graph/GraphOpt.h"

using namespace std;
  
Statistics stat;
Config cfg;

int dig_main(char const *argv[]) {
   
  cfg.verbLevel = 0;
  int num_cores = stoi(argv[2]);
  string cfgname(argv[3]);
  string cfgpath = "../sim/config/" + cfgname  +".txt";
  Reader r;    
  r.readCfg(cfgpath);
    
  Digestor* digestor=new Digestor();
  
  int arg_index=4;
  for (int i=0; i<num_cores; i++) {
    string name;
    if (i%2==0)
      name="Supply";
    else
      name="Compute";
    Simulator* sim = new Simulator();
    sim->name=name;
    string wlpath(argv[arg_index]);
    string cname = wlpath + "/output/ctrl.txt";     
    cout << cfgname << endl;
    string gname = wlpath + "/output/graphOutput.txt";
    string mname = wlpath + "/output/mem.txt";   
    
    r.readGraph(gname, sim->g);
    r.readProfMemory(mname , sim->memory);
    r.readProfCF(cname, sim->cf);
    
    GraphOpt opt(sim->g);
    opt.inductionOptimization();
    cout << "[5] Initialization Complete \n";
    
    sim->initialize();
    sim->digestor=digestor;
    sim->has_digestor=true;
    sim->cache=digestor->cache;
    sim->memInterface=digestor->memInterface;
    sim->intercon=digestor->intercon;    
    digestor->all_sims.push_back(sim);
    arg_index++;
  }
  
  digestor->run();
  return 0;
}

int main(int argc, char const *argv[]) {
  Simulator sim;
  string wlpath;
  string cfgpath;
  // set workload path
  if (argc >=2) {    
    string in(argv[1]);
    if(in=="-d")
      return dig_main(argv);
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
