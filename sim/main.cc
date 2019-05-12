#include <iostream>
#include <string>
#include "sim.h"
#include <chrono>
#include "tile/Accelerator.h"

using namespace std;
  
Statistics stat;
Config cfg;

class Core;

int main(int argc, char const *argv[]) {

  string filePath = argv[0];
  
  size_t slash = filePath.find_last_of("/");
  
  string dirPath = (slash != std::string::npos) ? filePath.substr(0, slash) : filePath;
  string pythia_home= dirPath+"/..";
    
  string wlpath;
  string cfgpath;
  string cfgname;
  int num_cores;
  int arg_index=1;
  bool test = false;
  if(argc == 1) {
    test = true;
    num_cores = 1;
    cfgname = "default";
    cout << "not enough args" << endl;
    assert(false);
  }
  else {
    string in(argv[arg_index++]);
    assert(in == "-n");
    num_cores = stoi(argv[arg_index++]);
    cfgname = argv[arg_index++];
  }

  cfgpath = cfgname;
  cfg.read(cfgpath);

  Simulator* simulator=new Simulator(pythia_home);
   
  simulator->init_time=chrono::high_resolution_clock::now();

  if(test) {
    simulator->registerCore("../workloads/test/output", cfgname, 0);    
  }
  else {
    //register the core tiles
    cout << "numcores is" << num_cores << endl;
    for (int i=0; i<num_cores; i++) {
      string wlpath(argv[arg_index++]);
      string cfgname(argv[arg_index++]);
      simulator->registerCore(wlpath, cfgname, i);
    }
    cfg.verbLevel = -1;
    if(argc > arg_index) {
      string verbosity(argv[arg_index]);
      if(verbosity == "-v")
        cfg.verbLevel = 2;
      cout << "[Sim] Verbose Output \n";
    }

    Tile* tile = new Accelerator(simulator,2000);
   
    simulator->registerTile(tile);
    
    /********
    register the other tiles here
    e.g.
    Tile* tile = new ExampleTile(simulator,2000);
    
    simulator->registerTile(tile,1);

    Tile* tile = new Core(simulator,2000); //"cast" as a tile
    
    simulator->registerTile(tile); //get assigned tile id
    or
    simulator->registerTile(tile, tid); //pick tile id, but unique from other already assigned ones, starting from num_cores
    *********/        
    
  }

  //set the DRAMSim clockspeed based on Tile0's clockspeed, assume tile 0 is a core
  
  simulator->initDRAM(simulator->tiles[0]->clockspeed);    
  

  
  simulator->run();
  return 0;
} 
