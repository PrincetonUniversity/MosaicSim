#include "header.h"
using namespace std;
  
Statistics stat;
Config cfg;

int dig_main(char const *argv[]) {
  vector<Simulator*> sims;
  vector<Simulator*> all_sims;
  Interconnect* intercon = new Interconnect();
  
  cfg.verbLevel = 0;
  int num_cores = stoi(argv[2]);

  string cfgname(argv[3]);
  string cfgpath = "../sim/config/" + cfgname  +".txt";
  Reader r;    
  r.readCfg(cfgpath);

  Cache* cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
  DRAMSimInterface* memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->memInterface = memInterface;
  
  int arg_index=4;
  for (int i=0; i<num_cores; i++) {
    string name;
    if (i==0)
      name="Supply";
    else
      name="Compute";
    Simulator* sim = new Simulator(name);
    sims.push_back(sim);
    all_sims.push_back(sim);
    string wlpath(argv[arg_index]);
    string cname = wlpath + "/output/ctrl.txt";   
  
    cout << cfgname << endl;
    string gname = wlpath + "/output/graphOutput.txt";
    string mname = wlpath + "/output/mem.txt";   
    
    //luwahere: there should be 1 config
    r.readGraph(gname, sim->g);
    r.readProfMemory(mname , sim->memory);
    r.readProfCF(cname, sim->cf);
    
    GraphOpt opt(sim->g);
    opt.inductionOptimization();
    cout << "[5] Initialization Complete \n";
    
    sim->initialize(cache, memInterface, intercon);
    arg_index++;
  }
  
  bool simulate=true;  
  while(simulate) {
    //cout << "Cycle count: " <<  cyclecount;
    
    vector<Simulator*> next_sims;
    simulate=false;
    for (auto it=sims.begin(); it!=sims.end(); ++it) {
      Simulator* sim=*it;
      cout << sim->name << endl;
      //cout << "Cycles: " << stat.get("cycles") << endl;
      if(sim->process_cycle()) {
        next_sims.push_back(sim);
        simulate=true;
      }
      else {        
        stat.set("cycles", sim->cycles);                
      }
    }
    intercon->cycles++;
    sims=next_sims;
    next_sims.clear();
  }

  for (auto it=all_sims.begin(); it!=all_sims.end(); it++) {
    Simulator* sim=*it;
    sim->printActivity(); 
  }

  cout << "----------------GLOBAL STATS--------------\n";
  stat.print();
  //cout << "-------------MEM DATA---------------\n";
  memInterface->mem->printStats(true);
  return 0;
}

int main(int argc, char const *argv[]) {
  Simulator sim;
  string wlpath;
  string cfgpath;
  Interconnect* intercon = new Interconnect();
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
  Cache* cache = new Cache(cfg.L1_latency, cfg.L1_size, cfg.L1_assoc, cfg.L1_linesize, cfg.ideal_cache);
  DRAMSimInterface* memInterface = new DRAMSimInterface(cache, cfg.ideal_cache, cfg.mem_load_ports, cfg.mem_store_ports);
  cache->memInterface = memInterface;

  sim.initialize(cache, memInterface, intercon);
  sim.run();
  return 0;
} 
