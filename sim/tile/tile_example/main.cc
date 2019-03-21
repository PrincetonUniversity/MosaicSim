#include <iostream>
#include <string>
#include "sim.h"
#include <chrono>

#include "tile/ExampleTile.cc"

using namespace std;
  
Statistics stat;
Config cfg;

class Core;

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
  Simulator* simulator=new Simulator();
  simulator->init_time=chrono::high_resolution_clock::now();

  if(test) {
    simulator->registerCore("../workloads/test", cfgname, 0);
  }
  else {
    int arg_index=4;
    //register the core tiles, they're treated specially
    cout << "numcores is" << num_cores << endl;
    for (int i=0; i<num_cores; i++) {
      string wlpath(argv[arg_index]);
      arg_index++;
      string cfgname(argv[arg_index]);
      simulator->registerCore(wlpath, cfgname, i);
      arg_index++;
    }
    cfg.verbLevel = -1;
    if(argc > arg_index) {
      string verbosity(argv[arg_index]);
      if(verbosity == "-v")
        cfg.verbLevel = 2;
      cout << "[Sim] Verbose Output \n";
    }

    //IMult accelerator tile clocked at 3,333 MHz, while core tile clocked at 2000MHz
    Tile* tile1 = new ExampleTile(simulator,2000); 
    
    simulator->registerTile(tile1,1);
    
    Tile* tile2 = new ExampleTile(simulator,2000); 
        
    simulator->registerTile(tile2,2);
    
    /********
    register the other tiles here
    e.g.
    Tile* tile = new ExampleTile(simulator,2000);
    
    simulator->registerTile(tile,1);

    Tile* tile = new Core(simulator,2000000000); //"cast" as a tile
    
    simulator->registerTile(tile); //get assigned tile id
    or
    simulator->registerTile(tile, tid); //pick tile id, but unique from other already assigned ones, starting from num_cores
    *********/
    
  }  
  simulator->run();
  return 0;
} 
