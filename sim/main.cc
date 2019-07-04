#include <iostream>
#include <string>
#include <chrono>
#include <unistd.h>
#include "sim.h"
#include "tile/Accelerator.h"
#include <limits>

using namespace std;
  
Statistics stat;
Config cfg;

class Core;

int main(int argc, char const *argv[]) {
    
  string pythia_home;
  string wlpath;
  string cfgpath;
  string cfgname;
  int num_cores;
  int arg_index=1;
  bool test = false;

  // set pythia home
  
  string f2(argv[0]);
  pythia_home = f2.substr(0, f2.find_last_of( "\\/" )) + "/../";
  
  //cout << pythia_home << " pythiahome \n";
  //assert(false);

  //Luwa: I don't know if this is what causes the temporary crash for not finding the DRAMSim ini file, but I'm going to try using argv[0] for binary path all the time to see if it fixes this problem.
  
  /*
  if(const char *p = std::getenv("PYTHIA_HOME"))
     pythia_home = p;
  else {   // infer the home from the simulator binary's path
   
    char f[1024]; string f2;
     readlink("/proc/self/exe", f, 1024);
     f2 = f;
     pythia_home = f2.substr(0, f2.find_last_of( "\\/" )) + "/../";
    
     }*/

  // set some default parameters
  num_cores = 1;
  cfgpath = pythia_home+"sim/config/";
  cfgname = "sim_default.txt";

  if(argc == 1)           // if no arguments provided then use default config files (or we could print an usage help text)
    test = true;
  else if(argc == 2) {    // next argument must be -n, but if not followed by a number then abort
    cout << "[ERROR] number of cores not properly set.\n";
    assert(false);
  }
  else if(argc >= 3) {
    string in(argv[arg_index++]);    // the next paramenter -n indicates the number of cores
    assert(in == "-n");
    num_cores = stoi(argv[arg_index++]);
    cfgname = argv[arg_index++];     // next paramenter is the GLOBAL config file (we assume that it already includes the path)
    cfgpath = "";
  }
  // read global configuration
  cfg.read(cfgpath+cfgname);
  cfg.verbLevel = -1;

  Simulator* simulator=new Simulator(pythia_home);
  simulator->init_time=chrono::high_resolution_clock::now();

  if(test)
    simulator->registerCore(pythia_home+"workloads/test/output", cfgpath, "core_inorder.txt", 0);
  else {
    // check the workload path and the local config is provided 
    if(argc < 4) {
       cout << "[ERROR] workload path or core's configuration not provided.\n";
       assert(false);
    }
    //register the core tiles
    cout << "[SIM] NumCores is: " << num_cores << endl;
    for (int i=0; i<num_cores; i++) {
      string wlpath(argv[arg_index++]);   // next paramenter is the WORKLOAD path
      string cfgname(argv[arg_index++]);  // next paramenter is the core configfile (we assume that it already includes the path)

      cfgpath="";
      simulator->registerCore(wlpath, cfgpath, cfgname, i);

    }
    // do a while loop here checking for addition command line parameters: verbosity, decoupling, output and debug
    while(argc > arg_index) {
      string curr_arg(argv[arg_index++]);
      if(curr_arg == "-v") {
        cfg.verbLevel = 2;
        cout << "[SIM] Verbose Output level: " << cfg.verbLevel << "\n";
      }
      else if(curr_arg == "-d") {
        simulator->decoupling_mode=true;        
      }
      else if(curr_arg == "-o") {
         simulator->outputDir=argv[arg_index++];
         simulator->outputDir+="/";
      }
      else if(curr_arg == "-debug") {
        simulator->debug_mode=true;        
      }
    }

    //register the acccelerator tiles
    Tile* tile = new Accelerator(simulator,cfg.chip_freq);
    tile->name="accelerator";
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
